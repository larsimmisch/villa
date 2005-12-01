#!/usr/bin/env caller
import logging
import sequencer

log = logging.getLogger('caller')

class CallDetails(object):
    def __init__(self, called, calling):
        self.called = called
        self.calling = calling

    def __repr__(self):
        return 'called: %s, calling: %s' % (self.called, self.calling)

class Caller(object):
    def __init__(self, sequencer, world = None, trunk='any', spec='any'):
        self.trunk = trunk
        self.spec = spec
        self.world = world
        self.device = None
        self.location = None
        self.user_data = None
        self.sequencer = sequencer
        self.send(self, 'LSTN %s %s' % (trunk, spec))

    def __repr__(self):
        if not self.device:
            return super(Caller, self).__repr__()

        return self.device

    def enqueue(self, molecule):
        id = self.send(self, 'MLCA %s %s' %
                       (self.device, molecule.as_command()))

    def send(self, sender, command, tid_data = None):
        return self.sequencer.send(sender, command, tid_data)

    def LSTN(self, event, user_data):
        self.device = event['device']
        d = event['data']
        self.details = CallDetails(d[0], d[2])
        log.debug('Connect request: %s %s', self.device, self.details)
        self.send(self, 'ACPT %s' % self.device)
    
    def ACPT(self, event, user_data):
        self.send(self, 'LSTN %s %s' % (self.trunk, self.spec))
        if self.world:
            self.world.enter(self)

    def disconnected(self):
        if self.world:
            self.world.leave(self)
        self.device = None
        self.details = None

    def RDIS(self, event):
        self.send(self, 'DISC %s 0' % self.device)
        self.disconnected()

    def DISC(self, event, user_data):
        self.disconnected()
        log.debug('disconnected: %s', event['device'])

if __name__ == '__main__':
    Callers = []

    def start(sequencer, world = None):
        for t in sequencer.trunks:
            for i in range(t.lines):
                Callers.append(Caller(sequencer, world, t.name))

    def stop():
        pass

    sequencer.run(start = start, stop = stop)
