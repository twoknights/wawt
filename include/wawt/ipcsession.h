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
#include <unordered_map>

#include "wawt/ipcprotocol.h"

namespace Wawt {

                                //=================
                                // class IpcSession
                                //=================

class IpcSession {
  public:
    // PUBLIC TYPES
    using  Id            = uint32_t;
    using  MessageNumber = IpcMessage::MessageNumber;
    using  MessageChain  = IpcProtocol::MessageChain;

    enum class State {
          eWAITING_ON_CONNECT
        , eWAITING_ON_DIGEST
        , eWAITING_ON_START
        , eOPEN
        , eWAITING_ON_DISC
    };

    // PUBLIC CONSTANTS
    static constexpr Id kLOCAL_SSNID = UINT32_MAX;

    // PUBLIC CONSTRUCTORS
    IpcSession(const IpcSession&)                 = delete;
    IpcSession& operator=(const IpcSession&)      = delete;

    IpcSession(const IpcMessage&         handshake,
               uint32_t                  random1,
               uint32_t                  random2,
               IpcProtocol              *adapter,
               IpcProtocol::ChannelId    id,
               IpcProtocol::ChannelMode  mode);

    // PUBLIC MANIPULATORS
    MessageNumber nextSalt() {
        return ++d_sendSalt;
    }

    bool enqueue(MessageChain&& chain, bool close) {
        auto ret = d_state == State::eOPEN
                && d_adapter->sendMessage(d_channelId, std::move(chain));
        if (ret && close) {
            d_state = State::eWAITING_ON_DISC;
        }
        return ret;
    }

    void lock() {
        d_lock.lock();
    }

    MessageChain getStartupDigest();

    void setClosed() {
        d_state = State::eWAITING_ON_DISC;
    }

    MessageChain saveStartupDigest(MessageNumber  initialValue,
                                   IpcMessage&&   receivedDigest);

    bool verifyStartupMessage(MessageNumber         digestValue,
                              uint32_t              random,
                              const IpcMessage&     message);

    void unlock() {
        d_lock.unlock();
    }

    // PUBLIC ACCESSORS
    Id    sessionId() const {
        return d_channelId;
    }

    State state() const {
        return d_state;
    }

    bool winner() const {
        return d_winner;
    }

  private:
    // PRIVATE DATA MEMBERS
    State                           d_state     = State::eWAITING_ON_CONNECT;
    bool                            d_winner    = false;
    MessageNumber                   d_sendSalt  = 0;
    std::mutex                      d_lock{};
    IpcMessage                      d_digest{};     // digest from downstream
    IpcMessage                      d_handshake;    // handshake from upstream
    MessageNumber                   d_rcvSalt;
    IpcProtocol                    *d_adapter;
    IpcProtocol::ChannelId          d_channelId;
    uint32_t                        d_random;
    IpcProtocol::ChannelMode        d_mode;
};

                           //========================
                           // class IpcSessionManager
                           //========================


class IpcSessionManager {
  public:
    // PUBLIC TYPES
    enum class MessageType {
          eDROP
        , eDIGEST
        , eDATA
        , eDIGESTED_DATA
    };

    using MessageCb = std::function<void(std::shared_ptr<IpcSession> session,
                                         MessageType                 msgtype,
                                         IpcMessage&&                message)>;

    // PUBLIC CONSTRUCTORS
    IpcSessionManager(IpcProtocol *adapter, MessageCb&&  messageCb);

    // PUBLIC DESTRUCTORS
    ~IpcSessionManager();

    // PUBLIC MANIPULATORS
    bool            handShakes(String_t     *diagnostics,
                               IpcMessage    dropMessage,
                               IpcMessage    handshakeMessage = IpcMessage());

    void            reset();

  private:
    // PRIVATE TYPES
    struct CryptoPrng;
    using SessionMap    = std::unordered_map<IpcProtocol::ChannelId,
                                             std::shared_ptr<IpcSession>,
                                             IpcProtocol::ChannelIdHash>;

    // PRVIATE MANIPULATORS
    void channelUpdate(IpcProtocol::ChannelId      id,
                       IpcProtocol::ChannelChange  changeTo,
                       IpcProtocol::ChannelMode    mode);

    void processMessage(IpcProtocol::ChannelId id, IpcMessage&& message);

    bool verifyStartupMessage(IpcMessage::MessageNumber  digestValue,
                              uint32_t                   random,
                              const IpcMessage&          message);

    // PRIVATE DATA MEMBERS
    CryptoPrng                     *d_prng_p    = nullptr;
    bool                            d_opened    = false;
    std::mutex                      d_lock{};
    IpcMessage                      d_handshake{};
    IpcMessage                      d_dropMessage{};
    SessionMap                      d_sessionMap{};
    IpcProtocol                    *d_adapter;
    MessageCb                       d_messageCb;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
