import socket

HOST = '127.0.0.1'
PORT = 7077
remote_host = 'www.cnbeta.com'
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))
s.sendall('GET http://%s/ HTTP/1.1\r\nHost: %s\r\nCache-Control: max-age=0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nUser-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/29.0.1547.62 Safari/537.36\r\nAccept-Language: en-US,en;q=0.8,zh-CN;q=0.6,zh;q=0.4\r\n\r\n' % (remote_host, remote_host))
while True:
    data = s.recv(4096)
    if not data:
        print 'Something is wrong, or the stream end reached.'
        break;
    print 'Received: ', repr(data)
s.close()
