import sys
import copy
import logging
from sequencer import callLater
from molecule import *

log = logging.getLogger('location')

class CallerIterator(object):
    """A specialised iterator for caller lists.
    It is cyclic and supports deletion of items in the sequence while the
    sequence is iterated over."""
    
    def __init__(self, items):
        self.items = items
        self.pos = None
        self.invalid = False

    def next(self):
        if not len(self.items):
            return None
        
        if self.pos is None:
            self.pos = 0
        elif self.invalid:
            self.invalid = False
            self.pos = self.pos % (len(self.items) - 1)
        else:
            self.pos = (self.pos + 1) % len(self.items)

        return self.items[self.pos]

    def prev(self):
        if not len(self.items):
            return None

        if self.pos is None:
            self.pos = len(self.items) - 1
        else:
            self.pos = (self.pos - 1) % len(self.items)
            
        return self.items[self.pos]

    def invalidate(self, item):
        """Indicate that an item in the sequence we iterate over will be
        deleted soon. This method gives the iterator the chance to adjust
        internal position counters.
        
        Note that the item must still be in the sequence when this method
        is called - the item should be deleted afterwards."""
        i = self.items.index(item)
        
        if i < self.pos:
            self.pos = self.pos - 1
        elif i == self.pos:
            self.invalid = True

class LocationData(object):
    '''Per caller data for each location.'''
    def __init__(self):
        self.tid_talk = None
        self.tm_move = None
        self.tm_starhash = None
        self.bf_starhash = ''
        self.it_callers = None
        self.dialog = None

    def __del__(self):
        self.cancel('tm_move')
        self.cancel('tm_starhash')

    def cancel(self, tm):
        '''Cancel the timer called tm. Sets the attribute tm to None'''
        t = getattr(self, tm)
        setattr(self, tm, None)
        if t:
            t.cancel()

class Location(object):

    def __init__(self):
        self.callers = []

    def user_data(self):
        return LocationData()

    def enter(self, caller):
        '''Enter the location'''
        caller.location = self
        caller.user_data = self.user_data()

        log.debug('%s enter: %s', self.__class__.__name__, caller)
        
        self.callers.append(caller)

        if hasattr(self, 'background'):
            caller.enqueue(self.background)

    def leave(self, caller, gone = False):
        '''Leave the location. Adjust caller list iterators
        in all participants.'''

        for c in self.callers:
            it = c.user_data.it_callers
            if it:
                it.invalidate(caller)
                
        self.callers.remove(caller)

        caller.discard(P_Background, P_Normal)
    
        log.debug('%s left: %s', self.__class__.__name__, caller)

        d = caller.user_data
        caller.user_data = None
        del d
        caller.location = None

    def move(self, caller, transition):
        # transition sound
        print transition.m_trans.as_command()
        caller.enqueue(transition.m_trans)

        # play door out sound to other callers
        for c in self.callers:
            if c != caller:
                c.enqueue(transition.m_out)

        self.leave(caller)
        transition.dest.enter(caller)

        # play door in sound to other callers
        for c in transition.dest.callers:
            if c != caller:
                c.enqueue(transition.m_in)

    def generic_invalid(self, caller):
        caller.enqueue(BeepMolecule(P_Normal, 2))

    def move_invalid(self, caller): 
        self.generic_invalid(caller)

    def move_timed_out(self, caller):
        caller.user_data.tm_move = None
        self.generic_invalid(caller)

    def starhash(self, caller, key):
        pass

    def starhash_invalid(self, caller):
        self.generic_invalid(caller)

    def starhash_timed_out(self, caller):
        caller.user_data.tm_starhash = None
        self.generic_invalid(caller)
    
    def DTMF(self, caller, dtmf):
        data = caller.user_data
        if data.tm_move:
            dir = None
            if dtmf == '1':
                dir = 'northwest'
            elif dtmf == '2':
                dir = 'north'
            elif dtmf == '3':
                dir = 'northeast'
            elif dtmf == '4':
                dir = 'west'
            elif dtmf == '6':
                dir = 'east'
            elif dtmf == '7':
                dir = 'southwest'
            elif dtmf == '8':
                dir = 'south'
            elif dtmf == '9':
                dir = 'southeast'
            else:
                data.cancel('tm_move')
                self.move_invalid(caller)
            if dir:
                data.cancel('tm_move')
                trans = getattr(self, dir, None)
                if trans:
                    self.move(caller, trans)
                else:
                    self.move_invalid(caller)

            return True
        elif data.tm_starhash:
            if dtmf == '#':
                data.cancel('tm_starhash')
                self.starhash(caller, bf_starhash)
            elif dtmf == '*':
                data.cancel('tm_starhash')
                self.starhash_invalid(caller)
            else:
                data.cancel('tm_starhash')
                # inter digit timer for direct access
                data.tm_starhash = callLater(2.0, self.starhash_timed_out)
                data.bf_starhash = data.bf_starhash + dtmf

            return True

        if dtmf == '5':
            data.tm_move = callLater(2.0, self.move_timed_out, caller)
            return True
        elif dtmf == '6':
            # Todo: this is probably bullshit
            data.it_callers = CallerIterator(self.callers)
            c = data.it_callers.next()
            caller.play(PRIORITY_NORMAL, MODE_NORMAL, c.name_audio)
        elif dtmf == '*':
            data.tm_starhash = callLater(2.0, self.starhash_timed_out,
                                         caller)
            return True
        elif dtmf == '#':
            if hasattr(self, 'help'):
                caller.enqueue(self.help)
            return True
        
        return False

class Room(Location):
    def __init__(self):
        super(Room, self).__init__()

    def DTMF(self, caller, dtmf):
        if super(Room, self).DTMF(caller, dtmf):
            return True
        
        data = caller.user_data
        if data.tid_talk:
            caller.stop(data.tid_talk)
            data.tid_talk = None
        elif dtmf == '4':
            data.tid_talk = caller.enqueue(
                RecordBeepMolecule(P_Normal, str(caller) + '.wav', 10.0))

class Transition(object):
    def __init__(self, m_trans, m_in, m_out):
        self.m_trans = m_trans
        self.m_in = m_in
        self.m_out = m_out
        
class Door(Transition):
    def __init__(self):
        self.m_in = PlayMolecule(P_Transition,
                                 'RBH_Household_front_door_open.wav')

        self.m_out = PlayMolecule(P_Transition,
                                  'RBH_Household_front_door_close.wav')

        self.m_trans = PlayMolecule(P_Transition,
                                    'RBH_Household_front_door_open.wav',
                                    'RBH_Household_front_door_close.wav')

class Stairs(Transition):
    def __init__(self):
        self.m_in = PlayMolecule(P_Transition,
                                 'RBH_Household_front_door_open.wav')

        self.m_out = PlayMolecule(P_Transition,
                                  'RBH_Household_front_door_close.wav')

        self.m_trans = PlayMolecule(P_Transition, 'treppe.wav')

_mirror = { 'north': 'south',
            'northeast': 'southwest',
            'east': 'west',
            'southeast': 'northwest',
            'south': 'north',
            'southwest': 'northeast',
            'west': 'east',
            'northwest': 'southeast' }

def connect(source, dest, transition, direction, reverse = None):
    k = _mirror.keys()
    if not direction in k:
        raise ValueError('direction must be in %s', k)
    
    if reverse is None:
        reverse = _mirror[direction]

    transition.source = source
    transition.dest = dest
    setattr(source, direction, transition)
    t = copy.copy(transition)
    t.source = dest
    t.dest = source
    setattr(dest, reverse, t)
    
