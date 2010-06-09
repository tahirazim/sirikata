#ifndef _BYTE_TRANSFER_SCENARIO_HPP_
#define _BYTE_TRANSFER_SCENARIO_HPP_

#include "Scenario.hpp"
namespace Sirikata {
class ScenarioFactory;
class ConnectedObjectTracker;
class Object;
class ByteTransferScenario : public Scenario {

    ObjectHostContext*mContext;
    ConnectedObjectTracker* mObjectTracker;

    unsigned int mPingID;
    bool mSameObjectHostPings;
    bool mForceSameObjectHostPings;
    uint64 mPort;
    std::tr1::function<void()> mGeneratePings;
    bool mReturned;
    Time mStartTime;
    int64 mNumTotalPings;
    Object* mSourceObject;
    UUID mDestinationObject;
    size_t mPacketSize;
    int64 mPingNumber;
    struct TransferTimeData {
        Time start;
        Time finish;
        bool received;
        explicit TransferTimeData(Time st):start(st),finish(Time::epoch()) {
            received=false;
        }
        void update(Time now) {
            if (!received) {
                finish=now;
                received=true;
            }
        }
    };
    std::vector<TransferTimeData > mOutstandingPackets;
    TimeProfiler::Stage* mPingProfiler;
    void generatePings();
    void pingReturn(const Sirikata::Protocol::Object::ObjectMessage&);
    static ByteTransferScenario*create(const String&options);
public:
    ByteTransferScenario(const String &options);
    ~ByteTransferScenario();
    virtual void initialize(ObjectHostContext*);
    void start();
    void stop();
    static void addConstructorToFactory(ScenarioFactory*);
};
}
#endif
