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
#include <unordered_set>

#include "wawt/ipcprotocol.h"

namespace Wawt {

                                //=================
                                // class IpcSession
                                //=================

class IpcSession {
  public:
    // PUBLIC TYPES
    using  MessageNumber = IpcMessage::MessageNumber;
    using  MessageChain  = IpcProtocol::Channel::MessageChain;
    using  ChannelPtr    = IpcProtocol::ChannelPtr;
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

    using MessageCb = std::function<void(MessageType               msgtype,
                                         IpcMessage&&              message)>;

    // PUBLIC CONSTANTS

    // PUBLIC CONSTRUCTORS
    IpcSession(const IpcSession&)                 = delete;
    IpcSession& operator=(const IpcSession&)      = delete;

    IpcSession(uint32_t                  random,
               ChannelPtr                channel,
               bool                      initiated)                  noexcept;

    // PUBLIC MANIPULATORS
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

    void startHandshake(PeerId        peerId,
                        IpcMessage&&  handshake,
                        MessageCb&&   messageCb)                     noexcept;

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
    IpcMessage                      d_digest{};     // digest from downstream
    IpcMessage                      d_handshake{};  // to be sent downstream

    MessageNumber                   d_rcvSalt;
    uint32_t                        d_random;
    PeerId                          d_peerId;
    ChannelPtr                      d_channel;
    bool                            d_initiated;
};

                           //========================
                           // class IpcSessionFactory
                           //========================


class IpcSessionFactory {
  public:
    // PUBLIC TYPES
    using PeerId      = IpcSession::PeerId;
    using SessionPtr  = std::weak_ptr<IpcSession>;
    using MessageType = IpcSession::MessageType;
    using MessageCb   = std::function<void(const SessionPtr&,
                                           MessageType,
                                           IpcMessage&&)>;

    struct SessionStartup {
        MessageCb   d_messageCb;
        IpcMessage  d_dropIndication;
        IpcMessage  d_handshakeMessage;
    };
    using StartupPtr  = std::unique_ptr<SessionStartup>;
    using SetupTicket = IpcProtocol::Provider::SetupTicket;
    using SetupUpdate = std::function<StartupPtr(bool success,
                                                 SetupTicket,
                                                 const String_t&)>;

    // PUBLIC CONSTRUCTORS
    IpcSessionFactory(IpcProtocol::Provider *adapter);

    // PUBLIC DESTRUCTORS
    ~IpcSessionFactory();

    // PUBLIC MANIPULATORS
    SetupTicket     acceptChannels(String_t        *diagnostic,
                                   std::any         configuration,
                                   SetupUpdate&&    completion)       noexcept;

    bool            cancelSetup(const SetupTicket& handle)            noexcept;

    SetupTicket     createNewChannel(String_t        *diagnostic,
                                     std::any         configuration,
                                     SetupUpdate&&    completion)     noexcept;

    void            shutdown()                                        noexcept;

    // PUBLIC ACCESSORS
    PeerId          peerId()                                    const noexcept;

  private:
    // PRIVATE TYPES
    using SetupBase = IpcProtocol::Provider::SetupBase;
    using SetupCb   = IpcProtocol::Provider::SetupCb;

    struct Completor;

    struct Setup : SetupBase {
        SetupUpdate             d_setupUpdate;

        Setup(std::any&& configuration, SetupUpdate&& completion)
            : SetupBase(std::move(configuration))
            , d_setupUpdate(std::move(completion)) { }
    };

    // PRVIATE MANIPULATORS
    SetupCb         makeSetupCb();

    // PRIVATE DATA MEMBERS
    std::mutex                      d_lock{};
    IpcProtocol::Provider          *d_adapter;
    bool                            d_opened;
    std::shared_ptr<Completor>      d_completor;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
