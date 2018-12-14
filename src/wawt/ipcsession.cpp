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

#include "wawt/ipcmessage.h"

#include <cryptopp/config.h>
#include <cryptopp/osrng.h>

#include <algorithm>
#include <memory>
#include <unordered_set>

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
                  IpcSession::PeerId   *peerId,
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
                return IpcMessageUtil::extractValue(peerId, &p, end); // RETURN
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
                         IpcSession::PeerId peerId,
                         const IpcMessage&  data)
{
    auto length    = data.length();
    auto payloadsz = sizeof(peerId) + length;
    auto size      = IpcMessageUtil::prefixsz + payloadsz;
    auto message   = IpcMessage(std::make_unique<char[]>(size), size, 0);
    auto p         = message.data();
    p              = IpcMessageUtil::initPrefix(p,
                                                random1,
                                                payloadsz,
                                                IpcMessageUtil::kSTARTUP);
    p              = IpcMessageUtil::initValue(p, peerId);
    std::memcpy(p, data.cbegin(), length); 
    return message;
}

} // unnamed namespace

                    //===========================
                    // struct IpcSessionCompletor
                    //===========================

struct IpcSessionCompletor : std::enable_shared_from_this<IpcSessionCompletor>{
    using SessionSet    = std::unordered_set<std::shared_ptr<IpcSession>>;
    using MessageType   = IpcSession::MessageType;

    mutable std::mutex                           d_lock{};
    SessionSet                                   d_sessionSet{};
    CryptoPP::AutoSeededRandomPool               d_prng{};
    IpcSession::PeerId                           d_peerId{};

    // PUBLIC CONSTRUCTOR
    IpcSessionCompletor() {
        d_peerId = (uint64_t(random32()) << 32)| uint64_t(random32());
    }

    // PUBLIC MANIUPLATORS
    //! Called when a connection attempt succeeds or fails (i.e. cancelation);
    void channelSetup(const IpcProtocol::ChannelPtr&            channel_wp,
                      const IpcProtocol::Provider::SetupTicket& ticket)
                                                                     noexcept;

    uint32_t random32()                                              noexcept {
        return d_prng.GenerateWord32();
    }
    void removeSession(const std::shared_ptr<IpcSession>& session)   noexcept {
        auto guard = std::unique_lock(d_lock);
        d_sessionSet.erase(session);
    }

    void shutdown()                                                   noexcept {
        auto guard = std::unique_lock(d_lock);

        while (d_sessionSet.end() != d_sessionSet.begin()) {
            (*d_sessionSet.begin())->shutdown();
            d_sessionSet.erase(d_sessionSet.begin());
        }
    }

    // PUBLIC ACCESSORS
    IpcSession::PeerId localPeerId()                           const noexcept {
        return d_peerId;
    }

    bool peerIdInUse(IpcSession::PeerId peerId)                const noexcept {
        auto guard = std::unique_lock(d_lock);
        auto testSession =
                [peerId](const SessionSet::value_type& session) {
                    return peerId == session->peerId();
                };
        return d_sessionSet.cend() != std::find_if(d_sessionSet.cbegin(),
                                                   d_sessionSet.cend(),
                                                   testSession);
    }
};

