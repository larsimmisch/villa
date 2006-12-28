#!/usr/bin/env python

import sys
import os
import logging
import sequencer
import sequencer
from location import *
from molecule import *
from mail import *
from caller import Caller, DBData
from twisted.enterprise import adbapi

log = logging.getLogger('world')

# set the filesystem root for all molecules
set_root('d:\\work\\sources\\voice3\\trunk\\actor\\congress')

class A_Hackcenter(Room):
    prefix = 'a_hackcenter'
    background = Play(P_Background, 'hepepe_-_Bingo_Baby_Babe.wav',
                      prefix=prefix)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

class A_Haecksen(ConferenceRoom):
    prefix = 'a_haecksen'
    background = Play(P_Background, 'party.wav',
                      prefix=prefix)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

class A_Saal4(ConferenceRoom):
    prefix = 'a_saal4'
    background = UDP(P_Background, 10004)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

class B_NW(Room):
    prefix = 'b_nw'
    background = Play(P_Background, 'nimmo_-_la_salle_verte.wav',
                      prefix=prefix)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

class B_NE(Room):
    prefix = 'b_ne'
    background = Play(P_Background, 'lethal_laurence_-_Sliding_Cavern_(Deep_Green_Mix).wav',
                      prefix=prefix)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

class B_SW(Room):
    prefix = 'b_sw'
    background = Play(P_Background, 'nimmo_-_pedacito.wav',
                      prefix=prefix)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

class B_SE(Room):
    prefix = 'b_se'
    background = Play(P_Background,
                      'weirdpolymer_-_Still_People.wav',
                      prefix=prefix)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

    def move_invalid(self, caller): 
        caller.enqueue(Play(P_Normal, 'wand.wav', prefix='bef'))

class B_Lounge(ConferenceRoom):
    prefix = 'b_lounge'
    background = Play(P_Background, 'ith_chopin-55-1.wav',
                      prefix=prefix)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

class B_Saal2(ConferenceRoom):
    prefix = 'b_saal2'
    background = UDP(P_Background, 10002)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

class B_Saal3(ConferenceRoom):
    prefix = 'b_saal3'
    background = UDP(P_Background, 10003)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

class C_NW(Room):
    prefix = 'c_nw'
    background = Play(P_Background, 'cdk_-_one_moment_(cdk_play_it_cool_mix).wav',
                      prefix=prefix)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

class C_Office(Room):
    prefix = 'c_office'
    background = Play(P_Background, 'asteria_-_Quant_la_doulce_jouvencelle_medieval_chanson.wav',
                      prefix=prefix)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

    def enter(self, caller):
        super(C_Office, self).enter(caller)
        if caller.mailbox:
            caller.mailbox.reset()

    def browse(self, caller, dtmf):
        '''Browse through the messages'''
        
        if caller.mailbox is None or not caller.mailbox.messages:
            caller.enqueue(Play(P_Discard,
                                'duhastkeinepost.wav', prefix='lars'))
            return True

        if dtmf == '1':
            msg = caller.mailbox.previous()
        elif dtmf == '2':
            msg = caller.mailbox.current()
        elif dtmf == '3':
            msg = caller.mailbox.next()

        if msg:
            log.debug('browsing - message: %s', msg.filename())
            caller.mailbox.play_current(caller)
        else:
            # wraparound, most likely. Beep for now.
            self.generic_invalid(caller)

        return True

    def DTMF(self, caller, dtmf):
        if super(C_Office, self).DTMF(caller, dtmf):
            return True

        if dtmf in ['1', '2', '3']:
            return self.browse(caller, dtmf)

        if dtmf == '4':
            # reply
            inmsg = caller.mailbox.current()
            if inmsg is None or inmsg.from_ is None:
                self.generic_invalid(caller)
                return True
            caller.startDialog(MailDialog(inmsg.from_))
        elif dtmf == '9':
            if caller.mailbox.delete_current(caller):
                # Play crumpling paper sound
                caller.enqueue(Play(P_Discard, 'crinkle.wav', prefix='lars'))
            else:
                self.generic_invalid(caller)
                
