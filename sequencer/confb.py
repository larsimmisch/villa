"""
conference background sequencer client:

allocate conference, background channel, add background to conference
listen, add caller to conference

$Id$
"""

import sys,getopt
from sequencer import *

class Conftest:

    def __init__(self, sequencer):
        self.device = None
        self.background = None
        self.conf = None
        self.sequencer = sequencer
        self.send('CNFO')

    def send(self, cmd):
        self.sequencer.send(self, cmd)

    def CNFO(self, event, user_data):
        self.conf = event['device']
        print 'conference is: ', self.conf
        self.send('BGRO') 

    def BGRO(self, event, user_data):
        self.background = event['device']
        print 'allocated:', self.background
        # add background to conference on channel 0
        self.send('MLCA %s 0 0 1 conf %s speak'
                  % (self.background, self.conf))
        # play loop in background on channel 1
        self.send('MLCA %s 1 32 1 play ../test/phone/wiese.al none'
                  % self.background)
        # wait for caller
        self.send('LSTN any any')

    def LSTN(self, event, user_data):
        self.device = event['device']
        # queue next listen as early as possible
        self.send('LSTN any any')
        # self.send('DISC ' + self.device)
        self.send('ACPT ' + self.device)
        print 'connected:', self.device

    def ACPT(self, event, user_data):
        self.send('MLCA %s 0 0 1 conf %s duplex'
                  % (self.device, self.conf))
    
    def BGRC(self, event, user_data):
        sys.exit(0)

    def MLCA(self, event, user_data):
        pass

    def DISC(self, event, user_data):
        self.send('BGRC ' + self.background)

    def RDIS(self, event):
        self.send('DISC ' + self.device)


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
        channels.append(Conftest(sequencer))

    sequencer.run()
