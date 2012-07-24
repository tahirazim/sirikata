/*  Sirikata
 *  main.cpp
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

#include <sirikata/core/util/Timer.hpp>

#include <sirikata/core/options/CommonOptions.hpp>
#include "Options.hpp"
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/core/network/ServerIDMap.hpp>
#include "DistributedCoordinateSegmentation.hpp"

int main(int argc, char** argv) {
    using namespace Sirikata;

    DynamicLibrary::Initialize();

    InitOptions();
    Trace::Trace::InitOptions();
    InitCSegOptions();
    ParseOptions(argc, argv, AllowUnregisteredOptions);

    PluginManager plugins;
    plugins.loadList( GetOptionValue<String>(OPT_PLUGINS));
    plugins.loadList( GetOptionValue<String>(OPT_EXTRA_PLUGINS) );
    plugins.loadList( GetOptionValue<String>(OPT_CSEG_PLUGINS));

    // Fill defaults after plugin loading to ensure plugin-added
    // options get their defaults.
    FillMissingOptionDefaults();
    // Rerun original parse to make sure any newly added options are
    // properly parsed.
    ParseOptions(argc, argv);

    DaemonizeAndSetOutputs();
    ReportVersion(); // After options so log goes to the right place

    ServerID server_id = GetOptionValue<ServerID>("cseg-id");
    String trace_file = GetPerServerFile(STATS_TRACE_FILE, server_id);
    Trace::Trace* trace = new Trace::Trace(trace_file);

    // Compute the starting date/time
    String start_time_str = GetOptionValue<String>("wait-until");
    Time start_time = start_time_str.empty() ? Timer::now() : Timer::getSpecifiedDate( start_time_str );

    Duration duration = GetOptionValue<Duration>("duration")
                        + GetOptionValue<Duration>("additional-cseg-duration")
                        + GetOptionValue<Duration>("wait-additional");

    Network::IOService* ios = new Network::IOService("CSeg");
    Network::IOStrand* mainStrand = ios->createStrand("CSeg Main");


    CSegContext* cseg_context = new CSegContext(server_id, ios, mainStrand, trace, start_time, duration);

    String timeseries_type = GetOptionValue<String>(OPT_TRACE_TIMESERIES);
    String timeseries_options = GetOptionValue<String>(OPT_TRACE_TIMESERIES_OPTIONS);
    Trace::TimeSeries* time_series = Trace::TimeSeriesFactory::getSingleton().getConstructor(timeseries_type)(cseg_context, timeseries_options);

    BoundingBox3f region = GetOptionValue<BoundingBox3f>("region");
    Vector3ui32 layout = GetOptionValue<Vector3ui32>("layout");

    uint32 max_space_servers = GetOptionValue<uint32>("max-servers");
    if (max_space_servers == 0)
      max_space_servers = layout.x * layout.y * layout.z;

    srand( GetOptionValue<uint32>("rand-seed") );

    String servermap_type = GetOptionValue<String>("servermap");
    String servermap_options = GetOptionValue<String>("cseg-servermap-options");
    ServerIDMap * server_id_map =
        ServerIDMapFactory::getSingleton().getConstructor(servermap_type)(cseg_context, servermap_options);

    DistributedCoordinateSegmentation* cseg = new DistributedCoordinateSegmentation(cseg_context, region, layout, max_space_servers, server_id_map);

    ///////////Go go go!! start of simulation/////////////////////

    srand(time(NULL));

    cseg_context->add(cseg_context);
    cseg_context->add(cseg);

    cseg_context->run(2);

    std::cout << "CSEG cleaning up\n";
    fflush(stdout);

    cseg_context->cleanup();

    if (GetOptionValue<bool>(PROFILE)) {
        cseg_context->profiler->report();
    }

    trace->prepareShutdown();

    delete cseg;

    trace->shutdown();
    delete trace;
    trace = NULL;

    delete cseg_context;
    cseg_context = NULL;

    delete time_series;

    delete mainStrand;
    delete ios;

    plugins.gc();

    Sirikata::Logging::finishLog();

    return 0;
}
