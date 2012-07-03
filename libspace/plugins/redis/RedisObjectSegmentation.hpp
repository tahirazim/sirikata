/*  Sirikata
 *  RedisObjectSegmentation.hpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_REDIS_OBJECT_SEGMENTATION_HPP_
#define _SIRIKATA_REDIS_OBJECT_SEGMENTATION_HPP_

#include <sirikata/space/ObjectSegmentation.hpp>
#include <hiredis/async.h>

namespace Sirikata {

class RedisObjectSegmentation : public ObjectSegmentation {
public:
    RedisObjectSegmentation(SpaceContext* con, Network::IOStrand* o_strand, CoordinateSegmentation* cseg, OSegCache* cache, const String& redis_host, uint32 redis_port, const String& redis_prefix);
    ~RedisObjectSegmentation();

    virtual void start();

    virtual OSegEntry cacheLookup(const UUID& obj_id);
    virtual OSegEntry lookup(const UUID& obj_id);

    virtual void addNewObject(const UUID& obj_id, float radius);
    virtual void addMigratedObject(const UUID& obj_id, float radius, ServerID idServerAckTo, bool);
    virtual void removeObject(const UUID& obj_id);

    virtual bool clearToMigrate(const UUID& obj_id);
    virtual void migrateObject(const UUID& obj_id, const OSegEntry& new_server_id);

    virtual void handleMigrateMessageAck(const Sirikata::Protocol::OSeg::MigrateMessageAcknowledge& msg);
    virtual void handleUpdateOSegMessage(const Sirikata::Protocol::OSeg::UpdateOSegMessage& update_oseg_msg);


    // Redis event handlers
    void disconnected();
    void addRead();
    void delRead();
    void addWrite();
    void delWrite();
    void cleanup();

    // Helper handlers, public since redis needs C functions as callbacks, which
    // then invoke these to complete operations.
    void finishReadObject(const UUID& obj_id, const String& data_str);
    void failReadObject(const UUID& obj_id);
    void finishWriteNewObject(const UUID& obj_id, OSegWriteListener::OSegAddNewStatus);
    void finishWriteMigratedObject(const UUID& obj_id, ServerID ackTo);

private:
    void connect();
    void ensureConnected();

    // If the appropriate flag is set, starts and stops read/write operations
    void startRead();
    void startWrite();

    void readHandler(const boost::system::error_code& ec);
    void writeHandler(const boost::system::error_code& ec);

    CoordinateSegmentation* mCSeg;
    OSegCache* mCache;

    typedef std::tr1::unordered_map<UUID, OSegEntry, UUID::Hasher> OSegMap;
    OSegMap mOSeg;

    String mRedisHost;
    uint16 mRedisPort;
    String mRedisPrefix;

    redisAsyncContext* mRedisContext;
    boost::asio::posix::stream_descriptor* mRedisFD; // Wrapped hiredis file descriptor
    bool mReading, mWriting;

    // Read and write events can happen concurrently and can generate callbacks
    // for either type (i.e. when we get a notice that the redis file descriptor
    // can read and call redisAsyncHandleRead, that can invoke both read and
    // write callbacks) and as far as I can tell hiredis isn't thread safe at
    // all. To protect hiredis, we need to add locks in some methods
    // (e.g. lookup) around just the redis command even though all the other
    // data is safe since these commands are known to come from only one strand
    typedef boost::recursive_mutex Mutex;
    typedef boost::lock_guard<Mutex> Lock;
    Mutex mMutex;
};

} // namespace Sirikata

#endif //_SIRIKATA_REDIS_OBJECT_SEGMENTATION_HPP_
