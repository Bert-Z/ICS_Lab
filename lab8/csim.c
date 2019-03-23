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
#include <math.h>
#include <ctype.h>

#include "cachelab.h"

#define ISVALIDE(x) (x & (1UL << 63))
#define SETVALIDE(x) (x | (1UL << 63))
#define ISHIT(x)

static int _v = 0;         // Has -v ?
static int _s = 0;         // Number of set index bits.
static int _E = 0;         // Number of lines per set.
static int _b = 0;         // Number of block offset bits.
static char _t[20];        // Trace file.
static char *cache = NULL; // Cache memory
static int SetNumber = 0;  // Number of sets
static int cachesize = 0;

static int hits = 0;
static int misses = 0;
static int evictions = 0;

static void getInput(int argc, char *argv[]);
static int readTraceFile(char *filename);
static void initCache();
static int isHit(long address);
static long getAddress(char *traceline);

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

static int readTraceFile(char *filename)
{
    FILE *trace;
    char traceLine[1024];
    long traceAddr;

    if ((trace = fopen(filename, "r")) == NULL)
    {
        printf("error");
        return -1;
    }

    while (fgets(traceLine, 1024, trace))
    {
        if (traceLine[0] != 'I' && traceLine[1] != ' ')
        {
            traceAddr = getAddress(traceLine);
            // printf("%ld\n", traceAddr);
            printf("%d\n", isHit(traceAddr));
        }
    }

    fclose(trace);
    return 0;
}

static void initCache()
{
    SetNumber = (int)pow(2, _s);
    cachesize = SetNumber * sizeof(long);

    cache = (char *)malloc(cachesize);
    memset(cache, 0, cachesize);

    // printf("%ld\n",SetNumber * sizeof(long));

    // for (int i = 0; i < 5; ++i)
    // {
    //     printf("%d\x20", ((int *)cache)[i]);
    // }
}

static int isHit(long address)
{
    long valAddress = (long)(SETVALIDE(address));

    for (int i = 0; i < (cachesize / (sizeof(long))); ++i)
        if (((long *)cache)[i] == valAddress)
            return 1;

    return 0;
}

static long getAddress(char *traceline)
{
    int i = 3, j = 0;
    char traAddr[1024];
    while (i < 1024)
    {
        if (!isdigit(traceline[i]))
        {
            traAddr[j] = '\0';
            break;
        }
        traAddr[j] = traceline[i];
        i++;
        j++;
    }

    return atoi(traAddr);
}

int main(int argc, char *argv[])
{
    getInput(argc, argv);
    initCache();

    readTraceFile(_t);

    // printf("%ld\n",sizeof(long));

    printSummary(hits, misses, evictions);
    return 0;
}
