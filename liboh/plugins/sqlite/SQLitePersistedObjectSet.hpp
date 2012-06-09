/*  Sirikata
 *  PersistedObjectSet.hpp
 *
 *  Copyright (c) 2010, Stanford University
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

#ifndef __SIRIKATA_OH_SQLITE_PERSISTED_OBJECT_SET_HPP__
#define __SIRIKATA_OH_SQLITE_PERSISTED_OBJECT_SET_HPP__

#include <sirikata/oh/PersistedObjectSet.hpp>
#include <sirikata/sqlite/SQLite.hpp>

namespace Sirikata {
namespace OH {

/** PersistedObjectSet tracks the set of objects which want to be restored if
 *  the object host fails and is restored.  It is the counterpart to an
 *  ObjectFactory which restores objects from object storage. It has a very
 *  simple interface because it assumes the object scripts (manually or via the
 *  ObjectScript implementation) tracks almost all data itself: each object can
 *  register to be restored and only the unique internal identifier (allowing
 *  access to storage), script type, and script (allowing the object to
 *  reconstruct itself) are stored.
 */
class SQLitePersistedObjectSet : public PersistedObjectSet {
public:
    /** RequestCallbacks are invoked when a request to change a persisted
     *  objects properties (or remove it from persistence) completes.
     */
    typedef std::tr1::function<void(bool success)> RequestCallback;

    SQLitePersistedObjectSet(ObjectHostContext* ctx, const String& dbpath);
    virtual ~SQLitePersistedObjectSet();

    /* Service Interface */
    virtual void start();
    virtual void stop();

    virtual void requestPersistedObject(const UUID& internal_id, const String& script_type, const String& script_args, const String& script_contents, RequestCallback cb, const String& timestamp="current");

private:
    // Helper that checks and logs errors, then returns bool indicating
    // success/failure
    bool checkSQLiteError(int rc, const String& msg) const;

    void initDB();
    void performUpdateWithRetry(const UUID& internal_id, const String& script_type, const String& script_args, const String& script_contents, RequestCallback cb);
    enum UpdateResult {
        SUCCESS,
        LOCK_ERROR,
        TRANSACTION_ERROR
    };
    UpdateResult performUpdate(const UUID& internal_id, const String& script_type, const String& script_args, const String& script_contents, RequestCallback cb);

    ObjectHostContext* mContext;
    String mDBFilename;
    SQLiteDBPtr mDB;

    // FIXME because we don't have proper multithreaded support in cppoh, we
    // need to allocate our own thread dedicated to IO
    Network::IOService* mIOService;
    Network::IOWork* mWork;
    Thread* mThread;
};

} //end namespace OH
} //end namespace Sirikata

#endif //__SIRIKATA_OH_SQLITE_PERSISTED_OBJECT_SET_HPP__
