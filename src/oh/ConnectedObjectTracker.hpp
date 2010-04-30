/*  cbr
 *  ConnectedObjectTracker.hpp
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_OH_CONNECTED_OBJECT_TRACKER_HPP_
#define _CBR_OH_CONNECTED_OBJECT_TRACKER_HPP_

#include "ObjectHostListener.hpp"
#include <boost/thread/shared_mutex.hpp>

namespace CBR {

/** Thread-safe container for tracking active objects. */
class ConnectedObjectTracker : public ObjectHostListener {
public:
    ConnectedObjectTracker(ObjectHost* parent);
    ~ConnectedObjectTracker();

    // Select random objects uniformly, uniformly from server, using round robin
    Object* randomObject();
    Object* randomObjectFromServer(ServerID whichServer);
    Object* randomObjectExcludingServer(ServerID whichServer, uint max_tries=3);
    Object* roundRobinObject();
    Object* roundRobinObject(ServerID whichServer);
    ServerID numServerIDs() const;
    ///WARNING: expensive (linear search)
    ServerID getServerID(int ObjectsByServerMapNumber);
private:
    void generatePairs();
    Object* getObject(const UUID& objid) const;

    virtual void objectHostConnectedObject(ObjectHost* oh, Object* obj, const ServerID& server);
    virtual void objectHostMigratedObject(ObjectHost* oh, const UUID& objid, const ServerID& from_server, const ServerID& to_server);
    virtual void objectHostDisconnectedObject(ObjectHost* oh, const UUID& objid, const ServerID& server);

    typedef std::tr1::unordered_set<UUID, UUIDHasher> ObjectIDSet;
    typedef std::tr1::unordered_map<ServerID, ObjectIDSet> ObjectsByServerMap;
    typedef std::tr1::unordered_map<UUID, Object*, UUIDHasher> ObjectsByID;

    ObjectHost* mParent;

    boost::shared_mutex mMutex;

    ObjectsByServerMap mObjectsByServer;
    ObjectsByID mObjectsByID;

    UUID mLastRRObject;
};

} // namespace CBR

#endif //_CBR_OH_CONNECTED_OBJECT_TRACKER_HPP_
