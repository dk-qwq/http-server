#include "http_parser.h"

#include <sstream>

HttpParser::HttpParser()
    :state_(ParseState::REQUEST_LINE),
    buffer(),
    content_length_(),
    body_read_(),
    request_() {}

void HttpParser::feed(const char* data, size_t len) {
    buffer.append(data, len);
    parse();
}

void HttpParser::parse() {
    if(state_ == ParseState::REQUEST_LINE) {
        if(parse_request_line()) state_ = ParseState::HEADERS;
    }
    if(state_ == ParseState::HEADERS) {
        if(parse_headers()) state_ = ParseState::BODY;
    }
    if(state_ == ParseState::BODY) {
        if(parse_body()) state_ = ParseState::DONE;
    }
}

bool HttpParser::is_done() const {
    return state_ == ParseState::DONE;
}

bool HttpParser::has_error() const {
    return state_ == ParseState::ERROR;
}

const HttpRequest& HttpParser::request() const {
    return request_;
}

void HttpParser::reset() {
    *this = HttpParser();
}

bool HttpParser::parse_request_line() {
    size_t lpos = 0, rpos = 0;
    rpos = buffer.find("\r\n", lpos);
    if(rpos == std::string::npos) return false;

    std::istringstream iss;
    std::string start_line;

    start_line = buffer.substr(lpos, rpos - lpos);
    buffer.erase(lpos, rpos - lpos + 2);
    iss.str(start_line);

    std::string method_string, path, version_string;
    if(iss >> method_string >> path >> version_string) ;
    else {
        state_ = ParseState::ERROR;
    }
    if(!iss.good() && !iss.eof()) {
        state_ = ParseState::ERROR;
    }
    if(state_ == ParseState::ERROR) return false;

    try {
        request_.SetMethod(method_string);
        request_.SetUri(path);
        request_.SetVersion(version_string);
    } catch(const std::exception& e) {
        state_ = ParseState::ERROR;        
    }

    if(state_ == ParseState::ERROR) return false;
    return true;
}

bool HttpParser::parse_headers() {
    size_t lpos = 0, rpos = 0;
    rpos = buffer.find("\r\n\r\n", lpos);
    if(rpos == std::string::npos) return false;

    std::string headers, line, key, value;
    headers = buffer.substr(lpos, rpos - lpos);
    buffer.erase(lpos, rpos - lpos + 4);
    std::istringstream iss;

    iss.str(headers);
    while(std::getline(iss, line)) {
        size_t colon_pos = line.find(": ");
        if(colon_pos == std::string::npos) {
            continue;
        }
        key = line.substr(0, colon_pos);
        value = line.substr(colon_pos + 2);
        
        auto is_space = [](char c) {
            return std::isspace(c);
        };
        std::erase_if(key, is_space);
        std::erase_if(value, is_space);

        request_.SetHeader(key, value);
    }

    std::string content_length_str = request_.header("Content-Length");
    if(!content_length_str.empty()) {
        try {
            content_length_ = std::stoul(content_length_str);
        } catch(const std::exception& e) {
            state_ = ParseState::ERROR;
            return false;
        }
    }

    return true;
}

bool HttpParser::parse_body() {
    size_t need = content_length_ - body_read_;
    if(buffer.size() < need) {
        request_.AppendContent(buffer);
        body_read_ += buffer.size();
        buffer.clear();
        return false;
    }

    request_.AppendContent(buffer.substr(0, need));
    buffer.erase(0, need);
    body_read_ += need;

    return true;
}
