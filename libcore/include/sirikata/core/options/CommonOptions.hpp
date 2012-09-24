/*  Sirikata
 *  CommonOptions.hpp
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

#ifndef _SIRIKATA_COMMON_OPTIONS_HPP_
#define _SIRIKATA_COMMON_OPTIONS_HPP_

#include <sirikata/core/util/Platform.hpp>

#define OPT_CRASHREPORT_URL       "crashreport"

#define OPT_PLUGINS               "plugins"
#define OPT_EXTRA_PLUGINS         "extra-plugins"

#define OPT_LOG_FILE                  "log-file"
#define OPT_LOG_ALL_TO_FILE           "log-all-to-file"
#define OPT_DAEMON                    "daemon"
#define OPT_PID_FILE                    "pid-file"

#define OPT_SST_DEFAULT_WINDOW_SIZE  "sst.default-window-size"

#define STATS_TRACE_FILE     "stats.trace-filename"
#define PROFILE                    "profile"

#define OPT_REGION_WEIGHT        "region-weight"
#define OPT_REGION_WEIGHT_ARGS   "region-weight-args"

#define OPT_CDN_HOST             "cdn.host"
#define OPT_CDN_SERVICE          "cdn.service"
#define OPT_CDN_DNS_URI_PREFIX   "cdn.dns.prefix"
#define OPT_CDN_DOWNLOAD_URI_PREFIX     "cdn.download.prefix"
#define OPT_CDN_UPLOAD_URI_PREFIX   "cdn.upload.prefix"
#define OPT_CDN_UPLOAD_STATUS_URI_PREFIX   "cdn.upload.status.prefix"

#define OPT_TRACE_TIMESERIES           "trace.timeseries"
#define OPT_TRACE_TIMESERIES_OPTIONS   "trace.timeseries-options"

#define OPT_COMMAND_COMMANDER           "command.commander"
#define OPT_COMMAND_COMMANDER_OPTIONS   "command.commander-options"

namespace Sirikata {

/// Report version information to the log
SIRIKATA_FUNCTION_EXPORT void ReportVersion();

SIRIKATA_FUNCTION_EXPORT void InitOptions();

enum UnregisteredOptionBehavior {
    AllowUnregisteredOptions,
    FailOnUnregisteredOptions
};

SIRIKATA_FUNCTION_EXPORT void ParseOptions(int argc, char** argv, UnregisteredOptionBehavior unreg = FailOnUnregisteredOptions);
SIRIKATA_FUNCTION_EXPORT void ParseOptionsFile(const String& fname, bool required=true, UnregisteredOptionBehavior unreg = FailOnUnregisteredOptions);

/** Parse command line options and config files, ensuring the command line
 *  arguments take priority but reading the config file from an option rather
 *  than hard coding it.
 */
SIRIKATA_FUNCTION_EXPORT void ParseOptions(int argc, char** argv, const String& config_file_option, UnregisteredOptionBehavior unreg = FailOnUnregisteredOptions);

// Parses empty options to get options properly initialized
SIRIKATA_FUNCTION_EXPORT void FakeParseOptions();

/// Fills in default values, used after initial parsing to make sure we don't
/// block overriding option values from a secondary source (e.g. config files)
/// after parsing the first source (e.g. cmd line)
SIRIKATA_FUNCTION_EXPORT void FillMissingOptionDefaults();


/// Daemonizes the process if requested and then sets up output, e.g. logfile,
/// remaps std::err, generates pidfile, etc.
SIRIKATA_FUNCTION_EXPORT void DaemonizeAndSetOutputs();
SIRIKATA_FUNCTION_EXPORT void DaemonCleanup();

// Be careful with GetOption.  Using it and ->as() directly can be dangerous
// because some types are defined per-library and won't dynamic_cast properly.
// It is suggested that you use GetOptionValue where possible.
SIRIKATA_FUNCTION_EXPORT OptionValue* GetOption(const char* name);
SIRIKATA_FUNCTION_EXPORT OptionValue* GetOption(const char* klass, const char* name);

template<typename T>
T GetOptionValue(const char* name) {
    OptionValue* opt = GetOption(name);
    return opt->as<T>();
}

template<typename T>
T GetOptionValue(const char* klass, const char* name) {
    OptionValue* opt = GetOption(klass, name);
    return opt->unsafeAs<T>(); // FIXME should be ->as<T>();
}

template<>
SIRIKATA_FUNCTION_EXPORT String GetOptionValue<String>(const char* name);
template<>
SIRIKATA_FUNCTION_EXPORT Vector3f GetOptionValue<Vector3f>(const char* name);
template<>
SIRIKATA_FUNCTION_EXPORT Vector3ui32 GetOptionValue<Vector3ui32>(const char* name);
template<>
SIRIKATA_FUNCTION_EXPORT BoundingBox3f GetOptionValue<BoundingBox3f>(const char* name);
template<>
SIRIKATA_FUNCTION_EXPORT ObjectHostID GetOptionValue<ObjectHostID>(const char* name);
template<>
SIRIKATA_FUNCTION_EXPORT Task::DeltaTime GetOptionValue<Task::DeltaTime>(const char* name);
template<>
SIRIKATA_FUNCTION_EXPORT uint32 GetOptionValue<uint32>(const char* name);
template<>
SIRIKATA_FUNCTION_EXPORT int32 GetOptionValue<int32>(const char* name);
template<>
SIRIKATA_FUNCTION_EXPORT uint64 GetOptionValue<uint64>(const char* name);
template<>
SIRIKATA_FUNCTION_EXPORT int64 GetOptionValue<int64>(const char* name);
template<>
SIRIKATA_FUNCTION_EXPORT bool GetOptionValue<bool>(const char* name);


SIRIKATA_FUNCTION_EXPORT String GetPerServerString(const String& orig, const ServerID& sid);
/** Get an option which is a filename and modify it to be server specific. */
SIRIKATA_FUNCTION_EXPORT String GetPerServerFile(const char* opt_name, const ServerID& sid);
SIRIKATA_FUNCTION_EXPORT String GetPerServerFile(const char* opt_name, const ObjectHostID& ohid);

} // namespace Sirikata


#endif //_SIRIKATA_COMMON_OPTIONS_HPP_
