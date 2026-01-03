#include "http_message.h"

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

int main() {
    std::cout << "Running tests..." << std::endl;

    test_http_message();

    std::cout << "All tests have finished. There were "
                << err << " errors." << std::endl;
    return 0;
}