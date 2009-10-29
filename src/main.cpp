/*  cbr
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

#include "Timer.hpp"
#include "TimeSync.hpp"
#include "TimeProfiler.hpp"

#include "Network.hpp"

#include "Forwarder.hpp"

#include "Object.hpp"
#include "ObjectFactory.hpp"

#include "LocationService.hpp"

#include "Proximity.hpp"
#include "Server.hpp"

#include "Options.hpp"
#include "Statistics.hpp"
#include "Analysis.hpp"
#include "Visualization.hpp"
#include "OracleLocationService.hpp"
#include "StandardLocationService.hpp"
#include "Test.hpp"
#include "SSTNetwork.hpp"
#include "ENetNetwork.hpp"
#include "TCPNetwork.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "FairServerMessageQueue.hpp"
#include "TabularServerIDMap.hpp"
#include "ExpIntegral.hpp"
#include "SqrIntegral.hpp"
#include "UniformCoordinateSegmentation.hpp"
#include "DistributedCoordinateSegmentation.hpp"
#include "CoordinateSegmentationClient.hpp"
#include "LoadMonitor.hpp"
#include "LocObjectSegmentation.hpp"
#include "CraqObjectSegmentation.hpp"



#include "ServerWeightCalculator.hpp"
namespace {
CBR::Network* gNetwork = NULL;
CBR::Trace* gTrace = NULL;
CBR::SpaceContext* gSpaceContext = NULL;
CBR::Time g_start_time( CBR::Time::null() );
}



void *main_loop(void *);

bool is_analysis() {
    using namespace CBR;

    if (GetOption(ANALYSIS_LOC)->as<bool>() ||
        GetOption(ANALYSIS_LOCVIS)->as<String>() != "none" ||
        GetOption(ANALYSIS_LATENCY)->as<bool>() ||
        GetOption(ANALYSIS_OBJECT_LATENCY)->as<bool>() ||
        GetOption(ANALYSIS_MESSAGE_LATENCY)->as<bool>() ||
        GetOption(ANALYSIS_BANDWIDTH)->as<bool>() ||
        !GetOption(ANALYSIS_WINDOWED_BANDWIDTH)->as<String>().empty() ||
        GetOption(ANALYSIS_OSEG)->as<bool>() ||
        GetOption(ANALYSIS_OBJECT_LATENCY)->as<bool>() ||
        GetOption(ANALYSIS_LOC_LATENCY)->as<bool>() ||
        !GetOption(ANALYSIS_PROX_DUMP)->as<String>().empty())
        return true;

    return false;
}

int main(int argc, char** argv) {
    using namespace CBR;

    InitOptions();
    ParseOptions(argc, argv);

    std::string time_server=GetOption("time-server")->as<String>();
    TimeSync sync;

    if (GetOption("cseg")->as<String>() != "distributed")
      sync.start(time_server);


    ServerID server_id = GetOption("id")->as<ServerID>();
    String trace_file = is_analysis() ? "analysis.trace" : GetPerServerFile(STATS_TRACE_FILE, server_id);
    gTrace = new Trace(trace_file);

    // Compute the starting date/time
    String start_time_str = GetOption("wait-until")->as<String>();
    g_start_time = start_time_str.empty() ? Timer::now() : Timer::getSpecifiedDate( start_time_str );
    Time start_time = g_start_time;
    start_time += GetOption("wait-additional")->as<Duration>();

    Duration duration = GetOption("duration")->as<Duration>();

    IOService* ios = IOServiceFactory::makeIOService();
    IOStrand* mainStrand = ios->createStrand();

    Time init_space_ctx_time = Time::null() + (Timer::now() - start_time);
    SpaceContext* space_context = new SpaceContext(server_id, ios, mainStrand, start_time, init_space_ctx_time, gTrace, duration);
    gSpaceContext = space_context;

    String network_type = GetOption(NETWORK_TYPE)->as<String>();
    if (network_type == "sst")
        gNetwork = new SSTNetwork(space_context);
    else if (network_type == "enet")
        gNetwork = new ENetNetwork(space_context,65536,GetOption(RECEIVE_BANDWIDTH)->as<uint32>(),GetOption(SEND_BANDWIDTH)->as<uint32>());
    else if (network_type == "tcp")
        gNetwork = new TCPNetwork(space_context,4096,GetOption(RECEIVE_BANDWIDTH)->as<uint32>(),GetOption(SEND_BANDWIDTH)->as<uint32>());
    gNetwork->init(&main_loop);

    sync.stop();

    return 0;
}
void *main_loop(void *) {
    using namespace CBR;

    ServerID server_id = GetOption("id")->as<ServerID>();

    String test_mode = GetOption("test")->as<String>();
    if (test_mode != "none") {
        String server_port = GetOption("server-port")->as<String>();
        String client_port = GetOption("client-port")->as<String>();
        String host = GetOption("host")->as<String>();
        if (test_mode == "server")
            CBR::testServer(server_port.c_str(), host.c_str(), client_port.c_str());
        else if (test_mode == "client")
            CBR::testClient(client_port.c_str(), host.c_str(), server_port.c_str());
        return 0;
    }


    BoundingBox3f region = GetOption("region")->as<BoundingBox3f>();
    Vector3ui32 layout = GetOption("layout")->as<Vector3ui32>();


    uint32 max_space_servers = GetOption("max-servers")->as<uint32>();
    if (max_space_servers == 0)
      max_space_servers = layout.x * layout.y * layout.z;
    uint32 num_oh_servers = GetOption("num-oh")->as<uint32>();
    uint32 nservers = max_space_servers + num_oh_servers;



    Duration duration = GetOption("duration")->as<Duration>();

    srand( GetOption("rand-seed")->as<uint32>() );

    SpaceContext* space_context = gSpaceContext;
    Forwarder* forwarder = new Forwarder(space_context);

    // FIXME we shouldn't need to instantiate these for space, only needed for analysis
    IOService* oh_ios = IOServiceFactory::makeIOService();
    IOStrand* oh_mainStrand = oh_ios->createStrand();
    ObjectHostContext* oh_ctx = new ObjectHostContext(0, oh_ios, oh_mainStrand, NULL, Time::null(), Time::null(), duration);
    ObjectFactory* obj_factory = new ObjectFactory(oh_ctx, region, duration);

    LocationService* loc_service = NULL;
    String loc_service_type = GetOption(LOC)->as<String>();
    if (loc_service_type == "oracle")
        loc_service = new OracleLocationService(space_context, obj_factory);
    else if (loc_service_type == "standard")
        loc_service = new StandardLocationService(space_context);
    else
        assert(false);

    String filehandle = GetOption("serverips")->as<String>();
    std::ifstream ipConfigFileHandle(filehandle.c_str());
    ServerIDMap * server_id_map = new TabularServerIDMap(ipConfigFileHandle);
    gTrace->setServerIDMap(server_id_map);



    ServerMessageQueue* sq = NULL;
    String server_queue_type = GetOption(SERVER_QUEUE)->as<String>();
    if (server_queue_type == "fifo")
        sq = new FIFOServerMessageQueue(space_context, gNetwork, server_id_map, GetOption(SEND_BANDWIDTH)->as<uint32>(),GetOption(RECEIVE_BANDWIDTH)->as<uint32>());
    else if (server_queue_type == "fair")
        sq = new FairServerMessageQueue(space_context, gNetwork, server_id_map, GetOption(SEND_BANDWIDTH)->as<uint32>(),GetOption(RECEIVE_BANDWIDTH)->as<uint32>());
    else {
        assert(false);
        exit(-1);
    }


    String cseg_type = GetOption(CSEG)->as<String>();
    CoordinateSegmentation* cseg = NULL;
    if (cseg_type == "uniform")
        cseg = new UniformCoordinateSegmentation(space_context, region, layout);
    else if (cseg_type == "distributed") {
      cseg = new DistributedCoordinateSegmentation(space_context, region, layout, max_space_servers, server_id_map);
    }
    else if (cseg_type == "client") {
      cseg = new CoordinateSegmentationClient(space_context, region, layout, server_id_map);
    }
    else {
        assert(false);
        exit(-1);
    }


    LoadMonitor* loadMonitor = new LoadMonitor(space_context, sq, cseg);



    if ( GetOption(ANALYSIS_LOC)->as<bool>() ) {
        LocationErrorAnalysis lea(STATS_TRACE_FILE, nservers);
        printf("Total error: %f\n", (float)lea.globalAverageError( Duration::milliseconds((int64)10), obj_factory));
        exit(0);
    }
    else if ( GetOption(ANALYSIS_LOCVIS)->as<String>() != "none") {
        String vistype = GetOption(ANALYSIS_LOCVIS)->as<String>();
        LocationVisualization lea(STATS_TRACE_FILE, nservers, space_context, obj_factory,cseg);

        if (vistype == "object")
            lea.displayRandomViewerError(GetOption(ANALYSIS_LOCVIS_SEED)->as<int>(), Duration::milliseconds((int64)30));
        else if (vistype == "server")
            lea.displayRandomServerError(GetOption(ANALYSIS_LOCVIS_SEED)->as<int>(), Duration::milliseconds((int64)30));

        exit(0);
    }
    else if ( GetOption(ANALYSIS_LATENCY)->as<bool>() ) {
        LatencyAnalysis la(STATS_TRACE_FILE,nservers);

        exit(0);
    }
    else if ( GetOption(ANALYSIS_OBJECT_LATENCY)->as<bool>() ) {
        ObjectLatencyAnalysis la(STATS_TRACE_FILE,nservers);
        std::ofstream histogram_data("distance_latency_histogram.csv");
        la.printHistogramDistanceData(histogram_data,10);
        histogram_data.close();
        exit(0);
    }
    else if ( GetOption(ANALYSIS_MESSAGE_LATENCY)->as<bool>() ) {
        uint16 ping_port=OBJECT_PORT_PING;
        uint32 unservers=nservers;
        MessageLatencyAnalysis::Filters filter(&ping_port,&unservers,//filter by created @ object host
                       &unservers);//filter by destroyed @ object host
        MessageLatencyAnalysis::Filters nilfilter;
        MessageLatencyAnalysis::Filters pingfilter(&ping_port);
        MessageLatencyAnalysis la(STATS_TRACE_FILE,nservers,pingfilter);
        exit(0);
    }
    else if ( GetOption(ANALYSIS_BANDWIDTH)->as<bool>() ) {
        BandwidthAnalysis ba(STATS_TRACE_FILE, max_space_servers);
        printf("Send rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                ba.computeSendRate(sender, receiver);
            }
        }
        printf("Receive rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                ba.computeReceiveRate(sender, receiver);
            }
        }

        ba.computeJFI(server_id);
        exit(0);
    }
    else if ( !GetOption(ANALYSIS_WINDOWED_BANDWIDTH)->as<String>().empty() ) {
        String windowed_analysis_type = GetOption(ANALYSIS_WINDOWED_BANDWIDTH)->as<String>();

        String windowed_analysis_send_filename = "windowed_bandwidth_";
        windowed_analysis_send_filename += windowed_analysis_type;
        windowed_analysis_send_filename += "_send.dat";
        String windowed_analysis_receive_filename = "windowed_bandwidth_";
        windowed_analysis_receive_filename += windowed_analysis_type;
        windowed_analysis_receive_filename += "_receive.dat";

        String queue_info_filename = "queue_info_";
        queue_info_filename += windowed_analysis_type;
        queue_info_filename += ".dat";


        String windowed_queue_info_send_filename = "windowed_queue_info_send_";
        windowed_queue_info_send_filename += windowed_analysis_type;
        windowed_queue_info_send_filename += ".dat";
        String windowed_queue_info_receive_filename = "windowed_queue_info_receive_";
        windowed_queue_info_receive_filename += windowed_analysis_type;
        windowed_queue_info_receive_filename += ".dat";

        std::ofstream windowed_analysis_send_file(windowed_analysis_send_filename.c_str());
        std::ofstream windowed_analysis_receive_file(windowed_analysis_receive_filename.c_str());

        std::ofstream queue_info_file(queue_info_filename.c_str());

        std::ofstream windowed_queue_info_send_file(windowed_queue_info_send_filename.c_str());
        std::ofstream windowed_queue_info_receive_file(windowed_queue_info_receive_filename.c_str());

        Duration window = GetOption(ANALYSIS_WINDOWED_BANDWIDTH_WINDOW)->as<Duration>();
        Duration sample_rate = GetOption(ANALYSIS_WINDOWED_BANDWIDTH_RATE)->as<Duration>();
        BandwidthAnalysis ba(STATS_TRACE_FILE, max_space_servers);
        Time start_time = Time::null();
        Time end_time = start_time + duration;
        printf("Send rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.computeWindowedDatagramSendRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_send_file);
                else if (windowed_analysis_type == "packet")
                    ba.computeWindowedPacketSendRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_send_file);
            }
        }
        printf("Receive rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.computeWindowedDatagramReceiveRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_receive_file);
                else if (windowed_analysis_type == "packet")
                    ba.computeWindowedPacketReceiveRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_receive_file);
            }
        }
        // Queue information
        //  * Raw dump
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.dumpDatagramQueueInfo(sender, receiver, std::cout, queue_info_file);
                else if (windowed_analysis_type == "packet")
                    ba.dumpPacketQueueInfo(sender, receiver, std::cout, queue_info_file);
            }
        }
        //  * Send
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.windowedDatagramSendQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_send_file);
                else if (windowed_analysis_type == "packet")
                    ba.windowedPacketSendQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_send_file);
            }
        }
        //  * Receive
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.windowedDatagramReceiveQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_receive_file);
                else if (windowed_analysis_type == "packet")
                    ba.windowedPacketReceiveQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_receive_file);
            }
        }

        exit(0);
    }
    else if ( GetOption(ANALYSIS_OSEG)->as<bool>() )
    {
      //bftm additional object messages log file creation.
      int osegProcessedAfterSeconds = GetOption(OSEG_ANALYZE_AFTER)->as<int>();


        //oseg migrates
        String object_segmentation_filename = "oseg_object_segmentation_file";
        object_segmentation_filename += ".dat";

        ObjectSegmentationAnalysis osegAnalysis (STATS_TRACE_FILE, max_space_servers);
        std::ofstream object_seg_stream (object_segmentation_filename.c_str());
        osegAnalysis.printData(object_seg_stream);
        object_seg_stream.flush();
        object_seg_stream.close();

        //oseg craq lookups
        String object_segmentation_craq_lookup_filename = "oseg_object_segmentation_craq_lookup_file";
        object_segmentation_craq_lookup_filename += ".dat";

        ObjectSegmentationCraqLookupRequestsAnalysis craqLookupAnalysis(STATS_TRACE_FILE,max_space_servers);
        std::ofstream oseg_craq_lookup_stream(object_segmentation_craq_lookup_filename.c_str());
        craqLookupAnalysis.printData(oseg_craq_lookup_stream);
        oseg_craq_lookup_stream.flush();
        oseg_craq_lookup_stream.close();


        //oseg not on server lookups
        String object_segmentation_lookup_not_on_server_filename = "oseg_object_segmentation_lookup_not_on_server_file";
        object_segmentation_lookup_not_on_server_filename += ".dat";

        ObjectSegmentationLookupNotOnServerRequestsAnalysis lookupNotOnServerAnalysis(STATS_TRACE_FILE,max_space_servers);
        std::ofstream oseg_lookup_not_on_server_stream(object_segmentation_lookup_not_on_server_filename.c_str());
        lookupNotOnServerAnalysis.printData(oseg_lookup_not_on_server_stream);
        oseg_lookup_not_on_server_stream.flush();
        oseg_lookup_not_on_server_stream.close();


        //oseg processed lookups
        String object_segmentation_processed_filename = "oseg_object_segmentation_processed_file";
        object_segmentation_processed_filename += ".dat";


        ObjectSegmentationProcessedRequestsAnalysis processedAnalysis(STATS_TRACE_FILE,max_space_servers);
        std::ofstream oseg_process_stream(object_segmentation_processed_filename.c_str());

        processedAnalysis.printData(oseg_process_stream, true, osegProcessedAfterSeconds);
        oseg_process_stream.flush();
        oseg_process_stream.close();

        //completed round trip migrate times
        String migration_round_trip_times_filename = "oseg_migration_round_trip_times_file";
        migration_round_trip_times_filename += ".dat";

        ObjectMigrationRoundTripAnalysis obj_mig_rdTripAnalysis(STATS_TRACE_FILE,max_space_servers);

        std::ofstream mig_rd_trip_times_stream(migration_round_trip_times_filename.c_str());

        obj_mig_rdTripAnalysis.printData(mig_rd_trip_times_stream, osegProcessedAfterSeconds);

        mig_rd_trip_times_stream.flush();
        mig_rd_trip_times_stream.close();

        //oseg shut down
        String oseg_shutdown_filename = "oseg_shutdown_file";
        oseg_shutdown_filename += ".dat";

        OSegShutdownAnalysis oseg_shutdown_analysis(STATS_TRACE_FILE,max_space_servers);

        std::ofstream oseg_shutdown_analysis_stream (oseg_shutdown_filename.c_str());
        oseg_shutdown_analysis.printData(oseg_shutdown_analysis_stream);

        oseg_shutdown_analysis_stream.flush();
        oseg_shutdown_analysis_stream.close();


        //oseg tracked set results analysis
        String oseg_tracked_set_results_filename = "oseg_tracked_set_results_file";
        oseg_tracked_set_results_filename += ".dat";

        OSegTrackedSetResultsAnalysis oseg_tracked_set_res_analysis(STATS_TRACE_FILE,max_space_servers);

        std::ofstream oseg_tracked_set_results_analysis_stream (oseg_tracked_set_results_filename.c_str());
        oseg_tracked_set_res_analysis.printData(oseg_tracked_set_results_analysis_stream,osegProcessedAfterSeconds);

        oseg_tracked_set_results_analysis_stream.flush();
        oseg_tracked_set_results_analysis_stream.close();


        //oseg cache response analysis
        String oseg_cache_response_filename = "oseg_cache_response_file";
        oseg_cache_response_filename += ".dat";

        OSegCacheResponseAnalysis oseg_cache_response_analysis(STATS_TRACE_FILE, max_space_servers);

        std::ofstream oseg_cached_response_analysis_stream(oseg_cache_response_filename.c_str());

        oseg_cache_response_analysis.printData(oseg_cached_response_analysis_stream,osegProcessedAfterSeconds);

        oseg_cached_response_analysis_stream.flush();
        oseg_cached_response_analysis_stream.close();

        //end cache response analysis

        //cached error analysis
        String oseg_cached_response_error_filename = "oseg_cached_response_error_file";
        oseg_cached_response_error_filename += ".dat";

        OSegCacheErrorAnalysis oseg_cached_error_response (STATS_TRACE_FILE,max_space_servers);

        std::ofstream oseg_cached_error_response_stream(oseg_cached_response_error_filename.c_str());
        oseg_cached_error_response.printData(oseg_cached_error_response_stream,osegProcessedAfterSeconds);

        oseg_cached_error_response_stream.flush();
        oseg_cached_error_response_stream.close();

        //end cached error analysis


        //end bftm additional object message log file creation.

        exit(0);
    }
    else if ( GetOption(ANALYSIS_LOC_LATENCY)->as<bool>() ) {
        LocationLatencyAnalysis(STATS_TRACE_FILE, nservers);
        exit(0);
    }
    else if ( !GetOption(ANALYSIS_PROX_DUMP)->as<String>().empty() ) {
        ProximityDumpAnalysis(STATS_TRACE_FILE, nservers, GetOption(ANALYSIS_PROX_DUMP)->as<String>());
        exit(0);
    }



    //Create OSeg
    std::string oseg_type=GetOption(OSEG)->as<String>();

    ObjectSegmentation* oseg;

    if (oseg_type == OSEG_OPTION_LOC && cseg_type != "distributed")
    {
      //using loc approach
      std::map<UUID,ServerID> dummyObjectToServerMap; //bftm note: this should be filled in later with a list of object ids and where they are located

      //Trying to populate objectToServerMap
      // FIXME this needs to go away, we can't rely on the object factory being there
      for(ObjectFactory::iterator it = obj_factory->begin(); it != obj_factory->end(); it++)
      {
        UUID obj_id = *it;
        Vector3f start_pos = obj_factory->motion(obj_id)->initial().extrapolate(Time::null()).position();
        dummyObjectToServerMap[obj_id] = cseg->lookup(start_pos);
      }

      //      ObjectSegmentation* oseg = new LocObjectSegmentation(space_context, cseg,loc_service,dummyObjectToServerMap);
      oseg = new LocObjectSegmentation(space_context, cseg,loc_service,dummyObjectToServerMap,forwarder);
    }

    if (oseg_type == OSEG_OPTION_CRAQ && cseg_type != "distributed")
    {
      //using craq approach
      std::vector<UUID> initServObjVec;

     std::vector<CraqInitializeArgs> craqArgsGet;
     CraqInitializeArgs cInitArgs1;

     cInitArgs1.ipAdd = "localhost";
     cInitArgs1.port  =     "10299";
     craqArgsGet.push_back(cInitArgs1);

     std::vector<CraqInitializeArgs> craqArgsSet;
     CraqInitializeArgs cInitArgs2;
     cInitArgs2.ipAdd = "localhost";
     cInitArgs2.port  =     "10298";
     craqArgsSet.push_back(cInitArgs2);




     std::string oseg_craq_prefix=GetOption(OSEG_UNIQUE_CRAQ_PREFIX)->as<String>();

     if (oseg_type.size() ==0)
     {
       std::cout<<"\n\nERROR: Incorrect craq prefix for oseg.  String must be at least one letter long.  (And be between G and Z.)  Please try again.\n\n";
       assert(false);
     }

     std::cout<<"\n\nUniquely appending  "<<oseg_craq_prefix[0]<<"\n\n";
     std::cout<<"\n\nAre any of my changes happening?\n\n";
     oseg = new CraqObjectSegmentation (space_context, cseg, initServObjVec, craqArgsGet, craqArgsSet, oseg_craq_prefix[0]);

    }      //end craq approach

    //end create oseg



    ServerWeightCalculator* weight_calc = NULL;
    if (cseg_type != "distributed") {
      if (GetOption("gaussian")->as<bool>()) {
        weight_calc =
        new ServerWeightCalculator(
            server_id,
            cseg,

            std::tr1::bind(&integralExpFunction,GetOption("flatness")->as<double>(),
                           std::tr1::placeholders::_1,
                           std::tr1::placeholders::_2,
                           std::tr1::placeholders::_3,
                           std::tr1::placeholders::_4),
            sq
        );
      }else {
        weight_calc =
        new ServerWeightCalculator(
            server_id,
            cseg,
            std::tr1::bind(SqrIntegral(false),GetOption("const-cutoff")->as<double>(),GetOption("flatness")->as<double>(),
                std::tr1::placeholders::_1,
                std::tr1::placeholders::_2,
                std::tr1::placeholders::_3,
                           std::tr1::placeholders::_4),
            sq);
      }
    }

    // We have all the info to initialize the forwarder now
    forwarder->initialize(oseg, sq);

    Proximity* prox = new Proximity(space_context, loc_service);


    Server* server = new Server(space_context, forwarder, loc_service, cseg, prox, oseg, server_id_map->lookupExternal(space_context->id()));

      prox->initialize(cseg);

      // NOTE: we don't initialize this because nothing should require it. We no longer have object host,
      // and are only maintaining ObjectFactory for analysis, visualization, and oracle purposes.
      //obj_factory->initialize(obj_host->context());

      Time start_time = g_start_time;
    // If we're one of the initial nodes, we'll have to wait until we hit the start time
    {
        Time now_time = Timer::now();
        if (start_time > now_time) {
            Duration sleep_time = start_time - now_time;
            printf("Waiting %f seconds\n", sleep_time.toSeconds() ); fflush(stdout);
            usleep( sleep_time.toMicroseconds() );
        }
    }

    ///////////Go go go!! start of simulation/////////////////////

    // FIXME we have a special case for the distributed cseg server, this should be
    // turned into a separate binary
    if (cseg_type == "distributed") {
      srand(time(NULL));


      space_context->add(space_context);
      space_context->add(cseg);

      space_context->ioService->run();

      exit(0);
    }


    gNetwork->start();

    space_context->add(space_context);
    space_context->add(gNetwork);
    space_context->add(cseg);
    space_context->add(loc_service);
    space_context->add(prox);
    space_context->add(server);
    space_context->add(forwarder);
    space_context->add(loadMonitor);

    space_context->ioService->run();


    if (GetOption(PROFILE)->as<bool>()) {
        space_context->profiler->report();
    }

    gTrace->prepareShutdown();

    prox->shutdown();

    delete server;
    delete sq;
    delete prox;
    delete server_id_map;
    if (weight_calc != NULL)
      delete weight_calc;
    delete cseg;
    delete oseg;

    delete loc_service;
    delete obj_factory;
    delete forwarder;

    delete gNetwork;
    gNetwork=NULL;

    gTrace->shutdown();
    delete gTrace;
    gTrace = NULL;

    IOStrand* mainStrand = space_context->mainStrand;
    IOService* ios = space_context->ioService;

    delete space_context;
    space_context = NULL;

    delete mainStrand;
    IOServiceFactory::destroyIOService(ios);

    return 0;
}
