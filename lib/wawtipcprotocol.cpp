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

using IpcMessage            = WawtIpcMessage;

using IpcConnectionProtocol = WawtIpcConnectionProtocol;

using IpcQueue              = WawtIpcQueue;

using IpcUtilities          = WawtIpcUtilities;

using MessageChain          = std::forward_list<IpcMessage>;


const char kSALT    = '\005';

const char kSTARTUP = '\146';
const char kDIGEST  = '\012';
const char kDATA    = '\055';
const char kDIGDATA = '\201';
const char kCLOSE   = '\303';

constexpr auto hdrsz    = int(sizeof(char) + sizeof(uint16_t));
constexpr auto numsz    = int(sizeof(uint32_t));
constexpr auto saltsz   = hdrsz    + numsz;
constexpr auto prefixsz = saltsz   + hdrsz;
constexpr auto digestsz = prefixsz + CryptoPP::SHA256::DIGESTSIZE;

inline
bool extractHdr(char *type, uint16_t *size, const char **p, const char *end)
{
    if (end - *p >= hdrsz) {
        auto a = *p;
        *type = a[0];
        *size = (uint8_t(a[1]) << 8) | uint8_t(a[2]);
        *p += hdrsz;
        return true;                                                  // RETURN
    }
    return false;                                                     // RETURN
}

inline
bool extractValue(uint32_t *value, const char **p, const char *end)
{
    if (end - *p >= numsz) {
        auto a = *p;
        *value = (uint8_t(a[0]) << 24) | (uint8_t(a[1]) << 16)
               | (uint8_t(a[2]) <<  8) |  uint8_t(a[3]);
        *p += numsz;
        return true;                                                  // RETURN
    }
    return false;                                                     // RETURN
}

inline
bool extractSalt(uint32_t *salt, const char **p, const char *end)
{
    auto type   = char{};
    auto size   = uint16_t{};

    if (extractHdr(&type, &size, p, end) && type == kSALT && size == saltsz) {
        if (salt) {
            extractValue(salt, p, end);
        }
        return true;                                                  // RETURN
    }
    return false;                                                     // RETURN
}

bool checkMessage(uint32_t             *salt,
                  uint32_t             *random,
                  char                 *type,
                  const IpcMessage&     message) {
    auto p      = message.cbegin();
    auto end    = message.cend();
    auto bytes  = uint16_t{};
    
    if (extractSalt(salt, &p, end) && extractHdr(type, &bytes, &p, end)) {
        bytes -= hdrsz;
        if (p + bytes == end) {
            if (*type == kDATA || *type == kDIGDATA || *type == kCLOSE) {
                return true;                                          // RETURN
            }

            if (*type == kDIGEST) {
                return bytes == CryptoPP::SHA256::DIGESTSIZE;         // RETURN
            }

            if (*type == kSTARTUP) {
                return extractValue(random, &p, end);                  // RETURN
            }
        }
    }
    return false;                                                     // RETURN
}

inline
char *initHeader(char *p, uint16_t size, char type) {
    *p++    =  type;
    *p++    =  char(size >>  8);
    *p++    =  char(size);
    return p;
}

inline
char *initValue(char *p, uint32_t value) {
    *p++    = char(value  >> 24);
    *p++    = char(value  >> 16);
    *p++    = char(value  >>  8);
    *p++    = char(value);
    return p;
}

char *initPrefix(char *p, uint32_t salt, uint16_t size, char type) {
    size   += hdrsz;

    p = initHeader(p, sizeof(salt), kSALT);
    p = initValue( p, salt);
    p = initHeader(p, size, type);
    return p;
}

IpcMessage makeDigest(uint32_t salt, CryptoPP::SHA256& hash) {
    auto message = IpcMessage(std::make_unique<char[]>(digestsz), digestsz, 0);
    auto p       = message.data();
    p            = initPrefix(p, salt, CryptoPP::SHA256::DIGESTSIZE, kDIGEST);
    hash.Final(reinterpret_cast<uint8_t*>(p));
    return message;
}

