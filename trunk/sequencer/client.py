"""
simple sequencer client:
connect to sequencer, accept incoming call[, play sample] and hangup

$Id: client.py,v 1.10 2001/10/07 00:48:13 lars Exp $
"""

import sys,getopt
from sequencer import *

class Call:

	def __init__(self, sequencer):
		self.device = None
		self.sequencer = sequencer

		self.sequencer.send(self, 'global listen any any') 

	def send(self, cmd):
		self.sequencer.send(self, cmd)

	def restart(self):
		d = self.device
		self.device = None
		# we don't want any unsolicited events after we sent a disconnect
		del self.sequencer.devices[d]
 		self.send(d + ' disconnect 0')
		
	def listen_done(self, event, data):
		self.device = event['device']
		self.sequencer.devices[self.device] = self
		# queue next listen as early as possible
 		self.send('global listen any any')
		print 'connected:', self.device
		self.send(self.device + ' add 2 1 play sitrtoot.al none')
		
	def molecule_done(self, event, data):
		self.restart()
		
	def disconnect_done(self, event, data):
		print "disconnected:", event['device']

	def disconnect(self, event):
		print "remote disconnect:", event['device']
		self.restart()
	
	def touchtone(self, event):
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
