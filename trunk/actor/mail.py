#!/usr/bin/env python

import time
import datetime
import logging
import os
import time
from molecule import *

log = logging.getLogger('mail')

def mailbox_path(id):
    return os.path.join(get_root(), 'user', id)

def mailbox_name(id):
    return os.path.join(get_root(), 'user', id, 'vbox')

class Message(object):
    '''A voicemail message'''
    
    def __init__(self, id, sent, sender, read = None):
        self.id = id
        self.sender = sender
        self.sent = sent
        self.read = read

    @classmethod
    def read(cls, f, id):
        """Read a message line from the vbox file. Pickling would have been
        easier, but I like human readable files.

        Each message is a single line with:
        sent(seconds since the epoch), sender, read(seconds since the epoch)
        delimited by a single space.

        sender and read may be 'None'.
        """

        l = f.readline()
        if not l:
            return None

        # strip l before splitting
        t_sent, sender, t_read = l.strip().split(' ')
        
        if t_read == 'None':
            t_read = None
        else:
            t_read = int(t_read);
        if sender == 'None':
            sender = None
        t_sent = int(t_sent)

        m = cls.__new__(cls)
        m.__init__(id, t_sent, sender, t_read)
        return m

    def write(self, f):
        '''Write a message line to the vbox file.'''
        f.write('%d %s %s\n' % (self.sent, self.sender, self.read))
        
    def mark_as_read(self):
        self.read = time.time()

    def as_play_atom(self):
        return PlayAtom('%d_%s.wav' % (self.sent, self.sender),
                        prefix=mailbox_path(self.id))

    def as_record_atom(self):
        return RecordAtom('%d_%s.wav' % (self.sent, self.sender), 60,
                        prefix=mailbox_path(self.id))

    def date_as_atom(msg):
        d = datetime.date(time.time()) - datetime.date(self.sent)

        s = ''
        if d.days == 0:
            s = 'heute'
        elif d.days == 1:
            s = 'gestern'
        elif d.days == 2:
            s = 'vorgestern'
        elif d.days == 3:
            s = 'vordreitagen'
        else:
            s = 'vornerhalbenewigkeit'

        return PlayAtom('%s.wav', prefix='lars')

    def __cmp__(self, other):
        '''Sort by sent time.'''
        return self.sent.__cmp__(other.sent)

class Mailbox(object):
    '''A mailbox'''

    def __init__(self, id):
        self.id = id
        self.icurrent = None
        self.messages = []

    def read(self):
        '''Read a Mailbox'''
        name = mailbox_name(self.id)
        if not os.path.exists(name):
            return
        
        f = open(name, 'r')

        self.messages = []
        
        m = Message.read(f, self.id)
        while m:
            self.messages.append(m)
            m = Message.read(f, self.id)
            
        self.messages.sort()
        f.close()

    def write(self):
        '''Write a mailbox to disk'''

        # Create directories if needed
        p = mailbox_path(self.id)
        if not os.path.exists(p):
            os.makedirs(p)

        f = open(mailbox_name(self.id), 'w')
        for m in self.messages:
            m.write(f)
        f.close()

    def add(self, message):
        '''Add a message to the mailbox'''
        self.messages.append(message)
        self.messages.sort()

        # write to disk immediately
        self.write()

        return message

    def current(self):
        '''Return the current message if there is any.'''
        if not self.messages:
            return None

        if self.icurrent is None:
            self.icurrent = 0

        return self.messages[self.icurrent]

    def next(self):
        '''Increment to the next message and return it.
        If it was the last message, return None and reset the message counter
        to zero.'''
        if not self.messages:
            return None

        if self.icurrent is None:
            self.icurrent = 0
        else:
            self.icurrent = self.icurrent + 1
        if self.icurrent >= len(self.messages):
            self.icurrent = None
            return None
        
        return self.messages[self.icurrent]
        
