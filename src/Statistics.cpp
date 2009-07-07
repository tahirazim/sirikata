/*  cbr
 *  Statistics.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#include "Statistics.hpp"
#include "Message.hpp"
#include "Options.hpp"
#include <iostream>

#include "ServerNetworkImpl.hpp"

#include "ServerIDMap.hpp"

namespace CBR {

// write the specified number of bytes from the pointer to the buffer
void BatchedBuffer::write(const void* buf, uint32 nbytes) {
    const uint8* bufptr = (const uint8*)buf;
    while( nbytes > 0 ) {
        if (batches.empty() || batches.back()->full())
            batches.push_back( new ByteBatch() );

        ByteBatch* batch = batches.back();
        uint32 avail = batch->max_size - batch->size;
        uint32 to_copy = std::min(avail, nbytes);

        memcpy( &batch->items[batch->size], bufptr, to_copy);
        batch->size += to_copy;
        bufptr += to_copy;
        nbytes -= to_copy;
    }
}

// write the buffer to an ostream
void BatchedBuffer::write(std::ostream& os) {
    for(std::vector<ByteBatch*>::iterator it = batches.begin(); it != batches.end(); it++) {
        ByteBatch* bb = *it;
        os.write( (const char*)&(bb->items[0]) , bb->size );
    }
}


const uint8 Trace::ProximityTag;
const uint8 Trace::LocationTag;
const uint8 Trace::SubscriptionTag;
const uint8 Trace::ServerDatagramQueueInfoTag;
const uint8 Trace::ServerDatagramQueuedTag;
const uint8 Trace::ServerDatagramSentTag;
const uint8 Trace::ServerDatagramReceivedTag;
const uint8 Trace::PacketQueueInfoTag;
const uint8 Trace::PacketSentTag;
const uint8 Trace::PacketReceivedTag;
const uint8 Trace::SegmentationChangeTag;
const uint8 Trace::ObjectBeginMigrateTag;
const uint8 Trace::ObjectAcknowledgeMigrateTag;


static uint64 GetMessageUniqueID(const Network::Chunk& msg) {
    uint64 offset = 0;
    offset += sizeof(ServerMessageHeader);
    offset += 1; // size of msg type

    uint64 uid;
    memcpy(&uid, &msg[offset], sizeof(uint64));
    return uid;
}


Trace::Trace()
 : mServerIDMap(NULL),
   mShuttingDown(false)
{
}

void Trace::setServerIDMap(ServerIDMap* sidmap) {
    mServerIDMap = sidmap;
}

void Trace::prepareShutdown() {
    mShuttingDown = true;
}

void Trace::prox(const Time& t, const UUID& receiver, const UUID& source, bool entered, const TimedMotionVector3f& loc) {
    if (mShuttingDown) return;
    data.write( &ProximityTag, sizeof(ProximityTag) );
    data.write( &t, sizeof(t) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &source, sizeof(source) );
    data.write( &entered, sizeof(entered) );
    data.write( &loc, sizeof(loc) );
}

void Trace::loc(const Time& t, const UUID& receiver, const UUID& source, const TimedMotionVector3f& loc) {
    if (mShuttingDown) return;
    data.write( &LocationTag, sizeof(LocationTag) );
    data.write( &t, sizeof(t) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &source, sizeof(source) );
    data.write( &loc, sizeof(loc) );
}

void Trace::subscription(const Time& t, const UUID& receiver, const UUID& source, bool start) {
    if (mShuttingDown) return;
    data.write( &SubscriptionTag, sizeof(SubscriptionTag) );
    data.write( &t, sizeof(t) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &source, sizeof(source) );
    data.write( &start, sizeof(start) );
}

void Trace::serverDatagramQueueInfo(const Time& t, const ServerID& dest, uint32 send_size, uint32 send_queued, float send_weight, uint32 receive_size, uint32 receive_queued, float receive_weight) {
    if (mShuttingDown) return;
    data.write( &ServerDatagramQueueInfoTag, sizeof(ServerDatagramQueueInfoTag) );
    data.write( &t, sizeof(t) );
    data.write( &dest, sizeof(dest) );
    data.write( &send_size, sizeof(send_size) );
    data.write( &send_queued, sizeof(send_queued) );
    data.write( &send_weight, sizeof(send_weight) );
    data.write( &receive_size, sizeof(receive_size) );
    data.write( &receive_queued, sizeof(receive_queued) );
    data.write( &receive_weight, sizeof(receive_weight) );
}

void Trace::serverDatagramQueued(const Time& t, const ServerID& dest, uint64 id, uint32 size) {
    if (mShuttingDown) return;
    data.write( &ServerDatagramQueuedTag, sizeof(ServerDatagramQueuedTag) );
    data.write( &t, sizeof(t) );
    data.write( &dest, sizeof(dest) );
    data.write( &id, sizeof(id) );
    data.write( &size, sizeof(size) );
}

void Trace::serverDatagramSent(const Time& start_time, const Time& end_time, float weight, const ServerID& dest, const Network::Chunk& data) {
    if (mShuttingDown) return;
    uint64 id = GetMessageUniqueID(data);
    uint32 size = data.size();

    serverDatagramSent(start_time, end_time, weight, dest, id, size);
}

void Trace::serverDatagramSent(const Time& start_time, const Time& end_time, float weight, const ServerID& dest, uint64 id, uint32 size) {
    if (mShuttingDown) return;
    data.write( &ServerDatagramSentTag, sizeof(ServerDatagramSentTag) );
    data.write( &start_time, sizeof(start_time) ); // using either start_time or end_time works since the ranges are guaranteed not to overlap
    data.write( &dest, sizeof(dest) );
    data.write( &id, sizeof(id) );
    data.write( &size, sizeof(size) );
    data.write( &weight, sizeof(weight));
    data.write( &start_time, sizeof(start_time) );
    data.write( &end_time, sizeof(end_time) );
}

void Trace::serverDatagramReceived(const Time& start_time, const Time& end_time, const ServerID& src, uint64 id, uint32 size) {
    if (mShuttingDown) return;
    data.write( &ServerDatagramReceivedTag, sizeof(ServerDatagramReceivedTag) );
    data.write( &start_time, sizeof(start_time) ); // using either start_time or end_time works since the ranges are guaranteed not to overlap
    data.write( &src, sizeof(src) );
    data.write( &id, sizeof(id) );
    data.write( &size, sizeof(size) );
    data.write( &start_time, sizeof(start_time) );
    data.write( &end_time, sizeof(end_time) );
}

void Trace::packetQueueInfo(const Time& t, const Address4& dest, uint32 send_size, uint32 send_queued, float send_weight, uint32 receive_size, uint32 receive_queued, float receive_weight) {
    if (mShuttingDown) return;
    data.write( &PacketQueueInfoTag, sizeof(PacketQueueInfoTag) );
    data.write( &t, sizeof(t) );
    ServerID* dest_server_id = mServerIDMap->lookup(dest);
    assert(dest_server_id);
    data.write( dest_server_id, sizeof(ServerID) );
    data.write( &send_size, sizeof(send_size) );
    data.write( &send_queued, sizeof(send_queued) );
    data.write( &send_weight, sizeof(send_weight) );
    data.write( &receive_size, sizeof(receive_size) );
    data.write( &receive_queued, sizeof(receive_queued) );
    data.write( &receive_weight, sizeof(receive_weight) );
}

void Trace::packetSent(const Time& t, const Address4& dest, uint32 size) {
    if (mShuttingDown) return;
    data.write( &PacketSentTag, sizeof(PacketSentTag) );
    data.write( &t, sizeof(t) );
    assert(mServerIDMap);
    ServerID* dest_server_id = mServerIDMap->lookup(dest);
    assert(dest_server_id);
    data.write( dest_server_id, sizeof(ServerID) );
    data.write( &size, sizeof(size) );
}

void Trace::packetReceived(const Time& t, const Address4& src, uint32 size) {
    if (mShuttingDown) return;
    data.write( &PacketReceivedTag, sizeof(PacketReceivedTag) );
    data.write( &t, sizeof(t) );
    assert(mServerIDMap);
    ServerID* src_server_id = mServerIDMap->lookup(src);
    assert(src_server_id);
    data.write( src_server_id, sizeof(ServerID) );
    data.write( &size, sizeof(size) );
}

void Trace::segmentationChanged(const Time& t, const BoundingBox3f& bbox, const ServerID& serverID){
    if (mShuttingDown) return;
    data.write( &SegmentationChangeTag, sizeof(SegmentationChangeTag) );
    data.write( &t, sizeof(t) );
    data.write( &bbox, sizeof(bbox) );
    data.write( &serverID, sizeof(serverID) );
}

  void Trace::objectBeginMigrate(const Time& t, const UUID& obj_id, const ServerID migrate_from, const ServerID migrate_to)
  {
    if (mShuttingDown) return;

    data.write(&ObjectBeginMigrateTag, sizeof(ObjectBeginMigrateTag));
    data.write(&t, sizeof(t));

    //    printf("\n\n******In Statistics.cpp.  Have an object begin migrate message. \n\n");
    
    data.write(&obj_id, sizeof(obj_id));
    data.write(&migrate_from, sizeof(migrate_from));
    data.write(&migrate_to,sizeof(migrate_to));
  }

  void Trace::objectAcknowledgeMigrate(const Time& t, const UUID& obj_id, const ServerID& acknowledge_from,const ServerID& acknowledge_to)
  //  void Trace::objectAcknowledgeMigrate(const Time& t, const UUID& obj_id, const ServerID acknowledge_from, acknowledge_to)
  {
    if (mShuttingDown) return;

    //    printf("\n\n******In Statistics.cpp.  Have an object ack migrate message. \n\n");
    data.write(&ObjectAcknowledgeMigrateTag, sizeof(ObjectAcknowledgeMigrateTag));
    data.write(&t, sizeof(t));
    data.write(&obj_id, sizeof(obj_id));
    data.write(&acknowledge_from, sizeof(ServerID));
    data.write(&acknowledge_to,sizeof(ServerID));

  }



  
void Trace::save(const String& filename) {
    std::ofstream of(filename.c_str(), std::ios::out | std::ios::binary);

    data.write(of);
}

} // namespace CBR
