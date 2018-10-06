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
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <deque>
#include <forward_list>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include "wawt.h"

namespace BDS {

struct WawtIpcMessage {
    // PUBLIC DATA MEMBERS
    std::unique_ptr<char[]>     d_data{};
    uint16_t                    d_size      = 0;
    uint16_t                    d_offset    = 0;

    // PUBLIC CONSTRUCTORS
    WawtIpcMessage()                            = default;
    WawtIpcMessage(WawtIpcMessage&&)            = default;

    WawtIpcMessage(std::unique_ptr<char[]>&&    data,
                   uint16_t                     size,
                   uint16_t                     offset);
    WawtIpcMessage(const std::string_view& data);
    WawtIpcMessage(const char* data, uint16_t length);
    WawtIpcMessage(const WawtIpcMessage& copy);

    // PUBLIC MANIPULATORS
    WawtIpcMessage& operator=(WawtIpcMessage&&) = default;
    WawtIpcMessage& operator=(const WawtIpcMessage& rhs);

    void        reset()                           noexcept {
        d_data.reset();
        d_size = d_offset = 0;
    }

    char  *data()                                 noexcept {
        return d_data.get() + d_offset;
    }

    // PUBLIC ACCESSORS
    const char  *cbegin()                   const noexcept {
        return d_data.get() + d_offset;
    }

    bool         empty()                    const noexcept {
        return length() == 0;
    }

    const char  *cend()                     const noexcept {
        return d_data.get() + d_size;
    }

    uint16_t        length()                const noexcept {
        return d_size - d_offset;
    }

    explicit operator std::string_view()    const noexcept {
        return std::string_view(cbegin(), length());
    }
};

WawtIpcMessage::WawtIpcMessage(std::unique_ptr<char[]>&&    data,
                               uint16_t                     size,
                               uint16_t                     offset)
: d_data(std::move(data))
, d_size(size)
, d_offset(offset)
{
}

WawtIpcMessage::WawtIpcMessage(const std::string_view& data)
: d_data(std::make_unique<char[]>(data.size()))
, d_size(data.size())
{
    data.copy(d_data.get(), d_size);
}

WawtIpcMessage::WawtIpcMessage(const char* data, uint16_t length)
: d_data(std::make_unique<char[]>(length))
, d_size(length)
{
    std::memcpy(d_data.get(), data, length);
}

WawtIpcMessage::WawtIpcMessage(const WawtIpcMessage& copy)
: d_data(std::make_unique<char[]>(copy.d_size))
, d_size(copy.d_size)
, d_offset(copy.d_offset)
{
    std::memcpy(d_data.get(), copy.d_data.get(), d_size);
}

WawtIpcMessage& WawtIpcMessage::operator=(const WawtIpcMessage& rhs)
{
    if (this != &rhs) {
        d_data      = std::make_unique<char[]>(rhs.d_size);
        d_size      = rhs.d_size;
        d_offset    = rhs.d_offset;
        std::memcpy(d_data.get(), rhs.d_data.get(), d_size);
    }
    return *this;                                                     // RETURN
}


                        //================================
                        // class WawtIpcConnectionProtocol
                        //================================

/**
 * @brief An interface whose providers support communication between tasks.
 */

class WawtIpcConnectionProtocol
{
  public:
    // PUBLIC TYPES
    enum class ConfigureStatus { eOK, eMALFORMED, eINVALID, eUNKNOWN };
    enum class ConnectStatus   { eOK, eDROP, eCLOSE, eERROR, ePROTOCOL };
    enum class ConnectRole     { eINITIATOR, eACCEPTOR, eUNUSED };

    using IpcMessage        = WawtIpcMessage;

    using ConnectionId      = uint32_t;

    using MessageChain      = std::forward_list<IpcMessage>;

    // Calling thread must not hold locks.
    using ConnectCb         = std::function<void(ConnectionId,
                                                 ConnectStatus,
                                                 ConnectRole)>;

    // Calling thread must not hold locks.
    using MessageCb         = std::function<void(ConnectionId, IpcMessage&&)>;

    static constexpr ConnectionId kINVALID_ID = UINT32_MAX;

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
    class  Session;

    struct Shutdown { };                    ///< Shutdown exception.

    using IpcConnectionProtocol = WawtIpcConnectionProtocol;
    using IpcMessage            = WawtIpcMessage;
    using Protocol              = IpcConnectionProtocol;
    using Header                = std::unique_ptr<char[]>;

    using   SessionId = Protocol::ConnectionId;
    static constexpr SessionId kLOCAL_SSNID = Protocol::kINVALID_ID;

