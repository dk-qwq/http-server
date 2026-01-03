#ifndef HTTP_MESSAGE_H_
#define HTTP_MESSAGE_H_

#include <string>
#include <map>

#include "uri.h"

// HTTP methods defined in the following document:
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods
enum class HttpMethod {
  GET,
  HEAD,
  POST,
  PUT,
  DELETE,
  CONNECT,
  OPTIONS,
  TRACE,
  PATCH
};

// Here we only support HTTP/1.1
enum class HttpVersion {
  HTTP_0_9 = 9,
  HTTP_1_0 = 10,
  HTTP_1_1 = 11,
  HTTP_2_0 = 20
};

// HTTP response status codes as listed in:
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
// Note that not all of them are present in this enum class
enum class HttpStatusCode {
  Continue = 100,
  SwitchingProtocols = 101,
  EarlyHints = 103,
  Ok = 200,
  Created = 201,
  Accepted = 202,
  NonAuthoritativeInformation = 203,
  NoContent = 204,
  ResetContent = 205,
  PartialContent = 206,
  MultipleChoices = 300,
  MovedPermanently = 301,
  Found = 302,
  NotModified = 304,
  BadRequest = 400,
  Unauthorized = 401,
  Forbidden = 403,
  NotFound = 404,
  MethodNotAllowed = 405,
  RequestTimeout = 408,
  ImATeapot = 418,
  InternalServerError = 500,
  NotImplemented = 501,
  BadGateway = 502,
  ServiceUnvailable = 503,
  GatewayTimeout = 504,
  HttpVersionNotSupported = 505
};


class HttpMessageInfterface {
public:
    HttpMessageInfterface(): version_(HttpVersion::HTTP_1_1) {}
    void SetHeader(const std::string& key, const std::string& value) {
        headers_[key] = std::move(value);
    }
    void SetContent(const std::string& content) {
        content_ = std::move(content);
    }
    HttpVersion version() const {
        return version_;
    }
    std::map<std::string, std::string> header() const {
        return headers_;
    }
    std::string header(const std::string& key) const {
        if(headers_.count(key)) return headers_.at(key);
        return std::string();
    }
    std::string content() const {
        return content_;
    }

protected:
    HttpVersion version_;
    std::map<std::string, std::string> headers_;
    std::string content_;

    friend std::string to_string(const HttpVersion& version);
};

std::string to_string(const HttpVersion& version);


HttpVersion string_to_version(const std::string& version_string);
HttpMethod string_to_method(const std::string& method_string);


class HttpRequest : public HttpMessageInfterface {
public:
    void SetMethod(HttpMethod method) {
        method_ = method;
    }
    void SetUri(const Uri& uri) {
        uri_ = uri;
    }
    HttpMethod method() const {
        return method_;
    }
    Uri uri() const {
        return uri_;
    }
private:
    HttpMethod method_;
    Uri uri_;

    friend HttpRequest string_to_request(const std::string& request_string);
};


class HttpResponse : public HttpMessageInfterface {
public:
    HttpResponse(HttpStatusCode status_code)
        :status_code_(status_code) {}
    void SetStatuscode(HttpStatusCode status_code) {
        status_code_ = status_code;
    }
    HttpStatusCode status_code() const {
        return status_code_;
    }
private:
    HttpStatusCode status_code_;

    friend std::string to_string(const HttpStatusCode& status_code);
    friend std::string to_string(const HttpResponse& response, bool Is_sendContent);
};

HttpRequest string_to_request(const std::string& request_string);

std::string to_string(const HttpStatusCode& status_code);
std::string to_string(const HttpResponse& response, bool Is_sendContent);

#endif