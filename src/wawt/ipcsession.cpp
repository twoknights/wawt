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

const String_t::value_type *s_messages[] = {
    S("Channel setup is in-progress."),
    S("Channel setup was canceled."),
    S("Channel configuration is malformed."),
    S("Channel configuration is invalid."),
    S("An error ocurred during channel setup."),
    S("Channel setup has completed."),
};

String_t
diagnostic(const IpcProtocol::Provider::SetupTicket& ticket) {
    auto p=s_messages[static_cast<unsigned int>(ticket->d_setupStatus.load())];
    return String_t(p);
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

// PRIVATE MEMBERS
void
IpcSession::saveStartupDigest(MessageNumber  initialValue,
                              IpcMessage&&   receivedDigest)
{
    assert(d_state == State::eWAITING_ON_DIGEST);
    d_sendSalt         = initialValue;
    d_state            = State::eWAITING_ON_START;
    d_digest           = std::move(receivedDigest);
    d_digest.d_offset += IpcMessageUtil::prefixsz;
    return;                                                           // RETURN
}

bool
IpcSession::verifyStartupMessage(IpcMessage::MessageNumber  digestValue,
                                 uint32_t                   random,
                                 const IpcMessage&          message)
{
    d_random ^= random;
    d_winner  = ((d_random & 8) == 0) == d_initiated;

    return digestValue == d_sendSalt
        && CryptoPP::SHA256().VerifyDigest(
                reinterpret_cast<const uint8_t*>(d_digest.cbegin()),
                reinterpret_cast<const uint8_t*>(message.cbegin()),
                message.length());     // RETURN
}

// PUBLIC MEMBERS
IpcSession::IpcSession(uint32_t                  random1,
                       uint32_t                  random2,
                       ChannelPtr                channel,
                       bool                      initiated)
: d_rcvSalt(random1)
, d_random(random2)
, d_channel(channel)
, d_initiated(initiated)
{
}

bool
IpcSession::enqueue(MessageChain&& chain, bool close)
{
    if (d_state == State::eOPEN) {
        auto channel = d_channel.lock();
        auto sent    = channel ? channel->sendMessage(std::move(chain)) 
                               : false;

        if (!sent || close) {
            d_state = State::eWAITING_ON_DISC;
        }
        return sent;                                                  // RETURN
    }
    return false;                                                     // RETURN
}

void
IpcSession::receivedMessage(IpcMessage&& message)
{
    auto guard = std::unique_lock(d_lock);

    auto salt    = uint32_t{};
    auto random  = uint32_t{};
    auto type    = char{};
    auto channel = d_channel.lock();

    assert(channel); // otherwise, who is calling this method?

    if (checkMessage(&salt, &random, &type, message)) {
        auto ssnguard   = std::unique_lock(d_lock);
        auto close      = false;

        switch (state()) {
          case State::eWAITING_ON_DIGEST: {
            if(IpcMessageUtil::kDIGEST == type) {
                saveStartupDigest(salt, std::move(message));
                auto chain = MessageChain{};
                chain.emplace_front(std::move(d_handshake));
                channel->sendMessage(std::move(chain));
                assert(state() == State::eWAITING_ON_START);
            }
            else {
                close = true;
                assert(!"Required a DIGEST.");
            }
          } break;                                                 // BREAK
          case State::eWAITING_ON_START: {
            if (IpcMessageUtil::kSTARTUP == type
             && verifyStartupMessage(salt, random, message)) {
                message.d_offset += IpcMessageUtil::prefixsz
                                  + sizeof(random);

                if (message.length() > 0) {
                    d_messageCb(MessageType::eDATA, std::move(message));
                }
                assert(state() == State::eOPEN);
            }
            else {
                close = true;
                assert(!"Required a STARTUP.");
            }
          } break;                                                 // BREAK
          case State::eOPEN: {
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
                if (message.length() > 0) {
                    setClosed();
                    channel->closeChannel();
                    assert(state() == State::eWAITING_ON_DISC);
                }
                else {
                    close = true;
                }
            }
            else {
                close = true;
                assert(!"STARTUP unexpected.");
            }

            if (!close) {
                if (message.length() > 0) {
                    d_messageCb(msgtype, std::move(message));
                }
                else {
                    close = true;
                    assert(!"Expected message content.");
                }
            }
          } break;                                                 // BREAK
          case State::eWAITING_ON_DISC: {
          } break;                                                 // BREAK
          default: {
            assert(!"Session in invalid state.");
            close = true;
          }
        }

        if (!close) {
            return;                                               // RETURN
        }
    }
    channel->closeChannel();
    return;                                                           // RETURN
}

void
IpcSession::shutdown()
{
    auto channel = d_channel.lock();

    if (channel) {
        d_lock.lock();
        channel->closeChannel();
        d_lock.unlock();
    }
    return;                                                           // RETURN
}

void
IpcSession::startHandshake(IpcMessage&& handshake, MessageCb&& messageCb)
{
    assert(d_state == State::eWAITING_ON_CONNECT);

    auto channel   = d_channel.lock();
    assert(channel);

    d_messageCb    = std::move(messageCb);
    d_state        = State::eWAITING_ON_DIGEST;
    d_handshake    = makeHandshake(d_rcvSalt, d_random, std::move(handshake));
    auto chain     = MessageChain{};
    auto hash      = CryptoPP::SHA256{};
    hash.Update(reinterpret_cast<const uint8_t*>(d_handshake.cbegin()),
                d_handshake.length());
    chain.emplace_front(makeDigest(d_rcvSalt, hash));
    channel->sendMessage(std::move(chain));
}

                    //=====================================
                    // struct IpcSessionFactory::CryptoPrng
                    //=====================================

