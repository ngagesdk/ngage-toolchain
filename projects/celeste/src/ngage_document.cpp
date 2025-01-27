/* @file ngage_document.cpp
 *
 * Nokia N-Gage toolchain demo application.
 *
 * CC0 http://creativecommons.org/publicdomain/zero/1.0/
 * SPDX-License-Identifier: CC0-1.0
 *
 */

#include "ngage_appui.h"
#include "ngage_document.h"

CNGageDocument* CNGageDocument::NewL(CEikApplication& aApp)
{
    CNGageDocument* self = NewLC(aApp);
    CleanupStack::Pop(self);
    return self;
}

CNGageDocument* CNGageDocument::NewLC(CEikApplication& aApp)
{
    CNGageDocument* self = new (ELeave) CNGageDocument(aApp);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

void CNGageDocument::ConstructL()
{
    /* No implementation required. */
}

CNGageDocument::CNGageDocument(CEikApplication& aApp) : CAknDocument(aApp)
{
    /* No implementation required. */
}

CNGageDocument::~CNGageDocument()
{
    /* No implementation required. */
}

CEikAppUi* CNGageDocument::CreateAppUiL()
{
    CEikAppUi* appUi = new (ELeave) CNGageAppUi;
    return appUi;
}
