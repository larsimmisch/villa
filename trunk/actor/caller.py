#!/usr/bin/env caller
import logging
import sequencer

log = logging.getLogger('caller')

class CallDetails:
    def __init__(self, called, calling):
        self.called = called
        self.calling = calling

    def __repr__(self):
        return 'called: %s, calling: %s' % (self.called, self.calling)

class Caller:
    def __init__(self, sequencer, trunk='any', spec='any'):
        self.trunk = trunk
        self.spec = spec
        self.sequencer = sequencer
        self.send(self, 'LSTN %s %s' % (trunk, spec))

    def send(self, sender, command, data = None):
        return self.sequencer.send(sender, command, data)

    def LSTN(self, event, user_data):
        self.device = event['device']
        d = event['data']
        self.details = CallDetails(d[0], d[2])
        log.debug('Connect request: %s %s', self.device, self.details)
        self.send(self, 'ACPT %s' % self.device)
    
    def ACPT(self, event, user_data):
        pass

    def RDIS(self, event):
        self.send(self, 'LSTN %s %s' % (self.trunk, self.spec))
        self.send(self, 'DISC %s 0' % self.device)

    def DISC(self, event, user_data):
        self.device = None
        self.details = None
        log.debug('disconnected: %s', event['device'])

Callers = []

def start(sequencer):
    for t in sequencer.trunks:
        for i in range(t.lines):
            Callers.append(Caller(sequencer, t.name))

def stop():
    pass

if __name__ == '__main__':
    sequencer.run(start = start, stop = stop)
