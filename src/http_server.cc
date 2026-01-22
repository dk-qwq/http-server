#include "http_server.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

#include <iostream>
#include <stdexcept>
#include <chrono>
#include <memory>

#include "http_message.h"
#include "connection.h"

HttpServer::HttpServer(const std::string& host, const std::uint16_t& port)
    :running_(false),
    host_(host),
    port_(port),
    server_fd_(-1),
    rng_(std::chrono::steady_clock::now().time_since_epoch().count()),
    sleep_times_(10, 100) {

}

void HttpServer::Start() {
    if((server_fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int flag = fcntl(server_fd_, F_GETFL);
    fcntl(server_fd_, F_SETFL, flag | O_NONBLOCK);


    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(host_.c_str());
    address.sin_port = htons(port_);

    if(bind(server_fd_, (sockaddr *)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind");
    }

    if(listen(server_fd_, KBackLogSiz) < 0) {
        throw std::runtime_error("Failed to listen");
    }

    running_ = true;
    SetUpEpoll();
    listener_thread_ = std::thread(&HttpServer::Listen, this);

    for(int i = 0; i < kThreadPollSize; i++) {
        epoll_thread_[i] = std::thread(&HttpServer::ProcessEvent, this, i);
    }
}

void HttpServer::Stop(){
    running_ = false;
    listener_thread_.join();

    for(int i = 0; i < kThreadPollSize; i++) {
        epoll_thread_[i].join();
        close(epoll_fd_[i]);
    }
    close(server_fd_);
}

void HttpServer::SetUpEpoll() {
    for(int i = 0; i < kThreadPollSize; i++) {
        if((epoll_fd_[i] = epoll_create1(0)) < 0) {
            throw std::runtime_error("Fail to create epoll");
        }
    }
}

void HttpServer::Listen() {
    int client_fd, next_worker_index = 0;

    std::unique_ptr<Connection> connection;    

    bool active = true;

    while(running_) {
        if(!active) {
            std::this_thread::sleep_for(
                std::chrono::microseconds(sleep_times_(rng_))
            );
        }

        client_fd = accept(server_fd_, nullptr, nullptr);
        if(client_fd == -1) {
            active = false;
            continue;
        }

        std::cout << "new client connection" << std::endl;
        
        active = true;

        connection = std::make_unique<Connection>(
            client_fd,
            [this](const HttpRequest& request) -> HttpResponse {
                return this->HandlerHttpRequest(request);
            }
        );

        Connection* connection_ptr = connection.get();
        ControlEpollEvent(epoll_fd_[next_worker_index], EPOLL_CTL_ADD, client_fd, EPOLLIN, connection_ptr);
        connection.release();

        next_worker_index++;
        if(next_worker_index == kThreadPollSize) next_worker_index = 0;
        
    }
}

void HttpServer::ProcessEvent(int worker_id) {
    int epoll_fd = epoll_fd_[worker_id];
    bool active = true;
    epoll_event* events = events_[worker_id];


    while(running_) {
        if(!active) {
            std::this_thread::sleep_for(
                std::chrono::microseconds(sleep_times_(rng_))
            );
        }
        int nfds = epoll_wait(epoll_fd, events, kMaxEvents, 0);

        if(nfds <= 0) {
            active = false;
            continue;
        }

        for(int i = 0; i < nfds; i++) {
            epoll_event *current_events = &events[i];

            Connection* ptr = reinterpret_cast<Connection *>(current_events->data.ptr);
            std::unique_ptr<Connection> connection(ptr);

            if(current_events->events == EPOLLHUP ||
                current_events->events == EPOLLERR) {
                ControlEpollEvent(epoll_fd, EPOLL_CTL_DEL, connection->fd());
                close(connection->fd());
            } else if(current_events->events == EPOLLIN || current_events->events == EPOLLOUT) {
                HandlerEpollEvent(epoll_fd, current_events->events, std::move(connection));
            } else {
                ControlEpollEvent(epoll_fd, EPOLL_CTL_DEL, connection->fd());
                close(connection->fd());
            }
        }

        active = true;
    }
}


void HttpServer::HandlerEpollEvent(int epoll_fd, std::uint32_t event, std::unique_ptr<Connection> connection) {
    // only EPOLLIN / EPOLLOUT
    int fd = connection->fd();

    try{
        if(event == EPOLLIN) {
            if(connection->Reading() != true) {
                throw std::logic_error("ConnectionState is not in expectation");
            }
            connection->handler_read();
        }
        if(event == EPOLLOUT) {
            if(connection->Writing() != true) {
                throw std::logic_error("ConnectionState is not in expectation");
            }
            connection->handler_write();
        }
    } catch(const std::logic_error& e) {
        std::cerr << "Logic error in HandlerEpollEvent: " << e.what() << std::endl;
        connection->handler_close();
    }

    if(connection->Closing()) {
        ControlEpollEvent(epoll_fd, EPOLL_CTL_DEL, fd);
    }
    else if(connection->Reading()) {
        ControlEpollEvent(epoll_fd, EPOLL_CTL_MOD, fd, EPOLLIN, connection.release());
    }
    else if(connection->Writing()) {
        ControlEpollEvent(epoll_fd, EPOLL_CTL_MOD, fd, EPOLLOUT, connection.release());
    }
}

void HttpServer::ControlEpollEvent(int epoll_fd, int op, int fd, std::uint32_t event, void* data) {
    if(op == EPOLL_CTL_DEL) {
        if(epoll_ctl(epoll_fd, op, fd, nullptr) < 0) {
            throw std::runtime_error("Failed to remove fd");
        }
    } else {
        epoll_event ev;
        ev.events = event;
        ev.data.ptr = data;
        if(epoll_ctl(epoll_fd, op, fd, &ev) < 0) {
            throw std::runtime_error("Failed to deal epoll ctl");
        }
    }
}


HttpResponse HttpServer::HandlerHttpRequest(const HttpRequest& request) {
    auto it = request_handlers_.find(request.uri());
    if(it == request_handlers_.end()) {
        return HttpResponse(HttpStatusCode::NotFound);
    }

    auto callback_it = it->second.find(request.method());
    if(callback_it == (it->second).end()) {
        return HttpResponse(HttpStatusCode::MethodNotAllowed);
    }

    return callback_it->second(request);
}