#!/usr/bin/env python

import sys
import logging
import sequencer
import sequencer
from location import Location, Room, Transition, Door, connect
from molecule import *
from caller import Caller

log = logging.getLogger('world')

# set the filesystem root for all molecules
set_root('c:\\Users\\lars\\Sounds\\8k')

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

class World(object):
    def __init__(self):
        self.callers = []
        self.b_se = B_SE()
        self.b_nw = B_NW()
        self.b_ne = B_NE()
        self.b_sw = B_SW()
        self.b_lounge = B_Lounge()
        
        self.entry = self.b_se

        connect(self.b_se, self.b_sw, 'west', Door())
        connect(self.b_se, self.b_ne, 'north',Door())
        connect(self.b_sw, self.b_nw, 'north', Door())
        connect(self.b_ne, self.b_nw, 'west', Door())

        connect(self.b_se, self.b_lounge, 'northwest', Door())
        connect(self.b_sw, self.b_lounge, 'northeast', Door())
        connect(self.b_nw, self.b_lounge, 'southeast', Door())
        connect(self.b_ne, self.b_lounge, 'southwest', Door())

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

    def stop(self):
        self.callers = []

    def run(self):
        sequencer.run(start = self.start, stop = self.stop)

if __name__ == '__main__':
    world = World()
    world.run()
