/*  cbr
 *  Analysis.cpp
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

#include "Analysis.hpp"
#include "Statistics.hpp"
#include "Options.hpp"
#include "MotionPath.hpp"
#include "ObjectFactory.hpp"

namespace CBR {

struct ObjectEvent {
    static ObjectEvent* read(std::istream& is);

    ObjectEvent()
     : time(0)
    {}
    virtual ~ObjectEvent() {}

    Time time;
    UUID receiver;
    UUID source;
};

struct ObjectEventTimeComparator {
    bool operator()(const ObjectEvent* lhs, const ObjectEvent* rhs) const {
        return (lhs->time < rhs->time);
    }
};

struct ProximityEvent : public ObjectEvent {
    bool entered;
};

struct LocationEvent : public ObjectEvent {
    TimedMotionVector3f loc;
};

struct SubscriptionEvent : public ObjectEvent {
    bool started;
};


ObjectEvent* ObjectEvent::read(std::istream& is) {
    char tag;
    is.read( &tag, sizeof(tag) );

    if (!is) return NULL;

    ObjectEvent* evt = NULL;

    if (tag == ObjectTrace::ProximityTag) {
        ProximityEvent* pevt = new ProximityEvent;
        is.read( (char*)&pevt->time, sizeof(pevt->time) );
        is.read( (char*)&pevt->receiver, sizeof(pevt->receiver) );
        is.read( (char*)&pevt->source, sizeof(pevt->source) );
        is.read( (char*)&pevt->entered, sizeof(pevt->entered) );
        evt = pevt;
    }
    else if (tag == ObjectTrace::LocationTag) {
        LocationEvent* levt = new LocationEvent;
        is.read( (char*)&levt->time, sizeof(levt->time) );
        is.read( (char*)&levt->receiver, sizeof(levt->receiver) );
        is.read( (char*)&levt->source, sizeof(levt->source) );
        is.read( (char*)&levt->loc, sizeof(levt->loc) );
        evt = levt;
    }
    else if (tag == ObjectTrace::SubscriptionTag) {
        SubscriptionEvent* sevt = new SubscriptionEvent;
        is.read( (char*)&sevt->time, sizeof(sevt->time) );
        is.read( (char*)&sevt->receiver, sizeof(sevt->receiver) );
        is.read( (char*)&sevt->source, sizeof(sevt->source) );
        is.read( (char*)&sevt->started, sizeof(sevt->started) );
        evt = sevt;
    }
    else {
        assert(false);
    }

    return evt;
}


LocationErrorAnalysis::LocationErrorAnalysis(const char* opt_name, const uint32 nservers) {
    // read in all our data
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            ObjectEvent* evt = ObjectEvent::read(is);
            if (evt == NULL)
                break;

            ObjectEventListMap::iterator it = mEventLists.find( evt->receiver );
            if (it == mEventLists.end()) {
                mEventLists[ evt->receiver ] = new EventList;
                it = mEventLists.find( evt->receiver );
            }
            assert( it != mEventLists.end() );

            EventList* evt_list = it->second;
            evt_list->push_back(evt);
        }
    }

    for(ObjectEventListMap::iterator event_lists_it = mEventLists.begin(); event_lists_it != mEventLists.end(); event_lists_it++) {
        EventList* event_list = event_lists_it->second;
        std::sort(event_list->begin(), event_list->end(), ObjectEventTimeComparator());
    }
}

LocationErrorAnalysis::~LocationErrorAnalysis() {
    for(ObjectEventListMap::iterator event_lists_it = mEventLists.begin(); event_lists_it != mEventLists.end(); event_lists_it++) {
        EventList* event_list = event_lists_it->second;
        for(EventList::iterator events_it = event_list->begin(); events_it != event_list->end(); events_it++)
            delete *events_it;
    }
}

bool LocationErrorAnalysis::observed(const UUID& observer, const UUID& seen) const {
    EventList* events = getEventList(observer);
    if (events == NULL) return false;

    // they were observed if both a proximity entered event and a location update were received
    bool found_prox_entered = false;
    bool found_loc = false;

    for(EventList::iterator event_it = events->begin(); event_it != events->end(); event_it++) {
        ProximityEvent* prox = dynamic_cast<ProximityEvent*>(*event_it);
        if (prox != NULL && prox->entered)
            found_prox_entered = true;

        LocationEvent* loc = dynamic_cast<LocationEvent*>(*event_it);
        if (loc != NULL)
            found_loc = true;
    }

    return (found_prox_entered && found_loc);
}

struct AlwaysUpdatePredicate {
    static float64 maxDist;
    bool operator()(const MotionVector3f& lhs, const MotionVector3f& rhs) const {
        return (lhs.position() - rhs.position()).length() > maxDist;
    }
};

static bool event_matches_prox_entered(ObjectEvent* evt) {
    ProximityEvent* prox = dynamic_cast<ProximityEvent*>(evt);
    return (prox != NULL && prox->entered);
}

static bool event_matches_prox_exited(ObjectEvent* evt) {
    ProximityEvent* prox = dynamic_cast<ProximityEvent*>(evt);
    return (prox != NULL && !prox->entered);
}

static bool event_matches_loc(ObjectEvent* evt) {
    LocationEvent* loc = dynamic_cast<LocationEvent*>(evt);
    return (loc != NULL);
}


// Return the average error in the approximation of an object over its observed period, sampled at the given rate.
double LocationErrorAnalysis::averageError(const UUID& observer, const UUID& seen, const Duration& sampling_rate, ObjectFactory* obj_factory) const {
    /* In this method we run through all the updates, tracking the real path along the way.
     * The main loop iterates over all the updates received, focusing on those dealing with
     * the specified target object.  We have 3 states we can be in
     * 1) Searching for a proximity update indicating the object has entered our region
     * 2) Searching for the first location update after getting that proximity update from
     *    part 1.  Note we also need to see if we get a proximity update indicating the object
     *    exited our region.
     * 3) Sampling.  In this mode, we're progressing forward in time and sampling.  At each event,
     *    we stop to see if we need to update the prediction or if the object has exited our proximity.
     */
    enum Mode {
        SEARCHING_PROX,
        SEARCHING_FIRST_LOC,
        SAMPLING
    };

    assert( observed(observer, seen) );

    MotionPath* true_path = obj_factory->motion(seen);
    TimedMotionVector3f true_motion = true_path->initial();
    const TimedMotionVector3f* next_true_motion = true_path->nextUpdate(true_motion.time());

    EventList* events = getEventList(observer);
    TimedMotionVector3f pred_motion;

    Time cur_time(0);
    Mode mode = SEARCHING_PROX;
    double error_sum = 0.0;
    uint32 sample_count = 0;
    for(EventList::iterator main_it = events->begin(); main_it != events->end(); main_it++) {
        ObjectEvent* cur_event = *main_it;
        if (!(cur_event->source == seen)) continue;

        if (mode == SEARCHING_PROX) {
            if (event_matches_prox_entered(cur_event))
                mode = SEARCHING_FIRST_LOC;
        }
        else if (mode == SEARCHING_FIRST_LOC) {
            if (event_matches_prox_exited(cur_event))
                mode = SEARCHING_PROX;
            else if (event_matches_loc(cur_event)) {
                LocationEvent* loc = dynamic_cast<LocationEvent*>(cur_event);
                pred_motion = loc->loc;
                cur_time = loc->time;
                mode = SAMPLING;
            }
        }
        else if (mode == SAMPLING) {
            Time end_sampling_time = cur_event->time;

            // sample up to the current time
            while( cur_time < end_sampling_time ) {
                // update the true motion vector if necessary, get true position
                while(next_true_motion != NULL && next_true_motion->time() < cur_time) {
                    true_motion = *next_true_motion;
                    next_true_motion = true_path->nextUpdate(true_motion.time());
                }
                Vector3f true_pos = true_motion.extrapolate(cur_time).position();

                // get the predicted position
                Vector3f pred_pos = pred_motion.extrapolate(cur_time).position();

                error_sum += (true_pos - pred_pos).length();
                sample_count++;

                cur_time += sampling_rate;
            }

            if (event_matches_prox_exited(cur_event))
                mode = SEARCHING_PROX;
            else if (event_matches_loc(cur_event)) {
                LocationEvent* loc = dynamic_cast<LocationEvent*>(cur_event);
                if (loc->loc.time() >= pred_motion.time())
                    pred_motion = loc->loc;
            }

            cur_time = end_sampling_time;
        }
    }

    return (sample_count == 0) ? 0 : error_sum / sample_count;
}

double LocationErrorAnalysis::globalAverageError(const Duration& sampling_rate, ObjectFactory* obj_factory) const {
    double total_error = 0.0;
    uint32 total_pairs = 0;
    for(ObjectFactory::iterator observer_it = obj_factory->begin(); observer_it != obj_factory->end(); observer_it++) {
        for(ObjectFactory::iterator seen_it = obj_factory->begin(); seen_it != obj_factory->end(); seen_it++) {
            if (*observer_it == *seen_it) continue;
            if (observed(*observer_it, *seen_it)) {
                double error = averageError(*observer_it, *seen_it, sampling_rate, obj_factory);
                total_error += error;
                total_pairs++;
            }
        }
    }
    return (total_pairs == 0) ? 0 : total_error / total_pairs;
}

LocationErrorAnalysis::EventList* LocationErrorAnalysis::getEventList(const UUID& observer) const {
    ObjectEventListMap::const_iterator event_lists_it = mEventLists.find(observer);
    if (event_lists_it == mEventLists.end()) return NULL;

    return event_lists_it->second;
}

} // namespace CBR
