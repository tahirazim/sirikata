
#include "JS_JSMessage.pbj.hpp"
#include "JSSerializer.hpp"
#include <string>
#include "JSUtil.hpp"
#include "JSObjects/JSFields.hpp"
#include "JSObjects/Addressable.hpp"

#include "JSObjectScript.hpp"


/*
  FIXME: If I do not include the JS_Sirikata.pbj.hpp, then just including the
  sirikata/core/util/routablemessagebody.hpp file will cause things to blow up
  (not even using anything in that file, just including it).
 */


#include <v8.h>

namespace Sirikata{
namespace JS{

// static const char* ToCString(const v8::String::Utf8Value& value) {
//   return *value ? *value : "<string conversion failed>";
// }


std:: string JSSerializer::serializeFunction(v8::Local<v8::Function> v8Func)
{
  Sirikata::JS::Protocol::JSMessage jsmessage ;
  Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();

  v8::HandleScope handle_scope;

  v8::Handle<v8::Value> name = v8Func->GetName();
  v8::String::Utf8Value msgBodyArgs1(name);
  const char* cMsgBody1 = ToCString(msgBodyArgs1);
  std::string cStrMsgBody1(cMsgBody1);

  jsf.set_name(cStrMsgBody1);

  v8::Local<v8::Value> value = v8Func->ToString();
  v8::String::Utf8Value msgBodyArgs2(value);
  const char* cMsgBody2 = ToCString(msgBodyArgs2);
  std::string cStrMsgBody2(cMsgBody2);

  Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
  jsf_value.set_s_value(cStrMsgBody2);

  v8::Local<v8::String> proto = v8Func->ObjectProtoToString();
  v8::String::Utf8Value msgBodyArgs3(proto);
  const char* cMsgBody3 = ToCString(msgBodyArgs3);
  std::string cStrMsgBody3(cMsgBody3);

  jsf.set_prototype(cStrMsgBody3);

  std::string serialized_message;
  jsmessage.SerializeToString(&serialized_message);


  return serialized_message;
}

void JSSerializer::serializeAddressable(v8::Local<v8::Object> jsAddressable, Sirikata::JS::Protocol::JSMessage& jsmessage)
{
  JSObjectScript* jsObjectScript;
  SpaceObjectReference* sporef;

  if( !JSAddressable::decodeAddressable(jsAddressable, jsObjectScript, sporef))
  {
    return ; 
  }
  
  // we don't want to serialize jsobjectscript for now

  // serialize SpaceObjectReference

  Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();
  jsf.set_name("spaceobjectreference"); 
  Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
  jsf_value.set_s_value(sporef->toString());

}


void JSSerializer::serializeInternalFields(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::JSMessage& jsmessage)
{
    v8::Local<v8::Value> typeidVal = v8Obj->GetInternalField(TYPEID_FIELD);
    if(typeidVal->IsUndefined() || !typeidVal->IsString())
    {
      return;
    }

    v8::String::Utf8Value u(typeidVal);
    const char* c = ToCString(u);
    std::string typeIdString(c);
      
    Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();
    jsf.set_name("typeId"); 
    Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
    jsf_value.set_s_value(typeIdString);

    // Add additinal fields based on the typeId
    if(typeIdString == "addressable")
    {
      serializeAddressable(v8Obj, jsmessage);  
    }
                
}

std::string JSSerializer::serializeObject(v8::Local<v8::Value> v8Val)
{
    if( v8Val->IsFunction())
    {
        return serializeFunction( v8::Local<v8::Function>::Cast(v8Val));
    }

    v8::HandleScope handle_scope;
    //otherwise assuming it is a v8 object for now
    v8::Local<v8::Object> v8Obj = v8Val->ToObject();
    v8::Local<v8::Array> properties = v8Obj->GetPropertyNames();
    
    JSObjectScript* jso;
    SpaceObjectReference* sporef;
    v8::Local<v8::External> wrapId;

    
    Sirikata::JS::Protocol::JSMessage jsmessage ;
    
    for( unsigned int i = 0; i < properties->Length(); i++)
    {
        v8::Local<v8::Value> value1 = properties->Get(i);

        v8::Local<v8::Value> value2 = v8Obj->Get(properties->Get(i));
        

        // create a JSField out of this
        v8::String::Utf8Value msgBodyArgs1(value1);

        const char* cMsgBody1 = ToCString(msgBodyArgs1);
        std::string cStrMsgBody1(cMsgBody1);


        v8::String::Utf8Value msgBodyArgs2(value2);
        const char* cMsgBody2 = ToCString(msgBodyArgs2);
        std::string cStrMsgBody2(cMsgBody2);


        Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();

                
        jsf.set_name(cStrMsgBody1);
        Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
        jsf_value.set_s_value(cStrMsgBody2);

       

    }

    serializeInternalFields(v8Obj, jsmessage);
    
    
    std::string serialized_message;
    jsmessage.SerializeToString(&serialized_message);

    return serialized_message;
}

/*

bool JSSerializer::deserializeObject( std::string strDecode,v8::Local<v8::Object>& deserializeTo)
{
    Sirikata::JS::Protocol::JSMessage jsmessage;
    bool parseWorked = jsmessage.ParseFromString(strDecode);


    if (! parseWorked)
        return false; //means that we couldn't parse out the message


    for(int i = 0; i < jsmessage.fields_size(); i++)
    {
        
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);

        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();
       
        const char* str = jsf.name().c_str();

        v8::Local<v8::String> key = v8::String::New(str, jsf.name().size());
        if(jsvalue.has_s_value())
        {
            const char* str1 = jsvalue.s_value().c_str();
            v8::Local<v8::String> val = v8::String::New(str1, jsvalue.s_value().size());
            deserializeTo->Set(key, val);
        }
        else if(jsvalue.has_i_value())
        {
            v8::Local<v8::Integer> intval = v8::Integer::New(jsvalue.i_value());
            deserializeTo->Set(key, intval);
        }
    }

    return true;
}
*/


bool JSSerializer::deserializeObject( Sirikata::JS::Protocol::JSMessage jsmessage,v8::Local<v8::Object>& deserializeTo)
{
    //check if there is a typeid field and what is the value for it
    bool isAddressable = false;
    for(int i = 0; i < jsmessage.fields_size(); i++)
    {
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);
        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();
        if(jsf.name() == "typeid")
        {
          deserializeTo->SetInternalField(TYPEID_FIELD, External::New(new std::string(jsvalue.s_value())));
        
          if(jsvalue.s_value() == "addressable")
          {
            isAddressable = true;
            break;
          }
        }
    } 

