import socket,sys,time,getopt,string

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
		# check if unsolicited event
		if tr[0] == '100':
			if len(tr) < 3:
				print "parse error:", line
				return
			
			event = { 'action': tr[2], 'device': tr[3] }

			if len(tr) >= 3:
				event['data'] = tr[3:]
		else:
			if len(tr) < 4:
				print "parse error:", line
				return
			
			event = { 'result': tr[0], 'action': tr[2],
                      'device': tr[3] }
		
			if len(tr) >= 4:
				event['data'] = tr[4:]
				
		action = event['action']

		# the sequencer protocol uses dashes '-'
		# - we need to translate them
		# to underscores because dashes are illegal in Python identifiers
		action = string.replace(action, '-', '_')
		device = event['device']
		
		if tr[0] == '100':
			if not self.devices.has_key(device):
				print "warning - no receiver found (probably already disconnected)"
				return
			
			receiver = self.devices[device]
			receiver.__class__.__dict__[action](receiver, event)
		else:
			receiver, data = self.transactions.remove(long(tr[1]))
			receiver.__class__.__dict__[action](receiver, event, data)
			
	def process(self):
		event = self.stdin.readline()[:-2]
		print "event from sequencer: " + event
		self.parse(event)

	def run(self):
		while 1:
			self.process()
