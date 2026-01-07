import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('localhost', 8080))
request1 = (
    b"GET / HTTP/1.1\r\n"
    b"Host: localhost\r\n"
    b"\r\n"
)
request2 = (
    b"GET / HTTP/1.1\r\n"
    b"Host: 127.0.0.1:8080\r\n"
    b"Connection: keep-alive\r\n"
    b"sec-ch-ua: \"Microsoft Edge\";v=\"143\", \"Chromium\";v=\"143\", \"Not A(Brand\";v=\"24\"\r\n"
    b"sec-ch-ua-mobile: ?0\r\n"
    b"sec-ch-ua-platform: \"Windows\"\r\n"
    b"Upgrade-Insecure-Requests: 1\r\n"
    b"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/143.0.0.0 Safari/537.36 Edg/143.0.0.0\r\n"
    b"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\r\n"
    b"Sec-Fetch-Site: none\r\n"
    b"Sec-Fetch-Mode: navigate\r\n"
    b"Sec-Fetch-User: ?1\r\n"
    b"Sec-Fetch-Dest: document\r\n"
    b"Accept-Encoding: gzip, deflate, br, zstd\r\n"
    b"Accept-Language: zh-CN,zh;q=0.9,en;q=0.8\r\n"
    b"\r\n"
)
s.sendall(request2)
response = s.recv(4096)
print(response.decode())

s.close()