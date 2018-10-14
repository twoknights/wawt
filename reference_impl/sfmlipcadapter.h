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

#ifndef SFMLIPCADAPTER_H
#define SFMLIPCADAPTER_H

#include <wawt/ipcprotocol.h>

#include <SFML/Network/TcpSocket.hpp>

#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <memory>
#include <regex>

                            //=====================
                            // class SfmlIpcAdapter
                            //=====================

class SfmlIpcAdapter : public Wawt::IpcProtocol {
  public:
    // PUBLIC TYPES

    // PUBLIC CLASS MEMBERS

    // PUBLIC CONSTRUCTORS
    SfmlIpcAdapter(uint8_t adapterId) : d_adapterId(adapterId) { }

    // PUBLIC DESTRUCTORS
    ~SfmlIpcAdapter();

    // PUBLIC WawtIpcAdapter INTERFACE

    ChannelStatus   acceptChannels(Wawt::String_t  *diagnostic,
                                   std::any         configuration)
                                                             noexcept override;

    // Asynchronous close of all channels. No new ones permitted.
    void            closeAdapter()                           noexcept override;

    //! Asynchronous close of a channel.
    void            closeChannel(ChannelId      id)          noexcept override;

    //! Synchronous call. If adapter permits, may be called more than once.
    ChannelStatus   createNewChannel(Wawt::String_t    *diagnostic,
                                     std::any           configuration)
                                                             noexcept override;

    void            installCallbacks(ChannelCb  channelUpdate,
                                     MessageCb  receivedMessage)
                                                             noexcept override;

    //! Enables the asynchronous creation of new channels.
    bool            openAdapter(Wawt::String_t *diagnostic)  noexcept override;

    //! Asynchronous call to send a message on a channel
    bool            sendMessage(ChannelId       id,
                                MessageChain&&  chain)       noexcept override;

    // PUBLIC ACCESSORS
    unsigned short listenPort() const { return 0; }

  private:
    // PRIVATE TYPES
    struct  Connection;

    using   ConnectionPtr           = std::weak_ptr<Connection>;
    using   ConnectionMap           = std::map<int,ConnectionPtr>;

    // PRIVATE MANIPULATORS
    void        accept(Connection                  *connection,
                       sf::TcpListener             *listener);

    void        connect(sf::IpAddress               ipV4,
                        unsigned short              port,
                        Connection                 *connection);

    ChannelStatus configureAdapter(Wawt::String_t *diagnostic,
                                   std::any        address) noexcept;

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
    ChannelCb                       d_channelCb{};
    MessageCb                       d_messageCb{};
    ConnectionMap                   d_connections{};
    bool                            d_shutdown      = false;
    int                             d_next          = 0;
    std::regex                      d_pattern{ // 1=type 2=ip|port 3=port|""
          R"(^(connect|listen)=([a-z\.\-\d]+)(?:\:(\d+))?$)"};
    uint8_t                         d_adapterId;
};

#endif
// vim: ts=4:sw=4:et:ai
