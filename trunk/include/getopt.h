/*
    // example commandline parsing

    while( (c = getopt(argc, argv, "d:l:")) != EOF) {
	switch(c) 
	{
	case 'd':
		// do something
		break;
	case 'l':
		// do something else
		break;
        case '?':
            usage();
        default:
            usage();
        }
    }
*/

#ifdef __cplusplus
extern "C" {
#endif

extern int  opterr;            /* error => print message */
extern int  optind;            /* next argv[] index */
extern char *optarg;       /* option parameter if any */

typedef int OPTCHAR;

extern OPTCHAR getopt( int argc, char *argv[], char *optstring );        /* returns letter, '?', EOF */

#ifdef __cplusplus
}
#endif