void
IpcSessionCompletor::channelSetup(
        const IpcProtocol::ChannelPtr&             channel_wp,
        const IpcProtocol::Provider::SetupTicket&  ticket) noexcept
{
    // Note: caller discards all state tracking "ticket" on return.  So must
    // this method.
    assert(ticket);

    auto guard      = std::unique_lock(d_lock);
    auto channel    = channel_wp.lock();
    auto pending    = static_cast<IpcSessionFactory::Setup*>(ticket.get());

    if (!pending) {
        if (channel) {
            channel->closeChannel();
        }
    }
    else if (!channel) {
        pending->d_setupUpdate(false, ticket, diagnostic(ticket));
    }
    else {
        auto startUp = pending->d_setupUpdate(true, ticket, S(""));

        if (!startUp) {
            channel->closeChannel();
        }
        else {
            auto initiated = ticket->d_setupStatus == 
                                    IpcProtocol::Provider::SetupBase::eFINISH;
            auto random    = random32();
            auto session   = std::make_shared<IpcSession>(random,
                                                          channel_wp,
                                                          weak_from_this(),
                                                          initiated);
            auto ssnWeak   = std::weak_ptr<IpcSession>(session);

            d_sessionSet.insert(session);
            // Note that channel callbacks are invoked serially so no race
            // conditions (e.g. messages are delivered in order received).
            channel->completeSetup(
                    // On receipt of messages (forward to session):
                    [ssnWeak](IpcMessage&& message) -> void {
                        if (auto ssn = ssnWeak.lock()) {
                            ssn->receivedMessage(std::move(message));
                        }
                    },
                    // On disconnect (forward drop indication to session):
                    [ssnWeak](IpcProtocol::Channel::State state) mutable {
                        if (auto ssn = ssnWeak.lock()) {
                            ssn->dropIndication(state);
                        }
                    });
            session->startHandshake(session, d_peerId, std::move(startUp));
        }
    }
    return;                                                           // RETURN
}

                            //-----------------
                            // class IpcSession
                            //-----------------

// PRIVATE MEMBERS
void
IpcSession::saveStartupDigest(MessageNumber  initialValue,
                              IpcMessage&&   receivedDigest) noexcept
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
                                 PeerId                     peerId,
                                 const IpcMessage&          message) noexcept
{
    auto completor = d_completor.lock();
    // There is a very small chance that the peer ID (64 bit random value)
    // is already in use (although a hacker could have "stole" it).  But if
    // it is, just drop the connection.

    if (!completor || completor->peerIdInUse(peerId)) {
        return false;                                                 // RETURN
    }
    d_peerId = peerId;

    return digestValue == d_sendSalt
        && CryptoPP::SHA256().VerifyDigest(
                reinterpret_cast<const uint8_t*>(d_digest.cbegin()),
                reinterpret_cast<const uint8_t*>(message.cbegin()),
                message.length());     // RETURN
}

// PUBLIC MEMBERS
IpcSession::IpcSession(uint32_t      random,
                       ChannelPtr    channel,
                       CompletorPtr  completor,
                       bool          initiated)
                                                                    noexcept
: d_rcvSalt(random)
, d_channel(channel)
, d_completor(completor)
, d_initiated(initiated)
{
}

void
IpcSession::dropIndication(IpcProtocol::Channel::State) noexcept
{
    // Note: channel is not usable at this point.
    auto  guard     = std::unique_lock(d_lock);
    auto  completor = d_completor.lock();

    if (completor) {
        completor->removeSession(d_self.lock());
    }

    if (state() == State::eOPEN) {
        d_upstream->d_messageCb(d_self,
                                MessageType::eDROP,
                                std::move(d_upstream->d_dropIndication));
    }
    return;                                                           // RETURN
}

bool
IpcSession::enqueue(MessageChain&& chain, bool close) noexcept
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
IpcSession::receivedMessage(IpcMessage&& message) noexcept
{
    auto  guard     = std::unique_lock(d_lock);

    auto  salt      = uint32_t{};
    auto  peerId    = PeerId{}; // only set when kSTARTUP message is processed
    auto  type      = char{};
    auto  channel   = d_channel.lock();
    auto& messageCb = d_upstream->d_messageCb;

    assert(channel); // otherwise, who is calling this method?

    if (checkMessage(&salt, &peerId, &type, message)) {
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
          } break;                                                     // BREAK
          case State::eWAITING_ON_START: {
            if (IpcMessageUtil::kSTARTUP == type
             && verifyStartupMessage(salt, peerId, message)) {
                message.d_offset += IpcMessageUtil::prefixsz + sizeof(peerId);

                if (message.length() > 0) {
                    messageCb(d_self, MessageType::eDATA, std::move(message));
                }
                assert(state() == State::eOPEN);
            }
            else {
                close = true;
                assert(!"Not a STARTUP or failed to verify start message.");
            }
          } break;                                                     // BREAK
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
                    messageCb(d_self, msgtype, std::move(message));
                }
                else {
                    close = true;
                    assert(!"Expected message content.");
                }
            }
          } break;                                                     // BREAK
          case State::eWAITING_ON_DISC: {
          } break;                                                     // BREAK
          default: {
            assert(!"Session in invalid state.");
            close = true;
          }
        }

        if (!close) {
            return;                                                   // RETURN
        }
    }
    channel->closeChannel();
    return;                                                           // RETURN
}

