// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_LOC_UPDATE_HPP_
#define _SIRIKATA_OH_LOC_UPDATE_HPP_

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/util/Noncopyable.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include <sirikata/core/util/MotionQuaternion.hpp>
#include <sirikata/core/util/ObjectReference.hpp>
#include <sirikata/core/util/AggregateBoundingInfo.hpp>

namespace Sirikata {

class SpaceID;
class ObjectHost;
class HostedObject;
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;

/** LocUpdate is an abstract representation of location updates. This is
 *  provided since some location updates may be used directly from the wire
 *  protocol whereas others may be delayed, 'reconstituted', or just created on
 *  the fly in-memory (e.g. due to local queries). This class acts as an adaptor
 *  to get at all the properties from the underlying representation.
 *
 *  Note that this assumes that this assumes times (in location and
 *  orientation) have been converted to the local timeframe.
 */
class SIRIKATA_OH_EXPORT LocUpdate : Noncopyable {
public:
    virtual ~LocUpdate() {}

    // Object
    virtual ObjectReference object() const = 0;

    // Request epoch
    virtual bool has_epoch() const = 0;
    virtual uint64 epoch() const = 0;

    // Parent aggregate
    virtual bool has_parent() const = 0;
    virtual ObjectReference parent() const = 0;
    virtual uint64 parent_seqno() const = 0;

    // Location
    virtual bool has_location() const = 0;
    virtual TimedMotionVector3f location() const = 0;
    virtual uint64 location_seqno() const = 0;

    // Orientation
    virtual bool has_orientation() const = 0;
    virtual TimedMotionQuaternion orientation() const = 0;
    virtual uint64 orientation_seqno() const = 0;

    // Bounds
    virtual bool has_bounds() const = 0;
    virtual AggregateBoundingInfo bounds() const = 0;
    virtual uint64 bounds_seqno() const = 0;

    // Mesh
    virtual bool has_mesh() const = 0;
    virtual String mesh() const = 0;
    virtual uint64 mesh_seqno() const = 0;
    String meshOrDefault() const { return (has_mesh() ? mesh() : ""); }

    // Physics
    virtual bool has_physics() const = 0;
    virtual String physics() const = 0;
    virtual uint64 physics_seqno() const = 0;
    String physicsOrDefault() const { return (has_physics() ? physics() : ""); }
};

} // namespace Sirikata

#endif //_SIRIKATA_OH_LOC_UPDATE_HPP_
