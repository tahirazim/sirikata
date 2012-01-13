(function()
 {
     //if we request the feature object this many times from another
     //visible, and do not receive it, then just fire the prox event
     //for the visible setting featureObject to be undefined.
     var NUM_TRIES_TO_GET_FEATURE_OBJECT = 3;
     //if we haven't heard back from visible in this amount of time,
     //then either re-issue the feature object request (up to
     //NUM_TRIES_TO_GET_FATURE_OBJECT times), or just pass through
     //undefined for the visible's featureObject (if tried that many
     //times, and didn't get a response).
     var TIME_TO_WAIT_FOR_FEATURE_OBJECT_RESP = 10;
     
     //map
     //keys: sporef of presence
     //values: map of actual visible objects
     var pResultSet = {};

     /**
      These get placed into the awaitingFeatureData and
      haveFeatureDataFor map
      @param {visible} vis - The visible that we request features for.
      @param {presence} featGrabber - The presence that we're using to
      request features and listen for feature updates.
      @param {presence} subscriber (optional) - A presence
      that wants to receive updates on vis.
      */
     function SubscriptionElement (vis,featGrabber,subscriber)
     {
         this.vis = vis;
         this.featGrabber =featGrabber;
         this.subscribers = {};
         if (typeof(subscriber) != 'undefined')
             this.subscribers[subscriber.toString()] = subscriber;
     }

     SubscriptionElement.prototype.addSubscriber =
         function(presToAdd)
     {
         this.subscribers[presToAdd.toString()] = presToAdd;
     };

     //returns true if there are no subscribers left.
     SubscriptionElement.prototype.noSubscribers =
         function()
     {
         for (var s in this.subscribers)
             return false;

         return true;
     };
     
     SubscriptionElement.prototype.removeSubscriber =
         function(presToRemove)
     {
         if (!(presToRemove.toString() in this.subscribers))
             delete this.subscribers[presToRemove.toString()];
     };

     
     //map
     //keys: sporef of visible awaiting data on
     //value: SubscriptionElement
     var awaitingFeatureData = {};

     //map
     //keys: sporef of visibles
     //keys: SubscriptionElement
     var haveFeatureDataFor  = {};
     
     //map
     //keys: sporef of presence
     //values: array of proximity added functions.  note, some
     //elements may be null if the proximity added function was
     //deregistered.
     var pAddCB = {};

     //map
     //keys: sporef of presence
     //values: array of proximity removed functions.  note, some
     //elements may be null if the proximity removed function was
     //deregistered.
     var pRemCB = {};

     //map
     //keys: sporef of presence
     //values: presences themselves.
     var sporefToPresences = {};

     system.onPresenceConnected(
         function(newPres)
         {
             sporefToPresences[newPres.toString()] = newPres;
             pResultSet[newPres.toString()]        = {};
             pAddCB[newPres.toString()]            = [];
             pRemCB[newPres.toString()]            = [];
         });

     
     //Does not hold internal data.  Instead, has private-like
     //variables defined above.
     function ProxManager()
     {
     }


     ProxManager.prototype.getProxResultSet = function(pres)
     {
         if (!(pres.toString() in pResultSet))
             throw new Error('Error getting prox result set.  ' +
                             'No record of presence.');

         return pResultSet[pres.toString()];
     };
     
     ProxManager.prototype.setProxAddCB = function(pres,proxAddCB)
     {
         if (!(pres.toString() in pAddCB))
         {
             throw new Error('Cannot add prox callback because do ' +
                             'Not have associated presence in ProxManager.');
         }

         
         var pArray = pAddCB[pres.toString()];
         
	 for (var i = 0; i <= pArray.length; i++)
	 {
	     if ((typeof(pArray[i]) == 'undefined') || (pArray[i] == null))
	     {
		 pArray[i] = proxAddCB;
		 return i;
	     }
	 }
         //shouldn't ever get here (notice the <= on exit condition of
         //for loop).  Just added so that syntax
         //highlighting of emacs doesn't keep bothering me.
         return null;
     };

     ProxManager.prototype.setProxRemCB = function (pres,proxRemCB)
     {
         if (!(pres.toString() in pRemCB))
         {
             throw new Error('Cannot add prox rem callback because do ' +
                             'not have associated presence in ProxManager.');
         }

         var pArray = pRemCB[pres.toString()];
         
	 for (var i = 0; i <= pArray.length; i++)
	 {
	     if ((typeof(pArray[i]) == 'undefined') || (pArray[i] == null))
	     {
		 pArray[i] = proxRemCB;
		 return i;
	     }
	 }
         //shouldn't ever get here (notice the <= on exit condition of
         //for loop).  Just added so that syntax
         //highlighting of emacs doesn't keep bothering me.
         return null;
     };


     ProxManager.prototype.delProxAddCB = function(pres,addID)
     {
         if (!(pres.toString() in pAddCB))
         {
             throw new Error('Cannot del prox add callback because do ' +
                             'not have associated presence in ProxManager.');
         }

         var pArray = pAddCB[pres.toString()];
         
         if (!(typeof(pArray[addID]) == 'undefined'))
             pArray[addID] = null;
     };

     ProxManager.prototype.delProxRemCB = function(pres,remID)
     {
         if (!(pres.toString() in pRemCB))
         {
             throw new Error('Cannot del prox rem callback because do ' +
                             'not have associated presence in ProxManager.');
         }

         var pArray = pRemCB[pres.toString()];
         
         
         if (!(typeof(pArray[addID]) == 'undefined'))
             pArray[addID] = null;         
     };


     /**
      Gets called by system whenever visibleObj is within proximity
      range of pres.  Will trigger proxadded callbacks to be called
      if we already have the featureData for visibleObj.  Otherwise,
      queue visibleObj onto awaitingFeatureData.
      */
     ProxManager.prototype.proxAddedEvent = function (pres,visibleObj)
     {
         if (!(pres.toString() in pAddCB))
         {
             throw new Error('Cannot call add event callback because do ' +
                             'not have associated presence in ProxManager.');
         }

         
         if (visibleObj.toString() in haveFeatureDataFor)
         {
             //we already have feature data for this visible.  add pres
             //as subscriber to haveFeatureDataFor map and trigger callback.
             var subElement = haveFeatureDataFor[visibleObj.toString()];
             subElement.addSubscriber(pres);
             this.triggerAddCallback(pres,subElement.vis);
         }
         else
         {
             //we do not already have feature data for this visible.
             //Check if we're waiting on feature data for this object.
             //If we are, then, just add pres as a subscriber.  If we
             //are not, initiate feature object subscription using
             //pres as the requester.  If we are, waiting, then add
             //pres as a subscriber for this feature data.
             if (visibleObj.toString() in awaitingFeatureData)
             {
                 var subElement = awaitingFeatureData[visibleObj.toString()];
                 subElement.addSubscriber(pres);
             }
             else
             {
                 var subElement = new SubscriptionElement(visibleObj,pres,pres);
                 this.beginFeatureSubscribe(pres, visibleObj);
                 awaitingFeatureData[visibleObj.toString()] = subElement;
             }
         }
     };

     //tells pres to listen for feature vector from visibleObj.
     //numTriesLeft should be left blank except when this function
     //calls itself.  (the idea is that it tries to get feature object
     //data from a presence multiple times and numTriesLeft is a
     //simple way for the function to keep track of how many times
     //we've attempted to get feature object data from other side.
     ProxManager.prototype.beginFeatureSubscribe =
         function(pres,visibleObj,numTriesLeft)
     {
         if (typeof(numTriesLeft) == 'undefined')
             numTriesLeft = NUM_TRIES_TO_GET_FEATURE_OBJECT;

         //If we try multiple times, and still don't get a response, the visible
         //probably is not running the feature object protocol.  We
         //can't do anything about that, and instead fire callback
         //with undefined passed through for featureObject.
         if (numTriesLeft <=0)
         {
             this.subscribeComplete(pres,visibleObj,undefined);
             return;
         }
         --numTriesLeft;

         
         //see handleFeautreObjectSubscribe (featureObject.em) to see
         //what format the feature object subscription request message
         //should take.
         pres # {'presFeatureSubRequest':true} >> visibleObj >>
             [std.core.bind(handleFeatureData,undefined,this,pres,visibleObj),
              TIME_TO_WAIT_FOR_FEATURE_OBJECT_RESP,
              std.core.bind(
                  this.beginFeatureSubscribe,
                  this,pres,visibleObj,numTriesLeft)];
         
         // //for now, simple sanity check.  After 1 second, release,
         // //saying that featureData is 1.
         // system.timeout(1,std.core.bind(this.subscribeComplete,this,pres,visibleObj,1));
     };

     function handleFeatureData(proxMan,mPres,vis,featureMsg,visSender)
     {
         //see handleFeatureObjectSubscribe for structure of reply
         //mesages.
         proxMan.subscribeComplete(mPres,vis,featureMsg.data);
     }
     

     //Call this function when you want pres to unsubscribe for
     //feature data from visibleObj.
     ProxManager.prototype.killSubscription = function(pres,visibleObj)
     {
         //see handleFeautreObjectRequestUnsubscribe message handler
         //in featureObject.em to see what the format of a
         //killSubscription message should look like.
         pres # {'presFeatureReqUnsub': true} >> visibleObj >> [];
     };

     
     //should get called when a presence has received feature data
     //from visibleObj.
     ProxManager.prototype.subscribeComplete =
         function(pres,visibleObj,featureData)
     {
         visibleObj.featureData = featureData;
         if (!(visibleObj.toString() in awaitingFeatureData))
         {
             throw new Error('Error in subscribeComplete.  No longer '+
                             'have visibleObj in awaitingFeatureData.');                 
         }

         var subElement = awaitingFeatureData[visibleObj.toString()];
         delete awaitingFeatureData[visibleObj.toString()];

         
         if (subElement.noSubscribers())
         {
             //no one is waiting for/actually cares about the new
             //data.
             this.killSubscription(pres,visibleObj);
             return;
         }

         //know that we've received the data and that others want to
         //hear about it.  add subElement to haveFeatureDataFor map
         //and notify all subscribers that visibleObj (with
         //featureData) is in its prox result set and add visibleObj
         //to result set.
         haveFeatureDataFor[visibleObj.toString()] = subElement;
         for (var s in subElement.subscribers)
         {
             this.triggerAddCallback(subElement.subscribers[s],
                                     subElement.vis);
         }
     };

     //actually fire callback for pres's having seen visibleObj in-world
     //and add visibleObj to result set.
     ProxManager.prototype.triggerAddCallback = function(pres,visibleObj)
     {
         pResultSet[pres.toString()][visibleObj.toString()] = visibleObj;
         var pArray = pAddCB[pres.toString()];
         //trigger all non-null non-undefined callbacks
         system.changeSelf(pres);
	 for (var i in pArray)
         {
	     if ((typeof(pArray[i]) != 'undefined') && (pArray[i] != null))
		 pArray[i](visibleObj);                 
         }
     };



     //called by system when pinto actually states that visibleObj is
     //distant from pres.  If the visible exists in awaitingFeatureData
     //then we just remove a subscriber from it.  If the visible exists in
     //haveFeatureDataFor, then fire removed callback, decrement
     //subscribers.  If no subscribers left in haveFeatureDataFor, then
     //unsubscribe from feature data, and remove element from
     //haveFeatureDataFor.
     ProxManager.prototype.proxRemovedEvent = function (pres,visibleObj)
     {
         if (visibleObj.toString() in awaitingFeatureData)
             awaitingFeatureData[visibleObj.toString()].removeSubscriber(pres);                 
         else
         {
             //this line ensures that visibleObj will maintain
             //featureData associated with stored vis.
             visibleObj = haveFeatureDataFor[visibleObj.toString()].vis;
             var subElem = haveFeatureDataFor[visibleObj.toString()];
             subElem.removeSubscriber(pres);

             if (subElem.noSubscribers())
             {
                 this.killSubscription(pres,visibleObj);
                 delete haveFeatureDataFor[visibleObj.toString()];
             }

             //actually fire removed callback.
             this.triggerRemoveCallback(pres,visibleObj);
         }
     };



     ProxManager.prototype.triggerRemoveCallback = function(pres,visibleObj)
     {
         if (!(pres.toString() in pRemCB))
         {
             throw new Error('Cannot call rem event callback because do ' +
                             'not have associated presence in ProxManager.');
         }
         if (!(pres.toString() in pResultSet))
         {
             throw new Error('Cannot call rem event callback for presence ' +
                             'because no presence in pResultSet matches.');
         }

         //remove from to proxResultSet
         delete pResultSet[pres.toString()][visibleObj.toString()];

         //actually issue callbacks
         var pArray = pRemCB[pres.toString()];
         system.changeSelf(pres);
         //trigger all non-null non-undefined callbacks

	 for (var i in pArray)
         {
	     if ((typeof(pArray[i]) != 'undefined') && (pArray[i] != null))
		 pArray[i](visibleObj);                          
         }

     };

     
     //actually register an instance of proxmanager with system.
     system.__registerProxManager(new ProxManager());
 }
)();