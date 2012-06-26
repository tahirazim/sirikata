/*  Sirikata
 *  DistributionPingScenario.cpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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

#include "DistributionPingScenario.hpp"
#include "ScenarioFactory.hpp"
#include "SimObjectHost.hpp"
#include "Object.hpp"
#include <sirikata/core/options/Options.hpp>
#include "ConnectedObjectTracker.hpp"

namespace Sirikata{
void DPSInitOptions(DistributionPingScenario *thus) {

    Sirikata::InitializeClassOptions ico("DistributedPingScenario",thus,
                                         new OptionValue("num-pings-per-second","1000",Sirikata::OptionValueType<double>(),"Number of pings launched per simulation second"),
                                         new OptionValue("allow-same-object-host","false",Sirikata::OptionValueType<bool>(),"allow pings to occasionally hit the same object host you are on"),
                                         new OptionValue("force-same-object-host","false",Sirikata::OptionValueType<bool>(),"force pings to only go through 1 spaec server hop"),
                                         new OptionValue("ping-server-distribution","uniform",Sirikata::OptionValueType<String>(),"objects on which space server are chosen?"),
        new OptionValue("ping-size","1024",Sirikata::OptionValueType<uint32>(),"Size of ping payloads.  Doesn't include any other fields in the ping or the object message headers."),
        NULL);
}
DistributionPingScenario::DistributionPingScenario(const String &options)
 : mStartTime(Time::epoch())
{
    mNumTotalPings=0;
    mContext=NULL;
    mObjectTracker = NULL;
    mPingID=0;
    DPSInitOptions(this);
    OptionSet* optionsSet = OptionSet::getOptions("DistributedPingScenario",this);
    optionsSet->parse(options);
    mNumPingsPerSecond=optionsSet->referenceOption("num-pings-per-second")->as<double>();
    mSameObjectHostPings=optionsSet->referenceOption("allow-same-object-host")->as<bool>();
    mForceSameObjectHostPings=optionsSet->referenceOption("force-same-object-host")->as<bool>();

    mPingPayloadSize=optionsSet->referenceOption("ping-size")->as<uint32>();

    mNumGeneratedPings = 0;
    mGeneratePingsStrand = NULL;
    mGeneratePingPoller = NULL;
    mPings = // We allocate space for 1/4 seconds worth of pings
        new Sirikata::SizedThreadSafeQueue<PingInfo,CountResourceMonitor>(
            CountResourceMonitor(std::max((uint32)(mNumPingsPerSecond / 4), (uint32)2))
        );
    mPingPoller = NULL;
    // NOTE: We have this limit because we can get in lock-step with the
    // generator, causing this to run for excessively long when we fall behind
    // on pings.  This allows us to burst to catch up when there is time (and
    // amortize the constant overhead of doing 1 round), but if
    // there is other work to be done we won't make our target rate.
    mMaxPingsPerRound = 20;
    // Do we want to vary this based on mNumPingsPerSecond? 20 is a decent
    // burst, but at 10k/s can take 2ms (we're seeing about 100us/ping
    // currently), which is potentially a long delay for other things in this
    // strand.  If we try to get to 100k, that can be up to 20ms which is very
    // long to block other things on the main strand.
}
DistributionPingScenario::~DistributionPingScenario(){
    delete mPings;
    delete mPingPoller;
    delete mPingProfiler;
}

DistributionPingScenario*DistributionPingScenario::create(const String&options){
    return new DistributionPingScenario(options);
}
void DistributionPingScenario::addConstructorToFactory(ScenarioFactory*thus){
    thus->registerConstructor("ping",&DistributionPingScenario::create);
}

void DistributionPingScenario::initialize(ObjectHostContext*ctx) {
    mContext=ctx;
    mObjectTracker = new ConnectedObjectTracker(mContext->objectHost);
    mPingProfiler = mContext->profiler->addStage("Object Host Send Pings");
    mPingPoller = new Poller(
        ctx->mainStrand,
        std::tr1::bind(&DistributionPingScenario::sendPings, this),
        "DistributionPingScenario Ping Poller",
        Duration::seconds(1.0/mNumPingsPerSecond)
    );

    mGeneratePingProfiler = mContext->profiler->addStage("Object Host Generate Pings");
    mGeneratePingsStrand = mContext->ioService->createStrand("DistributionPingScenario GeneratePings");
    mGeneratePingPoller = new Poller(
        mGeneratePingsStrand,
        std::tr1::bind(&DistributionPingScenario::generatePings, this),
        "DistributionPingScenario Generate Ping Poller",
        Duration::seconds(1.0/mNumPingsPerSecond)
    );
}

void DistributionPingScenario::start() {
    mGeneratePingPoller->start();
    mPingPoller->start();
}
void DistributionPingScenario::stop() {
    mPingPoller->stop();
    mGeneratePingPoller->stop();
}
bool DistributionPingScenario::generateOnePing(ServerID minServer, unsigned int distance, const Time& t, PingInfo* result) {
    unsigned int maxDistance = mObjectTracker->numServerIDs();
    Object * objA = mObjectTracker->randomObjectFromServer((ServerID)minServer);
    Object * objB = mObjectTracker->randomObjectFromServer((ServerID)(minServer+distance));

    if (rand()<RAND_MAX/2) {
        std::swap(objA, objB);
    }
    if (!objA || !objB) {
        return false;
    }

    result->objA = objA->uuid();
    result->objB = objB->uuid();
    result->dist = (objA->location().position(t) - objB->location().position(t)).length();
    return true;
}

void DistributionPingScenario::generatePings() {
    mGeneratePingProfiler->started();

    unsigned int maxDistance = mObjectTracker->numServerIDs();
    unsigned int distance=0;
    if (maxDistance&&((!mSameObjectHostPings)&&(!mForceSameObjectHostPings)))
        maxDistance-=1;
    if (maxDistance>1&&!mForceSameObjectHostPings) {
        distance=(rand()%maxDistance);//uniform distance at random
        if (!mSameObjectHostPings)
            distance+=1;
    }
    unsigned int minServer=(rand()%(maxDistance-distance+1))+1;
    Time newTime=mContext->simTime();
    int64 howManyPings=((newTime-mStartTime).toSeconds()+0.25)*mNumPingsPerSecond;

    Time t(mContext->simTime());

    bool broke=false;
    int64 limit=howManyPings-mNumGeneratedPings;
    int64 i;
    for (i=0;i<limit;++i) {
        PingInfo result;
        if (!mPings->probablyCanPush(result))
            break;
        if (!generateOnePing(minServer,distance, t, &result))
            break;
        mPings->push(result, false);
    }
    mNumGeneratedPings += i;

    mGeneratePingProfiler->finished();
}

void DistributionPingScenario::sendPings() {
    mPingProfiler->started();

    Time newTime=mContext->simTime();
    int64 howManyPings = (newTime-mStartTime).toSeconds()*mNumPingsPerSecond;

    bool broke=false;
    // NOTE: We limit here because we can get in lock-step with the generator,
    // causing this to run for excessively long when we fall behind on pings.
    int64 limit = std::min((int64)howManyPings-mNumTotalPings, mMaxPingsPerRound);
    int64 i;
    for (i=0;i<limit;++i) {
        Time t(mContext->simTime());
        PingInfo result;
        if (!mPings->pop(result)) {
            SILOG(oh,insane,"Ping queue underflowed.");
            break;
        }
        if (!mContext->objectHost->ping(t, result.objA, result.objB, result.dist, mPingPayloadSize))
            break;
    }
    mNumTotalPings += i;

    mPingProfiler->finished();

    static bool printed=false;
    if ( (i-limit) > (10*(int64)mNumPingsPerSecond) && !printed) {
        SILOG(oh,debug,i-limit<<" pending ");
        printed=true;
    }

}

}
