/*  Sirikata Network Utilities
 *  IOService.hpp
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
#ifndef _SIRIKATA_IOSERVICE_HPP_
#define _SIRIKATA_IOSERVICE_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/IODefs.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>
#include <sirikata/core/util/Noncopyable.hpp>
#include <boost/thread.hpp>

namespace Sirikata {
namespace Network {

/** IOService provides queuing, processing, and dispatch for
 *  asynchronous IO, including timers, sockets, resolvers, and simple
 *  tasks.
 *
 *  Note that currently you cannot add your own services to this.
 *  Therefore, the only way to extend the abilities of the IOService
 *  is via the existing mechanisms -- the default implementations for
 *  sockets and timers or via periodic tasks.
 */
class SIRIKATA_EXPORT IOService : public Noncopyable {
    InternalIOService* mImpl;
    const String mName;

#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    typedef std::tr1::function<void(const boost::system::error_code& e)> IOCallbackWithError;

    AtomicValue<uint32> mTimersEnqueued;
    AtomicValue<uint32> mEnqueued;

    // Track all strands that have been allocated. This needs to be
    // thread safe.
    typedef boost::mutex Mutex;
    typedef boost::lock_guard<Mutex> LockGuard;
    Mutex mMutex;
    typedef std::tr1::unordered_set<IOStrand*> StrandSet;
    StrandSet mStrands;
#endif

    IOService(const IOService&); // Disabled

    // For construction
    friend class IOServiceFactory;
    // For callbacks to track their lifetimes
    friend class IOStrand;

#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    void decrementTimerCount(const boost::system::error_code&e, const IOCallbackWithError& cb);
    void decrementCount(const IOCallback& cb);

    // Invoked by strands when they are being destroyed so we can
    // track which ones are alive.
    void destroyingStrand(IOStrand* child);
#endif

  protected:

    friend class InternalIOStrand;
    friend class InternalIOWork;
    friend class TCPSocket;
    friend class TCPListener;
    friend class TCPResolver;
    friend class UDPSocket;
    friend class UDPResolver;
    friend class DeadlineTimer;
    friend class IOTimer;

public:

    IOService(const String& name);
    ~IOService();

    /** Get the name of this IOService. */
    const String& name() const { return mName; }

    /** Get the underlying IOService.  Only made available to allow for
     *  efficient implementation of ASIO provided functionality such as
     *  tcp/udp sockets and deadline timers.
     */
    InternalIOService& asioService() {
        return *mImpl;
    }
    /** Get the underlying IOService.  Only made available to allow for
     *  efficient implementation of ASIO provided functionality such as
     *  tcp/udp sockets and deadline timers.
     */
    const InternalIOService& asioService() const {
        return *mImpl;
    }

    /** Creates a new IOStrand. */
    IOStrand* createStrand(const String& name);

    /** Run at most one handler in the event queue.
     *  \returns the number of handlers executed
     */
    uint32 pollOne();
    /** Run as many handlers as can be executed without blocking.
     *  \returns the number of handlers executed
     */
    uint32 poll();

    /** Run at most one handler, blocking if appropriate.
     *  \returns the number of handlers executed
     */
    uint32 runOne();
    /** Run as many handlers as are available, blocking as
     *  appropriate.  This will not return unless no more work is
     *  available.
     *  \returns the number of handlers executed
     */
    uint32 run();

    /** Run as many handlers as are available, blocking as
     *  appropriate.  This will not return unless no more work is
     *  available.  Don't return a value. Useful with std::tr1::bind
	 *  where the return value is discared.
     */
    void runNoReturn();

    /** Stop event processing, cancelling events as necessary. */
    void stop();
    /** Reset the event processing, discarding events as necessary and
     *  preparing it for additional run/runOne/poll/pollOne calls.
     */
    void reset();

    /** Request that the given handler be invoked, possibly before
     *  returning.
     *  \param handler the handler callback to be called
     */
    void dispatch(const IOCallback& handler);

    /** Request that the given handler be appended to the event queue
     *  and invoked later.  The handler will not be invoked during
     *  this method call.
     *  \param handler the handler callback to be called
     */
    void post(const IOCallback& handler);
    /** Request that the given handler be appended to the event queue
     *  and invoked later.  Regardless of the wait duration requested,
     *  the handler will never be invoked during this method call.
     *  \param waitFor the length of time to wait before invoking the handler
     *  \param handler the handler callback to be called
     */
    void post(const Duration& waitFor, const IOCallback& handler);

#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    uint32 numTimersEnqueued() const { return mTimersEnqueued.read(); }
    uint32 numEnqueued() const { return mEnqueued.read(); }

    /** Report statistics about this IOService and it's child
     *  IOStrands.
     */
    void reportStats() const;
#endif
};

} // namespace Network
} // namespace Sirikata

#endif //_SIRIKATA_IOSERVICE_HPP_
