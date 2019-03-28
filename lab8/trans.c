/**
 * name:    张万强
 * ID：     ics516072910091
 * 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, x, y;
    int t1, t2, t3, t4; //t5,t6,t7,t8;
    if (M == 32)
    {
        for (i = 0; i < 4; i++)
        {
            for (j = 0; j < 4; j++)
            {
                for (x = 0; x < 8; x++)
                {
                    for (y = 0; y < 8; y++)
                    {
                        if (8 * i + y == 8 * j + x)
                        {
                            t1 = A[8 * i + y][8 * j + x]; //avoid same row conflict
                            t2 = 8 * i + y;
                        }
                        else
                        {
                            B[8 * i + y][8 * j + x] = A[8 * j + x][8 * i + y];
                        }
                    }
                    if (i == j)
                    {
                        B[t2][t2] = t1; //set back
                    }
                }
            }
        }
    }
    else if (M == 61)
    {
        for (j = 0; j < 5; j++)
        {
            for (i = 0; i < 5; i++)
            {
                for (x = 0; x < 16 && (16 * i + x) < 61; x++)
                {
                    for (y = 0; y < 16 && (16 * j + y) < 67; y++)
                    {
                        if (16 * i + x == 16 * j + y)
                        {
                            t1 = A[16 * i + x][16 * j + y]; //avoid same row conflict
                            t2 = 16 * i + x;
                        }
                        else
                        {
                            B[16 * i + x][16 * j + y] = A[16 * j + y][16 * i + x];
                        }
                    }
                    if (i == j)
                    {
                        B[t2][t2] = t1; //set back
                    }
                }
            }
        }
    }
    else if (M == 64)
    {
        for (i = 0; i < 64; i += 8)
        { // diagonal
            for (x = 0; x < 4; x++)
            {
                for (y = 0; y < 8; y++)
                {
                    B[x][y + 8] = A[i + x][i + y];
                }
                for (y = 0; y < 8; y++)
                {
                    B[x][y + 16] = A[i + x + 4][i + y];
                }
            }
            for (x = 0; x < 4; x++)
            {
                for (y = 0; y < 4; y++)
                {
                    B[i + x][i + y] = B[y][x + 8];
                    B[i + x][i + y + 4] = B[y][x + 16];
                }
                for (y = 0; y < 4; y++)
                {
                    B[i + x + 4][i + y] = B[y][x + 12];
                    B[i + x + 4][i + y + 4] = B[y][x + 20];
                }
            }
        }
        for (i = 0; i < 64; i += 8)
        {
            for (j = 0; j < 64; j += 8)
            { //[i][j] a 8*8 block
                if (i != j)
                { //not special 8 blocks
                    for (y = i; y < i + 4; y++)
                    {
                        for (x = j; x < j + 4; x++)
                        {
                            B[x][y] = A[y][x];         //convert 4*4
                            B[x][y + 4] = A[y][x + 4]; //convert 4*4 in a temp place on right
                        }
                    }
                    for (x = j; x < j + 4; x++)
                    {
                        t1 = B[x][i + 4];
                        t2 = B[x][i + 5];
                        t3 = B[x][i + 6];
                        t4 = B[x][i + 7]; // save temx before cache tag change
                        B[x][i + 4] = A[i + 4][x];
                        B[x][i + 5] = A[i + 5][x];
                        B[x][i + 6] = A[i + 6][x];
                        B[x][i + 7] = A[i + 7][x]; // correct the temp B line
                        B[x + 4][i] = t1;
                        B[x + 4][i + 1] = t2;
                        B[x + 4][i + 2] = t3;
                        B[x + 4][i + 3] = t4; // replace temp line and change cache tag
                        for (y = i; y < i + 4; y++)
                        {
                            B[x + 4][y + 4] = A[y + 4][x + 4]; // normal convert
                        }
                    }
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
