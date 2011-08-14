
#include "JS_JSMessage.pbj.hpp"
#include "JSSerializer.hpp"
#include <string>
#include "JSUtil.hpp"
#include "JSObjects/JSFields.hpp"
#include "JSObjects/JSVisible.hpp"
#include "JSObjectStructs/JSVisibleStruct.hpp"
#include "JSObjectStructs/JSSystemStruct.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "JSObjectScript.hpp"
#include "EmersonScript.hpp"
#include "JSLogging.hpp"
#include "JSObjects/JSObjectsUtils.hpp"

/*
  FIXME: If I do not include the JS_Sirikata.pbj.hpp, then just including the
  sirikata/core/util/routablemessagebody.hpp file will cause things to blow up
  (not even using anything in that file, just including it).
 */


#include <v8.h>

namespace Sirikata{
namespace JS{


void annotateMessage(Sirikata::JS::Protocol::JSMessage& toAnnotate,int32 &toStampWith)
{
    toAnnotate.set_msg_id(toStampWith);
    ++toStampWith;
}
void annotateMessage(Sirikata::JS::Protocol::IJSMessage& toAnnotate, int32&toStampWith)
{
    toAnnotate.set_msg_id(toStampWith);
    ++toStampWith;
}


//Runs through all the objects in the object vector toUnmark.  For each of these
//objects, calls DeleteHiddenValue on each of them.
void JSSerializer::unmarkSerialized(ObjectVec& toUnmark)
{
    v8::HandleScope handle_scope;
    v8::Handle<v8::String> hiddenFieldName = v8::String::New(JSSERIALIZER_TOKEN_FIELD_NAME);
    for (ObjectVecIter iter = toUnmark.begin();  iter != toUnmark.end(); ++iter)
    {
        if ((*iter).IsEmpty())
        {
            JSLOG(error, "Error in unmarkSerialized.  Got an empty value to unmark.");
            continue;
        }

        v8::Local<v8::Value> hiddenVal = (*iter)->GetHiddenValue(hiddenFieldName);
        if (hiddenVal.IsEmpty())
            JSLOG(error, "Error in unmarkSerialized.  All values in vector should have a hidden value.");
        else
            (*iter)->DeleteHiddenValue(hiddenFieldName);
    }
}


void JSSerializer::unmarkDeserialized(ObjectMap& objMap)
{
    v8::Handle<v8::String> hiddenFieldName = v8::String::New(JSSERIALIZER_ROOT_OBJ_TOKEN);
    
    for (ObjectMapIter mapIter = objMap.begin(); mapIter != objMap.end();
         ++mapIter)
    {
        if (!mapIter->second->GetHiddenValue(hiddenFieldName).IsEmpty())
            mapIter->second->DeleteHiddenValue(hiddenFieldName);
    }
}



//Points the jsf_value to int32ToPointTo
void JSSerializer::pointOtherObject(int32 int32ToPointTo,Sirikata::JS::Protocol::IJSFieldValue& jsf_value)
{
    jsf_value.set_loop_pointer(int32ToPointTo);
}



void JSSerializer::serializeSystem(v8::Local<v8::Object> jsSystem, Sirikata::JS::Protocol::IJSMessage& jsmessage,int32& toStampWith,ObjectVec& allObjs)
{
    std::string err_msg;

    annotateMessage(jsmessage,toStampWith);

    JSSystemStruct* sys_struct = JSSystemStruct::decodeSystemStruct(jsSystem, err_msg);
    if(err_msg.size() > 0) {
        SILOG(js, error, "Could not decode system in JSSerializer::serializeSystem: "+ err_msg );
        return;
    }

    Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();
    jsf.set_name(TYPEID_FIELD_NAME);
    Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
    jsf_value.set_s_value(SYSTEM_TYPEID_STRING);
}


void JSSerializer::serializeVisible(v8::Local<v8::Object> jsVisible, Sirikata::JS::Protocol::IJSMessage&jsmessage,int32& toStampWith,ObjectVec& allObjs)
{
  std::string err_msg;
  annotateMessage(jsmessage,toStampWith);

  JSVisibleStruct* vstruct = JSVisibleStruct::decodeVisible(jsVisible, err_msg);
  if(err_msg.size() > 0)
  {
    SILOG(js, error, "Could not decode Visible in JSSerializer::serializeVisible: "+ err_msg );
    return ;
  }

  EmersonScript* emerScript         =      vstruct->jpp->emerScript;
  SpaceObjectReference sporef       =      vstruct->getSporef();

  fillVisible(jsmessage, sporef);
}


void JSSerializer::fillVisible(Sirikata::JS::Protocol::IJSMessage& jsmessage, const SpaceObjectReference& listenTo)
{
  // serialize SpaceObjectReference
  Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();
  jsf.set_name(TYPEID_FIELD_NAME);
  Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
  jsf_value.set_s_value(VISIBLE_TYPEID_STRING);

  jsf = jsmessage.add_fields();
  jsf.set_name(VISIBLE_SPACEOBJREF_STRING);
  jsf_value = jsf.mutable_value();
  jsf_value.set_s_value(listenTo.toString());

}

void JSSerializer::serializePresence(v8::Local<v8::Object> jsPresence, Sirikata::JS::Protocol::IJSMessage&jsmessage,int32& toStampWith,ObjectVec& allObjs)
{
  std::string err_msg;
  annotateMessage(jsmessage,toStampWith);

  JSPresenceStruct* presStruct = JSPresenceStruct::decodePresenceStruct(jsPresence, err_msg);
  if(err_msg.size() > 0)
  {
    SILOG(js, error, "Could not decode Presence in JSSerializer::serializePresence: "+ err_msg );
    return;
  }
  fillVisible(jsmessage, presStruct->getSporef());
}

std::string JSSerializer::serializeObject(v8::Local<v8::Value> v8Val,int32 toStampWith)
{
  ObjectVec allObjs;
  Sirikata::JS::Protocol::JSMessage jsmessage;

  v8::HandleScope handleScope;

  annotateMessage(jsmessage,toStampWith);

  if(v8Val->IsObject())
      serializeObjectInternal(v8Val, jsmessage,toStampWith,allObjs);


  std::string serialized_message;
  jsmessage.SerializeToString(&serialized_message);

  std::cout<<"\n\nDEBUGGING\n";
  debug_printSerialized(jsmessage,"debug");
  std::cout<<"\n\n";
  
  unmarkSerialized(allObjs);
  return serialized_message;
}


void debug_printSerialized(Sirikata::JS::Protocol::JSMessage jm, String prepend)
{
    std::cout<<prepend<<":id: "<<jm.msg_id()<<"\n";
    if (jm.has_f_value())
        std::cout<<prepend<<":functext: "<<jm.f_value()<<"\n";
    //printing breadth first:
    for (int s = 0; s < jm.fields_size(); ++s)
    {
        std::cout<<prepend<<":"<<jm.fields(s).name();
        std::cout<<"\n";
        if (jm.fields(s).has_value())
        {
            if (jm.fields(s).value().has_o_value())
                debug_printSerialized(jm.fields(s).value().o_value(), prepend+ ":"+ jm.fields(s).name()  );
            if (jm.fields(s).value().has_a_value())
                debug_printSerialized(jm.fields(s).value().a_value(), prepend + ":" + jm.fields(s).name());
            if (jm.fields(s).value().has_root_object())
                debug_printSerialized(jm.fields(s).value().root_object(), prepend + ":" + jm.fields(s).name());
            
            if (jm.fields(s).value().has_s_value())
                std::cout<<" s_val:     "<<jm.fields(s).value().s_value()<<"\n";
            if (jm.fields(s).value().has_i_value())
                std::cout<<" i_value:   "<<jm.fields(s).value().i_value()<<"\n";
            if (jm.fields(s).value().has_b_value())
                std::cout<<" b_value:   "<<jm.fields(s).value().b_value()<<"\n";
            if (jm.fields(s).value().has_ui_value())
                std::cout<<" ui_value:  "<<jm.fields(s).value().ui_value()<<"\n";
            if (jm.fields(s).value().has_d_value())
                std::cout<<" d_value:   "<<jm.fields(s).value().d_value()<<"\n";
            if (jm.fields(s).value().has_loop_pointer())
                std::cout<<" loop:      "<<jm.fields(s).value().loop_pointer()<<"\n";
        }
    }
}


std::vector<String> getPropertyNames(v8::Handle<v8::Object> obj) {
    std::vector<String> results;
    
    v8::Local<v8::Array> properties = obj->GetPropertyNames();
    for(uint32 i = 0; i < properties->Length(); i++) {
        v8::Local<v8::Value> prop_name = properties->Get(i);
        INLINE_STR_CONV(prop_name,tmpStr, "error decoding string in getPropertyNames");
        results.push_back(tmpStr);
    }

    return results;
}

// For some reason, V8 isn't providing this, so we have to be able to figure out
// the correct list ourselves.
std::vector<String> getOwnPropertyNames(v8::Local<v8::Object> obj) {
    std::vector<String> all_props = getPropertyNames(obj);

    if (obj->GetPrototype()->IsUndefined() || obj->GetPrototype()->IsNull())
        return all_props;

    std::vector<String> results;
    std::vector<String> prototype_props = getPropertyNames(v8::Local<v8::Object>::Cast(obj->GetPrototype()));

    // //necessary because getPropertyNames won't return "prototype" for functions.
    if (obj->Has(v8::String::New("prototype")))
        results.push_back("prototype");

    
    v8::Handle<v8::Object> protoObj = v8::Local<v8::Object>::Cast(obj->GetPrototype());
    for(std::vector<String>::size_type i = 0; i < all_props.size(); i++)
    {
        std::vector<String>::iterator propFindIter =
            std::find(prototype_props.begin(), prototype_props.end(), all_props[i]);

        //this property exists only on the object.
        if (propFindIter == prototype_props.end())
            results.push_back(all_props[i]);
        else
        {
            v8::Handle<v8::String> fieldName = v8::String::New(propFindIter->c_str(), propFindIter->size());
            //this property exists on the object and its prototype.  Must check
            //if the two properties are equal.  If they are, then don't add to
            //results.  If they are not equal, then add to results.
            if (! obj->Get(fieldName)->Equals(protoObj->Get(fieldName)))
                results.push_back(all_props[i]);
        }
    }

    if (obj->Has(v8::String::New("constructor")))
        results.push_back("constructor");

    
    results.push_back(JSSERIALIZER_PROTOTYPE_NAME);
    return results;
}


void JSSerializer::annotateObject(ObjectVec& objVec, v8::Handle<v8::Object> v8Obj,int32 toStampWith)
{
    //don't annotate any of the special objects that we have.  Can't form loops
    v8Obj->SetHiddenValue(v8::String::New(JSSERIALIZER_TOKEN_FIELD_NAME), v8::Int32::New(toStampWith));
    objVec.push_back(v8Obj);
}


void JSSerializer::serializeObjectInternal(v8::Local<v8::Value> v8Val, Sirikata::JS::Protocol::IJSMessage& jsmessage,int32 & toStampWith,ObjectVec& objVec)
{
    //otherwise assuming it is a v8 object for now
    v8::Local<v8::Object> v8Obj = v8Val->ToObject();

    //stamps message
    annotateObject(objVec,v8Obj,toStampWith);
    annotateMessage(jsmessage,toStampWith);

    //if the object is a function, save its function text separately
    if (v8Obj->IsFunction())
    {
        v8::Local<v8::Value> funcTextValue = v8::Handle<v8::Function>::Cast(v8Obj)->ToString();
        INLINE_STR_CONV(funcTextValue, funcTextStr, "error decoding string when serializing function.");
        jsmessage.set_f_value(funcTextStr);
        if (funcTextStr == FUNCTION_CONSTRUCTOR_TEXT)
            return;
    }

    
    if(v8Obj->InternalFieldCount() > 0)
    {
        v8::Local<v8::Value> typeidVal = v8Obj->GetInternalField(TYPEID_FIELD);
        if (!typeidVal.IsEmpty())
        {
            if(!typeidVal->IsNull() && !typeidVal->IsUndefined())
            {
                v8::Local<v8::External> wrapped  = v8::Local<v8::External>::Cast(typeidVal);
                void* ptr = wrapped->Value();
                std::string* typeId = static_cast<std::string*>(ptr);
                if(typeId == NULL) return;

                std::string typeIdString = *typeId;
                if (typeIdString == VISIBLE_TYPEID_STRING)
                {
                    serializeVisible(v8Obj, jsmessage,toStampWith,objVec);
                }
                else if (typeIdString == SYSTEM_TYPEID_STRING)
                {
                    serializeSystem(v8Obj, jsmessage,toStampWith,objVec);
                }
                else if (typeIdString == PRESENCE_TYPEID_STRING)
                {
                    serializePresence(v8Obj, jsmessage,toStampWith,objVec);
                }
                return;
            }
        }
    }

    bool wantBeforeAndAfter = false;

    std::vector<String> properties = getOwnPropertyNames(v8Obj);
    for( unsigned int i = 0; i < properties.size(); i++)
    {
        String prop_name = properties[i];
        
        v8::Local<v8::Value> prop_val;

        if (properties[i] == JSSERIALIZER_PROTOTYPE_NAME)
        {
            prop_val = v8Obj->GetPrototype();
        }
        else
            prop_val = v8Obj->Get( v8::String::New(properties[i].c_str(), properties[i].size()) );


        std::cout<<"\nWorking with property name: "<<prop_name<<"\n";
        if (prop_name == "constructor")
        {
            wantBeforeAndAfter = true;
            std::cout<<"\nTo break on.\n";
        }
        
        /* This is a little gross, but currently necessary. If something is
         * referring to native code, we shouldn't be shipping it. This means we
         * need to detect native code and drop the field. However, v8 doesn't
         * provide a way to check for native code. Instead, we need to check for
         * the { [native code] } definition.
         *
         * This is considered bad because of the way we are filtering instead of
         * detecting native types and . A better approach would work on the
         * whole-object level, detecting special types and converting them to an
         * appropriate form for restoration on the remote side. In some cases
         * this would be seamless (e.g. vec3) and sometimes would require
         * 'simplification' (e.g. Presence).
         */
        if(prop_val->IsFunction())
        {
            v8::Local<v8::Function> v8Func = v8::Local<v8::Function>::Cast(prop_val);
            INLINE_STR_CONV(v8Func->ToString(),cStrMsgBody2, "error decoding string in serializeObjectInternal");

            if ((cStrMsgBody2.find("{ [native code] }") != String::npos) && (cStrMsgBody2 != FUNCTION_CONSTRUCTOR_TEXT))
            {
                std::cout<<"\n\nDEBUG: screened function: "<<cStrMsgBody2<<"\n\n";
                continue;
            }
        }

        
        Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();
        Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();

        // create a JSField out of this
        jsf.set_name(prop_name);


        /* Check if the value is a function, object, bool, etc. */
        if(prop_val->IsFunction())
        {
            v8::Local<v8::Function> v8Func = v8::Local<v8::Function>::Cast(prop_val);
            v8::Local<v8::Value> hiddenValue = v8Func->GetHiddenValue(v8::String::New(JSSERIALIZER_TOKEN_FIELD_NAME));

            if (wantBeforeAndAfter)
            {
                std::cout<<"\n\n";
                debug_printSerialized(jsmessage, "before");
                std::cout<<"\n\n";
            }
            
            if (hiddenValue.IsEmpty())
            {
                //means that we have not already stamped this function object
                //as having been serialized.  need to serialize it now.
                //annotateObject(objVec,v8Func,toStampWith);
                //lkjs;
                Sirikata::JS::Protocol::IJSMessage ijs_m =jsf_value.mutable_o_value();
                serializeObjectInternal(v8Func, ijs_m, toStampWith,objVec);
                //serializeFunctionInternal(v8Func, jsf_value,toStampWith);
                //lkjs;
            }
            else
            {
                //we have already stamped this function object as having been
                //serialized.  Instead of serializing it again, point to
                //the old version.

                if (hiddenValue->IsInt32())
                    pointOtherObject(hiddenValue->ToInt32()->Value(),jsf_value);
                else
                    JSLOG(error,"Error in serialization.  Hidden value was not an int32");
            }

            if (wantBeforeAndAfter)
            {
                std::cout<<"\n\n";
                debug_printSerialized(jsmessage, "after");
                std::cout<<"\n\n";
            }
            
            
        }
        else if(prop_val->IsArray())
        {
            /* If this value is an object , then recursively call the serlialize on this */
            v8::Local<v8::Array> v8Array = v8::Local<v8::Array>::Cast(prop_val);
            v8::Local<v8::Value> hiddenValue = v8Array->GetHiddenValue(v8::String::New(JSSERIALIZER_TOKEN_FIELD_NAME));
            if (hiddenValue.IsEmpty())
            {
                Sirikata::JS::Protocol::IJSMessage ijs_m =jsf_value.mutable_a_value();
                serializeObjectInternal(v8Array, ijs_m, toStampWith,objVec);
            }
            else
            {
                //we have already stamped this function object as having been
                //serialized.  Instead of serializing it again, point to
                //the old version.
                if (hiddenValue->IsInt32())
                    pointOtherObject(hiddenValue->ToInt32()->Value(),jsf_value);
                else
                    JSLOG(error,"Error in serialization.  Hidden value was not an int32");
            }
        }
        else if(prop_val->IsObject())
        {
            v8::Local<v8::Object> v8obj = v8::Local<v8::Object>::Cast(prop_val);
            
            v8::Local<v8::Value> hiddenValue = v8obj->GetHiddenValue(v8::String::New(JSSERIALIZER_TOKEN_FIELD_NAME));

            if (hiddenValue.IsEmpty())
            {
                //check if it's the root object.
                v8::Handle<v8::Object> tmpObj = v8::Object::New();

                if (prop_val->Equals(tmpObj->GetPrototype()))
                {
                    Sirikata::JS::Protocol::IJSMessage ijs_o = jsf_value.mutable_root_object();
                    serializeObjectInternal(v8obj, ijs_o, toStampWith,objVec);;                    
                }
                else
                {
                    Sirikata::JS::Protocol::IJSMessage    ijs_o = jsf_value.mutable_o_value();
                    serializeObjectInternal(v8obj, ijs_o, toStampWith,objVec);;                    
                }
                

            }
            else
            {
                if (hiddenValue->IsInt32())
                    pointOtherObject(hiddenValue->ToInt32()->Value(),jsf_value);
                else
                    JSLOG(error,"Error in serialization.  Hidden value was not an int32");
            }
        }
        else if(prop_val->IsInt32())
        {
            int32_t i_value = prop_val->Int32Value();
            jsf_value.set_i_value(i_value);
        }
        else if(prop_val->IsUint32())
        {
            uint32 ui_value = prop_val->Uint32Value();
            jsf_value.set_ui_value(ui_value);
        }
        else if(prop_val->IsString())
        {
            INLINE_STR_CONV(prop_val,s_value, "error decoding string in serializeObjectInternal");
            jsf_value.set_s_value(s_value);
        }
        else if(prop_val->IsNumber())
        {
            float64 d_value = prop_val->NumberValue();
            jsf_value.set_d_value(d_value);
        }
        else if(prop_val->IsBoolean())
        {
            bool b_value = prop_val->BooleanValue();
            jsf_value.set_b_value(b_value);
        }
        else if(prop_val->IsDate())
        {
        }
        else if(prop_val->IsRegExp())
        {
        }
    }

}

/**
   This function runs through the map of values toFixUp, pointing them to the
   an object in labeledObjs instead.

   If a key for the toFixUp map does not exist as a key in labeledObjs, returns
   false.

   Should be run at the very end of deserializeObjectInternal
 */
bool JSSerializer::deserializePerformFixups(ObjectMap& labeledObjs, FixupMap& toFixUp)
{
    for (FixupMapIter iter = toFixUp.begin(); iter != toFixUp.end(); ++iter)
    {
        int32 objid = iter->first;
        for (LoopedObjPointerList::iterator liter = iter->second.begin(); liter != iter->second.end(); liter++) {
            LoopedObjPointer& objpointer = *liter;
            ObjectMapIter finder = labeledObjs.find(objid);
            if (finder == labeledObjs.end())
            {
                JSLOG(error, "error deserializing object pointing to "<< objid<< ". No record of that label.");
                return false;
            }
            if (objpointer.name != JSSERIALIZER_PROTOTYPE_NAME)
                objpointer.parent->Set(v8::String::New(objpointer.name.c_str(), objpointer.name.size()), finder->second);

            else
            {
                if (!finder->second.IsEmpty() && !finder->second->IsUndefined() && !finder->second->IsNull())
                {
                    //objpointer.parent->SetPrototype(finder->second);
                    setPrototype(objpointer.parent,finder->second);
                }
            }
        }
    }
    return true;
}


void JSSerializer::setPrototype(v8::Handle<v8::Object> toSetProtoOf, v8::Handle<v8::Object> toSetTo)
{
    if (toSetProtoOf->IsFunction())
    {
        v8::Handle<v8::Function> toSetOnFunc = v8::Handle<v8::Function>::Cast(toSetProtoOf);
        INLINE_STR_CONV(toSetOnFunc->ToString(), strFuncBefore,"nope");
        std::cout<<"\n\nBEFORE\n"<<strFuncBefore<<"\n\n";


        if (!toSetTo->GetHiddenValue(v8::String::New(JSSERIALIZER_ROOT_OBJ_TOKEN)).IsEmpty())
        {
            //means that we are pointing to the serialized version of
            //the root object.
            shallowCopyFields(toSetProtoOf, toSetTo);
            return;
        }

            
        if (toSetTo->IsFunction())
        {
            v8::Handle<v8::Function> toSetToFunc = v8::Handle<v8::Function>::Cast(toSetTo);
            toSetOnFunc->SetPrototype(toSetToFunc);
            std::cout<<"\nSetting func\n";
        }
        else
            toSetOnFunc->SetPrototype(toSetTo);

        INLINE_STR_CONV(toSetOnFunc->ToString(), strFuncAfter,"nope");
        std::cout<<"\n\nAfter\n"<<strFuncAfter<<"\n\n";
    }
    else
    {
        if (toSetTo->IsFunction())
        {
            v8::Handle<v8::Function> toSetToFunc = v8::Handle<v8::Function>::Cast(toSetTo);
            toSetProtoOf->SetPrototype(toSetToFunc);
        }
        else
            toSetProtoOf->SetPrototype(toSetTo);
    }
    
}


//Copies all fields from src to dst.  Does not 
void JSSerializer::shallowCopyFields(v8::Handle<v8::Object> dst, v8::Handle<v8::Object> src)
{
    std::vector<String> srcPropNames = getPropertyNames(src);
    std::vector<String> dstPropNames = getPropertyNames(dst);

    for (std::vector<String>::iterator srcIter = srcPropNames.begin();
         srcIter != srcPropNames.end(); ++srcIter)
    {
        if (std::find(dstPropNames.begin(), dstPropNames.end(), *srcIter) ==
            dstPropNames.end())
        {
            //don't have a copy of this property in the dst obj already.  Put
            //one in.
            v8::Handle<v8::String> fieldName = v8::String::New(srcIter->c_str(), srcIter->size());
            dst->Set(fieldName, src->Get(fieldName));
        }
    }
}


v8::Handle<v8::Object> JSSerializer::deserializeObject( EmersonScript* emerScript, Sirikata::JS::Protocol::JSMessage jsmessage,bool& deserializeSuccessful)
{
    v8::HandleScope handle_scope;
    ObjectMap labeledObjs;
    FixupMap  toFixUp;

    v8::Handle<v8::Object> deserializeTo;
    
    //if we're deserializing to a function, we need to know rigth off the bat,
    //and change toDeserializeTo to be a function.
    if (jsmessage.has_f_value())
        deserializeTo = emerScript->functionValue(jsmessage.f_value());
    else
        deserializeTo = v8::Object::New();


    //if can't handle first stage of deserialization, don't even worry about
    //fixups
    labeledObjs[jsmessage.msg_id()] = deserializeTo;
    if (! deserializeObjectInternal(emerScript, jsmessage,deserializeTo, labeledObjs,toFixUp))
        return handle_scope.Close(deserializeTo);
    
    //return whether fixups worked or not.
    deserializeSuccessful = deserializePerformFixups(labeledObjs,toFixUp);

    //unmark deserialized
    unmarkDeserialized(labeledObjs);
    return handle_scope.Close(deserializeTo);
}



bool JSSerializer::deserializeObjectInternal( EmersonScript* emerScript, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Handle<v8::Object>& deserializeTo, ObjectMap& labeledObjs,FixupMap& toFixUp)
{
    //check if there is a typeid field and what is the value for it
    bool isVisible = false;
    bool isSystem = false;
    for(int i = 0; i < jsmessage.fields_size(); i++)
    {
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);
        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();
        if(jsf.name() == TYPEID_FIELD_NAME) {
            if(jsvalue.s_value() == SYSTEM_TYPEID_STRING) {
                isSystem = true;
                break;
            }
            else if(jsvalue.s_value() == VISIBLE_TYPEID_STRING) {
                isVisible = true;
                break;
            }
            else if(jsvalue.s_value() == PRESENCE_TYPEID_STRING) {
                SILOG(js,fatal,"Received presence for deserialization, but they cannot be serialized! This object will likely be restored as an empty, useless object.");
            }
        }
    }

