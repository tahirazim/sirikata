if (typeof(std.persist) === 'undefined')
    throw 'Error.  Do not import this file directly.  Only import it from persistService.em';



/**
 Reads in a filename as a string.  Calls deserialize on that string to
 get back an object.  Runs through that object graph, and uses
 nameService to restore it.

 @param {String} name of file to read a serialized object graph in
 from.
 */
function restoreFrom(filename)
{
    var serializedGraph = system.__debugFileRead(filename);
    var deserializedGraph = system.deserialize(serializedGraph);

    //system.prettyprint(deserializedGraph);
    // return deserializedGraph;

    // system.print('\n\nDEBUG2\n');
    // system.prettyprint(deserializedGraph);
    // system.print('\n\n---2\n');
    
    
    //performs recursive restoration.
    var ptrsToFix = [];
    fixReferences(deserializedGraph,ptrsToFix);
    performPtrFinalFixups(ptrsToFix);
}

/**
 @param rootToFix An object that is in serialized form.
 @param {array} ptrsToFix Each record of array corresponds to one
 fixup operation to perform with pointers at end of deserialization.
 (A fixup operation occurs when we encounter an object pointer type in
 the serialized tree.)  Perform all these fixups at end in
 performPtrFinalFixups.
 
 Takes serialized object and changes an entities present state to
 reflect updates.
 */
function fixReferences(rootToFix, ptrsToFix)
{
    var returner = { };

    switch(std.persist.checkObjectSerial(rootToFix))
    {
        case std.persist.NOT_OBJECT:
        throw 'Error in fixReferences of restore.  Should only be fixing references for an object';
        break;

        case std.persist.OBJECT_SERIAL:
        returner = fixupObjectSerial(rootToFix,ptrsToFix);
        break;

        case std.persist.OBJECT_POINTER_SERIAL:
        throw 'Error.  Should not directly receive an object pointer in fixReferences.';
        break;

        case std.persist.SPECIAL_OBJECT:
        returner = fixupSpecialObject(rootToFix,ptrsToFix);
        break;

        default:
        throw 'Error in fixReferences of restore.em.  Unknown return value from std.persist.checkObjectSerial.';
    }

    return returner;
}

/**
 @param A basic object in serialized form (ie it isn't a "special"
 object [such as system, presence, etc], and it isn't a pointer to
 another object.
 */
function fixupObjectSerial(objToFix,ptrsToFix)
{
    system.print('\n\ndebug: In objectserial\n\n');
    system.prettyprint(objToFix);
    
    if (!(std.persist.ID_FIELD_STRING in objToFix))
        throw 'Error in fixupObjectSerial.  Do not have id associated with passed in argument.';      


    var objID = objToFix[std.persist.ID_FIELD_STRING];

    var localCopy =  nameService.lookupObject(objID);

    if (localCopy == nameService.DNE)
        throw 'Error in fixupObjectSerial.  Object should exist in the name service.';


    //temporarily copy old fields of local object to separate object.
    //Then delete them from local object
    var oldLocalFields = { };
    for (var s in localCopy)
        oldLocalFields[s] = localCopy[s];

    for (var s in oldLocalFields)
        delete localCopy[s];

    
    for (var s in objToFix)
    {
        //ignore the serialization metadata when deserializing the object
        if ((s == std.persist.ID_FIELD_STRING) || (s == std.persist.TYPE_FIELD_STRING) || (s == std.persist.NO_RESTORE_STRING) || (s == std.persist.POINTER_FIELD_STRING) || (s== 'prototype') || (s == std.persist.NON_RESTORE_MAX_NOT_PERSISTED))
            continue;
        
        var fieldValue = std.persist.getValueFromPropValPair(objToFix[s]);
        var indexValue = std.persist.getPropFromPropValPair (objToFix[s]);


        system.print('\nPrinting field value\n');
        system.prettyprint();

        
        if (std.persist.checkObjectSerial(fieldValue) == std.persist.OBJECT_POINTER_SERIAL)
        {
            system.print('\n\nDEBUG: got an object pointer\n\n');
            //register the field, s, of localCopy to be fixed up.
            registerFixupObjectPointer(fieldValue,indexValue, localCopy,ptrsToFix);                  
        }
        else if ((typeof (fieldValue) == "object") &&(fieldValue != null))
        {
            //finish copying data for remaining references
            var newObj = fixReferences(fieldValue,ptrsToFix);     
            //in case scripter had deleted field associated with this object....this will re-populate
            localCopy[indexValue] = newObj;
        }
        else
        {
            //copy value types to local
            localCopy[indexValue] = fieldValue;
        }
    }
    return localCopy;
}





/**
 @param object pointer to point localCopyToPointTo
 @param index of the local object to perform fixup on.
 @param {object} localCopyToPoint localCopyToPoint[index] should be the new
 object pointer
 @param {array} allToFix an array.  Push the above three arguments onto that
 array (as an array).  After rest of deserialization occurs, final
 step is to call performFinalFixups with this array as an argument.
 performPtrFinalFixups traverses array, and points localCopyToPoint[index]
 to the objects described by objPtrToFix.
 */
function registerFixupObjectPointer(objPtrToFix, index, localCopyToPoint, allToFix)
{
    allToFix.push([localCopyToPoint, objPtrToFix,index]);
}

/**
 @param {array} allToFix array.  Each element has three values. [0]
 localObjToPoint, [1] objPtrToFix, [2] index.  

 Runs through array generated by calls to registerFixupObjectPointer,
 and individually performs fixups for each one.

 @see registerFixupObjectPointer
 @see fixSinglePtr
 */
function performPtrFinalFixups(allToFix)
{
    for (var s in allToFix)
        fixSinglePtr(allToFix[s][0],allToFix[s][1],allToFix[s][2]);
}

/**
 localObjToPoint[index] should end up pointing to the object represented
 by objPtrToFix.

 @see registerFixupObjectPointer
 @see fixSinglePtr
 */
function fixSinglePtr(localObjToPoint, objPtrToFix,index)
{
    if ((typeof(objPtrToFix) != 'object') || (objPtrToFix == null))
        throw 'Error in fixSinglePtr.  Should not have received non-obect or null ptr record.';

    if (std.persist.checkObjectSerial(objPtrToFix) != std.persist.OBJECT_POINTER_SERIAL)
        throw 'Error in fixSinglePtr.  Should not have received non-pointer record to point to.';

    //lookup local object
    var localToPointTo = nameService.lookupObject(objPtrToFix[std.persist.POINTER_FIELD_STRING]);
    if (localToPointTo == nameService.DNE)
        throw 'Error in fixSinglePtr.  Have no record of object with name: ' + objPtrToFix[std.persist.POINTER_FIELD_STRING].toString();
    
    //perform fix    
    localObjToPoint[index] = localToPointTo;
}


/**
 */
function fixupSpecialObject(rootToFix,ptrsToFix)
{
   throw 'Error.  Not currently handling cases of deserializing special objects (eg. timers, visibles, etc.)' ;
}