/*  Meru
 *  ResourceLoadTask.hpp
 *
 *  Copyright (c) 2009, Stanford University
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
#ifndef _RESOURCE_LOAD_TASK_HPP_
#define _RESOURCE_LOAD_TASK_HPP_

#include "../meruCompat/DependencyTask.hpp"
#include "GraphicsResource.hpp"
#include "ResourceDownloadTask.hpp"
#include <stdio.h>

using namespace std;

namespace Meru {

class ResourceLoadTask : public DependencyTask, public ResourceRequestor
{
public:
  ResourceLoadTask(DependencyManager *mgr, SharedResourcePtr resource, const SHA256& hash, const unsigned int epoch);
  virtual ~ResourceLoadTask();

  virtual void operator() ()
  {
    mStarted = true;
    if (!mCancelled)
      doRun();
    finish(true);
  }

  virtual void setResourceBuffer(const SparseData& buffer) {
    mBuffer = buffer;
  }

  void cancel()
  {
    mCancelled = true;
  }

  inline bool isStarted() {
    return mStarted;
  }

protected:
  virtual void doRun() = 0;

  SharedResourcePtr mResource;
  SHA256 mHash;
  SparseData mBuffer;
  const unsigned int mEpoch;
  bool mCancelled;
  bool mStarted;
};


}

#endif
