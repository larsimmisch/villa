"""
simple sequencer client:
connect to sequencer, accept incoming call[, play sample] and hangup

$Id: client.py,v 1.2 2001/08/07 22:16:36 lars Exp $
"""
import socket,re,sys,time,getopt

class sequencer_client:
    def __init__(self,host,port):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((host, port))

        # make file from socket :-)
        self.stdin=self.socket.makefile("r")
        self.stdout=self.socket.makefile("w")

        # will be increased on each command
        self.transaction_id=0

        # will be set to the something the sequencer tells us
        self.device='(unset)'

    def send_command(self, command):
        # perform next transaction/command
        self.transaction_id = self.transaction_id+1

        cmd=str(self.transaction_id)+' '+command+'\r\n'

        print "command sent to sequencer: "+cmd
        
        self.stdout.write(cmd)
        self.stdout.flush()

    def listen_to_any(self):
        self.send_command("global listen any any")

    def wait_for_response(self):
        print "wait for data from server"
        line=self.stdin.readline()
        print "response from server: "+line
        self.split_server_response(line)
        
    def split_server_response(self,line):
        """Split server's response. `line' is supposed to be built from
        the transaction-id, the result of the operation, the action that
        was just performed and additional data.
        """
        m=re.match(r"(?P<transaction>\w*) (?P<result>\d) (?P<device>\w*\[\d,\d\])"+
                   r" (?P<action>[\w\-_]*)(?P<data>.*)", line)
        if (m==None):
            self.response=None
            print "parsing the server's response failed"
        else:
            self.response=m.groupdict()

    def act_on_response(self):
        if (self.response==None):
            print "(response couldn't be parsed)"
            return

        action=self.response['action']

        if (action=='alerting'):
            self.device=self.response['device']
            # FIXME: check for error

            self.accept_incoming_call()

        elif (action=='accept-done'):
            print "call accepted on device "+self.device
            self.play_sample('sitrtoot.al')
            time.sleep(5)
            self.send_command(self.device+' disconnect')
            
        elif (action=='disconnect-done'):
            print "disconnect done, quitting"
            sys.exit(1)
        else:
            print "(not processed)"

    def accept_incoming_call(self):
        self.send_command(self.device+' accept')

    def disconnect(self):
        self.send_command(self.device+' disconnect')

    def play_sample(self, filename):
        self.send_command(self.device+' add 2 1 play '+filename+' none')
        
    def event_loop(self):
        while 1:
            x.wait_for_response()
            x.act_on_response()

if __name__ == '__main__':

	# check commandline arguments
	optlist, args = getopt.getopt(sys.argv[1:], 'n', ['name='])

	hostname = None

	for opt in optlist:
		if opt[0] == '-n' or opt[0] == '--name':
			hostname = args[optlist.index(opt)]

	if hostname == None:
		hostname = socket.gethostname()

	x=sequencer_client(hostname, 2104)

	# 'sequence protocol...' lesen und ignorieren
	x.stdin.readline()

	# auf alles hören, was da anruft
	x.listen_to_any()

	# ab in die event loop
	x.event_loop()
