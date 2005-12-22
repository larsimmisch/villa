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

class DBData(object):
    def __init__(self, id, pin, cli, email, flags):
        self.id = id
        self.pin = pin
        self.cli = cli
        self.email = email
        self.flags = flags

class Caller(object):
    def __init__(self, sequencer, world = None, trunk='any', spec='any'):
        self.trunk = trunk
        self.spec = spec
        self.world = world
        # self.device stays valid in world.leave and location.leave
        # use is_disconnected to avoid sending messages to disconnected
        # devices
        self.device = None
        self.location = None
        self.user_data = None
        self.sequencer = sequencer
        self.db = None
        self.dialog = None
        self.is_disconnected = False
        self.send(self, 'LSTN %s %s' % (trunk, spec))

    def __repr__(self):
        if not self.device:
            return super(Caller, self).__repr__()

        return self.device

    def enqueue(self, molecule):
        return self.send(self, 'MLCA %s %s', self.device,
                         molecule.as_command())

    def discard(self, from_policy, to_policy, channel = 'all',
                immediately = True):
        return self.send(self, 'MLDP %s %s %d %d %d', self.device, channel,
                         from_policy.priority, to_policy.priority, immediately)
        
    def stop(self, mid):
        return self.send(self, 'MLCD %s %d', self.device, mid)

    def disconnect(self):
        return self.send(self, 'DISC %s', self.device)

    def send(self, sender, command, *args, **kwargs):
        if not self.is_disconnected:
            return self.sequencer.send(sender, command % args,
                                       kwargs.get('tid_data', None))

    def startDialog(self, dialog):
        self.dialog = dialog
        
    def LSTN(self, event, user_data):
        self.is_disconnected = False
        self.device = event['device']
        d = event['data']
        self.details = CallDetails(d[0], d[2])
        log.debug('Connect request: %s %s', self.device, self.details)
        self.send(self, 'ACPT %s' % self.device)
    
    def ACPT(self, event, user_data):
        self.send(self, 'LSTN %s %s' % (self.trunk, self.spec))
        if self.world:
            self.world.enter(self)
        
    def MLCA(self, event, user_data):
        pass

    def DTMF(self, event):
        dtmf = event['data'][0]

        if self.dialog:
            if self.dialog.DTMF(self, dtmf):
                self.dialog = None
        elif self.location:
            self.location.DTMF(self, dtmf)

    def disconnected(self):
        disc = self.is_disconnected
        self.is_disconnected = True
        # avoid calling leave twice
        if not disc:
            if self.location:
                self.location.leave(self)
            if self.world:
                self.world.leave(self)
        self.device = None
        self.details = None
        self.location = None
        self.db = None
        self.dialog = None

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
