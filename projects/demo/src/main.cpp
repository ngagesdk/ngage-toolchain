/* @file main.cpp
 *
 * Nokia N-Gage toolchain demo application.
 *
 * CC0 http://creativecommons.org/publicdomain/zero/1.0/
 * SPDX-License-Identifier: CC0-1.0
 *
 */

#include <e32std.h>
#include <e32def.h>
#include <e32svr.h>
#include <e32base.h>
#include <estlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <w32std.h>
#include <apgtask.h>

extern "C" int __gccmain(int argc, char* argv[]);

TInt E32Main()
{
    /*  Get the clean-up stack */
    CTrapCleanup* cleanup = CTrapCleanup::New();

    /* Arrange for multi-threaded operation */
    SpawnPosixServerThread();

    /* Get args and environment */
    int    argc = 0;
    char** argv = 0;
    char** envp = 0;

    __crt0(argc, argv, envp);

    /* Start the application! */

    /* Create stdlib */
    _REENT;

    /* Set process and thread priority and name */

    RThread  currentThread;
    RProcess thisProcess;
    TParse   exeName;
    exeName.Set(thisProcess.FileName(), NULL, NULL);
    currentThread.Rename(exeName.Name());
    currentThread.SetProcessPriority(EPriorityLow);
    currentThread.SetPriority(EPriorityMuchLess);

    /* Increase heap size */
    RHeap* newHeap = NULL;
    RHeap* oldHeap = NULL;
    TInt   heapSize = 7500000;
    int    ret;

    newHeap = User::ChunkHeap(NULL, heapSize, heapSize, KMinHeapGrowBy);

    if (NULL == newHeap)
    {
        ret = 3;
        goto cleanup;
    }
    else
    {
        oldHeap = User::SwitchHeap(newHeap);
        /* Call stdlib main */
        ret = __gccmain(argc, argv);
    }

cleanup:
    _cleanup();

    CloseSTDLIB();
    delete cleanup;
    return ret;
}
