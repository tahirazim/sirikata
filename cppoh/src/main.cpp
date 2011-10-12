/*  Sirikata
 *  main.cpp
 *
 *  Copyright (c) 2008, Daniel Reiter Horn
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

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/proxyobject/SimulationFactory.hpp>

#include "ObjectHost.hpp"
#include <sirikata/mesh/LightInfo.hpp>
#include <sirikata/oh/ObjectHostProxyManager.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <time.h>
#include <boost/thread.hpp>

#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include "Options.hpp"
#include <sirikata/oh/Trace.hpp>

#include <sirikata/core/network/ServerIDMap.hpp>
#include <sirikata/core/network/NTPTimeSync.hpp>

#include <sirikata/core/network/SSTImpl.hpp>

#include <sirikata/oh/ObjectHostContext.hpp>

#include <sirikata/oh/Storage.hpp>
#include <sirikata/oh/PersistedObjectSet.hpp>
#include <sirikata/oh/ObjectFactory.hpp>
#include <sirikata/oh/ObjectQueryProcessor.hpp>

#include <sirikata/mesh/Filter.hpp>
#include <sirikata/mesh/ModelsSystemFactory.hpp>
#include <sirikata/oh/ObjectScriptManagerFactory.hpp>
#ifdef __GNUC__
#include <fenv.h>
#endif

int main (int argc, char** argv) {
    using namespace Sirikata;

    DynamicLibrary::Initialize();

    InitOptions();
    Trace::Trace::InitOptions();
    OHTrace::InitOptions();
    InitCPPOHOptions();

    ParseOptions(argc, argv, OPT_CONFIG_FILE);

    PluginManager plugins;
    String search_path=GetOptionValue<String>(OPT_OH_PLUGIN_SEARCH_PATHS);
    if (search_path.length()) {
        while (true) {
            String::size_type where=search_path.find(":");
            if (where==String::npos) {
                DynamicLibrary::AddLoadPath(search_path);
                break;
            }else {
                DynamicLibrary::AddLoadPath(search_path.substr(0,where));
                search_path=search_path.substr(where+1);
            }
        }
    }
    plugins.loadList( GetOptionValue<String>(OPT_PLUGINS) );
    plugins.loadList( GetOptionValue<String>(OPT_OH_PLUGINS) );

    // Fill defaults after plugin loading to ensure plugin-added
    // options get their defaults.
    FillMissingOptionDefaults();
    // Rerun original parse to make sure any newly added options are
    // properly parsed.
    ParseOptions(argc, argv, OPT_CONFIG_FILE);

    ReportVersion(); // After options so log goes to the right place

    String time_server = GetOptionValue<String>("time-server");
    NTPTimeSync sync;
    if (time_server.size() > 0)
        sync.start(time_server);

#ifdef __GNUC__
#ifndef __APPLE__
    if (GetOptionValue<bool>(OPT_SIGFPE)) {
        feenableexcept(FE_DIVBYZERO|FE_INVALID|FE_OVERFLOW|FE_UNDERFLOW);
    }
#endif
#endif


    ObjectHostID oh_id = GetOptionValue<ObjectHostID>("ohid");
    String trace_file = GetPerServerFile(STATS_OH_TRACE_FILE, oh_id);
    Trace::Trace* trace = new Trace::Trace(trace_file);

    srand( GetOptionValue<uint32>("rand-seed") );

    Network::IOService* ios = Network::IOServiceFactory::makeIOService();
    Network::IOStrand* mainStrand = ios->createStrand();

    SSTConnectionManager* sstConnMgr = new SSTConnectionManager();

    Time start_time = Timer::now(); // Just for stats in ObjectHostContext.
    Duration duration = Duration::zero(); // Indicates to run forever.
    ObjectHostContext* ctx = new ObjectHostContext("cppoh", oh_id, sstConnMgr, ios, mainStrand, trace, start_time, duration);

    String servermap_type = GetOptionValue<String>("servermap");
    String servermap_options = GetOptionValue<String>("servermap-options");
    ServerIDMap * server_id_map =
        ServerIDMapFactory::getSingleton().getConstructor(servermap_type)(ctx, servermap_options);

    String timeseries_type = GetOptionValue<String>(OPT_TRACE_TIMESERIES);
    String timeseries_options = GetOptionValue<String>(OPT_TRACE_TIMESERIES_OPTIONS);
    Trace::TimeSeries* time_series = Trace::TimeSeriesFactory::getSingleton().getConstructor(timeseries_type)(ctx, timeseries_options);

    SpaceID mainSpace(GetOptionValue<UUID>(OPT_MAIN_SPACE));

    String oh_options = GetOptionValue<String>(OPT_OH_OPTIONS);
    ObjectHost *oh = new CppohObjectHost(ctx, ios, oh_options);

    // Add all the spaces to the ObjectHost.  We used to have SpaceIDMap and
    // fill in the same ServerIDMap for all these. Now we just add the
    // ServerIDMap for the main space. We need a better way of handling multiple
    // spaces.
    oh->addServerIDMap(mainSpace, server_id_map);

    String objstorage_type = GetOptionValue<String>(OPT_OBJECT_STORAGE);
    String objstorage_options = GetOptionValue<String>(OPT_OBJECT_STORAGE_OPTS);
    OH::Storage* obj_storage =
        OH::StorageFactory::getSingleton().getConstructor(objstorage_type)(ctx, objstorage_options);
    oh->setStorage(obj_storage);

    String objpersistentset_type = GetOptionValue<String>(OPT_OH_PERSISTENT_SET);
    String objpersistentset_options = GetOptionValue<String>(OPT_OH_PERSISTENT_SET_OPTS);
    OH::PersistedObjectSet* obj_persistent_set =
        OH::PersistedObjectSetFactory::getSingleton().getConstructor(objpersistentset_type)(ctx, objpersistentset_options);
        oh->setPersistentSet(obj_persistent_set);

    String objfactory_type = GetOptionValue<String>(OPT_OBJECT_FACTORY);
    String objfactory_options = GetOptionValue<String>(OPT_OBJECT_FACTORY_OPTS);
    ObjectFactory* obj_factory = NULL;

    String obj_query_type = GetOptionValue<String>(OPT_OBJECT_QUERY_PROCESSOR);
    String obj_query_options = GetOptionValue<String>(OPT_OBJECT_QUERY_PROCESSOR_OPTS);
    OH::ObjectQueryProcessor* obj_query_processor =
        OH::ObjectQueryProcessorFactory::getSingleton().getConstructor(obj_query_type)(ctx, obj_query_options);
    oh->setQueryProcessor(obj_query_processor);

    ///////////Go go go!! start of simulation/////////////////////
    ctx->add(ctx);
    ctx->add(obj_storage);
    ctx->add(obj_persistent_set);
    ctx->add(obj_query_processor);

    ctx->add(oh);
    ctx->add(sstConnMgr);

    if (!objfactory_type.empty())
    {
        obj_factory = ObjectFactoryFactory::getSingleton().getConstructor(objfactory_type)(ctx, oh, mainSpace, objfactory_options);
        obj_factory->generate();
    }


    ctx->run(1);

    ctx->cleanup();

    if (GetOptionValue<bool>(PROFILE)) {
        ctx->profiler->report();
    }

    trace->prepareShutdown();
    Mesh::FilterFactory::destroy();
    ModelsSystemFactory::destroy();
    ObjectScriptManagerFactory::destroy();
    delete oh;
    //delete pd;


    delete obj_storage;
    delete obj_persistent_set;


    SimulationFactory::destroy();



    delete ctx;
    delete time_series;

    trace->shutdown();
    delete trace;
    trace = NULL;

    delete mainStrand;
    Network::IOServiceFactory::destroyIOService(ios);

    delete sstConnMgr;

    plugins.gc();
    sync.stop();

    Sirikata::Logging::finishLog();

    return 0;
}
