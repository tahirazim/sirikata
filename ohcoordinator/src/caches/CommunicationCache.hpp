/*  Sirikata
 *  CommunicationCache.hpp
 *
 *  Copyright (c) 2010, Behram Mistree
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

#include "Complete_Cache.hpp"
#include <boost/thread/mutex.hpp>
#include <sirikata/ohcoordinator/OSegCache.hpp>


#ifndef __COMMUNICATION_CACHE_HPP__
#define __COMMUNICATION_CACHE_HPP__

namespace Sirikata
{

  double commCacheScoreFunction(const FCacheRecord* a);
  double commCacheScoreFunctionPrint(const FCacheRecord* a,bool toPrint);


  class CommunicationCache : public OSegCache
  {
  private:
    Complete_Cache mCompleteCache;
    float mDistScaledUnits;
    float mCentralX;
    float mCentralY;
    float mCentralZ;
    CoordinateSegmentation* mCSeg;
    SpaceContext* ctx;
    boost::mutex mMutex;
    uint32 mCacheSize;

  public:
    CommunicationCache(SpaceContext* spctx, float scalingUnits, CoordinateSegmentation* cseg,uint32 cacheSize);
      virtual ~CommunicationCache() {}

    virtual void insert(const UUID& uuid, const OSegEntry& sID);
    virtual const OSegEntry& get(const UUID& uuid);
    virtual void remove(const UUID& oid);

  };
}
#endif
