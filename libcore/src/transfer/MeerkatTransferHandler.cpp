#include <map>
#include <string>
#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/TransferHandlers.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <boost/bind.hpp>
#include <sirikata/core/transfer/MeerkatTransferHandler.hpp>
#include <sirikata/core/transfer/URL.hpp>
#include <boost/lexical_cast.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::MeerkatNameHandler);
AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::MeerkatChunkHandler);

namespace Sirikata {
namespace Transfer {

MeerkatNameHandler& MeerkatNameHandler::getSingleton() {
    return AutoSingleton<MeerkatNameHandler>::getSingleton();
}
void MeerkatNameHandler::destroy() {
    AutoSingleton<MeerkatNameHandler>::destroy();
}


MeerkatChunkHandler& MeerkatChunkHandler::getSingleton() {
    return AutoSingleton<MeerkatChunkHandler>::getSingleton();
}
void MeerkatChunkHandler::destroy() {
    AutoSingleton<MeerkatChunkHandler>::destroy();
}


MeerkatNameHandler::MeerkatNameHandler()
 : CDN_HOST_NAME(GetOptionValue<String>(OPT_CDN_HOST)),
   CDN_SERVICE(GetOptionValue<String>(OPT_CDN_SERVICE)),
   CDN_DNS_URI_PREFIX(GetOptionValue<String>(OPT_CDN_DNS_URI_PREFIX)),

