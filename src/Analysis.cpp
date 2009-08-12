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
#include "AnalysisEvents.hpp"
#include "Utility.hpp"
#include "ServerNetwork.hpp"

namespace CBR {



Event* Event::read(std::istream& is, const ServerID& trace_server_id) {
    char tag;
    is.read( &tag, sizeof(tag) );

    if (!is) return NULL;

    Event* evt = NULL;

    switch(tag) {
      case Trace::SegmentationChangeTag:
          {
  	      SegmentationChangeEvent* sevt = new SegmentationChangeEvent;
	      is.read( (char*)&sevt->time, sizeof(sevt->time) );
	      is.read( (char*)&sevt->bbox, sizeof(sevt->bbox) );
	      is.read( (char*)&sevt->server, sizeof(sevt->server) );
	      evt = sevt;
          }
	  break;
      case Trace::ProximityTag:
          {
              ProximityEvent* pevt = new ProximityEvent;
              is.read( (char*)&pevt->time, sizeof(pevt->time) );
              is.read( (char*)&pevt->receiver, sizeof(pevt->receiver) );
              is.read( (char*)&pevt->source, sizeof(pevt->source) );
              is.read( (char*)&pevt->entered, sizeof(pevt->entered) );
              is.read( (char*)&pevt->loc, sizeof(pevt->loc) );
              evt = pevt;
          }
          break;
      case Trace::LocationTag:
          {
              LocationEvent* levt = new LocationEvent;
              is.read( (char*)&levt->time, sizeof(levt->time) );
              is.read( (char*)&levt->receiver, sizeof(levt->receiver) );
              is.read( (char*)&levt->source, sizeof(levt->source) );
              is.read( (char*)&levt->loc, sizeof(levt->loc) );
              evt = levt;
          }
          break;
      case Trace::SubscriptionTag:
          {
              SubscriptionEvent* sevt = new SubscriptionEvent;
              is.read( (char*)&sevt->time, sizeof(sevt->time) );
              is.read( (char*)&sevt->receiver, sizeof(sevt->receiver) );
              is.read( (char*)&sevt->source, sizeof(sevt->source) );
              is.read( (char*)&sevt->started, sizeof(sevt->started) );
              evt = sevt;
          }
          break;
      case Trace::ServerDatagramQueueInfoTag:
          {
              ServerDatagramQueueInfoEvent* pqievt = new ServerDatagramQueueInfoEvent;
              is.read( (char*)&pqievt->time, sizeof(pqievt->time) );
              pqievt->source = trace_server_id;
              is.read( (char*)&pqievt->dest, sizeof(pqievt->dest) );
              is.read( (char*)&pqievt->send_size, sizeof(pqievt->send_size) );
              is.read( (char*)&pqievt->send_queued, sizeof(pqievt->send_queued) );
              is.read( (char*)&pqievt->send_weight, sizeof(pqievt->send_weight) );
              is.read( (char*)&pqievt->receive_size, sizeof(pqievt->receive_size) );
              is.read( (char*)&pqievt->receive_queued, sizeof(pqievt->receive_queued) );
              is.read( (char*)&pqievt->receive_weight, sizeof(pqievt->receive_weight) );
              evt = pqievt;
          }
          break;
      case Trace::ServerDatagramQueuedTag:
          {
              ServerDatagramQueuedEvent* pqevt = new ServerDatagramQueuedEvent;
              is.read( (char*)&pqevt->time, sizeof(pqevt->time) );
              pqevt->source = trace_server_id;
              is.read( (char*)&pqevt->dest, sizeof(pqevt->dest) );
              is.read( (char*)&pqevt->id, sizeof(pqevt->id) );
              is.read( (char*)&pqevt->size, sizeof(pqevt->size) );
              evt = pqevt;
          }
          break;
      case Trace::ServerDatagramSentTag:
          {
              ServerDatagramSentEvent* psevt = new ServerDatagramSentEvent;
              is.read( (char*)&psevt->time, sizeof(psevt->time) );
              psevt->source = trace_server_id;
              is.read( (char*)&psevt->dest, sizeof(psevt->dest) );
              is.read( (char*)&psevt->id, sizeof(psevt->id) );
              is.read( (char*)&psevt->size, sizeof(psevt->size) );
              is.read( (char*)&psevt->weight, sizeof(psevt->weight) );
              is.read( (char*)&psevt->_start_time, sizeof(psevt->_start_time) );
              is.read( (char*)&psevt->_end_time, sizeof(psevt->_end_time) );
              evt = psevt;
          }
          break;
      case Trace::ServerDatagramReceivedTag:
          {
              ServerDatagramReceivedEvent* prevt = new ServerDatagramReceivedEvent;
              is.read( (char*)&prevt->time, sizeof(prevt->time) );
              is.read( (char*)&prevt->source, sizeof(prevt->source) );
              prevt->dest = trace_server_id;
              is.read( (char*)&prevt->id, sizeof(prevt->id) );
              is.read( (char*)&prevt->size, sizeof(prevt->size) );
              is.read( (char*)&prevt->_start_time, sizeof(prevt->_start_time) );
              is.read( (char*)&prevt->_end_time, sizeof(prevt->_end_time) );
              evt = prevt;
          }
          break;
      case Trace::PacketQueueInfoTag:
          {
              PacketQueueInfoEvent* pqievt = new PacketQueueInfoEvent;
              is.read( (char*)&pqievt->time, sizeof(pqievt->time) );
              pqievt->source = trace_server_id;
              is.read( (char*)&pqievt->dest, sizeof(pqievt->dest) );
              is.read( (char*)&pqievt->send_size, sizeof(pqievt->send_size) );
              is.read( (char*)&pqievt->send_queued, sizeof(pqievt->send_queued) );
              is.read( (char*)&pqievt->send_weight, sizeof(pqievt->send_weight) );
              is.read( (char*)&pqievt->receive_size, sizeof(pqievt->receive_size) );
              is.read( (char*)&pqievt->receive_queued, sizeof(pqievt->receive_queued) );
              is.read( (char*)&pqievt->receive_weight, sizeof(pqievt->receive_weight) );
              evt = pqievt;
          }
          break;
      case Trace::PacketSentTag:
          {
              PacketSentEvent* psevt = new PacketSentEvent;
              is.read( (char*)&psevt->time, sizeof(psevt->time) );
              psevt->source = trace_server_id;
              is.read( (char*)&psevt->dest, sizeof(psevt->dest) );
              is.read( (char*)&psevt->size, sizeof(psevt->size) );
              evt = psevt;
          }
          break;
      case Trace::PacketReceivedTag:
          {
              PacketReceivedEvent* prevt = new PacketReceivedEvent;
              is.read( (char*)&prevt->time, sizeof(prevt->time) );
              is.read( (char*)&prevt->source, sizeof(prevt->source) );
              prevt->dest = trace_server_id;
              is.read( (char*)&prevt->size, sizeof(prevt->size) );
              evt = prevt;
          }
          break;
      case Trace::ObjectBeginMigrateTag:
        {
          ObjectBeginMigrateEvent* objBegMig_evt = new ObjectBeginMigrateEvent;
          is.read( (char*)&objBegMig_evt->time, sizeof(objBegMig_evt->time) );

          is.read((char*) & objBegMig_evt->mObjID, sizeof(objBegMig_evt->mObjID));

          is.read((char*) & objBegMig_evt->mMigrateFrom, sizeof(objBegMig_evt->mMigrateFrom));
          is.read((char*) & objBegMig_evt->mMigrateTo, sizeof(objBegMig_evt->mMigrateTo));

          evt = objBegMig_evt;
        }
        break;


      case Trace::ObjectAcknowledgeMigrateTag:
        {
          ObjectAcknowledgeMigrateEvent* objAckMig_evt = new ObjectAcknowledgeMigrateEvent;
          is.read( (char*)&objAckMig_evt->time, sizeof(objAckMig_evt->time) );

          is.read((char*) &objAckMig_evt->mObjID, sizeof(objAckMig_evt->mObjID) );

          is.read((char*) & objAckMig_evt->mAcknowledgeFrom, sizeof(objAckMig_evt->mAcknowledgeFrom));
          is.read((char*) & objAckMig_evt->mAcknowledgeTo, sizeof(objAckMig_evt->mAcknowledgeTo));

          evt = objAckMig_evt;
        }
        break;

      default:

        std::cout<<"\n*****I got an unknown tag in analysis.cpp.  Value:  "<<tag<<"\n";
        assert(false);
        break;
    }

    return evt;
}


template<typename EventType, typename IteratorType>
class EventIterator {
public:
    typedef EventType event_type;
    typedef IteratorType iterator_type;

