/** @file ipcsession.cpp
 *  @brief IPC sessions using 'IpcProtocol' and 'IpcMessage'.
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

#include "wawt/ipcsession.h"

#include <cryptopp/config.h>
#include <cryptopp/osrng.h>

#ifdef WAWT_WIDECHAR
#define S(str) (L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) (u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

namespace Wawt {

namespace {

bool checkMessage(uint32_t             *salt,
                  uint32_t             *random,
                  char                 *type,
                  const IpcMessage&     message) {
    auto p      = message.cbegin();
    auto end    = message.cend();
    auto bytes  = uint16_t{};
    
    if (IpcMessageUtil::extractSalt(salt, &p, end)
     && IpcMessageUtil::extractHdr(type, &bytes, &p, end)) {
        bytes -= IpcMessageUtil::hdrsz;

        if (p + bytes == end) {
            if (*type == IpcMessageUtil::kDATA
             || *type == IpcMessageUtil::kDIGDATA
             || *type == IpcMessageUtil::kCLOSE) {
                return true;                                          // RETURN
            }

            if (*type == IpcMessageUtil::kDIGEST) {
                return bytes == CryptoPP::SHA256::DIGESTSIZE;         // RETURN
            }

            if (*type == IpcMessageUtil::kSTARTUP) {
                return IpcMessageUtil::extractValue(random, &p, end); // RETURN
            }
        }
    }
    return false;                                                     // RETURN
}

IpcMessage makeDigest(uint32_t salt, CryptoPP::SHA256& hash) {
    constexpr auto digestsz = IpcMessageUtil::prefixsz
                            + CryptoPP::SHA256::DIGESTSIZE;
    auto message = IpcMessage(std::make_unique<char[]>(digestsz), digestsz, 0);
    auto p       = message.data();
    p            = IpcMessageUtil::initPrefix(p,
                                            salt,
                                            CryptoPP::SHA256::DIGESTSIZE,
                                            IpcMessageUtil::kDIGEST);
    hash.Final(reinterpret_cast<uint8_t*>(p));
    return message;
}

IpcMessage makeHandshake(uint32_t           random1,
                         uint32_t           random2,
                         const IpcMessage&  data)
{
    auto length     = data.length();
    auto size       = length + sizeof(random2) + IpcMessageUtil::prefixsz;
    auto message    = IpcMessage(std::make_unique<char[]>(size), size, 0);
    auto p          = message.data();
    p               = IpcMessageUtil::initPrefix(p,
                                               random1,
                                               length-IpcMessageUtil::prefixsz,
                                               IpcMessageUtil::kSTARTUP);
    p               = IpcMessageUtil::initValue(p, random2);
    std::memcpy(p, data.cbegin(), length); 
    return message;
}

} // unnamed namespace

                            //-----------------
                            // class IpcSession
                            //-----------------

IpcSession::IpcSession(const IpcMessage&         handshake,
                       uint32_t                  random1,
                       uint32_t                  random2,
                       IpcProtocol              *adapter,
                       IpcProtocol::ChannelId    id,
                       IpcProtocol::ChannelMode  mode)
: d_handshake(makeHandshake(random1, random2, handshake))
, d_rcvSalt(random1)
, d_adapter(adapter)
, d_channelId(id)
, d_random(random2)
, d_mode(mode)
{
}

IpcProtocol::MessageChain
IpcSession::getStartupDigest()
{
    assert(d_state == State::eWAITING_ON_CONNECT);
    d_state         = State::eWAITING_ON_DIGEST;
    auto    chain   = MessageChain{};
    auto    hash    = CryptoPP::SHA256{};
    hash.Update(reinterpret_cast<const uint8_t*>(d_handshake.cbegin()),
                d_handshake.length());
    chain.emplace_front(makeDigest(d_rcvSalt, hash));
    return chain;
}

IpcProtocol::MessageChain
IpcSession::saveStartupDigest(MessageNumber  initialValue,
                              IpcMessage&&   receivedDigest)
{
    assert(d_state == State::eWAITING_ON_DIGEST);
    d_sendSalt         = initialValue;
    d_state            = State::eWAITING_ON_START;
    d_digest           = std::move(receivedDigest);
    d_digest.d_offset += IpcMessageUtil::prefixsz;
    auto chain         = MessageChain{};
    chain.emplace_front(std::move(d_handshake));
    return chain;
}

                    //=====================================
                    // struct IpcSessionManager::CryptoPrng
                    //=====================================

struct IpcSessionManager::CryptoPrng {
    CryptoPP::AutoSeededRandomPool  d_prng;

    uint32_t random32() {
        return d_prng.GenerateWord32();
    }
};

                            //------------------------
                            // class IpcSessionManager
                            //------------------------

// PRIVATE MEMBERS
void
IpcSessionManager::channelUpdate(IpcProtocol::ChannelId      id,
                                 IpcProtocol::ChannelChange  changeTo,
                                 IpcProtocol::ChannelMode    mode)
{
    // Note: no other connection update for 'id' can be delivered
    // until this function returns.
    auto guard = std::unique_lock(d_lock);

    if (d_opened) {
        if (changeTo == IpcProtocol::ChannelChange::eREADY) {
            auto random1 = d_prng_p->random32();
            auto random2 = d_prng_p->random32();
            auto session = std::make_shared<IpcSession>(d_handshake,
                                                        random1,
                                                        random2,
                                                        d_adapter,
                                                        id,
                                                        mode);
            if (d_sessionMap.try_emplace(id, session).second) {
                guard.release()->unlock();
                d_adapter->sendMessage(id, session->getStartupDigest());
                return;                                               // RETURN
            }
            assert(!"Connection ID reused.");
        }
        else {
            auto it = d_sessionMap.find(id);

            if (it != d_sessionMap.end()) {
                auto session = it->second;
                d_sessionMap.erase(it);

                if (session->state() != IpcSession::State::eWAITING_ON_DISC
                 && d_dropMessage.length() > 0) {
                    session->setClosed();
                    d_messageCb(session,
                                MessageType::eDROP,
                                IpcMessage(d_dropMessage));
                }
            }
        }
    }
    d_adapter->closeChannel(id);
    return;                                                           // RETURN
}

void
IpcSessionManager::processMessage(IpcProtocol::ChannelId  id,
                                  IpcMessage&&            message)
{
    auto guard = std::unique_lock(d_lock);

    auto it     = d_sessionMap.find(id);
    auto salt   = uint32_t{};
    auto random = uint32_t{};
    auto type   = char{};

    if (d_opened) {
        if (it == d_sessionMap.end()) {
            assert(!"Message for unknown connection ID.");
        }
        else if (checkMessage(&salt, &random, &type, message)) {
            auto session    = it->second;
            auto ssnguard   = std::unique_lock(*session);
            auto close      = false;

            switch (session->state()) {
              case IpcSession::State::eWAITING_ON_DIGEST: {
                if(IpcMessageUtil::kDIGEST == type) {
                    d_adapter->sendMessage(
                        id,
                        session->saveStartupDigest(salt, std::move(message)));
                    assert(session->state()
                                    == IpcSession::State::eWAITING_ON_START);
                }
                else {
                    close = true;
                    assert(!"Required a DIGEST.");
                }
              } break;                                                 // BREAK
              case IpcSession::State::eWAITING_ON_START: {
                if (IpcMessageUtil::kSTARTUP == type
                 && session->verifyStartupMessage(salt, random, message)) {
                    message.d_offset += IpcMessageUtil::prefixsz
                                      + sizeof(random);

                    if (message.length() > 0) {
                        d_messageCb(std::move(session),
                                    MessageType::eDATA,
                                    std::move(message));
                    }
                    assert(session->state() == IpcSession::State::eOPEN);
                }
                else {
                    close = true;
                    assert(!"Required a STARTUP.");
                }
              } break;                                                 // BREAK
              case IpcSession::State::eOPEN: {
                auto msgtype        = MessageType::eDATA;
                message.d_offset   += IpcMessageUtil::prefixsz;

                if (IpcMessageUtil::kDATA == type) {
                }
                else if (IpcMessageUtil::kDIGDATA == type) {
                    msgtype = MessageType::eDIGESTED_DATA;
                }
                else if (IpcMessageUtil::kDIGEST == type) {
                    msgtype = MessageType::eDIGEST;
                }
                else if (IpcMessageUtil::kCLOSE == type) {
                    session->setClosed();
                    d_adapter->closeChannel(id);
                    assert(session->state()
                                    == IpcSession::State::eWAITING_ON_DISC);
                    session.reset();
                }
                else {
                    close = true;
                    assert(!"STARTUP unexpected.");
                }

                if (!close && message.length() > 0) {
                    d_messageCb(std::move(session),
                                msgtype,
                                std::move(message));
                }
              } break;                                                 // BREAK
              case IpcSession::State::eWAITING_ON_DISC: {
              } break;                                                 // BREAK
              default: {
                assert(!"Session in invalid state.");
              }
            }

            if (!close) {
                return;                                               // RETURN
            }
        }
    }
    d_adapter->closeChannel(id);
    return;                                                           // RETURN
}

bool
IpcSession::verifyStartupMessage(IpcMessage::MessageNumber  digestValue,
                                 uint32_t                   random,
                                 const IpcMessage&          message)
{
    d_random ^= random;
    d_winner  = ((d_random & 8) == 0) ==
                    (d_mode == IpcProtocol::ChannelMode::eCREATOR);

    return digestValue == d_sendSalt
        && CryptoPP::SHA256().VerifyDigest(
                reinterpret_cast<const uint8_t*>(d_digest.cbegin()),
                reinterpret_cast<const uint8_t*>(message.cbegin()),
                message.length());     // RETURN
}

// PUBLIC CONSTRUCTORS
IpcSessionManager::IpcSessionManager(IpcProtocol *adapter, MessageCb&& cb)
: d_prng_p(new IpcSessionManager::CryptoPrng())
, d_adapter(adapter)
, d_messageCb(std::move(cb))
{
    d_adapter->installCallbacks([this](auto id, auto changeTo, auto mode) {
                                    channelUpdate(id, changeTo, mode);
                                },
                                [this](auto id, auto&& message) {
                                    processMessage(id, std::move(message));
                                });
}

IpcSessionManager::~IpcSessionManager()
{
    delete d_prng_p;
}

// PUBLIC MEMBERS
bool
IpcSessionManager::handShakes(String_t         *diagnostics,
                              IpcMessage        disconnectMessage,
                              IpcMessage        handshakeMessage)
{
    auto guard = std::unique_lock(d_lock);

    if (!d_opened) {
        d_handshake     = std::move(handshakeMessage);
        d_dropMessage   = std::move(disconnectMessage);
        return d_opened = true;
    }
    *diagnostics = S("Reset required before changing handshake messages.");
    return false;                                                     // RETURN
}

void
IpcSessionManager::reset()
{
    auto guard = std::unique_lock(d_lock);

    if (d_opened) {
        d_opened = false;
        d_adapter->closeAdapter();
    }
}

}  // namespace Wawt

// vim: ts=4:sw=4:et:ai
