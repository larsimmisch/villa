#!/usr/bin/env caller
import logging
import sequencer

log = logging.getLogger('caller')

class Caller:
    def __init__(self, sequencer, trunkname='any', spec='any'):
        self.sequencer = sequencer
        self.send(self, 'LSTN %s %s' % (trunkname, spec))

    def send(self, sender, command, data = None):
        return self.sequencer.send(sender, command, data)

    def LSTN(self, event, user_data):
        self.device = event['device']
        log.debug('connected: %s', self.device)
        self.send(self, 'ACPT %s' % self.device)
    
    def ACPT(self, event, user_data):
        pass

Callers = []

def start(sequencer):
    for t in sequencer.trunks:
        for i in range(t.lines):
            Callers.append(Caller(sequencer, t.name))

if __name__ == '__main__':
    sequencer.run(start = start)
