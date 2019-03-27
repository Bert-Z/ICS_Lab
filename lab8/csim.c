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

#define RECENTUP(x) ((*(int *)((char *)(x + 1)))++)
#define ADDRESS(x) (*(unsigned long *)((char *)(x + 5)))
#define RECENT(x) (*(int *)((char *)(x + 1)))
#define SETADDRESS(x, val) (*(unsigned long *)((char *)(x + 5)) = val)
#define SETRECENT(x, val) (*(int *)((char *)(x + 1)) = val)
#define GETVALID(x) (*(char *)((char *)(x)))
#define SETVALID(x) ((*(char *)((char *)(x))) = '1') // set the address valid

static int _v = 0;         // Has -v ?
static int _s = 0;         // Number of set index bits.
static int _E = 0;         // Number of lines per set.
static int _b = 0;         // Number of block offset bits.
static char _t[20];        // Trace file.
static char *cache = NULL; // Cache memory
static int SetNumber = 0;  // Number of sets
static int cachesize = 0;  // size of cache

static int hits = 0;
static int misses = 0;
static int evictions = 0;

static void getInput(int argc, char *argv[]); // catch the command of the input
static void initCache();                      // init the cache in memmory
static int isHit(unsigned long address);               // if the trace hit the cache
static unsigned long getAddress(char *traceline);      // catch the address from the traceline
static int readTraceFile(char *filename);     // read file
static int isFull(int set);                   // is full
static int findLargest(int set);              // find the largest recent
static int getSet(unsigned long address);
static void dataLoad(unsigned long address); // load
static void othersup(int set, unsigned long address); // others recent ++ (so the lowest is the most recent one)

static int getSet(unsigned long address)
{
    int a = (int)pow(2, _s);
    return (int)(address % a);
}

// 1+4+8=13
static void othersup(int set, unsigned long address)
{
    for (int i = 0; i < (cachesize / (sizeof(unsigned long) + sizeof(char) + sizeof(int))); i++)
    {
        if (GETVALID(cache + 13 * i) == '1')
        {
            unsigned long thisaddr = ADDRESS(cache + 13 * i);

            if ((int)getSet(thisaddr) == set)
            {
                if (thisaddr != address)
                {
                    RECENTUP(cache + 13 * i);
                }
                else
                {
                    *(int *)((char *)(cache + 13 * i + 1)) = 0;
                    // break;
                }
            }
        }
    }
}

static void dataLoad(unsigned long address)
{
    int set = getSet(address);

    if (isHit(address))
    {
        hits++;
        othersup(set, address);
    }
    else
    {
        misses++;
        int rmax = findLargest(set);
        if (isFull(set))
        {
            evictions++;
            for (int i = 0; i < (cachesize / (sizeof(unsigned long) + sizeof(char) + sizeof(int))); i++)
            {
                unsigned long addr = ADDRESS(cache + 13 * i);
                if (getSet(addr) == set)
                {
                    int recent = RECENT(cache + 13 * i);
                    if (recent == rmax)
                    {
                        SETVALID(cache + 13 * i);
                        SETADDRESS(cache + 13 * i, address);
                        break;
                    }
                }
            }
            othersup(set, address);
        }
        else
        {
            for (int i = 0; i < (cachesize / (sizeof(unsigned long) + sizeof(char) + sizeof(int))); i++)
            {
                unsigned long addr = ADDRESS(cache + 13 * i);
                if (getSet(addr) == set)
                {

                    char valid = GETVALID(cache + 13 * i);
                    if (valid != '1')
                    {
                        SETVALID(cache + 13 * i);
                        SETADDRESS(cache + 13 * i, address);
                        break;
                    }
                }
            }

            othersup(set, address);
        }
    }
}

static int findLargest(int set)
{
    int rmax = 0;

    for (int i = 0; i < (cachesize / (sizeof(unsigned long) + sizeof(char) + sizeof(int))); i++)
    {
        unsigned long addr = ADDRESS(cache + 13 * i);
        if (getSet(addr) == set)
        {
            unsigned long recent = RECENT(cache + 13 * i);
            if (recent > rmax)
                rmax = recent;
        }
    }
    return rmax;
}

static int isFull(int set)
{
    unsigned long addr = 0;
    int count = 0;
    for (int i = 0; i < (cachesize / (sizeof(unsigned long) + sizeof(char) + sizeof(int))); i++)
    {
        addr = ADDRESS(cache + 13 * i);
        if ((int)getSet(addr) == set)
        {
            count++;
            char valid = GETVALID(cache + 13 * i);
            if (valid != '1')
            {
                return 0;
            }
        }
    }
    if (count == 0)
        return 0;
    return 1;
}

static void getInput(int argc, char *argv[])
{
    int ch;

    while ((ch = getopt(argc, argv, "vs:E:b:t:")) != -1)
    {
        switch (ch)
        {
        case 'v':
            _v = 1;
            break;
        case 's':
            _s = atoi(optarg);
            break;
        case 'E':
            _E = atoi(optarg);
            break;
        case 'b':
            _b = atoi(optarg);
            break;
        case 't':
            strcpy(_t, optarg);
            break;
        case '?':
            break;
        }
    }
}

//  cache structure
// |___________|_____________|________________________|
//     1bytes       4bytes              8bytes
//    isvalid      recently            content

static void initCache()
{
    SetNumber = (int)pow(2, _s) * _E;

    cachesize = SetNumber * (sizeof(char) + sizeof(int) + sizeof(unsigned long));

    cache = (char *)malloc(cachesize);
    memset(cache, 0, cachesize);

    for (int i = 0; i < (cachesize / (sizeof(unsigned long) + sizeof(char) + sizeof(int))); i++)
    {
        SETADDRESS(cache + 13 * i, i / _E);
    }
}

static unsigned long getAddress(char *traceline)
{
    int i = 3, j = 0;
    char traAddr[1024];
    while (i < 1024)
    {
        if (traceline[i] == ',')
        {
            traAddr[j] = '\0';
            break;
        }
        traAddr[j] = traceline[i];
        i++;
        j++;
    }

    int num;
    sscanf(traAddr, "%x", &num);

    return num;
}

static int readTraceFile(char *filename)
{
    FILE *trace;
    char traceLine[1024];
    unsigned long traceAddr;

    if ((trace = fopen(filename, "r")) == NULL)
    {
        printf("error");
        return -1;
    }

    while (fgets(traceLine, 1024, trace))
    {
        if (traceLine[0] != 'I' && traceLine[1] != ' ')
        {
            traceAddr = ((unsigned long)getAddress(traceLine) >> _b);

            if (traceLine[1] == 'M')
            {
                dataLoad(traceAddr);
                dataLoad(traceAddr);
            }
            else if (traceLine[1] == 'L')
            {
                dataLoad(traceAddr);
            }
            else
            {
                dataLoad(traceAddr);
            }
        }
    }

    fclose(trace);
    return 0;
}

static int isHit(unsigned long address)
{
    for (int i = 0; i < (cachesize / (sizeof(unsigned long) + sizeof(char) + sizeof(int))); i++)
    {
        if (GETVALID(cache + 13 * i) == '1')
        {
            unsigned long thisaddr = ADDRESS(cache + 13 * i);
            if (thisaddr == address)
                return 1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    getInput(argc, argv);
    initCache();

    readTraceFile(_t);

    printSummary(hits, misses, evictions);

    return 0;
}