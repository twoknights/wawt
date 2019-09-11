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

bool anyToAddress(String_t        *diagnostic,
                  sf::IpAddress   *ipAddress,
                  unsigned short  *port_us,
                  const std::any&  address) {
    auto string = std::string{};

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
        return false;                                                 // RETURN
    }
    // Address parse (regex):
    auto matched = false;
    auto parse   = std::smatch{};
    auto pattern = std::regex{R"(^([a-z\.\-\d]+)(?:\:(\d+))?$)"};

    try {
        matched = std::regex_match(string, parse, pattern);
    }
    catch (...) { }

    if (!matched) {
        *diagnostic = S("The address string is malformed.");
        return false;                                                 // RETURN
    }
    std::string     ipStr;
    std::string     portStr;

    assert(parse.size() == 3);

    if (parse.str(2).empty()) {
        portStr = parse.str(1);
    }
    else {
        ipStr   = parse.str(1);
        portStr = parse.str(2);
    }
    auto pos  = std::size_t{};
    auto port = int{};

    try {
        port = std::stoi(portStr, &pos);
    }
    catch (...) {
        port = -1;
    }

    if (port < 0 || port > UINT16_MAX || pos != portStr.length()) {
        *diagnostic
            = S("A port number between 0 and 65535 must be used.");
        return false;                                                 // RETURN
    }
    // This can result in a DNS lookup (this COULD be SLOW :-( ):
    *ipAddress = ipStr.empty() ? sf::IpAddress::Any : sf::IpAddress(ipStr);
    *port_us   = static_cast<unsigned short>(port);
    return true;                                                      // RETURN
}

} // end unnamed namespace

                                //--------------------
                                // class SfmlTcpSocket
                                //--------------------

// PRIVATE MEMBERS
bool
SfmlTcpSocket::connect(sf::IpAddress ipAddress, unsigned short port)
{
    auto select = sf::SocketSelector();
    select.add(d_socket);

    auto attempts = 0u;
    auto guard    = std::unique_lock(d_lock);

    while (state() == IpcProtocol::Channel::eREADY) {
        assert(++attempts < 900);

        auto status = d_socket.connect(ipAddress, port);

        if (sf::Socket::Done == status) {
            return true;                                              // RETURN
        }

        if (sf::Socket::Disconnected == status) {
            // Connect rejections are often immediate, so sleep before retry
            d_socket.disconnect();
            d_lock.unlock(); // UNLOCK
            std::this_thread::sleep_for(std::chrono::seconds(1));
            d_lock.lock(); // LOCK
        }
        else {
            d_lock.unlock(); // UNLOCK
            select.wait(sf::seconds(1.0f));
            d_lock.lock(); // LOCK
        }
    }
    d_socket.disconnect();
    return false;                                                     // RETURN
}

// PUBLIC MEMBERS
SfmlTcpSocket::SfmlTcpSocket()
{
    d_socket.setBlocking(false);
    d_state = IpcProtocol::Channel::eREADY;
}

SfmlTcpSocket::~SfmlTcpSocket()
{
    if (d_stateCb) {
        assert(state() != IpcProtocol::Channel::eREADY);
        d_stateCb(state());
    }
}

void
SfmlTcpSocket::readMsgLoop()
{
    assert(sf::IpAddress::None != d_socket.getRemoteAddress());

    sf::SocketSelector select;
    select.add(d_socket);

    while (state() == IpcProtocol::Channel::eREADY) {
        uint16_t size = readSizeHdr(select);

        if (size > 0) {
            readMsg(select, size);
        }
    }
    d_signalWriteThread.notify_one();
    return;                                                           // RETURN
}

std::size_t
SfmlTcpSocket::readSizeHdr(sf::SocketSelector& select)
{
    auto header   = std::array<unsigned char,4>{};
    auto bufsize  = std::size_t{};
    auto attempts = 0u;
    auto expected = IpcProtocol::Channel::eREADY;

    while (state() == IpcProtocol::Channel::eREADY) {
        assert(++attempts < 900); // inter-message interval should be < 15min

        if (select.wait(sf::seconds(1.0f))) { // poll for shutdown.
            auto readsize = std::size_t{};

            auto status   = d_socket.receive(header.data()+bufsize,
                                             header.size()-bufsize,
                                             readsize);
            bufsize += readsize;

            if (sf::Socket::Done == status || sf::Socket::Partial == status) {
                if (bufsize == header.size()) {
                    if (header[0] == BYTE1 && header[1] == BYTE2) {
                        auto byte3      = uint16_t(header[2]);
                        auto byte4      = uint16_t(header[3]);
                        auto size       = (byte3 << 8)|byte4;
                        return size;                                  // RETURN
                    }
                    d_state.compare_exchange_strong(
                            expected,
                            IpcProtocol::Channel::ePROTO);
                }
            }
            else if (sf::Socket::Disconnected == status) {
                d_state.compare_exchange_strong(expected,
                                                IpcProtocol::Channel::eDROP);
            }
            else if (sf::Socket::Error == status) {
                d_state.compare_exchange_strong(expected,
                                                IpcProtocol::Channel::eERROR);
            }
        }
    }
    return 0;                                                         // RETURN
}

void
SfmlTcpSocket::readMsg(sf::SocketSelector&     select,
                        uint16_t               msgSize)
{
    auto buffer   = std::make_unique<char[]>(msgSize);
    auto bufsize  = std::size_t{};
    auto expected = IpcProtocol::Channel::eREADY;
    auto attempts = 0u;

    while (state() == IpcProtocol::Channel::eREADY) {
        assert(++attempts < 900); // intra-message interval should be < 15min

        if (select.wait(sf::seconds(1.0f))) { // poll for shutdown.
            std::size_t readsize = 0;

            auto status = d_socket.receive(buffer.get()+bufsize,
                                           msgSize-bufsize,
                                           readsize);

            bufsize += readsize;

            if (sf::Socket::Done == status || sf::Socket::Partial == status) {
                if (bufsize == msgSize) {
                    d_messageCb(IpcMessage(std::move(buffer), msgSize, 0));
                    return;                                           // RETURN
                }
            }
            else if (sf::Socket::Disconnected == status) {
                d_state.compare_exchange_strong(
                        expected,
                        IpcProtocol::Channel::eDROP);
            }
            else if (sf::Socket::Error == status) {
                d_state.compare_exchange_strong(
                        expected,
                        IpcProtocol::Channel::eERROR);
            }
        }
    }
    return;                                                           // RETURN
}

bool
SfmlTcpSocket::start(String_t             *diagnostic,
                     const SetupTicket&    ticket,
                     const Task&           readerTask,
                     const Task&           writerTask)
{
    auto writer = std::thread();

    try {
        writer = std::thread(std::move(writerTask));
    }
    catch (...) {
    }

    if (!writer.joinable()) {
        if (ticket) {
            ticket->d_setupStatus = SfmlIpV4Provider::SetupBase::eERROR;
        }

        if (diagnostic) {
            *diagnostic = S("Failed to start message sender.");
        }
        return false;                                                 // RETURN
    }
    writer.detach();

    auto reader = std::thread();

    try {
        reader = std::thread(std::move(readerTask));
    }
    catch (...) {
    }

    if (!reader.joinable()) {
        if (ticket) {
            ticket->d_setupStatus = SfmlIpV4Provider::SetupBase::eERROR;
        }
        closeChannel();

        if (diagnostic) {
            *diagnostic = S("Failed to start message receiver.");
        }
        return false;                                                 // RETURN
    }
    reader.detach();

    if (diagnostic) {
        *diagnostic = S("");
    }
    return true;                                                      // RETURN
}

IpcProtocol::Channel::State
SfmlTcpSocket::writeMsg(const char *message, uint16_t msgSize)
{
    auto sent = std::size_t{};
    auto code = IpcProtocol::Channel::State::eREADY;

    while (state() == code) {
        auto size = std::size_t{};

        auto status = d_socket.send(message+sent, msgSize-sent, size);
        sent += size;

        if (sf::Socket::Done == status || sf::Socket::Partial == status) {
            if (sent == msgSize) {
                break;                                                 // BREAK
            }
        }
        else if (sf::Socket::Disconnected   == status) {
            code = IpcProtocol::Channel::State::eDROP;
            break;                                                     // BREAK
        }
        else if (sf::Socket::Error          == status) {
            code = IpcProtocol::Channel::State::eERROR;
            break;                                                     // BREAK
        }

        if (size == 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return code;                                                      // RETURN
}

void
SfmlTcpSocket::writeMsgLoop()
{
    // Note: connection may not be established at this point.
    bool socketConnected = false;

    auto guard = std::unique_lock(d_lock);

    while (state() == IpcProtocol::Channel::eREADY) {
        if (d_writeQ.empty()) {
            d_signalWriteThread.wait(guard);
            continue;                                               // CONTINUE
        }

        if (!socketConnected
         && sf::IpAddress::None == d_socket.getRemoteAddress()) {
            d_signalWriteThread.wait(guard); // connection not established
            continue;                                               // CONTINUE
        }
        socketConnected = true;

        auto chain = std::move(d_writeQ.back());
        d_writeQ.pop_back();

        d_lock.unlock(); /*UNLOCK*/

        auto msgsize = uint16_t{};

        for (auto& fragment : chain) {
            msgsize += fragment.length();
        }

        char header[4];
        header[0] = BYTE1;
        header[1] = BYTE2;
        header[2] = char(msgsize >> 8);
        header[3] = char(msgsize);

        auto expected = IpcProtocol::Channel::eREADY;
        auto result   = writeMsg(header, sizeof(header));

        if (d_state.compare_exchange_strong(expected, result)) {
            for (auto& fragment : chain) {
                result = writeMsg(fragment.cbegin(), fragment.length());

                if (!d_state.compare_exchange_strong(expected, result)) {
                    break;                                             // BREAK
                }
            }
        }
        d_lock.lock(); /*LOCK*/
    }
    return;                                                           // RETURN
}

// PUBLIC MEMBERS
void
SfmlTcpSocket::closeChannel() noexcept
{
    auto guard  = std::lock_guard(d_lock);
    d_state     = IpcProtocol::Channel::eCLOSE;
    d_socket.disconnect();
    d_writeQ.clear();
    d_signalWriteThread.notify_one();
}

void
SfmlTcpSocket::completeSetup(MessageCb&&    receivedMessage,
                             StateCb&&      channelClose) noexcept
{
    auto guard  = std::lock_guard(d_lock);

    d_messageCb = std::move(receivedMessage);
    d_stateCb   = std::move(channelClose);
    d_state     = IpcProtocol::Channel::eREADY;
    assert(d_messageCb);
    assert(d_stateCb);
}

bool
SfmlTcpSocket::sendMessage(MessageChain&&  chain) noexcept
{
    auto guard = std::lock_guard(d_lock);

    if (d_state == IpcProtocol::Channel::eREADY) {
        bool empty = d_writeQ.empty();

        try {
            d_writeQ.emplace_front(std::move(chain));

            if (empty) {
                d_signalWriteThread.notify_one();
            }
            return true;                                              // RETURN
        }
        catch(...) {
            d_state = IpcProtocol::Channel::eERROR;
        }
    }
    return false;                                                     // RETURN
}

IpcProtocol::Channel::State
SfmlTcpSocket::state() const noexcept
{
    return d_state.load();                                            // RETURN
}

                                //-----------------------
                                // class SfmlIpV4Provider
                                //-----------------------

// PRIVATE MEMBERS
void
SfmlIpV4Provider::addTicket(const SetupTicket&    ticket,
                            unsigned short        port,
                            const CancelCb&       cancel) noexcept
{
    auto guard = std::lock_guard(d_lock);
    d_tickets.try_emplace(ticket, port, cancel);
    d_signalSetupThread.notify_one();
}
                        
bool
SfmlIpV4Provider::start(String_t             *diagnostic,
                        SetupTicket           ticket,
                        const SetupCb&        channelSetupDone,
                        const sf::IpAddress&  ipAddress,
                        unsigned short        port) noexcept
{
    // d_lock held on entry
    auto socket     = std::make_shared<SfmlTcpSocket>();
    auto writerTask = [socket] { socket->writeMsgLoop(); };
    auto readerTask =
        [this, socket, channelSetupDone, ticket, ipAddress, port] () mutable {
            addTicket(ticket, 0, [socket] { socket->closeChannel(); });

            auto status   = socket->connect(ipAddress, port)
                    ? SetupBase::eFINISH
                    : SetupBase::eERROR;
            auto expected = SetupBase::eINPROGRESS;

            ticket->d_setupStatus.compare_exchange_strong(expected, status);

            if (ticket->d_setupStatus != SetupBase::eFINISH) {
                socket.reset();
            }
            channelSetupDone(socket, ticket);

            cancelSetup(ticket); // setup is finished so this just cleans up
            ticket.reset();

            if (socket && socket->state() == IpcProtocol::Channel::eREADY) {
                socket->readMsgLoop();
            }
            assert(!socket || socket->state() != IpcProtocol::Channel::eREADY);
            socket.reset();
            decrementCaptureCount();
        };
    return socket->start(diagnostic, ticket, readerTask, writerTask); // RETURN
}

bool
SfmlIpV4Provider::start(String_t                         *diagnostic,
                        const SetupTicket&                ticket,
                        const SetupCb&                    channelSetupDone,
                        std::shared_ptr<sf::TcpListener>  listener) noexcept
{
    // A timed "wait" operation is used to poll the shutdown flag while
    // watching for an indication that the listen socket has something to
    // accept.  Note that the "setup" is active until shutdown, so many
    // connections could be established with just one "setup".

    auto listenTask =
        [this, listener, channelSetupDone, ticket] () mutable {
            listener->setBlocking(false);
            sf::SocketSelector select;
            select.add(*listener);
            auto attempts = 0u;
            auto socket   = std::make_shared<SfmlTcpSocket>();
            auto code     = SetupBase::eCANCELED;

            auto shutdown = std::make_shared<std::atomic_bool>();
            *shutdown     = false;

            addTicket(ticket,
                      listener->getLocalPort(),
                      [shutdown] { shutdown->store(true); });

            while (!*shutdown) {
                assert(++attempts < 900);

                if (select.wait(sf::seconds(1.0f))) {
                    auto status = listener->accept(socket->socket());

                    if (sf::Socket::Done == status) {
                        auto reader = [socket] { socket->readMsgLoop(); };
                        auto writer = [socket] { socket->writeMsgLoop(); };

                        if (socket->start(nullptr, nullptr, reader, writer)) {
                            channelSetupDone(socket, ticket);
                        }
                        socket = std::make_shared<SfmlTcpSocket>();
                    }
                    else if (sf::Socket::Error == status) {
                        code = SetupBase::eERROR;
                        *shutdown = true;
                    }
                }
            }
            socket.reset();
            ticket->d_setupStatus = code;
            channelSetupDone(socket, ticket);
            listener->close();
            cancelSetup(ticket); // just to cleanup
            decrementCaptureCount();
        };
    auto listen = std::thread();

    try {
        listen = std::thread(std::move(listenTask));
    }
    catch (...) {
    }

    if (!listen.joinable()) {
        ticket->d_setupStatus = SetupBase::eERROR;
        *diagnostic = S("Failed to start listener.");
        return false;                                                 // RETURN
    }
    listen.detach();
    *diagnostic = S("");
    return true;                                                      // RETURN
}

// PUBLIC MEMBERS
SfmlIpV4Provider::~SfmlIpV4Provider()
{
    shutdown();
    // Can't proceed until all threads with lambdas that captured
    // the 'this' pointer have exited:
    auto guard = std::unique_lock(d_lock);

    while (d_thisCaptures > 0) {
        d_signalShutdownThread.wait(guard);
    }
}

bool
SfmlIpV4Provider::acceptChannels(String_t    *diagnostic,
                                 SetupTicket  ticket,
                                 SetupCb&&    channelSetupDone) noexcept
{
    auto  guard         = std::unique_lock(d_lock);
    auto  ipAddress     = sf::IpAddress();
    auto  port          = uint16_t{};
    auto& configuration = ticket->d_configuration;

    if (d_tickets.count(ticket) > 0) {
        *diagnostic = S("Ticket already pending.");
        ticket->d_setupStatus = SetupBase::eINVALID;
    }
    else if (anyToAddress(diagnostic, &ipAddress, &port, configuration)) {
        auto socket   = std::make_shared<sf::TcpListener>();

        if (sf::IpAddress::Any               != ipAddress
         && sf::IpAddress::getLocalAddress() != ipAddress
         && sf::IpAddress(127u,0u,0u,1u)     != ipAddress) {
            *diagnostic
                = S("Can only bind listen to this computer.");
            ticket->d_setupStatus = SetupBase::eINVALID;
        }
        else if (d_shutdown) {
            *diagnostic = S("IPC provider has been shut down.");
            ticket->d_setupStatus = SetupBase::eFINISH;
        }
        else if (sf::Socket::Error == socket->listen(port, ipAddress)) {
            *diagnostic =
                    S("Failed to listen on port. It may be already in use.");
            ticket->d_setupStatus = SetupBase::eCANCELED;
        }
        else {
            ticket->d_setupStatus = SetupBase::eINPROGRESS;

            if (start(diagnostic,
                      ticket,
                      channelSetupDone,
                      socket)) {
                d_thisCaptures += 1; // "Listener" captures a copy 'this'
                d_signalSetupThread.wait(guard);
                return true;                                          // RETURN
            }
        }
    }
    return false;                                                     // RETURN
}

bool
SfmlIpV4Provider::cancelSetup(const SetupTicket& ticket) noexcept
{
    auto guard = std::unique_lock(d_lock);
    auto it    = d_tickets.find(ticket);

    if (it != d_tickets.end()) {
        auto cancelCb = it->second.second;
        d_tickets.erase(it);
        auto expected = SetupBase::eINPROGRESS;

        if (ticket->d_setupStatus.compare_exchange_strong(
                    expected,
                    SetupBase::eCANCELED)) {
            guard.release()->unlock();
            cancelCb();
            return true;                                              // RETURN
        }
    }
    return false;                                                     // RETURN
}

bool
SfmlIpV4Provider::createNewChannel(String_t    *diagnostic,
                                   SetupTicket  ticket,
                                   SetupCb&&    channelSetupDone) noexcept
{
    auto  guard         = std::unique_lock(d_lock);
    auto  ipAddress     = sf::IpAddress();
    auto  port          = uint16_t{};
    auto& configuration = ticket->d_configuration;

    if (d_tickets.count(ticket) > 0) {
        *diagnostic = S("Ticket already pending.");
        ticket->d_setupStatus = SetupBase::eINVALID;
    }
    else if (anyToAddress(diagnostic, &ipAddress, &port, configuration)) {
        if (sf::IpAddress::Any               == ipAddress
         || sf::IpAddress::Broadcast         == ipAddress) {
            *diagnostic
                = S("Cannot connect to broadcast or '0.0.0.0' address.");
            ticket->d_setupStatus = SetupBase::eINVALID;
        }
        else if (sf::IpAddress::None == ipAddress) {
            *diagnostic = S("The destination host is malformed or unknown.");
            ticket->d_setupStatus = SetupBase::eINVALID;
        }
        else if (d_shutdown) {
            *diagnostic = S("IPC provider has been shut down.");
            ticket->d_setupStatus = SetupBase::eCANCELED;
        }
        else {
            ticket->d_setupStatus = SetupBase::eINPROGRESS;

            if (start(diagnostic,
                      ticket,
                      channelSetupDone,
                      ipAddress,
                      port)) {
                d_thisCaptures += 1; // "Reader" captures a copy 'this'
                d_signalSetupThread.wait(guard);
                return true;                                          // RETURN
            }
        }
    }
    return false;                                                     // RETURN
}

void
SfmlIpV4Provider::decrementCaptureCount() noexcept
{
    auto guard = std::lock_guard(d_lock);
    d_thisCaptures -= 1;

    if (d_shutdown) {
        d_signalShutdownThread.notify_one();
    }
    return;                                                           // RETURN
}

unsigned short
SfmlIpV4Provider::listenPort(const SetupTicket& ticket) const noexcept
{
    auto guard = std::lock_guard(d_lock);
    auto it    = d_tickets.find(ticket);

    return it != d_tickets.end() ? it->second.first : 0;
}

void
SfmlIpV4Provider::shutdown() noexcept
{
    auto guard = std::unique_lock(d_lock);

    if (!d_shutdown) {
        d_shutdown = true;

        while (d_tickets.end() != d_tickets.begin()) {
            auto cancelCb = d_tickets.begin()->second.second;
            d_tickets.erase(d_tickets.begin());
            d_lock.unlock();
            cancelCb();
            d_lock.lock();
        }
    }
    return;                                                           // RETURN
}

// vim: ts=4:sw=4:et:ai
