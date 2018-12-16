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
    using PeerId                = IpcSession::PeerId;

    // Methods are NOT thread-safe
    class ReplyQueue {
        friend class IpcQueue;
        mutable std::weak_ptr<IpcSession>  d_session;
        bool                               d_isLocal;
        PeerId                             d_peerId;
        mutable std::atomic_flag           d_flag;

        const IpcSession *safeGet() const;

      public:
        // PUBLIC CONSTRUCTORS

        //! Local reply queue.
        ReplyQueue();

        ReplyQueue(const std::weak_ptr<IpcSession>&   session,
                   IpcSession::PeerId                 peerId);

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
        bool            isClosed()                              const noexcept;

        bool            isLocal()                               const noexcept{
            return d_isLocal;
        }

        PeerId          peerId()                                const noexcept{
            return d_peerId;
        }
    };
    friend class ReplyQueue;

    using MessageType   = IpcSession::MessageType;
    using HandlePtr     = IpcSessionFactory::BaseTicket;
    using Indication    = std::tuple<ReplyQueue, IpcMessage, MessageType>;
    using SetupComplete = std::function<bool(IpcMessage *,
                                             IpcMessage *,
                                             const HandlePtr&,
                                             bool  success,
                                             const String_t&)>;
    using TimerId       = uint32_t;

    static constexpr TimerId kINVALID_TIMERID = UINT32_MAX;

    // PUBLIC CONSTRUCTORS
    IpcQueue(IpcProtocol::Provider *adapter) noexcept;
    ~IpcQueue();

    // PUBLIC MANIPULATORS
    bool            cancelDelayedEnqueue(TimerId id)                  noexcept;

    bool            cancelRemoteSetup(const HandlePtr& handle)        noexcept;

    TimerId         delayedLocalEnqueue(IpcMessage&&                message,
                                        std::chrono::milliseconds   delay)
                                                                      noexcept;

    bool            localEnqueue(IpcMessage&&   message)              noexcept;

    HandlePtr       remoteSetup(String_t        *diagnostic,
                                bool             acceptConfiguration,
                                std::any         configuration,
                                SetupComplete&&  completion)          noexcept;

    void            shutdown()                                        noexcept;

    // Can throw 'Shutdown' exception.
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
    void remoteEnqueue(const std::weak_ptr<IpcSession>& session,
                       MessageType                      msgtype,
                       IpcMessage&&                     message)  noexcept;

    void timerThread() noexcept;

    // PRIVATE DATA MEMBERS

    bool                            d_opened    = false; // adapter was opened
    bool                            d_shutdown  = false;
    std::mutex                      d_lock{};
    std::condition_variable         d_signalWaitThread{};
    std::deque<Indication>          d_incoming{};
    TimerId                         d_timerId   = 0;
    TimerMap                        d_timerIdMap{};
    TimerQueue                      d_timerQueue{timerCompare};
    std::condition_variable         d_signalTimerThread{};
    std::thread                     d_timerThread;
    IpcSessionFactory               d_factory;
};

} // end Wawt namespace

#endif

// vim: ts=4:sw=4:et:ai
