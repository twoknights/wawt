/** @file wawtipcprotocol.cpp
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

#include "wawtipcprotocol.h"

#include <cryptopp/config.h>
#include <cryptopp/osrng.h>

#undef  S
#undef  C
#define S(str) Wawt::String_t(u8"" str)      // UTF8 strings  (std::string)
#define C(c) (u8 ## c)
//#define S(str) Wawt::String_t(L"" str)  // wide char strings (std::wstring)
//#define C(c) (L ## c)

namespace BDS {

namespace {

using IpcMessage    = WawtIpcMessage;

using IpcConnectionProtocol = WawtIpcConnectionProtocol;

using IpcQueue      = WawtIpcQueue;

using IpcUtilities  = WawtIpcUtilities;

using MessageChain  = std::forward<IpcMessage>;


const uint8_t kSALT    = '\005';

const uint8_t kSTARTUP = '\146';
const uint8_t kDIGEST  = '\012';
const uint8_t kDATA    = '\055';
const uint8_t kDIGDATA = '\201';
const uint8_t kCLOSE   = '\303';

constexpr auto hdrsz  = 1 + sizeof(uint16_t);
constexpr auto prefixsz  = saltsz + hdrsz;
constexpr auto saltsz = 1 + sizeof(uint16_t) + sizeof(uint32_t);

IpcMessage dataPrefix(uint32_t salt, uint16_t dataSize, uint8_t prefix) {
    IpcMessage message = { std::make_unique<uint8_t>(prefixsz); prefixsz, 0 };

    auto p  = message.d_data.get();
    *p++    =  kSALT              & 0xFF;
    *p++    = (saltsz      >>  8) & 0xFF;
    *p++    =  saltsz             & 0xFF;
    *p++    = (salt        >> 24) & 0xFF;
    *p++    = (salt        >> 16) & 0xFF;
    *p++    = (salt        >>  8) & 0xFF;
    *p++    =  salt               & 0xFF;
    *p++    =  prefix             & 0xFF;
    *p++    = (dataSize    >>  8) & 0xFF;
    *p++    =  dataSize           & 0xFF;
    return message;
}

bool extractSalt(uint32_t *salt, uint8_t **p, uint8_t *end)
{
    if (end - *p >= saltsz) {
        auto a = *p;

        if (a[0] == kSALT) {
            auto length = (uint16_t(a[1]) << 8) | uint16_t(a[2]);

            if (sizeof(*salt) == length - hdrsz) {
                if (salt) {
                    *salt = (uint32_t(a[3]) << 24 | (uint32_t(a[4] << 16)
                                                  | (uint32_t(a[5] << 8)
                                                  |  uint32_t(a[6]);
                }
                *p += length;
                return true;                                          // RETURN
            }
        }
    }
    return false;                                                     // RETURN
}

int  addPrefix(WawtIpcMessage *message,
               uint8_t          header,
               uint32_t         random1,
               uint32_t         random2,
               uint32_t         salt)
{
    constexpr auto prefixsz = sizeof(header)    + sizeof(uint16_t)
                            + sizeof(random1)   + sizeof(random2)
                            + sizeof(salt);

    MessageFragment prefix{ std::make_unique<char[]>(prefixsz), prefixsz, 0 };

    auto p  = prefix.d_data.get();
    *p++    =  header             & 0xFF;
    *p++    = (prefixsz    >>  8) & 0xFF;
    *p++    =  prefixsz           & 0xFF;
    *p++    = (random1     >> 24) & 0xFF;
    *p++    = (random1     >> 16) & 0xFF;
    *p++    = (random1     >>  8) & 0xFF;
    *p++    =  random1            & 0xFF;
    *p++    = (random2     >> 24) & 0xFF;
    *p++    = (random2     >> 16) & 0xFF;
    *p++    = (random2     >>  8) & 0xFF;
    *p++    =  random2            & 0xFF;
    *p++    = (salt        >> 24) & 0xFF;
    *p++    = (salt        >> 16) & 0xFF;
    *p++    = (salt        >>  8) & 0xFF;
    *p++    =  salt               & 0xFF;
    
    message->emplace_front(std::move(prefix));
    return prefixsz;
}

constexpr auto sha256sz =  hdrsz  + CryptoPP::SHA256::DIGESTSIZE;
constexpr auto digestsz =  saltsz + sha256sz;

IpcMessage digest(uint32_t salt, CryptoPP::SHA256& hash) {
    IpcMessage message = { std::make_unique<uint8_t>(digestsz); digestsz, 0 };

    auto p  = message.d_data.get();
    *p++    =  kSALT              & 0xFF;
    *p++    = (saltsz      >>  8) & 0xFF;
    *p++    =  saltsz             & 0xFF;
    *p++    = (salt        >> 24) & 0xFF;
    *p++    = (salt        >> 16) & 0xFF;
    *p++    = (salt        >>  8) & 0xFF;
    *p++    =  salt               & 0xFF;
    *p++    =  kDIGEST            & 0xFF;
    *p++    = (sha256sz    >>  8) & 0xFF;
    *p++    =  sha256sz           & 0xFF;

    hash.Final(p);
    return message;
}

bool validateMessage(uint32_t          *salt,
                     uint8_t           *type,
                     const IpcMessage&  message) {
    auto& [buffer, size, offset] = message;
    auto p      = buffer.get() + offset;
    auto end    = buffer.get() + size;
    
    if (extractSalt(salt, &p, end)) {
        *type = p[0];
        auto bytes = (uint16_t(p[1]) << 8) | uint16_t(p[2]);

        if (p + bytes == end) {
            if (*type == kDIGEST) {
                return bytes == sha256sz;
            }
            return true;                                              // RETURN
        }
    }
    return false;                                                     // RETURN
}

}  // unnamed namespace


                        //============================
                        // struct IpcQueue::CryptoPrng
                        //============================

struct IpcQueue::CryptoPrng {
    CryptoPP::AutoSeededRandomPool  d_prng;

    CryptoPP::AutoSeededRandomPool& operator()() {
        return d_prng;
    }
};

                            //========================
                            // class IpcQueue::Session
                            //========================

class Session {
  public:
    // PUBLIC TYPES
    enum class State {
          eWAITING_ON_CONNECT
        , eWAITING_ON_DIGEST
        , eWAITING_ON_START
        , eOPEN
        , eWAITING_ON_DISC
    };
    using ConnectionId = IpcConnectionProtocol::ConnectionId;
    using Role         = IpcConnectionProtocol::ConnectRole;

    // PUBLIC CONSTRUCTORS
    Session(const Session&)                 = delete;
    Session& operator=(const Session&)      = delete;

    Session(const IpcMessage&       handshake,
            IpcConnectionProtocol  *adapter,
            ConnectionId            id,
            Role                    role)
        : d_handshake(makeStartupMessage(handshake)) TBD
        , d_adapter(adapter)
        , d_sessionId(id)
        , d_role(role) { }


    // PUBLIC MANIPULATORS
    IpcQueue::MessageNumber nextSalt() {
        return d_sendSalt++;
    }

    void enqueue(IpcConnectionProtocol::MessageChain&& chain, bool close) {
        auto ret = d_state == State::eOPEN
                && d_adapter->sendMessage(d_sessionId, std::move(chain));
        if (ret && close) {
            d_state = State::eWAITING_ON_DISC;
        }
        return ret;
    }

    void lock() {
        d_mutex.lock();
    }

    IpcMessage getStartupDigest() const {
        assert(d_state == State::eWAITING_ON_CONNECT);
        d_state = State::eWAITING_ON_DIGEST;
        return makeStartupDigest(d_handshake); TBD
    }

    void setClosed() {
        d_state = State::eWAITING_ON_DISC;
    }

    IpcMessage saveStartupDigest(IpcMessage&& receivedDigest) {
        assert(d_state == State::eWAITING_ON_DIGEST);
        d_digest = std::move(receivedDigest);
        d_state = State::eWAITING_ON_START;
        return std::move(d_handshake);
    }

    void unlock() {
        d_mutex.unlock();
    }

    // PUBLIC ACCESSORS
    State state() const {
        return d_state;
    }

    TBD
    bool verifyStartupMessage(IpcMessage *message) const; // also salt values.

  private:
    // PRIVATE DATA MEMBERS
    State                           d_state     = State::eWAITING_ON_CONNECT;
    IpcQueue::MessageNumber         d_sendSalt  = 0;
    IpcQueue::MessageNumber         d_rcvSalt   = 0;
    std::mutex                      d_lock{};
    IpcMessage                      d_digest{};     // digest from downstream
    IpcMessage                      d_handshake;    // handshake from upstream
    IpcConnectionProtocol          *d_adapter;
    ConnectionId                    d_sessionId;
    Role                            d_role;

};

                           //---------------------------
                           // class IpcQueue::ReplyQueue
                           //---------------------------

// PUBLIC CONSTRUCTORS
IpcQueue::ReplyQueue::ReplyQueue(const std::shared_ptr<Session>&  session,
                                 bool                             winner,
                                 int                              sessionId)
: d_session(session)
, d_winner(winnder)
, d_sessionId(sessionId)
{
    d_flag.clear();
}

// PUBLIC MEMBERS
bool
IpcQueue::ReplyQueue::enqueue(IpcMessage&&            message,
                              Header&&                header)
{
    auto datasize = message.length();

    while (d_flag.test_and_set()); auto copy=d_session.lock(); d_flag.clear();

    if (datasize > 0 && datasize <= UINT16_MAX-prefixsz && copy) {
        std::lock_guard guard(*copy);
        MessageChain chain;
        chain.emplace_front(std::move(message));

        if (header) {
            chain.emplace_front(std::move(header), prefixsz, 0);
        }
        else {
            auto salt     = copy->nextSalt();
            chain.emplace_front(dataPrefix(salt, datasize, kDATA);
        }
        return copy->enqueue(std::move(chain), false);                // RETURN
    }
    return false;                                                     // RETURN
}

bool
IpcQueue::ReplyQueue::enqueueDigest(Header             *header,
                                    const IpcMessage&   message)
{
    assert(header);

    while (d_flag.test_and_set()); auto copy=d_session.lock(); d_flag.clear();

    auto datasize = message.length();

    if (datasize > 0 && datasize <= UINT16_MAX-prefixsz && copy) {
        auto guard = std::scoped_lock(*this, *copy);
        auto& [buffer, size, offset] = message;
        auto salt  = copy->nextSalt();
        *header = dataPrefix(salt, datasize, kDIGDATA).d_data;

        auto hash = CryptoPP::SHA256;
        hash.Update(header->d_data.get(), header->d_size);
        hash.Update(buffer.get() + offset, datasize);

        auto chain  = IpcConnectionProtocol::MessageChain{};
        chain.emplace_front(digest(salt, hash));

        return copy->enqueue(std::move(chain), false);                // RETURN
    }
    return false;                                                     // RETURN
}

void
IpcQueue::ReplyQueue::closeQueue()
{
    while (d_flag.test_and_set());

    auto copy = d_session.lock();
    d_session.reset();

    d_flag.clear();

    if (copy) {
        auto guard  = std::lock_guard(*copy);
        auto chain  = IpcConnectionProtocol::MessageChain{};

        auto salt   = copy->nextSalt();
        chain.emplace_front(dataPrefix(salt, 0, kCLOSE);
        copy->enqueue(std::move(chain), true);
    }
    return;                                                           // RETURN
}

                               //---------------
                               // class IpcQueue
                               //---------------

// PRIVATE MEMBERS
void
IpcQueue::connectionUpdate(Protocol::ConnectionId   id,
                           Protocol::ConnectStatus  status,
                           Protocol::ConnectRole    role)
{
    // Note: no other connection update for 'id' can be delivered
    // until this function returns.
    auto guard = std::lock_guard(*this);

    if (status == Protocol::ConnectStatus::eOK) {
        auto session = std::make_shared<Session>(d_handshake,
                                                 d_adapter,
                                                 id,
                                                 role);
        if (d_sessionMap.try_emplace(id, session).second) {
            guard.release()->unlock();
            d_adapter->sendMessage(id, session->getStartupDigest());
            return;                                                   // RETURN
        }
        assert(!"Connection ID reused.");
    }
    else {
        auto it = d_sessionMap.find(id);

        if (it != d_sessionMap.end()) {
            auto session = it->second;
            d_sessionMap.erase(it);

            if (session->state() != Session::State::eWAITING_ON_DISC
             && d_disconnect.length() > 0) {
                auto reply      = ReplyQueue({}, session->winner(), id);
                d_incoming.emplace_back(std::move(reply),
                                        d_disconnect,
                                        MessageType::eDISCONNECT);
                d_signal.d_notify_all();
            }
            session->setClosed();
        }
    }
    d_adapter->closeConnection(id);
    return;                                                           // RETURN
}

void
IpcQueue::processMessage(ConnectionId id, IpcMessage&& message)
{
    auto guard = std::unique_lock(*this);

    auto it     = d_sessionMap.find(id);
    auto salt   = uint32_t{};
    auto type   = uint8_t{};

    if (it == d_sessionMap.end()) {
        assert(!"Message for unknown connection ID.");
    }
    else if (validateMessage(&salt, &type, message)) {
        auto session    = it->second;
        auto ssnguard   = std::unique_lock(*session);
        auto close      = false;

        switch (session->state()) {
          case Session::State::eWAITING_ON_DIGEST: {
            if(kDIGEST == type) {
                d_adapter->sendMessage(id,
                                       session->saveStartupDigest(message));
                assert(session->state() == Session::State::WAITING_ON_START);
            }
            else {
                close = true;
                assert(!"Required a DIGEST.");
            }
          } break;                                                     // BREAK
          case Session::State::eWAITING_ON_START: {
            if(kSTARTUP == type && session->verifyStartup(&message)) {
                if (message.length() > 0) {
                    forward_reply(session, std::move(message));
                }
                assert(session->state() == Session::State::eOPEN);
            }
            else {
                close = true;
                assert(!"Required a STARTUP.");
            }
          } break;                                                     // BREAK
          case Session::State::eOPEN: {
            auto reply          = ReplyQueue(session, session->winner(), id);
            auto msgtype        = MessageType::eDATA;
            message.d_offset   += prefixsz;

            if (kDATA == type) {
            }
            else if (kDIGDATA == type) {
                msgtype = MessageType::eDIGESTED_DATA;
            }
            else if (kDIGEST == type) {
                msgtype = MessageType::eDIGEST;
            }
            else if (kCLOSE == type) {
                session->setClosed();
                d_adapter->closeConnection(id);
                reply.d_session.reset();
                assert(session->state() == Session::State::eWAITING_ON_DISC);
            }
            else {
                close = true;
                assert(!"STARTUP unexpected.");
            }

            if (!close && message.length() > 0) {
                d_incoming.emplace_back(std::move(reply),
                                        std::move(message),
                                        msgtype);
                d_signal.notify_all();
            }
          } break;                                                     // BREAK
          case Session::State::eWAITING_ON_DISC: {
          } break;                                                     // BREAK
          default: {
            assert(!"Session in invalid state.");
          }
        }

        if (!close) {
            return;                                                   // RETURN
        }
    }
    d_adapter->closeConnection(id);
    return;                                                           // RETURN
}

void
IpcQueue::timerThread()
{
    auto local = ReplyQueue({}, false, kLOCAL_SSNID);
    auto guard = std::unique_lock<IpcQueue>(*this);

    while (!d_shutdown) {
        auto now  = Clock::now();

        if (d_timerQueue.empty() || now < d_timerQueue.top().first) {
            d_timerSignal.wait_until(guard, d_timerQueue.top().first);
            continue;                                               // CONTINUE
        }

        while (!d_timerQueue.empty() && now >= d_timerQueue.top().first) {
            auto it = d_timerMap.find(d_timerQueue.top().second);
            d_timerQueue.pop();

            if (d_timerIdMap.end() != it) {
                d_incoming.emplace_back(local, std::move(it->second));
                d_timerIdMap.erase(it);
            }
        }
    }
    return;                                                           // RETURN
}

// PUBLIC CONSTRUCTORS
IpcQueue::IpcQueue(IpcConnectionProtocol *adapter)
: d_prng(std::make_unique<IpcQueue::CryptoPrng>())
, d_adapter(adapter)
{
    d_adapter->installCallbacks([this](auto id, auto status, auto role) {
                                    connectionUpdate(id, status, role);
                                },
                                [this](auto id, auto&& message) {
                                    processMessage(id, std::move(message));
                                });
}

// PUBLIC MEMBERS
bool
IpcQueue::cancelDelayedEnqueue(TimerId timerId)
{
    if (!d_shutdown) {
        auto guard      = std::lock_guard(*this);
        return d_timerMap.erase(timerId) > 0;
    }
    return false;                                                     // RETURN
}

IpcQueue::TimerId
IpcQueue::delayedLocalEnqueue(IpcMessage&&              message,
                              std::chrono::milliseconds delay)
{
    if (!d_shutdown) {
        auto expiresAt  = Clock::now() + delay;
        auto guard      = std::lock_guard(*this);

        d_timerMap.emplace(++d_timerId, std::move(message));
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
    auto guard = std::unique_lock(*this);

    if (!d_shutdown) {
        auto local = ReplyQueue({}, false, kLOCAL_SSNID);
        d_incoming.emplace_back(std::move(local), std::move(message));
        d_signal.notify_all();
    }
    return !d_shutdown;                                               // RETURN
}

bool
IpcQueue::openAdapter(Wawt::String_t   *diagnostics,
                      IpcMessage        disconnectMessage,
                      IpcMessage        handshakeMessage)
{
    std::lock_guard guard(*this);

    if (!d_opened) {
        d_handshake     = std::move(handshakeMessage);
        d_disconnect    = std::move(disconnectMessage);
        return d_opened = d_adapter->openAdapter(diagnostics);
    }
    *diagnostics = S("Adapter previously opened.");
    return false;                                                     // RETURN
}

void
IpcQueue::reset()
{
    std::lock_guard guard(*this);
    // note that local enqueing can occur even when not opened.

    if (d_opened) {
        d_handshake.reset();
        d_disconnect.reset();
        d_adapter->closeAdapter();
    }
    d_opened = false;
    d_incoming.clear();
    d_signal.notify_all();
    return;                                                           // RETURN
}

IpcQueue::Indication
IpcQueue::waitForIndication()
{
    auto guard = std::unique_lock(*this);

    while (!d_shutdown) {
        if (d_incoming.empty()) {
            d_signal.wait(guard);
        }
        else {
            auto indication = std::move(d_incoming.front());
            d_incoming.pop_front();
            
            if (!indication.first.d_session->isOpen()) {
                indication.first.d_session.reset();
            }
            return indication;                                        // RETURN
        }
    }
    throw Shutdown();                                                  // THROW
}

                               //-------------------
                               // class IpcUtilities
                               //-------------------

bool
IpcUtilities::skipToUserData(IpcMessage *message)
{
    while (!message->empty()) {
        auto fragment = message->front();
        auto size     = fragment.d_size;
        auto offset   = fragment.d_offset;

        while (size > offset) {
            if (size - offset < hdrsz) {
                assert(!"Corrupt fragment");
                message->clear();
                return false;                                         // RETURN
            }
            auto hdr      = fragment.d_data.get() + offset;
            auto hdrbyte  = hdr[0];
            auto blocksz  = (uint16_t(hdr[1]) << 8) | uint16_t(hdr[2]);

            if (hdrbyte == kDATA) {
                fragment.d_offset = offset + hdrsz;

                if (fragment.d_offset == size) {
                    message->pop_front();
                }
                else if (fragment.d_offset > size) {
                    assert(!"Corrupt data header");
                    message->clear();
                    return false;                                     // RETURN
                }
                return true;                                          // RETURN
            }

            if (hdrbyte != kSALT) {
                if (hdrbyte == kDIGEST) {
                    return false;                                     // RETURN
                }
                assert(!"Corrupt header type");
                message->clear();
                return false;                                         // RETURN
            }

            if (blocksz + offset > size) {
                assert(!"Corrupt header");
                message->clear();
                return false;                                         // RETURN
            }
            offset += blocksz;
        }
        message->pop_front();
    }
    return false;                                                     // RETURN
}

IpcQueue::MessageNumber
IpcUtilities::messageNumber(const IpcMessage& message)
{
    auto *p     = message.get();
    auto *end   = p + prefixsz;
    auto salt   = uint32_t{};

    extractSalt(&salt, &p, end);
    return salt;
}

bool
IpcUtilities::verifyDigestPair(const IpcMessage&       digest,
                               const IpcMessage&       digestMessage)
{
    auto& [buffer, size, offset] = digestMessage;
    auto  length = size - offset;

    if (offset < prefixsz) {
        assert(!"Message missing header.");
        return false;                                                 // RETURN
    }
    auto data = buffer.get() + offset - prefixsz;
    auto datalng = size - (offset - prefixsz);

    uint32_t salt1, salt2;
    uint8_t *p1   = digest.d_data.get() + digest.d_data.d_offset - ;
    uint8_t *end1 = p1 +  digest.d_size - digest.d_offset;
    uint8_t *p2   = data;
    uint8_t *end2 = buffer.get() + size;

    if (extractSalt(&salt1, &p1, end1)
     && extractSalt(&salt2, &p2, end2)
     && salt1 == salt2
     && kDIGEST  == p1[0]
     && kDIGDATA == p2[0]
     && ((uint16_t(p1[1]) << 8) | uint16_t(p1[2])) == sha256sz;
     && ((uint16_t(p2[1]) << 8) | uint16_t(p2[2])) == length+hdrsz) {
        return CryptoPP::SHA256().VerifyDigest(p1 + hdrsz, /* hash */
                                               data,
                                               datalng);              // RETURN
    }
    return false;                                                     // RETURN
}

}  // namespace BDS
// vim: ts=4:sw=4:et:ai
