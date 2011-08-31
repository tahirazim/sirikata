/*  Sirikata
 *  JSObjectScript.hpp
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

#ifndef __SIRIKATA_JS_OBJECT_SCRIPT_HPP__
#define __SIRIKATA_JS_OBJECT_SCRIPT_HPP__


#include "Platform.hpp"
#include <string>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/proxyobject/SessionEventListener.hpp>

#include <boost/filesystem.hpp>

#include <v8.h>


#include <sirikata/proxyobject/ProxyCreationListener.hpp>
#include "JSObjects/JSInvokableObject.hpp"
#include "JSEntityCreateInfo.hpp"
#include <sirikata/oh/Storage.hpp>
#include <sirikata/oh/PersistedObjectSet.hpp>
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "JSObjectStructs/JSContextStruct.hpp"
#include "JSObjectScriptManager.hpp"
#include "EmersonHttpManager.hpp"
#include <sirikata/core/util/Liveness.hpp>
#include <stack>


namespace Sirikata {
namespace JS {



void printException(v8::TryCatch& try_catch);

// NOTE: virtual on Liveness because EmersonMessagingManager also uses it
class SIRIKATA_SCRIPTING_JS_EXPORT JSObjectScript : public ObjectScript, public virtual Liveness
{
public:
    JSObjectScript(JSObjectScriptManager* jMan, OH::Storage* storage, OH::PersistedObjectSet* persisted_set, const UUID& internal_id);
    virtual ~JSObjectScript();

    v8::Handle<v8::Value> debug_fileWrite(const String& strToWrite,const String& filename);
    v8::Handle<v8::Value> debug_fileRead(const String& filename);


    v8::Handle<v8::Value> executeInSandbox(JSContextStruct* jscont, v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv);


    //this function returns a context with
    v8::Local<v8::Object> createContext(JSPresenceStruct* jspres,const SpaceObjectReference& canSendTo,uint32 capNum, JSContextStruct*& internalContextField, JSContextStruct* creator);


    void initialize(const String& args, const String& script,int32 maxResThresh);

    // Sirikata::Service Interface
    virtual void start();
    virtual void stop();

    /** Print the given string to the current output. */
    void print(const String& str);


    /** Import a file, executing its contents in scope of most recent evaluation
     * stack frame.
     * Should not call import directly unless we are positive that we are inside
     * a valid mEvalContext.
     */
    v8::Handle<v8::Value> import(const String& filename, bool isJS);

    /**
       Adds jscont to evalStack.  Imports shim files and evals toEval.
       Pops jscont after this.
     */
    void shimImportAndEvalScript(JSContextStruct* jscont, const String& toEval);
    
    /** Require a file, executing its contents in the root object's scope iff it
     *  has not yet been imported.
     */
    v8::Handle<v8::Value> require(const String& filename,bool isJS);

    //JSContextStructs request the JSObjectScript to call finishClear on them
    //when doing so won't invalidate any iterators on the JSObjectScript.
    virtual void registerContextForClear(JSContextStruct* jscont);


    //all need to have jscontext structs so that can put context struct on
    //evaluation stack when call.
    v8::Handle<v8::Value> storageBeginTransaction(JSContextStruct* jscont);
    v8::Handle<v8::Value> storageCommit(JSContextStruct* jscont, v8::Handle<v8::Function> cb);
    v8::Handle<v8::Value> storageWrite(const OH::Storage::Key& key, const String& toWrite, v8::Handle<v8::Function> cb, JSContextStruct* jscont);
    v8::Handle<v8::Value> storageRead(const OH::Storage::Key& key, v8::Handle<v8::Function> cb, JSContextStruct* jscont);
    v8::Handle<v8::Value> storageErase(const OH::Storage::Key& key, v8::Handle<v8::Function> cb, JSContextStruct* jscont);

    v8::Handle<v8::Value> setRestoreScript(JSContextStruct* jscont, const String& script, v8::Handle<v8::Function> cb);

    /**
       Returns true if context eval stack is not empty and if the top context on
       the stack allows operation associated with whatCap on jspres.  Returns
       false otherwise.
     */
    bool checkCurCtxtHasCapability(JSPresenceStruct* jspres, Capabilities::Caps whatCap);

    /**
       We want to ensure that no sandboxes have while(1) loops.  Roughly, the
       way we do this is that our compiler inserts a lot of calls to
       checkResources.  If checkResources returns false, then we throw an
       uncatch-able error.

       Using a stupid implementation for checkResources.  Every time that we
       call eval, and are not currently executing code/handling an event, then
       we set mResourceCounter to zero.  Every interleaving call to
       checkResources increments mResourceCounter.  If mResourceCounter ever
       gets above a threshold value, the next checkResources call returns false,
       causing the script to throw an uncatchable error.

       @return {v8::Boolean} Returns true if still have adequate resources to
       continue computation.  Returns false otherwise.
     */
    v8::Handle<v8::Value> checkResources();
    bool checkResourcesCPP();

    Handle<v8::Context> context() { return mContext->mContext;}

    bool isRootContext(JSContextStruct* jscont);

    JSObjectScriptManager* manager() const { return mManager; }



    // A generic interface for invoking callback methods, used by other classes
    // that have JSObjectScript* (e.g. Invokable). Probably needs a version for
    // contexts if the function was bound within a context
    v8::Handle<v8::Value> invokeCallback(JSContextStruct* ctx, v8::Handle<v8::Object>* target, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[]);
    v8::Handle<v8::Value> invokeCallback(JSContextStruct* ctx, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[]);
    v8::Handle<v8::Value> invokeCallback(JSContextStruct* ctx, v8::Handle<v8::Function>& cb);


    //lkjs; note: will need to grab most recent context from stack.
    v8::Local<v8::Function> functionValue(const String& em_script_str);


    
    // Hook to invoke after a callback is invoked. Allows you to check for
    // conditions that may be set during the callback (kill requested, reset,
    // etc).
    virtual void postCallbackChecks() {}

    JSContextStruct* rootContext() const { return mContext; }

    /**
       Returns true if stop has ever been called on JSObjectScript.
     */
    bool isStopped();


    /**
       Runs the string toCompile through the emerson parser.  If fails to
       compile, then returns syntax error.
     */
    v8::Handle<v8::Value> emersonCompileString(const String& toCompile);
    

    /** Eval a string, executing its contents in the root object's scope. */
    v8::Handle<v8::Value> evalInGlobal(const String& contents, v8::ScriptOrigin* em_script_name,JSContextStruct* jscs);
 

    
