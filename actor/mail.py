#!/usr/bin/env python

import time
import datetime
import logging
import os
import time
from molecule import *

log = logging.getLogger('mail')

def mailbox_path(id, create = False):
    p = os.path.join(get_root(), 'user', id)

    if create and not os.path.exists(p):
        os.makedirs(p)

    return p

def mailbox_name(id):
    return os.path.join(get_root(), 'user', id, 'vbox')

class Message(object):
    '''A voicemail message'''
    
    def __init__(self, to, from_, sent = None, read = None):
        self.to = to
        self.from_ = from_
        if sent is None:
            self.sent = int(time.time())
        else:
            self.sent = sent
        self.read = read

    @classmethod
    def read(cls, f, to):
        """Read a message line from the vbox file. Pickling would have been
        easier, but I like human readable files.

        Each message is a single line with:
        sent(seconds since the epoch), from, read(seconds since the epoch)
        delimited by a single space.

        from and read may be 'None'.
        """

        l = f.readline()
        if not l:
            return None

        # strip l before splitting
        t_sent, from_, t_read = l.strip().split(' ')
        
        if t_read == 'None':
            t_read = None
        else:
            t_read = int(t_read);
        t_sent = int(t_sent)

        from_.strip()
        if from_ == 'None':
            from_ = None

        m = cls.__new__(cls)
        m.__init__(to, from_, t_sent, t_read)
        return m

    def write(self, f):
        '''Write a message line to the vbox file.'''
        f.write('%d %s %s\n' % (self.sent, self.from_, self.read))

    def record(self, caller):
        '''Record a message. Return the tid.
        Use MailDialog.start() unless you know what you are doing.'''
        
        log.debug('%s recording mail for %s: %d', caller, self.to, self.sent)

        # check if path exists and create if necessary
        path = mailbox_path(self.to, True)

        m = Molecule(P_Mail)
        m.append(PlayAtom('postfuer.wav', prefix='lars'))
        # No names yet, just numerical ids
        for c in self.to:
            m.append(PlayAtom('%s.wav' % c, prefix='lars'))
    
        m.append(BeepAtom(1))
        m.append(self.as_record_atom())
        
        return caller.enqueue(m)

    def play(self, caller, announce = False):
        '''Play a message and return the tid.
        Use Mailbox.play_current() unless you know what you are doing.'''

        log.debug('%s delivering mail from %s: %d', caller, self.from_,
                  self.sent)

        self.mark_as_read()

        m = Molecule(P_Mail)
        if announce:
            m.append(PlayAtom('duhastpost.wav', prefix='lars'))
            
        m.append(self.as_play_atom())
        m.append(PlayAtom('von.wav', prefix='lars'))
        if self.from_:
            for c in self.from_:
                m.append(PlayAtom('%s.wav' % c, prefix='lars'))
            
        m.append(self.date_as_atom())

        return caller.enqueue(m)

    def mark_as_read(self):
        self.read = int(time.time())

    def pathname(self):
        '''Return the path to the message file.'''
        return os.path.join(mailbox_path(self.to), self.filename())
    
    def filename(self):
        '''Return the filename (not the full path)'''
        return '%d_%s.wav' % (self.sent, self.from_)

    def file_exists(self):
        return os.path.exists(self.pathname())

    def as_play_atom(self):    
        return PlayAtom(self.pathname())

    def as_record_atom(self):
        return RecordAtom(self.pathname(), 60)

    def date_as_atom(self):
        d = datetime.date.fromtimestamp(time.time()) - \
            datetime.date.fromtimestamp(self.sent)

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

        return PlayAtom('%s.wav' % s, prefix='lars')

    def __cmp__(self, other):
        '''Sort by sent time.'''
        return other.sent - self.sent

class Mailbox(object):
    '''A mailbox'''

    def __init__(self, to):
        self.to = to
        self.icurrent = None
        self.messages = []

    def read(self):
        '''Read a Mailbox'''
        name = mailbox_name(self.to)
        if not os.path.exists(name):
            return
        
        f = open(name, 'r')

        self.messages = []
        
        m = Message.read(f, self.to)
        while m:
            self.messages.append(m)
            m = Message.read(f, self.to)
            
        self.messages.sort()
        f.close()

    def write(self):
        '''Write a mailbox to disk'''

        # Create directories if needed
        mailbox_path(self.to, True)

        f = open(mailbox_name(self.to), 'w')
        for m in self.messages:
            m.write(f)
        f.close()

    def add(self, message):
        '''Add a message to the mailbox.'''
        self.messages.append(message)
        self.messages.sort()

        # write to disk immediately
        self.write()

        return message

    def play_current(self, caller):
        '''Play the current message. Delete the message if it does not exist.
        Return tid of the playing of the first existing message.'''

        if not self.messages or self.icurrent is None:
            log.warning('%s no messages or no current message', caller)
            return None
        
        m = self.messages[self.icurrent]
        while m and not m.file_exists():
            log.warning('%s message file %s does not exit', caller,
                        m.filename())
            del self.messages[self.icurrent]
            m = self.current()

        if not m:
            log.warning('%s no message left', caller)
            return None

        tid = m.play(caller)

        # message was marked as read. Flush mailbox.
        self.write()

        return tid

    def delete_current(self, caller):
        '''Delete the current message.

        There is a known race condition: If a playing message is deleted,
        the file may be in use while we try to remove it. This will leak files,
        but we ignore it for now.
        '''

        if self.icurrent is None:
            return None

        m = self.messages[self.icurrent]        
        log.debug('Deleting message %d %s', self.icurrent, m.filename())

        del self.messages[self.icurrent]

        # write to disk immediately
        self.write()
        
        try:
            os.remove(m.pathname())
        except:
            log.error('Error removing message %s', m.filename(), exc_info=1)

        return m

    def reset(self):
        self.icurrent = None
    
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
        
    def previous(self):
        '''Decrement to the next message and return it.
        If it was the first message, return None.'''
        if not self.messages:
            return None

        if self.icurrent is None:
            self.icurrent = len(self.messages)-1
        else:
            self.icurrent = self.icurrent -1
        if self.icurrent ==  0:
            self.icurrent = None
            return None
        
        return self.messages[self.icurrent]
        