    EventIterator(const ServerID& sender, const ServerID& receiver, IteratorType begin, IteratorType end)
     : mSender(sender), mReceiver(receiver), mRangeCurrent(begin), mRangeEnd(end)
    {
        if (mRangeCurrent != mRangeEnd && !currentMatches())
            next();
    }

    EventType* current() {
        if (mRangeCurrent == mRangeEnd)
            return NULL;

        EventType* event = dynamic_cast<EventType*>(*mRangeCurrent);
        assert(event != NULL);
        return event;
    }

    EventType* next() {
        if (mRangeCurrent == mRangeEnd) return NULL;
        do {
            mRangeCurrent++;
        } while(mRangeCurrent != mRangeEnd && !currentMatches());
        return current();
    }

private:
    bool currentMatches() {
        EventType* event = dynamic_cast<EventType*>(*mRangeCurrent);
        if (event == NULL) return false;
        if (event->source != mSender || event->dest != mReceiver) return false;
        return true;
    }

    ServerID mSender;
    ServerID mReceiver;
    IteratorType mRangeCurrent;
    IteratorType mRangeEnd;
};




LocationErrorAnalysis::LocationErrorAnalysis(const char* opt_name, const uint32 nservers) {
    // read in all our data
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            Event* evt = Event::read(is, server_id);
            if (evt == NULL)
                break;
            ObjectEvent* obj_evt = dynamic_cast<ObjectEvent*>(evt);
            if (obj_evt == NULL) {
                delete evt;
                continue;
            }

            ObjectEventListMap::iterator it = mEventLists.find( obj_evt->receiver );
            if (it == mEventLists.end()) {
                mEventLists[ obj_evt->receiver ] = new EventList;
                it = mEventLists.find( obj_evt->receiver );
            }
            assert( it != mEventLists.end() );

            EventList* evt_list = it->second;
            evt_list->push_back(obj_evt);
        }
    }

    for(ObjectEventListMap::iterator event_lists_it = mEventLists.begin(); event_lists_it != mEventLists.end(); event_lists_it++) {
        EventList* event_list = event_lists_it->second;
        std::sort(event_list->begin(), event_list->end(), EventTimeComparator());
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


#define APPARENT_ERROR 1

// Return the average error in the approximation of an object over its observed period, sampled at the given rate.
double LocationErrorAnalysis::averageError(const UUID& observer, const UUID& seen, const Duration& sampling_rate, ObjectFactory* obj_factory) const {
    /* In this method we run through all the updates, tracking the real path along the way.
     * The main loop iterates over all the updates received, focusing on those dealing with
     * the specified target object.  We have 2 states we can be in
     * 1) Searching for a proximity update indicating the object has entered our region,
     *    which will also contain the initial location update
     * 2) Sampling.  In this mode, we're progressing forward in time and sampling.  At each event,
     *    we stop to see if we need to update the prediction or if the object has exited our proximity.
     */
    enum Mode {
        SEARCHING_PROX,
        SAMPLING
    };

    assert( observed(observer, seen) );

    MotionPath* true_path = obj_factory->motion(seen);
    TimedMotionVector3f true_motion = true_path->initial();
    const TimedMotionVector3f* next_true_motion = true_path->nextUpdate(true_motion.time());

    EventList* events = getEventList(observer);
    TimedMotionVector3f pred_motion;

#if APPARENT_ERROR
    MotionPath* observer_path = obj_factory->motion(observer);
    TimedMotionVector3f observer_motion = observer_path->initial();
    const TimedMotionVector3f* next_observer_motion = observer_path->nextUpdate(observer_motion.time());
#endif

    Time cur_time(Time::null());
    Mode mode = SEARCHING_PROX;
    double error_sum = 0.0;
    uint32 sample_count = 0;
    for(EventList::iterator main_it = events->begin(); main_it != events->end(); main_it++) {
        ObjectEvent* cur_event = *main_it;
        if (!(cur_event->source == seen)) continue;

        if (mode == SEARCHING_PROX) {
            if (event_matches_prox_entered(cur_event)) {
                ProximityEvent* prox = dynamic_cast<ProximityEvent*>(cur_event);
                pred_motion = prox->loc;
                cur_time = prox->time;
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

#if APPARENT_ERROR
                // update the observer position if necessary
                while(next_observer_motion != NULL && next_observer_motion->time() < cur_time) {
                    observer_motion = *next_observer_motion;
                    next_observer_motion = observer_path->nextUpdate(observer_motion.time());
                }
                Vector3f observer_pos = observer_motion.extrapolate(cur_time).position();

                Vector3f to_true = (true_pos - observer_pos).normal();
                Vector3f to_pred = (pred_pos - observer_pos).normal();
                float dot_val = to_true.dot(to_pred);
                if (dot_val > 1.0f) dot_val = 1.0f;
                if (dot_val < 1.0f) dot_val = -1.0f;
                error_sum += acos(dot_val);
#else
                error_sum += (true_pos - pred_pos).length();
#endif
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





template<typename EventType, typename EventListType, typename EventListMapType>
void insert_event(EventType* evt, EventListMapType& lists) {
    assert(evt != NULL);

    // put it in the source queue
    typename EventListMapType::iterator source_it = lists.find( evt->source );
    if (source_it == lists.end()) {
        lists[ evt->source ] = new EventListType;
        source_it = lists.find( evt->source );
    }
    assert( source_it != lists.end() );

    EventListType* evt_list = source_it->second;
    evt_list->push_back(evt);

    // put it in the dest queue
    typename EventListMapType::iterator dest_it = lists.find( evt->dest );
    if (dest_it == lists.end()) {
        lists[ evt->dest ] = new EventListType;
        dest_it = lists.find( evt->dest );
    }
    assert( dest_it != lists.end() );

    evt_list = dest_it->second;
    evt_list->push_back(evt);
}

template<typename EventListType, typename EventListMapType>
void sort_events(EventListMapType& lists) {
    for(typename EventListMapType::iterator event_lists_it = lists.begin(); event_lists_it != lists.end(); event_lists_it++) {
        EventListType* event_list = event_lists_it->second;
        std::sort(event_list->begin(), event_list->end(), EventTimeComparator());
    }
}

BandwidthAnalysis::BandwidthAnalysis(const char* opt_name, const uint32 nservers) {
    // read in all our data
    mNumberOfServers = nservers;

    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            Event* evt = Event::read(is, server_id);
            if (evt == NULL)
                break;

            bool used = false;

            ServerDatagramEvent* datagram_evt = dynamic_cast<ServerDatagramEvent*>(evt);
            if (datagram_evt != NULL) {
                used = true;
                insert_event<ServerDatagramEvent, DatagramEventList, ServerDatagramEventListMap>(datagram_evt, mDatagramEventLists);
            }

            PacketEvent* packet_evt = dynamic_cast<PacketEvent*>(evt);
            if (packet_evt != NULL) {
                used = true;
                insert_event<PacketEvent, PacketEventList, ServerPacketEventListMap>(packet_evt, mPacketEventLists);
            }

            ServerDatagramQueueInfoEvent* datagram_qi_evt = dynamic_cast<ServerDatagramQueueInfoEvent*>(evt);
            if (datagram_qi_evt != NULL) {
                used = true;
                insert_event<ServerDatagramQueueInfoEvent, DatagramQueueInfoEventList, ServerDatagramQueueInfoEventListMap>(datagram_qi_evt, mDatagramQueueInfoEventLists);
            }

            PacketQueueInfoEvent* packet_qi_evt = dynamic_cast<PacketQueueInfoEvent*>(evt);
            if (packet_qi_evt != NULL) {
                used = true;
                insert_event<PacketQueueInfoEvent, PacketQueueInfoEventList, ServerPacketQueueInfoEventListMap>(packet_qi_evt, mPacketQueueInfoEventLists);
            }

            if (!used) delete evt;
        }
    }

    // Sort all lists of events by time
    sort_events<DatagramEventList, ServerDatagramEventListMap>(mDatagramEventLists);
    sort_events<PacketEventList, ServerPacketEventListMap>(mPacketEventLists);

    sort_events<DatagramQueueInfoEventList, ServerDatagramQueueInfoEventListMap>(mDatagramQueueInfoEventLists);
    sort_events<PacketQueueInfoEventList, ServerPacketQueueInfoEventListMap>(mPacketQueueInfoEventLists);
}

BandwidthAnalysis::~BandwidthAnalysis() {
    for(ServerDatagramEventListMap::iterator event_lists_it = mDatagramEventLists.begin(); event_lists_it != mDatagramEventLists.end(); event_lists_it++) {
        ServerID server_id = event_lists_it->first;
        DatagramEventList* event_list = event_lists_it->second;
        for(DatagramEventList::iterator events_it = event_list->begin(); events_it != event_list->end(); events_it++) {
            // each event is put in both the source and the dest server event lists,
            // to avoid double deleting, only delete if this is the event's source list
            if ((*events_it)->source == server_id)
                delete *events_it;
        }
    }

    for(ServerPacketEventListMap::iterator event_lists_it = mPacketEventLists.begin(); event_lists_it != mPacketEventLists.end(); event_lists_it++) {
        ServerID server_id = event_lists_it->first;
        PacketEventList* event_list = event_lists_it->second;
        for(PacketEventList::iterator events_it = event_list->begin(); events_it != event_list->end(); events_it++) {
            // each event is put in both the source and the dest server event lists,
            // to avoid double deleting, only delete if this is the event's source list
            if ((*events_it)->source == server_id)
                delete *events_it;
        }
    }
}

const BandwidthAnalysis::DatagramEventList* BandwidthAnalysis::getDatagramEventList(const ServerID& server) const {
    ServerDatagramEventListMap::const_iterator event_lists_it = mDatagramEventLists.find(server);
    if (event_lists_it == mDatagramEventLists.end()) return &mEmptyDatagramEventList;

    return event_lists_it->second;
}

const BandwidthAnalysis::PacketEventList* BandwidthAnalysis::getPacketEventList(const ServerID& server) const {
    ServerPacketEventListMap::const_iterator event_lists_it = mPacketEventLists.find(server);
    if (event_lists_it == mPacketEventLists.end()) return &mEmptyPacketEventList;

    return event_lists_it->second;
}


const BandwidthAnalysis::DatagramQueueInfoEventList* BandwidthAnalysis::getDatagramQueueInfoEventList(const ServerID& server) const {
    ServerDatagramQueueInfoEventListMap::const_iterator event_lists_it = mDatagramQueueInfoEventLists.find(server);
    if (event_lists_it == mDatagramQueueInfoEventLists.end()) return &mEmptyDatagramQueueInfoEventList;

    return event_lists_it->second;
}

const BandwidthAnalysis::PacketQueueInfoEventList* BandwidthAnalysis::getPacketQueueInfoEventList(const ServerID& server) const {
    ServerPacketQueueInfoEventListMap::const_iterator event_lists_it = mPacketQueueInfoEventLists.find(server);
    if (event_lists_it == mPacketQueueInfoEventLists.end()) return &mEmptyPacketQueueInfoEventList;

    return event_lists_it->second;
}

BandwidthAnalysis::DatagramEventList::const_iterator BandwidthAnalysis::datagramBegin(const ServerID& server) const {
    return getDatagramEventList(server)->begin();
}

BandwidthAnalysis::DatagramEventList::const_iterator BandwidthAnalysis::datagramEnd(const ServerID& server) const {
    return getDatagramEventList(server)->end();
}

BandwidthAnalysis::PacketEventList::const_iterator BandwidthAnalysis::packetBegin(const ServerID& server) const {
    return getPacketEventList(server)->begin();
}

BandwidthAnalysis::PacketEventList::const_iterator BandwidthAnalysis::packetEnd(const ServerID& server) const {
    return getPacketEventList(server)->end();
}

BandwidthAnalysis::DatagramQueueInfoEventList::const_iterator BandwidthAnalysis::datagramQueueInfoBegin(const ServerID& server) const {
    return getDatagramQueueInfoEventList(server)->begin();
}

BandwidthAnalysis::DatagramQueueInfoEventList::const_iterator BandwidthAnalysis::datagramQueueInfoEnd(const ServerID& server) const {
    return getDatagramQueueInfoEventList(server)->end();
}

BandwidthAnalysis::PacketQueueInfoEventList::const_iterator BandwidthAnalysis::packetQueueInfoBegin(const ServerID& server) const {
    return getPacketQueueInfoEventList(server)->begin();
}

BandwidthAnalysis::PacketQueueInfoEventList::const_iterator BandwidthAnalysis::packetQueueInfoEnd(const ServerID& server) const {
    return getPacketQueueInfoEventList(server)->end();
}

template<typename EventType, typename EventIteratorType>
void computeRate(const ServerID& sender, const ServerID& receiver, const EventIteratorType& filter_begin, const EventIteratorType& filter_end) {
    uint64 total_bytes = 0;
    uint32 last_bytes = 0;
    Duration last_duration;
    Time last_time(Time::null());
    double max_bandwidth = 0;
    for(EventIterator<EventType, EventIteratorType> event_it(sender, receiver, filter_begin, filter_end); event_it.current() != NULL; event_it.next() ) {
        EventType* p_evt = event_it.current();

        total_bytes += p_evt->size;

        if (p_evt->time != last_time) {
            double bandwidth = (double)last_bytes / last_duration.toSeconds();
            if (bandwidth > max_bandwidth)
                max_bandwidth = bandwidth;

            last_bytes = 0;
            last_duration = p_evt->time - last_time;
            last_time = p_evt->time;
        }

        last_bytes += p_evt->size;
    }

    printf("%d to %d: %ld total, %f max\n", sender, receiver, total_bytes, max_bandwidth);
}


void BandwidthAnalysis::computeSendRate(const ServerID& sender, const ServerID& receiver) const {
    if (getDatagramEventList(sender) == NULL ) {
        printf("%d to %d: unknown total, unknown max\n", sender, receiver);
	return;
    }

    computeRate<ServerDatagramSentEvent, DatagramEventList::const_iterator>(sender, receiver, datagramBegin(sender), datagramEnd(sender));
}

void BandwidthAnalysis::computeReceiveRate(const ServerID& sender, const ServerID& receiver) const {
    if (getDatagramEventList(receiver) == NULL ) {
        printf("%d to %d: unknown total, unknown max\n", sender, receiver);
	return;
    }

    computeRate<ServerDatagramReceivedEvent, DatagramEventList::const_iterator>(sender, receiver, datagramBegin(receiver), datagramEnd(receiver));
}

// note: swap_sender_receiver optionally swaps order for sake of graphing code, generally will be used when collecting stats for "receiver" side
template<typename EventType, typename EventIteratorType>
void computeWindowedRate(const ServerID& sender, const ServerID& receiver, const EventIteratorType& filter_begin, const EventIteratorType& filter_end, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out, bool swap_sender_receiver) {
    EventIterator<EventType, EventIteratorType> event_it(sender, receiver, filter_begin, filter_end);
    std::queue<EventType*> window_events;

    uint64 bytes = 0;
    uint64 total_bytes = 0;
    double max_bandwidth = 0;

    for(Time window_center = start_time; window_center < end_time; window_center += sample_rate) {
        Time window_end = window_center + window / 2.f;

        // add in any new packets that now fit in the window
        uint32 last_packet_partial_size = 0;
        while(true) {
            EventType* evt = event_it.current();
            if (evt == NULL) break;
            if (evt->end_time() > window_end) {
                if (evt->begin_time() + window < window_end) {
                    double packet_frac = (window_end - evt->begin_time()).toSeconds() / (evt->end_time() - evt->begin_time()).toSeconds();
                    last_packet_partial_size = evt->size * packet_frac;
                }
                break;
            }
            bytes += evt->size;
            total_bytes += evt->size;
            window_events.push(evt);
            event_it.next();
        }

        // subtract out any packets that have fallen out of the window
        // note we use event_time + window < window_end because subtracting could underflow the time
        uint32 first_packet_partial_size = 0;
        while(!window_events.empty()) {
            EventType* pevt = window_events.front();
            if (pevt->begin_time() + window >= window_end) break;

            bytes -= pevt->size;
            window_events.pop();

            if (pevt->end_time() + window >= window_end) {
                // note the order of the numerator is important to avoid underflow
                double packet_frac = (pevt->end_time() + window - window_end).toSeconds() / (pevt->end_time() - pevt->begin_time()).toSeconds();
                first_packet_partial_size = pevt->size * packet_frac;
                break;
            }
        }

        // finally compute the current bandwidth
        double bandwidth = (first_packet_partial_size + bytes + last_packet_partial_size) / window.toSeconds();
        if (bandwidth > max_bandwidth)
            max_bandwidth = bandwidth;

        // optionally swap order for sake of graphing code, generally will be used when collecting stats for "receiver" side
        if (swap_sender_receiver)
            detail_out << receiver << " " << sender << " ";
        else
            detail_out << sender << " " << receiver << " ";
        detail_out << (window_center-Time::null()).toMilliseconds() << " " << bandwidth << std::endl;
    }

    // optionally swap order for sake of graphing code, generally will be used when collecting stats for "receiver" side
    if (swap_sender_receiver)
        summary_out << receiver << " from " << sender << ": ";
    else
        summary_out << sender << " to " << receiver << ": ";
    summary_out << total_bytes << " total, " << max_bandwidth << " max" << std::endl;
}

template<typename EventType, typename EventIteratorType>
void BandwidthAnalysis::computeJFI(const ServerID& sender, const ServerID& filter) const {
      uint64 total_bytes = 0;

      float sum = 0;
      float sum_of_squares = 0;

      for (uint32 receiver = 1; receiver <= mNumberOfServers; receiver++) {
        if (receiver != sender) {
          total_bytes = 0;
          float weight = 0;
          if (getDatagramEventList(receiver) == NULL) continue;

          for (EventIterator<EventType, EventIteratorType> event_it(sender, receiver, datagramBegin(receiver), datagramEnd(receiver)); event_it.current() != NULL; event_it.next() )
            {
              EventType* p_evt = event_it.current();

              total_bytes += p_evt->size;

              weight = p_evt->weight;


            }

          sum += total_bytes/weight;
          sum_of_squares += (total_bytes/weight) * (total_bytes/weight);
        }
      }

      if ( (mNumberOfServers-1)*sum_of_squares != 0) {
          printf("JFI for sender %d is %f\n", sender, (sum*sum/((mNumberOfServers-1)*sum_of_squares) ) );
      }
      else {
          printf("JFI for sender %d cannot be computed\n", sender);
      }
}



void BandwidthAnalysis::computeWindowedDatagramSendRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    computeWindowedRate<ServerDatagramSentEvent, DatagramEventList::const_iterator>(sender, receiver, datagramBegin(sender), datagramEnd(sender), window, sample_rate, start_time, end_time, summary_out, detail_out, false);
}

void BandwidthAnalysis::computeWindowedDatagramReceiveRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    computeWindowedRate<ServerDatagramReceivedEvent, DatagramEventList::const_iterator>(sender, receiver, datagramBegin(receiver), datagramEnd(receiver), window, sample_rate, start_time, end_time, summary_out, detail_out, true);
}

void BandwidthAnalysis::computeWindowedPacketSendRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    computeWindowedRate<PacketSentEvent, PacketEventList::const_iterator>(sender, receiver, packetBegin(sender), packetEnd(sender), window, sample_rate, start_time, end_time, summary_out, detail_out, false);
}

void BandwidthAnalysis::computeWindowedPacketReceiveRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    computeWindowedRate<PacketReceivedEvent, PacketEventList::const_iterator>(sender, receiver, packetBegin(receiver), packetEnd(receiver), window, sample_rate, start_time, end_time, summary_out, detail_out, true);
}

void BandwidthAnalysis::computeJFI(const ServerID& sender) const {
  computeJFI<ServerDatagramSentEvent, DatagramEventList::const_iterator>(sender, sender);
}


template<typename EventType, typename EventIteratorType>
void dumpQueueInfo(const ServerID& sender, const ServerID& receiver, const EventIteratorType& filter_begin, const EventIteratorType& filter_end, std::ostream& summary_out, std::ostream& detail_out) {
    EventType* q_evt = NULL;
    EventIterator<EventType, EventIteratorType> event_it(sender, receiver, filter_begin, filter_end);

    while((q_evt = event_it.current()) != NULL) {
        detail_out << sender << " " << receiver << " " << (q_evt->time-Time::null()).toMilliseconds() << " " << q_evt->send_size << " " << q_evt->send_queued << " " << q_evt->send_weight << " " << q_evt->receive_size << " " << q_evt->receive_queued << " " << q_evt->receive_weight << std::endl;
        event_it.next();
    }
    //summary_out << std::endl;
}

void BandwidthAnalysis::dumpDatagramQueueInfo(const ServerID& sender, const ServerID& receiver, std::ostream& summary_out, std::ostream& detail_out) {
    dumpQueueInfo<ServerDatagramQueueInfoEvent, DatagramQueueInfoEventList::const_iterator>(sender, receiver, datagramQueueInfoBegin(sender), datagramQueueInfoEnd(sender), summary_out, detail_out);
}

void BandwidthAnalysis::dumpPacketQueueInfo(const ServerID& sender, const ServerID& receiver, std::ostream& summary_out, std::ostream& detail_out) {
    dumpQueueInfo<PacketQueueInfoEvent, PacketQueueInfoEventList::const_iterator>(sender, receiver, packetQueueInfoBegin(sender), packetQueueInfoEnd(sender), summary_out, detail_out);
}


// note: swap_sender_receiver optionally swaps order for sake of graphing code, generally will be used when collecting stats for "receiver" side
template<typename EventType, typename EventIteratorType, typename ValueFunctor>
void windowedQueueInfo(const ServerID& sender, const ServerID& receiver, const EventIteratorType& filter_begin, const EventIteratorType& filter_end, const ValueFunctor& value_func, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out, bool swap_sender_receiver) {
    EventIterator<EventType, EventIteratorType> event_it(sender, receiver, filter_begin, filter_end);
    std::queue<EventType*> window_events;

    uint64 window_queued_bytes = 0;
    float window_weight = 0.f;

    for(Time window_center = start_time; window_center < end_time; window_center += sample_rate) {
        Time window_end = window_center + window / 2.f;

        // add in any new packets that now fit in the window
        while(true) {
            EventType* evt = event_it.current();
            if (evt == NULL) break;
            if (evt->end_time() > window_end)
                break;
            window_queued_bytes += value_func.queued(evt);
            window_weight += value_func.weight(evt);
            window_events.push(evt);
            event_it.next();
        }

        // subtract out any packets that have fallen out of the window
        // note we use event_time + window < window_end because subtracting could underflow the time
        while(!window_events.empty()) {
            EventType* pevt = window_events.front();
            if (pevt->begin_time() + window >= window_end) break;

            window_queued_bytes -= value_func.queued(pevt);
            window_weight -= value_func.weight(pevt);
            window_events.pop();
        }

        // finally compute the current values
        uint64 window_queued_avg = (window_events.size() == 0) ? 0 : window_queued_bytes / window_events.size();
        float window_weight_avg = (window_events.size() == 0) ? 0 : window_weight / window_events.size();

        // optionally swap order for sake of graphing code, generally will be used when collecting stats for "receiver" side
        if (swap_sender_receiver)
            detail_out << receiver << " " << sender << " ";
        else
            detail_out << sender << " " << receiver << " ";
        detail_out << (window_center-Time::null()).toMilliseconds() << " " << window_queued_avg << " " << window_weight_avg << std::endl;
    }

    // FIXME we should probably output *something* for summary_out
}


