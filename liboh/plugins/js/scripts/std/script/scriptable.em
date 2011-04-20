/*  Sirikata
 *  scriptable.em
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

system.require('std/core/bind.js');

if (typeof(std) === "undefined") std = {};
if (typeof(std.script) === "undefined") std.script = {};

(
function() {

    var ns = std.script;

    /** A Scriptable is an object which listens for messages from
     *  other objects, executes their contents, and replies with the
     *  results.
     */
    ns.Scriptable = function() {
        var scriptRequestPattern = new util.Pattern("request", "script");
        var scriptRequestHandler = std.core.bind(this._handleScriptRequest, this);
        scriptRequestHandler <- scriptRequestPattern;
    };

    var deepCopy = function(val) {
        if (typeof(val) === "object") {
            var ret = {};
            var props = Object.getOwnPropertyNames(val);
            for(var f in props)
                ret[props[f]] = deepCopy(val[props[f]]);
            return ret;
        }
        else
            return val;
    };
    ns.Scriptable.prototype._handleScriptRequest = function(msg, sender) {
        if (!this._printer || sender != this._printer) {
            this._printer = sender;
            system.onPrint( std.core.bind(this._handlePrint, this) );
        }

        if (this._printerTimeout)
            this._printerTimeout.reset();
        this._printerTimeout = system.timeout(60, std.core.bind(this._handlePrinterTimeout, this));

        var cmd = msg.script;
        var result = undefined, excep = undefined;
        try {
            result = system.eval(cmd);
        } catch (ex) {
            // Currently, we need to do a deep copy because some
            // fields seem to be some sort of weird built in type (or
            // accessor) which the C++ V8 API doesn't seem to
            // expose. Copying forces the data to be evaluated into
            // real values. So far we've only seen this with
            // exceptions thrown by the V8 runtime (ones generated by
            // our C++ code are fine)
            excep = deepCopy(ex);
        }
        var retmsg = {
            reply : 'script',
            value : result,
            exception : excep
        };
        retmsg -> sender;
    };

    ns.Scriptable.prototype._handlePrint = function() {
        var print_msg = {
            request: 'print',
            print: (arguments.length == 1 ? arguments[0] : arguments)
        };
        print_msg -> this._printer;
    };

    ns.Scriptable.prototype._handlePrinterTimeout = function() {
        system.onPrint(undefined);
        this._timerPrintout = null;
    };

})();