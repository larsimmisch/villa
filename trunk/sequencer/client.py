"""
simple sequencer client:
connect to sequencer, accept incoming call[, play sample] and hangup

$Id: client.py,v 1.3 2001/09/11 22:10:11 lars Exp $
"""
import socket,re,sys,time,getopt, string

class Transactions:

	def __init__(self):
		self.tid = 0
		self.transactions = {}

	def create(self, call, d = None):
		self.tid = self.tid + 1
		# only return valid transaction IDs - '-1' is reserved
		if self.tid == -1:
			self.tid = 1

		self.transactions[self.tid] = (call, d)
		return self.tid

	def remove(self, tid):
		if not self.transactions.has_key(tid):
			print "error: tid %d not found. probable protocol violation" % (tid)
			sys.exit(1)
		else:
			call, d = self.transactions[tid]
			del self.transactions[tid]

		return call, d

class Interface:
	
	def __init__(self,host,port):
		self.transactions = Transactions()

		self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.socket.connect((host, port))

		# make file from socket
		self.stdin=self.socket.makefile("r")
		self.stdout=self.socket.makefile("w")
		# initialise devices for unsolicited events
		self.devices = {}
		# read and ignore initial 'sequence protocol...'
		self.stdin.readline()

	def send(self, call, command, data = None):
		# perform next transaction/command
		tid = self.transactions.create(call, data)
		cmd = str(tid) + ' ' + command

		print "command sent to sequencer: " + cmd
        
		self.stdout.write(cmd + '\r\n')
		self.stdout.flush()

	def parse(self, line):
		"""parse the event. `line' is supposed to be built from
		the transaction-id, the result of the operation, the action that
		was just performed and additional data.
		"""
		m = re.match(r"(?P<tid>\w*) (?P<result>\d) (?P<device>\w*\[\d,\d\])"+
                   r" (?P<action>[\w\-_]*)(?P<data>.*)", line)
		
		if  m == None:
			print "parse error:", line
			return None
		else:
			return m.groupdict()
			
	def process(self):
		event = self.stdin.readline()[:-2]
		print "event from sequencer: " + event
		event = self.parse(event)

		if not event:
			return

		tid = event['tid']
		action = event['action']

		# the sequencer protocol uses dashes '-'  - we need to translate them
		# to underscores because dashes are illegal in Python identifiers
		action = string.replace(action, '-', '_')
		
		if tid == '-1':
			device = self.devices[event['device']]
			device.__class__.__dict__[action](device, event['data'])
			return

		device, data = self.transactions.remove(long(tid))
		device.__class__.__dict__[action](device, event, data)
		return

	def run(self):
		while 1:
			self.process()

class Call:

	def __init__(self, interface):
		self.device = None
		self.interface = interface

		interface.send(self, 'global listen any any') 
	
	def listen_done(self, event, data):
		self.device = event['device']
		interface.devices[self.device] = self

		interface.send(self, self.device + ' accept')

	def accept_done(self, event, data):
		interface.send(self, self.device + ' add 2 1 play sitrtoot.al none')

	def molecule_done(self, event, data):
		interface.send(self, 'global listen any any') 
		interface.send(self, self.device + ' disconnect')

	def disconnect_done(self, event, data):
		del interface.devices[self.device]
		self.device = None

if __name__ == '__main__':

	# check commandline arguments
	optlist, args = getopt.getopt(sys.argv[1:], 'n', ['name='])

	hostname = None

	for opt in optlist:
		if opt[0] == '-n' or opt[0] == '--name':
			hostname = args[optlist.index(opt)]

	if hostname == None:
		hostname = socket.gethostname()

	interface = Interface(hostname, 2104)

	# start one call
	call = Call(interface)

	interface.run()
