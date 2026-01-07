#include <iostream>
#include <string>
#include <sstream>

#include <config.h>
#include <http_server.h>
#include <http_message.h>


int main() {
    HttpServer server(kDefaultHost, kDefaultPort);
    auto say_hello = [](const HttpRequest& request) -> HttpResponse {
        HttpResponse response(HttpStatusCode::Ok);
        response.SetHeader("Content-Type", "text/plain");
        response.SetContent("hello, world");
        return response;
    };
    auto time_html = [](const HttpRequest& request) -> HttpResponse {
        HttpResponse response(HttpStatusCode::Ok);
        response.SetHeader("Content-Type", "text/html");
        std::time_t now = std::time(nullptr);
        std::ostringstream oss;
        oss << "<!DOCTYPE html>\r\n"
            << "<html lang=\"en\">\r\n"
            << "<head>\r\n"
            << "    <meta charset=\"UTF-8\">\r\n"
            << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n"
            << "    <title>Time Page</title>\r\n"
            << "</head>\r\n"
            << "<body>\r\n"
            << "    <h1>This is the time page</h1>\r\n"
            << "    <p>Current server time is: " << std::ctime(&now) << "</p>\r\n"
            << "    <a href=\"/\">Go to Home</a>\r\n"
            << "</body>\r\n"
            << "</html>\r\n";
        response.SetContent(oss.str());
        return response;
    };

    server.RegisterHttpRequestHandler("/", HttpMethod::HEAD, say_hello);
    server.RegisterHttpRequestHandler("/", HttpMethod::GET, say_hello);
    server.RegisterHttpRequestHandler("/time", HttpMethod::HEAD, time_html);
    server.RegisterHttpRequestHandler("/time", HttpMethod::GET, time_html);
    

    try {
        std::cout << "Server start running.." << std::endl;
        server.Start();
        std::cout << "Server listening on " << kDefaultHost << ':' << kDefaultPort << std::endl;

        std::cout << "Enter [quit] to stop the server" << std::endl;

        std::string command;
        while(std::cin >> command, command != "quit") ;

        std::cout << "'quit' command entered. Stopping the web server.." << std::endl;
        server.Stop();
        std::cout << "Server stopped" << std::endl;
    } catch(std::exception& e) {
        std::cout << "An error occurred: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}