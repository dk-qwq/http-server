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

std::string to_string(const HttpMethod& method) {
    switch(method) {
        case HttpMethod::GET:
            return "GET";
        case HttpMethod::HEAD:
            return "HEAD";
        case HttpMethod::POST:
            return "POST";
        case HttpMethod::PUT:
            return "PUT";
        case HttpMethod::DELETE:
            return "DELETE";
        case HttpMethod::CONNECT:
            return "CONNECT";
        case HttpMethod::OPTIONS:
            return "OPTIONS";
        case HttpMethod::TRACE:
            return "TRACE";
        case HttpMethod::PATCH:
            return "PATCH";
        default:
            throw std::invalid_argument("Invalid HTTP method");
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

std::string to_string(const HttpResponse& response) {
    std::ostringstream oss;
    oss << to_string(response.version()) << ' '
        << static_cast<int>(response.status_code()) << ' '
        << to_string(response.status_code()) << "\r\n";
    for(const auto& [key, value]: response.header()) {
        oss << key << ": " << value << "\r\n";
    }
    oss << "\r\n";
    oss << response.content();
    return oss.str();
}

std::string to_string(const HttpRequest& request) {
    std::ostringstream oss;
    oss << to_string(request.method()) << ' '
        << request.uri().path() << ' '
        << to_string(request.version()) << "\r\n";
    for(const auto& [key, value]: request.header()) {
        oss << key << ": " << value << "\r\n";
    }
    oss << "\r\n";
    oss << request.content();
    return oss.str();
}
