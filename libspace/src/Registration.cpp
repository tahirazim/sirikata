/*  Sirikata libspace - Registration Services
 *  Registration.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sirikata/space/Registration.hpp>
#include "Protocol_Sirikata.pbj.hpp"
#include <sirikata/core/util/RoutableMessage.hpp>
#include <sirikata/core/util/KnownServices.hpp>
namespace Sirikata {

Registration::Registration(const SHA256&privateKey):mPrivateKey(privateKey) {

}

Registration::~Registration() {

}

bool Registration::forwardMessagesTo(MessageService*ms) {
    mServices.push_back(ms);
    return true;
}

bool Registration::endForwardingMessagesTo(MessageService*ms) {
    std::vector<MessageService*>::iterator where=std::find(mServices.begin(),mServices.end(),ms);
    if (where==mServices.end())
        return false;
    mServices.erase(where);
    return true;
}

void Registration::processMessage(const RoutableMessageHeader&header,MemoryReference message_body) {
    RoutableMessageBody body;
    if (body.ParseFromArray(message_body.data(),message_body.size())) {
        asyncRegister(header,body);
    }else{
        SILOG(registration,warning,"Unable to parse message body from message originating from "<<header.source_object());
    }
}
void Registration::asyncRegister(const RoutableMessageHeader&header,const RoutableMessageBody& body) {
    RoutableMessageBody retval;
    //for now do so synchronously in a very short-sighted manner.
    int num_messages=body.message_size();
    for (int i=0;i<num_messages;++i) {
        if (body.message_names(i)=="NewObj") {
            Protocol::NewObj newObj;
            newObj.ParseFromString(body.message_arguments(i));
            if (newObj.has_requested_object_loc()&&newObj.has_bounding_sphere()) {
                unsigned char evidence[SHA256::static_size+UUID::static_size];
                UUID private_object_evidence (newObj.object_uuid_evidence());
                std::memcpy(evidence,mPrivateKey.rawData().begin(),SHA256::static_size);
                std::memcpy(evidence+SHA256::static_size,private_object_evidence.getArray().begin(),UUID::static_size);
                RoutableMessageHeader destination_header;
                Protocol::RetObj retObj;
                std::string obj_loc_string;
                newObj.requested_object_loc().SerializeToString(&obj_loc_string);
                retObj.mutable_location().ParseFromString(obj_loc_string);
                retObj.set_bounding_sphere(newObj.bounding_sphere());
                if (private_object_evidence.getArray()[0]==private_object_evidence.getArray()[1]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[2]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[3]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[4]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[5]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[6]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[7]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[8]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[9]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[10]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[11]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[12]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[13]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[14]&&
                    private_object_evidence.getArray()[1]==private_object_evidence.getArray()[15]&&
                    private_object_evidence.getArray()[0]!=0) {
                    retObj.set_object_reference(private_object_evidence);
                }else {
                    retObj.set_object_reference(UUID(SHA256::computeDigest(evidence,sizeof(evidence)).rawData().begin(),UUID::static_size));
                }
                destination_header.set_destination_object(header.source_object());
                destination_header.set_destination_port(header.source_port());
                destination_header.set_source_object(ObjectReference::spaceServiceID());
                destination_header.set_source_port(Services::REGISTRATION);
                if (header.has_id()) {
                    destination_header.set_reply_id(header.id());
                }
                retObj.SerializeToString(retval.add_message("RetObj"));
                std::string return_message;
                retval.SerializeToString(&return_message);
                for (std::vector<MessageService*>::iterator i=mServices.begin(),ie=mServices.end();i!=ie;++i) {
                    (*i)->processMessage(destination_header,MemoryReference(return_message));
                }
            }else {
                SILOG(registration,warning,"Insufficient information in NewObj request"<<body.message_names(i));
            }
        }else if (body.message_names(i)=="DelObj") {
            Protocol::DelObj delObj;
            delObj.ParseFromString(body.message_arguments(i));
            if (delObj.has_object_reference()) {
                if (header.source_object()==ObjectReference::null()) {
                    RoutableMessageHeader destination_header;
                    destination_header.set_destination_object(ObjectReference(delObj.object_reference()));
                    destination_header.set_source_object(ObjectReference::spaceServiceID());
                    destination_header.set_source_port(Services::REGISTRATION);
                    for (std::vector<MessageService*>::reverse_iterator i=mServices.rbegin(),ie=mServices.rend();i!=ie;++i) {
                        std::string body_string;
                        body.SerializeToString(&body_string);
                        (*i)->processMessage(destination_header,MemoryReference(body_string));
                    }
                }
            }
        }else {
            SILOG(registration,warning,"Do not understand message of type "<<body.message_names(i));
        }
    }
}
}
