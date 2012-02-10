/*  Sirikata Network Utilities
 *  IOTimer.hpp
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
#ifndef _SIRIKATA_IOTIMER_HPP_
#define _SIRIKATA_IOTIMER_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/IODefs.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>
#include <sirikata/core/util/SerializationCheck.hpp>

namespace Sirikata {
namespace Network {

typedef std::tr1::shared_ptr<IOTimer> IOTimerPtr;
typedef std::tr1::weak_ptr<IOTimer> IOTimerWPtr;

/** A timer which handles events using an IOService.  The user specifies
 *  a timeout and a callback and that callback is triggered after at least
 *  the specified duration has passed.  Repeated, periodic callbacks are
 *  supported by specifying a callback up front which will be called at each
 *  timeout.
 *
 *  Because this works with the multithreaded event loop you should be careful
 *  of how use use IOTimers: the handler may be queued to run or running on a
 *  different thread, meaning it can actually run *after* you cancel it. If you
 *  need single-threaded-style semantics, provide an IOStrand for the IOTimer to
 *  use and *only operate on it from within that strand*. When you do this, you
 *  don't have to deal with wrapping your own callbacks: they are guaranteed to
 *  be invoked from the strand you pass in.
 *
 *  Note: Instances of this class should not be stored directly, instead they
 *  must be stored using a shared_ptr<IOTimer> (which is available as IOTimerPtr).
 *  In order to enforce this, you cannot allocate one directly -- instead you
 *  must use the static IOTimer::create() methods.
 */
class SIRIKATA_EXPORT IOTimer : public std::tr1::enable_shared_from_this<IOTimer> {
    DeadlineTimer *mTimer;
    IOStrand* mStrand;
    IOCallback mFunc;
    SerializationCheck chk;

    /**
     * Since the cancel does not always stop the boost callback this adds
     * a level of safety to prevent the callback: This value gets incremented
     * when call cancel.  When creating callback, bind it with the value of
     * callbackToken when at this point.  When executing callback compare the
     * bound value passed through with current value of callbackToken.  If
     * tokens aren't equal, programmer called cancel in intervening time.
     *
     * Note that this is only guaranteed effective if you provide a strand to
     * work with. Otherwise it just reduces the likelihood of executing the
     * callback after the cancel is finished (or of executing it *while*
     * executing the cancel, in separate threads).
     */
    AtomicValue<uint32> mCanceled;

    class TimedOut;


    /** Create a new timer, serviced by the specified IOService.
     *  \param io the IOService to service this timers events
     */
    IOTimer(IOService &io);

    /** Create a new timer, serviced by the specified IOService.
     *  \param io the IOService to service this timers events
     *  \param cb the handler for this timer's events.
     */
    IOTimer(IOService &io, const IOCallback& cb);

    /** Create a new timer, serviced within the given IOStrand.
     *  \param ios the IOStrand to service this timer's events
     */
    IOTimer(IOStrand* ios);

    /** Create a new timer, serviced by the specified IOStrand.
     *  \param ios the IOStrand to service this timer's events
     *  \param cb the handler for this timer's events.
     */
    IOTimer(IOStrand* ios, const IOCallback& cb);

public:
    /** Create a new timer, serviced by the specified IOService.
     *  \param io the IOService to service this timers events
     */
    static IOTimerPtr create(IOService *io);
    static IOTimerPtr create(IOService &io);
    /** Create a new timer, serviced by the specified IOStrand
     *  \param ios the IOStrand to service this timers events
     */
    static IOTimerPtr create(IOStrand *ios);
    static IOTimerPtr create(IOStrand &ios);

    /** Create a new timer, serviced by the specified IOService.
     *  \param io the IOService to service this timers events
     *  \param cb the handler for this timer's events.
     */
    static IOTimerPtr create(IOService *io, const IOCallback& cb);
    static IOTimerPtr create(IOService &io, const IOCallback& cb);
    /** Create a new timer, serviced by the specified IOStrand.
     *  \param io the IOStrand to service this timers events
     *  \param cb the handler for this timer's events.
     */
    static IOTimerPtr create(IOStrand *ios, const IOCallback& cb);
    static IOTimerPtr create(IOStrand &ios, const IOCallback& cb);

    ~IOTimer();

    /** Set the callback which will be used by this timer.  Note that this sets
     *  the callback regardless of the current state of the timer, and will be
     *  used for timeouts currently in progress.
     */
    void setCallback(const IOCallback& cb);

    /** Wait for the specified duration, then call the previously set callback.
     *  \param waitFor the amount of time to wait.
     */
    void wait(const Duration &waitFor);

    /** Wait for the specified duration, then call the previously set callback.
     *  \param waitFor the amount of time to wait.
     *  \param cb the callback to set for this, and future, timeout events.
     */
    void wait(const Duration &waitFor, const IOCallback& cb);

    /** Cancel the current timer.  This will cancel the callback that would
     *  have resulted when the timer expired.
     */
    void cancel();
    Duration expiresFromNow();
};

} // namespace Network
} // namespace Sirikata

#endif //_SIRIKATA_IOTIMER_HPP_
