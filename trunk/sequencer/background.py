"""
simple sequencer client:

allocate background channel, start play and deallocate

$Id: background.py,v 1.3 2004/01/15 17:18:59 lars Exp $
"""

import sys,getopt
from sequencer import *

class Background:

    def __init__(self, sequencer):
        self.device = None
        self.sequencer = sequencer
        self.conf = None
        self.send('CNFO')

    def CNFO(self, event, data):
        self.conf = event['device']
        print 'conference is: ', self.conf
        self.send('BGRO') 

    def send(self, cmd):
        self.sequencer.send(self, cmd)

    def BGRO(self, event, data):
        self.device = event['device']
        self.sequencer.devices[self.device] = self
        print 'allocated:', self.device
        self.send('MLCA %s 0 2 1 play ../test/phone/sitrtoot none'
                  % self.device)

    def BGRC(self, event, data):
        sys.exit(0)

    def MLCA(self, event, data):
        pass

if __name__ == '__main__':

    # check commandline arguments
    optlist, args = getopt.getopt(sys.argv[1:], 's:c:', ['server=','channels='])

    nchannels = 1
    hostname = None

    for opt in optlist:
        if opt[0] == '-s' or opt[0] == '--server':
            hostname = opt[1]
        elif opt[0] == '-c' or opt[0] == '--calls':
            print opt
            nchannels = long(opt[1])

    if hostname == None:
        hostname = socket.gethostname()

    sequencer = Sequencer(hostname, 2104)

    channels = []
    # start calls
    for i in range(nchannels):
        channels.append(Background(sequencer))

    sequencer.run()
