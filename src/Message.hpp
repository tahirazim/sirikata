/*  cbr
 *  Message.hpp
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

#ifndef _CBR_MESSAGE_HPP_
#define _CBR_MESSAGE_HPP_

#include "Utility.hpp"
#include "Network.hpp"
#include "MotionVector.hpp"
#include "ServerNetwork.hpp"

namespace CBR {

typedef uint8 MessageType;

#define MESSAGE_TYPE_PROXIMITY    1
#define MESSAGE_TYPE_LOCATION     2
#define MESSAGE_TYPE_SUBSCRIPTION 3
#define MESSAGE_TYPE_MIGRATE      4
#define MESSAGE_TYPE_COUNT        5
#define MESSAGE_TYPE_CSEG_CHANGE  6
#define MESSAGE_TYPE_LOAD_STATUS  7

struct OriginID {
    uint32 id;
};

template <typename scalar>
class SplitRegion {
public:
  ServerID mNewServerID;
  
  scalar minX, minY, minZ;
  scalar maxX, maxY, maxZ;
  
};

typedef SplitRegion<float> SplitRegionf;

OriginID GetUniqueIDOriginID(uint32 uid);
uint32 GetUniqueIDMessageID(uint32 uid);

/** Base class for messages that go over the network.  Must provide
 *  message type and serialization methods.
 */
class Message {
public:
    virtual ~Message();

    virtual MessageType type() const = 0;
    uint32 id() const;

    virtual uint32 serialize(Network::Chunk& wire, uint32 offset) = 0;
    static uint32 deserialize(const Network::Chunk& wire, uint32 offset, Message** result);
protected:
    Message(const OriginID& origin, bool x); // note the bool is here to make the signature different than the next constructor
    Message(uint32 id);

    // takes care of serializing the header information properly, will overwrite
    // contents of chunk.  returns the offset after serializing the header
    uint32 serializeHeader(Network::Chunk& wire, uint32 offset);
private:
    static uint32 sIDSource;
    uint32 mID;
}; // class Message



class ProximityMessage : public Message {
public:
    enum EventType {
        Entered = 1,
        Exited = 2
    };

    ProximityMessage(const OriginID& origin, const UUID& dest_object, const UUID& nbr, EventType evt, const TimedMotionVector3f& loc);

    virtual MessageType type() const;

    const UUID& destObject() const;
    const UUID& neighbor() const;
    const EventType event() const;
    const TimedMotionVector3f& location() const;

    virtual uint32 serialize(Network::Chunk& wire, uint32 offset);
private:
    friend class Message;
    ProximityMessage(const Network::Chunk& wire, uint32& offset, uint32 _id);

    UUID mDestObject;
    UUID mNeighbor;
    EventType mEvent;
    TimedMotionVector3f mLocation;
}; // class ProximityMessage


// Base class for object to object messages.  Mostly saves a bit of serialization code
class ObjectToObjectMessage : public Message {
public:
    ObjectToObjectMessage(const OriginID& origin, const UUID& src_object, const UUID& dest_object);

    const UUID& sourceObject() const;
    const UUID& destObject() const;

protected:
    uint32 serializeSourceDest(Network::Chunk& wire, uint32 offset);
    ObjectToObjectMessage(const Network::Chunk& wire, uint32& offset, uint32 _id);

private:
    UUID mSourceObject;
    UUID mDestObject;
}; // class ObjectToObjectMessage


class LocationMessage : public ObjectToObjectMessage {
public:
    LocationMessage(const OriginID& origin, const UUID& src_object, const UUID& dest_object, const TimedMotionVector3f& loc);

    virtual MessageType type() const;

    const TimedMotionVector3f& location() const;

    virtual uint32 serialize(Network::Chunk& wire, uint32 offset);
private:
    friend class Message;
    LocationMessage(const Network::Chunk& wire, uint32& offset, uint32 _id);

    TimedMotionVector3f mLocation;
}; // class LocationMessage


class SubscriptionMessage : public ObjectToObjectMessage {
public:
    enum Action {
        Subscribe = 1,
        Unsubscribe = 2
    };

    SubscriptionMessage(const OriginID& origin, const UUID& src_object, const UUID& dest_object, const Action& act);

    virtual MessageType type() const;

    const Action& action() const;

    virtual uint32 serialize(Network::Chunk& wire, uint32 offset);
private:
    friend class Message;
    SubscriptionMessage(const Network::Chunk& wire, uint32& offset, uint32 _id);

    Action mAction;
}; // class SubscriptionMessage

class MigrateMessage : public Message {
public:
    MigrateMessage(const OriginID& origin, const UUID& obj, float proxRadius, uint16_t subscriberCount);

    ~MigrateMessage();

    virtual MessageType type() const;

    const UUID& object() const;

    const float proximityRadius() const;

    const int subscriberCount() const;

    UUID* subscriberList() const;

    virtual uint32 serialize(Network::Chunk& wire, uint32 offset);
private:
    friend class Message;
    MigrateMessage(const Network::Chunk& wire, uint32& offset, uint32 _id);

    UUID mObject;

    float mProximityRadius;

    uint16_t mCountSubscribers;
    UUID* mSubscribers;

}; // class MigrateMessage

class CSegChangeMessage : public Message {
public:
  CSegChangeMessage(const OriginID& origin, uint8_t number_of_regions);

  ~CSegChangeMessage();

  virtual MessageType type() const;

  const uint8_t numberOfRegions() const;

  SplitRegionf* splitRegions() const;

  virtual uint32 serialize(Network::Chunk& wire, uint32 offset);

private:
  friend class Message;
  CSegChangeMessage(const Network::Chunk& wire, uint32& offset, uint32 _id);

  
  uint8_t mNumberOfRegions;
  SplitRegionf* mSplitRegions;
  
};

class LoadStatusMessage : public Message {

public:
  LoadStatusMessage(const OriginID& origin, float loadReading);

  ~LoadStatusMessage();

  virtual MessageType type() const;

  const float loadReading() const;

  virtual uint32 serialize(Network::Chunk& wire, uint32 offset);

private:
  friend class Message;
  LoadStatusMessage(const Network::Chunk& wire, uint32& offset, uint32 _id);

  
  float mLoadReading;
  
};

} // namespace CBR

#endif //_CBR_MESSAGE_HPP_
