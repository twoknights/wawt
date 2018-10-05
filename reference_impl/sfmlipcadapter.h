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

class SfmlIpcAdapter : public WawtIpcConnectionProtocol {
  public:
    // PUBLIC TYPES

    // PUBLIC CLASS MEMBERS

    // PUBLIC CONSTRUCTORS

    // PUBLIC DESTRUCTORS
    ~SfmlIpcAdapter();

    // PUBLIC WawtIpcAdapter INTERFACE
    // No callbacks invoked in the calling thread's context.
    void            closeConnection(ConnectionId        id)           noexcept;

    AddressStatus   prepareConnection(Wawt::String_t   *diagnostic,
                                      ConnectionId     *id,
                                      ConnectCb         connectionUpdate,
                                      MessageCb         receivedMessage,
                                      std::any          address)      noexcept;

    // No callbacks invoked in the calling thread's context.
    bool            sendMessage(ConnectionId            id,
                                Message&&               message)      noexcept;

    void            closeAll()                                        noexcept;

    bool            startConnection(Wawt::String_t     *diagnostic,
                                    ConnectionId        connectionId) noexcept;


    // PUBLIC ACCESSORS

  private:
    // PRIVATE TYPES
    struct  Connection;

    using   ConnectionPtr           = std::weak_ptr<Connection>;
    using   ConnectionMap           = std::map<ConnectionId,ConnectionPtr>;

    // PRIVATE MANIPULATORS
    void        accept(Connection                  *connection,
                       sf::TcpListener             *listener);

    void        connect(sf::IpAddress               ipV4,
                        unsigned short              port,
                        Connection                 *connection);

    void        readMsgLoop(Connection             *connection);

    std::size_t readSizeHdr(sf::SocketSelector&     select,
                            Connection             *connection);

    void        readMsg(sf::SocketSelector&         select,
                        Connection                 *connection,
                        uint16_t                    msgSize);

    bool        writeMsg(Connection                *connection,
                         const char                *message,
                         uint16_t                   msgSize);

    void        writeMsgLoop(Connection            *connection);


    // PRIVATE DATA
    std::mutex                      d_lock{};
    ConnectionMap                   d_connections{};
    bool                            d_shutdown      = false;
    ConnectionId                    d_next          = 0;
    std::regex                      d_pattern{ // 1=type 2=ip|port 3=port|""
          R"(^(connect|listen)=([a-z\.\-\d]+)(?:\:(\d+))?$)"};
};

} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai
