/**
 * name:        张万强
 * loginID:     ics516072910091
 * 
*/
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cachelab.h"

static int _v = 0;  // Has -v ?
static int _s = 0;  // Number of set index bits.
static int _E = 0;  // Number of lines per set.
static int _b = 0;  // Number of block offset bits.
static char _t[20]; // Trace file.

static int hits = 0;
static int misses = 0;
static int evictions = 0;

static void getInput(int argc, char *argv[]);

static void getInput(int argc, char *argv[])
{
    int ch;
    // printf("\n\n");
    // printf("optind:%d，opterr：%d\n", optind, opterr);
    // printf("--------------------------\n");
    while ((ch = getopt(argc, argv, "vs:E:b:t:")) != -1)
    {
        // printf("optind: %d\n", optind);
        switch (ch)
        {
        case 'v':
            printf("HAVE option: -v\n\n");
            _v = 1;
            break;
        case 's':
            _s = atoi(optarg);
            printf("HAVE option: -s\n");
            printf("The argument of -s is %d\n\n", _s);
            break;
        case 'E':
            _E = atoi(optarg);
            printf("HAVE option: -E\n");
            printf("The argument of -E is %d\n\n", _E);
            break;
        case 'b':
            _b = atoi(optarg);
            printf("HAVE option: -b\n");
            printf("The argument of -b is %d\n\n", _b);
            break;
        case 't':
            strcpy(_t, optarg);
            printf("HAVE option: -t\n");
            printf("The argument of -t is %s\n\n", _t);
            break;
        case '?':
            printf("Unknown option: %c\n", (char)optopt);
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    getInput(argc, argv);

    printSummary(hits, misses, evictions);
    return 0;
}
