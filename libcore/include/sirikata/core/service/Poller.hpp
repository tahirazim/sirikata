/*  Sirikata
 *  Poller.hpp
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

#ifndef _SIRIKATA_POLLER_HPP_
#define _SIRIKATA_POLLER_HPP_

#include "Service.hpp"
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOStrand.hpp>
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/util/Time.hpp>

namespace Sirikata {

/** Poller allows you to generate a callback periodically without having
 *  to inherit from the PollingService class.  It serves the same function
 *  but requires a new object for every callback instead of using an existing
 *  service.
 *
 *  Normally, the rate of invocations is approximate and essentially assumes
 *  that your callback will execute very quickly. You can also use a more
 *  accurate mode that tries to account for the time spent in your callback and
 *  tries to consistently trigger your callback at the requested period. Even
 *  with the 'accurate' setting, you're still subject to delays due to the event
 *  queue and imprecision from the timer. Further, there is overhead for timing
 *  the user callback. The inaccurate version will only invoke callbacks at a
 *  slower rate, and usually not by much, so you should only use this in special
 *  circumstances.
 */
class SIRIKATA_EXPORT Poller {
public:
    Poller(Network::IOStrand* str, const Network::IOCallback& cb, const char* cb_tag, const Duration& max_rate = Duration::microseconds(0), bool accurate = false);
    virtual ~Poller();

    /** Start polling this service on this strand at the given maximum rate. */
    virtual void start();

    /** Stop scheduling this service. Note that this does not immediately
     *  stop the service, it simply guarantees the service will not
     *  be scheduled again.  This allows outstanding events to be handled
     *  properly.
     */
    virtual void stop();
private:
    void setupNextTimeout(const Duration& user_time);
    void handleExec(const Network::IOTimerWPtr& timer);

    Network::IOStrand* mStrand;
    Network::IOTimerPtr mTimer;
    Duration mMaxRate;
    bool mAccurate;
    bool mUnschedule;
    Network::IOCallback mCB; // Our callback, just saves us from reconstructing it all the time
    Network::IOCallback mUserCB; // The user's callback
    const char* mCBTag;

#if SIRIKATA_DEBUG
    // Track +1 on start(), -1 on stop()
    int8 mPollerRunCount;
#endif
}; // class Poller

} // namespace Sirikata

#endif //_SIRIKATA_POLLER_HPP_
