#!/usr/bin/env python

import sys
import string
import time
import socket
import getopt
import logging
from twisted.internet import reactor
from twisted.protocols.basic import LineReceiver
from twisted.internet.protocol import ReconnectingClientFactory

log = logging.getLogger('sequencer')

_device_add = [ 'LSTN', 'CONN', 'BGRO' ]
_device_remove = [ 'DISC', 'BGRC' ]

class Transactions:

    def __init__(self):
        self.tid = 0
        self.transactions = {}

    def create(self, sender, d = None):
        self.tid = self.tid + 1
        # only return valid transaction IDs - '-1' is reserved
        if self.tid == -1:
            self.tid = 1

        self.transactions[self.tid] = (sender, d)
        return self.tid

    def remove(self, tid):
        if not self.transactions.has_key(tid):
            log.error('tid %d not found. Probable protocol violation',
                      tid)
            sys.exit(1)
        else:
            sender, d = self.transactions[tid]
            del self.transactions[tid]

            return sender, d

    def find(self, tid):
        if not self.transactions.has_key(tid):
            return None
        
        return self.transactions[tid]

class Trunk:
    def __init__(self, name, number, lines):
        self.name = name
        self.number = number
        self.lines = lines

    def __repr__(self):
        return 'Trunk(%s, %s, %d)' % (self.name, self.number, self.lines)

class Sequencer(LineReceiver):
    
    def __init__(self, start):
        self.start = start
        self.initialized = False
        self.transactions = Transactions()

        # initialise devices for unsolicited events
        self.devices = {}
        self.trunks = []

    def DESC(self, event, user_data):
        d = event['data']
        for i in range(0, len(d), 3):
            self.trunks.append(Trunk(d[i], d[1], int(d[2])))

        if self.start:
            self.start(self)

    def send(self, sender, command, tid_data = None):
        # perform next transaction/command
        tid = self.transactions.create(sender, tid_data)
        cmd = '%d %s' % (tid, command)

        # update self.devices
        split = command.split(' ')
        action = split[0]
        if action in _device_remove:
            device = split[1]
            if device in self.devices:
                del self.devices[device]

        log.debug('sent: %s', cmd)
        
        self.sendLine(cmd)

        return tid

    def parse(self, line):
        """parse the event. `line' is supposed to be built from
        the transaction-id, the result of the operation, the action that
        was just performed and additional data.
        """

        status = None
        tid = None
        action = None
        device = None
        data = None
        
        tr = line.split(' ')
        # check if unsolicited event
        try:
            status = int(tr[0])
        except ValueError:
            log.error('Invalid event/response: status must be an int: %s',
                      line)
            return

        if status >= 200:
            try:
                tid = long(tr[1])
            except ValueError:
                log.error('Invalid response: tid must be a long int: %s',
                          line)
                return

            # remove the tid
            del tr[1]

        action = tr[1]
        device = tr[2]
        if len(tr) >= 3:
            data = tr[3:]
                
        # the sequencer protocol uses dashes '-'
        # - we need to translate them
        # to underscores because dashes are illegal in Python identifiers
        action = string.replace(action, '-', '_')

        # update self.devices
        if action in _device_add:
            self.devices[device] = self.transactions.find(tid)[0]

        event = { 'action': action, 'device': device, 'data': data }
        if status:
            event['status'] = status

        if status == 100:
            if not self.devices.has_key(device):
                log.warning('No receiver found for %s '
                            '(probably already disconnected)', device)
                return
            
            receiver = self.devices[device]
            m = getattr(receiver, action, None)
            if not m:
                log.error('Receiver %s does not implement: %s',
                          receiver, action)
                return
                
            m(event)
        else:
            receiver, user_data = self.transactions.remove(tid)
            event['tid'] = tid
            m = getattr(receiver, action, None)
            if not m:
                log.error('Receiver %s does not implement: %s',
                          receiver, action)
                return
            
            if status >= 600:
                log.error('Fatal sequencer error: %s', line)
            else:
                m(event, user_data)
                                        
    def lineReceived(self, line):
        if not self.initialized:
            self.send(self, 'DESC')
            self.initialized = True
        else:
            log.debug('rcvd: %s', line)
            self.parse(line)

class SequencerFactory(ReconnectingClientFactory):
    def __init__(self, start = None, stop = None):
        self.start = start
        self.stop = stop

    def buildProtocol(self, addr):
        log.info('Connected to %s', addr)
        return Sequencer(self.start)

    def clientConnectionLost(self, connector, reason):
        log.info('Lost connection.  Reason: %s', reason)
        if self.stop:
            self.stop()
        ReconnectingClientFactory.clientConnectionLost(self, connector, reason)

    def clientConnectionFailed(self, connector, reason):
        log.info('Connection failed. Reason: %s', reason)
        ReconnectingClientFactory.clientConnectionFailed(self, connector,
                                                         reason)

callLater = reactor.callLater

def run(host = 'localhost', port = 2104, start = None, stop = None,
        loglevel = logging.DEBUG):

    from util import defaultLogging

    defaultLogging(loglevel)

    reactor.connectTCP(host, port, SequencerFactory(start, stop))
    reactor.run()

if __name__ == '__main__':
    run()