void
IpcSession::shutdown() noexcept
{
    if (auto channel = d_channel.lock()) {
        d_lock.lock();
        channel->closeChannel();
        d_lock.unlock();
    }
    return;                                                           // RETURN
}

void
IpcSession::startHandshake(const SelfPtr&     self,
                           PeerId             peerId,
                           StartupPtr&&       finishSetup) noexcept
{
    assert(d_state == State::eWAITING_ON_CONNECT);
    d_upstream     = std::move(finishSetup);
    d_self         = self;

    auto channel   = d_channel.lock();
    assert(channel);
    d_state        = State::eWAITING_ON_DIGEST;
    d_handshake    = makeHandshake(d_rcvSalt,
                                   peerId,
                                   std::move(d_upstream->d_handshakeMessage));
    auto chain     = MessageChain{};
    auto hash      = CryptoPP::SHA256{};
    hash.Update(reinterpret_cast<const uint8_t*>(d_handshake.cbegin()),
                d_handshake.length());
    chain.emplace_front(makeDigest(d_rcvSalt, hash));
    channel->sendMessage(std::move(chain));
}

                            //------------------------
                            // class IpcSessionFactory
                            //------------------------

// PRIVATE MEMBERS

// PUBLIC MEMBERS
IpcSessionFactory::IpcSessionFactory(IpcProtocol::Provider *adapter)
: d_adapter(adapter)
, d_opened(true)
, d_completor(std::make_shared<IpcSessionCompletor>())
{
}

IpcSessionFactory::~IpcSessionFactory()
{
    shutdown();
}

IpcSessionFactory::SetupTicket
IpcSessionFactory::acceptChannels(String_t        *diagnostic,
                                  std::any         configuration,
                                  SetupUpdate&&    completion) noexcept
{
    auto guard   = std::unique_lock(d_lock);
    auto handle  = std::shared_ptr<Setup>();

    if (d_opened) {
        handle  = std::make_shared<Setup>(std::move(configuration),
                                          std::move(completion));

        if (!d_adapter->acceptChannels(diagnostic, handle, makeSetupCb())) {
            handle.reset();
        }
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
    auto handle  = std::shared_ptr<Setup>();

    if (d_opened) {
        handle   = std::make_shared<Setup>(std::move(configuration),
                                           std::move(completion));

        if (!d_adapter->createNewChannel(diagnostic, handle, makeSetupCb())) {
            handle.reset();
        }
    }
    return handle;                                                    // RETURN
}

IpcProtocol::Provider::SetupCb
IpcSessionFactory::makeSetupCb() noexcept
{
    auto completor = std::weak_ptr<IpcSessionCompletor>(d_completor);
    return [completor] (const IpcProtocol::ChannelPtr&             channel,
                        const IpcProtocol::Provider::SetupTicket&  ticket) {
        if (auto p = completor.lock()) {
            p->channelSetup(channel, ticket);
        }
        else if (auto ch = channel.lock()) {
            ch->closeChannel();
        }
    };                                                                // RETURN
}

IpcSession::PeerId
IpcSessionFactory::localPeerId() const noexcept
{
    return d_completor->localPeerId();
}

void
IpcSessionFactory::shutdown() noexcept
{
    auto guard = std::unique_lock(d_lock);

    if (d_opened) {
        d_opened = false;
        d_adapter->shutdown(); // cancels all pending setups
        d_completor->shutdown();
        d_completor.reset();
    }
}

}  // namespace Wawt

// vim: ts=4:sw=4:et:ai