class MailDialog(object):

    def __init__(self, to):
        self.to = to
        self.caller = None
        self.tid = None
        self.message = None

    def start(self, caller):
        self.caller = caller
        self.message = Message(self.to, caller.details.calling)
        self.tid = self.message.record(caller)
        
        return self.tid

    def MLCA(self, caller, event, user_data):
        is_online = False
        tid = event['tid']
        if tid == self.tid:
            self.tid = None
            # if the caller is online, deliver immediately
            for c in self.caller.world.callers.itervalues():
                if c.details.calling == self.to:
                    is_online = True
                    if c.mailbox is None:
                        c.mailbox = Mailbox(self.to)

                    c.mailbox.add(self.message)
                    # play with announcement
                    self.message.play(c, True)
                    break

            # Save the message if the caller wasn't online
            if not is_online:
                mb = Mailbox(self.to)
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

            p = mailbox_path('42', True)

            f = open(mailbox_name('42'), 'w')
            t = int(time.time())
            f.write('%d %s %s\n' % ( t, '23', None))
            f.write('%d %s %s\n' % (t + 1, '24', None))
            f.close()

            mb = Mailbox('42')
            mb.read()

            self.failUnless(mb.messages[0].from_ == '23')
            self.failUnless(mb.messages[0].sent == t)
            self.failUnless(mb.messages[0].read == None)
            self.failUnless(mb.messages[1].from_ == '24')
            self.failUnless(mb.messages[1].sent == t + 1)
            self.failUnless(mb.messages[1].read == None)

        def testBAppend(self):
            'Test appending to a mailbox file'

            t = int(time.time())

            mb = Mailbox('42')
            mb.add(Message('42', '23', t))

            self.failUnless(mb.messages[0].from_ == '23')
            self.failUnless(mb.messages[0].sent == t)
            self.failUnless(mb.messages[0].read == None)

        def testCWrite(self):
            'Test reading a self-written mailbox file'

            t = int(time.time())

            mb = Mailbox('42')
            mb.add(Message('42', '23', t))
            mb.add(Message('42', '24', t+1))

            mb.read()

            self.failUnless(mb.messages[0].from_ == '23')
            self.failUnless(mb.messages[0].sent == t)
            self.failUnless(mb.messages[0].read == None)
            self.failUnless(mb.messages[1].from_ == '24')
            self.failUnless(mb.messages[1].sent == t + 1)
            self.failUnless(mb.messages[1].read == None)

        def testEEnumerate(self):
            'Test current() in a one-message mailbox'

            t = int(time.time())

            mb = Mailbox('42')
            mb.add(Message('42', '23', t))

            m = mb.current()

            self.failUnless(m.from_ == '23')
            self.failUnless(m.sent == t)
            self.failUnless(m.read == None)
             
        def testFNext(self):
            'Test next() in a one-message mailbox'

            t = int(time.time())

            mb = Mailbox('42')
            mb.add(Message('42', '23', t))

            m = mb.current()
            m = mb.next()

            self.failUnless(m is None)

        def testGNextWrap(self):
            'Test wraparound of next() in a one-message mailbox'

            t = int(time.time())

            mb = Mailbox('42')
            mb.add(Message('42', '23', t))

            m = mb.current()
            m = mb.next()

            self.failUnless(m is None)

            m = mb.current()

            self.failUnless(m.from_ == '23')
            self.failUnless(m.sent == t)
            self.failUnless(m.read == None)

        def testHMessageMolecule(self):
            'Test that the message can output itself as a play atom'

            m = Message('42', '23')
            s = m.as_play_atom().as_command()

            n = os.path.join(mailbox_path('42'), m.filename())
            
            self.failUnless(s == ('play %s none' % n))

        def testIMessageExists(self):
            'Test file_exists'

            m = Message('42', '23')
            f = open(os.path.join(mailbox_path('42'), m.filename()), 'w')
            f.close()
            
            self.failUnless(m.file_exists())

    unittest.main()
    
