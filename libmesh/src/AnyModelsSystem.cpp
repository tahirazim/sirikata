/*  Sirikata
 *  AnyModelsSystem.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#include <sirikata/mesh/AnyModelsSystem.hpp>
#include <sirikata/mesh/ModelsSystemFactory.hpp>

namespace Sirikata {

String AnyModelsSystem::sAnyName("any");

AnyModelsSystem::AnyModelsSystem() {
    ModelsSystemFactory::ConstructorNameList cons = ModelsSystemFactory::getSingleton().getNames();
    for(ModelsSystemFactory::ConstructorNameList::iterator it = cons.begin(); it != cons.end(); it++) {
        String& name = *it;
        if (name == sAnyName) continue;
        String no_args("");
        ModelsSystem* ms = ModelsSystemFactory::getSingleton().getConstructor(name)(no_args);
        mModelsSystems[name] = ms;
    }
}

AnyModelsSystem::~AnyModelsSystem () {
    for(SystemsMap::iterator it = mModelsSystems.begin(); it != mModelsSystems.end(); it++) {
        ModelsSystem* ms = it->second;
        delete ms;
    }
    mModelsSystems.clear();
}

ModelsSystem* AnyModelsSystem::create(const String& args) {
    return new AnyModelsSystem();
}

bool AnyModelsSystem::canLoad(Transfer::DenseDataPtr data) {
    for(SystemsMap::iterator it = mModelsSystems.begin(); it != mModelsSystems.end(); it++) {
        ModelsSystem* ms = it->second;
        if (ms->canLoad(data)) return true;
    }
    return false;
}

Mesh::VisualPtr AnyModelsSystem::load(const Transfer::RemoteFileMetadata& metadata, const Transfer::Fingerprint& fp,
    Transfer::DenseDataPtr data) {
    Mesh::VisualPtr result;
    for(SystemsMap::iterator it = mModelsSystems.begin(); it != mModelsSystems.end(); it++) {
        ModelsSystem* ms = it->second;
        if (ms->canLoad(data)) {
            result = ms->load(metadata, fp, data);
            if (result) return result;
        }
    }
    SILOG(AnyModelsSystem,error,"Couldn't find parser for " << metadata.getURI());
    return result;
}

Mesh::VisualPtr AnyModelsSystem::load(Transfer::DenseDataPtr data) {
    Mesh::VisualPtr result;
    for(SystemsMap::iterator it = mModelsSystems.begin(); it != mModelsSystems.end(); it++) {
        ModelsSystem* ms = it->second;
        if (ms->canLoad(data)) {
            result = ms->load(data);
            if (result) return result;
        }
    }
    SILOG(AnyModelsSystem,error,"Couldn't find parser for model data without a URI");
    return result;

}


bool AnyModelsSystem::convertVisual(const Mesh::VisualPtr& visual, const String& format, std::ostream& vout) {
    SystemsMap::iterator it = mModelsSystems.find(format);
    if (it == mModelsSystems.end()) {
        SILOG(AnyModelsSystem,error,"AnyModelsSystem couldn't find format " << format << " during mesh conversion.");
        return false;
    }
    ModelsSystem* ms = it->second;
    return ms->convertVisual(visual, "", vout);
}

bool AnyModelsSystem::convertVisual(const Mesh::VisualPtr& visual, const String& format, const String& filename) {
    SystemsMap::iterator it = mModelsSystems.find(format);
    if (it == mModelsSystems.end()) {
        SILOG(AnyModelsSystem,error,"AnyModelsSystem couldn't find format " << format << " during mesh conversion.");
        return false;
    }
    ModelsSystem* ms = it->second;
    return ms->convertVisual(visual, "", filename);
}

} // namespace Sirikata
