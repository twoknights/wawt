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
#include <iostream>

#undef  S
#undef  C
#define S(str) Wawt::String_t(u8"" str)      // UTF8 strings  (std::string)
#define C(c) (u8 ## c)
//#define S(str) Wawt::String_t(L"" str)  // wide char strings (std::wstring)
//#define C(c) (L ## c)

namespace BDS {

namespace {

static constexpr unsigned char BYTE1 = '\125';
static constexpr unsigned char BYTE2 = '\252';

} // end unnamed namespace


                        //=================================
                        // class SfmlIpcAdapter::Connection
                        //=================================

struct SfmlIpcAdapter::Connection {
    using Address = std::pair<unsigned short, sf::IpAddress>;

    std::thread                     d_reader;
    std::thread                     d_writer;
    sf::TcpSocket                   d_socket;
    std::shared_ptr<Connection>     d_self;
    ConnectStatus                   d_status    = ConnectStatus::eOK;
    bool                            d_shutdown  = false;
    std::mutex                      d_lock{};
    std::condition_variable         d_signal{};
    std::deque<Message>             d_writeQ{};
    ConnectionId                    d_id;
    SfmlIpcAdapter                 *d_adapter;
    bool                            d_listen;
    Address                         d_address;
    ConnectCb                       d_connectionCb;
    MessageCb                       d_messageCb;

    Connection(ConnectionId         id,
               SfmlIpcAdapter      *adapter,
               bool                 listen,
               Address              address,
               ConnectCb&&          ccb,
               MessageCb&&          mcb)
        : d_id(id)
        , d_adapter(adapter)
        , d_listen(listen)
        , d_address(std::move(address))
        , d_connectionCb(std::move(ccb))
        , d_messageCb(std::move(mcb)) {
        d_socket.setBlocking(false);
    }

