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

#ifndef WAWT_IPCQUEUE_H
#define WAWT_IPCQUEUE_H

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

#include "wawt/ipcprotocol.h"
#include "wawt/ipcsession.h"

namespace Wawt {

                               //===============
                               // class IpcQueue
                               //===============

class IpcQueue
{
  public:
    // PUBLIC TYPES
    struct Shutdown { };                    ///< Shutdown exception.

    using Header                = std::unique_ptr<char[]>;

    // Methods are NOT thread-safe
    class ReplyQueue {
        friend class IpcQueue;
        mutable std::weak_ptr<IpcSession>  d_session;
        bool                               d_isLocal;
        bool                               d_winner;
        mutable std::atomic_flag           d_flag;

        const IpcSession *safeGet() const;

      public:
        // PUBLIC CONSTRUCTORS

        //! Local reply queue.
        ReplyQueue();

        ReplyQueue(const std::weak_ptr<IpcSession>&   session,
                   bool                               winner);

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
            return d_isLocal;
        }
    };
    friend class ReplyQueue;

    using MessageType   = IpcSession::MessageType;
    using Indication    = std::tuple<ReplyQueue, IpcMessage, MessageType>;
    using TimerId       = uint32_t;

    static constexpr TimerId kINVALID_TIMERID = UINT32_MAX;

    // PUBLIC CONSTRUCTORS
    IpcQueue();
    ~IpcQueue();

    // PUBLIC MANIPULATORS
    IpcProtocol       *adapter() {
        return d_adapter;
    }
    bool            cancelDelayedEnqueue(TimerId id);

    TimerId         delayedLocalEnqueue(IpcMessage&&                message,
                                        std::chrono::milliseconds   delay);

    bool            localEnqueue(IpcMessage&&   message);

    void            remoteEnqueue(std::weak_ptr<IpcSession> session,
                                  MessageType               msgtype,
                                  IpcMessage&&              message);

    void            reset(); // flushes queued messages.

    Indication      waitForIndication();

  private:

    // PRIVATE TYPES
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
    void timerThread();

    // PRIVATE DATA MEMBERS

    bool                            d_opened    = false; // adapter was opened
    bool                            d_shutdown  = false;
    IpcProtocol                    *d_adapter   = nullptr;
    std::mutex                      d_lock{};
    std::condition_variable         d_signal{};
    std::deque<Indication>          d_incoming{};
    TimerId                         d_timerId   = 0;
    TimerMap                        d_timerIdMap{};
    TimerQueue                      d_timerQueue{timerCompare};
    std::condition_variable         d_timerSignal{};
    std::thread                     d_timerThread;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