    for(int i = 0; i < jsmessage.fields_size(); i++)
    {
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);

        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();

        if(jsf.name() == "typeid") { continue; }

        const char* str = jsf.name().c_str();

        v8::Local<v8::String> key = v8::String::New(str, jsf.name().size());
        
        if(jsvalue.has_s_value())
        {
          if(isAddressable && jsf.name() == "spaceobjectreference")
          {
            SpaceObjectReference* sporef = new SpaceObjectReference(jsvalue.s_value());  
            deserializeTo->SetInternalField(ADDRESSABLE_JSOBJSCRIPT_FIELD, External::New(NULL));
            deserializeTo->SetInternalField(ADDRESSABLE_SPACEOBJREF_FIELD, External::New(sporef));
            
          }
          else
          {
            const char* str1 = jsvalue.s_value().c_str();
            v8::Local<v8::String> val = v8::String::New(str1, jsvalue.s_value().size());
            deserializeTo->Set(key, val);
          }

        }
        else if(jsvalue.has_i_value())
        {
            v8::Local<v8::Integer> intval = v8::Integer::New(jsvalue.i_value());
            deserializeTo->Set(key, intval);
        }
    }

    return true;
}




// bool JSSerializer::deserializeObject( MemoryReference payload,v8::Local<v8::Object>& deserializeTo)
// {
//     RoutableMessageBody body;
//     body.ParseFromArray(payload.data(), payload.size());
//     std::string msgString(body.payload());

//     return deserializeObject(msgString,deserializeTo);
// }




}}//end namespaces
