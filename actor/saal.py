#!/usr/bin/env python

import urllib
import os
import logging
import time
import threading
import socket

def run(outfile, port):
    log.debug('Saal %d reader thread started', port - 10000)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    while True:
        d = outfile.read(160)
        if not d:
            time.sleep(1.0)
        # log.debug('sent %d', len(d))
        s.sendto(d, ('127.0.0.1', port))

def recode(url, port):
    cmd = 'ffmpeg -f ogg -i - -vc none -f alaw -ar 8000 -ac 1 -'
    log.debug(cmd)
    
    infile, outfile = os.popen2(cmd, 'b')

    t = threading.Thread(target=run, name='Saal%d reader' % (port - 1000),
                         args=(outfile, port))
    t.setDaemon(True)
    t.start()
    
    while True:
        try:
            stream = urllib.urlopen(url)
        except:
            log.error('could not open %s', url, exc_info=1)

        log.debug('opened %s', url)

        while True:
            # 182 should be about our framerate
            s = stream.read(182)
            if not s:
                log.debug('no data from %s', url)
                time.sleep(10.0)
                break
            infile.write(s)


if __name__ == '__main__':
    
    logging.basicConfig(format = '%(asctime)s %(levelname)-5s %(message)s',
                        level=logging.DEBUG)
    log = logging.getLogger()

    streams = [('http://audio.ulm.ccc.de:8000/saal1.ogg', 10001),
               ('http://audio.ulm.ccc.de:8000/saal2.ogg', 10002),
               ('http://audio.ulm.ccc.de:8000/saal3.ogg', 10003),
               ('http://audio.ulm.ccc.de:8000/saal4.ogg', 10004)]
               
    for s in streams[1:]:
        t = threading.Thread(target=recode, name='Saal%d' % (s[1] - 10000),
                             args=(s[0], s[1]))
        t.setDaemon(True)
        t.start()

    recode(*streams[0])
