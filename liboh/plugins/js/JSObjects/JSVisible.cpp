#include "JSVisible.hpp"

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"
#include "JSObjectsUtils.hpp"
#include "../JSSerializer.hpp"
#include "../JSObjectStructs/JSVisibleStruct.hpp"
#include "../JSObjectStructs/JSSystemStruct.hpp"
#include "JSFields.hpp"
#include "JSVec3.hpp"
#include "JSObjectsUtils.hpp"

#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <boost/tr1/tuple.hpp>

namespace Sirikata {
namespace JS {
namespace JSVisible {

/**
   @param Vec3.
   @return Number.

   Returns the distance from this visible object to the position
   specified by first argument vector.
 */
v8::Handle<v8::Value> dist(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: need exactly one argument to dist method of Visible")));

    String errorMessage = "Error in dist method of JSVisible.cpp.  Cannot decode visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);

    if (jsvis == NULL)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    if (! args[0]->IsObject())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in dist of JSVisible.cpp.  Argument should be an objet.")));

    v8::Handle<v8::Object> argObj = args[0]->ToObject();

    bool isVec3 = Vec3Validate(argObj);
    if (! isVec3)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: argument to dist method of Visible needs to be a vec3")));

    Vector3d vec3 = Vec3Extract(argObj);

    return jsvis->struct_getDistance(vec3);
}

/**
   @return A string corresponding to the URI for your current mesh.  Can pass
   this uri to setMesh functions on your own presences, but cannot set mesh
   directly on a visible.
 */
Handle<v8::Value> getMesh(const v8::Arguments& args)
{

    String errorMessage = "Error in getMesh while decoding visible.  ";
    JSVisibleStruct* mStruct = JSVisibleStruct::decodeVisible(args.This() ,errorMessage);

    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return mStruct->struct_getMesh();
}

/**
   @return A string containing physics settings for this object.
 */
Handle<v8::Value> getPhysics(const v8::Arguments& args)
{
    String errorMessage = "Error in getPhysics while decoding visible.  ";
    JSVisibleStruct* mStruct = JSVisibleStruct::decodeVisible(args.This() ,errorMessage);

    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return mStruct->struct_getPhysics();
}


v8::Handle<v8::Value> getAllData(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;

    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: getAllData call takes no arguments.")));


    std::string errorMessage = "In getAllData function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return handle_scope.Close(jsvis->struct_getAllData());
}

v8::Handle<v8::Value> getType(const v8::Arguments& args)
{
    return v8::String::New("visible");
}




/**
   @return Vec3 associated with the position of this visible object.

   Note: the returned value may be stale if the visible object is far away from you.
 */
v8::Handle<v8::Value> getPosition(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: getPosition method of Visible takes no arguments")));

    std::string errorMessage = "In getPosition function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return jsvis->struct_getPosition();
}


/**
   @return Number associated with the velocity at which this visible object is travelling.

   Note: the returned value may be stale if the visible object is far away from you.
 */
Handle<v8::Value> getVelocity(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: getVelocity method of Visible takes no arguments")));

    std::string errorMessage = "In getVelocity function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    return jsvis->struct_getVelocity();
}

v8::Handle<v8::Value> getSpace(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error.  Getting spaceID requires no arguments.")));

    String errorMessage = "Error in getSpaceID while decoding presence.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This() ,errorMessage);

    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str()) ));

    SpaceObjectReference sporef = jsvis->getSporef();

    if (sporef == SpaceObjectReference::null())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  SpaceID is not defined.")));

    return v8::String::New(sporef.space().toString().c_str());
}

v8::Handle<v8::Value>  getOref(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error.  Getting objectID requires no arguments.")));

    String errorMessage = "Error in getObjectID while decoding visible.  ";
    JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (jspres == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str()) ));

    SpaceObjectReference sporef = jspres->getSporef();

    if (sporef == SpaceObjectReference::null())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  ObjectID is not defined.")));

    return v8::String::New(sporef.object().toString().c_str());
}


/**
   @return Quaternion associated with visible object's orientation.

   Note: the returned value may be stale if the visible object is far away from you.
 */
Handle<v8::Value> getOrientation(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: need exactly one argument to getOrientation method of Visible")));

    std::string errorMessage = "In getOrientation function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    return jsvis->struct_getOrientation();
}

