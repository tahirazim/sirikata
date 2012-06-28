/*  Sirikata Utilities -- Sirikata Synchronization Utilities
 *  UUID.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/util/UUID.hpp>
#include "boost_uuid.hpp"
#include <boost/functional/hash.hpp>
#include <boost/asio.hpp> // htonl, ntonl

BOOST_STATIC_ASSERT(Sirikata::UUID::static_size==sizeof(boost_::uuid));

namespace Sirikata {

static unsigned int fromHex(char a) {
    if (a >= 0 && a <='9')
        return a - '0';
    if (a >= 'a' && a <= 'f')
        return 10 + (unsigned int)(a-'a');
    if (a >= 'A' && a <= 'F')
        return 10 + (unsigned int)(a-'A');
    return 0;
}

UUID::UUID(const std::string & other,HumanReadable ) {
    try {
        boost_::uuid parsed_string(other);
        mData.initialize(parsed_string.begin(),parsed_string.end());
    }
    catch(std::invalid_argument exc) {
        mData.initialize(UUID::null().mData.begin(), UUID::null().mData.end());
    }
}
UUID::UUID(const std::string & other, HexString) {
    assert(other.size() == 2*static_size);
    for(unsigned int i=0;i<static_size;++i)
        mData[i] = fromHex(other[2*i])*16 + fromHex(other[2*i+1]);
    assert(rawHexData() == other);
}
UUID::UUID(const boost_::uuid&other){
    mData.initialize(other.begin(),other.end());
}
UUID::UUID(UUID::GenerateRandom) {
    boost_::uuid randval = boost_::uuid::create();
    mData.initialize(randval.begin(),randval.end());
}

UUID::UUID(const uint32 v) {
    uint32 vnet = htonl(v);
    unsigned int i;
    for(i = 0; i < sizeof(v); i++)
        mData[i] = ((const char*)&vnet)[i];
    for(; i< static_size; i++)
        mData[i] = 0;
}

UUID UUID::random() {
    return UUID(UUID::GenerateRandom());
}
std::string UUID::readableHexData()const{
    std::ostringstream oss;
    oss<<boost_::uuid(getArray().begin(),getArray().end());
    return oss.str();
}
static char toHex(unsigned int a) {
    if (a<10)
        return a+'0';
    if (a<16)
        return (a-10)+'a';
    return 'x';
}
std::string UUID::rawData()const {
    return std::string ((const char*)mData.begin(),static_size);
}
std::string UUID::rawHexData()const{
    std::string retval;
    retval.resize(2*static_size);
    for(unsigned int i=0;i<static_size;++i) {
        retval[i*2]=toHex(mData[i]/16);
        retval[i*2+1]=toHex(mData[i]%16);
    }
    return retval;
}
size_t UUID::hash() const {
    uint64* dat = (uint64*)getArray().data();
    size_t seed = 0;
    boost::hash_combine(seed, dat[0]);
    boost::hash_combine(seed, dat[1]);
    return seed;
}

uint32 UUID::asUInt32() const {
    return ntohl(*((int32*)mData.data()));
}

std::ostream& operator << (std::ostream &os, const Sirikata::UUID& output) {
    os << boost_::uuid(output.getArray().begin(),output.getArray().end());
    return os;
}

std::istream& operator>>(std::istream & is, Sirikata::UUID & uuid) {
    boost_::uuid internal;
    is >> internal;
    uuid = Sirikata::UUID(internal);
    return is;
}

} // namespace Sirikata
