"""
conference test sequencer client:

allocate conference, listen, add callers to conference

$Id$
"""

import sys,getopt
from sequencer import *

class Conftest:

    def __init__(self, sequencer):
        self.devices = []
        self.conf = None
        self.sequencer = sequencer
        self.send('CNFO')

    def send(self, cmd):
        self.sequencer.send(self, cmd)

    def CNFO(self, event, user_data):
        self.conf = event['device']
        print 'conference is: ', self.conf
        self.send('LSTN any any')

    def LSTN(self, event, user_data):
        device = event['device']
        self.devices.append(device)
        # queue next listen as early as possible
        self.send('LSTN any any')
        # self.send('DISC ' + self.device)
        self.send('ACPT ' + device)
        print 'connected:', device

    def ACPT(self, event, user_data):
        device = event['device']
        mode = 'duplex'
        
        self.send('MLCA %s 0 0 1 conf %s %s'
                  % (device, self.conf, mode))
    
    def MLCA(self, event, user_data):
        pass

    def DTMF(self, event):
        print event

    def DISC(self, event, user_data):
        pass

    def RDIS(self, event):
        self.send('DISC %s' % event['device'])

if __name__ == '__main__':

    # check commandline arguments
    optlist, args = getopt.getopt(sys.argv[1:], 's:c:',
                                  ['server=','channels='])

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
