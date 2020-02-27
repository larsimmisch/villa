/*
 * getopt.c
 */

#include <stdio.h>
#include <string.h>

#include "getopt.h"

#ifdef __cplusplus
extern "C" {
#endif

int    opterr = 1;            /* error => print message */
int    optind = 1;            /* next argv[] index */
char   *optarg = NULL;            /* option parameter if any */

static int
Err( char *name, char *mess, int c )            /* returns '?' */
{
    if ( opterr )
        printf("%s: %s -- %c\n", name, mess, c );
    return '?';            /* erroneous-option marker */
}

OPTCHAR
getopt( int argc, char *argv[], char *optstring )        /* returns letter, '?', EOF */
{
    static int    sp = 1;        /* position within argument */
    register int    osp;        /* saved `sp' for param test */
    register int    c;        /* option letter */
    register char *cp;        /* -> option in `optstring' */

    optarg = NULL;

    if ( sp == 1 )            /* fresh argument */
        if ( optind >= argc        /* no more arguments */
          || argv[optind][0] != '-'    /* no more options */
          || argv[optind][1] == '\0'    /* not option; stdin */
           )
            return EOF;
        else if ( strcmp( argv[optind], "--" ) == 0 ) {
            ++optind;    /* skip over "--" */
            return EOF;    /* "--" marks end of options */
        }

    c = argv[optind][sp];        /* option letter */
    osp = sp++;            /* get ready for next letter */

    if ( argv[optind][sp] == '\0' ) {    /* end of argument */
        ++optind;        /* get ready for next try */
        sp = 1;            /* beginning of next argument */
    }

    if ( c == ':'            /* optstring syntax conflict */
      || (cp = strchr( optstring, c )) == NULL    /* not found */
       )
        return Err( argv[0], "illegal option", c );

    if ( cp[1] == ':' ) {       /* option takes parameter */
        if ( osp != 1 )
            return Err( argv[0], "option must not be clustered", c );

        if ( sp != 1 )        /* reset by end of argument */
            return Err( argv[0], "option must be followed by white space", c );

        if ( optind >= argc )
            return Err( argv[0], "option requires an argument", c );

        optarg = argv[optind];    /* make parameter available */
        ++optind;        /* skip over parameter */
    }

    return c;
}

#ifdef __cplusplus
}
#endif
