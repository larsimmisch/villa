import socket
import time
import sys

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
f = open('orientation.al', 'rb')

l = 0
d = f.read(192)
while d:
    s.sendto(d, ('localhost', 10001))
    l = l + len(d)
    print 'sent %d' % len(d)
    d = f.read(192)
    if d:
        time.sleep((len(d) - 50) / 8000.0)

f.close()
