/*  Sirikata Object Host -- Proxy Creation and Destruction manager
 *  SimulationFactory.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#ifndef _SIRIKATA_SIMULATION_FACTORY_
#define _SIRIKATA_SIMULATION_FACTORY_

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/proxyobject/TimeSteppedQueryableSimulation.hpp>
#include <sirikata/proxyobject/TimeOffsetManager.hpp>

namespace Sirikata{

///Class to create graphics subsystems. FIXME: should this load a dll when a named factory is not found
class SIRIKATA_PROXYOBJECT_EXPORT SimulationFactory
    : public AutoSingleton<SimulationFactory>,
      public Factory4<TimeSteppedQueryableSimulation*,
                      Context*,
                      Provider<ProxyCreationListener*>*,//the ProxyManager
                      const TimeOffsetManager*,//so we can get any local time offset for objects
                      const String&> //options string for the graphics system
{
public:
    static SimulationFactory&getSingleton();
    static void destroy();
};


}
#endif
