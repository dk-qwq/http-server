#include <iostream>
#include <string>

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

    // server.RegisterHttpRequestHanlder("/", HttpMethod::HEAD, say_hello);
    server.RegisterHttpRequestHanlder("/", HttpMethod::GET, say_hello);

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