/*  Sirikata Object Host
 *  ProxyObject.cpp
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

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/core/util/Extrapolation.hpp>
#include <sirikata/proxyobject/PositionListener.hpp>
#include <sirikata/proxyobject/ProxyManager.hpp>
#include "Protocol_Sirikata.pbj.hpp"

#include <sirikata/core/util/RoutableMessageBody.hpp>
#include <sirikata/core/util/RoutableMessageHeader.hpp>
#include <sirikata/core/util/KnownServices.hpp>

namespace Sirikata {

ProxyObject::ProxyObject(ProxyManager *man, const SpaceObjectReference&id, VWObjectPtr vwobj, const SpaceObjectReference& owner_sor)
 : mID(id),
   mManager(man),
   mLoc(Time::null(), MotionVector3f(Vector3f::nil(), Vector3f::nil())),
   mOrientation(Time::null(), MotionQuaternion(Quaternion::identity(), Quaternion::identity())),
   mParent(vwobj)
{
    assert(mParent);

    mDefaultPort = mParent->bindODPPort(owner_sor);
}

ProxyObject::~ProxyObject() {
    delete mDefaultPort;
}

void ProxyObject::destroy() {
    ProxyObjectProvider::notify(&ProxyObjectListener::destroyed);
    //FIXME mManager->notify(&ProxyCreationListener::onDestroyProxy);
}

bool ProxyObject::UpdateNeeded::operator() (
    const Location&updatedValue,
    const Location&predictedValue) const {
    Vector3f ux,uy,uz,px,py,pz;
    updatedValue.getOrientation().toAxes(ux,uy,uz);
    predictedValue.getOrientation().toAxes(px,py,pz);
    return (updatedValue.getPosition()-predictedValue.getPosition()).lengthSquared()>1.0 ||
           ux.dot(px)<.9||uy.dot(py)<.9||uz.dot(pz)<.9;
}

bool ProxyObject::isStatic() const {
    return mLoc.velocity() == Vector3f::nil() && mOrientation.velocity() == Quaternion::identity();
}

QueryTracker *ProxyObject::getQueryTracker() const {
    DEPRECATED(ProxyObject);
    return mManager->getQueryTracker(getObjectReference());
}

bool ProxyObject::sendMessage(const ODP::PortID& dest_port, MemoryReference message) const {
    ODP::Endpoint dest(getObjectReference().space(), getObjectReference().object(), dest_port);
    return mDefaultPort->send(dest, message);
}

void ProxyObject::setLocation(const TimedMotionVector3f& reqloc) {
    mLoc = reqloc;
    PositionProvider::notify(&PositionListener::updateLocation, mLoc, mOrientation);
}

void ProxyObject::setOrientation(const TimedMotionQuaternion& reqorient) {
    mOrientation = TimedMotionQuaternion(reqorient.time(), MotionQuaternion(reqorient.position().normal(), reqorient.velocity().normal()));
    PositionProvider::notify(&PositionListener::updateLocation, mLoc, mOrientation);
}

void ProxyObject::setBounds(const BoundingSphere3f& bnds) {
    mBounds = bnds;
}

ProxyObjectPtr ProxyObject::getParentProxy() const {
    return ProxyObjectPtr();
}



}
