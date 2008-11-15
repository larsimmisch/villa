#!/usr/bin/env python

import sys

chunk = 12
args = sys.argv[1:]

usage = """usage: raw-to-c <infile> <outfile> <name>

       raw-to-c reads the data from <infile> and creates
       a const char[] with <name> in <outfile>"""

if len(args) < 3:
    print usage
    sys.exit(1)

inf = open(args[0], 'rb')
outf = open(args[1], 'wb')

outf.write('static const unsigned char %s[] = {\n' % args[2])

data = inf.read(chunk)
while data:
    outf.write('    ')
    for c in data:
        outf.write('0x%02x, ' % ord(c))
    outf.write('\n')

    data = inf.read(chunk)
    
outf.write('};\n')

inf.close()
outf.close()