template<typename EventType>
struct SendQueueFunctor {
    uint32 size(const EventType* evt) const {
        return evt->send_size;
    }
    uint32 queued(const EventType* evt) const {
        return evt->send_queued;
    }
    float weight(const EventType* evt) const {
        return evt->send_weight;
    }
};

template<typename EventType>
struct ReceiveQueueFunctor {
    uint32 size(const EventType* evt) const {
        return evt->receive_size;
    }
    uint32 queued(const EventType* evt) const {
        return evt->receive_queued;
    }
    float weight(const EventType* evt) const {
        return evt->receive_weight;
    }
};


void BandwidthAnalysis::windowedDatagramSendQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    windowedQueueInfo<ServerDatagramQueueInfoEvent, DatagramQueueInfoEventList::const_iterator, SendQueueFunctor<ServerDatagramQueueInfoEvent> >(
        sender, receiver,
        datagramQueueInfoBegin(sender), datagramQueueInfoEnd(sender),
        SendQueueFunctor<ServerDatagramQueueInfoEvent>(),
        window, sample_rate, start_time, end_time,
        summary_out, detail_out,
        false
    );
}

void BandwidthAnalysis::windowedDatagramReceiveQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    windowedQueueInfo<ServerDatagramQueueInfoEvent, DatagramQueueInfoEventList::const_iterator, ReceiveQueueFunctor<ServerDatagramQueueInfoEvent> >(
        sender, receiver,
        datagramQueueInfoBegin(receiver), datagramQueueInfoEnd(receiver),
        ReceiveQueueFunctor<ServerDatagramQueueInfoEvent>(),
        window, sample_rate, start_time, end_time,
        summary_out, detail_out,
        true
    );
}

