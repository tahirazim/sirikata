/*  Sirikata
 *  Test.cpp
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

#include <cxxtest/TestRunner.h>
#include <cxxtest/TestListener.h>
#include <stdio.h>
#include <string.h>

#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/trace/Trace.hpp>

namespace Sirikata {

using namespace CxxTest;

class StandardTestListener : public TestListener {
public:
    StandardTestListener(int argc, const char** argv)
     : m_argc(argc), m_argv(argv)
    {
    }
    virtual ~StandardTestListener() {
    }

    int run(const std::string& singleSuite) {

    	bool foundTest = false;

    	if (!singleSuite.empty()) {
            std::string suite_name, test_name;
            size_t dot_pos = singleSuite.find('.');
            if (dot_pos == std::string::npos) {
                suite_name = singleSuite;
                test_name = "";
            }
            else {
                suite_name = singleSuite.substr(0, dot_pos);
                test_name = singleSuite.substr(dot_pos+1);
            }

            TestRunner::setListener(this);

            RealWorldDescription wd;

            tracker().enterWorld( wd );
            if ( wd.setUp() ) {
                for ( SuiteDescription *sd = wd.firstSuite(); sd; sd = sd->next() )
                    if ( sd->active() && suite_name == sd->suiteName() ) {

                        if (test_name.empty()) {
                            foundTest = true;
                            printf("\n\n========================================\n");
                            printf("RUNNING SINGLE SUITE\n%s\n", singleSuite.c_str());
                            printf("========================================");
                        }

                        tracker().enterSuite( *sd );
                        if ( sd->setUp() ) {
                            for ( TestDescription *td = sd->firstTest(); td; td = td->next() )
                                if ( td->active() && (test_name.empty() || test_name == td->testName())) {

                                    if (!test_name.empty()) {
                                        foundTest = true;
                                        printf("\n\n========================================\n");
                                        printf("RUNNING SINGLE TEST\n%s\n", singleSuite.c_str());
                                        printf("========================================");
                                    }

                                    tracker().enterTest( *td );

                                    // Make sure options are reparsed for each
                                    // test suite, in case the test suites
                                    // themselves (e.g. a test of options) mucks
                                    // with them
                                    InitOptions();
                                    Trace::Trace::InitOptions();
                                    ParseOptions(m_argc, const_cast<char**>(m_argv));

                                    if ( td->setUp() ) {
                                        td->run();
                                        td->tearDown();
                                    }
                                    tracker().leaveTest( *td );

                                }

                            sd->tearDown();
                        }
                        tracker().leaveSuite( *sd );

                    }
                wd.tearDown();
            }
            tracker().leaveWorld( wd );

            TestRunner::setListener(NULL);

            if (!foundTest) {
                printf("Test suite %s not found, aborting\n", singleSuite.c_str());
            }
    	}
    	if (singleSuite.empty()) {
            printf("\n\n========================================\n");
            printf("RUNNING ALL SUITES\n");
            printf("========================================\n\n");

            TestRunner::runAllTests( *this );
    	}

        printf("\n\n========================================\n");
        printf("SUMMARY: %d failed, %d warnings.\n",
            tracker().failedTests(), tracker().warnings());
        std::map<std::string, std::string> reported_failed_tests;
        for(uint32 i = 0; i < mFailedTests.size(); i++) {
            if (reported_failed_tests.find(mFailedTests[i]) != reported_failed_tests.end()) continue;
            printf(" - %s\n", mFailedTests[i].c_str());
            reported_failed_tests[mFailedTests[i]] = mFailedTests[i];
        }
        printf("========================================\n");
        return tracker().failedTests();
    }

    virtual void process_commandline(int& /*argc*/, char** /*argv*/) {}

    virtual void enterWorld( const WorldDescription & /*desc*/ ) {}
    virtual void enterSuite( const SuiteDescription & desc ) {
    }
    virtual void enterTest( const TestDescription & desc ) {
    	printf("\n\n========================================\n");
        printf("BEGIN TEST %s.%s\n", desc.suiteName(), desc.testName());
        mCurrentTest = std::string(desc.suiteName()) + "." + std::string(desc.testName());
    }
    virtual void trace( const char * file, int line, const char * expression ) {
        printf("Trace: At %s:%d: %s\n", file, line, expression);
    }
    virtual void warning( const char * file, int line, const char * expression ) {
        printf("Warning: At %s:%d: %s\n", file, line, expression);
    }
    virtual void failedTest( const char * file, int line, const char * expression ) {
        printf("Error: At %s:%d: %s\n", file, line, expression);
        mFailedTests.push_back(mCurrentTest);
    }
    virtual void failedAssert( const char * file, int line, const char * expression ) {
        printf("Error: Assert failed at %s:%d: %s\n", file, line, expression);
        mFailedTests.push_back(mCurrentTest);
    }
    virtual void failedAssertEquals( const char * file, int line, const char * xStr,
        const char * yStr, const char * x, const char * y ) {
        printf("Error: Assert failed at %s:%d: (%s == %s), was (%s == %s)\n", file, line, xStr, yStr, x, y);
        mFailedTests.push_back(mCurrentTest);
    }
    virtual void failedAssertSameData( const char * file, int line, const char * xStr,
        const char * yStr, const char * sizeStr, const void * x, const void * y, unsigned size) {
        printf("Error: Assert failed at %s:%d: (%s same as %s, size = %s)\n", file, line, xStr, yStr, sizeStr);
        mFailedTests.push_back(mCurrentTest);
    }
    virtual void failedAssertDelta( const char * file, int line, const char * xStr,
        const char * yStr, const char * dStr, const char * x, const char * y, const char * d) {
        printf("Error: Assert failed at %s:%d: (%s == %s, delta = %s), was (%s == %s, delta = %s)\n", file, line, xStr, yStr, dStr, x, y, d);
        mFailedTests.push_back(mCurrentTest);
    }
    virtual void failedAssertDiffers( const char * file, int line, const char * xStr,
        const char * yStr, const char * value) {
        printf("Error: Assert failed at %s:%d: (%s != %s), value: %s\n", file, line, xStr, yStr, value);
        mFailedTests.push_back(mCurrentTest);
    }
    virtual void failedAssertLessThan( const char * /*file*/, int /*line*/,
        const char * /*xStr*/, const char * /*yStr*/,
        const char * /*x*/, const char * /*y*/ ) {}
    virtual void failedAssertLessThanEquals( const char * /*file*/, int /*line*/,
        const char * /*xStr*/, const char * /*yStr*/,
        const char * /*x*/, const char * /*y*/ ) {}
    virtual void failedAssertPredicate( const char * /*file*/, int /*line*/,
        const char * /*predicate*/, const char * /*xStr*/, const char * /*x*/ ) {}
    virtual void failedAssertRelation( const char * /*file*/, int /*line*/,
        const char * /*relation*/, const char * /*xStr*/, const char * /*yStr*/,
        const char * /*x*/, const char * /*y*/ ) {}
    virtual void failedAssertThrows( const char * /*file*/, int /*line*/,
        const char * /*expression*/, const char * /*type*/,
        bool /*otherThrown*/ ) {}
    virtual void failedAssertThrowsNot( const char * /*file*/, int /*line*/,
        const char * /*expression*/ ) {}
    virtual void failedAssertSameFiles( const char* , unsigned , const char* , const char*, const char* ) {}
    virtual void leaveTest( const TestDescription & desc ) {
    	printf("END TEST %s.%s\n", desc.suiteName(), desc.testName());
    	printf("========================================\n");
        mCurrentTest = "";
    }
    virtual void leaveSuite( const SuiteDescription & /*desc*/ ) {}
    virtual void leaveWorld( const WorldDescription & /*desc*/ ) {}

private:
    int m_argc;
    const char** m_argv;
    std::string mCurrentTest;
    // Track list of failed tests for summary output
    std::vector<std::string> mFailedTests;
};

} // namespace Sirikata


int main(int argc, const char * argv [])
{
    if(argc > 1) {
        return Sirikata::StandardTestListener(argc, argv).run(std::string(argv[1]));
    } else {
        return Sirikata::StandardTestListener(argc, argv).run(std::string());
    }
}
