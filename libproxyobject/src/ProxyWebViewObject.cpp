/*  Sirikata Object Host -- Proxy WebView Object
 *  ProxyWebViewObject.cpp
 *
 *  Copyright (c) 2009, Adam Jean Simmons
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

#include <sirikata/proxyobject/ProxyWebViewObject.hpp>

using namespace Sirikata;

ProxyWebViewObject::ProxyWebViewObject(ProxyManager* man, const SpaceObjectReference& id, VWObjectPtr vwobj)
 : ProxyMeshObject(man, id, vwobj)
{
}

void ProxyWebViewObject::loadURL(const std::string& url)
{
	WebViewProvider::notify(&WebViewListener::loadURL, url);
}

void ProxyWebViewObject::loadFile(const std::string& filename)
{
	WebViewProvider::notify(&WebViewListener::loadFile, filename);
}

void ProxyWebViewObject::loadHTML(const std::string& html)
{
	WebViewProvider::notify(&WebViewListener::loadHTML, html);
}

void ProxyWebViewObject::evaluateJS(const std::string& javascript)
{
	WebViewProvider::notify(&WebViewListener::evaluateJS, javascript);
}

void ProxyWebViewObject::setPosition(const OverlayPosition& position)
{
	WebViewProvider::notify(&WebViewListener::setPosition, position);
}

void ProxyWebViewObject::hide()
{
	WebViewProvider::notify(&WebViewListener::hide);
}

void ProxyWebViewObject::show()
{
	WebViewProvider::notify(&WebViewListener::show);
}

void ProxyWebViewObject::resize(int width, int height)
{
	WebViewProvider::notify(&WebViewListener::resize, width, height);
}