    ~Connection() {
        if (d_connectionCb) {
            d_connectionCb(d_id, d_status);
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

    void shutdown(ConnectStatus status) noexcept {
        // caller must hold connection lock.
        d_shutdown = true;
        if (d_status == ConnectStatus::eOK) {
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

            connection->d_messageCb    = MessageCb();
            connection->d_connectionCb = ConnectCb();
            connection->shutdown(ConnectStatus::eCLOSED);
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

    std::unique_lock<Connection> guard(*connection);

    auto attempts = 0u;

    while (!connection->d_shutdown) { // exit/break from while with lock held!

        assert(++attempts < 1000);

        connection->unlock(); /*UNLOCK*/

        if (select.wait(sf::seconds(1.0f))) {
            auto status = listener->accept(connection->d_socket);

            connection->lock();/*LOCK*/

            auto cb = connection->d_connectionCb;

            if (!cb) {
                break;                                                 // BREAK
            }

            if (sf::Socket::Done == status) {

                guard.release()->unlock(); /*UNLOCK*/

                cb(connection->d_id, ConnectStatus::eOK);
                readMsgLoop(connection);
                return;                                               // RETURN
            }

            if (sf::Socket::Error == status) {
                connection->shutdown(ConnectStatus::eERROR);
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

void
SfmlIpcAdapter::closeAll()
{
    std::lock_guard<std::mutex>  guard(d_lock);
    d_shutdown = true;

    for (auto& pair : d_connections) {
        auto connection = pair.second.lock();
        
        if (connection) {
            connection->lock();

            connection->d_messageCb    = MessageCb();
            connection->d_connectionCb = ConnectCb();
            connection->shutdown(ConnectStatus::eCLOSED);
            connection->d_self.reset();
            
            connection->unlock();
        }
    }
    return;                                                           // RETURN
}

void
SfmlIpcAdapter::closeConnection(ConnectionId id)
{
    std::lock_guard<std::mutex>  guard(d_lock);
    auto it = d_connections.find(id);

    if (it != d_connections.end()) {
        auto connection = it->second.lock();
        
        connection->lock();

        connection->shutdown(ConnectStatus::eCLOSED);
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

    std::unique_lock<Connection> guard(*connection);
    auto attempts = 0u;

    while (!connection->d_shutdown) {
        assert(++attempts < 1000);

        connection->unlock(); /*UNLOCK*/

        auto status = connection->d_socket.connect(ipV4, port);

        if (sf::Socket::Done == status) {

            connection->lock(); /*LOCK*/

            auto cb = connection->d_connectionCb;

            if (cb) {
                guard.release()->unlock(); /*UNLOCK*/

                cb(connection->d_id, ConnectStatus::eOK);
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

WawtIpcProtocol::AddressStatus
SfmlIpcAdapter::prepareConnection(Wawt::String_t *diagnostic,
                                  ConnectionId   *id,
                                  ConnectCb       connectionUpdate,
                                  MessageCb       receivedMessage,
                                  std::any        address)
{
    std::string string;
    // Preliminaries

    if (!connectionUpdate || !receivedMessage) {
        *diagnostic = S("IPC is shutting down.");
        return AddressStatus::eINVALID;                               // RETURN
    }
    
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
        return AddressStatus::eMALFORMED;                             // RETURN
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
        return AddressStatus::eMALFORMED;                             // RETURN
    }
    bool            listen = false;
    std::string     ipStr;
    std::string     portStr;

    if (parse.str(1) == "listen") {
        listen = true;
    }
    else if (parse.str(1) != "connect") {
        *diagnostic = S("The connection type is invalid.");
        return AddressStatus::eINVALID;                               // RETURN
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
        return AddressStatus::eINVALID;                               // RETURN
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
            return AddressStatus::eINVALID;                           // RETURN
        }
    }
    else {
        if (sf::IpAddress::Any               == ipAddress
         && sf::IpAddress::Broadcast         == ipAddress) {
            *diagnostic
                = S("Cannot connect to broadcast or '0.0.0.0' address.");
            return AddressStatus::eINVALID;                           // RETURN
        }

        if (sf::IpAddress::None == ipAddress) {
            *diagnostic = S("The destination host is malformed or unknown.");
            return AddressStatus::eUNKNOWN;                           // RETURN
        }
    }

    std::lock_guard<std::mutex>  guard(d_lock);

    if (d_shutdown) {
        *diagnostic = S("IPC is shutting down.");
        return AddressStatus::eINVALID;                               // RETURN
    }
    unsigned short port_us = static_cast<unsigned short>(port);
    *id = d_next++;
    auto connection
        = std::make_shared<Connection>(*id,
                                       this,
                                       listen,
                                       std::pair(port_us, ipAddress),
                                       std::move(connectionUpdate),
                                       std::move(receivedMessage));
    connection->d_self = connection; // to keep "alive" until startConnection
    d_connections[*id] = connection; // a weak pointer.

    return AddressStatus::eOK;
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
    auto code = ConnectStatus::eOK;

    std::unique_lock<Connection> guard(*connection);

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
                        code = ConnectStatus::ePROTCOL;
                        break;                                         // BREAK
                    }
                    auto byte3      = uint16_t(header[2]);
                    auto byte4      = uint16_t(header[3]);
                    auto size       = (byte3 << 8)|byte4;
                    return size;                                      // RETURN
                }
            }
            else if (sf::Socket::Disconnected == status) {
                code = ConnectStatus::eDISCONNECT;
                break;                                                 // BREAK
            }
            else if (sf::Socket::Error == status) {
                code = ConnectStatus::eERROR;
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
    auto        code        = ConnectStatus::eOK;
    auto attempts = 0u;
    
    std::unique_lock<Connection> guard(*connection);

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
                    
                    cb(connection->d_id, std::pair(std::move(buffer),msgSize));
                    return;                                           // RETURN
                }
            }
            else if (sf::Socket::Disconnected == status) {
                code = ConnectStatus::eDISCONNECT;
                break;                                                 // BREAK
            }
            else if (sf::Socket::Error == status) {
                code = ConnectStatus::eERROR;
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
SfmlIpcAdapter::sendMessage(ConnectionId id, Message&& message)
{
    std::lock_guard<std::mutex>  guard(d_lock);

    if (!d_shutdown) {
        auto it = d_connections.find(id);

        if (it != d_connections.end()) {
            auto connection = it->second.lock();

            if (connection) {
                std::lock_guard<Connection> cn_guard(*connection);

                if (!connection->d_self && !connection->d_shutdown) {
                    // Start was done but shutdown was not
                    bool empty = connection->d_writeQ.empty();

                    try {
                        connection->d_writeQ.emplace_front(std::move(message));
                    }
                    catch(...) {
                        connection->shutdown(ConnectStatus::eERROR);
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
SfmlIpcAdapter::startConnection(Wawt::String_t *diagnostic, ConnectionId id)
{
    std::unique_lock<std::mutex>  guard(d_lock);

    if (d_shutdown) {
        *diagnostic = S("IPC is shutting down.");
        return false;                                                 // RETURN
    }
    auto it = d_connections.find(id);

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
        connection->d_connectionCb = ConnectCb();
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
        std::lock_guard<Connection> cn_guard(*connection);
        connection->d_connectionCb = ConnectCb();
        connection->shutdown(ConnectStatus::eERROR); // wake-up writer
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
    auto        code = ConnectStatus::eOK;

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
            code = ConnectStatus::eDISCONNECT;
            connection->lock(); /*LOCK*/
            break;                                                     // BREAK
        }
        else if (sf::Socket::Error          == status) {
            code = ConnectStatus::eERROR;
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

    std::unique_lock<std::mutex> guard(connection->d_lock);

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

        auto [uptr, msgsize] = std::move(connection->d_writeQ.back());
        connection->d_writeQ.pop_back();

        connection->unlock(); /*UNLOCK*/

        char header[4];
        header[0] = BYTE1;
        header[1] = BYTE2;
        header[2] = char(msgsize >> 8);
        header[3] = char(msgsize & 0377);

        if (writeMsg(connection, header, sizeof(header))) {
            writeMsg(connection, uptr.get(), msgsize);
        }

        connection->lock(); /*LOCK*/
    }
    return;                                                           // RETURN
}

}  // namespace BDS

// vim: ts=4:sw=4:et:ai
