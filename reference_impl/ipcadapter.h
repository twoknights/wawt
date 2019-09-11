/** @file ipcadapter.h
 *  @brief Reference implementation adapting C++ networking extensions to Wawt.
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

#ifndef IPCADAPTER_H
#define IPCADAPTER_H

#include <wawt/ipcprotocol.h>

//#include <experimental/net>
//namespace net = std::experimental::net;
//using error_code = std::error_code;

#include <ts/net.hpp>
namespace net = boost::asio;
using error_code = boost::system::error_code;

namespace Ip  = net::ip;


#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <memory>
#include <regex>
#include <set>
#include <thread>
#include <tuple>
#include <variant>

                            //====================
                            // class TcpSocket
                            //====================

class TcpSocket : public Wawt::IpcProtocol::Channel {
  public:
    // PUBLIC TYPES
    using SetupTicket = Wawt::IpcProtocol::Provider::SetupTicket;
    using Socket      = Ip::tcp::socket;
    using DeleteCb    = std::function<void()>;

    // PUBLIC CLASS MEMBERS
    static constexpr std::size_t HEADERSIZE = 4u;

    // PUBLIC CONSTRUCTORS
    TcpSocket(Socket&& socket, DeleteCb&& deleteCb);

    // PUBLIC DESTRUCTORS
    ~TcpSocket();

    // PUBLIC Wawt::IpcProtocol::Channel INTERFACE
    void             closeChannel()                      noexcept override;

    void             completeSetup(MessageCb&&    receivedMessage,
                                   StateCb&&      channelClose)
                                                         noexcept override;

    //! Asynchronous call to send a message on a channel
    bool             sendMessage(MessageChain&&  chain)  noexcept override;

    State            state()                       const noexcept override;

    // PUBLIC MANIPULATORS
    void             readMessage()                       noexcept;

    void             setSelf(std::shared_ptr<TcpSocket>& self) noexcept {
        d_self = self;
    }

  private:
    // PRIVATE TYPES

    // PRIVATE MANIPULATORS
    void             readBody(error_code ec, std::size_t count) noexcept;

    void             readHeader(error_code  ec,
                                std::size_t count) noexcept;

    void             writeQ(error_code ec, std::size_t count);

    // PRIVATE DATA
    mutable std::mutex              d_lock;
    Wawt::IpcMessage                d_readBuf;
    Socket                          d_socket;
    DeleteCb                        d_deleteCb;
    std::shared_ptr<TcpSocket>      d_self;
    State                           d_state;
    StateCb                         d_stateCb;
    MessageCb                       d_messageCb;
    std::deque<Wawt::IpcMessage>    d_writeQ;
};

                            //==================
                            // class IpcProvider
                            //==================

class IpcProvider : public Wawt::IpcProtocol::Provider {
  public:
    // PUBLIC TYPES
    using Task       = std::function<void()>;
    using ChannelPtr = Wawt::IpcProtocol::ChannelPtr;

    // PUBLIC CLASS MEMBERS

    // PUBLIC CONSTRUCTORS
    IpcProvider();

    // PUBLIC DESTRUCTORS
    ~IpcProvider();

    // PUBLIC Wawt::IpcProtocol::Provider INTERFACE
    bool            acceptChannels(Wawt::String_t  *diagnostic,
                                   SetupTicket      ticket,
                                   SetupCb&&        channelSetupDone)
                                                           noexcept override;

    // On return, setup is not "in-progress", but may have "finished".
    bool            cancelSetup(const SetupTicket& ticket) noexcept override;

    //! Create a channel to a peer that is accepting channels.
    bool            createNewChannel(Wawt::String_t  *diagnostic,
                                     SetupTicket      ticket,
                                     SetupCb&&        channelSetupDone)
                                                           noexcept override;

    // Asynchronous cancel of all pending channels. No new ones permitted.
    void            shutdown()                             noexcept override;

    // PUBLIC MANIPULATORS

    // PUBLIC ACCESSORS
    unsigned short listenPort(const SetupTicket& ticket) const noexcept;

  private:
    // PRIVATE TYPES
    using SocketInfo     = std::variant<Ip::tcp::acceptor,Ip::tcp::socket>;
    using TicketInfo     = std::tuple<SetupCb,SocketInfo,unsigned short>;
    using TicketMap      = std::unordered_map<SetupTicket,TicketInfo>;
    using HandleEndpoint = std::function<void(TicketMap::iterator,
                                              const Ip::tcp::endpoint&)>;
    using Executor       = net::io_context::executor_type;

    // PRIVATE MANIPULATORS
    void            completeConnect(const SetupTicket&  ticket,
                                    error_code          error) noexcept;

    void            decrementChannelCount() noexcept;

    void            newChannel(Ip::tcp::socket&&   socket,
                               const SetupTicket&  ticket,
                               const SetupCb&      channelSetupDone) noexcept;

    void            resolveResults(error_code                       error,
                                   Ip::tcp::resolver::results_type  results,
                                   const SetupTicket&               ticket,
                                   const HandleEndpoint&            handleEp)
                                                                     noexcept;

    bool            setupError(TicketMap::iterator  it,
                               SetupBase::Status    status,
                               bool                 useThread = false)noexcept;

    // PRIVATE DATA
    mutable std::mutex                  d_lock;
    net::io_context                     d_context;
    Ip::tcp::resolver                   d_resolver;
    std::condition_variable             d_signalDestructorThread;
    TicketMap                           d_tickets;
    unsigned int                        d_channelCount;
    bool                                d_shutdown;
    net::executor_work_guard<Executor>  d_work;
    std::thread                         d_thread;
};

#endif
// vim: ts=4:sw=4:et:ai
