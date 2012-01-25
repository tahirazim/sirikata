#ifndef __SIRIKATA_JS_POSITION_LISTENER_STRUCT_HPP__
#define __SIRIKATA_JS_POSITION_LISTENER_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include <sirikata/core/util/Liveness.hpp>
#include <sirikata/mesh/Visual.hpp>
#include "../JSVisibleManager.hpp"
#include "../JSCtx.hpp"

namespace Sirikata {
namespace JS {

class JSVisibleData;
class JSContextStruct;

//note: only position and isConnected will actually set the flag of the watchable
struct JSPositionListener : public Liveness
{
    friend class JSSerializer;
    friend class JSVisibleStruct;
    friend class JSPresenceStruct;

public:
    virtual ~JSPositionListener();

    virtual Vector3f     getPosition();
    virtual Vector3f     getVelocity();
    virtual Quaternion   getOrientation();
    virtual Quaternion   getOrientationVelocity();
    virtual BoundingSphere3f   getBounds();
    virtual String getMesh();
    virtual String getPhysics();
    virtual bool getStillVisible();

    virtual v8::Handle<v8::Value> struct_getPosition();
    virtual v8::Handle<v8::Value> struct_getVelocity();
    virtual v8::Handle<v8::Value> struct_getOrientation();
    virtual v8::Handle<v8::Value> struct_getOrientationVel();
    virtual v8::Handle<v8::Value> struct_getScale();
    virtual v8::Handle<v8::Value> struct_getMesh();
    virtual v8::Handle<v8::Value> struct_getPhysics();
    virtual v8::Handle<v8::Value> struct_getTransTime();
    virtual v8::Handle<v8::Value> struct_getOrientTime();
    virtual v8::Handle<v8::Value> struct_getSporef();
    virtual v8::Handle<v8::Value> struct_getStillVisible();

    virtual v8::Handle<v8::Value> struct_getAnimationList();


    virtual v8::Handle<v8::Value> struct_getAllData();
    virtual v8::Handle<v8::Value> struct_checkEqual(JSPositionListener* jpl);

    virtual v8::Handle<v8::Value> struct_getDistance(const Vector3d& distTo);

    v8::Handle<v8::Value> loadMesh(JSContextStruct* ctx, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> meshBounds();
    v8::Handle<v8::Value> untransformedMeshBounds();
    // NOTE: This the ray parameters are in *object* space.
    v8::Handle<v8::Value> raytrace(const Vector3f& mesh_ray_start, const Vector3f& mesh_ray_dir);
    v8::Handle<v8::Value> unloadMesh();

    //simple accessors for sporef fields
    SpaceObjectReference getSporef();


protected:
    v8::Handle<v8::Value> wrapSporef(SpaceObjectReference sporef);


    EmersonScript* mParentScript;
    JSAggregateVisibleDataPtr jpp;
    // We don't store this in jpp because we would just have to keep track of
    // separate flags for whether we loaded it so we could do some refcounting.
    Mesh::VisualPtr mVisual;
    JSCtx* mCtx;

private:

    void eLoadMesh(
        JSContextStruct* ctx,v8::Persistent<v8::Function>cb);

    void iFinishLoadMesh(
        Liveness::Token alive, Liveness::Token ctx_alive,
        JSContextStruct* ctx, v8::Persistent<v8::Function> cb,
        Mesh::VisualPtr data);


    //private constructor.  Can only be made through serializer,
    //JSVisibleStruct, or JSPresenceStruct.
    JSPositionListener(EmersonScript* parent, JSAggregateVisibleDataPtr _jpp, JSCtx* ctx);
    // Disabled default constructor
    JSPositionListener();

    // Invoked after loading is complete, invokes callback if all necessary
    // components are still alive.
    void finishLoadMesh(Liveness::Token alive, Liveness::Token ctx_alive, JSContextStruct* ctx, v8::Persistent<v8::Function> cb, Mesh::VisualPtr data);
};


//Throws an error if jpp has not yet been initialized.
#define CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(funcIn,alternateVal)     \
{\
    if (!jpp)\
    {\
        JSLOG(detailed,"Error in jspositionlistener.  Position proxy was not set."); \
        return alternateVal;\
    }\
}



//Throws an error if jpp has not yet been initialized.
#define CHECK_JPP_INIT_THROW_V8_ERROR(funcIn)\
{\
    if (!jpp)\
    {\
        JSLOG(error,"Error in jspositionlistener.  Position proxy was not set."); \
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when calling " #funcIn ".  Proxy ptr was not set.")));\
    }\
}


//Throws an error if not in context.
//funcIn specifies which function is asking passErrorChecks, and gets printed in
//an error message if call fails.
//If in context, returns current context in con.
#define JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(funcIn,con)\
    CHECK_JPP_INIT_THROW_V8_ERROR(funcIn);\
    if (!v8::Context::InContext())                  \
    {\
        JSLOG(error,"Error in jspositionlistener.  Was not in a context."); \
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error when calling " #funcIn ".  Not currently within a context.")));\
    }\
    v8::Handle<v8::Context>con = v8::Context::GetCurrent();



}//end namespace js
}//end namespace sirikata

#endif
