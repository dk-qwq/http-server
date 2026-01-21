#include "http_message.h"

#include <stdexcept>
#include <sstream>
#include <algorithm>

std::string to_string(const HttpVersion& version) {
    switch(version) {
        case HttpVersion::HTTP_0_9:
            return "HTTP/0.9";
        case HttpVersion::HTTP_1_0:
            return "HTTP/1.0";
        case HttpVersion::HTTP_1_1:
            return "HTTP/1.1";
        case HttpVersion::HTTP_2_0:
            return "HTTP/2.0";
        default:
            throw std::invalid_argument("Invalid HTTP version");
    }
}

HttpVersion string_to_version(const std::string& version_string) {
    std::string version_string_uppered;
    for(auto c: version_string) {
        version_string_uppered += static_cast<char>(toupper(c));
    }
    
    if(version_string_uppered == "HTTP/0.9") {
        return HttpVersion::HTTP_0_9;
    } else if(version_string_uppered == "HTTP/1.0") {
        return HttpVersion::HTTP_1_0;
    } else if(version_string_uppered == "HTTP/1.1") {
        return HttpVersion::HTTP_1_1;
    } else if(version_string_uppered == "HTTP/2.0") {
        return HttpVersion::HTTP_2_0;
    } else {
        throw std::invalid_argument("Invalid HTTP version string");
    }
}

HttpMethod string_to_method(const std::string& method_string) {
    std::string method_string_uppered;
    for(auto c: method_string) {
        method_string_uppered += static_cast<char>(toupper(c));
    }

    if(method_string_uppered == "GET") {
        return HttpMethod::GET;
    } else if(method_string_uppered == "HEAD") {
        return HttpMethod::HEAD;
    } else if(method_string_uppered == "POST") {
        return HttpMethod::POST;
    } else if(method_string_uppered == "PUT") {
        return HttpMethod::PUT;
    } else if(method_string_uppered == "DELETE") {
        return HttpMethod::DELETE;
    } else if(method_string_uppered == "CONNECT") {
        return HttpMethod::CONNECT;
    } else if(method_string_uppered == "OPTIONS") {
        return HttpMethod::OPTIONS;
    } else if(method_string_uppered == "TRACE") {
        return HttpMethod::TRACE;
    } else if(method_string_uppered == "PATCH") {
        return HttpMethod::PATCH;
    } else {
        throw std::invalid_argument("Invalid HTTP method string");
    }
}

std::string to_string(const HttpStatusCode& status_code) {
    switch(status_code) {
        case HttpStatusCode::Continue:
            return "Continue";
        case HttpStatusCode::SwitchingProtocols:
            return "Switching Protocols";
        case HttpStatusCode::EarlyHints:
            return "Early Hints";
        case HttpStatusCode::Ok:
            return "OK";
        case HttpStatusCode::Created:
            return "Created";
        case HttpStatusCode::Accepted:
            return "Accepted";
        case HttpStatusCode::NonAuthoritativeInformation:
            return "Non-Authoritative Information";
        case HttpStatusCode::NoContent:
            return "No Content";
        case HttpStatusCode::ResetContent:
            return "Reset Content";
        case HttpStatusCode::PartialContent:
            return "Partial Content";
        case HttpStatusCode::MultipleChoices:
            return "Multiple Choices";
        case HttpStatusCode::MovedPermanently:
            return "Moved Permanently";
        case HttpStatusCode::Found:
            return "Found";
        case HttpStatusCode::NotModified:
            return "Not Modified";
        case HttpStatusCode::BadRequest:
            return "Bad Request";
        case HttpStatusCode::Unauthorized:
            return "Unauthorized";
        case HttpStatusCode::Forbidden:
            return "Forbidden";
        case HttpStatusCode::NotFound:
            return "Not Found";
        case HttpStatusCode::MethodNotAllowed:
            return "Method Not Allowed";
        case HttpStatusCode::RequestTimeout:
            return "Request Timeout";
        case HttpStatusCode::ImATeapot:
            return "I'm a teapot";
        case HttpStatusCode::InternalServerError:
            return "Internal Server Error";
        case HttpStatusCode::NotImplemented:
            return "Not Implemented";
        case HttpStatusCode::BadGateway:
            return "Bad Gateway";
        case HttpStatusCode::ServiceUnvailable:
            return "Service Unavailable";
        case HttpStatusCode::GatewayTimeout:
            return "Gateway Timeout";
        case HttpStatusCode::HttpVersionNotSupported:
            return "HTTP Version Not Supported";
        default:
            throw std::invalid_argument("Invalid HTTP status code");
    }
}

std::string to_string(const HttpResponse& response, bool Is_sendContent) {
    std::ostringstream oss;
    oss << to_string(response.version()) << ' '
        << static_cast<int>(response.status_code()) << ' '
        << to_string(response.status_code()) << "\r\n";
    for(const auto& [key, value]: response.header()) {
        oss << key << ": " << value << "\r\n";
    }
    oss << "\r\n";
    if(Is_sendContent) oss << response.content() << "\r\n";
    return oss.str();
}

HttpRequest string_to_request(const std::string& request_string) {
    std::istringstream iss;
    std::string start_line, headers, body;
    std::string line;
    std::string method_string, path, version_string, key, value;
    HttpRequest request;
    size_t lpos = 0, rpos = 0;

    rpos = request_string.find("\r\n", lpos);
    if(rpos == std::string::npos) {
        throw std::invalid_argument("Could not find start line");
    }
    start_line = request_string.substr(lpos, rpos - lpos);

    lpos = rpos + 2;
    rpos = request_string.find("\r\n\r\n", lpos);
    if(rpos != std::string::npos) {
        headers = request_string.substr(lpos, rpos - lpos);

        if(rpos + 4 < request_string.length()) {
            body = request_string.substr(rpos + 4);
        }
    }

    iss.str(start_line);
    iss >> method_string >> path >> version_string;
    if(!iss.good() && !iss.eof()) {
        throw std::invalid_argument("Invalid start line format");
    }
    request.SetMethod(string_to_method(method_string));
    request.SetUri(Uri(path));
    request.version_ = string_to_version(version_string);

    iss.clear();
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

        request.SetHeader(key, value);

    }

    request.SetContent(body);

    return request;
}

