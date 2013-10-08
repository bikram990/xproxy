import socket

#HOST = '203.208.37.20'
HOST = '74.125.128.60'
PORT = 80
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))

content = 'GET http://kelvinh.github.io/ HTTP/1.1\r\nHost: kelvinh.github.io\r\nProxy-Connection: keep-alive\r\nCache-Control: max-age=0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.69 Safari/537.36\r\nAccept-Encoding: gzip,deflate,sdch\r\nAccept-Language: en-US,en;q=0.8,zh-CN;q=0.6,zh;q=0.4\r\n\r\n'

data = "POST /proxy HTTP/1.1\r\nHost: 0x77ff.appspot.com\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:24.0) Gecko/20100101 Firefox/24.0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nAccept-Language: null\r\nAccept-Encoding: gzip, deflate\r\nConnection: keep-alive\r\nContent-Length: %d\r\nContent-Type: text/plain; charset=UTF-8\r\nPragma: no-cache\r\nCache-Control: no-cache\r\n\r\n%s" % (len(content), content)
#data = "POST /proxy HTTP/1.1\r\nHost: 0x77ff.appspot.com\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:24.0) Gecko/20100101 Firefox/24.0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nAccept-Language: null\r\nAccept-Encoding: gzip, deflate\r\nConnection: keep-alive\r\nContent-Length: %d\r\nContent-Type: text/plain; charset=UTF-8\r\nPragma: no-cache\r\nCache-Control: no-cache\r\n\r\n%s" % (len(content), content)

print data

s.sendall(data)

while True:
    data = s.recv(4096)
    if not data:
        print 'Something is wrong, or the stream end reached.'
        break;
    print 'Received: ', repr(data)
s.close()
