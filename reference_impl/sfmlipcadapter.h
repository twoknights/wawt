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

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <memory>
#include <regex>
#include <thread>

                            //=======================
                            // class SfmlIpV4Provider
                            //=======================

class SfmlIpV4Provider : public Wawt::IpcProtocol::Provider {
  public:
    // PUBLIC TYPES
    using Task     = std::function<void()>;
    using CancelCb = std::function<void()>;

    // PUBLIC CLASS MEMBERS

    // PUBLIC CONSTRUCTORS

    // PUBLIC DESTRUCTORS

    // PUBLIC Wawt::IpcProtocol::Provider INTERFACE
    bool            acceptChannels(Wawt::String_t  *diagnostic,
                                   SetupTicket      ticket,
                                   std::any         configuration,
                                   SetupCb&&        channelSetupDone)
                                                           noexcept override;

    // On return, setup is not "in-progress", but may have "finished".
    bool            cancelSetup(const SetupTicket& ticket) noexcept override;

    //! Create a channel to a peer that is accepting channels.
    bool            createNewChannel(Wawt::String_t  *diagnostic,
                                     SetupTicket      ticket,
                                     std::any         configuration,
                                     SetupCb&&        channelSetupDone)
                                                           noexcept override;

    // Asynchronous cancel of all pending channels. No new ones permitted.
    void            shutdown()                             noexcept override;

    // PUBLIC MANIPULATORS
    void            addTicket(const SetupTicket&    ticket,
                              unsigned short        port,
                              const CancelCb&       cancel) noexcept;

    bool            start(Wawt::String_t       *diagnostic,
                          SetupTicket           ticket,
                          const SetupCb&        channelSetupDone,
                          const sf::IpAddress&  ipAddress,
                          unsigned short        port) noexcept;

    bool            start(Wawt::String_t                   *diagnostic,
                          const SetupTicket&                ticket,
                          const SetupCb&                    channelSetupDone,
                          std::shared_ptr<sf::TcpListener>  listener) noexcept;

    // PUBLIC ACCESSORS
    unsigned short listenPort(const SetupTicket& ticket) const noexcept;

  private:
    // PRIVATE TYPES
    using   TicketCb   = std::pair<unsigned short, std::function<void()>>;
    using   TicketMap  = std::unordered_map<SetupTicket,TicketCb>;

    // PRIVATE MANIPULATORS

    // PRIVATE DATA
    mutable std::mutex              d_lock{};
    TicketMap                       d_tickets{};
    std::condition_variable         d_signalSetupThread{};
    bool                            d_shutdown      = false;
};

                            //====================
                            // class SfmlTcpSocket
                            //====================

class SfmlTcpSocket : public Wawt::IpcProtocol::Channel {
  public:
    // PUBLIC TYPES
    using Task        = SfmlIpV4Provider::Task;
    using SetupTicket = SfmlIpV4Provider::SetupTicket;

    // PUBLIC CLASS MEMBERS

    // PUBLIC CONSTRUCTORS
    SfmlTcpSocket();

    // PUBLIC DESTRUCTORS
    ~SfmlTcpSocket();

    // PUBLIC Wawt::IpcProtocol::Channel INTERFACE
    void            closeChannel()                      noexcept override;

    void            completeSetup(MessageCb&&    receivedMessage,
                                  StateCb&&      channelClose)
                                                        noexcept override;

    //! Asynchronous call to send a message on a channel
    bool            sendMessage(MessageChain&&  chain)  noexcept override;

    State           state()                       const noexcept override;

    // PUBLIC MANIPULATORS
    bool            connect(sf::IpAddress ipAddress, unsigned short port);

    void            readMsgLoop();

    sf::TcpSocket&  socket() {
        return d_socket;
    }

    bool            start(Wawt::String_t       *diagnostic,
                          const SetupTicket&    ticket,
                          const Task&           readerTask,
                          const Task&           writerTask);

    void            writeMsgLoop();

  private:
    // PRIVATE TYPES

    // PRIVATE MANIPULATORS
    std::size_t readSizeHdr(sf::SocketSelector& select);

    void        readMsg(sf::SocketSelector&     select,
                        uint16_t               msgSize);

    State       writeMsg(const char *message, uint16_t msgSize);

    // PRIVATE DATA
    std::mutex                      d_lock{};
    std::thread                     d_reader{};
    std::thread                     d_writer{};
    sf::TcpSocket                   d_socket{};
    std::condition_variable         d_signalWriteThread{};
    std::deque<MessageChain>        d_writeQ{};
    MessageCb                       d_messageCb{};
    StateCb                         d_stateCb{};
    std::atomic<State>              d_state;
};

#endif
// vim: ts=4:sw=4:et:ai