/**
   @return Angular velocity of visible object (rad/s).

   Note: the returned value may be stale if the visible object is far away from you.
 */
Handle<v8::Value> getOrientationVel(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: need exactly one argument to getOrientationVel method of Visible")));

    std::string errorMessage = "In getOrientationVel function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    return jsvis->struct_getOrientationVel();
}


/*
  @return Number associated with how large the visible object is compared to the
  mesh it came from.
 */
Handle<v8::Value> getScale(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: need exactly one argument to getOrientationVel method of Visible")));

    std::string errorMessage = "In getOrientationVel function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    return jsvis->struct_getScale();
}

v8::Handle<v8::Value> toString(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: need exactly one argument to toString method of Visible")));

    std::string errorMessage = "In toString function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    return jsvis->toString();
}


v8::Handle<v8::Value> __debugRef(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to __debugRef.  Requires 0 arguments.")) );

    std::string errorMessage = "In __debugRef function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    return jsvis->printData();
}


bool isVisibleObject(v8::Handle<v8::Value> v8Val)
{
  if( v8Val->IsNull() || v8Val->IsUndefined() || !v8Val->IsObject())
  {
    return false;
  }

  // This is an object

  v8::Handle<v8::Object>v8Obj = v8::Handle<v8::Object>::Cast(v8Val);

  if (TYPEID_FIELD >= v8Obj->InternalFieldCount()) return false;

  v8::Local<v8::Value> typeidVal = v8Obj->GetInternalField(TYPEID_FIELD);
  if(typeidVal->IsNull() || typeidVal->IsUndefined())
  {
      return false;
  }

  v8::Local<v8::External> wrapped  = v8::Local<v8::External>::Cast(typeidVal);
  void* ptr = wrapped->Value();
  std::string* typeId = static_cast<std::string*>(ptr);
  if(typeId == NULL) return false;

  std::string typeIdString = *typeId;

  if(typeIdString == VISIBLE_TYPEID_STRING)
  {
    return true;
  }

  return false;

}


/**
   @return Boolean.  If true, positions and velocities for this visible object
   are automatically being updated by the system.

 */
v8::Handle<v8::Value> getStillVisible(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to getStillVisible.  Requires 0 arguments.")) );

    std::string errorMessage = "In getStillVisible function of visible.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));

    return jsvis->struct_getStillVisible();

}


/**
   @param Visible object.
   @return Returns true if the visible objects correspond to the same presence
   in the virtual world.
 */
v8::Handle<v8::Value> checkEqual(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to checkEqual.  Requires 1 argument: another JSVisibleStruct.")) );


    std::string errorMessage = "In checkEqual function of visible.  Decoding caller.  ";
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (jsvis == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));


    errorMessage = "In checkEqual function of visible.  Decoding argument.  ";
    JSVisibleStruct* jsvis2 = JSVisibleStruct::decodeVisible(args[0],errorMessage);
    if (jsvis2 == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));


    return jsvis->struct_checkEqual(jsvis2);
}


namespace {
v8::Handle<v8::Function> decodeCallbackArgument(const v8::Arguments& args, int32 idx) {
    v8::Handle<v8::Function> cb;
    if (args.Length() > idx) {
        v8::Handle<v8::Value> cbVal = args[idx];
        if (cbVal->IsFunction())
            cb = v8::Handle<v8::Function>::Cast(cbVal);
    }
    return cb;
}
}

v8::Handle<v8::Value> getAnimationList(const v8::Arguments& args)
{
  String errorMessage = "Error in getAnimationList while decoding presence.  ";
  JSVisibleStruct* mStruct = JSVisibleStruct::decodeVisible(args.This() ,errorMessage);

  if (mStruct == NULL)
    return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

  return mStruct->struct_getAnimationList();
}

