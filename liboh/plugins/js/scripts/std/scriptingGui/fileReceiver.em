system.require('std/core/bind.em');
system.require('std/core/deepCopy.em');

if (typeof(std) === "undefined") std = {};
if (typeof(std.script) === "undefined") std.script = {};


(function()
{
    function generateFolderName (vis)
    {
        var entToke = system.__entityToken();
        if (typeof(entToke) == 'undefined')
        {
            throw new Error ('Error generating a folder name when ' +
                             'receiving a file.  Have no token.');                
        }

        return 'ishmael_scripting_receiver/'+entToke+'__'+vis.toString();
    }
    
    function fileReceiver(msg,sender)
    {
        var newFilename =
            generateFolderName(sender) + msg.filename;

        system.__debugFileWrite(msg.text,newFilename);
        msg.makeReply({}) >> [];
    }
    fileReceiver << [{'fileManagerUpdate'::},
                     {'filename'::},
                     {'version'::},
                     {'text'::}];



    //traditional script import

    var ns = std.script;

    
    /** A Scriptable is an object which listens for messages from
     *  other objects, executes their contents, and replies with the
     *  results.
     */
    ns.Scriptable = function() {
        var scriptRequestPattern = new util.Pattern("request", "script");
        var scriptRequestHandler = std.core.bind(this._handleScriptRequest, this);
        scriptRequestHandler << scriptRequestPattern;
    };


    ns.Scriptable.prototype._handleScriptRequest = function(msg, sender)
    {
        var prevPrefix = system.__getImportPrefix();
        try
        {

            if (sender.toString() != system.self.toString())
            {
                //any call to import from these files will first check in
                //these directories.
                system.__setImportPrefix(generateFolderName(sender));
            }
            
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
                result = system.__evalInGlobal(cmd);
            } catch (ex) {
                // Currently, we need to do a deep copy because some
                // fields seem to be some sort of weird built in type (or
                // accessor) which the C++ V8 API doesn't seem to
                // expose. Copying forces the data to be evaluated into
                // real values. So far we've only seen this with
                // exceptions thrown by the V8 runtime (ones generated by
                // our C++ code are fine)
                excep = std.core.deepCopy(ex);
            }

            // This is the old version, not using makeReply. Now
            // deprecated, but leaving in so old scripts/clients continue
            // to work
            var retmsg = {
                'reply' : 'script',
                'value' : result,
                'exception' : excep,
                'deprecated' : true
            };
            retmsg >> sender >> [];
            
            // This is the new version and is the one you should actually
            // use now. Note that it *doesn't* contain the reply: 'script'
            // field. We don't need it since the reply is automatically
            // matched (more conservatively than the old style as well),
            // but also so that old style matching doesn't match this
            // message.
            var retmsg_new = {
                'value' : result,
                'exception' : excep
            };
            msg.makeReply(retmsg_new) >> [];
        }
        catch(excep)
        { }
        finally
        {
            system.__setImportPrefix(prevPrefix);
        }
        
    };

        ns.Scriptable.prototype._handlePrint = function() {
            var print_msg = {
                'request': 'print',
                'print': (arguments.length == 1 ? arguments[0] : arguments)
            };
            print_msg >> this._printer >> [];
        };

        ns.Scriptable.prototype._handlePrinterTimeout = function() {
            system.onPrint(undefined);
            this._timerPrintout = null;
        };


})();
