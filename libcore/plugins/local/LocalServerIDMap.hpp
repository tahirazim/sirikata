/*  Sirikata
 *  LocalServerIDMap.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_LOCAL_SERVERID_MAP_HPP_
#define _SIRIKATA_LOCAL_SERVERID_MAP_HPP_

#include <sirikata/core/network/ServerIDMap.hpp>
#include <fstream>

namespace Sirikata {

/** A LocalServerIDMap implements the ServerIDMap interface for single-server
 * setups: it only provides a single external server (frequently localhost and a
 * port), and no internal addresses since there is no inter-space-server
 * communication. Only a single ServerID is provided.
 */
class LocalServerIDMap : public ServerIDMap {
    ServerID mID;
    Address4 mAddress;
public:
    LocalServerIDMap(const String& server_host, uint16 server_port);
    virtual ~LocalServerIDMap() {}

    virtual ServerID* lookupInternal(const Address4& pos);
    virtual Address4* lookupInternal(const ServerID& obj_id);

    virtual ServerID* lookupExternal(const Address4& pos);
    virtual Address4* lookupExternal(const ServerID& obj_id);

    virtual void __debugPrintInternalIDMap(std::ostream& toPrintFrom);
    virtual void __debugPrintExternalIDMap(std::ostream& toPrintFrom);
};

} // namespace Sirikata

#endif //_SIRIKATA_LOCAL_SERVERID_MAP_HPP_
