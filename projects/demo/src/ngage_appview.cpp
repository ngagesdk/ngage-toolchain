/* @file ngage_appview.cpp
 *
 * Nokia N-Gage toolchain demo application.
 *
 * CC0 http://creativecommons.org/publicdomain/zero/1.0/
 * SPDX-License-Identifier: CC0-1.0
 *
 */

#include <coemain.h>
#include "ngage_appview.h"

CNGageAppView* CNGageAppView::NewL(const TRect& aRect)
{
    CNGageAppView* self = CNGageAppView::NewLC(aRect);
    CleanupStack::Pop(self);
    return self;
}

CNGageAppView* CNGageAppView::NewLC(const TRect& aRect)
{
    CNGageAppView* self = new (ELeave) CNGageAppView;
    CleanupStack::PushL(self);
    self->ConstructL(aRect);
    return self;
}

CNGageAppView::CNGageAppView()
{
    /* No implementation required. */
}

CNGageAppView::~CNGageAppView()
{
    /* No implementation required. */
}

void CNGageAppView::ConstructL(const TRect& aRect)
{
    CreateWindowL();
    SetRect(aRect);
    ActivateL();
}

// Draw this application's view to the screen
void CNGageAppView::Draw(const TRect& /*aRect*/) const
{
    CWindowGc& gc = SystemGc();
    TRect      rect = Rect();

    gc.Clear(rect);
}
