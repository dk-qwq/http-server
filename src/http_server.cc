#include "http_server.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdexcept>

#include <http_message.h>

HttpServer::HttpServer(const std::string& host, const std::uint16_t& port)
    :running_(false),
    host_(host),
    port_(port),
    server_fd_(-1) {

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
    listener_thread_ = std::thread(&HttpServer::Listen, this);
}

void HttpServer::Stop(){
    running_ = false;
    listener_thread_.join();
    close(server_fd_);
    buffers.clear();
}

void HttpServer::Listen() {
    int client_fd;
    char buf[1 << 20];
    while(running_) {
        client_fd = accept(server_fd_, nullptr, nullptr);
        if(client_fd == -1) {
            continue;
        }
        ssize_t n = read(client_fd, buf, sizeof(buf));

        if(n > 0) {
            buffers[client_fd].append(buf, n);
        }
        auto& buffer = buffers[client_fd];
        if(buffer.find("\r\n\r\n") != std::string::npos) {
            // 在 v1阶段，假设仅有 HEAD 和 GET 操作，所以我们不处理 content
            
            HttpRequest request = string_to_request(buffer);
            HttpResponse response = HandlerHttpRequest(request);
            std::string string_response = to_string(response, request.method() == HttpMethod::GET);

            write(client_fd, string_response.c_str(), string_response.size());
            close(client_fd);

            buffer.clear();
            buffers.erase(client_fd);
        }
    }
}

HttpResponse HttpServer::HandlerHttpRequest(const HttpRequest& request) {
    auto it = request_hanlders_.find(request.uri());
    if(it == request_hanlders_.end()) {
        return HttpResponse(HttpStatusCode::NotFound);
    }

    auto callback_it = it->second.find(request.method());
    if(callback_it == (it->second).end()) {
        return HttpResponse(HttpStatusCode::MethodNotAllowed);
    }

    return callback_it->second(request);
}

