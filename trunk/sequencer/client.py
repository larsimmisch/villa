"""
simple sequencer client:
connect to sequencer, accept incoming call[, play sample] and hangup

$Id: client.py,v 1.15 2003/12/07 23:35:44 lars Exp $
"""

import sys,getopt
from sequencer import *

class Call:

    def __init__(self, sequencer):
        self.device = None
        self.sequencer = sequencer

        self.sequencer.send(self, 'LSTN any any') 

    def send(self, cmd):
        self.sequencer.send(self, cmd)

    def restart(self):
        d = self.device
        self.device = None
        # we don't want any unsolicited events after we sent a disconnect
        del self.sequencer.devices[d]
        self.send('DISC %s 0' % d)
        
    def LSTN(self, event, data):
        self.device = event['device']
        self.sequencer.devices[self.device] = self
        # queue next listen as early as possible
        self.send('LSTN any any')
        # self.send('DISC ' + self.device)
        self.send('ACPT ' + self.device)
        print 'connected:', self.device

    def ACPT(self, event, data):
        self.send('MLCA %s 2 1 play ../test/phone/sitrtoot.wav none'
                  % self.device)

    def MLCA(self, event, data):
        self.send('DISC %s 0' % self.device)
    
    def DISC(self, event, data):
        print "disconnected:", event['device']

    def RDIS(self, event):
        print "remote disconnect:", event['device']
        self.restart()
    
    def DTMF(self, event):
        tt = event['data'][0]
        print "touchtone:", tt

        if tt == '#':
            self.restart()
        elif tt == '0':
            self.send(self.device + ' add 2 1 conference '
                           + sequencer.conference + ' none')


if __name__ == '__main__':

    # check commandline arguments
    optlist, args = getopt.getopt(sys.argv[1:], 's:c:', ['server=','calls='])

    ncalls = 1
    hostname = None

    for opt in optlist:
        if opt[0] == '-s' or opt[0] == '--server':
            hostname = opt[1]
        elif opt[0] == '-c' or opt[0] == '--calls':
            print opt
            ncalls = long(opt[1])

    if hostname == None:
        hostname = socket.gethostname()

    sequencer = Sequencer(hostname, 2104)

    calls = []
    # start calls
    for i in range(ncalls):
        calls.append(Call(sequencer))

    sequencer.run()
