/*  Sirikata
 *  CassandraStorageTest.hpp
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

#include <cxxtest/TestSuite.h>
#include "StorageTestBase.hpp"
#include <sirikata/cassandra/Cassandra.hpp>

class CassandraStorageTest : public CxxTest::TestSuite
{
    static const String dbhost;
    static const String dbport;
    StorageTestBase _base;
public:
    CassandraStorageTest()
     : _base("oh-cassandra", "cassandra", String("--host=") + dbhost + String(" --port=") + dbport)
    {
    }

    void setUp() {_base.setUp(); }
    void tearDown() {_base.tearDown(); }

    void testSetupTeardown() {_base.testSetupTeardown(); }

    void testSingleWrite() {_base.testSingleWrite(); }
    void testSingleRead() {_base.testSingleRead(); }
    void testSingleInvalidRead() {_base.testSingleInvalidRead(); }
    void testSingleCompare() {_base.testSingleCompare(); }
    void testSingleInvalidCompare() {_base.testSingleInvalidCompare(); }
    void testSingleErase() {_base.testSingleErase(); }

    void testMultiWrite() {_base.testMultiWrite(); }
    void testMultiRead() {_base.testMultiRead(); }
    void testMultiInvalidRead() {_base.testMultiInvalidRead(); }
    void testMultiSomeInvalidRead() {_base.testMultiSomeInvalidRead(); }
    void testMultiErase() {_base.testMultiErase(); }

    void testAtomicWrite() {_base.testAtomicWrite(); }
    void testAtomicWriteErase() {_base.testAtomicWriteErase(); }

    void testRangeRead() {_base.testRangeRead(); }
    void testCount() {_base.testCount(); }
    void testRangeErase() {_base.testRangeErase(); }

    void testAllTransaction() {_base.testAllTransaction(); }
};

const String CassandraStorageTest::dbhost("localhost");
const String CassandraStorageTest::dbport("9160");