   mCdnAddr(CDN_HOST_NAME, CDN_SERVICE)
{
}

MeerkatNameHandler::~MeerkatNameHandler() {

}

void MeerkatNameHandler::resolve(std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) {
    URL url(request->getURI());
    assert(!url.empty());

    std::string dns_uri_prefix = CDN_DNS_URI_PREFIX;
    std::string host_name = CDN_HOST_NAME;
    Network::Address cdn_addr = mCdnAddr;
    if (url.host() != "") {
        host_name = url.context().hostname();
        std::string service = url.context().service();
        if (service == "") {
            service = CDN_SERVICE;
        }
        Network::Address given_addr(host_name, service);
        cdn_addr = given_addr;
        dns_uri_prefix = "";
    }

    HttpManager::Headers headers;
    headers["Host"] = host_name;

    HttpManager::getSingleton().head(
        cdn_addr, dns_uri_prefix + url.fullpath(),
        std::tr1::bind(&MeerkatNameHandler::request_finished, this, _1, _2, _3, request, callback),
        headers
    );
}

void MeerkatNameHandler::request_finished(std::tr1::shared_ptr<HttpManager::HttpResponse> response,
        HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error,
        std::tr1::shared_ptr<MetadataRequest> request, NameCallback callback) {

    std::tr1::shared_ptr<RemoteFileMetadata> bad;

    if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
        SILOG(transfer, error, "Request parsing failed during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
        SILOG(transfer, error, "Response parsing failed during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::BOOST_ERROR) {
        SILOG(transfer, error, "A boost error happened during an HTTP name lookup (" << request->getURI() << "). Boost error = " << boost_error.message());
        callback(bad);
        return;
    } else if (error != HttpManager::SUCCESS) {
        SILOG(transfer, error, "An unknown error happened during an HTTP name lookup. (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    if (response->getHeaders().size() == 0) {
        SILOG(transfer, error, "There were no headers returned during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    HttpManager::Headers::const_iterator it;
    it = response->getHeaders().find("Content-Length");
    if (it != response->getHeaders().end()) {
        SILOG(transfer, error, "Content-Length header was present when it shouldn't be during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    if (response->getStatusCode() != 200) {
        SILOG(transfer, error, "HTTP status code = " << response->getStatusCode() << " instead of 200 during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    it = response->getHeaders().find("File-Size");
    if (it == response->getHeaders().end()) {
        SILOG(transfer, error, "Expected File-Size header not present during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }
    std::string file_size_str = it->second;

    it = response->getHeaders().find("Hash");
    if (it == response->getHeaders().end()) {
        SILOG(transfer, error, "Expected Hash header not present during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }
    std::string hash = it->second;

    if (response->getData()) {
        SILOG(transfer, error, "Body present during an HTTP name lookup (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    Fingerprint fp;
    try {
        fp = Fingerprint::convertFromHex(hash);
    } catch(std::invalid_argument e) {
        SILOG(transfer, error, "Hash header didn't contain a valid Fingerprint string (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    std::istringstream istream(file_size_str);
    uint64 file_size;
    istream >> file_size;
    std::ostringstream ostream;
    ostream << file_size;
    if(ostream.str() != file_size_str) {
        SILOG(transfer, error, "Error converting File-Size header string to integer (" << request->getURI() << ")");
        callback(bad);
        return;
    }

    //Just treat everything as a single chunk for now
    Range whole(0, file_size, LENGTH, true);
    Chunk chunk(fp, whole);
    ChunkList chunkList;
    chunkList.push_back(chunk);

    std::tr1::shared_ptr<RemoteFileMetadata> met(new RemoteFileMetadata(fp, request->getURI(),
            file_size, chunkList, response->getRawHeaders()));

    callback(met);
    SILOG(transfer, detailed, "done http name handler request_finished");
}

MeerkatChunkHandler::MeerkatChunkHandler()
 : CDN_HOST_NAME(GetOptionValue<String>(OPT_CDN_HOST)),
   CDN_SERVICE(GetOptionValue<String>(OPT_CDN_SERVICE)),
   CDN_DOWNLOAD_URI_PREFIX(GetOptionValue<String>(OPT_CDN_DOWNLOAD_URI_PREFIX)),
   mCdnAddr(CDN_HOST_NAME, CDN_SERVICE)
{
}

MeerkatChunkHandler::~MeerkatChunkHandler() {
}

void MeerkatChunkHandler::get(std::tr1::shared_ptr<RemoteFileMetadata> file,
        std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback) {

    //Check for null arguments
    std::tr1::shared_ptr<DenseData> bad;
    if (!file) {
        SILOG(transfer, error, "HttpChunkHandler get called with null file parameter");
        callback(bad);
        return;
    }
    if (!chunk) {
        SILOG(transfer, error, "HttpChunkHandler get called with null chunk parameter");
        callback(bad);
        return;
    }

    //Make sure chunk given is part of file
    bool foundIt = false;
    const ChunkList & chunks = file->getChunkList();
    for (ChunkList::const_iterator it = chunks.begin(); it != chunks.end(); it++) {
        if(*chunk == *it) {
            foundIt = true;
        }
    }
    if(!foundIt) {
        SILOG(transfer, error, "HttpChunkHandler get called with chunk not present in file metadata");
        callback(bad);
        return;
    }

    //Check to see if it's in the cache first
    SharedChunkCache::getSingleton().getCache()->getData(file->getFingerprint(), chunk->getRange(), std::tr1::bind(
            &MeerkatChunkHandler::cache_check_callback, this, _1, file->getURI(), chunk, callback));
}

void MeerkatChunkHandler::get(std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback) {
    std::tr1::shared_ptr<DenseData> bad;
    if (!chunk) {
        SILOG(transfer, error, "HttpChunkHandler get called with null chunk parameter");
        callback(bad);
        return;
    }
    //Check to see if it's in the cache first
    SharedChunkCache::getSingleton().getCache()->getData(chunk->getHash(), chunk->getRange(), std::tr1::bind(
            &MeerkatChunkHandler::cache_check_callback, this, _1, URI("meerkat:///"), chunk, callback));
}

void MeerkatChunkHandler::cache_check_callback(const SparseData* data, const URI& uri,
            std::tr1::shared_ptr<Chunk> chunk, ChunkCallback callback) {
    if (data) {
        std::tr1::shared_ptr<const DenseData> flattened = data->flatten();
        callback(flattened);
    } else {
        URL url(uri);
        assert(!url.empty());

        std::string download_uri_prefix = CDN_DOWNLOAD_URI_PREFIX;
        std::string host_name = CDN_HOST_NAME;
        Network::Address cdn_addr = mCdnAddr;
        if (url.host() != "") {
            host_name = url.context().hostname();
            std::string service = url.context().service();
            if (service == "") {
                service = CDN_SERVICE;
            }
            Network::Address given_addr(host_name, service);
            cdn_addr = given_addr;
            download_uri_prefix = "";
        }

        HttpManager::Headers headers;
        headers["Host"] = host_name;
        headers["Accept-Encoding"] = "deflate, gzip";

        bool chunkReq = false;
        if(!chunk->getRange().goesToEndOfFile() || chunk->getRange().startbyte() != 0) {
            chunkReq = true;
            headers["Range"] = "bytes=" + boost::lexical_cast<String>(chunk->getRange().startbyte()) +
                "-" + boost::lexical_cast<String>(chunk->getRange().endbyte());
        }

        HttpManager::getSingleton().get(
            cdn_addr, download_uri_prefix + "/" + chunk->getHash().convertToHexString(),
            std::tr1::bind(&MeerkatChunkHandler::request_finished, this, _1, _2, _3, uri, chunk, chunkReq, callback),
            headers
        );
    }
}

void MeerkatChunkHandler::request_finished(std::tr1::shared_ptr<HttpManager::HttpResponse> response,
        HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error,
        const URI& uri, std::tr1::shared_ptr<Chunk> chunk,
        bool chunkReq, ChunkCallback callback) {

    std::tr1::shared_ptr<DenseData> bad;
    std::string reqType = "file request";
    if(chunkReq) reqType = "chunk request";

    if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
        SILOG(transfer, error, "Request parsing failed during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
        SILOG(transfer, error, "Response parsing failed during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    } else if (error == Transfer::HttpManager::BOOST_ERROR) {
        SILOG(transfer, error, "A boost error happened during an HTTP " << reqType << ". Boost error = " << boost_error.message() << " (" << uri << ")");
        callback(bad);
        return;
    } else if (error != HttpManager::SUCCESS) {
        SILOG(transfer, error, "An unknown error happened during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    }

    if (response->getHeaders().size() == 0) {
        SILOG(transfer, error, "There were no headers returned during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    }

    HttpManager::Headers::const_iterator it;
    it = response->getHeaders().find("Content-Length");
    if (it == response->getHeaders().end()) {
        SILOG(transfer, error, "Content-Length header was not present when it should be during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    }

    if (response->getStatusCode() != 200) {
        SILOG(transfer, error, "HTTP status code = " << response->getStatusCode() << " instead of 200 during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    }

    if (!response->getData()) {
        SILOG(transfer, error, "Body not present during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    }

    it = response->getHeaders().find("Range");
    if (chunkReq && it == response->getHeaders().end()) {
        SILOG(transfer, error, "Expected Range header not present during an HTTP " << reqType << " (" << uri << ")");
        callback(bad);
        return;
    } else if (chunkReq) {
        std::string range_str = it->second;
        bool range_parsed = false;

        if (range_str.substr(0,6) == "bytes=") {
            range_str = range_str.substr(6);
            size_type dashPos = range_str.find('-');
            if (dashPos != std::string::npos) {
                range_str.replace(dashPos, 1, " ");
                std::istringstream instream(range_str);
                uint64 range_start;
                uint64 range_end;
                instream >> range_start;
                instream >> range_end;

                if (range_start == chunk->getRange().startbyte() &&
                        range_end == chunk->getRange().endbyte() &&
                        response->getData()->length() == chunk->getRange().length()) {
                    range_parsed = true;
                }
            }
        }

        if(!range_parsed) {
            SILOG(transfer, error, "Range header has invalid format during an HTTP " << reqType << " header='" << it->second << "'" << " (" << uri << ")");
            callback(bad);
            return;
        }
    } else if (!chunkReq && response->getData()->length() != chunk->getRange().size()) {
        SILOG(transfer, error, "Data retrieved not expected size during an HTTP "
                << reqType << " response=" << response->getData()->length() << " expected=" << chunk->getRange().size());
        callback(bad);
        return;
    }

    SILOG(transfer, detailed, "about to call addToCache with fingerprint ID = " << chunk->getHash().convertToHexString());
    SharedChunkCache::getSingleton().getCache()->addToCache(chunk->getHash(), response->getData());

    callback(response->getData());
    SILOG(transfer, detailed, "done http chunk handler request_finished");
}

}
}