IpcMessage makeHandshake(uint32_t           random1,
                         uint32_t           random2,
                         const IpcMessage&  data)
{
    auto length     = data.length();
    auto size       = length + sizeof(random2) + prefixsz;
    auto message    = IpcMessage(std::make_unique<char[]>(size), size, 0);
    auto p          = message.data();
    p               = initPrefix(p, random1, length-prefixsz, kSTARTUP);
    p               = initValue( p, random2);
    std::memcpy(p, data.cbegin(), length); 
    return message;
}

IpcMessage makePrefix(uint32_t salt, uint16_t dataSize, char type) {
    auto message = IpcMessage(std::make_unique<char[]>(prefixsz), prefixsz, 0);

    initPrefix(message.data(), salt, dataSize, type);
    return message;
}

}  // unnamed namespace


                        //============================
                        // struct IpcQueue::CryptoPrng
                        //============================

struct IpcQueue::CryptoPrng {
    CryptoPP::AutoSeededRandomPool  d_prng;

    uint32_t random32() {
        return d_prng.GenerateWord32();
    }
};

                            //========================
                            // class IpcQueue::Session
                            //========================

class IpcQueue::Session {
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
            uint32_t                random1,
            uint32_t                random2,
            IpcConnectionProtocol  *adapter,
            ConnectionId            id,
            Role                    role)
        : d_handshake(makeHandshake(random1, random2, handshake))
        , d_rcvSalt(random1)
        , d_adapter(adapter)
        , d_sessionId(id)
        , d_random(random2)
        , d_role(role) { }


    // PUBLIC MANIPULATORS
    IpcQueue::MessageNumber nextSalt() {
        return ++d_sendSalt;
    }

    bool enqueue(MessageChain&& chain, bool close) {
        auto ret = d_state == State::eOPEN
                && d_adapter->sendMessage(d_sessionId, std::move(chain));
        if (ret && close) {
            d_state = State::eWAITING_ON_DISC;
        }
        return ret;
    }

    void lock() {
        d_lock.lock();
    }

    MessageChain getStartupDigest() {
        assert(d_state == State::eWAITING_ON_CONNECT);
        d_state         = State::eWAITING_ON_DIGEST;
        auto    chain   = MessageChain{};
        auto    hash    = CryptoPP::SHA256{};
        hash.Update(reinterpret_cast<const uint8_t*>(d_handshake.cbegin()),
                    d_handshake.length());
        chain.emplace_front(makeDigest(d_rcvSalt, hash));
        return chain;
    }

    void setClosed() {
        d_state = State::eWAITING_ON_DISC;
    }

    MessageChain saveStartupDigest(IpcQueue::MessageNumber  initialValue,
                                   IpcMessage&&             receivedDigest) {
        assert(d_state == State::eWAITING_ON_DIGEST);
        d_sendSalt  = initialValue;
        d_state     = State::eWAITING_ON_START;
        d_digest    = std::move(receivedDigest);
        d_digest.d_offset += prefixsz;
        auto chain  = MessageChain{};
        chain.emplace_front(std::move(d_handshake));
        return chain;
    }

    bool verifyStartupMessage(IpcQueue::MessageNumber   digestValue,
                              uint32_t                  random,
                              const IpcMessage&         message) {
        d_random ^= random;
        d_winner  = ((d_random & 8) == 0) == (d_role == Role::eINITIATOR);

        return digestValue == d_sendSalt
            && CryptoPP::SHA256().VerifyDigest(
                    reinterpret_cast<const uint8_t*>(d_digest.cbegin()),
                    reinterpret_cast<const uint8_t*>(message.cbegin()),
                    message.length());     // RETURN
    }

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
    // PRIVATE DATA MEMBERS
    State                           d_state     = State::eWAITING_ON_CONNECT;
    bool                            d_winner    = false;
    IpcQueue::MessageNumber         d_sendSalt  = 0;
    std::mutex                      d_lock{};
    IpcMessage                      d_digest{};     // digest from downstream
    IpcMessage                      d_handshake;    // handshake from upstream
    IpcQueue::MessageNumber         d_rcvSalt;
    IpcConnectionProtocol          *d_adapter;
    ConnectionId                    d_sessionId;
    uint32_t                        d_random;
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
, d_winner(winner)
, d_sessionId(sessionId)
{
    d_flag.clear();
}

IpcQueue::ReplyQueue::ReplyQueue(const ReplyQueue& copy)
: d_session(copy.d_session)
, d_winner(copy.d_winner)
, d_sessionId(copy.d_sessionId)
{
    d_flag.clear();
}

IpcQueue::ReplyQueue::ReplyQueue(ReplyQueue&& copy)
: d_session(std::move(copy.d_session))
, d_winner(copy.d_winner)
, d_sessionId(copy.d_sessionId)
{
    d_flag.clear();
}


// PUBLIC MEMBERS
IpcQueue::ReplyQueue&
IpcQueue::ReplyQueue::operator=(const ReplyQueue& rhs)
{
    if (this != &rhs) {
        d_session     = rhs.d_session;
        d_winner      = rhs.d_winner;
        d_sessionId   = rhs.d_sessionId;
        d_flag.clear();
    }
    return *this;
}

IpcQueue::ReplyQueue&
IpcQueue::ReplyQueue::operator=(ReplyQueue&& rhs)
{
    if (this != &rhs) {
        d_session     = std::move(rhs.d_session);
        d_winner      = rhs.d_winner;
        d_sessionId   = rhs.d_sessionId;
        d_flag.clear();
    }
    return *this;
}

bool
IpcQueue::ReplyQueue::enqueue(IpcMessage&&            message,
                              Header&&                header) noexcept
{
    auto datasize = message.length();

    if (datasize > 0 && datasize <= UINT16_MAX-prefixsz) {
        while (d_flag.test_and_set()) {};
        auto copy = d_session.lock();
        d_flag.clear();

        if (copy) {
            auto guard  = std::unique_lock(*copy);
            auto chain  = MessageChain{};
            chain.emplace_front(std::move(message));

            if (header) {
                chain.emplace_front(std::move(header), prefixsz, 0);
            }
            else {
                auto salt     = copy->nextSalt();
                chain.emplace_front(makePrefix(salt, datasize, kDATA));
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

    if (header && datasize > 0 && datasize <= UINT16_MAX-prefixsz) {
        while (d_flag.test_and_set()) {};
        auto copy = d_session.lock();
        d_flag.clear();

        if (copy) {
            *header     = std::make_unique<char[]>(prefixsz);
            auto chain  = MessageChain{};

            auto guard = std::unique_lock(*copy);
            auto salt  = copy->nextSalt();
            initPrefix(header->get(), salt, datasize, kDIGDATA);

            auto hash = CryptoPP::SHA256{};
            hash.Update(reinterpret_cast<const uint8_t*>(header->get()),
                        prefixsz);
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
        auto chain  = MessageChain{};

        auto salt   = copy->nextSalt();
        chain.emplace_front(makePrefix(salt, 0, kCLOSE));
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

        if (copy->state() == Session::State::eOPEN) {
            return false;                                            // RETURN
        }
        copy.reset();

        while (d_flag.test_and_set()) {};
        d_session.reset();
        d_flag.clear();
    }
    return true;                                                     // RETURN
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
    auto guard = std::unique_lock(d_lock);

    if (status == Protocol::ConnectStatus::eOK) {
        auto random1 = d_prng_p->random32();
        auto random2 = d_prng_p->random32();
        auto session = std::make_shared<Session>(d_handshake,
                                                 random1,
                                                 random2,
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
                d_signal.notify_all();
            }
            session->setClosed();
        }
    }
    d_adapter->closeConnection(id);
    return;                                                           // RETURN
}

void
IpcQueue::processMessage(Protocol::ConnectionId id, IpcMessage&& message)
{
    auto guard = std::unique_lock(d_lock);

    auto it     = d_sessionMap.find(id);
    auto salt   = uint32_t{};
    auto random = uint32_t{};
    auto type   = char{};

    if (it == d_sessionMap.end()) {
        assert(!"Message for unknown connection ID.");
    }
    else if (checkMessage(&salt, &random, &type, message)) {
        auto session    = it->second;
        auto ssnguard   = std::unique_lock(*session);
        auto close      = false;

        switch (session->state()) {
          case Session::State::eWAITING_ON_DIGEST: {
            if(kDIGEST == type) {
                d_adapter->sendMessage(
                    id,
                    session->saveStartupDigest(salt, std::move(message)));
                assert(session->state() == Session::State::eWAITING_ON_START);
            }
            else {
                close = true;
                assert(!"Required a DIGEST.");
            }
          } break;                                                     // BREAK
          case Session::State::eWAITING_ON_START: {
            if (kSTARTUP == type
             && session->verifyStartupMessage(salt, random, message)) {
                message.d_offset += prefixsz + sizeof(random);

                if (message.length() > 0) {
                    auto reply  = ReplyQueue(session, session->winner(), id);
                    d_incoming.emplace_back(std::move(reply),
                                            std::move(message),
                                            MessageType::eDATA);
                    d_signal.notify_all();
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
                d_incoming.emplace_back(local,
                                        std::move(it->second),
                                        MessageType::eDATA);
                d_timerIdMap.erase(it);
            }
        }
    }
    return;                                                           // RETURN
}

// PUBLIC CONSTRUCTORS
WawtIpcQueue::WawtIpcQueue(IpcConnectionProtocol *adapter)
: d_prng_p(new IpcQueue::CryptoPrng())
, d_adapter(adapter)
{
    d_adapter->installCallbacks([this](auto id, auto status, auto role) {
                                    connectionUpdate(id, status, role);
                                },
                                [this](auto id, auto&& message) {
                                    processMessage(id, std::move(message));
                                });
}

WawtIpcQueue::~WawtIpcQueue()
{
    delete d_prng_p;
}

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
        auto local = ReplyQueue({}, false, kLOCAL_SSNID);
        d_incoming.emplace_back(std::move(local),
                                std::move(message),
                                MessageType::eDATA);
        d_signal.notify_all();
    }
    return !d_shutdown;                                               // RETURN
}

bool
IpcQueue::openAdapter(Wawt::String_t   *diagnostics,
                      IpcMessage        disconnectMessage,
                      IpcMessage        handshakeMessage)
{
    auto guard = std::unique_lock(d_lock);

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
    auto guard = std::unique_lock(d_lock);
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

                               //-------------------
                               // class IpcUtilities
                               //-------------------

IpcQueue::MessageNumber
IpcUtilities::messageNumber(const IpcMessage& message)
{
    auto salt   = uint32_t{};

    if (message.d_offset >= prefixsz) {
        auto *end   = message.cbegin();
        auto *p     = end - prefixsz;
        extractSalt(&salt, &p, end);
    }
    return salt;
}

bool
IpcUtilities::verifyDigestPair(const IpcMessage&       digest,
                               const IpcMessage&       digestMessage)
{

    if (digestMessage.d_offset < prefixsz) {
        assert(!"Message missing header.");
        return false;                                                 // RETURN
    }
    auto msglength  = digestMessage.length();
    auto data       = digestMessage.cbegin() - prefixsz;
    auto datalng    = msglength + prefixsz;

    uint32_t salt1, salt2;
    auto p1     = digest.cbegin();
    auto end1   = digest.cend();
    auto p2     = data;
    auto end2   = digestMessage.cend();

    if (extractSalt(&salt1, &p1, end1)
     && extractSalt(&salt2, &p2, end2)
     && salt1 == salt2
     && kDIGEST  == p1[0]
     && kDIGDATA == p2[0]
     && ((uint8_t(p1[1]) << 8) | uint8_t(p1[2])) ==  digestsz-saltsz
     && ((uint8_t(p2[1]) << 8) | uint8_t(p2[2])) == msglength+hdrsz) {
        return CryptoPP::SHA256().VerifyDigest(
                reinterpret_cast<const uint8_t*>(p1) + hdrsz, /* hash */
                reinterpret_cast<const uint8_t*>(data),
                datalng);                                             // RETURN
    }
    return false;                                                     // RETURN
}

}  // namespace BDS
// vim: ts=4:sw=4:et:ai