protected:

    // Object host internal identifier for the object associated with
    // this script. We copy this information here because this base
    // class is used for emheadless, which can't get the identifier
    // from the HostedObjectPtr.
    UUID mInternalID;

    /**
       @see checkResources
     */
    int32 mResourceCounter;
    /**
       We want to keep track of whether we're in
       an eval loop or not.  If we are, then mNestedEvalCounter keeps track of
       how many levels of evals we're in.  Ie, if we call eval and from inside
       that eval, call eval again, then we're mNestedEvalCounter should be 2.
       If we are outside of all evals, then mNestedEvalCounter should be zero.
    */
    int32 mNestedEvalCounter;

    /**
       Each time before we call an eval function into v8, we first call
       preEvalOps.  preEvalOps sets up mResourceCounter and mNestedEvalCounter
       before calling eval/run/call.  (It increments mNestedEvalCounter and if
       this eval isn't called from inside another eval, then we set
       mResourceCounter to zero.)
     */
    void preEvalOps();

    /**
       Decrements mNestedEvalCounter.  Used for bookkeeping so that we can keep
       track of what level of eval nesting we're in.
     */
    void postEvalOps();



    // Each context has an id that is assigned from this variable.
    uint32 contIDTracker;
    std::map<uint32,JSContextStruct*> mContStructMap;


    // EvalContext tracks the current state w.r.t. eval-related statements which
    // may change in response to user actions (changing directory) or due to the
    // way the system defines actions (e.g. import searches the current script's
    // directory before trying other import paths).
    struct EvalContext {
        EvalContext(JSContextStruct* jsctx);
        EvalContext(const EvalContext& rhs);
        EvalContext(const EvalContext& rhs, JSContextStruct* jsctx);
        
        // Current directory the script being evaluated was in,
        // e.g. ../../liboh/plugins/js/scripts/std/movement
        boost::filesystem::path currentScriptDir;
        // Current base import-path directory the script was found in,
        // e.g. for the above it might look like
        // ../../liboh/plugins/js/scripts/.
        // This is used to provide nice relative paths in exceptions.
        boost::filesystem::path currentScriptBaseDir;

        // Gets the full, but relative, path for the script. In the
        // above example this would be std/movement because the
        // currentScriptBaseDir is stripped off to leave just the
        // relative part.
        boost::filesystem::path getFullRelativeScriptDir() const;

        std::ostream* currentOutputStream;

        //the associated jscontextstruct containing the evaluation context.
        JSContextStruct* jscont;
    };
    // This is a helper which adds an EvalContext to the stack and ensures that
    // when it goes out of scope it is removed. This will almost always be the
    // right way to add and remove an EvalContext from the stack, ensure
    // multiple exit paths from a method don't cause the stack to become
    // incorrect.
    struct ScopedEvalContext {
        ScopedEvalContext(JSObjectScript* _parent, const EvalContext& _ctx);
        ~ScopedEvalContext();

        JSObjectScript* parent;
    };
    friend class ScopedEvalContext;

    std::stack<EvalContext> mEvalContextStack;


    
    //indexed by which context/sandbox you're in.
    typedef     std::map<uint32,std::set<String>  > ImportedFileMap;
    typedef ImportedFileMap::iterator ImportedFileMapIter;
    ImportedFileMap mImportedFiles;


    // add an extenstion to the filename that you get for import/require
    std::string* extensionize(std::string);

    // Resolve a relative path for import to an absolute
    // path. "Returns" the full path of the file as well as the import
    // base path.
    void resolveImport(const String& filename, boost::filesystem::path* full_file_out, boost::filesystem::path* base_path_out);
    // Perform an import on the absolute path filename. This performs no
    // resolution and *always* performs the import, even if the file has already
    // been imported.
    v8::Handle<v8::Value> absoluteImport(const boost::filesystem::path& full_filename, const boost::filesystem::path& full_base_dir,bool isJS);


    JSContextStruct* mContext;



    void  printStackFrame(std::stringstream&, v8::Local<v8::StackFrame>);

    typedef std::map<uint32, SuspendableVec> ContIDToSuspMap;
    ContIDToSuspMap toFixup;

    JSObjectScriptManager* mManager;
    OH::Storage* mStorage;
    OH::PersistedObjectSet* mPersistedObjectSet;
    
    void storageCommitCallback(JSContextStruct* jscont, v8::Persistent<v8::Function> cb, bool success, OH::Storage::ReadSet* rs);

    void setRestoreScriptCallback(JSContextStruct* jscont, v8::Persistent<v8::Function> cb, bool success);

    /**
       If we execute this number of cycles in one pass of event loop, throw a
       resource error.
     */
    int32 maxResourceThresh;

    /**
       Will be true after stop method is called.
     */
    bool stopCalled;


    
  private:
    //should already be inside of a frame;
    v8::Handle<v8::Value> compileFunctionInContext( v8::Handle<v8::Function>&cb);

    // Print an exception "to" the script, i.e. using its system.print
    // method. This is useful for callbacks which are executed directly from C++
    // code but which should report errors to the user.
    void printExceptionToScript(const String& exc);

    v8::Handle<v8::Value> protectedEval(const String& em_script_str, v8::ScriptOrigin* em_script_name, const EvalContext& new_ctx, bool return_exc = false, bool isJS=false);


    // is_emerson controls whether this is compiled as emerson or
    // javascript code.
    // \param return_exc if true, return the exception generated (or
    //         empty handle for none) instead of the return value. The
    //         return value is discarded. This prevents throwing an
    //         exception (or rethrowing a caught exception), which is
    //         necessary if there is no JS caller higher on the
    //         stack. Otherwise, V8 gets stuck with an uncaught
    //         exception and fails on future V8 calls.
    v8::Handle<v8::Value> internalEval( const String& em_script_str, v8::ScriptOrigin* em_script_name, bool is_emerson, bool return_exc = false);
    

    //Takes the context from the top value of context stack and returns it.  If
    //context stack is empty, prints error, and returns context associated with mContext.
    v8::Handle<v8::Context> getCurrentV8Context();
    
};

} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_OBJECT_SCRIPT_HPP_
