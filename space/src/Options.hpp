/*  Sirikata
 *  Options.hpp
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

#ifndef _SIRIKATA_SPACE_OPTIONS_HPP_
#define _SIRIKATA_SPACE_OPTIONS_HPP_

#define OPT_CONFIG_FILE          "cfg"

#define OPT_SPACE_PLUGINS           "space.plugins"
#define OPT_SPACE_EXTRA_PLUGINS     "space.extra-plugins"

#define SERVER_QUEUE         "server.queue"
#define SERVER_QUEUE_LENGTH  "server.queue.length"
#define SERVER_RECEIVER      "server.receiver"
#define SERVER_ODP_FLOW_SCHEDULER   "server.odp.flowsched"

#define NETWORK_TYPE         "net"

#define CSEG                "cseg"

#define SPACE_OPT_AUTH                        "auth"
#define SPACE_OPT_AUTH_OPTIONS                "auth-options"

#define LOC                        "loc"
#define LOC_OPTIONS                "loc-options"
#define LOC_UPDATE                 "loc-update"
#define LOC_UPDATE_OPTIONS         "loc-update-options"

#define OSEG                       "oseg"
#define OSEG_OPTIONS               "oseg-options"
#define OSEG_CACHE_SIZE              "oseg-cache-size"
#define OSEG_CACHE_CLEAN_GROUP_SIZE  "oseg-cache-clean-group-size"
#define OSEG_CACHE_ENTRY_LIFETIME    "oseg-cache-entry-lifetime"

#define CACHE_SELECTOR              "oseg-cache-selector"
#define CACHE_TYPE_COMMUNICATION    "cache_communication"
#define CACHE_TYPE_ORIGINAL_LRU     "cache_originallru"


#define CACHE_COMM_SCALING          "oseg-cache-scaling"

#define FORWARDER_SEND_QUEUE_SIZE "forwarder.send-queue-size"
#define FORWARDER_RECEIVE_QUEUE_SIZE "forwarder.receive-queue-size"

#define OSEG_LOOKUP_QUEUE_SIZE     "oseg_lookup_queue_size"

#define OPT_PROX                   "prox"
#define OPT_PROX_OPTIONS           "prox-options"

#define OPT_MODULES                "modules"

// AggregateManager options
#define OPT_AGGMGR_HOSTNAME          "aggmgr.host"
#define OPT_AGGMGR_SERVICE           "aggmgr.service"
#define OPT_AGGMGR_CONSUMER_KEY      "aggmgr.consumer-key"
#define OPT_AGGMGR_CONSUMER_SECRET   "aggmgr.consumer-secret"
#define OPT_AGGMGR_ACCESS_KEY        "aggmgr.access-key"
#define OPT_AGGMGR_ACCESS_SECRET     "aggmgr.access-secret"
#define OPT_AGGMGR_USERNAME          "aggmgr.username"
#define OPT_AGGMGR_SKIP_UPLOAD       "aggmgr.skip-upload"

namespace Sirikata {

void InitSpaceOptions();

} // namespace Sirikata


#endif //_SIRIKATA_SPACE_OPTIONS_HPP_
