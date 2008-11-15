#!/usr/bin/env python

import sys
import getopt
import re

preamble = '''#include <string.h>
#include <stdio.h>
#include "%s"

typedef struct
{
    char *name;
    int value;
} SYMENT;

static SYMENT SymTab[] = {
'''

postamble = '''};

const char *symname(int value)
{
    static char buf[256];
    int i;

    for (i = 0; i < sizeof(SymTab)/sizeof(SymTab[0]); ++i)
	if (SymTab[i].value == value)
	    return SymTab[i].name;

    sprintf(buf, "0x%X", value);
	return buf;
}
'''

def strip_comment(open_comment, line):
    "returns the tuple (open_comment, line)"
    # strip /* */ comments

    if open_comment:
        p = line.find('*/')
        if p >= 0:
            line = line[p+2:]
            open_comment = False
        else:
            return (True, '')
    
    p = line.find('/*')
    while p >= 0:
        # search closing '*/'
        pc = line.find('*/', p)
        if pc >= p:
            # found. continue searching
            le = line[pc+2:]
            open_comment = False
            line = line[:p] + le
            p = line.find('/*')
        else:
            return (True, line[:p])
 
    # strip '//' comments
    p = line.find('//')
    if p >= 0:
        return (False, line[:p])

    return (p >= 0, line)

def read_symbols(filename, prefix, symbols, ignore):
    "returns a dictionary from name to value"
    
    symbol = re.compile(r'#define\s*(' + prefix + r'[a-zA-Z0-9_]+)')

    f = open(filename, "r")
    lines = f.readlines()

    oc = False

    for l in lines:

        oc, l = strip_comment(oc, l)
        
        m = symbol.search(l)
        if m:
            sym = m.group(1)
            if not sym in ignore:
                symbols.append(sym)

    f.close()

    return symbols

def usage():
    print >> sys.stderr, "usage: symname.py [-i <ignore>] [-o <out> ] <prefix> <file>"
    sys.exit(2)

if __name__ == '__main__':

    ignore = []
    out = sys.stdout

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'o:i:')
    except getopt.GetoptError:
        usage()

    for o, a in opts:
        if o == '-i':
            ignore.append(a)
        elif o == '-o':
            out = open(a, 'w')
        else:
            usage()

    if len(args) < 2:
        usage()

    symbols = []

    read_symbols(args[1], args[0], symbols, ignore)

    symbols.sort()

    out.write(preamble % args[1])

    for s in symbols:
        out.write("    { \"%s\", %s },\n" % (s, s));

    out.write(postamble)
