#!/usr/bin/env python

import sys
import logging
import sequencer
import sequencer
from location import Location, Room
from molecule import *
from caller import Caller

log = logging.getLogger('world')

# set the filesystem root for all molecules
set_root('c:\\Users\\lars\\Sounds')

class EntryRoom(Room):
    
    background = PlayMolecule(Background, 'alternat-Jay_Berl-6607.8k.wav')
    door_in = PlayMolecule(Transition, 'Door_fu-Martin-1581.8k.wav')
    door_out = door_in

class World(object):
    def __init__(self):
        self.callers = []
        self.entry = EntryRoom()

    def enter(self, caller):
        log.debug('%s entered', caller)
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