class MailDialog(object):

    def __init__(self, rcpt):
        self.rcpt = rcpt
        self.caller = None
        self.tid = None
        self.message = None

    def start(self, caller):
        self.caller = caller

        t = int(time.time())
        self.message = Message(self.rcpt, t, caller.details.calling)

        log.debug('%s recording mail for %s: %d', caller, self.rcpt, t)

        # check if path exists and create if necessary
        path = mailbox_path(self.rcpt)
        if not os.path.exists(path):
            log.debug('%s creating dirs %s', caller, path)
            os.makedirs(path)

        m = Molecule(P_Mail)
        m.append(PlayAtom('postfuer.wav', prefix='lars'))
        # No names yet, just numerical ids
        for c in self.rcpt:
            m.append(PlayAtom('%s.wav' % c, prefix='lars'))
    
        m.append(BeepAtom(1))
        m.append(self.message.as_record_atom())
        self.tid = caller.enqueue(m)

        return self.tid

    def MLCA(self, caller, event, user_data):
        is_online = False
        tid = event['tid']
        if tid == self.tid:
            self.tid = None
            # if the caller is online, deliver immediately
            for c in self.caller.world.callers.itervalues():
                if c.details.calling == self.rcpt:
                    is_online = True
                    log.debug('%s delivering immediately %s', c, self.name)
                    m = Molecule(P_Mail)
                    m.append(PlayAtom('duhastpost.wav', prefix='lars'))
                    m.append(self.message.as_play_atom())
                    c.enqueue(m)
                    if c.mailbox:
                        self.message.mark_as_read()
                        c.mailbox.add(self.message)

            # Save the message if the caller wasn't online
            if not is_online:
                mb = Mailbox(self.rcpt)
                mb.add(self.message)

            # we're done
            return True

        return False

    def DTMF(self, caller, dtmf):
        return False

if __name__ == '__main__':
    import unittest

    if os.name == 'nt':
        set_root('c:\\temp')
    else:
        set_root('/tmp')

    class MailboxTest(unittest.TestCase):

        def testARead(self):
            'Test reading a mailbox file'

            p = mailbox_path('42')
            if not os.path.exists(p):
                os.makedirs(p)

            f = open(mailbox_name('42'), 'w')
            t = int(time.time())
            f.write('%d %s %s\n' % ( t, '23', None))
            f.write('%d %s %s\n' % (t + 1, '24', None))
            f.close()

            mb = Mailbox('42')
            mb.read()

            self.failUnless(mb.messages[0].sender == '23')
            self.failUnless(mb.messages[0].sent == t)
            self.failUnless(mb.messages[0].read == None)
            self.failUnless(mb.messages[1].sender == '24')
            self.failUnless(mb.messages[1].sent == t + 1)
            self.failUnless(mb.messages[1].read == None)

        def testBAppend(self):
            'Test appending to a mailbox file'

            t = int(time.time())

            mb = Mailbox('42')
            mb.add(Message('42', t, '23'))

            self.failUnless(mb.messages[0].sender == '23')
            self.failUnless(mb.messages[0].sent == t)
            self.failUnless(mb.messages[0].read == None)

        def testCWrite(self):
            'Test reading a self-written mailbox file'

            t = int(time.time())

            mb = Mailbox('42')
            mb.add(Message('42', t, '23'))
            mb.add(Message('42', t+1, '24'))

            mb.read()

            self.failUnless(mb.messages[0].sender == '23')
            self.failUnless(mb.messages[0].sent == t)
            self.failUnless(mb.messages[0].read == None)
            self.failUnless(mb.messages[1].sender == '24')
            self.failUnless(mb.messages[1].sent == t + 1)
            self.failUnless(mb.messages[1].read == None)

        def testEEnumerate(self):
            'Test current() in a one-message mailbox'

            t = int(time.time())

            mb = Mailbox('42')
            mb.add(Message('42', t, '23'))

            m = mb.current()

            self.failUnless(m.sender == '23')
            self.failUnless(m.sent == t)
            self.failUnless(m.read == None)
             
        def testFNext(self):
            'Test next() in a one-message mailbox'

            t = int(time.time())

            mb = Mailbox('42')
            mb.add(Message('42', t, '23'))

            m = mb.current()
            m = mb.next()

            self.failUnless(m is None)

        def testGNextWrap(self):
            'Test wraparound of next() in a one-message mailbox'

            t = int(time.time())

            mb = Mailbox('42')
            mb.add(Message('42', t, '23'))

            m = mb.current()
            m = mb.next()

            self.failUnless(m is None)

            m = mb.current()

            self.failUnless(m.sender == '23')
            self.failUnless(m.sent == t)
            self.failUnless(m.read == None)

        def testHMessageMolecule(self):
            'Test that the message can output itself as a play atom'

            t = int(time.time())

            m = Message('42', t, '23')
            s = m.as_play_atom().as_command()

            n = os.path.join(mailbox_path('42'), '%d_23.wav' % t)
            
            self.failUnless(s == ('play %s none' % n))

    unittest.main()
    
