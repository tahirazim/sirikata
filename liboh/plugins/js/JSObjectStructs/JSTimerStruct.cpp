
#include "../JSUtil.hpp"
#include "../EmersonScript.hpp"
#include "JSTimerStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include <v8.h>
#include "../JSLogging.hpp"
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/Asio.hpp>
#include <sirikata/core/service/Context.hpp>
#include "JSSuspendable.hpp"
#include "Util.hpp"

namespace Sirikata {
namespace JS {

JSTimerStruct::JSTimerStruct(EmersonScript* eobj, Duration dur, v8::Persistent<v8::Function>& callback,
    JSContextStruct* jscont, Sirikata::Context* ctx, uint32 contID,
    double timeRemaining, bool isSuspended,bool isCleared)
 :JSSuspendable(),
  emerScript(eobj),
  cb(callback),
  jsContStruct(jscont),
  mContext(ctx),
  mDeadlineTimer(Sirikata::Network::IOTimer::create(*ctx->ioService)),
  timeUntil(dur),
  mTimeRemaining(timeRemaining),
  killAfterFire(false),
  noTimerWaiting(true)
{
    if (isCleared)
    {
        clear();
        return;
    }

    if (isSuspended)
        suspend();


    if (contID != jscont->getContextID())
    {
        eobj->registerFixupSuspendable(this,contID);
    }
    else
    {
        noTimerWaiting=false;

        if (timeRemaining == 0)
            mDeadlineTimer->wait(timeUntil,std::tr1::bind(&JSTimerStruct::evaluateCallback,this));
        else
            mDeadlineTimer->wait(Duration::microseconds(timeRemaining*1000000),std::tr1::bind(&JSTimerStruct::evaluateCallback,this));


        if (jscont != NULL)
            jscont->struct_registerSuspendable(this);
    }
}

void JSTimerStruct::setPersistentObject(v8::Persistent<v8::Object>pers)
{
    mPersistentHandle = pers;
}

void JSTimerStruct::timerWeakReferenceCleanup(v8::Persistent<v8::Value> containsTimer, void* otherArg)
{
    if (!containsTimer->IsObject())
    {
        JSLOG(error, "Error when cleaning up jstimer.  Received a timer to clean up that wasn't an object.");
        return;
    }

    v8::Handle<v8::Object> timer = containsTimer->ToObject();

    //check to make sure object has adequate number of fields.
    CHECK_INTERNAL_FIELD_COUNT(timer,jstimer,TIMER_JSTIMER_TEMPLATE_FIELD_COUNT, );


    //should return if have already cleared the timer.
    String err = "Potential error when cleaning up jstimer.  Could not decode timer struct.  Likely the timer struct was already cleared, but could be more serious.";
    JSTimerStruct* jstimer = JSTimerStruct::decodeTimerStruct(timer,err);
    if (jstimer == NULL)
    {
        JSLOG(insane,err);
        return;
    }

    //asks the particular timer to free itself if it's never going to fire
    //again, or schedule itself to be freed after will never fire again.
    jstimer->noReference(jstimer->mLiveness.livenessToken());//not async, so you can just pass the shared ptr in
}


/**
   Called by timerWeakReferenceCleanup and after firing timer when killAfterFire
   is true.  Means that no Emerson objects hold references to this timer any
   longer and that this object should either: 1) Free itself if there's no way
   that its timer will re-fire.  2) Schedule itself to be freed after its timer
   fires.

   For case 1: the timer has already fired, and (the timer isn't suspended and
   its context isn't suspended).
   For case 2: just sets killAfterFire to be true.  Will re-evaluate these
   conditions after timer actually fires.
 */
void JSTimerStruct::noReference(const Liveness::Token& alive)
{
    if (alive) {
        killAfterFire = true;

        if (noTimerWaiting)
        {
            //check if it's suspended and its context is suspended
            if (!(getIsSuspended() && jsContStruct->getIsSuspended()))
            {
                //can kill
                clear();
            }
        }
    }
}



void JSTimerStruct::fixSuspendableToContext(JSContextStruct* toAttachTo)
{
    jsContStruct = toAttachTo;

    noTimerWaiting=false;
    if (mTimeRemaining == 0)
        mDeadlineTimer->wait(timeUntil,std::tr1::bind(&JSTimerStruct::evaluateCallback,this));
    else
        mDeadlineTimer->wait(Duration::microseconds(mTimeRemaining*1000000),std::tr1::bind(&JSTimerStruct::evaluateCallback,this));

    jsContStruct->struct_registerSuspendable(this);
}


JSTimerStruct* JSTimerStruct::decodeTimerStruct(v8::Handle<v8::Value> toDecode,String& errorMessage)
{
    v8::HandleScope handle_scope;  //for garbage collection.

    if (! toDecode->IsObject())
    {
        errorMessage += "Error in decode of timer object in JSTimerStruct.cpp.  Should have received an object to decode.";
        return NULL;
    }

    v8::Handle<v8::Object> toDecodeObject = toDecode->ToObject();

    //now check internal field count
    if (toDecodeObject->InternalFieldCount() != TIMER_JSTIMER_TEMPLATE_FIELD_COUNT)
    {
        errorMessage += "Error in decode of timer object in JSTimerStruct.  Object given does not have adequate number of internal fields for decode.";
        return NULL;
    }

    //now actually try to decode each.
    //decode the jsTimerStruct field
    v8::Local<v8::External> wrapJSTimerStruct;
    wrapJSTimerStruct = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(TIMER_JSTIMERSTRUCT_FIELD));
    void* ptr = wrapJSTimerStruct->Value();


