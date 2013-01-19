/*  Sirikata
 *  Options.cpp
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

#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/util/Time.hpp>

namespace Sirikata {

void InitPintoOptions() {
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)

        .addOption(new OptionValue(OPT_CONFIG_FILE,"pinto.cfg",Sirikata::OptionValueType<String>(),"Configuration file to load."))

        .addOption(new OptionValue(OPT_PINTO_PLUGINS,"",Sirikata::OptionValueType<String>(),"Plugin list to load."))

        .addOption(new OptionValue(OPT_PINTO_PROTOCOL, "tcpsst", Sirikata::OptionValueType<String>(), "Protocol to use for connections from space servers."))
        .addOption(new OptionValue(OPT_PINTO_PROTOCOL_OPTIONS, "", Sirikata::OptionValueType<String>(), "Protocol options to use for connections from space servers."))
        .addOption(new OptionValue(OPT_PINTO_HOST, "0.0.0.0", Sirikata::OptionValueType<String>(), "IP address or host to listen for connections from space servers on."))
        .addOption(new OptionValue(OPT_PINTO_PORT, "6789", Sirikata::OptionValueType<String>(), "Port to listen for connections from space servers."))

        .addOption(new OptionValue(OPT_PINTO_TYPE, "solidangle", Sirikata::OptionValueType<String>(), "Type of query/protocol the pinto server should run. Currently options are solidangle (responds to aggregate solid angle queries) and manual (manual walking of data structure)."))

        .addOption(new OptionValue(OPT_PINTO_HANDLER_TYPE, "rtreecut", Sirikata::OptionValueType<String>(), "Type of libprox query handler to use for queries from servers."))
        .addOption(new OptionValue(OPT_PINTO_HANDLER_OPTIONS, "", Sirikata::OptionValueType<String>(), "Options for the query handler."))
        .addOption(new OptionValue(OPT_PINTO_HANDLER_NODE_DATA, "maxsize", Sirikata::OptionValueType<String>(), "Per-node data, e.g. bounds, maxsize, similarmaxsize."))
        ;
}

} // namespace Sirikata