class C_SW(Room):
    prefix = 'c_sw'
    background = Play(P_Background, 'cdk_-_the_haunting_-_(cdk_analog_ambience_mix).wav',
                      prefix=prefix)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

class C_SE(Room):
    prefix = 'c_se'
    background = Play(P_Background,
                      'marcoraaphorst_-_Blowing_Snow.wav',
                      prefix=prefix)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

class C_Saal1(ConferenceRoom):
    prefix = 'c_saal1'
    background = UDP(P_Background, 10001)
    orientation = Play(P_Discard, 'orientation.wav', prefix=prefix)

class DBEntryDialog:
    max_retries = 3

    def __init__(self, caller, world):
        self.caller = caller
        self.world = world
        self.buffer = ''
        self.retries = 0

        if self.caller.details.calling:
            q = world.db.runQuery('SELECT * FROM user WHERE cli = %s;' %
                                  self.caller.details.calling)
            
            q.addCallback(self.db_cli)
            q.addErrback(self.db_error)
        
    def db_cli(self, result):
        if result:
            log.debug('%s db_cli result: %s', self.caller, result[0])
            self.caller.db = DBData(*result[0])
            self.startPin()
        else:
            log.debug('%s db_cli empty result', self.caller)
            self.startId()

    def db_id(self, result):
        if result:
            log.debug('%s db_id result: %s', self.caller, result[0])
            self.caller.db = DBData(*result[0])
            self.state = 'pin'
            self.caller.enqueue(Play(P_Normal, 'prima.wav', 'pin.wav',
                                     prefix='lars'))
        else:
            log.debug('%s db_id empty result', self.caller)
            self.invalid()

    def db_error(self, result):
        log.debug('%s db error: %s', self.caller, result)
        self.caller.disconnect()

    def startId(self):
        self.state = 'id'
        self.caller.enqueue(Play(P_Normal, 'id.wav', prefix='lars'))

    def startPin(self):
        self.state = 'pin'
        self.caller.enqueue(Play(P_Normal, 'pin.wav', prefix='lars'))

    def invalid(self):
        self.retries = self.retries + 1
        if self.retries > self.max_retries:
            self.state = 'rejected'
            self.caller.enqueue(Play(P_Normal, '4003_suonho_accessdenied_iLLCommunications_suonho.wav'))
        else:
            if self.state == 'id':
                self.caller.enqueue(Play(P_Normal,'id_wrong.wav',
                                         prefix='lars'))
            else:
                self.caller.enqueue(Play(P_Normal, 'pin_wrong.wav',
                                         prefix='lars'))
            self.startId()

    def hash(self, caller):
        log.debug('%s entry state %s', self.caller, self.state)
        if self.state == 'id':
            if self.buffer:
                q = self.world.db.runQuery(
                    'SELECT * FROM user WHERE id = %s;' % self.buffer)
                
                q.addCallback(self.db_id)
                q.addErrback(self.db_error)
                self.state = 'waiting'
                self.buffer = ''
            else:
                self.invalid()
        else:
            if self.buffer == self.caller.db.pin:
                self.world.entry.enter(caller)
                return True

            self.invalid()
            
    def DTMF(self, caller, dtmf):
        # block DTMF while DB lookup pending
        if self.state == 'waiting':
            self.caller.enqueue(BeepMolecule(P_Normal, 2))
            return
        
        if dtmf == '#':
            return self.hash(caller)
        else:
            self.buffer = self.buffer + dtmf

