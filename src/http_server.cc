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

#include <http_message.h>

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
    EventData *client_data;

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

        std::cerr << "new client connection" << std::endl;
        
        active = true;

        client_data = new EventData();
        client_data->fd = client_fd;
        ControlEpollEvent(epoll_fd_[next_worker_index], EPOLL_CTL_ADD, client_fd, EPOLLIN, client_data);

        next_worker_index++;
        if(next_worker_index == kThreadPollSize) next_worker_index = 0;
        
    }
}

void HttpServer::ProcessEvent(int worker_id) {
    int epoll_fd = epoll_fd_[worker_id];
    bool active = true;
    epoll_event* events = events_[worker_id];

    EventData *data;

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
            data = reinterpret_cast<EventData *>(current_events->data.ptr);
            if(current_events->events == EPOLLHUP ||
                current_events->events == EPOLLERR) {
                ControlEpollEvent(epoll_fd, EPOLL_CTL_DEL, data->fd);
                close(data->fd);
                delete data;
            } else if(current_events->events == EPOLLIN || current_events->events == EPOLLOUT) {
                HandlerEpollEvent(epoll_fd, current_events->events, data);
            } else {
                ControlEpollEvent(epoll_fd, EPOLL_CTL_DEL, data->fd);
                close(data->fd);
                delete data;
            }
        }

        active = true;
    }
}

void HttpServer::HandlerEpollEvent(int epoll_fd, std::uint32_t event, EventData* data) {
    // only EPOLLIN / EPOLLOUT
    int fd = data->fd;
    EventData *request, *response;

    if(event == EPOLLIN) {
        request = data;
        size_t *cursor = &(request->cursor);
        ssize_t byte_count = recv(fd, request->buffer + *cursor,
                                    kMaxBufferSize - *cursor, 0);
        if(byte_count > 0) {
            *cursor += byte_count;

            if(request->length == SIZE_MAX) {
                request->length = GetContentLength(request->buffer, *cursor);
            } else {
                request->length -= byte_count;
            }

            if(request->length == 0) {
                response = new EventData();
                response->fd = fd;
                HandleHttpData(*request, response);
                ControlEpollEvent(epoll_fd, EPOLL_CTL_MOD, fd, EPOLLOUT, response);
                delete request;
            } else {
                ControlEpollEvent(epoll_fd, EPOLL_CTL_MOD, fd, EPOLLIN, request);
            }
        } else if(byte_count == 0) {
            ControlEpollEvent(epoll_fd, EPOLL_CTL_DEL, fd);
            close(fd);
            delete request;
        } else {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                ControlEpollEvent(epoll_fd, EPOLL_CTL_MOD, fd, EPOLLIN, request);
            } else {
                ControlEpollEvent(epoll_fd, EPOLL_CTL_DEL, fd);
                close(fd);
                delete request;
            }
        }
    } else {
        response = data;
        size_t *cursor = &(response->cursor);
        ssize_t byte_count = send(fd, response->buffer + *cursor, response->length, 0);
        if(byte_count > 0) {
            *cursor += byte_count;
            response->length -= byte_count;
            if(response->length > 0) {
                ControlEpollEvent(epoll_fd, EPOLL_CTL_MOD, fd, EPOLLOUT, response);
            } else {
                request = new EventData();
                request->fd = fd;
                ControlEpollEvent(epoll_fd, EPOLL_CTL_MOD, fd, EPOLLIN, request);
                delete response;
            }
        } else if(byte_count == 0) {
            ControlEpollEvent(epoll_fd, EPOLL_CTL_DEL, fd);
            delete response;
        } else {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                ControlEpollEvent(epoll_fd, EPOLL_CTL_MOD, fd, EPOLLOUT, response);
            } else {
                ControlEpollEvent(epoll_fd, EPOLL_CTL_DEL, fd);
                close(fd);
                delete response;
            }
        }
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

size_t GetContentLength(char* buffer, size_t len) {
    std::string header(buffer, len);
    size_t lpos = 0, rpos = header.find("\r\n\r\n");
    if(rpos == std::string::npos) {
        return SIZE_MAX;
    }
    header = header.substr(lpos, rpos - lpos);

    HttpRequest request = string_to_request(header);

    std::string result = request.header("Content-Length");

    if(result == "") return 0;

    return atol(result.c_str());
}

void HttpServer::HandleHttpData(const EventData& raw_request, EventData* raw_response) {
    std::string request_string(raw_request.buffer, raw_request.cursor);
    std::cerr << "request_string: " << request_string << std::endl;
    std::string response_string;
    HttpRequest request;
    HttpResponse response;
    try {
        request = string_to_request(request_string);
        response = HandlerHttpRequest(request);
    } catch(const std::invalid_argument& e) {
        response.SetStatuscode(HttpStatusCode::BadRequest);
        response.SetContent(e.what());
    } catch(const std::exception& e) {
        response.SetStatuscode(HttpStatusCode::InternalServerError);
        response.SetContent(e.what());
    }
    response_string = to_string(response,
                                 request.method() == HttpMethod::GET);

    std::cerr << "response_string: " << to_string(response, true) << std::endl;
    
    std::size_t len = std::min(response_string.size(), kMaxBufferSize - 1);
    std::memcpy(raw_response->buffer, response_string.data(), 
                 len * sizeof(char));
    raw_response->buffer[len] = '\0';
    raw_response->length = len;
}