/** @file wawtipcprotocol.h
 *  @brief Abstract interface for inter-process communication.
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

#ifndef BDS_WAWTIPCPROTOCOL_H
#define BDS_WAWTIPCPROTOCOL_H

#include <any>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "wawt.h"

namespace BDS {

struct WawtIpcMessage {
    WawtIpcMessage()                                        = default;
    WawtIpcMessage(WawtIpcMessage&&)                        = default;
    WawtIpcMessage& operator=(WawtIpcMessage&&)             = default;

    std::unique_ptr<uint8_t[]>  d_data{};
    uint16_t                    d_size      = 0;
    uint16_t                    d_offset    = 0;

    WawtIpcMessage(const std::string_view& data)
        : d_data(std::make_unique<uint8_t[]>(data.size())
        , d_size(data.size()) {
        data.copy(d_data.get(), d_size);
    }

    WawtIpcMessage(const WawtIpcMessage& copy)
        : d_data(std::make_unique<uint8_t[]>(copy.d_size)
        , d_size(copy.d_size)
        , d_offset(copy.d_offset) {
        std::memcpy(d_data.get(), copy.d_data.get(), d_size);
    }

    WawtIpcMessage& operator=(const WawtIpcMessage& rhs) {
        if (this != &rhs) {
            d_data      = std::make_unique<uint8_t[]>(rhs.d_size);
            d_size      = rhs.d_size;
            d_offset    = rhs.d_offset;
            std::memcpy(d_data.get(), rhs.d_data.get(), d_size);
        }
    }

    void        reset() {
        d_data.reset();
        d_size = d_offset = 0;
    }

    uint16_t    length() const noexcept {
        return d_size - d_offset;
    }

    explicit operator std::string_view() const noexcept {
        return std::string_view(d_data.get()+d_offset, d_size-d_offset);
    }
};


                        //================================
                        // class WawtIpcConnectionProtocol
                        //================================

/**
 * @brief An interface whose providers support communication between tasks.
 */

class WawtIpcConnectionProtocol {
  public:
    // PUBLIC TYPES
    enum class ConfigureStatus { eOK, eMALFORMED, eINVALID, eUNKNOWN };
    enum class ConnectStatus   { eOK, eDROP, eCLOSE, eERROR, ePROTOCOL };
    enum class ConnectRole     { eCREATOR, eACCEPTOR, eUNUSED };

    using IpcMessage        = WawtIpcMessage;

    using ConnectionId      = uint32_t;

    using MessageChain      = std::forward<IpcMessage>;

    using ConnectCb         = std::function<void(ConnectionId,
                                                 ConnectStatus,
                                                 ConnectRole)>;

    using MessageCb         = std::function<void(ConnectionId, IpcMessage&&)>;

    constexpr static ConnectionId kINVALID_ID = UINT32_MAX;

    // Drop new connections until next call to 'configureAdapter'
    virtual void            dropNewConnections()                            =0;

    // Asynchronous close of all connections. No new ones permitted.
    virtual void            closeAdapter()                          noexcept=0;

    //! Asynchronous close of connection.
    virtual void            closeConnection(ConnectionId    id)     noexcept=0;

    //! Synchronous call. If adapter permits, may be called more than once.
    virtual ConfigureStatus configureAdapter(Wawt::String_t *diagnostic,
                                             std::any        configuration)
                                                                    noexcept=0;

    virtual void            installCallbacks(ConnectCb       connectionUpdate,
                                             MessageCb       receivedMessage)
                                                                    noexcept=0;

    //! Enables the asynchronous creation of new connections.
    virtual bool            openAdapter(Wawt::String_t *diagnostic) noexcept=0;

    //! Asynchronous call to send a message on a connection
    virtual bool            sendMessage(ConnectionId        id,
                                        MessageChain&&      chain)  noexcept=0;
};


                               //===================
                               // class WawtIpcQueue
                               //===================


class WawtIpcQueue
{
  public:
    // PUBLIC TYPES
    struct  Session;

    struct Shutdown { };                    ///< Shutdown exception.

    using IpcConnectionProtocol = WawtIpcConnectionProtocol;
    using IpcMessage            = WawtIpcMessage;
    using Protocol              = IpcConnectionProtocol;
    using Signature             = std::unique_ptr<uint8_t[]>;

    using   SessionId = Protocol::ConnectionId;
    static constexpr static SessionId kLOCAL_SSNID = Protocol::kINVALID_ID;

    // Methods are NOT thread-safe
    class ReplyQueue {
        friend class WawtIpcQueue;
        std::shared_ptr<Session>    d_session;
        bool                        d_winner;
        SessionId                   d_sessionId;
        std::atomic_flag            d_flag;

        const Session *safeGet() const;

      public:
        // PUBLIC CONSTRUCTORS
        ReplyQueue(std::shared_ptr<Session>             session,
                   bool                                 winner,
                   int                                  sessionId);

        // PUBLIC MANIPULATORS
        bool            enqueue(IpcMessage&&            message,
                                Signature&&             signature= Signature())
                                                                      noexcept;

