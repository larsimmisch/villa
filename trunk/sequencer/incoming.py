"""
simple sequencer client:
connect to sequencer, accept incoming call, wait for hangup

$Id: incoming.py,v 1.2 2001/09/30 09:51:57 lars Exp $
"""

import sys,getopt
from sequencer import *

class Call:

	def __init__(self, sequencer):
		self.sequencer = sequencer
		self.sequencer.send(self, 'global listen any any') 

	def send(self, cmd):
		self.sequencer.send(self, cmd)

	def listen_done(self, event, data):
		d = event['device']
		self.sequencer.devices[d] = self
		# queue next listen as early as possible
 		self.send('global listen any any')
		print 'connected:', d

	def disconnect_done(self, event, data):
		print "disconnected:", event['device']

	def disconnect(self, event):
		d = event['device']
		print "remote disconnect:", d
		del self.sequencer.devices[d]
 		self.send(d + ' disconnect')

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
