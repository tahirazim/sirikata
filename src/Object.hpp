/*  cbr
 *  Object.hpp
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

#ifndef _CBR_OBJECT_HPP_
#define _CBR_OBJECT_HPP_

#include "Utility.hpp"
#include "Message.hpp"
#include "MotionPath.hpp"
#include "SolidAngle.hpp"
#include "ObjectHost.hpp"

namespace CBR {

class ObjectFactory;
class ObjectMessageQueue;

typedef std::set<UUID> ObjectSet;

struct MaxDistUpdatePredicate {
    static float64 maxDist;
    bool operator()(const MotionVector3f& lhs, const MotionVector3f& rhs) const {
        return (lhs.position() - rhs.position()).length() > maxDist;
    }
};

class Object {
public:
    /** Standard constructor. */
    Object(const UUID& id, MotionPath* motion, SolidAngle queryAngle, const ObjectHostContext* ctx);
    /** Global knowledge constructor - used to give object knowledge of all other objects in the world. */
    Object(const UUID& id, MotionPath* motion, SolidAngle queryAngle, const ObjectHostContext* ctx, const std::set<UUID>& objects);

    ~Object();

    const UUID& uuid() const {
        return mID;
    }

    const SolidAngle queryAngle() const {
        return mQueryAngle;
    }

    const TimedMotionVector3f location() const;

    const BoundingSphere3f bounds() const {
        return BoundingSphere3f( Vector3f(0, 0, 0), 1.f ); // FIXME
    }

    const ObjectSet& subscriberSet() const {
        return mSubscribers;
    }

    void tick();

    void receiveMessage(const CBR::Protocol::Object::ObjectMessage* msg);

    void migrateMessage(const UUID& oid, const SolidAngle& sa, const std::vector<UUID> subs);
private:
    void sessionMessage(const CBR::Protocol::Object::ObjectMessage& msg);
    void locationMessage(const CBR::Protocol::Object::ObjectMessage& msg);
    void proximityMessage(const CBR::Protocol::Object::ObjectMessage& msg);
    void subscriptionMessage(const CBR::Protocol::Object::ObjectMessage& msg);

    void addSubscriber(const UUID& sub);
    void removeSubscriber(const UUID& sub);

    void checkPositionUpdate();

    UUID mID;
    const ObjectHostContext* mContext;
    bool mGlobalIntroductions;
    MotionPath* mMotion;
    TimedMotionVector3f mLocation;
    SimpleExtrapolator<MotionVector3f, MaxDistUpdatePredicate> mLocationExtrapolator;
    ObjectSet mSubscribers;
    SolidAngle mQueryAngle;
    bool mMigrating;
}; // class Object

} // namespace CBR

#endif //_CBR_OBJECT_HPP_
