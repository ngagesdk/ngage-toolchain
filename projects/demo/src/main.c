/* @file main.c
 *
 * Nokia N-Gage toolchain demo application.
 *
 * CC0 http://creativecommons.org/publicdomain/zero/1.0/
 * SPDX-License-Identifier: CC0-1.0
 *
 */

#include <stdlib.h>
#include <stdio.h>

int __gccmain(int argc, char* argv[])
{
    FILE* file = fopen("E:\\hello.txt", "w");
    if (NULL == file)
    {
        return 1;
    }

    for (int i = 0; i < 100; i++)
    {
        fprintf(file, "Hello, world! %d\n", i);
    }
    fclose(file);

    return 0;
}
