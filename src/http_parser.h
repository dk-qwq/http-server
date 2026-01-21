#pragma once

#include <string>

#include <http_message.h>

enum class ParseState {
    REQUEST_LINE,
    HEADERS,
    BODY,
    DONE,
    ERROR
};

class HttpParser{
public:
    HttpParser();
    
    void feed(const char* data, size_t len);

    void parse();

    bool is_done() const;

    bool has_error() const;

    const HttpRequest& request() const;

    void reset();
private:
    ParseState state_;

    std::string buffer;

    size_t content_length_;
    size_t body_read_;

    HttpRequest request_;

    bool parse_request_line();
    bool parse_headers();
    bool parse_body();
};