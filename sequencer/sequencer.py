import sys
import string
import time
import socket
import getopt

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
            print "error: tid %d not found. probable protocol violation" % (tid)
            sys.exit(1)
        else:
            sender, d = self.transactions[tid]
            del self.transactions[tid]

            return sender, d

    def find(self, tid):
        if not self.transactions.has_key(tid):
            return None
        
        return self.transactions[tid]

class Sequencer:
    
    def __init__(self,host,port):
        self.transactions = Transactions()
        self.device = None

        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((host, port))

        self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

        # make file from socket
        self.stdin=self.socket.makefile("r")
        self.stdout=self.socket.makefile("w")
        # initialise devices for unsolicited events
        self.devices = {}
        # read and ignore initial 'sequence protocol...'
        self.stdin.readline()
        # open one conference
        # self.send(self, "CNFO")

    def CNFO(self, event, data):
        self.conference = event['data'][0]
        print "conference:", self.conference

    def send(self, sender, command, data = None):
        # perform next transaction/command
        tid = self.transactions.create(sender, data)
        cmd = '%d %s\r\n' % (tid, command)

        # update self.devices
        split = command.split(' ')
        action = split[0]
        if action in _device_remove:
            device = split[1]
            if  device in self.devices:
                del self.devices[device]

        print 'command sent to sequencer:', cmd
        
        self.stdout.write(cmd)
        self.stdout.flush()

        return tid

    def parse(self, line):
        """parse the event. `line' is supposed to be built from
        the transaction-id, the result of the operation, the action that
        was just performed and additional data.
        """

        result = None
        action = None
        device = None
        data = None
        
        tr = line.split(' ')
        # check if unsolicited event
        if tr[0] == '100':
            if len(tr) < 3:
                print "parse error:", line
                return

            action = tr[1]
            device = tr[2]
            if len(tr) >= 3:
                data = tr[3:]
        else:
            if len(tr) < 4:
                print "parse error:", line
                return

            result = tr[0]
            action = tr[2]
            device = tr[3]
            if len(tr) >= 4:
                data = tr[4:]
                
        # the sequencer protocol uses dashes '-'
        # - we need to translate them
        # to underscores because dashes are illegal in Python identifiers
        action = string.replace(action, '-', '_')

        # update self.devices
        if action in _device_add:
            self.devices[device] = self.transactions.find(long(tr[1]))[0]

        event = { 'action': action, 'device': device, 'data': data }
        if result:
            event['result'] = result

        if tr[0] == '100':
            if not self.devices.has_key(device):
                print "warning - no receiver found for %s (probably already disconnected)" % device
                return
            
            receiver = self.devices[device]
            receiver.__class__.__dict__[action](receiver, event)
        else:
            receiver, user_data = self.transactions.remove(long(tr[1]))
            event['tid'] = long(tr[1])
            receiver.__class__.__dict__[action](receiver, event, user_data)
            
    def process(self):
        event = self.stdin.readline()[:-2]
        print "event from sequencer: " + event
        self.parse(event)

    def run(self):
        while 1:
            self.process()
