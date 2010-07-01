#ifndef __SIRIKATA_JS_JSSYSTEM_HPP__
#define _SIRIKATA_JS_JSSYSTEM_HPP__

#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include <v8.h>

namespace Sirikata {
namespace JS {
namespace JSSystem{

v8::Handle<v8::Value> ScriptTimeout(const v8::Arguments& args);

template<typename WithHolderType>
JSObjectScript* GetTargetJSObjectScript(const WithHolderType& with_holder);

v8::Handle<v8::Value> ScriptCreateEntity(const v8::Arguments& args);	
v8::Handle<v8::Value> ScriptReboot(const v8::Arguments& args);	
v8::Handle<v8::Value> ScriptImport(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptGetVisual(v8::Local<v8::String> property, const v8::AccessorInfo &info);
void ScriptSetVisual(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info);
v8::Handle<v8::Value> __ScriptGetTest(const v8::Arguments& args);
v8::Handle<v8::Value> __ScriptTestBroadcastMessage(const v8::Arguments& args);
v8::Handle<v8::Value> Print(const v8::Arguments& args);

v8::Handle<v8::Value> ScriptGetScale(v8::Local<v8::String> property, const v8::AccessorInfo &info);
void ScriptSetScale(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info);
v8::Handle<v8::Value> ScriptGetPosition(v8::Local<v8::String> property, const v8::AccessorInfo &info);
void ScriptSetPosition(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info);
v8::Handle<v8::Value> ScriptGetVelocity(v8::Local<v8::String> property, const v8::AccessorInfo &info);
void ScriptSetVelocity(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info);
v8::Handle<v8::Value> ScriptGetOrientation(v8::Local<v8::String> property, const v8::AccessorInfo &info);
void ScriptSetOrientation(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info);
v8::Handle<v8::Value> ScriptGetAxisOfRotation(v8::Local<v8::String> property, const v8::AccessorInfo &info);
void ScriptSetAxisOfRotation(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info);
v8::Handle<v8::Value> ScriptGetAngularSpeed(v8::Local<v8::String> property, const v8::AccessorInfo &info);
void ScriptSetAngularSpeed(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info);

//v8::Handle<v8::Object> ScriptRegisterHandler(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptRegisterHandler(const v8::Arguments& args);


}}}

#endif
