#!/usr/bin/env python2.4

import urllib
import re
import sys

import StringIO

obs_data = '<observation observer="http://sputnik.congress.ccc.de/tracking-observers/10.254.1.16" id="254002" observed-object="http://sputnik.congress.ccc.de/tracking-users/254002" time="1167491168.0e0" priority="0" min-distance="0.0" max-distance="51.000000e0" direction="[0,0,0]"/>'

from types import *
from xml import sax

def parseDirection(dir):
    return [int(d) for d in dir[1:-1].split(',')]

class Observation(object):
    def __init__(self, attrs):
        for t in attrs:
            if t[0] in ['time', 'min-distance', 'max-distance']:
                setattr(self, t[0], float(t[1]))
            elif t[0] == 'priority':
                setattr(self, t[0], int(t[1]))                
            elif t[0] == 'direction':
                setattr(self, t[0], parseDirection(t[1]))                
            else:
                setattr(self, *t)

class ObservationParser(sax.ContentHandler):
    def __init__(self):
        self.stack = []

    def startElement(self, name, attrs):
        if name == "observation":
            self.stack.append(Observation(attrs.items()))

def track(id):
    f = urllib.urlopen('http://sputnik.congress.ccc.de:8000/0')
    p = re.compile(r'id="%s"' % id)

    l = f.readline()
    while l:
        if p.search(l):
            print l

        l = f.readline()

if __name__ == '__main__':

    parser = ObservationParser()
    obs = sax.parseString(obs_data, parser)

##     if len(sys.argv) > 1:
##         track(sys.argv[0])
##     else:
##         track('254101')
