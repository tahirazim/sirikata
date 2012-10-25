/*  Sirikata
 *  StorageTestBase.hpp
 *
 *  Copyright (c) 2011, Stanford University
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

#ifndef __SIRIKATA_STORAGE_TEST_BASE_HPP__
#define __SIRIKATA_STORAGE_TEST_BASE_HPP__

#include <cxxtest/TestSuite.h>
#include <sirikata/oh/Storage.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOStrand.hpp>
#include <sirikata/core/network/IOWork.hpp>
#include <sirikata/core/odp/SST.hpp>
#include <sirikata/core/ohdp/SST.hpp>

class StorageTestBase
{
protected:
    typedef Sirikata::OH::Storage::Result Result;
    typedef Sirikata::OH::Storage::ReadSet ReadSet;

    static const Sirikata::OH::Storage::Bucket _buckets[2];

    int _initialized;

    Sirikata::String _plugin;
    Sirikata::String _type;
    Sirikata::String _args;

    Sirikata::PluginManager _pmgr;
    Sirikata::OH::Storage* _storage;

    // Helpers for getting event loop setup/torn down
    Sirikata::Trace::Trace* _trace;
    Sirikata::ODPSST::ConnectionManager* _sstConnMgr;
    Sirikata::OHDPSST::ConnectionManager* _ohSSTConnMgr;
    Sirikata::Network::IOService* _ios;
    Sirikata::Network::IOStrand* _mainStrand;
    Sirikata::Network::IOWork* _work;
    Sirikata::ObjectHostContext* _ctx;

    // Processing happens in another thread, but the main test thread
    // needs to wait for the test to complete before proceeding. This
    // CV notifies the main thread as each callback finishes.
    boost::mutex _mutex;
    boost::condition_variable _cond;

public:
    StorageTestBase(Sirikata::String plugin, Sirikata::String type, Sirikata::String args)
     : _initialized(0),
       _plugin(plugin),
       _type(type),
       _args(args),
       _storage(NULL),
       _trace(NULL),
       _sstConnMgr(NULL),
       _ohSSTConnMgr(NULL),
       _mainStrand(NULL),
       _work(NULL),
       _ctx(NULL)
    {}

    void setUp() {
        if (!_initialized) {
            _initialized = 1;
            _pmgr.load(_plugin);
        }

        // Storage is tied to the main event loop, which requires quite a bit of setup
        Sirikata::ObjectHostID oh_id(1);
        _trace = new Sirikata::Trace::Trace("dummy.trace");
        _ios = new Sirikata::Network::IOService("StorageTest");
        _mainStrand = _ios->createStrand("StorageTest");
        _work = new Sirikata::Network::IOWork(*_ios, "StorageTest");
        Sirikata::Time start_time = Sirikata::Timer::now(); // Just for stats in ObjectHostContext.
        Sirikata::Duration duration = Sirikata::Duration::zero(); // Indicates to run forever.
        _sstConnMgr = new Sirikata::ODPSST::ConnectionManager();
        _ohSSTConnMgr = new Sirikata::OHDPSST::ConnectionManager();

        _ctx = new Sirikata::ObjectHostContext("test", oh_id, _sstConnMgr, _ohSSTConnMgr, _ios, _mainStrand, _trace, start_time, duration);

        _storage = Sirikata::OH::StorageFactory::getSingleton().getConstructor(_type)(_ctx, _args);

        for(int i = 0; i < 2; i++)
            _storage->leaseBucket(_buckets[i]);

        _ctx->add(_ctx);
        _ctx->add(_storage);

        // Run the context, but we need to make sure it only lives in other
        // threads, otherwise we'd block up this one.  We include 4 threads to
        // exercise support for multiple threads.
        _ctx->run(4, Sirikata::Context::AllNew);
    }

    void tearDown() {
        for(int i = 0; i < 2; i++)
            _storage->releaseBucket(_buckets[i]);

        delete _work;
        _work = NULL;

        _ctx->shutdown();

        _trace->prepareShutdown();

        delete _storage;
        _storage = NULL;

        delete _ctx;
        _ctx = NULL;

        _trace->shutdown();
        delete _trace;
        _trace = NULL;

        delete _sstConnMgr;
        _sstConnMgr = NULL;

        delete _ohSSTConnMgr;
        _ohSSTConnMgr = NULL;

        delete _mainStrand;
        _mainStrand = NULL;
        delete _ios;
        _ios = NULL;
    }

    void checkReadValuesImpl(Result expected_result, ReadSet expected, Result result, ReadSet* rs) {
        TS_ASSERT_EQUALS(expected_result, result);
        if ((result != Sirikata::OH::Storage::SUCCESS) || (expected_result != Sirikata::OH::Storage::SUCCESS)) return;

        if (!rs) {
            TS_ASSERT_EQUALS(expected.size(), 0);
            return;
        }

        TS_ASSERT_EQUALS(expected.size(), rs->size());
        for(ReadSet::iterator it = expected.begin(); it != expected.end(); it++) {
            Sirikata::String key = it->first; Sirikata::String value = it->second;
            TS_ASSERT(rs->find(key) != rs->end());
            TS_ASSERT_EQUALS((*rs)[key], value);
        }
    }

    void checkReadValues(Result expected_result, ReadSet expected, Result result, ReadSet* rs) {
        boost::unique_lock<boost::mutex> lock(_mutex);
        checkReadValuesImpl(expected_result, expected, result, rs);
        delete rs;
        _cond.notify_one();
    }

    void checkReadCountValueImpl(Result expected_result, Sirikata::int32 expected_count, Result result, Sirikata::int32 count) {
        TS_ASSERT_EQUALS(expected_result, result);
        if ((result != Sirikata::OH::Storage::SUCCESS) || (expected_result != Sirikata::OH::Storage::SUCCESS)) return;

        TS_ASSERT_EQUALS(expected_count, count);
        if (!count || !expected_count) return;
    }

    void checkCountValue(Result expected_result, Sirikata::int32 expected_count, Result result, Sirikata::int32 count) {
        boost::unique_lock<boost::mutex> lock(_mutex);
        checkReadCountValueImpl(expected_result, expected_count, result, count);
        _cond.notify_one();
    }

    void waitForTransaction() {
        boost::unique_lock<boost::mutex> lock(_mutex);
        _cond.wait(lock);
    }

    void testSetupTeardown() {
        TS_ASSERT(_storage);
    }

    void testSingleWrite() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->write(_buckets[0], "a", "abcde",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testSingleRead() {
        // NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        ReadSet rs;
        rs["a"] = "abcde";
        _storage->read(_buckets[0], "a",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, rs, _1, _2)
        );
        waitForTransaction();
    }

    void testSingleInvalidRead() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->read(_buckets[0], "key_does_not_exist",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testSingleCompare() {
        // NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->compare(_buckets[0], "a", "abcde",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testSingleInvalidCompare() {
        // NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->compare(_buckets[0], "a", "wrong_key_value",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testSingleErase() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->erase(_buckets[0], "a",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, ReadSet(), _1, _2)
        );
        waitForTransaction();

        // After erase, a read should fail
        _storage->read(_buckets[0], "a",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testMultiWrite() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "a", "abcde");
        _storage->write(_buckets[0], "f", "fghij");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testMultiRead() {
        // NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        ReadSet rs;
        rs["a"] = "abcde";
        rs["f"] = "fghij";
        _storage->beginTransaction(_buckets[0]);
        _storage->read(_buckets[0], "a");
        _storage->read(_buckets[0], "f");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, rs, _1, _2)
        );
        waitForTransaction();
    }

    void testMultiInvalidRead() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->beginTransaction(_buckets[0]);
        _storage->read(_buckets[0], "key_does_not_exist");
        _storage->read(_buckets[0], "another_key_does_not_exist");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testMultiSomeInvalidRead() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->beginTransaction(_buckets[0]);
        _storage->read(_buckets[0], "a");
        _storage->read(_buckets[0], "another_key_does_not_exist");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }

    void testMultiErase() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->beginTransaction(_buckets[0]);
        _storage->erase(_buckets[0], "a");
        _storage->erase(_buckets[0], "f");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, ReadSet(), _1, _2)
        );
        waitForTransaction();

        _storage->read(_buckets[0], "a",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, ReadSet(), _1, _2)
        );
        waitForTransaction();

        _storage->read(_buckets[0], "f",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, ReadSet(), _1, _2)
        );
        waitForTransaction();

    }

    void testAtomicWrite() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "a", "abcde");
        _storage->write(_buckets[0], "f", "fghij");
        _storage->read(_buckets[0], "x");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, ReadSet(), _1, _2)
        );
        waitForTransaction();

        ReadSet rs;
        rs["a"] = "abcde";
        rs["f"] = "fghij";

        _storage->beginTransaction(_buckets[0]);
        _storage->read(_buckets[0], "a");
        _storage->read(_buckets[0], "f");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, rs, _1, _2)
        );
        waitForTransaction();

        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "a", "abcde");
        _storage->write(_buckets[0], "f", "fghij");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, ReadSet(), _1, _2)
        );
        waitForTransaction();

        _storage->beginTransaction(_buckets[0]);
        _storage->read(_buckets[0], "a");
        _storage->read(_buckets[0], "f");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, rs, _1, _2)
        );
        waitForTransaction();
    }

    void testAtomicWriteErase() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        ReadSet rs1;
        rs1["a"] = "abcde";

        ReadSet rs2;
        rs2["k"] = "klmno";

        // To erase and ensure avoiding an error, first write to make
        // sure the value is there, then erase. Otherwise, erasing
        // could fail on the first run (empty db) and succeed
        // otherwise.
        _storage->write(_buckets[0], "k", "x",
             std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, ReadSet(), _1, _2)
         );
         waitForTransaction();
        _storage->erase(_buckets[0], "k",
             std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, ReadSet(), _1, _2)
         );
         waitForTransaction();

        _storage->beginTransaction(_buckets[0]);
        _storage->erase(_buckets[0], "a");
        _storage->erase(_buckets[0], "f");
        _storage->read(_buckets[0], "x");
        _storage->write(_buckets[0], "k", "klmno");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, ReadSet(), _1, _2)
        );
        waitForTransaction();

        _storage->read(_buckets[0], "a",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, rs1, _1, _2)
        );
        waitForTransaction();

        _storage->read(_buckets[0], "k",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, rs2, _1, _2)
        );
        waitForTransaction();

        _storage->beginTransaction(_buckets[0]);
        _storage->erase(_buckets[0], "a");
        _storage->erase(_buckets[0], "f");
        _storage->write(_buckets[0], "k", "klmno");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, ReadSet(), _1, _2)
        );
        waitForTransaction();

        _storage->read(_buckets[0], "a",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, rs1, _1, _2)
        );
        waitForTransaction();

        _storage->read(_buckets[0], "k",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, rs2, _1, _2)
        );
        waitForTransaction();
    }

    void testRangeRead() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "map:name:a", "abcde");
        _storage->write(_buckets[0], "map:name:f", "fghij");
        _storage->write(_buckets[0], "map:name:k", "klmno");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, ReadSet(), _1, _2)
        );

        ReadSet rs;
        rs["map:name:a"] = "abcde";
        rs["map:name:f"] = "fghij";
        rs["map:name:k"] = "klmno";

        _storage->rangeRead(_buckets[0],"map:name", "map:name@",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, rs, _1, _2)
        );
        waitForTransaction();
    }

    void testCount() {
    	// NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        Sirikata::int32 count = 3;
    	_storage->count(_buckets[0],"map:name", "map:name@",
    		std::tr1::bind(&StorageTestBase::checkCountValue, this, Sirikata::OH::Storage::SUCCESS, count, _1, _2)
    	);

    	waitForTransaction();
    }

    void testRangeErase() {
    	// NOTE: Depends on above write
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

    	_storage->rangeErase(_buckets[0],"map:name", "map:name@",
    		std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, ReadSet(), _1, _2)
    	);
    	waitForTransaction();

        _storage->rangeRead(_buckets[0],"map:name", "map:name@",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }


    void testAllTransaction() {
        // Test that all operations work together in a transaction. Importantly,
        // this ensures that the expected read operations come back when you
        // group different types of operations.
        //
        // This was added because some operations weren't following the
        // transactional semantics properly, so it tries to test that operations
        // aren't performed independently of the rest of the transaction.

        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        // Setup some data
        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "foo", "bar");
        _storage->write(_buckets[0], "baz", "baz");
        _storage->write(_buckets[0], "map:all:a", "abcde");
        _storage->write(_buckets[0], "map:all:f", "fghij");
        _storage->write(_buckets[0], "map:all:k", "klmno");
        _storage->write(_buckets[0], "map:todelete:a", "xyzw");
        _storage->write(_buckets[0], "map:todelete:c", "xyzw");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, ReadSet(), _1, _2)
        );
        waitForTransaction();

        ReadSet rs;
        // These are values we'll read via rangeRead
        rs["map:all:a"] = "abcde";
        rs["map:all:f"] = "fghij";
        rs["map:all:k"] = "klmno";
        // And an individual key we'll read back.
        rs["foo"] = "bar";

        // We don't care about order here, just that if reads occur before the
        // commitTransaction, they won't properly be included in the results.
        _storage->beginTransaction(_buckets[0]);
        // Read existing data
        _storage->read(_buckets[0], "foo");
        // Range read existing data
        _storage->rangeRead(_buckets[0], "map:all", "map:all@");
        // Write some new data, verified below
        _storage->write(_buckets[0], "y", "z");
        // Delete individual key
        _storage->erase(_buckets[0], "baz");
        // Delete a range of data
        _storage->rangeErase(_buckets[0], "map:todelete", "map:todelete@");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, rs, _1, _2)
        );
        waitForTransaction();

        // Verify data written in above transaction
        ReadSet rs2;
        rs2["y"] = "z";
        _storage->read(_buckets[0], "y",
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, rs2, _1, _2)
        );
        waitForTransaction();
    }


    void resetRollbackData() {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        // Setup some data
        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "foo", "bar");
        _storage->write(_buckets[0], "baz", "baz");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, ReadSet(), _1, _2)
        );
        waitForTransaction();
    }
    void verifyRollbackData(Sirikata::String key, Sirikata::String value) {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        ReadSet rs;
        rs[key] = value;
        _storage->beginTransaction(_buckets[0]);
        _storage->read(_buckets[0], key);
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::SUCCESS, rs, _1, _2)
        );
        waitForTransaction();
    }
    void testRollback() {
        // This test can't be performed completely because an
        // implementation can collect mutations (writes, erases) into
        // a single operation, and since none of these should fail for
        // any reason other than underlying storage limitations
        // (e.g. limited value sizes) there wouldn't be a need for
        // rollback.
        //
        // Instead, we just do the best we can, trying to test for
        // invalid operations even if they could be reordered.

        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        // Valid write, invalid read
        resetRollbackData();
        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "foo", "xxx");
        _storage->read(_buckets[0], "key_that_does_not_exist");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, ReadSet(), _1, _2)
        );
        waitForTransaction();
        verifyRollbackData("foo", "bar");

        // Valid write, invalid compare
        resetRollbackData();
        _storage->beginTransaction(_buckets[0]);
        _storage->write(_buckets[0], "baz", "xxx");
        _storage->compare(_buckets[0], "foo", "not_bar");
        _storage->commitTransaction(_buckets[0],
            std::tr1::bind(&StorageTestBase::checkReadValues, this, Sirikata::OH::Storage::TRANSACTION_ERROR, ReadSet(), _1, _2)
        );
        waitForTransaction();
        verifyRollbackData("baz", "baz");
    }

};

const Sirikata::OH::Storage::Bucket StorageTestBase::_buckets[2] = {
    Sirikata::OH::Storage::Bucket("72a537a6-c18f-48fe-a97d-90b40727062e", Sirikata::OH::Storage::Bucket::HumanReadable()),
    Sirikata::OH::Storage::Bucket("12345678-9101-1121-3141-516171819202", Sirikata::OH::Storage::Bucket::HumanReadable())
};

#endif //__SIRIKATA_STORAGE_TEST_BASE_HPP__
