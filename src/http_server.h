#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <string>
#include <cstdint>
#include <thread>
#include <unordered_map>
#include <functional>
#include <utility>

#include <http_message.h>

using HttpRequestHandler_t = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
public:
    explicit HttpServer(const std::string& host, const std::uint16_t& port);
    ~HttpServer() = default;

    HttpServer(HttpServer&&) = default;
    HttpServer& operator=(HttpServer&&) = default;

    void Start();
    void Stop();

    void RegisterHttpRequestHanlder(const std::string& path, HttpMethod method,
                                    const HttpRequestHandler_t& callback) {
        RegisterHttpRequestHanlder(Uri(path), method, callback);
    }

    void RegisterHttpRequestHanlder(const Uri& uri, HttpMethod method,
                                    const HttpRequestHandler_t& callback) {
        request_hanlders_[uri].insert(std::make_pair(method, callback));
    }

private:
    static constexpr int KBackLogSiz = 1000;

    bool running_;

    std::string host_;
    std::uint16_t port_;
    int server_fd_;
    std::thread listener_thread_;
    std::unordered_map<int, std::string> buffers;

    std::map<Uri, std::map<HttpMethod, HttpRequestHandler_t>> request_hanlders_;
    
    void Listen();

    HttpResponse HandlerHttpRequest(const HttpRequest& request);
};


#endif