    if (isSystem) {
        static String sysFieldname = "builtin";
        static String sysFieldval = "[object system]";
        v8::Local<v8::String> v8_name = v8::String::New(sysFieldname.c_str(), sysFieldval.size());
        v8::Local<v8::String> v8_val = v8::String::New(sysFieldval.c_str(), sysFieldval.size());
        deserializeTo->Set(v8_name, v8_val);
        return true;
    }

    if(isVisible)
    {
      SpaceObjectReference visibleObj;

      for(int i = 0; i < jsmessage.fields_size(); i++)
      {
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);
        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();

        if(jsf.name() == VISIBLE_SPACEOBJREF_STRING)
          visibleObj = SpaceObjectReference(jsvalue.s_value());

      }

      //error if not in context, won't be able to create a new v8 object.
      //should just abort here before seg faulting.
      if (! v8::Context::InContext())
      {
          JSLOG(error, "Error deserializing visible object.  Am not inside a v8 context.  Aborting.");
          return false;
      }
      v8::Handle<v8::Context> ctx = v8::Context::GetCurrent();

      //create the vis obj through objScript
      deserializeTo = emerScript->createVisiblePersistent(visibleObj, JSProxyPtr(), ctx);
      return true;
    }

    
    for(int i = 0; i < jsmessage.fields_size(); i++)
    {
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);

        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();
        String fieldname = jsf.name();

        
        v8::Local<v8::String> fieldkey = v8::String::New(fieldname.c_str(), fieldname.size());
        v8::Handle<v8::Value> val;