struct IpcSessionFactory::CryptoPrng {
    CryptoPP::AutoSeededRandomPool  d_prng;

    uint32_t random32() {
        return d_prng.GenerateWord32();
    }
};

                            //------------------------
                            // class IpcSessionFactory
                            //------------------------

// PRIVATE MEMBERS
void
IpcSessionFactory::channelSetup(IpcProtocol::Provider::ChannelPtr  channel_wp,
                                IpcProtocol::Provider::SetupTicket ticket)
                                                                      noexcept
{
    // Note: caller discards all state tracking "ticket" on return.  So must
    // this method.
    assert(ticket);

    auto guard      = std::unique_lock(d_lock);
    auto channel    = channel_wp.lock();
    auto pending    = static_cast<Setup*>(ticket.get());

    if (!pending) {
        if (channel) {
            channel->closeChannel();
        }
    }
    else if (!channel) {
        pending->d_setupUpdate(false, ticket, diagnostic(ticket));
    }
    else if (!d_opened) {
        channel->closeChannel();
        pending->d_setupUpdate(false, ticket, S("Shutting down."));
    }
    else {
        auto startUp = pending->d_setupUpdate(true, ticket, S(""));

        if (!startUp) {
            channel->closeChannel();
        }
        else {
            auto initiated = ticket->d_setupStatus == 
                                    IpcProtocol::Provider::SetupBase::eFINISH;
            auto random1   = d_prng_p->random32();
            auto random2   = d_prng_p->random32();
            auto session   = std::make_shared<IpcSession>(random1,
                                                          random2,
                                                          channel_wp,
                                                          initiated);
            auto ssnWeak   = std::weak_ptr(session);

            d_sessionSet.insert(session);
            channel->completeSetup(
                    [ssnWeak](IpcMessage&& message) -> void {
                        auto ssn = ssnWeak.lock();
                        if (ssn) {
                            ssn->receivedMessage(std::move(message));
                        }
                    },
                    [this,
                     dropMsg = std::move(startUp->d_dropIndication),
                     ssnWeak](IpcProtocol::Channel::State) mutable -> void {
                        auto ssn = ssnWeak.lock();
                        if (ssn) {
                            d_lock.lock();
                            d_sessionSet.erase(ssn);
                            d_lock.unlock();
                            ssn->receivedMessage(std::move(dropMsg));
                            ssn->setClosed();
                        }
                        ssnWeak.reset();
                    });
            session->startHandshake(std::move(startUp->d_handshakeMessage),
                                    [ssnWeak,cb = startUp->d_messageCb]
                                        (MessageType type, IpcMessage&& msg) {
                                        cb(ssnWeak, type, std::move(msg));
                                    });
        }
    }
    return;                                                           // RETURN
}

// PUBLIC MEMBERS
IpcSessionFactory::IpcSessionFactory(IpcProtocol::Provider *adapter)
: d_adapter(adapter)
, d_opened(true)
, d_prng_p(new IpcSessionFactory::CryptoPrng())
{
}

IpcSessionFactory::~IpcSessionFactory()
{
    delete d_prng_p;
}

IpcSessionFactory::SetupTicket
IpcSessionFactory::acceptChannels(String_t        *diagnostic,
                                  std::any         configuration,
                                  SetupUpdate&&    completion) noexcept
{
    auto guard   = std::unique_lock(d_lock);
    auto handle  = std::make_shared<Setup>(std::move(completion));
    auto setupCb =
        [this](auto channel, auto ticket) -> void {
            channelSetup(channel, ticket);
        };

    if (!d_adapter->acceptChannels(diagnostic,
                                   handle,
                                   configuration,
                                   std::move(setupCb))) {
        handle.reset();
    }
    return handle;                                                    // RETURN
}

bool
IpcSessionFactory::cancelSetup(const SetupTicket& handle) noexcept
{
    return d_adapter->cancelSetup(handle);                            // RETURN
}

IpcSessionFactory::SetupTicket
IpcSessionFactory::createNewChannel(String_t        *diagnostic,
                                    std::any         configuration,
                                    SetupUpdate&&    completion) noexcept
{
    auto guard   = std::unique_lock(d_lock);
    auto handle  = std::make_shared<Setup>(std::move(completion));
    auto setupCb =
        [this](auto channel, auto ticket) -> void {
            channelSetup(channel, ticket);
        };

    if (!d_adapter->createNewChannel(diagnostic,
                                     handle,
                                     configuration,
                                     std::move(setupCb))) {
        handle.reset();
    }
    return handle;                                                    // RETURN
}

void
IpcSessionFactory::shutdown() noexcept
{
    auto guard = std::unique_lock(d_lock);

    if (d_opened) {
        d_opened = false;
        d_adapter->shutdown(); // cancels all pending setups

        while (d_sessionSet.end() != d_sessionSet.begin()) {
            (*d_sessionSet.begin())->shutdown();
            d_sessionSet.erase(d_sessionSet.begin());
        }
    }
}

}  // namespace Wawt

// vim: ts=4:sw=4:et:ai