void BandwidthAnalysis::windowedPacketSendQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    windowedQueueInfo<PacketQueueInfoEvent, PacketQueueInfoEventList::const_iterator, SendQueueFunctor<PacketQueueInfoEvent> >(
        sender, receiver,
        packetQueueInfoBegin(sender), packetQueueInfoEnd(sender),
        SendQueueFunctor<PacketQueueInfoEvent>(),
        window, sample_rate, start_time, end_time,
        summary_out, detail_out,
        false
    );
}

void BandwidthAnalysis::windowedPacketReceiveQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    windowedQueueInfo<PacketQueueInfoEvent, PacketQueueInfoEventList::const_iterator, ReceiveQueueFunctor<PacketQueueInfoEvent> >(
        sender, receiver,
        packetQueueInfoBegin(receiver), packetQueueInfoEnd(receiver),
        ReceiveQueueFunctor<PacketQueueInfoEvent>(),
        window, sample_rate, start_time, end_time,
        summary_out, detail_out,
        true
    );
}


LatencyAnalysis::PacketData::PacketData()
 :_send_start_time(Time::null()),
  _send_end_time(Time::null()),
  _receive_start_time(Time::null()),
  _receive_end_time(Time::null())
{
    mSize=0;
    mId=0;
}
void LatencyAnalysis::PacketData::addPacketSentEvent(ServerDatagramQueuedEvent*sde) {
    mSize=sde->size;
    mId=sde->id;
    source=sde->source;
    dest=sde->dest;
    if (_send_start_time==Time::null()||_send_start_time>=sde->begin_time()) {
        _send_start_time=sde->time;
        _send_end_time=sde->time;
    }
}
void LatencyAnalysis::PacketData::addPacketReceivedEvent(ServerDatagramReceivedEvent*sde) {
    mSize=sde->size;
    mId=sde->id;
    source=sde->source;
    dest=sde->dest;
    if (_receive_end_time==Time::null()||_receive_end_time<=sde->end_time()) {
        _receive_start_time=sde->begin_time();
        _receive_end_time=sde->end_time();
    }
}
LatencyAnalysis::LatencyAnalysis(const char* opt_name, const uint32 nservers) {
    // read in all our data
    mNumberOfServers = nservers;
    std::tr1::unordered_map<uint64,PacketData> packetFlow;
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            Event* evt = Event::read(is, server_id);
            if (evt == NULL)
                break;


            {
                ServerDatagramReceivedEvent* datagram_evt = dynamic_cast<ServerDatagramReceivedEvent*>(evt);
                if (datagram_evt != NULL) {
                    packetFlow[datagram_evt->id].addPacketReceivedEvent(datagram_evt);
                }
            }
            {
                ServerDatagramQueuedEvent* datagram_evt = dynamic_cast<ServerDatagramQueuedEvent*>(evt);
                if (datagram_evt != NULL) {
                    packetFlow[datagram_evt->id].addPacketSentEvent(datagram_evt);
                }
            }
            PacketEvent* packet_evt = dynamic_cast<PacketEvent*>(evt);
            if (packet_evt != NULL) {
                //insert_event<PacketEvent, PacketEventList, ServerPacketEventListMap>(packet_evt, mPacketEventLists);
            }
            ServerDatagramQueueInfoEvent* datagram_qi_evt = dynamic_cast<ServerDatagramQueueInfoEvent*>(evt);
            if (datagram_qi_evt != NULL) {
                //insert_event<ServerDatagramQueueInfoEvent, DatagramQueueInfoEventList, ServerDatagramQueueInfoEventListMap>(datagram_qi_evt, mDatagramQueueInfoEventLists);
            }

            PacketQueueInfoEvent* packet_qi_evt = dynamic_cast<PacketQueueInfoEvent*>(evt);
            if (packet_qi_evt != NULL) {
                //insert_event<PacketQueueInfoEvent, PacketQueueInfoEventList, ServerPacketQueueInfoEventListMap>(packet_qi_evt, mPacketQueueInfoEventLists);
            }

            delete evt;
        }
    }
    for (std::tr1::unordered_map<uint64,PacketData>::iterator i=packetFlow.begin(),ie=packetFlow.end();i!=ie;++i) {
        mServerPairPacketMap.insert(ServerPairPacketMap::value_type(SourceDestinationPair(i->second.source,i->second.dest ),
                                                                    i->second));
    }
    packetFlow.clear();

    struct ServerLatencyInfo {
        Duration in_latency;
        size_t in_samples;
        Duration out_latency;
        size_t out_samples;
    };

    ServerLatencyInfo* server_latencies = new ServerLatencyInfo[nservers];
    memset(server_latencies, 0, nservers * sizeof(ServerLatencyInfo));

    Time epoch(Time::null());
    for(uint32 source_id = 1; source_id <= nservers; source_id++) {
        for(uint32 dest_id = 1; dest_id <= nservers; dest_id++) {
            Duration latency;
            size_t numSamples=0;
            size_t numUnfinished=0;
            SourceDestinationPair key(source_id,dest_id);
            ServerPairPacketMap::iterator i=mServerPairPacketMap.find(key);
            for (;i!=mServerPairPacketMap.end()&&i->first== key;++i) {
                if (i->second._receive_end_time!=epoch&&i->second._send_start_time!=epoch) {
                    PacketData dat=i->second;
                    Duration delta=i->second._receive_end_time-i->second._send_start_time;
                    if (delta>Duration::seconds(0.0f)) {
                        ++numSamples;
                        latency+=delta;
                    }else if (delta<Duration::seconds(0.0f)){
                        printf ("Packet with id %ld, negative duration %f (%f %f)->(%f %f) %d (%f %f) (%f %f)\n",dat.mId,delta.toSeconds(),0.,(dat._send_end_time-dat._send_start_time).toSeconds(),(dat._receive_start_time-dat._send_start_time).toSeconds(),(dat._receive_end_time-dat._send_start_time).toSeconds(),dat.mSize, (dat._send_start_time-Time::null()).toSeconds(),(dat._send_end_time-Time::null()).toSeconds(),(dat._receive_start_time-Time::null()).toSeconds(),(dat._receive_end_time-Time::null()).toSeconds());
                    }
                }else{
                    ++numUnfinished;
                    if (i->second._receive_end_time==epoch&&i->second._send_start_time==epoch) {
                        printf ("Packet with uninitialized duration\n");
                    }
                }
            }

            server_latencies[source_id].out_latency += latency;
            server_latencies[source_id].out_samples += numSamples;
            server_latencies[dest_id].in_latency += latency;
            server_latencies[dest_id].in_samples += numSamples;

            if (numSamples)
                latency/=(double)numSamples;
            std::cout << "Server "<<source_id<<" to "<<dest_id<<" : "<<latency<<" ("<<numSamples<<","<<numUnfinished<<")"<<std::endl;
        }
    }

    for(uint32 serv_id = 1; serv_id <= nservers; serv_id++) {
        if (server_latencies[serv_id].in_samples)
            std::cout << "Server "<<serv_id<<" In: "<<server_latencies[serv_id].in_latency/(double)server_latencies[serv_id].in_samples<<" ("<<server_latencies[serv_id].in_samples<<")"<<std::endl;
        if (server_latencies[serv_id].out_samples)
            std::cout << "Server "<<serv_id<<" Out: "<<server_latencies[serv_id].out_latency/(double)server_latencies[serv_id].out_samples<<" ("<<server_latencies[serv_id].out_samples<<")"<<std::endl;
    }

    // Sort all lists of events by time
    /*    sort_events<DatagramEventList, ServerDatagramEventListMap>(mDatagramEventLists);
    sort_events<PacketEventList, ServerPacketEventListMap>(mPacketEventLists);

    sort_events<DatagramQueueInfoEventList, ServerDatagramQueueInfoEventListMap>(mDatagramQueueInfoEventLists);
    sort_events<PacketQueueInfoEventList, ServerPacketQueueInfoEventListMap>(mPacketQueueInfoEventLists);
    */
}

