#!/usr/bin/env python

import sys
import logging
import sequencer
import sequencer
from location import *
from molecule import *
from caller import Caller

log = logging.getLogger('world')

# set the filesystem root for all molecules
set_root('c:\\Users\\lars\\Sounds\\8k')

class A_Hackcenter(Room):
    background = PlayMolecule(P_Background, 'hepepe_-_Bingo_Baby_Babe.wav')

class A_Haecksen(Room):
    background = PlayMolecule(P_Background, 'party.wav')

class B_NW(Room):
    background = PlayMolecule(P_Background, 'nimmo_-_la_salle_verte.wav')

class B_NE(Room):
    background = PlayMolecule(P_Background, 'lethal_laurence_-_Sliding_Cavern_(Deep_Green_Mix).wav')

class B_SW(Room):
    background = PlayMolecule(P_Background, 'nimmo_-_pedacito.wav')

class B_SE(Room):
    background = PlayMolecule(P_Background,
                              '369_TwistedLemon_ComeAndGo-metro-BCN_lars.wav')

class B_Lounge(Room):
    background = PlayMolecule(P_Background,
                              'melquiades_-_Brahms_Intermezzo_116.4.wav')

class B_Saal2(Room):
    background = PlayMolecule(P_Background,
                              'ith_brahms-10-4.wav')

class B_Saal3(Room):
    background = PlayMolecule(P_Background, 'asteria_-_Quant_la_doulce_jouvencelle_medieval_chanson.wav')

class C_NW(Room):
    background = PlayMolecule(P_Background, 'cdk_-_one_moment_(cdk_play_it_cool_mix).wav')

class C_Office(Room):
    background = PlayMolecule(P_Background, 'lethal_laurence_-_Sliding_Cavern_(Deep_Green_Mix).wav')

class C_SW(Room):
    background = PlayMolecule(P_Background, 'cdk_-_the_haunting_-_(cdk_analog_ambience_mix).wav')

class C_SE(Room):
    background = PlayMolecule(P_Background,
                              'marcoraaphorst_-_Blowing_Snow.wav')

class C_Saal1(Room):
    background = PlayMolecule(P_Background, 'ith_don_schumann-arabesque.wav')

class World(object):
    def __init__(self):
        self.callers = []

    def enter(self, caller):
        log.debug('%s entered', caller)
        caller.enqueue(PlayMolecule(P_Transition, '4011_suonho_sweetchoff_iLLCommunications_suonho.wav'))
        self.entry.enter(caller)

    def leave(self, caller):
        log.debug('%s left', caller)

    def start(self, seq):
        for t in seq.trunks:
            for i in range(t.lines):
                self.callers.append(Caller(seq, self, t.name))

        self.a_hackcenter = A_Hackcenter()
        self.a_haecksen = A_Haecksen()
        
        self.b_se = B_SE()
        self.b_nw = B_NW()
        self.b_ne = B_NE()
        self.b_sw = B_SW()
        self.b_lounge = B_Lounge()
        self.b_saal2 = B_Saal2()
        self.b_saal3 = B_Saal3()

        self.c_se = C_SE()
        self.c_nw = C_NW()
        self.c_ne = C_Office()
        self.c_sw = C_SW()
        self.c_saal1 = C_Saal1()
        
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
        self.callers = []

    def run(self):
        sequencer.run(start = self.start, stop = self.stop)

if __name__ == '__main__':
    world = World()
    world.run()
