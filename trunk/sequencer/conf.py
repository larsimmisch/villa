"""
conference test sequencer client:

allocate conference, listen, add callers to conference

$Id$
"""

import sys,getopt
from sequencer import *

caller = 0

class Conftest:

    def __init__(self, sequencer):
        self.device = None
        self.background = None
        self.conf = None
        self.sequencer = sequencer
        self.send('CNFO')

    def send(self, cmd):
        self.sequencer.send(self, cmd)

    def CNFO(self, event, data):
        self.conf = event['device']
        print 'conference is: ', self.conf
        self.send('LSTN any any')

    def LSTN(self, event, data):
        self.device = event['device']
        self.sequencer.devices[self.device] = self
        # queue next listen as early as possible
        self.send('LSTN any any')
        # self.send('DISC ' + self.device)
        self.send('ACPT ' + self.device)
        print 'connected:', self.device

    def ACPT(self, event, data):
        mode = 'duplex'
        
        global caller
        caller = caller + 1

        if caller == 2:
            mode = 'speak'
        
        self.send('MLCA %s 0 0 1 conf %s %s'
                  % (self.device, self.conf, mode))
    
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
        channels.append(Conftest(sequencer))

    sequencer.run()
