/** @file ipcsession.h
 *  @brief IpcProtocol based sessions
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

#ifndef WAWT_IPCSESSION_H
#define WAWT_IPCSESSION_H

#include <cassert>
#include <cstdint>
#include <memory>
#include <mutex>

#include "wawt/ipcprotocol.h"

namespace Wawt {

struct IpcSessionHelper;

                                //=================
                                // class IpcSession
                                //=================

class IpcSession {
  public:
    // PUBLIC TYPES
    using  MessageNumber = IpcMessage::MessageNumber;
    using  MessageChain  = IpcProtocol::Channel::MessageChain;
    using  ChannelPtr    = IpcProtocol::ChannelPtr;
    using  HelperPtr     = std::weak_ptr<IpcSessionHelper>;
    using  SelfPtr       = std::weak_ptr<IpcSession>;
    using  PeerId        = uint64_t;

    enum class State {
          eWAITING_ON_CONNECT
        , eWAITING_ON_DIGEST
        , eWAITING_ON_START
        , eOPEN
        , eWAITING_ON_DISC
    };

    enum class MessageType { eDROP
                           , eDIGEST
                           , eDATA
                           , eDIGESTED_DATA };

    using MessageCb   = std::function<void(const SelfPtr&,
                                           MessageType,
                                           IpcMessage&&)>;
    // PUBLIC CONSTANTS

    // PUBLIC CONSTRUCTORS
    IpcSession(const IpcSession&)                 = delete;
    IpcSession& operator=(const IpcSession&)      = delete;

    IpcSession(uint32_t                  random,
               ChannelPtr                channel,
               HelperPtr                 completor,
               bool                      initiated)                  noexcept;

    // PUBLIC MANIPULATORS
    void dropIndication(IpcProtocol::Channel::State state)           noexcept;

    bool enqueue(MessageChain&& chain, bool close)                   noexcept;

    void lock()                                                      noexcept {
        d_lock.lock();
    }

    MessageNumber nextSalt()                                         noexcept {
        return ++d_sendSalt;
    }

    void receivedMessage(IpcMessage&& message)                       noexcept;

    void setClosed()                                                 noexcept {
        d_state = State::eWAITING_ON_DISC;
    }

    void shutdown()                                                  noexcept;

    void startHandshake(const SelfPtr& self, PeerId peerId)          noexcept;

    void unlock()                                                    noexcept {
        d_lock.unlock();
    }

    // PUBLIC ACCESSORS
    State state()                                              const noexcept {
        return d_state;
    }

    PeerId peerId()                                            const noexcept {
        return d_peerId;
    }

  private:
    friend struct IpcSessionHelper;
    // PRIVATE TYPES

    // PRIVATE MANIPULATORS
    void saveStartupDigest(MessageNumber  initialValue,
                           IpcMessage&&   receivedDigest)            noexcept;

    bool verifyStartupMessage(MessageNumber         digestValue,
                              PeerId                peerId,
                              const IpcMessage&     message)         noexcept;

    // PRIVATE DATA MEMBERS
    State                           d_state     = State::eWAITING_ON_CONNECT;
    MessageNumber                   d_sendSalt  = 0;
    std::mutex                      d_lock{};
    MessageCb                       d_messageCb{};
    IpcMessage                      d_dropIndication{}; // to be sent upstream
    IpcMessage                      d_handshake{};  // to be sent downstream
    IpcMessage                      d_digest{};     // digest from downstream

    SelfPtr                         d_self{};
    PeerId                          d_peerId{};

    MessageNumber                   d_rcvSalt;
    ChannelPtr                      d_channel;
    HelperPtr                       d_completor;
    bool                            d_initiated;
};

                           //========================
                           // class IpcSessionFactory
                           //========================


class IpcSessionFactory {
  public:
    // PUBLIC TYPES
    using BaseTicket  = IpcProtocol::Provider::SetupTicket;
    using PeerId      = IpcSession::PeerId;
    using SessionPtr  = std::weak_ptr<IpcSession>;
    using MessageType = IpcSession::MessageType;
    using MessageCb   = std::function<void(const SessionPtr&,
                                           MessageType,
                                           IpcMessage&&)>;

    using SetupUpdate = std::function<bool(MessageCb*,
                                           IpcMessage* drop,
                                           IpcMessage* handshake,
                                           const BaseTicket&,
                                           const String_t&)>;

    // PUBLIC CONSTRUCTORS
    IpcSessionFactory(IpcProtocol::Provider *adapter);

    // PUBLIC DESTRUCTORS
    ~IpcSessionFactory();

    // PUBLIC MANIPULATORS
    bool            cancelSetup(const BaseTicket& handle)             noexcept;

    BaseTicket      channelSetup(String_t        *diagnostic,
                                 bool             acceptConfiguration,
                                 std::any         configuration,
                                 SetupUpdate&&    completion)         noexcept;

    void            shutdown()                                        noexcept;

    // PUBLIC ACCESSORS
    PeerId          localPeerId()                               const noexcept;

  private:
    friend struct IpcSessionHelper;

    // PRIVATE TYPES
    using HelperPtr    = std::shared_ptr<IpcSessionHelper>;
    using SetupBase    = IpcProtocol::Provider::SetupBase;
    using SetupCb      = IpcProtocol::Provider::SetupCb;

    struct Setup : SetupBase {
        SetupUpdate             d_setupUpdate;

        Setup(std::any&& configuration, SetupUpdate&& completion)
            : SetupBase(std::move(configuration))
            , d_setupUpdate(std::move(completion)) { }
    };

    // PRVIATE MANIPULATORS

    // PRIVATE DATA MEMBERS
    std::mutex                      d_lock{};
    bool                            d_shutdown = false;

    IpcProtocol::Provider          *d_adapter;
    HelperPtr                       d_completor;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