v8::Handle<v8::Value> loadMesh(const v8::Arguments& args)
{
    if (args.Length() != 2 && args.Length() != 3) // only tell about one, first should be system
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling visible.loadMesh.  Requires 1 argument: the callback to invoke when loading is complete.")));

    String errorMessage = "Error in loadMesh while decoding visible.  ";
    JSVisibleStruct* visstruct = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (visstruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    errorMessage = "Error decoding system in visible.loadMesh";
    JSSystemStruct* sysstruct = JSSystemStruct::decodeSystemStruct(args[0], errorMessage);

    v8::Handle<v8::Function> cb = decodeCallbackArgument(args, 1);
    if (cb.IsEmpty())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid callback argument to visible.loadMesh.")));

    bool downloadFullAsset = false;
    if (args.Length() >= 3) {
        errorMessage = "Invalid downloadFullAsset argument.";
        bool decoded = decodeBool(args[2], downloadFullAsset, errorMessage);
        if (!decoded) return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid argument to visible.loadMesh.")));
    }

    return visstruct->loadMesh(sysstruct->getContext(), cb, downloadFullAsset);
}

v8::Handle<v8::Value> meshBounds(const v8::Arguments& args) {
    if (args.Length() != 0)
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling visible.meshBounds.  Should take no arguments.")));

    String errorMessage = "Error in meshBounds while decoding visible.";
    JSVisibleStruct* visstruct = JSVisibleStruct::decodeVisible(args.This() ,errorMessage);
    if (visstruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return visstruct->meshBounds();
}

v8::Handle<v8::Value> untransformedMeshBounds(const v8::Arguments& args) {
    if (args.Length() != 0)
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling visible.meshBounds.  Should take no arguments.")));

    String errorMessage = "Error in untransformedMeshBounds while decoding visible.";
    JSVisibleStruct* visstruct = JSVisibleStruct::decodeVisible(args.This() ,errorMessage);
    if (visstruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return visstruct->untransformedMeshBounds();
}

v8::Handle<v8::Value> raytrace(const v8::Arguments& args) {
    if (args.Length() != 2)
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling visible.raytrace.  Should take ray start and ray direction.")));

    String errorMessage = "Error in raytrace while decoding visible.";
    JSVisibleStruct* visstruct = JSVisibleStruct::decodeVisible(args.This() ,errorMessage);
    if (visstruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    v8::Handle<v8::Object> arg0 = args[0]->ToObject();
    if (!args[0]->IsObject() || !Vec3Validate(arg0))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: first argument to raytrace should be vec3.")));
    Vector3d ray_start = Vec3Extract(arg0);

    v8::Handle<v8::Object> arg1 = args[1]->ToObject();
    if (!args[1]->IsObject() || !Vec3Validate(arg1))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: second argument to raytrace should be vec3.")));
    Vector3d ray_dir = Vec3Extract(arg1);

    return visstruct->raytrace(Vector3f(ray_start), Vector3f(ray_dir));
}

v8::Handle<v8::Value> unloadMesh(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException ( v8::Exception::Error(v8::String::New("Error calling visible.unloadMesh. Takes no arguments.")));

    String errorMessage = "Error in unloadMesh while decoding visible.  ";
    JSVisibleStruct* visstruct = JSVisibleStruct::decodeVisible(args.This(),errorMessage);
    if (visstruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return visstruct->unloadMesh();
}




static std::tr1::tuple<JSVisibleStruct*, JSSystemStruct*, v8::Handle<v8::Function>, String> registerEventHandlerHelper(const v8::Arguments& args, const String& name) {
    if (args.Length() != 2) {
        String errmsg = String("Error calling visible.") + name + String(".  Requires 2 arguments: the system and the callback to invoke.");
        return std::tr1::make_tuple((JSVisibleStruct*)NULL, (JSSystemStruct*)NULL, v8::Handle<v8::Function>(), errmsg);
    }

    String errorMessage = String("Error in visible.") + name + String(" while decoding visible.  ");
    JSVisibleStruct* visstruct = JSVisibleStruct::decodeVisible(args.This(), errorMessage);
    if (visstruct == NULL)
        return std::tr1::make_tuple((JSVisibleStruct*)NULL, (JSSystemStruct*)NULL, v8::Handle<v8::Function>(), errorMessage);

    errorMessage = String("Error decoding system in visible.") + name;
    JSSystemStruct* sysstruct = JSSystemStruct::decodeSystemStruct(args[0], errorMessage);
    if (visstruct == NULL)
        return std::tr1::make_tuple((JSVisibleStruct*)NULL, (JSSystemStruct*)NULL, v8::Handle<v8::Function>(), errorMessage);

    v8::Handle<v8::Function> cb = decodeCallbackArgument(args, 1);
    if (cb.IsEmpty()) {
        String errmsg = String("Invalid callback argument to visible.") + name + String(".");
        return std::tr1::make_tuple((JSVisibleStruct*)NULL, (JSSystemStruct*)NULL, v8::Handle<v8::Function>(), errmsg);
    }

    return std::tr1::make_tuple(visstruct, sysstruct, cb, String());
}

v8::Handle<v8::Value> onPositionChanged(const v8::Arguments& args) {
    JSVisibleStruct* visstruct; JSSystemStruct* sysstruct; v8::Handle<v8::Function> cb; String errmsg;
    std::tr1::tie(visstruct, sysstruct, cb, errmsg) = registerEventHandlerHelper(args, "onPositionChanged");
    if (visstruct == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New(errmsg.c_str())));
    return visstruct->onPositionChanged(sysstruct->getContext(), cb);
}

v8::Handle<v8::Value> onVelocityChanged(const v8::Arguments& args) {
    JSVisibleStruct* visstruct; JSSystemStruct* sysstruct; v8::Handle<v8::Function> cb; String errmsg;
    std::tr1::tie(visstruct, sysstruct, cb, errmsg) = registerEventHandlerHelper(args, "onVelocityChanged");
    if (visstruct == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New(errmsg.c_str())));
    return visstruct->onVelocityChanged(sysstruct->getContext(), cb);
}

v8::Handle<v8::Value> onOrientationChanged(const v8::Arguments& args) {
    JSVisibleStruct* visstruct; JSSystemStruct* sysstruct; v8::Handle<v8::Function> cb; String errmsg;
    std::tr1::tie(visstruct, sysstruct, cb, errmsg) = registerEventHandlerHelper(args, "onOrientationChanged");
    if (visstruct == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New(errmsg.c_str())));
    return visstruct->onOrientationChanged(sysstruct->getContext(), cb);
}

v8::Handle<v8::Value> onOrientationVelChanged(const v8::Arguments& args) {
    JSVisibleStruct* visstruct; JSSystemStruct* sysstruct; v8::Handle<v8::Function> cb; String errmsg;
    std::tr1::tie(visstruct, sysstruct, cb, errmsg) = registerEventHandlerHelper(args, "onOrientationVelChanged");
    if (visstruct == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New(errmsg.c_str())));
    return visstruct->onOrientationVelChanged(sysstruct->getContext(), cb);
}

v8::Handle<v8::Value> onScaleChanged(const v8::Arguments& args) {
    JSVisibleStruct* visstruct; JSSystemStruct* sysstruct; v8::Handle<v8::Function> cb; String errmsg;
    std::tr1::tie(visstruct, sysstruct, cb, errmsg) = registerEventHandlerHelper(args, "onScaleChanged");
    if (visstruct == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New(errmsg.c_str())));
    return visstruct->onScaleChanged(sysstruct->getContext(), cb);
}

v8::Handle<v8::Value> onMeshChanged(const v8::Arguments& args) {
    JSVisibleStruct* visstruct; JSSystemStruct* sysstruct; v8::Handle<v8::Function> cb; String errmsg;
    std::tr1::tie(visstruct, sysstruct, cb, errmsg) = registerEventHandlerHelper(args, "onMeshChanged");
    if (visstruct == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New(errmsg.c_str())));
    return visstruct->onMeshChanged(sysstruct->getContext(), cb);
}

v8::Handle<v8::Value> onPhysicsChanged(const v8::Arguments& args) {
    JSVisibleStruct* visstruct; JSSystemStruct* sysstruct; v8::Handle<v8::Function> cb; String errmsg;
    std::tr1::tie(visstruct, sysstruct, cb, errmsg) = registerEventHandlerHelper(args, "onPhysicsChanged");
    if (visstruct == NULL) return v8::ThrowException( v8::Exception::Error(v8::String::New(errmsg.c_str())));
    return visstruct->onPhysicsChanged(sysstruct->getContext(), cb);
}


}//end jsvisible namespace
}//end js namespace
}//end sirikata
