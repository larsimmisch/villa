import time
import logging
from molecule import *

log = logging.getLogger('mail')

class MailDialog(object):

    def __init__(self, rcpt):
        self.rcpt = rcpt
        self.caller = None
        self.tid = None

    def start(self, caller):
        self.caller = caller

        self.name = '%s_%s.wav' % (time.strftime('%y-%m-%d_%H-%M-%S'),
                                   self.rcpt)
        
        log.debug('%s recording mail for %s: %s', caller, self.rcpt,
                  self.name)

        # check if path exists and create if necessary
        path = os.path.join(get_root(), 'user', self.rcpt)
        if not os.path.exists(path):
            log.debug('%s creating dirs %s', caller, path)
            os.makedirs(path)

        m = Molecule(P_Normal)
        m.append(PlayAtom('postfuer.wav', prefix='lars'))
        # No names yet, just numbers
        for c in self.rcpt:
            m.append(PlayAtom('%s.wav' % c, prefix='lars'))
    
        m.append(BeepAtom(1))
        m.append(RecordAtom(self.name, 60,
                            prefix=os.path.join('user', self.rcpt)))
        self.tid = caller.enqueue(m)

        return self.tid

    def MLCA(self, caller, event, user_data):
        tid = event['tid']
        if tid == self.tid:
            self.tid = None
            # if the caller is online, deliver immediately
            for c in self.caller.world.callers.itervalues():
                if c.details.calling == self.rcpt:
                    log.debug('%s delivering immediately %s', c, self.name)
                    m = Molecule(P_Normal)
                    m.append(PlayAtom('duhastpost.wav', prefix='lars'))
                    m.append(PlayAtom(self.name,
                                      prefix=os.path.join('user', self.rcpt)))
                    c.enqueue(m)

            # we're done
            return True

        return False

    def DTMF(self, event):
        pass
