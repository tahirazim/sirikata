/*  Sirikata
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

#include <sirikata/cbrcore/Statistics.hpp>
#include <sirikata/cbrcore/Message.hpp>
#include <sirikata/cbrcore/Options.hpp>
#include <sirikata/core/options/Options.hpp>

#include <iostream>

#include <boost/thread/locks.hpp>


namespace Sirikata {

BatchedBuffer::BatchedBuffer()
 : filling(NULL)
{
}

// write the specified number of bytes from the pointer to the buffer
void BatchedBuffer::write(const void* buf, uint32 nbytes)
{
    boost::lock_guard<boost::recursive_mutex> lck(mMutex);

    const uint8* bufptr = (const uint8*)buf;
    while( nbytes > 0 ) {
        if (filling == NULL)
            filling = new ByteBatch();

        uint32 to_copy = std::min(filling->avail(), nbytes);

        memcpy( &filling->items[filling->size], bufptr, to_copy);
        filling->size += to_copy;
        bufptr += to_copy;
        nbytes -= to_copy;

        if (filling->full())
            flush();
    }
}

void BatchedBuffer::write(const IOVec* iov, uint32 iovcnt) {
    boost::lock_guard<boost::recursive_mutex> lck(mMutex);

    for(uint32 i = 0; i < iovcnt; i++)
        write(iov[i].base, iov[i].len);
}

void BatchedBuffer::flush() {
    boost::lock_guard<boost::recursive_mutex> lck(mMutex);

    if (filling == NULL)
        return;

    batches.push_back(filling);
    filling = NULL;
}

// write the buffer to an ostream
void BatchedBuffer::store(FILE* os) {
    boost::lock_guard<boost::recursive_mutex> lck(mMutex);

    std::deque<ByteBatch*> bufs;
    batches.swap(bufs);

    for(std::deque<ByteBatch*>::iterator it = bufs.begin(); it != bufs.end(); it++) {
        ByteBatch* bb = *it;
        fwrite((void*)&(bb->items[0]), 1, bb->size, os);
        delete bb;
    }
}


namespace Trace {

OptionValue* Trace::mLogLocProx;
OptionValue* Trace::mLogCSeg;
OptionValue* Trace::mLogPacket;
OptionValue* Trace::mLogMessage;

#define TRACE_LOCPROX_NAME                  "trace-locprox"
#define TRACE_CSEG_NAME                     "trace-cseg"
#define TRACE_PACKET_NAME                   "trace-packet"
#define TRACE_MESSAGE_NAME                  "trace-message"

void Trace::InitOptions() {
    mLogLocProx = new OptionValue(TRACE_LOCPROX_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogCSeg = new OptionValue(TRACE_CSEG_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogPacket = new OptionValue(TRACE_PACKET_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogMessage = new OptionValue(TRACE_MESSAGE_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");

    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(mLogLocProx)
        .addOption(mLogCSeg)
        .addOption(mLogPacket)
        .addOption(mLogMessage)
        ;
}


Trace::Trace(const String& filename)
 : mShuttingDown(false),
   mStorageThread(NULL),
   mFinishStorage(false)
{
    mStorageThread = new Thread( std::tr1::bind(&Trace::storageThread, this, filename) );
}

void Trace::prepareShutdown() {
    mShuttingDown = true;
}

void Trace::shutdown() {
    data.flush();
    mFinishStorage = true;
    mStorageThread->join();
    delete mStorageThread;
}

void Trace::storageThread(const String& filename) {
    FILE* of = fopen(filename.c_str(), "wb");

    while( !mFinishStorage.read() ) {
        data.store(of);
        fflush(of);

#if SIRIKATA_PLATFORM == SIRIKATA_WINDOWS
        Sleep( Duration::seconds(1).toMilliseconds() );
#else
        usleep( Duration::seconds(1).toMicroseconds() );
#endif
    }
    data.store(of);
    fflush(of);

// FIXME #91
#if SIRIKATA_PLATFORM != SIRIKATA_WINDOWS
    fsync(fileno(of));
#endif
    fclose(of);
}

void Trace::writeRecord(uint16 type_hint, BatchedBuffer::IOVec* data_orig, uint32 iovcnt) {
    assert(iovcnt < 30);

    BatchedBuffer::IOVec data_vec[32];

    uint32 total_size = 0;
    for(uint32 i = 0; i < iovcnt; i++)
        total_size += data_orig[i].len;


    data_vec[0] = BatchedBuffer::IOVec(&total_size, sizeof(total_size));
    data_vec[1] = BatchedBuffer::IOVec(&type_hint, sizeof(type_hint));
    for(uint32 i = 0; i < iovcnt; i++)
        data_vec[i+2] = data_orig[i];

    data.write(data_vec, iovcnt+2);
}


std::ostream&Drops::output(std::ostream&output) {
    for (int i=0;i<NUM_DROPS;++i) {
        if (d[i]&&n[i]) {
            output<<n[i]<<':'<<d[i]<<'\n';
        }
    }
    return output;
}

Trace::~Trace() {
    std::cout<<"Summary of drop data:\n";
    drops.output(std::cout)<<"EOF\n";
}


CREATE_TRACE_DEF(Trace, timestampMessageCreation, mLogMessage, const Time&sent, uint64 uid, MessagePath path, ObjectMessagePort srcprt, ObjectMessagePort dstprt) {
    if (mShuttingDown) return;

    const uint32 num_data = 5;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&sent, sizeof(sent)),
        BatchedBuffer::IOVec(&uid, sizeof(uid)),
        BatchedBuffer::IOVec(&path, sizeof(path)),
        BatchedBuffer::IOVec(&srcprt, sizeof(srcprt)),
        BatchedBuffer::IOVec(&dstprt, sizeof(dstprt)),
    };
    writeRecord(MessageCreationTimestampTag, data_vec, num_data);
}

CREATE_TRACE_DEF(Trace, timestampMessage, mLogMessage, const Time&sent, uint64 uid, MessagePath path) {
    if (mShuttingDown) return;

    const uint32 num_data = 3;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&sent, sizeof(sent)),
        BatchedBuffer::IOVec(&uid, sizeof(uid)),
        BatchedBuffer::IOVec(&path, sizeof(path)),
    };
    writeRecord(MessageTimestampTag, data_vec, num_data);
}

CREATE_TRACE_DEF(Trace, serverLoc, mLogLocProx, const Time& t, const ServerID& sender, const ServerID& receiver, const UUID& obj, const TimedMotionVector3f& loc) {
    if (mShuttingDown) return;

    const uint32 num_data = 5;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&sender, sizeof(sender)),
        BatchedBuffer::IOVec(&receiver, sizeof(receiver)),
        BatchedBuffer::IOVec(&obj, sizeof(obj)),
        BatchedBuffer::IOVec(&loc, sizeof(loc)),
    };
    writeRecord(ServerLocationTag, data_vec, num_data);
}

CREATE_TRACE_DEF(Trace, serverObjectEvent, mLogLocProx, const Time& t, const ServerID& source, const ServerID& dest, const UUID& obj, bool added, const TimedMotionVector3f& loc) {
    if (mShuttingDown) return;

    uint8 raw_added = (added ? 1 : 0);

    const uint32 num_data = 6;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&source, sizeof(source)),
        BatchedBuffer::IOVec(&dest, sizeof(dest)),
        BatchedBuffer::IOVec(&obj, sizeof(obj)),
        BatchedBuffer::IOVec(&raw_added, sizeof(raw_added)),
        BatchedBuffer::IOVec(&loc, sizeof(loc)),
    };
    writeRecord(ServerObjectEventTag, data_vec, num_data);
}

CREATE_TRACE_DEF(Trace, segmentationChanged, mLogCSeg, const Time& t, const BoundingBox3f& bbox, const ServerID& serverID){
    if (mShuttingDown) return;

    const uint32 num_data = 3;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&bbox, sizeof(bbox)),
        BatchedBuffer::IOVec(&serverID, sizeof(serverID)),
    };
    writeRecord(SegmentationChangeTag, data_vec, num_data);
}


} // namespace Trace

} // namespace Sirikata