    // Methods are NOT thread-safe
    class ReplyQueue {
        friend class WawtIpcQueue;
        mutable std::weak_ptr<Session>  d_session;
        bool                            d_winner;
        SessionId                       d_sessionId;
        mutable std::atomic_flag        d_flag;

        const Session *safeGet() const;

      public:
        // PUBLIC CONSTRUCTORS
        ReplyQueue(const std::shared_ptr<Session>&      session,
                   bool                                 winner,
                   int                                  sessionId);

        ReplyQueue(ReplyQueue&&         copy);

        ReplyQueue(const ReplyQueue&    copy);

        ReplyQueue& operator=(ReplyQueue&& rhs);

        ReplyQueue& operator=(const ReplyQueue& rhs);

        // PUBLIC MANIPULATORS
        bool            enqueue(IpcMessage&&            message,
                                Header&&                header = Header())
                                                                      noexcept;

        bool            enqueueDigest(Header           *header,
                                      const IpcMessage& message)      noexcept;

        // Can no longer enqueue or dequeue messages on session.
        // connection not dropped until peer echos back the close message.
        void            closeQueue()                                  noexcept;

        // PUBLIC ACCESSORS
        bool            tossResult()                            const noexcept{
            return d_winner;
        }

        bool            isClosed()                              const noexcept;

        bool            isLocal()                               const noexcept{
            return d_sessionId == kLOCAL_SSNID;
        }

        bool            operator==(const ReplyQueue&    rhs)    const noexcept{
            return d_sessionId == rhs.d_sessionId;
        }

        bool            operator!=(const ReplyQueue&    rhs)    const noexcept{
            return d_sessionId != rhs.d_sessionId;
        }
    };
    friend class ReplyQueue;

    enum class MessageType {
          eDISCONNECT
        , eDIGEST
        , eDATA
        , eDIGESTED_DATA
    };
    using MessageNumber = uint32_t;
    using Indication    = std::tuple<ReplyQueue, IpcMessage, MessageType>;
    using TimerId       = uint32_t;

    static constexpr TimerId kINVALID_TIMERID = UINT32_MAX;

    // PUBLIC CONSTRUCTORS
    WawtIpcQueue(Protocol *adapter);
    ~WawtIpcQueue();

    // PUBLIC MANIPULATORS
    Protocol       *adapter() {
        return d_adapter;
    }
    bool            cancelDelayedEnqueue(TimerId id);

    TimerId         delayedLocalEnqueue(IpcMessage&&                message,
                                        std::chrono::milliseconds   delay);

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
    using TimerMap      = std::unordered_map<TimerId, WawtIpcMessage>;
    using Clock         = std::chrono::high_resolution_clock;
    using TimerPair     = std::pair<Clock::time_point,TimerId>;

    // PRIVATE CLASS MEMBERS
    static constexpr bool timerCompare(const TimerPair& lhs,
                                       const TimerPair& rhs) {
        return lhs.first > rhs.first;
    }

    using TimerQueue    = std::priority_queue<TimerPair,
                                              std::deque<TimerPair>,
                                              decltype(&timerCompare)>;

    // PRIVATE MANIPULATORS
    void connectionUpdate(Protocol::ConnectionId  id,
                          Protocol::ConnectStatus status,
                          Protocol::ConnectRole   role);
    void processMessage(Protocol::ConnectionId id, IpcMessage&& message);

    void timerThread();

    // PRIVATE DATA MEMBERS

    CryptoPrng                     *d_prng_p    = nullptr;
    bool                            d_opened    = false; // adapter was opened
    bool                            d_shutdown  = false;
    std::mutex                      d_lock{};
    std::condition_variable         d_signal{};
    std::deque<Indication>          d_incoming{};
    SessionMap                      d_sessionMap{};
    TimerId                         d_timerId   = 0;
    TimerMap                        d_timerIdMap{};
    TimerQueue                      d_timerQueue{timerCompare};
    std::condition_variable         d_timerSignal{};
    std::thread                     d_timerThread;
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
    using MessageNumber = WawtIpcQueue::MessageNumber;
    using IpcMessage    = WawtIpcMessage;

    template<typename... Args>
    IpcMessage      formatMessage(const char *format, Args&&... args);

    template<typename... Args>
    bool            parseMessage(const IpcMessage&  message,
                                 const char        *format,
                                 Args&&...          args);

    MessageNumber   messageNumber(const IpcMessage& message);

    bool            verifyDigestPair(const IpcMessage&    digest,
                                     const IpcMessage&    digestMessage);
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
        if constexpr(NUMARGS == 0) {
            return !std::strncmp(message.cbegin(), format, message.length());
        }
        else {
            return NUMARGS == std::sscanf(message.cbegin(),
                                          format,
                                          std::forward<Args>(args)...);
        }
    }
    return false;
}

} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai
