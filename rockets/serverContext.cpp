/* Copyright (c) 2017, EPFL/Blue Brain Project
 *                     Raphael.Dumusc@epfl.ch
 *
 * This file is part of Rockets <https://github.com/BlueBrain/Rockets>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "serverContext.h"

#include "http/connection.h"
#include "ws/connection.h"

namespace rockets
{
ServerContext::ServerContext(const std::string& uri, const std::string& name,
                             const unsigned int threadCount,
                             lws_callback_function* callback,
                             lws_callback_function* wsCallback, void* user)
    : protocols{make_protocol("http", callback, user), null_protocol()}
    , wsProtocolName{name}
{
    if (!wsProtocolName.empty() && wsCallback)
        createWebsocketsProtocol(wsCallback, user);

    fillContextInfo(uri, threadCount);

    context = lws_create_context(&info);
    if (!context)
        throw std::runtime_error("libwebsocket init failed");
}

ServerContext::~ServerContext()
{
    lws_context_destroy(context);
}

const std::string& ServerContext::getInterface() const
{
    return interface;
}

uint16_t ServerContext::getPort() const
{
    return info.port;
}

int ServerContext::getThreadCount() const
{
    return lws_get_count_threads(context);
}

void ServerContext::requestBroadcast()
{
    lws_callback_on_writable_all_protocol(context, &protocols[1]);
}

bool ServerContext::service(const int tsi, const int timeout_ms)
{
    return lws_service_tsi(context, timeout_ms, tsi) >= 0;
}

void ServerContext::service(const int timeout_ms)
{
    lws_service(context, timeout_ms);
}

void ServerContext::service(PollDescriptors& pollDescriptors,
                            const SocketDescriptor fd, const int events)
{
    pollDescriptors.service(context, fd, events);
}

void ServerContext::cancelService()
{
    lws_cancel_service(context);
}

void ServerContext::createWebsocketsProtocol(lws_callback_function* wsCallback,
                                             void* user)
{
    protocols.insert(protocols.begin() + 1,
                     make_protocol(wsProtocolName.c_str(), wsCallback, user));
}

void ServerContext::fillContextInfo(const std::string& uri,
                                    const unsigned int threadCount)
{
    memset(&info, 0, sizeof(info));
    const auto parsedUri = parse(uri);
    interface = parsedUri.host;
    if (!interface.empty())
        info.iface = interface.c_str();
    info.port = parsedUri.port;
    info.protocols = protocols.data();
    // we're not using ssl
    info.ssl_cert_filepath = nullptr;
    info.ssl_private_key_filepath = nullptr;
    info.gid = -1;
    info.uid = -1;
    // service threads
    info.count_threads = threadCount;
    info.max_http_header_pool = threadCount;
}
}