LatencyAnalysis::~LatencyAnalysis() {
/*
    for(ServerDatagramEventListMap::iterator event_lists_it = mDatagramEventLists.begin(); event_lists_it != mDatagramEventLists.end(); event_lists_it++) {
        ServerID server_id = event_lists_it->first;
        DatagramEventList* event_list = event_lists_it->second;
        for(DatagramEventList::iterator events_it = event_list->begin(); events_it != event_list->end(); events_it++) {
            // each event is put in both the source and the dest server event lists,
            // to avoid double deleting, only delete if this is the event's source list
            if ((*events_it)->source == server_id)
                delete *events_it;
        }
    }

    for(ServerPacketEventListMap::iterator event_lists_it = mPacketEventLists.begin(); event_lists_it != mPacketEventLists.end(); event_lists_it++) {
        ServerID server_id = event_lists_it->first;
        PacketEventList* event_list = event_lists_it->second;
        for(PacketEventList::iterator events_it = event_list->begin(); events_it != event_list->end(); events_it++) {
            // each event is put in both the source and the dest server event lists,
            // to avoid double deleting, only delete if this is the event's source list
            if ((*events_it)->source == server_id)
                delete *events_it;
        }
    }
*/
}





  //object move messages
  ObjectSegmentationAnalysis::ObjectSegmentationAnalysis(const char* opt_name, const uint32 nservers)
  {

    for(uint32 server_id = 1; server_id <= nservers; server_id++)
    {
      String loc_file = GetPerServerFile(opt_name, server_id);
      std::ifstream is(loc_file.c_str(), std::ios::in);

      while(is)
      {
        Event* evt = Event::read(is, server_id);
        if (evt == NULL)
          break;


        ObjectBeginMigrateEvent* obj_mig_evt = dynamic_cast<ObjectBeginMigrateEvent*>(evt);
        if (obj_mig_evt != NULL)
        {
          objectBeginMigrateTimes.push_back(obj_mig_evt->time);

          objectBeginMigrateID.push_back(obj_mig_evt->mObjID);
          objectBeginMigrateMigrateFrom.push_back(obj_mig_evt->mMigrateFrom);
          objectBeginMigrateMigrateTo.push_back(obj_mig_evt->mMigrateTo);

        }

        ObjectAcknowledgeMigrateEvent* obj_ack_mig_evt = dynamic_cast<ObjectAcknowledgeMigrateEvent*> (evt);
        if (obj_ack_mig_evt != NULL)
        {
          objectAcknowledgeMigrateTimes.push_back(obj_ack_mig_evt->time);

          objectAcknowledgeMigrateID.push_back(obj_ack_mig_evt->mObjID);
          objectAcknowledgeAcknowledgeFrom.push_back(obj_ack_mig_evt->mAcknowledgeFrom);
          objectAcknowledgeAcknowledgeTo.push_back(obj_ack_mig_evt->mAcknowledgeTo);

        }
      } //end while(is)

    }//end for
  }//end constructor



  /*
    Nothing to populate in destructor.
  */

  ObjectSegmentationAnalysis::~ObjectSegmentationAnalysis()
  {
  }


  void ObjectSegmentationAnalysis::printData(std::ostream &fileOut)
  {

    fileOut<<"\n\nSize of objectMigrateTimes:               "<<objectBeginMigrateTimes.size()<<"\n\n";
    fileOut<<"\n\nSize of objectAcknowledgeMigrateTimes:    "<<(int)objectAcknowledgeMigrateTimes.size()<<"\n\n";


    fileOut << "\n\n*******************Begin Migrate Messages*************\n\n\n";


    for (int s=0; s < (int)objectBeginMigrateTimes.size(); ++s)
    {
      fileOut << objectBeginMigrateTimes[s].raw() << "\n";
      fileOut << "          obj id:       " << objectBeginMigrateID[s].toString() << "\n";
      fileOut << "          migrate from: " << objectBeginMigrateMigrateFrom[s] << "\n";
      fileOut << "          migrate to:   " << objectBeginMigrateMigrateTo[s] << "\n";
      fileOut << "\n\n";

    }

    fileOut << "\n\n\n\n*******************Begin Migrate Acknowledge Messages*************\n\n\n";

    for (int s=0; s < (int)objectAcknowledgeMigrateTimes.size(); ++s)
    {
      fileOut << objectAcknowledgeMigrateTimes[s].raw() << "\n";
      fileOut << "          obj id:           " << objectAcknowledgeMigrateID[s].toString() << "\n";
      fileOut << "          acknowledge from: " << objectAcknowledgeAcknowledgeFrom[s] << "\n";
      fileOut << "          acknowledge to:   " << objectAcknowledgeAcknowledgeTo[s] << "\n";
      fileOut << "\n\n";
    }
  }


} // namespace CBR
