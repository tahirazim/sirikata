/*  Sirikata
 *  Bounds.cpp
 *
 *  Copyright (c) 2011, Stanford University
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

#include <sirikata/mesh/Platform.hpp>
#include <sirikata/mesh/Bounds.hpp>

namespace Sirikata {
namespace Mesh {

void SIRIKATA_MESH_FUNCTION_EXPORT ComputeBounds(VisualPtr vis, BoundingBox3f3f* bbox, double* rad) {
    // Just dispatches based on type
    MeshdataPtr md( std::tr1::dynamic_pointer_cast<Meshdata>(vis) );
    if (md) {
        ComputeBounds(md, bbox, rad);
        return;
    }
    BillboardPtr bboard( std::tr1::dynamic_pointer_cast<Billboard>(vis) );
    if (bboard) {
        ComputeBounds(bboard, bbox, rad);
        return;
    }
}

void SIRIKATA_MESH_FUNCTION_EXPORT ComputeBounds(VisualPtr vis, const Matrix4x4f& xform, BoundingBox3f3f* bbox, double* rad) {
    // Just dispatches based on type
    MeshdataPtr md( std::tr1::dynamic_pointer_cast<Meshdata>(vis) );
    if (md) {
        ComputeBounds(md, xform, bbox, rad);
        return;
    }
    BillboardPtr bboard( std::tr1::dynamic_pointer_cast<Billboard>(vis) );
    if (bboard) {
        ComputeBounds(bboard, xform, bbox, rad);
        return;
    }
}





void SIRIKATA_MESH_FUNCTION_EXPORT ComputeBounds(MeshdataPtr mesh, BoundingBox3f3f* bbox, double* rad) {
    Meshdata::GeometryInstanceIterator geoIter = mesh->getGeometryInstanceIterator();
    uint32 indexInstance; Matrix4x4f transformInstance;

    if (bbox != NULL) *bbox = BoundingBox3f3f::null();
    if (rad != NULL) *rad = 0.0;

    //loop through the instances, expand the bounding box and find the radius
    while(geoIter.next(&indexInstance, &transformInstance)) {
        GeometryInstance& geoInst = mesh->instances[indexInstance];
        BoundingBox3f3f inst_bnds;
        double inst_rad = 0;
        geoInst.computeTransformedBounds(mesh, transformInstance, &inst_bnds, &inst_rad);

        if (bbox != NULL) {
            if (*bbox == BoundingBox3f3f::null())
                *bbox = inst_bnds;
            else
                (*bbox).mergeIn(inst_bnds);
        }
        if (rad != NULL) *rad = std::max(*rad, inst_rad);
    }
}

void SIRIKATA_MESH_FUNCTION_EXPORT ComputeBounds(MeshdataPtr mesh, const Matrix4x4f& xform, BoundingBox3f3f* bbox, double* rad) {
    Meshdata::GeometryInstanceIterator geoIter = mesh->getGeometryInstanceIterator();
    uint32 indexInstance; Matrix4x4f transformInstance;

    if (bbox != NULL) *bbox = BoundingBox3f3f::null();
    if (rad != NULL) *rad = 0.0;

    //loop through the instances, expand the bounding box and find the radius
    while(geoIter.next(&indexInstance, &transformInstance)) {
        GeometryInstance& geoInst = mesh->instances[indexInstance];
        BoundingBox3f3f inst_bnds;
        double inst_rad = 0;
        geoInst.computeTransformedBounds(mesh, xform * transformInstance, &inst_bnds, &inst_rad);

        if (bbox != NULL) {
            if (*bbox == BoundingBox3f3f::null())
                *bbox = inst_bnds;
            else
                (*bbox).mergeIn(inst_bnds);
        }
        if (rad != NULL) *rad = std::max(*rad, inst_rad);
    }
}



void SIRIKATA_MESH_FUNCTION_EXPORT ComputeBounds(BillboardPtr bboard, BoundingBox3f3f* bbox, double* rad) {
    // Billboards without any sizes specified are just squares -- X & Y but no
    // depth in Z
    static float one_over_rad2 = 0.707106781;
    if (bbox != NULL) *bbox = BoundingBox3f3f(Vector3f(-one_over_rad2, -one_over_rad2, 0), Vector3f(one_over_rad2, one_over_rad2, 0));
    if (rad != NULL) *rad = 1.0;
}

void SIRIKATA_MESH_FUNCTION_EXPORT ComputeBounds(BillboardPtr bboard, const Matrix4x4f& xform, BoundingBox3f3f* bbox, double* rad) {
    // Billboards without any sizes specified are just squares -- X & Y but no
    // depth in Z
    static float one_over_rad2 = 0.707106781;
    if (bbox != NULL) {
        // Transform and merge in each corner
        *bbox = BoundingBox3f3f(
            xform * Vector3f(one_over_rad2, one_over_rad2, 0),
            xform * Vector3f(one_over_rad2, one_over_rad2, 0)
        );
        bbox->mergeIn( xform * Vector3f(-one_over_rad2, one_over_rad2, 0) );
        bbox->mergeIn( xform * Vector3f(one_over_rad2, -one_over_rad2, 0) );
        bbox->mergeIn( xform * Vector3f(-one_over_rad2, -one_over_rad2, 0) );
    }
    if (rad != NULL) *rad = 1.0;
}

} // namespace Mesh
} // namespace Sirikata