        if(jsvalue.has_s_value())
        {
            String str1 = jsvalue.s_value();
            val = v8::String::New(str1.c_str(), str1.size());
        }
        else if(jsvalue.has_i_value())
        {
            val = v8::Integer::New(jsvalue.i_value());
        }
        else if(jsvalue.has_ui_value())
        {
            val = v8::Integer::NewFromUnsigned(jsvalue.ui_value());
        }
        else if (jsvalue.has_root_object())
        {
            //instead of modifying the root object directly, will write all
            //fields.
            v8::Handle<v8::Object>rootObj = v8::Object::New();
            rootObj->SetHiddenValue(v8::String::New(JSSERIALIZER_ROOT_OBJ_TOKEN),v8::Boolean::New(true));
            Sirikata::JS::Protocol::JSMessage internal_js_message = jsvalue.root_object();
            JSSerializer::deserializeObjectInternal(emerScript, internal_js_message, rootObj,labeledObjs,toFixUp);
            labeledObjs[jsvalue.root_object().msg_id()] = rootObj;
            std::cout<<"\n\nSetting an id for root obj with id: "<<jsvalue.root_object().msg_id()<<"\n\n";
            shallowCopyFields(deserializeTo,rootObj);
            continue;
        }
        
        else if(jsvalue.has_o_value())
        {
            std::cout<<"\n\nDEBUG: This is msg id: "<<jsvalue.o_value().msg_id()<<"\n\n\n";
            //check if what is to be serialized is a function or a plain-old object
            Sirikata::JS::Protocol::JSMessage internal_js_message = jsvalue.o_value();

            //we're dealing with a function
            if (internal_js_message.has_f_value())
            {
                v8::Handle<v8::Function>intFuncObj;
                if (internal_js_message.f_value() == FUNCTION_CONSTRUCTOR_TEXT)
                {
                    std::cout<<"\n\nHas function constructor text\n\n";
                    
                    v8::Local<v8::Function> tmpFun = emerScript->functionValue("function(){}");
                    if ((tmpFun->Has(v8::String::New("constructor"))) &&
                        (tmpFun->Get(v8::String::New("constructor"))->IsFunction()))
                    {
                        std::cout<<"\n\nDEBUG: Got into check\n\n";
                        intFuncObj = v8::Handle<v8::Function>::Cast(tmpFun->Get(v8::String::New("constructor")));
                    }
                    else
                    {
                        JSLOG(error, "Error setting the constructor of an object.  Setting to dummy constructor.");
                        intFuncObj = tmpFun;
                    }
                }
                else
                    intFuncObj = emerScript->functionValue(internal_js_message.f_value());


                v8::Handle<v8::Object> tmpObjer = v8::Handle<v8::Object>::Cast(intFuncObj);
                //lkjs;
//                JSSerializer::deserializeObjectInternal(emerScript,
//                internal_js_message,
//                v8::Handle<v8::Object>::Cast(intFuncObj),labeledObjs,toFixUp);
                JSSerializer::deserializeObjectInternal(emerScript, internal_js_message, tmpObjer,labeledObjs,toFixUp);
                val = intFuncObj;
                labeledObjs[jsvalue.o_value().msg_id()] = intFuncObj;
            }
            else
            {
                v8::Handle<v8::Object>intDesObj = v8::Object::New();
                JSSerializer::deserializeObjectInternal(emerScript, internal_js_message, intDesObj,labeledObjs,toFixUp);
                val = intDesObj;
                labeledObjs[jsvalue.o_value().msg_id()] = intDesObj;
            }


            
        }
        else if(jsvalue.has_a_value())
        {
            v8::Handle<v8::Array> intDesArr = v8::Array::New();
            v8::Handle<v8::Object> intDesObj(intDesArr);
            Sirikata::JS::Protocol::JSMessage internal_js_message = jsvalue.a_value();

            JSSerializer::deserializeObjectInternal(emerScript, internal_js_message, intDesObj,labeledObjs,toFixUp);
            val = intDesObj;
            labeledObjs[jsvalue.a_value().msg_id()] = intDesObj;
        }
        else if(jsvalue.has_d_value())
        {
            val = v8::Number::New(jsvalue.d_value());
        }
        else if(jsvalue.has_b_value())
        {
            val = v8::Boolean::New(jsvalue.b_value());
        }
        else if (jsvalue.has_loop_pointer())
        {
            toFixUp[jsvalue.loop_pointer()].push_back(LoopedObjPointer(deserializeTo,fieldname,jsvalue.loop_pointer()));
            continue;
        }


        
        if (fieldname == JSSERIALIZER_PROTOTYPE_NAME)
        {
            if (!val.IsEmpty() && !val->IsUndefined() && !val->IsNull())
            {
                if (val->IsObject())
                    setPrototype(deserializeTo,val->ToObject());
                else
                    deserializeTo->SetPrototype(val);
            }
            
        }
        else {
            if (!val.IsEmpty())
                deserializeTo->Set(fieldkey, val);
        }
    }
    return true;
}


} //end namespace js
} //end namespace sirikata
