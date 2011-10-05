
#include <sirikata/core/util/Platform.hpp>

#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/VWObject.hpp>
#include <sirikata/oh/ObjectHostProxyManager.hpp>
#include "PerPresenceData.hpp"
#include <sirikata/oh/ObjectHostContext.hpp>


namespace Sirikata{

    SpaceObjectReference PerPresenceData::id() const
    {
        if (! validSpaceObjRef)
        {
            std::cout<<"\n\nERROR should have set which space earlier\n\n";
            assert(false);
        }
        return SpaceObjectReference(space, object);
    }


PerPresenceData::PerPresenceData(HostedObject* _parent, const SpaceID& _space, const ObjectReference& _oref, const HostedObject::BaseDatagramLayerPtr&layer, const String& _query)
     : parent(_parent),
       space(_space),
       object(_oref),
       mUpdatedLocation(
            Duration::seconds(.1),
            TemporalValue<Location>::Time::null(),
            Location(Vector3d(0,0,0),Quaternion(Quaternion::identity()),
                     Vector3f(0,0,0),Vector3f(0,1,0),0),
            ProxyObject::UpdateNeeded()),
       proxyManager(new ObjectHostProxyManager(_space)),
       validSpaceObjRef(true),
       query(_query),
       mSSTDatagramLayers(layer),
       updateFields(LOC_FIELD_NONE),
       rerequestTimer( Network::IOTimer::create(_parent->context()->ioService) )
    {
    }


PerPresenceData::PerPresenceData(HostedObject* _parent, const SpaceID& _space, const HostedObject::BaseDatagramLayerPtr&layer, const String& _query)
     : parent(_parent),
       mUpdatedLocation(
            Duration::seconds(.1),
            TemporalValue<Location>::Time::null(),
            Location(Vector3d(0,0,0),Quaternion(Quaternion::identity()),
                     Vector3f(0,0,0),Vector3f(0,1,0),0),
            ProxyObject::UpdateNeeded()),
       proxyManager(new ObjectHostProxyManager(_space)),
       validSpaceObjRef(false),
       query(_query),
       mSSTDatagramLayers(layer),
       updateFields(LOC_FIELD_NONE),
       rerequestTimer( Network::IOTimer::create(_parent->context()->ioService) )
    {
    }

PerPresenceData::~PerPresenceData() {
    if (mSSTDatagramLayers)
        mSSTDatagramLayers->invalidate();
    // We no longer have this session, so none of the proxies are usable
    // anymore. We can't delete the ProxyManager, but we can clear it out and
    // trigger destruction events for the proxies it holds.
    proxyManager->destroy();

    rerequestTimer->cancel();
}

    void PerPresenceData::populateSpaceObjRef(const SpaceObjectReference& sporef)
    {
        validSpaceObjRef = true;
        space   = sporef.space();
        object  = sporef.object();
    }

    ObjectHostProxyManagerPtr PerPresenceData::getProxyManager()
    {
        return proxyManager;
    }

    void PerPresenceData::initializeAs(ProxyObjectPtr proxyobj) {
        object = proxyobj->getObjectReference().object();

        mProxyObject = proxyobj;
    }



}
