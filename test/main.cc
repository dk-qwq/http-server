#include "http_message.h"
#include "http_parser.h"

#include <iostream>
#include <string>

int err = 0;

#define EXPECT_TRUE(x) \
    do  { \
        if (!(x)) { \
            std::cerr << "Expectation failed: " #x ", at " << __FILE__ << ":" << __LINE__ << std::endl; \
            err++; \
        } \
    } while (0)


void test_http_parser_full_request() {
    HttpParser parser;
    std::string raw_request =
        "POST /submit HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "hello world";

    parser.feed(raw_request.data(), raw_request.size());

    EXPECT_TRUE(parser.is_done());
    EXPECT_TRUE(!parser.has_error());

    const HttpRequest& parsed = parser.request();
    EXPECT_TRUE(parsed.method() == HttpMethod::POST);
    EXPECT_TRUE(parsed.uri().path() == "/submit");
    EXPECT_TRUE(parsed.version() == HttpVersion::HTTP_1_1);
    EXPECT_TRUE(parsed.header("Host") == "www.example.com");
    EXPECT_TRUE(parsed.header("Content-Length") == "11");
    EXPECT_TRUE(parsed.content() == "hello world");
}

void test_http_parser_partial_body() {
    HttpParser parser;
    std::string part1 =
        "GET /chunk HTTP/1.1\r\n"
        "Host: chunk.test\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "he";

    parser.feed(part1.data(), part1.size());
    EXPECT_TRUE(!parser.is_done());
    EXPECT_TRUE(!parser.has_error());

    const std::string part2 = "llo";
    parser.feed(part2.data(), part2.size());

    EXPECT_TRUE(parser.is_done());
    EXPECT_TRUE(!parser.has_error());

    const HttpRequest& parsed = parser.request();
    EXPECT_TRUE(parsed.method() == HttpMethod::GET);
    EXPECT_TRUE(parsed.uri().path() == "/chunk");
    EXPECT_TRUE(parsed.header("Host") == "chunk.test");
    EXPECT_TRUE(parsed.content() == "hello");
}

void test_http_parser_invalid_start_line() {
    HttpParser parser;
    const std::string bad_request = "INVALID\r\n\r\n";

    parser.feed(bad_request.data(), bad_request.size());

    EXPECT_TRUE(parser.has_error());
    EXPECT_TRUE(!parser.is_done());
}

// ========== HttpMessage Tests ==========

void test_http_version_conversion() {
    EXPECT_TRUE(string_to_version("HTTP/1.1") == HttpVersion::HTTP_1_1);
    EXPECT_TRUE(string_to_version("http/1.1") == HttpVersion::HTTP_1_1);
    EXPECT_TRUE(string_to_version("HTTP/1.0") == HttpVersion::HTTP_1_0);
    EXPECT_TRUE(string_to_version("HTTP/2.0") == HttpVersion::HTTP_2_0);
    EXPECT_TRUE(string_to_version("HTTP/0.9") == HttpVersion::HTTP_0_9);

    EXPECT_TRUE(to_string(HttpVersion::HTTP_1_1) == "HTTP/1.1");
    EXPECT_TRUE(to_string(HttpVersion::HTTP_1_0) == "HTTP/1.0");
    EXPECT_TRUE(to_string(HttpVersion::HTTP_2_0) == "HTTP/2.0");
}

void test_http_method_conversion() {
    EXPECT_TRUE(string_to_method("GET") == HttpMethod::GET);
    EXPECT_TRUE(string_to_method("get") == HttpMethod::GET);
    EXPECT_TRUE(string_to_method("POST") == HttpMethod::POST);
    EXPECT_TRUE(string_to_method("PUT") == HttpMethod::PUT);
    EXPECT_TRUE(string_to_method("DELETE") == HttpMethod::DELETE);
    EXPECT_TRUE(string_to_method("HEAD") == HttpMethod::HEAD);
    EXPECT_TRUE(string_to_method("CONNECT") == HttpMethod::CONNECT);
    EXPECT_TRUE(string_to_method("OPTIONS") == HttpMethod::OPTIONS);
    EXPECT_TRUE(string_to_method("TRACE") == HttpMethod::TRACE);
    EXPECT_TRUE(string_to_method("PATCH") == HttpMethod::PATCH);
}

void test_http_status_code_conversion() {
    EXPECT_TRUE(to_string(HttpStatusCode::Ok) == "OK");
    EXPECT_TRUE(to_string(HttpStatusCode::NotFound) == "Not Found");
    EXPECT_TRUE(to_string(HttpStatusCode::BadRequest) == "Bad Request");
    EXPECT_TRUE(to_string(HttpStatusCode::InternalServerError) == "Internal Server Error");
    EXPECT_TRUE(to_string(HttpStatusCode::Created) == "Created");
    EXPECT_TRUE(to_string(HttpStatusCode::MovedPermanently) == "Moved Permanently");
}

void test_http_request_basic() {
    HttpRequest request;

    request.SetMethod(HttpMethod::GET);
    request.SetUri("/test/path");
    request.SetVersion("HTTP/1.1");
    request.SetHeader("Host", "example.com");
    request.SetHeader("User-Agent", "TestAgent/1.0");
    
    EXPECT_TRUE(request.method() == HttpMethod::GET);
    EXPECT_TRUE(request.uri().path() == "/test/path");
    EXPECT_TRUE(request.version() == HttpVersion::HTTP_1_1);
    EXPECT_TRUE(request.header("Host") == "example.com");
    EXPECT_TRUE(request.header("User-Agent") == "TestAgent/1.0");
}

void test_http_request_with_content() {
    HttpRequest request;
    
    request.SetMethod("POST");
    request.SetUri("/api/data");
    request.SetVersion("HTTP/1.1");
    request.SetContent("test body content");
    
    EXPECT_TRUE(request.method() == HttpMethod::POST);
    EXPECT_TRUE(request.content() == "test body content");
    EXPECT_TRUE(request.header("Content-Length") == "17");
}

void test_http_request_append_content() {
    HttpRequest request;
    
    request.SetMethod(HttpMethod::POST);
    request.AppendContent("Hello");
    request.AppendContent(" ");
    request.AppendContent("World");
    
    EXPECT_TRUE(request.content() == "Hello World");
}

void test_http_response_basic() {
    HttpResponse response(HttpStatusCode::Ok);
    
    response.SetVersion("HTTP/1.1");
    response.SetHeader("Content-Type", "text/html");
    response.SetContent("<html><body>Hello</body></html>");
    
    EXPECT_TRUE(response.status_code() == HttpStatusCode::Ok);
    EXPECT_TRUE(response.version() == HttpVersion::HTTP_1_1);
    EXPECT_TRUE(response.header("Content-Type") == "text/html");
    EXPECT_TRUE(response.content() == "<html><body>Hello</body></html>");
}

void test_http_response_to_string() {
    HttpResponse response(HttpStatusCode::Ok);
    response.SetVersion("HTTP/1.1");
    response.SetHeader("Content-Type", "text/plain");
    response.SetContent("Test");
    
    std::string response_str = to_string(response);
    
    EXPECT_TRUE(response_str.find("HTTP/1.1 200 OK") != std::string::npos);
    EXPECT_TRUE(response_str.find("Content-Type: text/plain") != std::string::npos);
    EXPECT_TRUE(response_str.find("Content-Length: 4") != std::string::npos);
    EXPECT_TRUE(response_str.find("Test") != std::string::npos);
}

void test_http_response_status_codes() {
    HttpResponse response1(HttpStatusCode::NotFound);
    EXPECT_TRUE(response1.status_code() == HttpStatusCode::NotFound);
    
    HttpResponse response2(HttpStatusCode::InternalServerError);
    response2.SetStatusCode(HttpStatusCode::BadRequest);
    EXPECT_TRUE(response2.status_code() == HttpStatusCode::BadRequest);
}

void test_http_message_empty_header() {
    HttpRequest request;
    request.SetHeader("Host", "example.com");
    
    EXPECT_TRUE(request.header("Host") == "example.com");
    EXPECT_TRUE(request.header("NonExistent") == "");
}

int main() {
    std::cout << "Running tests..." << std::endl;

    // HTTP Parser Tests
    test_http_parser_full_request();
    test_http_parser_partial_body();
    test_http_parser_invalid_start_line();
    
    // HTTP Message Tests
    test_http_version_conversion();
    test_http_method_conversion();
    test_http_status_code_conversion();
    test_http_request_basic();
    test_http_request_with_content();
    test_http_request_append_content();
    test_http_response_basic();
    test_http_response_to_string();
    test_http_response_status_codes();
    test_http_message_empty_header();

    std::cout << "All tests have finished. There were "
                << err << " errors." << std::endl;
    return 0;
}