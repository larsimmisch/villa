#!/usr/bin/env python

import urllib
import re
import sys

# <observation observer="http://sputnik.congress.ccc.de/tracking-observers/10.254.1.16" id="254002" observed-object="http://sputnik.congress.ccc.de/tracking-users/254002" time="1167491168.0e0" priority="0" min-distance="0.0" max-distance="51.000000e0" direction="[0,0,0]"/>

def track(id):
    f = urllib.urlopen('http://sputnik.congress.ccc.de:8000/0')
    p = re.compile(r'id="%s"' % id)

    l = f.readline()
    while l:
        if p.search(l):
            print l

        l = f.readline()

if __name__ == '__main__':
    if len(sys.argv) > 1:
        track(sys.argv[0])
    else:
        track('254101')
