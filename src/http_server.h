#pragma once

#include <sys/epoll.h>

#include <string>
#include <cstdint>
#include <thread>
#include <unordered_map>
#include <functional>
#include <utility>
#include <random>

#include <http_message.h>

static constexpr size_t kMaxBufferSize = 4096;

size_t GetContentLength(char* buffer, size_t len);

struct EventData {
    EventData() : fd(0), length(SIZE_MAX), cursor(0), buffer() {}
    int fd;
    size_t length;
    size_t cursor;
    char buffer[kMaxBufferSize];
};

using HttpRequestHandler_t = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
public:
    explicit HttpServer(const std::string& host, const std::uint16_t& port);
    ~HttpServer() = default;

    HttpServer(HttpServer&&) = default;
    HttpServer& operator=(HttpServer&&) = default;

    void Start();
    void Stop();

    void RegisterHttpRequestHandler(const std::string& path, HttpMethod method,
                                    const HttpRequestHandler_t& callback) {
        RegisterHttpRequestHandler(Uri(path), method, callback);
    }

    void RegisterHttpRequestHandler(const Uri& uri, HttpMethod method,
                                    const HttpRequestHandler_t& callback) {
        request_handlers_[uri].insert(std::make_pair(method, callback));
    }

private:
    static constexpr int KBackLogSiz = 1000;

    static constexpr size_t kThreadPollSize = 5;
    static constexpr size_t kMaxConnections = 10000;
    static constexpr size_t kMaxEvents = 10000;
    
    bool running_;

    std::string host_;
    std::uint16_t port_;
    int server_fd_;
    std::thread listener_thread_;
    std::thread epoll_thread_[kThreadPollSize];
    int epoll_fd_[kThreadPollSize];
    epoll_event events_[kThreadPollSize][kMaxEvents];

    std::map<Uri, std::map<HttpMethod, HttpRequestHandler_t>> request_handlers_;

    std::mt19937 rng_;
    std::uniform_int_distribution<int> sleep_times_;
    
    void SetUpEpoll();
    void Listen();
    void ProcessEvent(int worker_id);
    void HandlerEpollEvent(int epoll_fd, std::uint32_t event, EventData* data);
    void ControlEpollEvent(int epoll_fd, int op, int fd,
                                 std::uint32_t event = 0, void* data = nullptr);

    HttpResponse HandlerHttpRequest(const HttpRequest& request);

    void HandleHttpData(const EventData& raw_request, EventData* raw_response);
};
