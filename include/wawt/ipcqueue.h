/** @file ipcqueue.h
 *  @brief Queue based inter-process communications.
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

#ifndef WAWT_IPCPROTOCOL_H
#define WAWT_IPCPROTOCOL_H

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

#include "ipcprotocol.h"

namespace Wawt {

                               //===============
                               // class IpcQueue
                               //===============


class IpcQueue
{
  public:
    // PUBLIC TYPES
    class  Session;

    struct Shutdown { };                    ///< Shutdown exception.

    using Header                = std::unique_ptr<char[]>;

    using   SessionId = uint32_t;
    static constexpr SessionId kLOCAL_SSNID = UINT32_MAX;

    // Methods are NOT thread-safe
    class ReplyQueue {
        friend class IpcQueue;
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
        // channel not dropped until peer echos back the close message.
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
          eDROP
        , eDIGEST
        , eDATA
        , eDIGESTED_DATA
    };
    using MessageNumber = uint32_t;
    using Indication    = std::tuple<ReplyQueue, IpcMessage, MessageType>;
    using TimerId       = uint32_t;

    static constexpr TimerId kINVALID_TIMERID = UINT32_MAX;

    // PUBLIC CONSTRUCTORS
    IpcQueue(IpcProtocol *adapter);
    ~IpcQueue();

    // PUBLIC MANIPULATORS
    IpcProtocol       *adapter() {
        return d_adapter;
    }
    bool            cancelDelayedEnqueue(TimerId id);

    TimerId         delayedLocalEnqueue(IpcMessage&&                message,
                                        std::chrono::milliseconds   delay);

    bool            localEnqueue(IpcMessage&&   message);
    
    bool            openAdapter(String_t     *diagnostics,
                                IpcMessage    dropMessage,
                                IpcMessage    handshakeMessage = IpcMessage());

    void            reset(); // closes adapter too.

    Indication      waitForIndication();

  private:
    // PRIVATE TYPES
    struct CryptoPrng;

    using SessionMap    = std::unordered_map<SessionId,
                                             std::shared_ptr<Session>>;
    using TimerMap      = std::unordered_map<TimerId, IpcMessage>;
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
    void channelUpdate(IpcProtocol::ChannelId        id,
                       IpcProtocol::ChannelChange    status,
                       IpcProtocol::ChannelMode      mode);
    void processMessage(IpcProtocol::ChannelId id, IpcMessage&& message);

    void timerThread();

    // PRIVATE DATA MEMBERS

    CryptoPrng                     *d_prng_p    = nullptr;
    bool                            d_opened    = false; // adapter was opened
    bool                            d_shutdown  = false;
    IpcProtocol                    *d_adapter   = nullptr;
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
    IpcMessage                      d_dropMessage{};
    IpcProtocol::ChannelId          d_startupId{};
};

                           //===================
                           // class IpcUtilities
                           //===================

struct IpcUtilities
{
    using string_view   = std::string_view;
    using MessageNumber = IpcQueue::MessageNumber;

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
IpcMessage
IpcUtilities::formatMessage(const char *format, Args&&... args)
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
IpcUtilities::parseMessage(const IpcMessage&        message,
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
