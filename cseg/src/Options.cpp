/*  Sirikata
 *  Options.cpp
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

#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/util/Time.hpp>

namespace Sirikata {

void InitCSegOptions() {
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(new OptionValue(OPT_CSEG_PLUGINS,"weight-exp",Sirikata::OptionValueType<String>(),"Plugin list to load."))

        .addOption(new OptionValue("random-splits-merges", "false", Sirikata::OptionValueType<bool>(), "Whether to enable random splits and merges in DistributedCoordinateSegmentation."))
        .addOption(new OptionValue("cseg-server-ll-port", "3234", Sirikata::OptionValueType<uint16>(), "Port where CSEG servers can be contacted for lower-tree requests."))
        .addOption(new OptionValue("num-cseg-servers", "1", Sirikata::OptionValueType<uint16>(), "Number of CSEG servers for the distributed implementation."))

        .addOption(new OptionValue("cseg-uses-world-pop", "false", Sirikata::OptionValueType<bool>(), "If true, CSEG uses the world population data to create the BSP tree."))

        .addOption(new OptionValue("cseg-world-width", "8640", Sirikata::OptionValueType<uint32>(), "The number of cells across the width of the world population dataset."))

        .addOption(new OptionValue("cseg-world-height", "3432", Sirikata::OptionValueType<uint32>(), "The number of cells across the height of the world population dataset."))

        .addOption(new OptionValue("cseg-max-leaf-population", "800", Sirikata::OptionValueType<uint32>(), "The maximum number of avatars/people at the leaf of the BSP tree."))

      .addOption(new OptionValue("cseg-population-density-file", "glds00ag.asc", Sirikata::OptionValueType<String>(), "The file containing the population density numbers."))

      .addOption(new OptionValue("cseg-servermap-options", "--filename=cseg_serverip.txt", Sirikata::OptionValueType<String>(), "The file containing the server ip list for cseg servers."))

      .addOption(new OptionValue("cseg-id", "1", Sirikata::OptionValueType<ServerID>(), "Server ID for this CSEG server"))

      .addOption(new OptionValue("additional-cseg-duration", "0s", Sirikata::OptionValueType<Duration>(), "Additional duration to run CSEG after the simulation"))

      .addOption(new OptionValue("cseg-service-tcp-port", "6234", Sirikata::OptionValueType<String>(), "TCP listening port number on host running the CSEG service (running with --cseg=distributed)"))

      .addOption(new OptionValue("num-upper-tree-cseg-servers", "1", Sirikata::OptionValueType<uint16>(), "Number of CSEG servers that solely maintain the upper tree"))

      ;
}

} // namespace Sirikata
