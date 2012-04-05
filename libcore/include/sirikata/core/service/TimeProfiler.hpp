/*  Sirikata
 *  TimeProfiler.hpp
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

#ifndef _SIRIKATA_TIME_PROFILER_HPP_
#define _SIRIKATA_TIME_PROFILER_HPP_

#include <sirikata/core/util/Timer.hpp>

namespace Sirikata {

class Context;

/** A simple class which helps to time profiling to determine
 *  what fraction of time each component of a loop is taking.
 *  It assumes each step is performed every time.  Events with
 *  human-friendly names are added to the list and then the main
 *  loop simply indicates when each stage completes.  When complete,
 *  call the report method to get a simple analysis.
 */
class SIRIKATA_EXPORT TimeProfiler {
public:
    struct SIRIKATA_EXPORT Stage {
        ~Stage();

        void started();
        void finished();

        String name() const;
        Duration avg() const;
        Duration minimum() const;
        Duration maximum() const;
        uint64 its() const;

        void report(const String& indent) const;
    private:
        friend class TimeProfiler;

        Stage(TimeProfiler* parent, const String& name);
        void invalidate();

        TimeProfiler* mParent;
        String mName;

        Time mStartTime;

        Duration mMinimum;
        Duration mMaximum;
        Duration mSum;
        uint64 mIts;

        bool mValid;
    };

    TimeProfiler(const Context* ctx, const String& name);
    ~TimeProfiler();

    /** Add a stage for profiling, not categorized under any group. Returns a
     *  Stage which can be used to update the statistics on each iteration. Note
     *  that the caller obtains ownership of the stage and is responsible for
     *  freeing it.
     */
    Stage* addStage(const String& name);
    /** Add a stage for profiling under the specified group. Returns a
     *  Stage which can be used to update the statistics on each iteration. Note
     *  that the caller obtains ownership of the stage and is responsible for
     *  freeing it.
     */
    Stage* addStage(const String& group_name, const String& name);

    void report() const;
private:
    friend class Stage;

    void remove(Stage* stage);

    const Context* mContext;
    String mName;
    typedef std::vector<Stage*> StageList;
    typedef std::map<String, StageList> GroupMap;
    GroupMap mGroups;
    StageList mFreeStages;
}; // class TimeProfiler

} // namespace Sirikata

#endif //_SIRIKATA_TIME_PROFILER_HPP_
