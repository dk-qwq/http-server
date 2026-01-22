#pragma once

#include <string>
#include <functional>

#include "http_parser.h"
#include "config.h"

using RequestHandler = std::function<HttpResponse(const HttpRequest&)>;

enum class ConnectionState {
    Reading,
    Writing,
    Closing,
};

class Connection{
public:
    explicit Connection(int fd, RequestHandler request_handler, ConnectionState state = ConnectionState::Reading);
    ~Connection() = default;

    int fd() const;

    void handler_read();
    void handler_write();
    void handler_close();

    bool Reading() const;
    bool Writing() const;
    bool Closing() const;

    void reset();

    // ConnectionState state();

private:
    int fd_;
    
    char read_buffer_[kMaxBufferSize];
    std::string write_buffer_;
    std::size_t write_offset_;

    HttpParser parser_;

    RequestHandler request_handler_;

    ConnectionState state_;
};