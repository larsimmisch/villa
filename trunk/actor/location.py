import sys
import logging
from sequencer import callLater
from molecule import BeepMolecule, Background, Normal

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
        '''Enter the location. Play door_in sound to all'''
        caller.location = self
        caller.user_data = self.user_data()

        log.debug('enter: %s', caller)
        
        self.callers.append(caller)

        # door sound
        if hasattr(self, 'door_in'):
            caller.enqueue(self.door_in)

            # play door sound to other callers
            for c in self.callers:
                if c != caller:
                    c.enqueue(self.door_in)

        if hasattr(self, 'background'):
            caller.enqueue(self.background)

    def leave(self, caller, gone = False):
        '''Leave the location. Adjust caller list iterators
        in all participants.
        Play door_out sound to all.'''
        for c in self.callers:
            it = c.user_data.it_callers
            if it:
                it.invalidate(caller)
                
        self.callers.remove(caller)

        caller.discard(Background, Normal)
    
        if hasattr(self, 'door_out'):
            caller.enqueue(self.door_out)

            # play door sound to other callers
            for c in self.callers:
                c.enqueue(self.door_out)

        d = caller.user_data
        caller.user_data = None
        del d
        caller.location = None

    def move(self, caller, location):
        self.leave(caller)
        location.enter(caller)

    def generic_invalid(self, caller):
        caller.enqueue(BeepMolecule(Normal, 2))

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
                dir == 'southeast'
            else:
                data.cancel('tm_move')
                self.move_invalid(caller)
            if dir:
                m = getattr(self, dir)
                if m:
                    m(caller)

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
            data.tid.talk = None
        elif dtmf == '4':
            data.tid_talk = caller.enqueue(
                RecordBeepAtom(Normal, str(caller) + '.wav', 10.0))