class EntryDialog:
    '''Ask the caller for a name'''

    def __init__(self, caller, world):
        self.caller = caller
        self.world = world
        self.cli = self.caller.details.calling

        self.tid = caller.enqueue(
            Molecule(P_Transition,
                     PlayAtom('wieheisstdu.wav', prefix='lars'),
                     RecordBeep(P_Transition, 'name.wav', 10.0,
                                prefix=os.path.join('user', cli))))

    def MLCA(self, caller, event, user_data):
        tid = event['tid']
        status = event['status']
        if self.tid_talk == tid:
            data.tid_talk = None
            log.info('talk %s (%d): status %s',  user_data, tid, status)
            for c in self.callers.itervalues():
                if c != caller:
                    c.enqueue(Play(P_Discard, user_data,
                              prefix='talk'))

class World(object):
    def __init__(self):
        self.devices = []
        self.callers = {}

    def start(self, seq):
##         self.db = adbapi.ConnectionPool('MySQLdb', host='localhost',
##                                         user='actor', passwd='HerrMeister',
##                                         db='actor')

        for t in seq.trunks:
            for i in range(t.lines):
                c = Caller(seq, self, t.name)
                self.devices.append(c)
                c.send(c, 'LSTN %s %s' % (t.name, 'any'))

        self.a_hackcenter = A_Hackcenter()
        self.a_haecksen = A_Haecksen(seq)
        
        self.b_se = B_SE()
        self.b_nw = B_NW()
        self.b_ne = B_NE()
        self.b_sw = B_SW()
        self.b_lounge = B_Lounge(seq)
        self.b_saal2 = B_Saal2(seq)
        self.b_saal3 = B_Saal3(seq)

        self.c_se = C_SE()
        self.c_nw = C_NW()
        self.c_ne = C_Office()
        self.c_sw = C_SW()
        self.c_saal1 = C_Saal1(seq)
        
        self.entry = self.b_se

        # Floor A
        connect(self.a_hackcenter, self.a_haecksen, Door(), 'south')

        # Floor B
        connect(self.b_se, self.b_sw, Door(), 'west')
        connect(self.b_se, self.b_ne, Door(), 'north')
        connect(self.b_sw, self.b_nw, Door(), 'north')
        connect(self.b_ne, self.b_nw, Door(), 'west')

        connect(self.b_se, self.b_lounge, Door(), 'northwest')
        connect(self.b_sw, self.b_lounge, Door(), 'northeast')
        connect(self.b_nw, self.b_lounge, Door(), 'southeast')
        connect(self.b_ne, self.b_lounge, Door(), 'southwest')

        connect(self.b_sw, self.b_saal3, Door(), 'west')
        connect(self.b_nw, self.b_saal2, Door(), 'west')

        # Floor C
        connect(self.c_se, self.c_sw, Door(), 'west')
        connect(self.c_se, self.c_ne, Door(), 'north')
        connect(self.c_sw, self.c_nw, Door(), 'north')
        connect(self.c_ne, self.c_nw, Door(), 'west')

        connect(self.c_se, self.c_saal1, Door(), 'northwest')
        connect(self.c_sw, self.c_saal1, Door(), 'northeast')
        connect(self.c_nw, self.c_saal1, Door(), 'southeast')
        connect(self.c_ne, self.c_saal1, Door(), 'southwest')

        # Stairs
        connect(self.b_ne, self.a_hackcenter, Stairs(),
                'northeast', 'northeast')
        connect(self.b_se, self.c_se, Stairs(), 'southeast', 'southeast')
        connect(self.b_sw, self.c_sw, Stairs(), 'southwest', 'southwest')
        connect(self.b_nw, self.c_nw, Stairs(), 'northwest', 'northwest')

    def stop(self):
        self.devices = []
        self.callers = {}

    def enter(self, caller):
        log.debug('%s entered', caller)

        self.callers[caller.device] = caller

        caller.enqueue(Play(P_Transition, '4011_suonho_sweetchoff_iLLCommunications_suonho.wav'))

        # caller.startDialog(EntryDialog(caller, self))

        self.entry.enter(caller)

    def leave(self, caller):
        del self.callers[caller.device]
        
        log.debug('%s left', caller)

    def run(self):
        sequencer.run(start = self.start, stop = self.stop)

if __name__ == '__main__':
    world = World()
    world.run()
