/*  Sirikata
 *  Liveness.cpp
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

#include <sirikata/core/util/Liveness.hpp>

namespace Sirikata {

Liveness::Token::Token(InternalToken t)
 : mData(t)
{}

Liveness::Lock::Lock(InternalToken t)
 : mData(t)
{}

Liveness::Lock::Lock(const Token& t)
 : mData(t.mData)
{}

Liveness::Liveness()
 : mLivenessStrongToken(new int)
{}

Liveness::~Liveness() {
    assert(!mLivenessStrongToken && "You need to call letDie in the subclass of Liveness");
}

void Liveness::letDie() {
    // The basic strategy is to save a weak pointer to the token, clear the
    // strong pointer, and then block until we can't lock the weak pointer
    // anymore, i.e. when all strong pointers have disappeared, meaning no other
    // thread could see the object as alive anymore.
    assert(mLivenessStrongToken);

    InternalToken weak_self = mLivenessStrongToken;
    mLivenessStrongToken.reset();
    // busy wait
    while(weak_self.lock()) {}
}

}
