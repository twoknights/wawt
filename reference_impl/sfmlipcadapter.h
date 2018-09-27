/** @file sfmlipcadapter.h
 *  @brief Adapts SFML Graphics and Events to Wawt.
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

#ifndef BDS_SFMLIPCADAPTER_H
#define BDS_SFMLIPCADAPTER_H

#include <wawtipcprotocol.h>

#include <SFML/Network/TcpSocket.hpp>

#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <memory>
#include <regex>

namespace BDS {

                            //=====================
                            // class SfmlIpcAdapter
                            //=====================

class SfmlIpcAdapter : public WawtIpcProtocol {
  public:
    // PUBLIC CLASS MEMBERS

    // PUBLIC CREATORS

    // PUBLIC WawtIpcAdapter INTERFACE
    bool closeConnection(ConnectionId id)                             override;

    //! Synchronous call to form a connection address from "instructions".
    AddressStatus  makeAddress(Address             *address,
                               std::string         *errorMessage,
                               std::any             directions)       override;

    //! Asynchronous call for "mutual" message exchange.
    bool sendMessage(ConnectionId               id,
                     Message&&                  message)              override;

    //! Asynchronous establishment of a connection.
    ConnectionId establishConnection(std::string     *diagnostic,
                                     ConnectCallback  connectionUpdate,
                                     MessageCallback  receivedMessage,
                                     const Address&   address)        override;


    // PUBLIC ACCESSORS

  private:
    // PRIVATE TYPES
    struct Connection {
        ConnectionStatus                d_status = ConnectionStatus::eOK;
        bool                            d_shutdown = false;
        int                             d_id       = 0;
        std::mutex                      d_lock{};
        std::condition_variable         d_signal{};
        std::deque<Message>             d_writeQ{};
        ConnectCallback                 d_connectionCb{};
        MessageCallback                 d_messageCb{};
        sf::TcpSocket                   d_socket;

        Connection(ConnectionId         id,
                   ConnectCallback&&    ccb,
                   MessageCallback&&    mcb)
            : d_id(id)
            , d_connectionCb(std::move(ccb))
            , d_messageCb(std::move(mcb)) {
            d_socket.setBlocking(false);
        }

        ~Connection() {
            if (d_connectionCb) {
                d_connectionCb(d_id, d_status);
            }
        }

        void lock() { d_lock.lock(); }

        void shutdown(ConnectionStatus status) {
            // caller must hold connection lock.
            d_shutdown = true;
            if (d_status == ConnectionStatus::eOK) {
                d_status = status;
            }
            d_socket.disconnect();
            d_writeQ.clear();
            d_signal.notify_one();
        }

        void unlock() { d_lock.unlock(); }
    };
    using ConnectionPtr             = std::weak_ptr<Connection>;
    using ConnectionMap             = std::map<ConnectionId,ConnectionPtr>;

    // PRIVATE MANIPULATORS
    void accept(Connection *connection, sf::TcpListener *listener);

    void connect(sf::Uint32 ipV4, unsigned short port, Connection *connection);

    void readMsgLoop(Connection *connection);

    std::size_t readSizeHdr(sf::SocketSelector&  select,
                            Connection          *connection);

    void readMsg(sf::SocketSelector&     select,
                 Connection             *connection,
                 uint16_t                msgSize);

    bool writeMsg(Connection            *connection,
                  const char            *message,
                  uint16_t               msgSize);

    void writeMsgLoop(Connection *connection);


    // PRIVATE DATA
    std::mutex                      d_lock{};
    ConnectionMap                   d_connections{};
    ConnectionId                    d_next          = 1;
    std::regex                      d_addressRegex{R"(^([a-z.\-\d]+):(\d+)$)"};
};

} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai
