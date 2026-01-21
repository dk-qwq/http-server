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

void test_http_message() {
    HttpRequest request;
    std::string request_string =
        "GET /index.html HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "User-Agent: TestAgent/1.0\r\n"
        "\r\n"
        "body_content";
    request = string_to_request(request_string);
    EXPECT_TRUE(request.method() == HttpMethod::GET);
    EXPECT_TRUE(request.uri().path() == "/index.html");
    EXPECT_TRUE(request.version() == HttpVersion::HTTP_1_1);
    EXPECT_TRUE(request.header("Host") == "www.example.com");
    EXPECT_TRUE(request.header("User-Agent") == "TestAgent/1.0");
    EXPECT_TRUE(request.content() == "body_content");
}

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

int main() {
    std::cout << "Running tests..." << std::endl;

    test_http_message();
    test_http_parser_full_request();
    test_http_parser_partial_body();
    test_http_parser_invalid_start_line();

    std::cout << "All tests have finished. There were "
                << err << " errors." << std::endl;
    return 0;
}