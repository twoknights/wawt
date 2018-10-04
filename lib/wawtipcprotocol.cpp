/** @file wawtipcprotocol.cpp
 *  @brief Implementation of IPC utilities.
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

#include "wawtipcprotocol.h"

#include <cryptopp/config.h>
#include <cryptopp/osrng.h>

#undef  S
#undef  C
#define S(str) Wawt::String_t(u8"" str)      // UTF8 strings  (std::string)
#define C(c) (u8 ## c)
//#define S(str) Wawt::String_t(L"" str)  // wide char strings (std::wstring)
//#define C(c) (L ## c)

namespace BDS {

namespace {

using IpcMessage    = WawtIpcMessage;

using IpcConnectionProtocol = WawtIpcConnectionProtocol;

using IpcQueue      = WawtIpcQueue;

using IpcUtilities  = WawtIpcUtilities;

using MessageChain  = std::forward<IpcMessage>;


const uint8_t kSTARTUP = '\146';
const uint8_t kSALT    = '\005';
const uint8_t kDIGEST  = '\012';
const uint8_t kDATA    = '\055';
const uint8_t kSIGNED  = '\201';
const uint8_t kCLOSE   = '\303';

constexpr auto hdrsz  = 1 + sizeof(uint16_t);
constexpr auto prefixsz  = saltsz + hdrsz;
constexpr auto saltsz = 1 + sizeof(uint16_t) + sizeof(uint32_t);

IpcMessage dataPrefix(uint32_t salt, uint16_t dataSize, uint8_t prefix) {
    IpcMessage message = { std::make_unique<uint8_t>(saltsz + hdrsz);
                           saltsz + hdrsz,
                           0 };

    auto p  = message.d_data.get();
    *p++    =  kSALT              & 0xFF;
    *p++    = (saltsz      >>  8) & 0xFF;
    *p++    =  saltsz             & 0xFF;
    *p++    = (salt        >> 24) & 0xFF;
    *p++    = (salt        >> 16) & 0xFF;
    *p++    = (salt        >>  8) & 0xFF;
    *p++    =  salt               & 0xFF;
    *p++    =  prefix             & 0xFF;
    *p++    = (dataSize    >>  8) & 0xFF;
    *p++    =  dataSize           & 0xFF;
    return message;
}

bool extractSalt(uint32_t *salt, uint8_t **p, uint8_t *end)
{
    if (end - *p >= saltsz) {
        auto a = *p;

        if (a[0] == kSALT) {
            auto length = (uint16_t(a[1]) << 8) | uint16_t(a[2]);

            if (sizeof(*salt) == length - hdrsz) {
                if (salt) {
                    *salt = (uint32_t(a[3]) << 24 | (uint32_t(a[4] << 16)
                                                  | (uint32_t(a[5] << 8)
                                                  |  uint32_t(a[6]);
                }
                *p += length;
                return true;                                          // RETURN
            }
        }
    }
    return false;                                                     // RETURN
}

int  addPrefix(WawtIpcMessage *message,
               uint8_t          header,
               uint32_t         random1,
               uint32_t         random2,
               uint32_t         salt)
{
    constexpr auto prefixsz = sizeof(header)    + sizeof(uint16_t)
                            + sizeof(random1)   + sizeof(random2)
                            + sizeof(salt);

    MessageFragment prefix{ std::make_unique<char[]>(prefixsz), prefixsz, 0 };

    auto p  = prefix.d_data.get();
    *p++    =  header             & 0xFF;
    *p++    = (prefixsz    >>  8) & 0xFF;
    *p++    =  prefixsz           & 0xFF;
    *p++    = (random1     >> 24) & 0xFF;
    *p++    = (random1     >> 16) & 0xFF;
    *p++    = (random1     >>  8) & 0xFF;
    *p++    =  random1            & 0xFF;
    *p++    = (random2     >> 24) & 0xFF;
    *p++    = (random2     >> 16) & 0xFF;
    *p++    = (random2     >>  8) & 0xFF;
    *p++    =  random2            & 0xFF;
    *p++    = (salt        >> 24) & 0xFF;
    *p++    = (salt        >> 16) & 0xFF;
    *p++    = (salt        >>  8) & 0xFF;
    *p++    =  salt               & 0xFF;
    
    message->emplace_front(std::move(prefix));
    return prefixsz;
}

constexpr auto sha256sz =  hdrsz  + CryptoPP::SHA256::DIGESTSIZE;
constexpr auto digestsz =  saltsz + sha256sz;

IpcMessage digest(uint32_t salt, CryptoPP::SHA256& hash) {
    IpcMessage message = { std::make_unique<uint8_t>(digestsz); digestsz, 0 };

    auto p  = message.d_data.get();
    *p++    =  kSALT              & 0xFF;
    *p++    = (saltsz      >>  8) & 0xFF;
    *p++    =  saltsz             & 0xFF;
    *p++    = (salt        >> 24) & 0xFF;
    *p++    = (salt        >> 16) & 0xFF;
    *p++    = (salt        >>  8) & 0xFF;
    *p++    =  salt               & 0xFF;
    *p++    =  kDIGEST            & 0xFF;
    *p++    = (sha256sz    >>  8) & 0xFF;
    *p++    =  sha256sz           & 0xFF;

    hash.Final(p);
    return message;
}

}  // unnamed namespace


                        //============================
                        // struct IpcQueue::CryptoPrng
                        //============================

struct IpcQueue::CryptoPrng {
    CryptoPP::AutoSeededRandomPool  d_prng;

    CryptoPP::AutoSeededRandomPool& operator()() {
        return d_prng;
    }
};

                           //---------------------------
                           // class IpcQueue::ReplyQueue
                           //---------------------------

IpcQueue::Session *
IpcQueue::safeGet()
{
    // In C++20 there will be atomic shared pointers...
    while (d_flag.test_and_set());

    auto copy = d_session.get();
    
    d_flag.clear();
    return copy; // NOT safe to dereference
}

IpcQueue::ReplyQueue::ReplyQueue(std::shared_ptr<Session>   session,
                                 bool                       winner,
                                 int                        sessionId)
: d_session(session)
, d_winner(winnder)
, d_sessionId(sessionId)
{
    d_flag.clear();
}

bool
IpcQueue::ReplyQueue::enqueue(IpcMessage&&            message,
                              Signature&&             signature)
{
    auto datasize = message.length();

    while (d_flag.test_and_set()); auto copy = d_session; d_flag.clear();

    if (datasize > 0 && datasize <= UINT16_MAX-prefixsz && copy) {
        std::lock_guard guard(*copy);
        MessageChain chain;
        chain.emplace_front(std::move(message));

        if (signature) {
            chain.emplace_front(std::move(signature), saltsz+hdrsz, 0);
        }
        else {
            auto salt     = d_session->nextSalt(); TBD
            chain.emplace_front(dataPrefix(salt, datasize, kDATA);
        }
        return copy->enqueue(chain, false);                           // RETURN
    }
    return false;                                                     // RETURN
}

bool
IpcQueue::ReplyQueue::enqueueDigest(Signature        *signature,
                                    const IpcMessage& message)
{
    assert(signature);

    while (d_flag.test_and_set()); auto copy = d_session; d_flag.clear();

    auto datasize = message.length();

    if (datasize > 0 && datasize <= UINT16_MAX-prefixsz && copy) {
        auto guard = std::scoped_lock(*this, *copy);
        auto& [buffer, size, offset] = message;
        auto salt  = d_session->nextSalt();
        *signature = dataPrefix(salt, datasize, kSIGNED).d_data;

        CryptoPP::SHA256 hash;
        hash.Update(signature->d_data.get(), signature->d_size);
        hash.Update(buffer.get() + offset, datasize);

        MessageChain chain;
        chain.emplace_front(digest(salt, hash));

        return copy->enqueue(chain, false);                           // RETURN
    }
    return false;                                                     // RETURN
}

void
IpcQueue::ReplyQueue::closeQueue(IpcMessage&&         message)
{
    std::shared_ptr<Session> copy;

    while (d_flag.test_and_set());

    d_session.swap(copy);

    d_flag.clear();

    if (copy) {
        std::lock_guard guard(*copy);
        MessageChain chain;
        auto datasize = message.length();

        if (datasize > 0) {
            chain.emplace_front(std::move(message));
        }

        auto salt     = d_session->nextSalt();
        chain.emplace_front(dataPrefix(salt, datasize, kCLOSE);
        d_session->enqueue(chain, true); TBD
    }
    return;                                                           // RETURN
}

                               //---------------
                               // class IpcQueue
                               //---------------

void
IpcQueue::connectionUpdate(Protocol::ConnectionId   id,
                           Protocol::ConnectStatus  status,
                           Protocol::ConnectRole    role)
{
    // Note: no other connection update for 'id' can be delivered
    // until this function returns.
    auto guard = std::unique_lock(*this);

    if (status == Protocol::ConnectStatus::eOK) {
        auto session = std::make_shared<Session>(id, role);

        if (d_sessionMap.try_emplace(id, session).second) {
            handshake(session);
            return;                                                   // RETURN
        }
        assert(!"Connection ID reused.");
    }
    else {
        auto it = d_sessionMap.find(id);

        if (it != d_sessionMap.end()) {
            auto session = it->second;
            d_sessionMap.erase(it);
            disconnected(session);
        }
    }
    guard.release()->unlock();
    d_adapter->closeConnection(id);
    return;                                                           // RETURN
}

void
IpcQueue::processMessage(ConnectionId id, IpcMessage&& message)
{
    auto guard = std::unique_lock(*this);

    auto it     = d_sessionMap.find(id);
    auto salt   = uint32_t{};
    auto type   = uint8_t{};
    auto length = uint16_t{};

    if (it == d_sessionMap.end()) {
        assert(!"Message for unknown connection ID.");
    }
    else if (validateChain(&salt, &type, &length, message)) {
        auto session    = it->second;
        auto ssnguard   = std::unique_lock(*session);

        if (session->isOpen()) { TBD
            if (kSTARTUP == type) {
                TBD
                return;                                               // RETURN
            }

            if (kCLOSE != type) {
                auto reply      = ReplyQueue(session, session->winner(), id);
                auto msgtype    = MessageType::eDIGEST;

                if (kDIGEST != type) {
                    message.d_offset += saltsz + hdrsz;
                    msgtype = type == kDATA ? MessageType::eDATA
                                            : MessageType::eSIGNED_DATA;
                }
                d_incoming.emplace_back(std::move(reply),
                                        std::move(message),
                                        MessageType::eDIGEST);
                d_signal.notify_all();
                return;                                               // RETURN
            }
            session->setClosed(); TBD
            auto reply = ReplyQueue({}, session->winner(), id); TBD

            if (length > 0) {
                d_incoming.emplace_back(reply,
                                        std::move(message),
                                        MessageType::eDATA);
            }

            if (disconnect.length() > 0) {
                d_incoming.emplace_back(std::move(reply),
                                        IpcMessage(disconnect),
                                        MessageType::eDATA);
            }
        }
    }
    guard.release()->unlock();
    d_adapter->closeConnection(id);
    return;                                                           // RETURN
}

IpcQueue::IpcQueue(IpcConnectionProtocol *adapter)
: d_prng(std::make_unique<IpcQueue::CryptoPrng>())
, d_adapter(adapter)
{
    d_adapter->installCallbacks([this](auto id, auto status, auto role) {
                                    connectionUpdate(id, status, role);
                                },
                                [this](auto id, auto&& message) {
                                    processMessage(id, std::move(message));
                                });
}

bool
IpcQueue::localEnqueue(IpcMessage&& message)
{
    auto guard = std::unique_lock(*this);

    if (!d_shutdown) {
        ReplyQueue local(std::shared_ptr<Session>(), false, kLOCAL_SSNID);
        d_incoming.emplace_back(std::move(local), std::move(message));
        d_signal.notify_all();
    }
    return !d_shutdown;                                               // RETURN
}

bool
IpcQueue::openAdapter(Wawt::String_t   *diagnostics,
                      IpcMessage        disconnectMessage,
                      IpcMessage        handshakeMessage)
{
    std::lock_guard guard(*this);

    if (!d_opened) {
        d_handshake     = std::move(handshakeMessage);
        d_disconnect    = std::move(disconnectMessage);
        return d_opened = d_adapter->openAdapter(diagnostics);
    }
    *diagnostics = S("Adapter previously opened.");
    return false;                                                     // RETURN
}

void
IpcQueue::reset()
{
    std::lock_guard guard(*this);
    // note that local enqueing can occur even when not opened.

    if (d_opened) {
        d_handshake.reset();
        d_disconnect.reset();
        d_adapter->closeAdapter();
    }
    d_opened = false;
    d_incoming.clear();
    d_signal.notify_all();
    return;                                                           // RETURN
}

IpcQueue::Indication
IpcQueue::waitForIndication()
{
    auto guard = std::unique_lock(*this);

    while (!d_shutdown) {
        if (d_incoming.empty()) {
            d_signal.wait(guard);
        }
        else {
            auto indication = std::move(d_incoming.front());
            d_incoming.pop_front();
            
            if (!indication.first.d_session->isOpen()) {
                indication.first.d_session.reset();
            }
            return indication;                                        // RETURN
        }
    }
    throw Shutdown();                                                  // THROW
}

                               //-------------------
                               // class IpcUtilities
                               //-------------------

bool
IpcUtilities::skipToUserData(IpcMessage *message)
{
    while (!message->empty()) {
        auto fragment = message->front();
        auto size     = fragment.d_size;
        auto offset   = fragment.d_offset;

        while (size > offset) {
            if (size - offset < hdrsz) {
                assert(!"Corrupt fragment");
                message->clear();
                return false;                                         // RETURN
            }
            auto hdr      = fragment.d_data.get() + offset;
            auto hdrbyte  = hdr[0];
            auto blocksz  = (uint16_t(hdr[1]) << 8) | uint16_t(hdr[2]);

            if (hdrbyte == kDATA) {
                fragment.d_offset = offset + hdrsz;

                if (fragment.d_offset == size) {
                    message->pop_front();
                }
                else if (fragment.d_offset > size) {
                    assert(!"Corrupt data header");
                    message->clear();
                    return false;                                     // RETURN
                }
                return true;                                          // RETURN
            }

            if (hdrbyte != kSALT) {
                if (hdrbyte == kDIGEST) {
                    return false;                                     // RETURN
                }
                assert(!"Corrupt header type");
                message->clear();
                return false;                                         // RETURN
            }

            if (blocksz + offset > size) {
                assert(!"Corrupt header");
                message->clear();
                return false;                                         // RETURN
            }
            offset += blocksz;
        }
        message->pop_front();
    }
    return false;                                                     // RETURN
}

IpcQueue::MessageNumber
IpcUtilities::messageNumber(const IpcMessage& message)
{
    auto *p     = message.get();
    auto *end   = p + saltsz+hdrsz;
    auto salt   = uint32_t{};

    extractSalt(&salt, &p, end);
    return salt;
}

bool
IpcUtilities::verifySignedMessage(const IpcMessage&       digest,
                                  const IpcMessage&       signedMessage)
{
    auto& [buffer, size, offset] = signedMessage;
    auto  length = size - offset;

    if (offset < prefixsz) {
        assert(!"Message missing signature.");
        return false;                                                 // RETURN
    }
    auto data = buffer.get() + offset - prefixsz;
    auto datalng = size - (offset - prefixsz);

    uint32_t salt1, salt2;
    uint8_t *p1   = digest.d_data.get() + digest.d_data.d_offset;
    uint8_t *end1 = p1 +  digest.d_size - digest.d_offset;
    uint8_t *p2   = data;
    uint8_t *end2 = buffer.get() + size;

    if (extractSalt(&salt1, &p1, end1)
     && extractSalt(&salt2, &p2, end2)
     && salt1 == salt2
     && kDIGEST == p1[0]
     && kSIGNED == p2[0]
     && ((uint16_t(p1[1]) << 8) | uint16_t(p1[2])) == sha256sz;
     && ((uint16_t(p2[1]) << 8) | uint16_t(p2[2])) == length+hdrsz) {
        return CryptoPP::SHA256().VerifyDigest(p1 + hdrsz, /* hash */
                                               data,
                                               datalng);              // RETURN
    }
    return false;                                                     // RETURN
}

}  // namespace BDS
// vim: ts=4:sw=4:et:ai