        bool            enqueueDigest(Signature        *signature,
                                      const IpcMessage& message)      noexcept;

        // Can no longer enqueue or dequeue messages on session.
        // connection not dropped until peer echos back the close message.
        void            closeQueue(IpcMessage&&         message = IpcMessage())
                                                                      noexcept;

        // PUBLIC ACCESSORS
        bool            tossResult() const noexcept {
            return d_winner;
        }

        bool            isClosed() const noexcept {
            return safeGet() == nullptr;
        }

        bool            isLocal() const noexcept {
            return d_sessionId == kLOCAL_SSNID;
        }

        bool            operator==(const ReplyQueue&    rhs) const noexcept {
            return d_sessionId == rhs.d_sessionId;
        }

        bool            operator!=(const ReplyQueue&    rhs) const noexcept {
            return d_sessionId != rhs.d_sessionId;
        }
    };
    friend class ReplyQueue;

    using MessageNumber = uint32_t;
    using Indication    = std::tuple<ReplyQueue, IpcMessage, MessageType>;

    // PUBLIC CONSTRUCTORS
    WawtIpcQueue(Protocol *adapter);

    // PUBLIC MANIPULATORS
    Protocol       *adapter() {
        return d_adapter;
    }

    bool            localEnqueue(IpcMessage&& message);
    
    bool            openAdapter(Wawt::String_t         *diagnostics,
                                IpcMessage              disconnectMessage,
                                IpcMessage              handshakeMessage
                                                               = IpcMessage());

    void            reset(); // closes adapter too.

    Indication      waitForIndication();

  private:
    // PRIVATE TYPES
    struct CryptoPrng;

    using SessionMap    = std::unordered_map<Protocol::ConnectionId,
                                             std::shared_ptr<Session>>;

    // PRIVATE MANIPULATORS
    void connectionUpdate(Protocol::ConnectionId  id,
                          Protocol::ConnectStatus status,
                          Protocol::ConnectRole   role);

    void lock()     { d_mutex.lock(); }

    void processMessage(ConnectionId id, IpcMessage&& message);

    bool try_lock() { return d_mutex.try_lock(); }

    void unlock()   { d_mutex.unlock(); }

    // PRIVATE DATA MEMBERS
    std::unique_ptr<CryptoPrng>     d_prng_p{};
    bool                            d_opened    = false; // adapter was opened
    bool                            d_shutdown  = false;
    std::mutex                      d_mutex{};
    std::condition_variable         d_signal{};
    std::deque<Indication>          d_incoming{};
    SessionMap                      d_sessionMap{};
    IpcMessage                      d_handshake{};
    IpcMessage                      d_disconnect{};
    Protocol::ConnectionId          d_startupId = Protocol::kINVALID_ID;
    Protocol                       *d_adapter;
};

                           //=======================
                           // class WawtIpcUtilities
                           //=======================

struct WawtIpcUtilities
{
    using string_view   = std::string_view;
    using MessageNumber = WawtIpcQueue::MessageNumber
    using IpcMessage    = WawtIpcMessage;

    template<typename... Args>
    IpcMessage      formatMessage(const char *format, Args&&... args);

    template<typename... Args>
    bool            parseMessage(const Message&     message,
                                 const char        *format,
                                 Args...            args);

    MessageNumber   messageNumber(const IpcMessage& message);

    bool            verifySignedMessage(const IpcMessage&    digest,
                                        const IpcMessage&    signedMessage);
};

template<typename... Args>
WawtIpcMessage
WawtIpcUtilities::formatMessage(const char *format, Args&&... args)
{
    constexpr int NUMARGS = sizeof...(Args);

    if constexpr(NUMARGS == 0) {
        auto size   = std::strlen(format) + 1;
        auto buffer = std::make_unique<char[]>(size);
        std::memcpy(buffer.get(), format, size);
        return {std::move(buffer), size, 0};
    }
    else {
        auto size   = std::snprintf(nullptr, 0, format,
                                             std::forward<Args>(args)...) + 1;
        auto buffer = std::make_unique<char[]>(size);
        std::snprintf(buffer.get(), size, format, std::forward<Args>(args)...);
        return {std::move(buffer), size, 0};
    }
}

template<typename... Args>
inline bool
WawtIpcUtilities::parseMessage(const IpcMessage&        message,
                               const char              *format,
                               Args&&...                args)
{
    // A received message always consists of one message fragment.
    // It should also have a EOS at the end (see 'formatMessage').
    constexpr int NUMARGS = sizeof...(Args);

    if (!message.empty()) {
        auto& [buffer, offset, size] = message.front();

        if constexpr(NUMARGS == 0) {
            auto length = size - offset;
            return 0 == std::strncmp(buffer.get() + offset, format, length);
        }
        else {
            return NUMARGS == std::sscanf(buffer.get() + offset,
                                          format,
                                          std::forward<Args>(args)...);
        }
    }
    return false;
}

} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai
