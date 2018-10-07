/** @file sfmlipcadapter.cpp
 *  @brief Implements Wawt::IpcProtocol for SFML.
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

#include "sfmlipcadapter.h"

#include <SFML/Network/IpAddress.hpp>
#include <SFML/Network/SocketSelector.hpp>
#include <SFML/Network/SocketSelector.hpp>
#include <SFML/Network/TcpListener.hpp>
#include <SFML/System/String.hpp>

#include <cassert>
#include <regex>
#include <thread>
#include <iostream>

using namespace Wawt;

#ifdef WAWT_WIDECHAR
#define S(str) Wawt::String_t(L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) Wawt::String_t(u8"" str)      // UTF8 strings  (std::string)
#define C(c) (u8 ## c)
#endif

namespace BDS {

namespace {

static constexpr unsigned char BYTE1 = '\125';
static constexpr unsigned char BYTE2 = '\252';

using MessageChain = IpcProtocol::MessageChain;

} // end unnamed namespace


                        //=================================
                        // class SfmlIpcAdapter::Connection
                        //=================================

struct SfmlIpcAdapter::Connection {
    using Address = std::pair<unsigned short, sf::IpAddress>;
    using Mode    = IpcProtocol::ChannelMode;

    std::thread                     d_reader;
    std::thread                     d_writer;
    sf::TcpSocket                   d_socket;
    std::shared_ptr<Connection>     d_self;
    ChannelChange                   d_status    = ChannelChange::eREADY;
    bool                            d_shutdown  = false;
    std::mutex                      d_lock{};
    std::condition_variable         d_signal{};
    std::deque<MessageChain>        d_writeQ{};
    IpcProtocol::ChannelId          d_id;
    SfmlIpcAdapter                 *d_adapter;
    bool                            d_listen;
    Address                         d_address;
    ChannelCb                       d_channelCb;
    MessageCb                       d_messageCb;
    Mode                            d_mode;

    Connection(IpcProtocol::ChannelId   id,
               SfmlIpcAdapter          *adapter,
               bool                     listen,
               Address                  address,
               const ChannelCb&         ccb,
               const MessageCb&         mcb)
        : d_id(id)
        , d_adapter(adapter)
        , d_listen(listen)
        , d_address(std::move(address))
        , d_channelCb(ccb)
        , d_messageCb(mcb)
        , d_mode(listen ? Mode::eACCEPTOR : Mode::eCREATOR) {
        d_socket.setBlocking(false);
    }

    ~Connection() {
        if (d_channelCb) {
            d_channelCb(d_id, d_status, d_mode);
        }

        if (d_reader.joinable()) {
            if (d_reader.get_id() == std::this_thread::get_id()) {
                d_reader.detach();
            }
            else {
                d_reader.join();
            }
        }

        if (d_writer.joinable()) {
            if (d_writer.get_id() == std::this_thread::get_id()) {
                d_writer.detach();
            }
            else {
                d_writer.join();
            }
        }
    }

    void lock() { d_lock.lock(); }

    void shutdown(ChannelChange status) noexcept {
        // caller must hold connection lock.
        d_shutdown = true;
        if (d_status == ChannelChange::eREADY) {
            d_status = status;
        }
        d_socket.disconnect();
        d_writeQ.clear();
        d_signal.notify_one();
    }

    void unlock() { d_lock.unlock(); }
};

                                //----------------------
                                // class SfmlIpcAdapter
                                //----------------------

SfmlIpcAdapter::~SfmlIpcAdapter()
{
    for (auto& pair : d_connections) {
        auto connection = pair.second.lock();

        if (connection) {
            connection->lock();

            connection->d_messageCb = MessageCb();
            connection->d_channelCb = ChannelCb();
            connection->shutdown(ChannelChange::eCLOSE);
            connection->d_self.reset();   // 'connection' COULD be the only ref.

            connection->unlock();

            // The only other join's are in the Connection destructor, so
            // those can't be called.

            if (connection->d_reader.joinable()) {
                connection->d_reader.join();
            }

            if (connection->d_writer.joinable()) {
                connection->d_writer.join();
            }
        }
    }
}

void
SfmlIpcAdapter::accept(Connection *connection, sf::TcpListener *listener)
{
    // A timed "wait" operation is used to poll the shutdown flag while
    // watching for an indication that the listen socket has something to
    // accept.
    listener->setBlocking(false);
    sf::SocketSelector select;
    select.add(*listener);

    auto guard = std::unique_lock(*connection);

    auto attempts = 0u;

    while (!connection->d_shutdown) { // exit/break from while with lock held!

        assert(++attempts < 1000);

        connection->unlock(); /*UNLOCK*/

        if (select.wait(sf::seconds(1.0f))) {
            auto status = listener->accept(connection->d_socket);

            connection->lock();/*LOCK*/

            auto cb = connection->d_channelCb;

            if (!cb) {
                break;                                                 // BREAK
            }

            if (sf::Socket::Done == status) {

                guard.release()->unlock(); /*UNLOCK*/

                cb(connection->d_id, ChannelChange::eREADY,connection->d_mode);
                readMsgLoop(connection);
                return;                                               // RETURN
            }

            if (sf::Socket::Error == status) {
                connection->shutdown(ChannelChange::eERROR);
                break;                                                 // BREAK
            }
        }
        else {
            connection->lock(); /*LOCK*/
        }
    }
    listener->close();

    return;                                                           // RETURN
}

IpcProtocol::ChannelStatus
SfmlIpcAdapter::acceptChannels(String_t  *diagnostic,
                               std::any         configuration)
                                                             noexcept
{
    return configureAdapter(diagnostic, configuration);
}

void
SfmlIpcAdapter::closeAdapter() noexcept
{
    auto guard = std::lock_guard(d_lock);
    d_shutdown = true;

    for (auto& pair : d_connections) {
        auto connection = pair.second.lock();
        
        if (connection) {
            connection->lock();

            connection->d_messageCb    = MessageCb();
            connection->d_channelCb = ChannelCb();
            connection->shutdown(ChannelChange::eCLOSE);
            connection->d_self.reset();
            
            connection->unlock();
        }
    }
    return;                                                           // RETURN
}

void
SfmlIpcAdapter::closeChannel(ChannelId id) noexcept
{
    auto guard = std::lock_guard(d_lock);
    auto it = d_connections.find(id.d_internalId);

    if (it != d_connections.end()) {
        auto connection = it->second.lock();
        
        connection->lock();

        connection->shutdown(ChannelChange::eCLOSE);
        d_connections.erase(it);
        
        connection->unlock();
    }
    return;                                                           // RETURN
}

void
SfmlIpcAdapter::connect(sf::IpAddress   ipV4,
                        unsigned short  port,
                        Connection     *connection)
{
    sf::SocketSelector select;
    select.add(connection->d_socket);

    auto guard = std::unique_lock(*connection);
    auto attempts = 0u;

    while (!connection->d_shutdown) {
        assert(++attempts < 1000);

        connection->unlock(); /*UNLOCK*/

        auto status = connection->d_socket.connect(ipV4, port);

        if (sf::Socket::Done == status) {

            connection->lock(); /*LOCK*/

            auto cb = connection->d_channelCb;

            if (cb) {
                guard.release()->unlock(); /*UNLOCK*/

                cb(connection->d_id, ChannelChange::eREADY,connection->d_mode);
                readMsgLoop(connection);
                return;                                               // RETURN
            }
        }
        else {
            if (sf::Socket::NotReady != status) {
                // Connect rejections are often immediate, so sleep before retry
                connection->d_socket.disconnect();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            else {
                select.wait(sf::seconds(1.0f)); // poll for shutdown.
            }

            connection->lock(); /*LOCK*/
        }
    }
    connection->d_socket.disconnect();

    connection->unlock();/*UNLOCK*/

    return;                                                           // RETURN
}

IpcProtocol::ChannelStatus
SfmlIpcAdapter::createNewChannel(String_t    *diagnostic,
                                 std::any     configuration) noexcept
{
    return configureAdapter(diagnostic, configuration);
}


void
SfmlIpcAdapter::installCallbacks(ChannelCb       connectionUpdate,
                                 MessageCb       receivedMessage) noexcept
{
    if (connectionUpdate && receivedMessage) {
        auto guard = std::lock_guard(d_lock);
        d_channelCb = connectionUpdate;
        d_messageCb = receivedMessage;
    }
    else {
        assert(!"Invalid callbacks installed.");
    }
    return;                                                           // RETURN
}

IpcProtocol::ChannelStatus
SfmlIpcAdapter::configureAdapter(Wawt::String_t *diagnostic,
                                 std::any        address) noexcept
{
    std::string string;
    // Preliminaries

    try {
        if (address.type() == typeid(std::string)) {
            string = std::any_cast<std::string>(address);
        }
        else if (address.type() == typeid(std::wstring)) {
            string = sf::String(
                        std::any_cast<std::wstring>(address)).toAnsiString();
        }
    }
    catch (...) { }

    if (string.empty()) {
        *diagnostic = S("Expected address to be a string.");
        return ChannelStatus::eMALFORMED;                           // RETURN
    }
    // Address parse (regex):
    bool        matched = false;
    std::smatch parse;

    try {
        matched = std::regex_match(string, parse, d_pattern);
    }
    catch (...) { }

    if (!matched) {
        *diagnostic = S("The address string is malformed.");
        return ChannelStatus::eMALFORMED;                           // RETURN
    }
    bool            listen = false;
    std::string     ipStr;
    std::string     portStr;

    if (parse.str(1) == "listen") {
        listen = true;
    }
    else if (parse.str(1) != "connect") {
        *diagnostic = S("The connection type is invalid.");
        return ChannelStatus::eINVALID;                             // RETURN
    }
    assert(parse.size() == 4);

    if (parse.str(3).empty()) {
        portStr = parse.str(2);
    }
    else {
        ipStr   = parse.str(2);
        portStr = parse.str(3);
    }
    std::size_t     pos;
    int             port;

    try {
        port = std::stoi(portStr, &pos);
    }
    catch (...) {
        port = -1;
    }

    if (port < 0 || port > UINT16_MAX || pos != portStr.length()) {
        *diagnostic
            = S("A port number between 0 and 65535 must be used.");
        return ChannelStatus::eINVALID;                             // RETURN
    }
    // This can result in a DNS lookup (this COULD be SLOW :-( ):
    sf::IpAddress   ipAddress = ipStr.empty() ? sf::IpAddress::Any
                                              : sf::IpAddress(ipStr);

    if (listen) {
        if (sf::IpAddress::Any               != ipAddress
         && sf::IpAddress::getLocalAddress() != ipAddress
         && sf::IpAddress(127u,0u,0u,1u)     != ipAddress) {
            *diagnostic
                = S("Can only bind listen to this computer.");
            return ChannelStatus::eINVALID;                         // RETURN
        }
    }
    else {
        if (sf::IpAddress::Any               == ipAddress
         && sf::IpAddress::Broadcast         == ipAddress) {
            *diagnostic
                = S("Cannot connect to broadcast or '0.0.0.0' address.");
            return ChannelStatus::eINVALID;                         // RETURN
        }

        if (sf::IpAddress::None == ipAddress) {
            *diagnostic = S("The destination host is malformed or unknown.");
            return ChannelStatus::eUNKNOWN;                         // RETURN
        }
    }

    auto guard = std::lock_guard(d_lock);

    if (d_shutdown) {
        *diagnostic = S("IPC is shutting down.");
        return ChannelStatus::eINVALID;                             // RETURN
    }
    unsigned short port_us = static_cast<unsigned short>(port);
    auto id = d_next++;
    auto connection
        = std::make_shared<Connection>(IpcProtocol::ChannelId(d_adapterId, id),
                                       this,
                                       listen,
                                       std::pair(port_us, ipAddress),
                                       d_channelCb,
                                       d_messageCb);
    connection->d_self = connection; // to keep "alive" until startConnection
    d_connections[id] = connection; // a weak pointer.

    return ChannelStatus::ePENDING;                                 // RETURN
}

void
SfmlIpcAdapter::readMsgLoop(Connection *connection)
{
    assert(sf::IpAddress::None != connection->d_socket.getRemoteAddress());

    sf::SocketSelector select;
    select.add(connection->d_socket);

    connection->lock(); /*LOCK*/

    connection->d_signal.notify_one(); // wake up the writer thread

    while (!connection->d_shutdown) {

        connection->unlock(); /*UNLOCK*/

        uint16_t size = readSizeHdr(select, connection);

        if (size > 0) {
            readMsg(select, connection, size);
        }

        connection->lock(); /*LOCK*/
    }

    connection->unlock(); /*UNLOCK*/
    return;                                                           // RETURN
}

std::size_t
SfmlIpcAdapter::readSizeHdr(sf::SocketSelector& select, Connection *connection)
{
    unsigned char header[4];
    std::size_t   bufsize = 0;
    auto attempts = 0u;
    auto code = ChannelChange::eREADY;

    auto guard = std::unique_lock(*connection);

    while (!connection->d_shutdown) {
        assert(++attempts < 1000);

        connection->unlock(); /*UNLOCK*/

        if (select.wait(sf::seconds(1.0f))) { // poll for shutdown.
            std::size_t readsize = 0;

            auto status = connection->d_socket.receive(header+bufsize,
                                                       sizeof(header)-bufsize,
                                                       readsize);

            connection->lock(); /*LOCK*/

            bufsize += readsize;

            if (sf::Socket::Done == status || sf::Socket::Partial == status) {
                if (bufsize == sizeof(header)) {
                    if (header[0] != BYTE1 || header[1] != BYTE2) {
                        code = ChannelChange::ePROTO;
                        break;                                         // BREAK
                    }
                    auto byte3      = uint16_t(header[2]);
                    auto byte4      = uint16_t(header[3]);
                    auto size       = (byte3 << 8)|byte4;
                    return size;                                      // RETURN
                }
            }
            else if (sf::Socket::Disconnected == status) {
                code = ChannelChange::eDROP;
                break;                                                 // BREAK
            }
            else if (sf::Socket::Error == status) {
                code = ChannelChange::eERROR;
                break;                                                 // BREAK
            }
        }
        else {
            connection->lock(); /*LOCK*/
        }
    }

    connection->shutdown(code);
    connection->unlock(); /*UNLOCK*/
    return 0;                                                         // RETURN
}

void
SfmlIpcAdapter::readMsg(sf::SocketSelector&     select,
                        Connection             *connection,
                        uint16_t                msgSize)
{
    auto        buffer      = std::make_unique<char[]>(msgSize);
    std::size_t bufsize     = 0;
    auto        code        = ChannelChange::eREADY;
    auto attempts = 0u;
    
    auto guard = std::unique_lock(*connection);

    while (!connection->d_shutdown) {
        assert(++attempts < 1000);

        connection->unlock();/*UNLOCK*/

        if (select.wait(sf::seconds(1.0f))) { // poll for shutdown.
            std::size_t readsize = 0;

            auto status = connection->d_socket.receive(buffer.get()+bufsize,
                                                       msgSize-bufsize,
                                                       readsize);

            connection->lock(); /*LOCK*/
            bufsize += readsize;

            if (sf::Socket::Done == status || sf::Socket::Partial == status) {
                auto cb = connection->d_messageCb;

                if (bufsize == msgSize && cb) {

                    guard.release()->unlock(); /*UNLOCK*/
                    
                    cb(connection->d_id,
                       IpcMessage(std::move(buffer), msgSize, 0));
                    return;                                           // RETURN
                }
            }
            else if (sf::Socket::Disconnected == status) {
                code = ChannelChange::eDROP;
                break;                                                 // BREAK
            }
            else if (sf::Socket::Error == status) {
                code = ChannelChange::eERROR;
                break;                                                 // BREAK
            }
        }
        else {
            connection->lock(); /*LOCK*/
        }
    }

    connection->shutdown(code);
    return;                                                           // RETURN
}

bool
SfmlIpcAdapter::sendMessage(ChannelId id, MessageChain&& chain) noexcept
{
    auto guard = std::lock_guard(d_lock);

    if (!d_shutdown) {
        auto it = d_connections.find(id.d_internalId);

        if (it != d_connections.end()) {
            auto connection = it->second.lock();

            if (connection) {
                auto guard2 = std::lock_guard(d_lock);

                if (!connection->d_self && !connection->d_shutdown) {
                    // Start was done but shutdown was not
                    bool empty = connection->d_writeQ.empty();

                    try {
                        connection->d_writeQ.emplace_front(std::move(chain));
                    }
                    catch(...) {
                        connection->shutdown(ChannelChange::eERROR);
                        return false;
                    }

                    if (empty) {
                        connection->d_signal.notify_one();
                    }
                    return true;                                      // RETURN
                }
            }
            else {
                d_connections.erase(it);
            }
        }
    }
    return false;                                                     // RETURN
}

bool
SfmlIpcAdapter::openAdapter(Wawt::String_t *diagnostic) noexcept
{
    auto guard = std::unique_lock(d_lock);

    if (d_shutdown) {
        *diagnostic = S("IPC is shutting down.");
        return false;                                                 // RETURN
    }
    auto it = d_connections.begin(); // TBF

    if (it == d_connections.end()) {
        *diagnostic = S("Invalid connection identifier.");
        return false;                                                 // RETURN
    }
    auto connection = it->second.lock(); // shared ptr

    if (!connection) {
        d_connections.erase(it);
        *diagnostic = S("Invalid connection identifier.");
        return false;                                                 // RETURN
    }
    connection->d_self.reset(); // Now connection deletes when threads exit.
    try {
        connection->d_writer = std::thread( [this, connection] {
                                                writeMsgLoop(connection.get());
                                            });
    }
    catch (...) {
    }

    if (!connection->d_writer.joinable()) {
        connection->d_channelCb = ChannelCb();
        d_connections.erase(it);
        *diagnostic = S("Failed to start message sender.");
        return false;                                                 // RETURN
    }
    auto [port, ipV4] = connection->d_address;

    std::function<void()> task;

    if (connection->d_listen) {
        auto listener     = std::make_shared<sf::TcpListener>();

        if (sf::Socket::Error == listener->listen(port, ipV4)) {
            *diagnostic =
                    S("Failed to listen on port. It may be already in use.");
            return false;                                             // RETURN
        }
        task =  [this, connection, listener] {
                    accept(connection.get(), listener.get());
                };
    }
    else {
        task = [this, port, ipV4, connection] {
                   connect(ipV4, port, connection.get());
               };
    }

    try {
        connection->d_reader  = std::thread(std::move(task));
    }
    catch (...) {
    }
    
    if (!connection->d_reader.joinable()) {
        auto guard2 = std::lock_guard(d_lock);
        connection->d_channelCb = ChannelCb();
        connection->shutdown(ChannelChange::eERROR); // wake-up writer
        d_connections.erase(it);
        *diagnostic = S("Failed to start connection.");
        return false;                                                 // RETURN
    }

    *diagnostic = S("");
    return true;                                                      // RETURN
}

bool
SfmlIpcAdapter::writeMsg(Connection            *connection,
                         const char            *message,
                         uint16_t               msgSize)
{
    std::size_t sent = 0;
    auto        code = ChannelChange::eREADY;

    do {
        std::size_t size = 0;

        sf::Socket::Status status = connection->d_socket.send(message+sent,
                                                              msgSize-sent,
                                                              size);
        sent += size;

        if (sf::Socket::Done == status || sf::Socket::Partial == status) {
            if (sent == msgSize) {
                return true;                                          // RETURN
            }
        }
        else if (sf::Socket::Disconnected   == status) {
            code = ChannelChange::eDROP;
            connection->lock(); /*LOCK*/
            break;                                                     // BREAK
        }
        else if (sf::Socket::Error          == status) {
            code = ChannelChange::eERROR;
            connection->lock(); /*LOCK*/
            break;                                                     // BREAK
        }

        if (size == 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        connection->lock(); /*LOCK*/

    } while (!connection->d_shutdown);

    connection->shutdown(code);

    connection->unlock(); /*UNLOCK*/

    return false;                                                     // RETURN
}

void
SfmlIpcAdapter::writeMsgLoop(Connection *connection)
{
    // Note: connection may not be established at this point.
    bool socketConnected = false;

    auto guard = std::unique_lock(connection->d_lock);

    while (!connection->d_shutdown) {
        if (connection->d_writeQ.empty()) {
            connection->d_signal.wait(guard);
            continue;                                               // CONTINUE
        }

        if (!socketConnected
         && sf::IpAddress::None == connection->d_socket.getRemoteAddress()) {
            connection->d_signal.wait(guard); // connection not established
            continue;                                               // CONTINUE
        }
        socketConnected = true;

        auto chain = std::move(connection->d_writeQ.back());
        connection->d_writeQ.pop_back();

        connection->unlock(); /*UNLOCK*/

        auto msgsize = uint16_t{};

        for (auto& fragment : chain) {
            msgsize += fragment.length();
        }

        char header[4];
        header[0] = BYTE1;
        header[1] = BYTE2;
        header[2] = char(msgsize >> 8);
        header[3] = char(msgsize);

        if (writeMsg(connection, header, sizeof(header))) {
            for (auto& fragment : chain) {
                writeMsg(connection, fragment.cbegin(), fragment.length());
            }
        }

        connection->lock(); /*LOCK*/
    }
    return;                                                           // RETURN
}

}  // namespace BDS

// vim: ts=4:sw=4:et:ai
