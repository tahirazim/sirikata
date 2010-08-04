/*  Sirikata Utilities -- Sirikata Listener Pattern
 *  ProxyObject.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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

#ifndef _SIRIKATA_PROXY_OBJECT_HPP_
#define _SIRIKATA_PROXY_OBJECT_HPP_

#include <sirikata/core/util/Extrapolation.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include "ProxyObjectListener.hpp"
#include "ProxyObject.hpp"
#include <sirikata/core/util/ListenerProvider.hpp>
#include "PositionListener.hpp"
#include <sirikata/core/util/QueryTracker.hpp>

#include <sirikata/core/odp/Service.hpp>
#include <sirikata/core/odp/Port.hpp>

namespace Sirikata {

class ProxyObject;
typedef std::tr1::shared_ptr<ProxyObject> ProxyObjectPtr;
typedef std::tr1::weak_ptr<ProxyObject> ProxyObjectWPtr;

typedef Provider<ProxyObjectListener*> ProxyObjectProvider;
class ProxyManager;

typedef double AbsTime;

typedef Provider<PositionListener*> PositionProvider;

/**
 * This class represents a generic object on a remote server
 * Every object has a SpaceObjectReference that allows one to communicate
 * with it. Subclasses implement several Providers for concerned listeners
 * This class should be casted to the various subclasses (ProxyLightObject,etc)
 * and appropriate listeners be registered.
 */
class SIRIKATA_PROXYOBJECT_EXPORT ProxyObject
  : public ProxyObjectProvider,
    public PositionProvider,
    protected ProxyObjectListener // Parent death notification. FIXME: or should we leave the parent here, but ignore it in globalLocation()???
{

public:
    class SIRIKATA_PROXYOBJECT_EXPORT UpdateNeeded {
    public:
        bool operator()(
            const Location&updatedValue,
            const Location&predictedValue) const;
    };
    typedef TimedWeightedExtrapolator<Location,UpdateNeeded> Extrapolator;
private:
    const SpaceObjectReference mID;
    ProxyManager *const mManager;

    Extrapolator mLocation;
    SpaceObjectReference mParentId;
    LocationAuthority* mLocationAuthority;

    ODP::Service* mODPService;
    ODP::Port* mDefaultPort; // Default port used to send messages to the object
                             // this ProxyObject represents

protected:
    /// Notification that the Parent has been destroyed.
    virtual void destroyed(const TemporalValue<Location>::Time& when);

public:
    /** Constructs a new ProxyObject. After constructing this object, it
        should be wrapped in a shared_ptr and sent to ProxyManager::createObject().
        @param man  The ProxyManager controlling this object.
        @param id  The SpaceID and ObjectReference assigned to this proxyObject.
        \param odp_service the ODP::Service this ProxyObject can use to send
               messages, i.e. its parent for messaging purposes
    */
    ProxyObject(ProxyManager *man, const SpaceObjectReference&id, ODP::Service* odp_service);
    virtual ~ProxyObject();

    // MCB: default to true for legacy proxies. FIX ME when all converted.
    virtual bool hasModelObject () const { return true; }

    /// Subclasses can do any necessary cleanup first.
    virtual void destroy(const TemporalValue<Location>::Time& when);

    /// Gets a class that can send messages to this Object.
    QueryTracker *getQueryTracker() const;

    ODP::Service* odp() const {
        DEPRECATED(ProxyObject);
        return mODPService;
    }

    /// Send a message.  FIXME this is temporary to transition from QueryTracker.
    bool sendMessage(const ODP::PortID& dest_port, MemoryReference message) const;


    ///Returns the unique identification for this object and the space to which it is connected that gives it said name
    inline const SpaceObjectReference&getObjectReference() const{
        return mID;
    }
    /// Returns the ProxyManager that owns this object. There is currently one ProxyManager per Space per ObjectHost.
    inline ProxyManager *getProxyManager() const {
        return mManager;
    }

    /// Returns the last updated position for this object.
    inline const Vector3d& getPosition() const{
        return mLocation.lastValue().getPosition();
    }
    /// Returns the last updated Quaternion for this object.
    inline const Quaternion& getOrientation() const{
        return mLocation.lastValue().getOrientation();
    }
    /// Returns the full last updated Location for this object.
    inline const Location& getLastLocation() const{
        return mLocation.lastValue();
    }
    /// Returns the time of the last update (even if partial)
    inline TemporalValue<Location>::Time getLastUpdated() const {
        return mLocation.lastUpdateTime();
    }

    /// Gets the parent object reference.
    inline const SpaceObjectReference& getParent() const{
        return mParentId;
    }
    /// Gets the parent ProxyObject. This may return null!
    ProxyObjectPtr getParentProxy() const;

    /// Returns if this object has a zero velocity and requires no extrapolation.
    bool isStatic(const TemporalValue<Location>::Time& when) const;

    /** Sets the location for this update. Note: This does not tell the
        Space that we have moved, but it is the first step in moving a local object. */
    void setLocation(TemporalValue<Location>::Time timeStamp,
                             const Location&location);

    static void updateLocationWithObjLoc(
        Location&location,
        const Protocol::ObjLoc& reqLoc);

    /** requests a new location for this object.  May involve physics
    or other authority to actually move object */
    void requestLocation(TemporalValue<Location>::Time timeStamp, const Protocol::ObjLoc& reqLoc);

    /** set current authority */
    void setLocationAuthority(LocationAuthority* auth) {
        mLocationAuthority = auth;
    }

    /** @see setLocation. This disables interpolation from the last update. */
    void resetLocation(TemporalValue<Location>::Time timeStamp,
                               const Location&location);

    /** Sets the parent of an object. Note that while this is generally sent
        in response to a property update, it is possible to set the parent
        locally only for the purposes of extrapolation. All position updates
        sent over the network are in global coordinates. */
    void setParent(const ProxyObjectPtr &parent,
               TemporalValue<Location>::Time timeStamp,
               const Location &absLocation,
               const Location &relLocation);
    /// Locally unsets a parent, so this object always uses global coords.
    void unsetParent(TemporalValue<Location>::Time timeStamp,
               const Location &absLocation);

    /// Locally sets a parent, and recomputes the relative position at this timeStamp.
    void setParent(const ProxyObjectPtr &parent,
               TemporalValue<Location>::Time timeStamp);
    /// Locally unsets a parent, and recomputes the global position at this timeStamp.
    void unsetParent(TemporalValue<Location>::Time timeStamp);

    /// Returns the global location of this object in space coordinates at timeStamp.
    Location globalLocation(TemporalValue<Location>::Time timeStamp) const {
        ProxyObjectPtr ppop = getParentProxy();
        if (ppop) {
            return extrapolateLocation(timeStamp).
                toWorld(ppop->globalLocation(timeStamp));
        } else {
            return extrapolateLocation(timeStamp);
        }
    }

    /** Retuns the local location of this object at the current timestamp.
        Should return the same value as lastLocation() if current == getLastUpdated().
    */
    Location extrapolateLocation(TemporalValue<Location>::Time current) const {
        return mLocation.extrapolate(current);
    }
};
}
#endif
