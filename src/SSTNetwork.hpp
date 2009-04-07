#ifndef _CBR_SST_NETWORK_HPP_
#define _CBR_SST_NETWORK_HPP_

#include "Network.hpp"

namespace CBR {

class CBRSST;

class SSTNetwork : public Network {
public:
    SSTNetwork();
    virtual ~SSTNetwork();
    virtual bool send(const Address4& addy, const Network::Chunk& data, bool reliable, bool ordered, int priority);
    virtual void listen (const Address4&);
    virtual Chunk* front(const Address4& from, uint32 max_size);
    virtual Network::Chunk* receiveOne(const Address4& from, uint32 max_size);
    virtual void service(const Time& t);
    virtual void init(void* (*)(void*));
    virtual void start();
private:

    CBRSST* mImpl;
};

} // namespace CBR

#endif //_CBR_SST_NETWORK_HPP_
