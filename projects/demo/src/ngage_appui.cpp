/* @file ngage_appui.cpp
 *
 * Nokia N-Gage toolchain demo application.
 *
 * CC0 http://creativecommons.org/publicdomain/zero/1.0/
 * SPDX-License-Identifier: CC0-1.0
 *
 */

#include <avkon.hrh>
#include <aknnotewrappers.h>

#include "ngage_appui.h"
#include "ngage_appview.h"

void CNGageAppUi::ConstructL()
{
    BaseConstructL();

    iAppView = CNGageAppView::NewL(ClientRect());

    AddToStackL(iAppView);
}

CNGageAppUi::CNGageAppUi()
{
    RProcess Proc;

    iAppView = NULL;

    if (KErrNone == Proc.Create(_L("E:\\System\\Apps\\demo\\game.exe"), _L("")))
    {
        Proc.Resume();
        Proc.Close();
        User::After(10000000);
        Exit();
    }
    else
    {
        Exit();
    }
}

CNGageAppUi::~CNGageAppUi()
{
    if (iAppView)
    {
        RemoveFromStack(iAppView);
        delete iAppView;
        iAppView = NULL;
    }
}

void CNGageAppUi::HandleCommandL(TInt aCommand)
{
    /* No implementation required. */
}
