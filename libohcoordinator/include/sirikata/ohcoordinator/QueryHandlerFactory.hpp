/*  Sirikata
 *  QueryHandlerFactory.cpp
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/options/Options.hpp>

#include <prox/geom/BruteForceQueryHandler.hpp>
#include <prox/geom/RTreeAngleQueryHandler.hpp>
#include <prox/geom/RTreeDistanceQueryHandler.hpp>
#include <prox/geom/RTreeCutQueryHandler.hpp>
#include <prox/geom/RebuildingQueryHandler.hpp>

namespace Sirikata {

/** Creates a Prox::QueryHandler of the specified type. Parses the arguments
 *  specified and passes them to the query handler constructor.
 */
template<typename SimulationTraits>
Prox::QueryHandler<SimulationTraits>* QueryHandlerFactory(const String& type, const String& args) {
    static OptionValue* branching = NULL;
    static OptionValue* rebuild_batch_size = NULL;
    if (branching == NULL) {
        branching = new OptionValue("branching", "10", Sirikata::OptionValueType<uint32>(), "Number of children each node should have.");
        rebuild_batch_size = new OptionValue("rebuild-batch-size", "10", Sirikata::OptionValueType<uint32>(), "Number of queries to transition on each iteration when rebuilding. Keep this small to avoid long latencies between updates.");
        Sirikata::InitializeClassOptions ico("query_handler", NULL,
            branching,
            rebuild_batch_size,
            NULL);
    }

    assert(branching != NULL);

    // Since these options end up being shared if you instantiate multiple
    // QueryHandlers, reset them each time.
    branching->unsafeAs<uint32>() = 10;

    OptionSet* optionsSet = OptionSet::getOptions("query_handler", NULL);
    optionsSet->parse(args);

    if (type == "brute") {
        return new Prox::RebuildingQueryHandler<SimulationTraits>(
            Prox::BruteForceQueryHandler<SimulationTraits>::Constructor(), rebuild_batch_size->unsafeAs<uint32>()
        );
    }
    else if (type == "rtree") {
        return new Prox::RebuildingQueryHandler<SimulationTraits>(
            Prox::RTreeAngleQueryHandler<SimulationTraits>::Constructor(branching->unsafeAs<uint32>()), rebuild_batch_size->unsafeAs<uint32>()
        );
    }
    else if (type == "rtreedist" || type == "dist") {
        return new Prox::RebuildingQueryHandler<SimulationTraits>(
            Prox::RTreeDistanceQueryHandler<SimulationTraits>::Constructor(branching->unsafeAs<uint32>()), rebuild_batch_size->unsafeAs<uint32>()
        );
    }
    else if (type == "rtreecut") {
        return new Prox::RebuildingQueryHandler<SimulationTraits>(
            Prox::RTreeCutQueryHandler<SimulationTraits>::Constructor(branching->unsafeAs<uint32>(), false), rebuild_batch_size->unsafeAs<uint32>()
        );
    }
    else if (type == "rtreecutagg") {
        return new Prox::RebuildingQueryHandler<SimulationTraits>(
            Prox::RTreeCutQueryHandler<SimulationTraits>::Constructor(branching->unsafeAs<uint32>(), true), rebuild_batch_size->unsafeAs<uint32>()
        );
    }
    else {
        return NULL;
    }
}

} // namespace Sirikata
