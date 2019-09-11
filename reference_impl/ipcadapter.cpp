/** @file ipcadapter.cpp
 *  @brief Implements Wawt::IpcProtocol using C++ networking extensions.
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

#include "ipcadapter.h"

#include <cassert>
#include <regex>
#include <system_error>
#include <thread>
#include <iostream>

using namespace Wawt;

#ifdef WAWT_WIDECHAR
#define S(str) (L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) (u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

namespace {

static constexpr unsigned char BYTE1 = '\125';
static constexpr unsigned char BYTE2 = '\252';

bool anyToHostPort(String_t          *diagnostic,
                   std::string       *host,
                   std::string       *port, // or service name
                   const std::any&    address) {
    auto string = std::string();

    try {
        if (address.type() == typeid(std::string)) {
            string = std::any_cast<std::string>(address);
        }
        else if (address.type() == typeid(std::wstring)) {
            auto wstring = std::any_cast<std::wstring>(address);
            for (auto it = wstring.begin(); it != wstring.end(); ) {
                auto ch = char(*it++);

                if ((ch & 0200) == 0) {
                    string += ch;
                }
                else {
                    string.clear();
                    break;                                             // BREAK
                }
            }
        }
    }
    catch (...) { }

    if (string.empty()) {
        *diagnostic = S("Expected address to be a string.");
        return false;                                                 // RETURN
    }
    auto pos = string.rfind(":");

    if (std::string::npos == pos) {
        *port = string;
        host->clear();
    }
    else {
        *port = string.substr(pos+1);

        if (port->empty()) {
            *diagnostic = S("Expected address to have a port.");
            return false;                                             // RETURN
        }
        *host = string.substr(0, pos);
    }
    *diagnostic = S("");
    return true;                                                      // RETURN
}

} // end unnamed namespace

                                //--------------------
                                // class TcpSocket
                                //--------------------

// PRIVATE MEMBERS
void
TcpSocket::readBody(error_code ec, std::size_t count) noexcept
{
    auto guard = std::unique_lock(d_lock);

    if (d_state == IpcProtocol::Channel::eREADY) {
        if (ec) {
            d_state = IpcProtocol::Channel::eERROR;
            auto tmp = d_self;
            d_self.reset();
            guard.release()->unlock();
            return;                                                   // RETURN
        }
        d_readBuf   += count;
        auto bytes   = d_readBuf.length();

        if (0 < bytes) {
            d_socket.async_receive(net::buffer(d_readBuf.data(), bytes),
                                   [me = d_self](auto code, auto n) -> void {
                                       me->readBody(code, n);
                                   });
            return;                                                   // RETURN
        }
        d_readBuf.d_offset = 0u;
        d_messageCb(std::move(d_readBuf));
    }
    return;                                                           // RETURN
}

void
TcpSocket::readHeader(error_code ec, std::size_t count) noexcept
{
    auto guard = std::unique_lock(d_lock);

    if (d_state == IpcProtocol::Channel::eREADY) {
        if (ec) {
            d_state = IpcProtocol::Channel::eERROR;
            auto tmp = d_self;
            d_self.reset();
            guard.release()->unlock();
            return;                                                   // RETURN
        }
        d_readBuf   += count;
        auto bytes   = d_readBuf.length();
        auto readCb  = std::function<void(error_code, std::size_t)>{};

        if (0 < bytes) { // header has NOT been read
            readCb =
                [me = d_self](error_code code, std::size_t n) -> void {
                    me->readHeader(code, n);
                };
        }
        else {
            auto header = d_readBuf.data();

            if (header[0] != BYTE1 || header[1] != BYTE2) {
                d_state = IpcProtocol::Channel::ePROTO;
                auto tmp = d_self;
                d_self.reset();
                guard.release()->unlock();
                return;                                               // RETURN
            }
            auto byte3 = uint16_t(header[2]);
            auto byte4 = uint16_t(header[3]);
            bytes      = (byte3 << 8)|byte4;
            d_readBuf  = IpcMessage(bytes);
            readCb     =
                [me = d_self](error_code code, std::size_t n) -> void {
                    me->readBody(code, n);
                };
        }
        d_socket.async_receive(net::buffer(d_readBuf.data(), bytes), readCb);
    }
    return;                                                           // RETURN
}

void
TcpSocket::writeQ(error_code ec, std::size_t count)
{
    auto guard = std::unique_lock(d_lock);

    if (d_state == IpcProtocol::Channel::eREADY) {
        if (ec) {
            d_state = IpcProtocol::Channel::eERROR;
            auto tmp = d_self;
            d_self.reset();
            guard.release()->unlock();
            return;                                                   // RETURN
        }
        auto *message = &d_writeQ.front();

        if (count < message->length()) {
            *message += count;
        }
        else {
            d_writeQ.pop_front();

            if (d_writeQ.empty()) {
                return;                                               // RETURN
            }
            message = &d_writeQ.front();
        }
        auto bufsize = std::size_t{message->length()};

        d_socket.async_send(net::buffer(message->data(), bufsize),
                            [this](auto code, auto n) { writeQ(code, n); });
    }
    return;                                                           // RETURN
}

// PUBLIC MEMBERS
TcpSocket::TcpSocket(Socket&& socket, DeleteCb&& deleteCb)
: d_lock{}
, d_socket(std::move(socket))
, d_deleteCb(std::move(deleteCb))
, d_self()
, d_state{IpcProtocol::Channel::eREADY}
, d_stateCb{}
, d_messageCb{}
, d_writeQ{}
{
}

TcpSocket::~TcpSocket()
{
    assert(d_state != IpcProtocol::Channel::eREADY);
    
    if (d_stateCb) {
        d_stateCb(d_state);
    }
    d_deleteCb(); // notify provider
}

void
TcpSocket::closeChannel() noexcept
{
    auto tmp    = d_self;
    auto guard  = std::lock_guard(d_lock);

    if (d_state == IpcProtocol::Channel::eREADY) {
        d_state = IpcProtocol::Channel::eCLOSE;
    }
    d_writeQ.clear();
    d_socket.cancel();
    d_socket.shutdown(Socket::shutdown_both);
    d_socket.close();
    
    d_self.reset();
    return;                                                           // RETURN
}

void
TcpSocket::completeSetup(MessageCb&&    receivedMessage,
                         StateCb&&      channelClose) noexcept
{
    auto guard  = std::lock_guard(d_lock);

    if (d_state == IpcProtocol::Channel::eREADY) {
        d_messageCb = std::move(receivedMessage);
        d_stateCb   = std::move(channelClose);
        assert(d_messageCb);
        assert(d_stateCb);
    }
    return;                                                           // RETURN
}

bool
TcpSocket::sendMessage(MessageChain&&  chain) noexcept
{
    auto guard = std::unique_lock(d_lock);

    if (d_state == IpcProtocol::Channel::eREADY) {
        bool empty = d_writeQ.empty();

        try {
            auto  msgsize = 0u;
            auto *header  = d_writeQ.emplace_back(IpcMessage(HEADERSIZE))
                                    .data();

            for (auto&& message : chain) {
                msgsize += message.length();
                d_writeQ.emplace_back(std::move(message));
            }
            assert(msgsize < (1u << 16));
            assert(HEADERSIZE == 4u);
            header[0] = BYTE1;
            header[1] = BYTE2;
            header[2] = char(msgsize >> 8);
            header[3] = char(msgsize);

            if (empty) {
                d_socket.async_send(net::buffer(header, HEADERSIZE),
                                    [me = d_self](auto ec, auto n) -> void {
                                        me->writeQ(ec, n);
                                    });
            }
            return true;                                              // RETURN
        }
        catch(...) {
            d_state = IpcProtocol::Channel::eERROR;
            auto tmp = d_self;
            d_self.reset();
            guard.release()->unlock();
        }
    }
    return false;                                                     // RETURN
}

void
TcpSocket::readMessage() noexcept
{
    auto guard = std::lock_guard(d_lock);

    if (d_state == IpcProtocol::Channel::eREADY) {
        assert(!d_readBuf.data());
        auto *header  = (d_readBuf = IpcMessage(HEADERSIZE)).data();

        d_socket.async_receive(net::buffer(header, HEADERSIZE),
                               [me = d_self](auto ec, auto n) -> void {
                                   me->readHeader(ec, n);
                               });
    }
    return;                                                           // RETURN
}

IpcProtocol::Channel::State
TcpSocket::state() const noexcept
{
    auto guard = std::lock_guard(d_lock);
    return d_state;                                                   // RETURN
}

                                //-----------------------
                                // class IpcProvider
                                //-----------------------

// PRIVATE MEMBERS
void
IpcProvider::completeConnect(const SetupTicket&  ticket,
                             error_code          error) noexcept
{
    auto guard = std::unique_lock(d_lock);
    auto it    = d_tickets.find(ticket);

    ticket->d_setupStatus = error ? SetupBase::eERROR : SetupBase::eFINISH;

    if (it != d_tickets.end()) {
        auto  cb     = std::get<0>(it->second);
        auto& var    = std::get<1>(it->second);
        auto *sockp  = std::get_if<Ip::tcp::socket>(&var);
        assert(sockp);
        auto  socket = std::move(*sockp);

        d_tickets.erase(it);

        if (error) {
            std::cerr << error.message() << std::endl;
            guard.release()->unlock();
            cb(ChannelPtr(), ticket);
        }
        else {
            newChannel(std::move(socket), ticket, cb);
        }
    }
}

void
IpcProvider::decrementChannelCount() noexcept
{
    auto guard = std::lock_guard(d_lock);
    assert(d_channelCount > 0);

    if (0 == --d_channelCount) {
        d_signalDestructorThread.notify_one();
    }
    return;                                                           // RETURN
}

void
IpcProvider::newChannel(Ip::tcp::socket&&   socket,
                        const SetupTicket&  ticket,
                        const SetupCb&      channelSetupDone) noexcept
{
    assert(!d_shutdown);
    ++d_channelCount;
    auto ptr        = std::make_shared<TcpSocket>(std::move(socket),
                                                  [this] {
                                                      decrementChannelCount();
                                                  });
    ptr->setSelf(ptr);
    auto channelptr = std::shared_ptr<IpcProtocol::Channel>(ptr);
    d_lock.unlock();
    channelSetupDone(ChannelPtr(channelptr), ticket);
    // channel setup has been completed
    d_lock.lock();
    ptr->readMessage();
    return;                                                           // RETURN
}

void
IpcProvider::resolveResults(error_code                        error,
                            Ip::tcp::resolver::results_type   results,
                            const SetupTicket&                ticket,
                            const HandleEndpoint&             handleEp)noexcept
{
    auto guard  = std::lock_guard(d_lock);
    auto it     = d_tickets.find(ticket);

    if (it == d_tickets.end()) {
        return;                                                       // RETURN
    }
    auto status = SetupBase::eINPROGRESS;
    auto ep     = Ip::tcp::endpoint{};

    if (error) {
        std::cerr << error.message() << std::endl;
        status  = SetupBase::eERROR;
    }
    else if (results.empty()) {
        status  = SetupBase::eINVALID;
    }
    else { // only try one (the first listed) destination
        ep      = results.begin()->endpoint();
    }

    if (status != SetupBase::eINPROGRESS) {
        setupError(it, status);
    }
    else {
        handleEp(it, ep); // connect-to OR listen-on
    }
    return;                                                           // RETURN
}

bool
IpcProvider::setupError(TicketMap::iterator  it,
                        SetupBase::Status    status,
                        bool                 useThread) noexcept
{
    if (it != d_tickets.end()) {
        auto  ticket   = it->first;
        auto  setupCb  = std::get<0>(it->second);
        auto& socket   = std::get<1>(it->second);

        ticket->d_setupStatus = status;

        if (auto *acceptor = std::get_if<Ip::tcp::acceptor>(&socket)) {
            acceptor->cancel();
            acceptor->close();
        }
        else {
            auto *connection = std::get_if<Ip::tcp::socket>(&socket);
            assert(connection);
            connection->cancel();
            connection->close();
        }
        d_tickets.erase(it); // see shutdown()

        if (useThread) {
            std::thread(
                [setupCb,ticket] {
                    setupCb(ChannelPtr(), ticket);
                }).detach();
        }
        else {
            d_lock.unlock();
            setupCb(ChannelPtr(), ticket);
            d_lock.lock();
        }

        return true;                                                  // RETURN
    }
    return false;                                                     // RETURN
}

// PUBLIC MEMBERS
IpcProvider::IpcProvider()
: d_lock{}
, d_context{}
, d_resolver{d_context}
, d_signalDestructorThread{}
, d_tickets{}
, d_channelCount{0u}
, d_shutdown{false}
, d_work(net::make_work_guard(d_context))
, d_thread([this] { d_context.run(); })
{
}

IpcProvider::~IpcProvider()
{
    shutdown();
    auto guard = std::unique_lock(d_lock);

    while (d_channelCount > 0) {
        d_signalDestructorThread.wait(guard);
    }
    d_context.stop();
    d_thread.join();
}

bool
IpcProvider::acceptChannels(String_t    *diagnostic,
                            SetupTicket  ticket,
                            SetupCb&&    channelSetupDone) noexcept
{
    auto  host          = std::string{};
    auto  port          = std::string{};
    auto& configuration = ticket->d_configuration;
    auto  guard         = std::lock_guard(d_lock);

    if (d_shutdown) {
        *diagnostic = S("Adapter shutdown.");
        ticket->d_setupStatus = SetupBase::eCANCELED;
    }
    else if (d_tickets.count(ticket) > 0) {
        *diagnostic = S("Ticket already pending.");
        ticket->d_setupStatus = SetupBase::eINVALID;
    }
    else if (anyToHostPort(diagnostic, &host, &port, configuration)) {
        // Synchronously resolve the listen port so application can identify
        // the bound address on return.
        if (!host.empty()
         && host != "localhost"
         && host != "0.0.0.0"
         && host != "::") {
            *diagnostic
                = S("Binding to a non-localhost interface is not supported.");
            ticket->d_setupStatus = SetupBase::eINVALID;
        }
        else if (d_shutdown) {
            *diagnostic = S("IPC provider has been shut down.");
            ticket->d_setupStatus = SetupBase::eFINISH;
        }
        else {
            if (host.empty()) {
                host = "::";
            }
            // resolve listen port synchronously
            auto error = error_code{};
            auto eps   = d_resolver.resolve(host,
                                            port,
                                            error);

            if (error) {
                *diagnostic
                    = S("A port number between 0 and 65535 must be used.");
                ticket->d_setupStatus = SetupBase::eINVALID;
            }
            else {
                auto ep       = eps.begin()->endpoint();
                auto acceptor = Ip::tcp::acceptor(d_context, ep.protocol());
        std::cerr << " ep.address = '" << ep.address()
                  << "' ep.port = " << ep.port() << std::endl;

                acceptor.set_option(Ip::tcp::acceptor::reuse_address(true));
                acceptor.bind(ep, error);

                if (!error) {
                    acceptor.listen(5, error);
                }

                if (!error) {
                    auto localPort = acceptor.local_endpoint().port();
                    auto acceptCb =
                        [this, ticket](error_code      code,
                                       Ip::tcp::socket socket) -> void {
                            auto guard2 = std::lock_guard(d_lock);
                            auto it     = d_tickets.find(ticket);

                            if (it != d_tickets.end()) {
                                if (!code) {
                                    auto& cb = std::get<0>(it->second);
                                    newChannel(std::move(socket), ticket, cb);
                                    return;
                                }
                                // accept failure:
                                setupError(it, SetupBase::eERROR);
                            }
                            // cancel or accept failure:
                            socket.close();
                         };

                    ticket->d_setupStatus = SetupBase::eINPROGRESS;
                    acceptor.async_accept(std::move(acceptCb));
                    d_tickets.try_emplace(ticket,
                                          std::move(channelSetupDone),
                                          std::move(acceptor),
                                          localPort);
                    return true;                                      // RETURN
                }
                *diagnostic
                    = S("Failed to listen on port. It may be already in use.");
                ticket->d_setupStatus = SetupBase::eCANCELED;
            }
        }
    }
    return false;                                                     // RETURN
}

bool
IpcProvider::cancelSetup(const SetupTicket& ticket) noexcept
{
    auto guard = std::lock_guard(d_lock);

    return setupError(d_tickets.find(ticket),
                      SetupBase::eCANCELED,
                      true);  // RETURN
}

bool
IpcProvider::createNewChannel(String_t    *diagnostic,
                              SetupTicket  ticket,
                              SetupCb&&    channelSetupDone) noexcept
{
    auto  host          = std::string{};
    auto  port          = std::string{};
    auto& configuration = ticket->d_configuration;
    auto  guard         = std::lock_guard(d_lock);

    if (d_shutdown) {
        *diagnostic = S("Adapter shutdown.");
        ticket->d_setupStatus = SetupBase::eCANCELED;
    }
    else if (d_tickets.count(ticket) > 0) {
        *diagnostic = S("Ticket already pending.");
        ticket->d_setupStatus = SetupBase::eINVALID;
    }
    else if (anyToHostPort(diagnostic, &host, &port, configuration)) {
        ticket->d_setupStatus = SetupBase::eINPROGRESS;

        auto connection = Ip::tcp::socket(d_context);

        d_tickets.try_emplace(ticket,
                              std::move(channelSetupDone),
                              std::move(connection),
                              0u);

        auto completeFn =
            [this, ticket](error_code error) -> void {
                completeConnect(ticket, error);
            };
        auto connectFn = 
            [completeFn](TicketMap::iterator it, const Ip::tcp::endpoint& ep) {
                auto& anySocket = std::get<1>(it->second);
                auto *socket    = std::get_if<Ip::tcp::socket>(&anySocket);
                assert(socket);
        std::cerr << "handle = " << (int)socket->native_handle()
                  << " ep.address = '" << ep.address()
                  << "' ep.port = " << ep.port() << std::endl;
                auto ec = error_code{};
                socket->open(ep.protocol(), ec);
                assert(!ec);
                socket->async_connect(ep, std::move(completeFn));
            };
        d_resolver.async_resolve(host,
                                 port,
                               /*Ip::resolver_base::numeric_host,
                                 | Ip::resolver_base::numeric_service,
                                 | Ip::resolver_base::canonical_name */
                                 [this,ticket,connectFn](auto  error,
                                                         auto& results) {
                                     resolveResults(error,
                                                    results,
                                                    ticket,
                                                    connectFn);
                                 });
        return true;                                                  // RETURN
    }
    return false;                                                     // RETURN
}

unsigned short
IpcProvider::listenPort(const SetupTicket& ticket) const noexcept
{
    auto guard = std::lock_guard(d_lock);
    auto it    = d_tickets.find(ticket);

    return it != d_tickets.end() ? std::get<2>(it->second) : 0;       // RETURN
}

void
IpcProvider::shutdown() noexcept
{
    auto guard = std::lock_guard(d_lock);

    if (!d_shutdown) {
        d_shutdown = true;

        while (d_tickets.end() != d_tickets.begin()) {
            setupError(d_tickets.begin(), SetupBase::eCANCELED, true);
        }
    }
    return;                                                           // RETURN
}

// vim: ts=4:sw=4:et:ai
