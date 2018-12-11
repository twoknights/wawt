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
    using  ChannelPtr    = IpcProtocol::Provider::ChannelPtr;

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

    IpcSession(uint32_t                  random1,
               uint32_t                  random2,
               ChannelPtr                channel,
               bool                      initiated);

    // PUBLIC MANIPULATORS
    bool enqueue(MessageChain&& chain, bool close);

    void lock() {
        d_lock.lock();
    }

    MessageNumber nextSalt() {
        return ++d_sendSalt;
    }

    void receivedMessage(IpcMessage&& message);

    void setClosed() {
        d_state = State::eWAITING_ON_DISC;
    }

    void shutdown();

    void startHandshake(IpcMessage&& handshake, MessageCb&& messageCb);

    void unlock() {
        d_lock.unlock();
    }

    // PUBLIC ACCESSORS
    State state() const {
        return d_state;
    }

    bool winner() const {
        return d_winner;
    }

  private:
    // PRIVATE MANIPULATORS
    void saveStartupDigest(MessageNumber  initialValue,
                           IpcMessage&&   receivedDigest);

    bool verifyStartupMessage(MessageNumber         digestValue,
                              uint32_t              random,
                              const IpcMessage&     message);

    // PRIVATE DATA MEMBERS
    State                           d_state     = State::eWAITING_ON_CONNECT;
    bool                            d_winner    = false;
    MessageNumber                   d_sendSalt  = 0;
    std::mutex                      d_lock{};
    MessageCb                       d_messageCb{};
    IpcMessage                      d_digest{};     // digest from downstream
    IpcMessage                      d_handshake{};  // to be sent downstream

    MessageNumber                   d_rcvSalt;
    uint32_t                        d_random;
    ChannelPtr                      d_channel;
    bool                            d_initiated;
};

                           //========================
                           // class IpcSessionFactory
                           //========================


class IpcSessionFactory {
  public:
    // PUBLIC TYPES
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

  private:
    // PRIVATE TYPES
    struct CryptoPrng;

    struct Setup : IpcProtocol::Provider::SetupBase {
        SetupUpdate             d_setupUpdate;

        Setup(SetupUpdate&& completion)
            : d_setupUpdate(std::move(completion)) { }
    };
    using SessionSet    = std::unordered_set<std::shared_ptr<IpcSession>>;

    // PRVIATE MANIPULATORS
    //! Called on connection success or failure (i.e. cancelation);
    void channelSetup(IpcProtocol::Provider::ChannelPtr  channel,
                      IpcProtocol::Provider::SetupTicket ticket) noexcept;

    //! Called when connection is lost.
    void sessionClose(SessionSet::value_type       session,
                      IpcProtocol::Channel::State  reason) noexcept;

    // PRIVATE DATA MEMBERS
    std::mutex                      d_lock{};
    SessionSet                      d_sessionSet{};
    IpcProtocol::Provider          *d_adapter;
    bool                            d_opened;
    CryptoPrng                     *d_prng_p;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
