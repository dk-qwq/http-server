#include "connection.h"

#include <sys/socket.h>

#include <unistd.h>

#include <iostream>

#include "http_message.h"

Connection::Connection(int fd, RequestHandler request_handler, ConnectionState state)
    :fd_(fd),
    read_buffer_(),
    write_buffer_(),
    write_offset_(),
    parser_(),
    request_handler_(request_handler),
    state_(state)
     {}

int Connection::fd() const {
    return fd_;
}

void Connection::handler_read() {
    state_ = ConnectionState::Reading;

    ssize_t byte_count =  recv(fd_, read_buffer_, kMaxBufferSize, 0);
    if(byte_count > 0) {
        parser_.feed(read_buffer_, byte_count);
    } else {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            return ;
        } else {
            state_ = ConnectionState::Closing;
            handler_close();
        }
    }
    if(parser_.has_error()) {
        state_ = ConnectionState::Closing;
        handler_close();
    }
    if(parser_.is_done()) {
        std::cout << "Request: " << std::endl
                << to_string(parser_.request()) << std::endl;
        HttpResponse response = request_handler_(parser_.request());
        write_buffer_ = to_string(response);
        std::cout << "Response: " << std::endl
                << write_buffer_ << std::endl;
        state_ = ConnectionState::Writing;
    }
}

void Connection::handler_write() {
    state_ = ConnectionState::Writing;

    ssize_t byte_count =  send(fd_,
                        write_buffer_.data() + write_offset_,
                        write_buffer_.size() - write_offset_, 0);
    if(byte_count > 0) {
        write_offset_ += byte_count;
    } else {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            return ;
        } else {
            state_ = ConnectionState::Closing;
            handler_close();
        }
    }

    if(write_offset_ == write_buffer_.size()) {
        state_ = ConnectionState::Reading;
        reset();
    }
}

void Connection::handler_close() {
    state_ = ConnectionState::Closing;
}

bool Connection::Reading() const {
    return state_ == ConnectionState::Reading;
}

bool Connection::Writing() const {
    return state_ == ConnectionState::Writing;
}

bool Connection::Closing() const {
    return state_ == ConnectionState::Closing;
}

void Connection::reset() {
    write_buffer_.clear();
    write_offset_ = 0;

    parser_.reset();
}

// ConnectionState Connection::state() {
//     return state_;
// }
