/*  Sirikata
 *  Meshdata.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#include <sirikata/mesh/Filter.hpp>

namespace Sirikata {
namespace Mesh {

SIRIKATA_MESH_EXPORT NodeIndex NullNodeIndex = -1;

Node::Node()
 : parent(NullNodeIndex),
   transform(Matrix4x4f::identity())
{
}

Node::Node(NodeIndex par, const Matrix4x4f& xform)
 : parent(par),
   transform(xform)
{
}

Node::Node(const Matrix4x4f& xform)
 : parent(NullNodeIndex),
   transform(xform)
{
}

Matrix4x4f Meshdata::getTransform(NodeIndex index) const {
    // Just trace up the tree, multiplying in transforms
    Matrix4x4f xform(Matrix4x4f::identity());

    while(index != NullNodeIndex) {
        xform = nodes[index].transform * xform;
        index = nodes[index].parent;
    }

    return globalTransform * xform;
}

Matrix4x4f Meshdata::getTransform(const GeometryInstance& geo) const {
    return getTransform(geo.parentNode);
}

Matrix4x4f Meshdata::getTransform(const LightInstance& light) const {
    return getTransform(light.parentNode);
}

} // namespace Mesh
} // namespace Sirikata
