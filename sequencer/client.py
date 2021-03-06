"""
simple sequencer client:
connect to sequencer, accept incoming call[, play sample] and hangup

$Id$
"""

import sys,getopt
from sequencer import *

class Call:

    def __init__(self, sequencer):
        self.device = None
        self.record = None
        self.play = None
        self.sequencer = sequencer

        self.sequencer.send(self, 'LSTN any any') 

    def send(self, cmd):
        return self.sequencer.send(self, cmd)

    def restart(self):
        d = self.device
        self.device = None
        self.send('DISC %s 0' % d)
        
    def LSTN(self, event, user_data):
        self.device = event['device']
         # queue next listen as early as possible
        self.send('LSTN any any')
        # self.send('DISC ' + self.device)
        self.send('ACPT ' + self.device)
        print 'connected:', self.device

    def ACPT(self, event, user_data):
         self.send('MLCA %s 0 2 1 play ../test/phone/short.al none'
                   % self.device)
         self.send('MLCA %s 0 2 1 play ../test/phone/sitrtoot.al none'
                   % self.device)
##         self.send('MLCA %s 0 2 1 beep 2'
##                   % self.device)
##        self.send('MLCA %s 0 16 4 beep 1 none ' % self.device)
##       self.record = self.send('MLCA %s 0 80 4 rec foo 10000 none'
##                                % self.device)

    def MLCA(self, event, user_data):
        if event['tid'] == self.record:
            self.play = self.send('MLCA %s 0 2 1 play foo none' % self.device)
        elif event['tid'] == self.play:
            self.send('DISC %s 0' % self.device)
    
    def DISC(self, event, user_data):
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
