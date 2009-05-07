/*  cbr
 *  UniformCoordinateSegmentation.hpp
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

#ifndef _CBR_UNIFORM_COORDINATE_SEGMENTATION_HPP_
#define _CBR_UNIFORM_COORDINATE_SEGMENTATION_HPP_

#include "CoordinateSegmentation.hpp"

namespace CBR {

struct LayoutChangeEntry {
  uint64 time;

  Vector3ui32 layout;
};


/** Uniform grid implementation of CoordinateSegmentation. */
class UniformCoordinateSegmentation : public CoordinateSegmentation {
public:
    UniformCoordinateSegmentation(const BoundingBox3f& region, const Vector3ui32& perdim);
    virtual ~UniformCoordinateSegmentation();

    virtual ServerID lookup(const Vector3f& pos) const;
    virtual BoundingBox3f serverRegion(const ServerID& server) const;
    virtual BoundingBox3f region() const;
    virtual uint32 numServers() const;

    virtual void tick(const Time& t);

    virtual void csegChangeMessage(CSegChangeMessage*);

private:
    BoundingBox3f mRegion;
    Vector3ui32 mServersPerDim;

    std::vector<LayoutChangeEntry> mLayoutChangeEntries;
    uint32 lastLayoutChangeIdx;

}; // class CoordinateSegmentation

} // namespace CBR

#endif
