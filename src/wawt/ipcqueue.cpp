/** @file ipcqueue.cpp
 *  @brief Implementation of IPC utilities.
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

#include "wawt/ipcqueue.h"

#include <cryptopp/config.h>
#include <cryptopp/osrng.h>

#include <cassert>
#include <cstring>
#include <forward_list>
#include <memory>
#include <mutex>
#include <utility>
#include <thread>


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

constexpr auto digestsz = IpcMessageUtil::prefixsz
                        + CryptoPP::SHA256::DIGESTSIZE;

IpcMessage makeDigest(uint32_t salt, CryptoPP::SHA256& hash) {
    auto message = IpcMessage(std::make_unique<char[]>(digestsz), digestsz, 0);
    auto p       = message.data();
    p            = IpcMessageUtil::initPrefix(p,
                                              salt,
                                              CryptoPP::SHA256::DIGESTSIZE,
                                              IpcMessageUtil::kDIGEST);
    hash.Final(reinterpret_cast<uint8_t*>(p));
    return message;
}

IpcMessage makePrefix(uint32_t salt, uint16_t dataSize, char type) {
    auto message =
        IpcMessage(std::make_unique<char[]>(IpcMessageUtil::prefixsz),
                   IpcMessageUtil::prefixsz,
                   0);

    IpcMessageUtil::initPrefix(message.data(), salt, dataSize, type);
    return message;
}

}  // unnamed namespace


                           //---------------------------
                           // class IpcQueue::ReplyQueue
                           //---------------------------

// PUBLIC CONSTRUCTORS
IpcQueue::ReplyQueue::ReplyQueue()
: d_session()
, d_isLocal(true)
, d_winner(false)
{
    d_flag.clear();
}

IpcQueue::ReplyQueue::ReplyQueue(const std::weak_ptr<IpcSession>&  session,
                                 bool                              winner)
: d_session(session)
, d_isLocal(false)
, d_winner(winner)
{
    d_flag.clear();
}

IpcQueue::ReplyQueue::ReplyQueue(const ReplyQueue& copy)
: d_session(copy.d_session)
, d_isLocal(copy.d_isLocal)
, d_winner(copy.d_winner)
{
    d_flag.clear();
}

IpcQueue::ReplyQueue::ReplyQueue(ReplyQueue&& copy)
: d_session(std::move(copy.d_session))
, d_isLocal(copy.d_isLocal)
, d_winner(copy.d_winner)
{
    d_flag.clear();
}


// PUBLIC MEMBERS
IpcQueue::ReplyQueue&
IpcQueue::ReplyQueue::operator=(const ReplyQueue& rhs)
{
    if (this != &rhs) {
        d_session     = rhs.d_session;
        d_isLocal     = rhs.d_isLocal;
        d_winner      = rhs.d_winner;
        d_flag.clear();
    }
    return *this;
}

IpcQueue::ReplyQueue&
IpcQueue::ReplyQueue::operator=(ReplyQueue&& rhs)
{
    if (this != &rhs) {
        d_session     = std::move(rhs.d_session);
        d_isLocal     = rhs.d_isLocal;
        d_winner      = rhs.d_winner;
        d_flag.clear();
    }
    return *this;
}

bool
IpcQueue::ReplyQueue::enqueue(IpcMessage&&            message,
                              Header&&                header) noexcept
{
    auto datasize = message.length();

    if (datasize > 0 && datasize <= UINT16_MAX - IpcMessageUtil::prefixsz) {
        while (d_flag.test_and_set()) {};
        auto copy = d_session.lock();
        d_flag.clear();

        if (copy) {
            auto guard  = std::unique_lock(*copy);
            auto chain  = IpcProtocol::Channel::MessageChain{};
            chain.emplace_front(std::move(message));

            if (header) {
                chain.emplace_front(std::move(header),
                                    IpcMessageUtil::prefixsz,
                                    0);
            }
            else {
                auto salt     = copy->nextSalt();
                chain.emplace_front(makePrefix(salt,
                                               datasize,
                                               IpcMessageUtil::kDATA));
            }
            return copy->enqueue(std::move(chain), false);            // RETURN
        }
    }
    return false;                                                     // RETURN
}

bool
IpcQueue::ReplyQueue::enqueueDigest(Header             *header,
                                    const IpcMessage&   message) noexcept
{
    assert(header);
    auto datasize = message.length();

    if (header
     && datasize > 0
     && datasize <= UINT16_MAX - IpcMessageUtil::prefixsz) {
        while (d_flag.test_and_set()) {};
        auto copy = d_session.lock();
        d_flag.clear();

        if (copy) {
            *header     = std::make_unique<char[]>(IpcMessageUtil::prefixsz);
            auto chain  = IpcProtocol::Channel::MessageChain{};

            auto guard = std::unique_lock(*copy);
            auto salt  = copy->nextSalt();
            IpcMessageUtil::initPrefix(header->get(),
                                       salt,
                                       datasize,
                                       IpcMessageUtil::kDIGDATA);

            auto hash = CryptoPP::SHA256{};
            hash.Update(reinterpret_cast<const uint8_t*>(header->get()),
                        IpcMessageUtil::prefixsz);
            hash.Update(reinterpret_cast<const uint8_t*>(message.cbegin()),
                        datasize);

            chain.emplace_front(makeDigest(salt, hash));

            return copy->enqueue(std::move(chain), false);            // RETURN
        }
    }
    return false;                                                     // RETURN
}

void
IpcQueue::ReplyQueue::closeQueue() noexcept
{
    while (d_flag.test_and_set());

    auto copy = d_session.lock();
    d_session.reset();

    d_flag.clear();

    if (copy) {
        auto guard  = std::lock_guard(*copy);
        auto chain  = IpcProtocol::Channel::MessageChain{};

        auto salt   = copy->nextSalt();
        chain.emplace_front(makePrefix(salt, 0, IpcMessageUtil::kCLOSE));
        copy->enqueue(std::move(chain), true);
    }
    return;                                                           // RETURN
}

bool
IpcQueue::ReplyQueue::isClosed() const noexcept
{
    while (d_flag.test_and_set()) {};
    auto copy = d_session.lock();
    d_flag.clear();

    if (copy) {
        auto guard  = std::lock_guard(*copy);

        if (copy->state() == IpcSession::State::eOPEN) {
            return false;                                             // RETURN
        }
        copy.reset();

        while (d_flag.test_and_set()) {};
        d_session.reset();
        d_flag.clear();
    }
    return true;                                                      // RETURN
}

                               //---------------
                               // class IpcQueue
                               //---------------

// PRIVATE MEMBERS
void
IpcQueue::remoteEnqueue(std::weak_ptr<IpcSession>   session,
                        MessageType                 msgtype,
                        IpcMessage&&                message)
{
    auto guard = std::unique_lock(d_lock);
    auto ssn   = session.lock();
    assert(ssn);
    d_incoming.emplace_back(ReplyQueue(session, ssn->winner()),
                            std::move(message),
                            msgtype);
    d_signal.notify_all();
}

void
IpcQueue::timerThread()
{
    auto guard = std::unique_lock(d_lock);

    while (!d_shutdown) {
        auto now  = Clock::now();

        if (d_timerQueue.empty() || now < d_timerQueue.top().first) {
            d_timerSignal.wait_until(guard, d_timerQueue.top().first);
            continue;                                               // CONTINUE
        }

        while (!d_timerQueue.empty() && now >= d_timerQueue.top().first) {
            auto it = d_timerIdMap.find(d_timerQueue.top().second);
            d_timerQueue.pop();

            if (d_timerIdMap.end() != it) {
                d_incoming.emplace_back(ReplyQueue(),
                                        std::move(it->second),
                                        MessageType::eDATA);
                d_timerIdMap.erase(it);
            }
        }
    }
    return;                                                           // RETURN
}

// PUBLIC CONSTRUCTORS

// PUBLIC MEMBERS
bool
IpcQueue::cancelDelayedEnqueue(TimerId timerId)
{
    if (!d_shutdown) {
        auto guard      = std::lock_guard(d_lock);
        return d_timerIdMap.erase(timerId) > 0;
    }
    return false;                                                     // RETURN
}

IpcQueue::TimerId
IpcQueue::delayedLocalEnqueue(IpcMessage&&              message,
                              std::chrono::milliseconds delay)
{
    if (!d_shutdown) {
        auto expiresAt  = Clock::now() + delay;
        auto guard      = std::lock_guard(d_lock);

        d_timerIdMap.emplace(++d_timerId, std::move(message));
        d_timerQueue.emplace(expiresAt, d_timerId);

        if (d_timerQueue.top().second == d_timerId) {
            if (d_timerThread.joinable()) {
                d_timerSignal.notify_one();
            }
            else {
                try {
                    d_timerThread = std::thread( [this] { timerThread(); });
                }
                catch (...) {
                    assert(!"Failed to start timer.");
                    throw;
                }
            }
        }
        return d_timerId;                                             // RETURN
    }
    return kINVALID_TIMERID;                                          // RETURN
}

bool
IpcQueue::localEnqueue(IpcMessage&& message)
{
    auto guard = std::unique_lock(d_lock);

    if (!d_shutdown) {
        d_incoming.emplace_back(ReplyQueue(),
                                std::move(message),
                                MessageType::eDATA);
        d_signal.notify_all();
    }
    return !d_shutdown;                                               // RETURN
}

void
IpcQueue::reset()
{
    auto guard = std::unique_lock(d_lock);
    d_incoming.clear();
    d_signal.notify_all();
    return;                                                           // RETURN
}

IpcQueue::Indication
IpcQueue::waitForIndication()
{
    auto guard = std::unique_lock(d_lock);

    while (!d_shutdown) {
        if (d_incoming.empty()) {
            d_signal.wait(guard);
        }
        else {
            auto indication = std::move(d_incoming.front());
            d_incoming.pop_front();
            return indication;                                        // RETURN
        }
    }
    throw Shutdown();                                                  // THROW
}

}  // namespace Wawt
// vim: ts=4:sw=4:et:ai
