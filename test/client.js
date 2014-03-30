#!/usr/bin/env node

var net = require('net');

var HOST = '127.0.0.1';
var PORT = 7077;

var socket = new net.Socket();
socket.connect(PORT, HOST, function() {
    console.log("Connected.");
    socket.write('GET http://www.baidu.com HTTP/1.1\r\nHost: www.baidu.com\r\n\r\n');
});

socket.on('data', function(data) {
    console.log('Response: ' + data);
});

socket.on('end', function() {
    socket.destroy();
});

socket.on('close', function() {
    console.log('Connection closed.');
});
