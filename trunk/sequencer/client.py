"""
simple sequencer client:
connect to sequencer, accept incoming call[, play sample] and hangup

$Id: client.py,v 1.6 2001/09/25 09:01:45 lars Exp $
"""
import socket,re,sys,time,getopt,string

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
		# open one conference
		self.send(self, "global open-conference")

	def open_conference_done(self, event, data):
		self.conference = event['data'][0]
		print "conference:", self.conference

	def send(self, sender, command, data = None):
		# perform next transaction/command
		tid = self.transactions.create(sender, data)
		cmd = str(tid) + ' ' + command

		print "command sent to sequencer: " + cmd
        
		self.stdout.write(cmd + '\r\n')
		self.stdout.flush()

	def parse(self, line):
		"""parse the event. `line' is supposed to be built from
		the transaction-id, the result of the operation, the action that
		was just performed and additional data.
		"""

		tr = string.split(line, ' ')
		print tr
		# check if unsolicited event
		if tr[0] == '-1':
			if len(tr) < 3:
				print "parse error:", line
				return
			
			event = { 'device': tr[1], 'action': tr[2] }

			if len(tr) >= 3:
				event['data'] = tr[3:]
		else:
			if len(tr) < 4:
				print "parse error:", line
				return
			
			event = { 'result': tr[1], 'device': tr[2],
					  'action': tr[3] }
		
			if len(tr) >= 4:
				event['data'] = tr[4:]
				
		action = event['action']

		# the sequencer protocol uses dashes '-'
		# - we need to translate them
		# to underscores because dashes are illegal in Python identifiers
		action = string.replace(action, '-', '_')
		
		if tr[0] == '-1':
			device = self.devices[event['device']]
			device.__class__.__dict__[action](device, event)
			return

		sender, data = self.transactions.remove(long(tr[0]))
		sender.__class__.__dict__[action](sender, event, data)
			
	def process(self):
		event = self.stdin.readline()[:-2]
		print "event from sequencer: " + event
		self.parse(event)

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
		print 'connected:', self.device
		interface.send(self, self.device + ' add 2 1 play sitrtoot.al none')

	def molecule_done(self, event, data):
 		interface.send(self, 'global listen any any') 
 		interface.send(self, self.device + ' disconnect')
		pass

	def disconnect_done(self, event, data):
		del interface.devices[self.device]
		self.device = None

	def disconnect(self, event):
		interface.send(self, 'global listen any any')
 		interface.send(self, self.device + ' disconnect')

	def touchtone(self, event):
		tt = event['data'][0]
		print "touchtone:", tt

		if tt == '#':
	 		interface.send(self, 'global listen any any') 
			interface.send(self, self.device + ' disconnect')
		elif tt == '0':
			interface.send(self, self.device + ' add 2 1 conference '
						   + interface.conference + ' none')


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

	interface = Interface(hostname, 2104)

	calls = []
	# start calls
	for i in range(ncalls):
		calls.append(Call(interface))

	interface.run()
