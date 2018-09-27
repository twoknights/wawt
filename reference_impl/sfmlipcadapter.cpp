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
#include "sfmldrawadapter.h"

#include <SFML/Network/SocketSelector.hpp>
#include <SFML/Network/TcpListener.hpp>

#include <cassert>
#include <regex>

namespace BDS {

namespace {

static constexpr unsigned char BYTE1 = '\125';
static constexpr unsigned char BYTE2 = '\252';

} // end unnamed namespace

                                //----------------------
                                // class SfmlIpcAdapter
                                //----------------------
void
SfmlIpcAdapter::accept(Connection *connection, sf::TcpListener *listener)
{
    // A timed "wait" operation is used to poll the shutdown flag while
    // watching for an indication that the listen socket has something to
    // accept.
    listener->setBlocking(false);
    sf::SocketSelector select;
    select.add(*listener);

    connection->lock(); /*LOCK*/

    auto attempts = 0u;

    while (!connection->d_shutdown) { // break from while with lock held!

        assert(++attempts < 1000);

        connection->unlock(); /*UNLOCK*/

        if (select.wait(sf::seconds(1.0f))) {
            auto status = listener->accept(connection->d_socket);

            if (sf::Socket::Done == status) {
                connection->d_connectionCb(connection->d_id,
                                           ConnectionStatus::eOK);
                readMsgLoop(connection);
                return;                                           // RETURN
            }

            if (sf::Socket::Error == status) {
                connection->lock(); /*LOCK*/
                connection->shutdown(ConnectionStatus::eERROR);
                break;                                            // BREAK
            }
        }
        connection->lock(); /*LOCK*/
    }
    listener->close();

    connection->unlock();/*UNLOCK*/

    return;                                                           // RETURN
}

bool
SfmlIpcAdapter::closeConnection(ConnectionId id)
{
    std::unique_lock<std::mutex>  guard(d_lock);
    auto it = d_connections.find(id);

    if (it != d_connections.end()) {
        auto connection = it->second.lock();
        std::lock_guard<std::mutex> cn_guard(connection->d_lock);
        connection->shutdown(ConnectionStatus::eCLOSED);
        d_connections.erase(it);
        return true;                                                  // RETURN
    }
    return false;                                                     // RETURN
}

void
SfmlIpcAdapter::connect(sf::Uint32      ipV4,
                        unsigned short  port,
                        Connection     *connection)
{
    sf::IpAddress address(ipV4);

    sf::SocketSelector select;
    select.add(connection->d_socket);

    connection->lock(); /*LOCK*/
    auto attempts = 0u;

    while (!connection->d_shutdown) {
        assert(++attempts < 1000);

        connection->unlock(); /*UNLOCK*/

        auto status = connection->d_socket.connect(address, port);

        if (sf::Socket::Done == status) {
            connection->d_connectionCb(connection->d_id,
                                       ConnectionStatus::eOK);
            readMsgLoop(connection);
            return;                                                   // RETURN
        }

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
    connection->d_socket.disconnect();

    connection->unlock();/*UNLOCK*/

    return;                                                           // RETURN
}

WawtIpcProtocol::ConnectionId
SfmlIpcAdapter::establishConnection(std::string     *diagnostic,
                                    ConnectCallback  connectionUpdate,
                                    MessageCallback  messageCallback,
                                    const Address&   address)
{
    using IpPortPair = std::pair<sf::Uint32,unsigned short>;

    if (!connectionUpdate || !messageCallback) {
        *diagnostic = "Required functionals not supplied.";
        return 0;                                                     // RETURN
    }

    std::unique_lock<std::mutex>  guard(d_lock);
    auto connection = std::make_shared<Connection>(d_next,
                                                   std::move(connectionUpdate),
                                                   std::move(messageCallback));

    auto sender     = std::thread(  [this, connection] {
                                        writeMsgLoop(connection.get());
                                    });

    if (!sender.joinable()) {
        connection->d_connectionCb = ConnectCallback();
        *diagnostic = "Failed to start message sender.";
        return 0;                                                     // RETURN
    }
    sender.detach();

    std::function<void()> task;

    if (address.type() == typeid(unsigned short)) {
        auto port = std::any_cast<unsigned short>(address);
        auto l    = std::make_unique<sf::TcpListener>();

        if (sf::Socket::Error == l->listen(port)) {
            *diagnostic =
                    "Failed to listen on port. It may be already in use.";
            return 0;                                                 // RETURN
        }
        task =  [this, connection, lp = l.release()] {
                    std::unique_ptr<sf::TcpListener> listener(lp);
                    accept(connection.get(), listener.get());
                };
    }
    else if (address.type() == typeid(IpPortPair)) {
        auto [ipV4, port] = std::any_cast<IpPortPair>(address);
        task = [this, ipV4, port, connection] {
                   connect(ipV4, port, connection.get());
               };
    }
    else {
        *diagnostic = "Unrecognized address.";
        return 0;                                                     // RETURN
    }
    auto reader  = std::thread(task);
    
    if (!reader.joinable()) {
        std::lock_guard<std::mutex> cn_guard(connection->d_lock);
        connection->d_connectionCb = ConnectCallback();
        connection->shutdown(ConnectionStatus::eERROR);
        *diagnostic = "Failed to start connection.";
        return 0;                                                     // RETURN
    }
    reader.detach();

    d_connections.emplace(d_next, connection).first;
    *diagnostic = "Waiting on connection.";
    return d_next++;                                                  // RETURN
}

WawtIpcProtocol::AddressStatus
SfmlIpcAdapter::makeAddress(Address        *address,
                            std::string    *errorMessage,
                            std::any        directive)
{
    // handle [0-9]+ as listen port, regex match as connect adress.
    std::string string;
    
    try {
        if (directive.type() == typeid(std::string)) {
            string = std::any_cast<std::string>(directive);
        }
        else if (directive.type() == typeid(std::wstring)) {
            string = SfmlDrawAdapter
                        ::toAnsiString(std::any_cast<std::wstring>(directive));
        }
    }
    catch (...) {
    }

    if (string.empty()) {
        *errorMessage = "Expected address to be a string.";
        return WawtIpcProtocol::AddressStatus::eMALFORMED;
    }

    if (string.find_first_not_of("0123456789") == std::string::npos) {
        int port = std::stoi(string);

        if (port < 0 || port > UINT16_MAX) {
            *errorMessage = "A port number between 0 and 65535 must be used.";
            return WawtIpcProtocol::AddressStatus::eINVALID;
        }
        *address = static_cast<unsigned short>(port);
    }
    else {
        std::size_t pos;
        std::smatch addressAndPort;
        int         port;
        // parse based on <host>:<port>:

        if (!std::regex_match(string, addressAndPort, d_addressRegex)
         || (port = std::stoi(addressAndPort.str(2),&pos)
             , pos != addressAndPort.str(2).length())) {
            *errorMessage = "The destination string is malformed.";
            return WawtIpcProtocol::AddressStatus::eMALFORMED;
        }
        // This can result in a DNS lookup:
        sf::IpAddress ipV4(addressAndPort.str(1));

        if (sf::IpAddress::Any       == ipV4
         || sf::IpAddress::Broadcast == ipV4
         || port < 0
         || port > UINT16_MAX) {
            *errorMessage = "The destination string is invalid.";
            return WawtIpcProtocol::AddressStatus::eINVALID;
        }

        if (sf::IpAddress::None == ipV4) {
            *errorMessage = "The destination host is unknown.";
            return WawtIpcProtocol::AddressStatus::eUNKNOWN;
        }
        sf::Uint32 ip = ipV4.toInteger();
        *address = std::pair(ip, static_cast<unsigned short>(port));
    }
    return AddressStatus::eOK;
}

void
SfmlIpcAdapter::readMsgLoop(Connection *connection)
{
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
    auto code = ConnectionStatus::eOK;

    do {
        assert(++attempts < 1000);

        if (select.wait(sf::seconds(1.0f))) { // poll for shutdown.
            std::size_t readsize = 0;

            auto status = connection->d_socket.receive(header+bufsize,
                                                       sizeof(header)-bufsize,
                                                       readsize);

            bufsize += readsize;

            if (sf::Socket::Done == status || sf::Socket::Partial == status) {
                if (bufsize == sizeof(header)) {
                    if (header[0] != BYTE1 || header[1] != BYTE2) {
                        connection->lock(); /*LOCK*/
                        break;                                         // BREAK
                    }
                    auto byte3      = uint16_t(header[2]);
                    auto byte4      = uint16_t(header[3]);
                    auto size       = (byte3 << 8)|byte4;
                    return size;                                      // RETURN
                }
            }
            else if (sf::Socket::Disconnected == status) {
                code = ConnectionStatus::eDISCONNECT;
                connection->lock(); /*LOCK*/
                break;                                                 // BREAK
            }
            else if (sf::Socket::Error == status) {
                code = ConnectionStatus::eERROR;
                connection->lock(); /*LOCK*/
                break;                                                 // BREAK
            }
        }
        connection->lock(); /*LOCK*/
    }
    while (!connection->d_shutdown);

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
    auto        code        = ConnectionStatus::eOK;
    auto attempts = 0u;

    do {
        assert(++attempts < 1000);

        if (select.wait(sf::seconds(1.0f))) { // poll for shutdown.
            std::size_t readsize = 0;

            auto status = connection->d_socket.receive(buffer.get()+bufsize,
                                                       msgSize-bufsize,
                                                       readsize);

            bufsize += readsize;

            if (sf::Socket::Done == status || sf::Socket::Partial == status) {
                if (bufsize == msgSize) {
                    connection->d_messageCb(connection->d_id,
                                            std::pair(std::move(buffer),
                                                      msgSize));
                    return;                                           // RETURN
                }
            }
            else if (sf::Socket::Disconnected == status) {
                code = ConnectionStatus::eDISCONNECT;
                connection->lock(); /*LOCK*/
                break;                                                 // BREAK
            }
            else if (sf::Socket::Error == status) {
                code = ConnectionStatus::eERROR;
                connection->lock(); /*LOCK*/
                break;                                                 // BREAK
            }
        }
        connection->lock(); /*LOCK*/
    }
    while (!connection->d_shutdown);

    connection->shutdown(code);
    connection->unlock(); /*UNLOCK*/
    return;                                                           // RETURN
}

bool
SfmlIpcAdapter::sendMessage(ConnectionId id, Message&& message)
{
    std::lock_guard<std::mutex>  guard(d_lock);
    auto it = d_connections.find(id);

    if (it != d_connections.end()) {
        auto connection = it->second.lock();
        std::lock_guard<std::mutex> cn_guard(connection->d_lock);

        if (!connection->d_shutdown) {
            bool empty = connection->d_writeQ.empty();
            connection->d_writeQ.emplace_front(std::move(message));

            if (empty) {
                connection->d_signal.notify_one();
            }
            return true;                                              // RETURN
        }
    }
    return false;                                                     // RETURN
}

bool
SfmlIpcAdapter::writeMsg(Connection            *connection,
                         const char            *message,
                         uint16_t               msgSize)
{
    std::size_t sent = 0;
    auto        code = ConnectionStatus::eOK;

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
            code = ConnectionStatus::eDISCONNECT;
            connection->lock(); /*LOCK*/
            break;                                                     // BREAK
        }
        else if (sf::Socket::Error          == status) {
            code = ConnectionStatus::eERROR;
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
        header[3] = char(msgsize >> 8);
        header[4] = char(msgsize & 0377);

        if (writeMsg(connection, header, sizeof(header))) {
            writeMsg(connection, uptr.get(), msgsize);
        }

        connection->lock(); /*LOCK*/
    }
    return;                                                           // RETURN
}

}  // namespace BDS

// vim: ts=4:sw=4:et:ai
