/** @file ipcmessage.cpp
 *  @brief IPC message definition and related utilities
 *
 * Copyright 2018 Bruce Szablak
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wawt/ipcmessage.h"

#include <cryptopp/config.h>
#include <cryptopp/osrng.h>

#ifdef WAWT_WIDECHAR
#define S(str) (L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) (u8"" str) // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

namespace Wawt {

namespace {

constexpr auto digestsz = IpcMessageUtil::prefixsz
                        + CryptoPP::SHA256::DIGESTSIZE;

}  // unnamed namespace

                               //----------------------
                               // struct IpcMessageUtil
                               //----------------------

bool
IpcMessageUtil::extractSalt(uint32_t *salt, const char **p, const char *end)
{
    auto type   = char{};
    auto size   = uint16_t{};

    if (extractHdr(&type, &size, p, end)
     && type == kSALT
     && size == saltsz) {
        if (salt) {
            extractValue(salt, p, end);
        }
        return true;
    }
    return false;
}

char *
IpcMessageUtil::initPrefix(char *p, uint32_t salt, uint16_t size, char type)
{
    size   += hdrsz;

    p       = initHeader(p, sizeof(salt), kSALT);
    p       = initValue( p, salt);
    p       = initHeader(p, size, type);
    return p;
}

IpcMessage::MessageNumber
IpcMessageUtil::messageNumber(const IpcMessage& message)
{
    auto salt   = uint32_t{};

    if (message.d_offset >= prefixsz) {
        auto *end   = message.cbegin();
        auto *p     = end - prefixsz;
        extractSalt(&salt, &p, end);
    }
    return salt;
}

bool
IpcMessageUtil::verifyDigestPair(const IpcMessage&       digest,
                                 const IpcMessage&       digestMessage)
{

    if (digestMessage.d_offset < prefixsz) {
        assert(!"Message missing header.");
        return false;                                                 // RETURN
    }
    auto msglength  = digestMessage.length();
    auto data       = digestMessage.cbegin() - prefixsz;
    auto datalng    = msglength + prefixsz;

    uint32_t salt1, salt2;
    auto p1         = digest.cbegin();
    auto end1       = digest.cend();
    auto p2         = data;
    auto end2       = digestMessage.cend();

    if (extractSalt(&salt1, &p1, end1)
     && extractSalt(&salt2, &p2, end2)
     && salt1 == salt2
     && kDIGEST  == p1[0]
     && kDIGDATA == p2[0]
     && ((uint8_t(p1[1]) << 8) | uint8_t(p1[2])) == digestsz  - saltsz
     && ((uint8_t(p2[1]) << 8) | uint8_t(p2[2])) == msglength + hdrsz) {
        return CryptoPP::SHA256().VerifyDigest(
                reinterpret_cast<const uint8_t*>(p1) + hdrsz, /* hash */
                reinterpret_cast<const uint8_t*>(data),
                datalng);                                             // RETURN
    }
    return false;                                                     // RETURN
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai
