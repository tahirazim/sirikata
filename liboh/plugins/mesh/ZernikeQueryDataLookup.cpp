// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ZernikeQueryDataLookup.hpp"

namespace Sirikata {

void ZernikeQueryDataLookup::lookup(HostedObjectPtr ho, HostedObject::OHConnectInfoPtr ci) {
    if (ci->mesh.find("meerkat:") == 0)
        downloadZernikeDescriptor(ho, ci);
    else
        ho->objectHostConnectIndirect(ho, this, ci);
}




void ZernikeQueryDataLookup::downloadZernikeDescriptor(HostedObjectPtr ho, HostedObject::OHConnectInfoPtr ocip, uint8 n_retry)
{
    Transfer::TransferRequestPtr req(
        new Transfer::MetadataRequest(
            Transfer::URI(ocip->mesh), 1.0,
            std::tr1::bind(
                &ZernikeQueryDataLookup::metadataDownloaded, this, ho, ocip, n_retry,
                std::tr1::placeholders::_1, std::tr1::placeholders::_2)
        )
    );
    ho->getObjectHost()->getTransferPool()->addRequest(req);
}

void ZernikeQueryDataLookup::metadataDownloaded(HostedObjectPtr ho, HostedObject::OHConnectInfoPtr ocip,
    uint8 retryCount,
    Transfer::MetadataRequestPtr request,
    Transfer::RemoteFileMetadataPtr response)
{
    // Provide default if we repeatedly fail to look it up
    if (response == NULL && retryCount >= 3) {
        String zernike="[";
        ocip->query_data = zernike;
        for (uint32 i = 0; i < 121; i++) {
            if (i < 120)
                zernike += "0, ";
            else
                zernike += "0]";
        }
        ho->context()->mainStrand->post(std::tr1::bind(&HostedObject::objectHostConnectIndirect, ho.get(), ho, this, ocip));
        return;
    }

    if (response != NULL) {
        const Sirikata::Transfer::FileHeaders& headers = response->getHeaders();
        String zernike = "";
        if (headers.find("Zernike") != headers.end()) {
            zernike = (headers.find("Zernike"))->second;
        }
        ocip->query_data = zernike;
        ho->context()->mainStrand->post(std::tr1::bind(&HostedObject::objectHostConnectIndirect, ho.get(), ho, this, ocip));
    }
    else if (retryCount < 3) {
        downloadZernikeDescriptor(ho, ocip, retryCount+1);
    }
}

} // namespace Sirikata
