#!/usr/bin/env python

import os

_root = ''

def set_root(root):
    global _root
    _root = root

def get_root():
    global _root
    return _root

class Atom(object):
    def __init__(self):
        """notify should be 'none', 'begin', 'end' or 'both'"""
        self.notify = 'none'

class PlayAtom(Atom):
    def __init__(self, filename, prefix = None):
        Atom.__init__(self)
        if prefix:
            self.filename = os.path.join(_root, prefix, filename)
        else:
            self.filename = os.path.join(_root, filename)

    def as_command(self):
        return 'play %s %s' % (self.filename, self.notify)

class RecordAtom(Atom):
    def __init__(self, filename, maxtime, maxsilence = 2.0, prefix = None):
        Atom.__init__(self)
        if prefix:
            self.filename = os.path.join(_root, prefix, filename)
        else:
            self.filename = os.path.join(_root, filename)
        self.maxtime = maxtime
        self.maxsilence = maxsilence

    def as_command(self):
        return 'rec %s %d %d %s' % (self.filename, self.maxtime * 1000,
                                    self.maxsilence * 1000, self.notify)

class BeepAtom(Atom):
    def __init__(self, count):
        Atom.__init__(self)
        self.count = count

    def as_command(self):
        return 'beep %d %s' % (self.count, self.notify)

class DTMFAtom(Atom):
    def __init__(self, digits):
        Atom.__init__(self)
        self.digits = digits

    def as_command(self):
        return 'dtmf %s %s' % (self.digits, self.notify)

class UDPAtom(Atom):
    def __init__(self, port):
        Atom.__init__(self)
        self.port = port

    def as_command(self):
        return 'udp %d %s' % (self.port, self.notify)
                          
class ConferenceAtom(Atom):
    def __init__(self, conference, mode):
        Atom.__init__(self)
        self.conference = conference
        if type(mode) != type('') or not mode in ['listen', 'speak', 'duplex']:
            raise TypeError("mode must be one of 'listen', 'speak' or " \
                            "'duplex'")
        self.mode = mode

    def as_command(self):
        return 'conf %s %s' % (self.conference, self.notify)

class Molecule(list):
    def __init__(self, policy, *atoms):
        self.policy = policy
        for a in atoms:
            self.append(a)

    def __setitem__(self, key, item):
        if not isinstance(item, Atom):
            raise TypeError('%s must be a subclass of Atom' % repr(item))
        super(Molecule, self).__setitem__(self, key, item)

    def as_command(self, channel = None):
        """Generate the molecule as a sequencer command.
        The policy's channel can be overwritten"""
        if channel:
            s = '%d %d %d' % (channel, self.policy.mode, self.policy.priority)
        else:
            s = '%d %d %d' % (self.policy.channel, self.policy.mode,
                              self.policy.priority)
                        
        for i in self:
            s = s + ' ' + i.as_command()

        return s

class Play(Molecule):
    def __init__(self, policy, *args, **kwargs):
        self.policy = policy
        prefix = kwargs.get('prefix', None)
        for a in args:
            self.append(PlayAtom(a, prefix))

class Beep(Molecule):
    def __init__(self, policy, count):
        self.policy = policy
        self.append(BeepAtom(count))

class Record(Molecule):
    def __init__(self, policy, filename, maxtime, maxsilence = 2.0,
                 prefix = None):
        self.policy = policy
        self.append(RecordAtom(filename, maxtime, maxsilence, prefix))

class RecordBeep(Molecule):
    def __init__(self, policy, filename, maxtime, maxsilence = 2.0,
                 prefix = None):
        self.policy = policy
        self.append(BeepAtom(1))
        self.append(RecordAtom(filename, maxtime, maxsilence, prefix))

class Conference(Molecule):
    def __init__(self, policy, conference, mode):
        self.policy = policy
        self.append(ConferenceAtom(conference, mode))

class UDP(Molecule):
    def __init__(self, policy, port):
        self.policy = policy
        self.append(UDPAtom(port))

# mode definitions
mode_discard = 0x01
mode_pause = 0x02
mode_mute = 0x04
mode_restart = 0x08
# Don't interrupt molecule for molecules with higher priority. 
# mode_dtmf_stop is unaffected 
mode_dont_interrupt = 0x10
mode_loop = 0x20
mode_dtmf_stop = 0x40

# priority definitions
pr_background = 0
pr_normal = 1
pr_mail = 2
pr_transition = 3
pr_urgent = 4

class Policy(object):
    def __init__(self, channel, priority, mode):
        self.channel = channel
        self.priority = priority
        self.mode = mode

# define application specific policies
P_Background = Policy(0, pr_background, mode_mute|mode_loop)
P_Normal = Policy(0, pr_normal, mode_mute)
P_Discard = Policy(0, pr_normal, mode_discard|mode_dtmf_stop)
P_Mail = Policy(0, pr_mail, mode_discard|mode_dtmf_stop)
P_Transition = Policy(0, pr_transition, mode_dont_interrupt|mode_dtmf_stop)
P_Urgent = Policy(0, pr_urgent, mode_dont_interrupt)

if __name__ == '__main__':
    import unittest

    class MoleculeTest(unittest.TestCase):

        def testAtoms(self):
            'Construction of a Molecule with all Atom subclasses'
            m = Molecule(Normal,
                         BeepAtom(1),
                         RecordAtom('foo.wav', 10000, 1000),
                         PlayAtom('foo.wav'),
                         DTMFAtom('1234'),
                         ConferenceAtom('conf[0]', 'duplex'))

            self.assertEqual(m.as_command(),
                             '0 4 1 beep 1 none '\
                             'rec foo.wav 10000 1000 none '\
                             'play foo.wav none ' \
                             'dtmf 1234 none ' \
                             'conf conf[0] none') 

        def testInvalidAtom(self):
            'Adding an invalid atom raises a TypeError'
            m = Molecule(Normal)

            self.assertRaises(TypeError, lambda m: m.append(12)) 

        def testInvalidConferenceMode(self):
            'Invalid conference mode raises a TypeError'
            self.assertRaises(TypeError,
                              lambda x:
                              Molecule(Normal,
                                       ConferenceAtom('conf[0]', 'foo'))) 

    unittest.main()
