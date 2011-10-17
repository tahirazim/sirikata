/*  Sirikata
 *  Service.hpp
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

#ifndef _SIRIKATA_ODP_SERVICE_HPP_
#define _SIRIKATA_ODP_SERVICE_HPP_

#include <sirikata/core/odp/Defs.hpp>
#include <sirikata/core/xdp/Port.hpp>

namespace Sirikata {
namespace ODP {

typedef Sirikata::XDP::Port<Endpoint> Port;

/** ODP::Service is the interface provided by classes that are able to send ODP
 *  messages. ODP::Service mainly handles management of ODP::Ports, which in
 *  turn allow sending and receiving of ODP messages.
 *
 *  A Service allocates Ports and (behind the scenes) handles requests on the
 *  Ports, but once allocated the Port is owned by the allocator. The allocator
 *  is responsible for deleting all allocated Ports, even if the Service that
 *  generated them was destroyed.
 */
class SIRIKATA_EXPORT Service {
public:
    typedef Endpoint::MessageHandler MessageHandler;

    virtual ~Service() {}

    /** Bind an ODP port for use.
     *  \param space the Space to communicate via
     *  \param objref the Object to communicate via
     *  \param port the PortID to attempt to bind
     *  \returns an ODP Port object which can be used immediately, or NULL if
     *           the port is already bound
     *  \throws PortAllocationError if the Service cannot allocate the port for
     *          some reason other than it already being allocated.
     */
    virtual Port* bindODPPort(const SpaceID& space, const ObjectReference& objref, PortID port) = 0;
    virtual Port* bindODPPort(const SpaceObjectReference& sor, PortID port) = 0;

    /** Bind a random, unused ODP port for use.
     *  \param space the Space to communicate via
     *  \returns an ODP Port object which can be used immediately, or, in
     *           extremely rare cases, NULL when an unused port isn't available
     *  \throws PortAllocationError if the Service cannot allocate the port for
     *          some reason other than it already being allocated.
     */
    virtual Port* bindODPPort(const SpaceID& space, const ObjectReference& objref) = 0;
    virtual Port* bindODPPort(const SpaceObjectReference& sor) = 0;

    /** Get a random, unused ODP port. */
    virtual PortID unusedODPPort(const SpaceID& space, const ObjectReference& objref) = 0;
    virtual PortID unusedODPPort(const SpaceObjectReference& sor) = 0;

    /** Register a handler for messages that arrive on unbound ports.  By
     *  default there is no handler and such messages are ignored.  Note that
     *  this handler will not be invoked for messages arriving at a bound port
     *  for which no handler has been registered.
     *  \param cb the handler for messages arriving at unbound ports
     */
    virtual void registerDefaultODPHandler(const MessageHandler& cb) = 0;

}; // class Service

} // namespace ODP
} // namespace Sirikata

#endif //_SIRIKATA_ODP_SERVICE_HPP_