    JSTimerStruct* returner;
    returner = static_cast<JSTimerStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of timer object in JSTimerStruct.cpp.  Internal field of object given cannot be casted to a JSTimerStruct.";

    return returner;
}


JSTimerStruct::~JSTimerStruct()
{
    clear();
}

//returning all data necessary to re-generate timer
//  uint32   contextId
//  double period
//  double timeUntil timer expires
//  bool  isSuspended
//  bool  isCleared
//  func  cb
v8::Handle<v8::Value> JSTimerStruct::struct_getAllData()
{
    v8::HandleScope handle_scope;

    uint32  contId   = jsContStruct->getContextID();
    bool    issusp   = getIsSuspended();
    bool    isclear  = getIsCleared();
    double  period   = -1;
    double  tUntil   = -1;

    v8::Handle<v8::Function> cbFunc;

    if (! isclear)
    {
        cbFunc = cb;
        period = timeUntil.toSeconds();
        if (issusp)
            tUntil = timeUntil.toSeconds();
        else
        {
            Duration pt = mDeadlineTimer->expiresFromNow();
            tUntil = pt.seconds();
        }
    }

    v8::Handle<v8::Object> returner = v8::Object::New();

    if (isclear)
    {
        returner->Set(v8::String::New("isCleared"),v8::Boolean::New(isclear));
        returner->Set(v8::String::New("contextId"), v8::Integer::NewFromUnsigned(contId));
        return handle_scope.Close(returner);
    }

    returner->Set(v8::String::New("period"), v8::Number::New(period));
    returner->Set(v8::String::New("callback"),cbFunc);
    returner->Set(v8::String::New("timeRemaining"),v8::Number::New(tUntil));
    returner->Set(v8::String::New("isSuspended"),v8::Boolean::New(issusp));


    return handle_scope.Close(returner);
}



void JSTimerStruct::evaluateCallback()
{
    Liveness::Token token=mLiveness.livenessToken();
    emerScript->invokeCallbackInContext(emerScript->livenessToken(), cb, jsContStruct);
    if (token) {
        if (mContext->stopped()) {
            JSLOG(warn, "Timer evaluateCallback invoked after stop request, ignoring...");
            noTimerWaiting=true; // Allow cleanup, see notes below
            return;
        }

        //if we were told to kill the timer after firing, then check kill conditions
        //again in noReference.
        if (killAfterFire)
            mContext->mainStrand->post(std::tr1::bind(&JSTimerStruct::noReference,this,token));

        //means that we have no pending timer operation.
        // Note that since this allows the JS GC thread to destroy this object
        // in response to all references to it being lost,
        // we need to make sure it is absolutely the *last* operation we do on
        // member variables.
        noTimerWaiting=true;
    }
}


v8::Handle<v8::Value> JSTimerStruct::suspend()
{
    if (getIsCleared())
    {
        JSLOG(info, "Error in suspend of JSTimerStruct.cpp.  Called suspend even though the timer had previously been cleared.");
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Called suspend on a timer that had already been cleared.")));
    }

    JSLOG(insane,"suspending timer");

    //note, it is important that call to JSSuspendable::supsend occurs before
    //actually cancelling the deadline timer so that we're performing correct
    //checks for timer cleanup.
    v8::HandleScope handle_scope;
    v8::Handle<v8::Value>returner = JSSuspendable::suspend();
    mDeadlineTimer->cancel();
    // Note that since this allows the JS GC thread to destroy this object
    // in response to all references to it being lost,
    // we need to make sure it is absolutely the *last* operation we do on
    // member variables.
    noTimerWaiting = true;
    return handle_scope.Close(returner);
}


//has more of a reset-type functionality than resume
//if the time has not been cleared, then, cancel the current timer,
//and start a new countdown to execute the callback.
v8::Handle<v8::Value> JSTimerStruct::resume()
{
    if (getIsCleared())
    {
        JSLOG(info,"Error in JSTimerStruct.  Trying to resume a timer object that has already been cleared.  Taking no action");
        return JSSuspendable::getIsSuspendedV8();
    }
    mDeadlineTimer->cancel();

    noTimerWaiting=false;
    mDeadlineTimer->wait(timeUntil,std::tr1::bind(&JSTimerStruct::evaluateCallback,this));

    return JSSuspendable::resume();
}



v8::Handle<v8::Value> JSTimerStruct::clear()
{
    if (getIsCleared())
    {
        JSLOG(insane,"In JSTimerStruct, calling clear on a timer that has already been cleared.");
        return JSSuspendable::clear();
    }

    v8::HandleScope handle_scope;

    JSSuspendable::clear();

    mDeadlineTimer->cancel();

    if (! cb.IsEmpty())
        cb.Dispose();

    if (! mPersistentHandle.IsEmpty())
    {
        //check to make sure object has adequate number of fields.
        CHECK_INTERNAL_FIELD_COUNT(mPersistentHandle,jstimer,TIMER_JSTIMER_TEMPLATE_FIELD_COUNT, v8::Boolean::New(true));

        //delete typeId, and return if have incorrect params for type id
        DEL_TYPEID_AND_CHECK(mPersistentHandle,jstimer,TIMER_TYPEID_STRING,v8::Boolean::New(true));

        mPersistentHandle->SetInternalField(TIMER_JSTIMERSTRUCT_FIELD, External::New(NULL));
    }

    // Be careful after this! JSContextStruct::struct_deregisterSuspendable will
    // delete this object so you shouldn't use any member variables after
    // invoking it.
    if (jsContStruct != NULL)
        jsContStruct->struct_deregisterSuspendable(this);

    // Note that since this allows the JS GC thread to destroy this object
    // in response to all references to it being lost,
    // we need to make sure it is absolutely the *last* operation we do on
    // member variables.
    // In this case, the above call will have deleted this JSTimerStruct, so we
    // don't even set it at all. It would have been unsafe to set it earlier
    // because we could have ended up with two threads trying to do the deletion.
    //noTimerWaiting = true;

    return v8::Boolean::New(true);
}


v8::Handle<v8::Value> JSTimerStruct::struct_resetTimer(double timeInSecondsToRefire)
{
    if (getIsCleared())
    {
        JSLOG(info,"Error in JSTimerStruct.  Calling reset on a timer that has already been cleared.");
        return JSSuspendable::clear();
    }

    mDeadlineTimer->cancel();
    noTimerWaiting=false;
    mDeadlineTimer->wait(Duration::seconds(timeInSecondsToRefire),std::tr1::bind(&JSTimerStruct::evaluateCallback,this));

    return JSSuspendable::resume();
}



} //end namespace js
} //end namespace sirikata
