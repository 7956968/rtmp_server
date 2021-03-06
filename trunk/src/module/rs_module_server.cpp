/*
MIT License

Copyright (c) 2016 ME_Kun_Han

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <memory>
#include "rs_kernel_io.h"
#include "rs_module_server.h"
#include "rs_module_config.h"
#include "rs_common_utility.h"
#include "rs_module_config.h"
#include "rs_module_log.h"

RsRtmpServer::RsRtmpServer() {
    _listen_sock = std::unique_ptr<RsTCPListener>(new RsTCPListener());
}

RsRtmpServer::~RsRtmpServer() {
    dispose();
    _connections.clear();

}

void RsRtmpServer::on_new_connection(IRsReaderWriter *io, void *param) {
    rs_info(io, "get one connection for rtmp");

    auto *pt_this = (RsRtmpServer *) param;
    assert(pt_this != nullptr);

    int ret = ERROR_SUCCESS;

    auto conn = std::shared_ptr<RsServerRtmpConn>(new RsServerRtmpConn());
    if ((ret = conn->initialize(io)) != ERROR_SUCCESS) {
        rs_error(io, "initialize the rtmp connection failed. ret=%d", ret);
        return;
    }

    pt_this->_connections.push_back(conn);
}

int RsRtmpServer::initialize(rs_config::RsConfigBaseServer *config) {
    int ret = ERROR_SUCCESS;

    rs_info(_listen_sock.get(), "ready to initialize a new rtmp server, name=%s, port=%d",
            config->get_server_name().c_str(), config->get_port());

    _listen_sock->initialize("0.0.0.0", config->get_port(), on_new_connection, this);

    return ret;
}

int RsRtmpServer::dispose() {
    int ret = ERROR_SUCCESS;

    return ret;
}

int RsRtmpServer::update_status() {
    int ret = ERROR_SUCCESS;

    for (auto i = _connections.begin(); i != _connections.end();) {
        auto conn = *i;
        conn->update_status();

        if (conn->is_stopped()) {
            i = _connections.erase(i);
            continue;
        }

        i++;
    }

    return ret;
}

int RsServerManager::initialize(const rs_config::ConfigServerContainer &servers) {
    int ret = ERROR_SUCCESS;

    for (auto &i : servers) {
        auto config = i.second.get();

        std::shared_ptr<RsBaseServer> baseServer = nullptr;
        switch (config->get_type()) {
            case rs_config::RS_SERVER_TYPE_RTMP:
                baseServer = std::make_shared<RsRtmpServer>();
                break;
            default:
                rs_error(nullptr, "Sorry, we only support rtmp server now");
                return ERROR_CONFIGURE_TYPE_OF_SERVER_NOT_SUPPORT;
        }

        assert(baseServer != nullptr);

        if ((ret = baseServer->initialize(config)) != ERROR_SUCCESS) {
            rs_error(nullptr, "initialize server failed. name=%s, type=%d, ret=%d",
                     config->get_server_name().c_str(), config->get_type(), ret);
            return ret;
        }

        server_container[config->get_server_name()] = baseServer;
    }

    if ((ret = uv_timer_init(uv_default_loop(), &_timer)) != 0) {
        rs_error(nullptr, "initialize timer for servers manager failed.");
        return ret;
    }

    _timer.data = this;

    // TODO:FIXME: use configure items to set timeout and repeat
    if ((ret = uv_timer_start(&_timer, do_update_status, 100, 100)) != 0) {
        rs_error(nullptr, "start timer failed.");
        return ret;
    }

    return ret;
}

int RsServerManager::run() {
    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void RsServerManager::stop() {
    for (auto &i : server_container) {
        i.second->dispose();
    }

    server_container.clear();
}

void RsServerManager::do_update_status(uv_timer_t *timer) {
    auto manager = (RsServerManager *) timer->data;

    assert(manager != nullptr);

    int ret = ERROR_SUCCESS;
    for (auto &i : manager->server_container) {
        if ((ret = i.second->update_status()) != ERROR_SUCCESS) {
            rs_error(nullptr, "update status of server=%s failed. ret=%d", i.first.c_str(), ret);
        }
    }

}
