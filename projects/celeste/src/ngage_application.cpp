/* @file ngage_application.cpp
 *
 * Nokia N-Gage toolchain demo application.
 *
 * CC0 http://creativecommons.org/publicdomain/zero/1.0/
 * SPDX-License-Identifier: CC0-1.0
 *
 */

#include "ngage_document.h"
#include "ngage_application.h"

static const TUid KUidNGageApp = { UID3 };

CApaDocument* CNGageApplication::CreateDocumentL()
{
    CApaDocument* document = CNGageDocument::NewL(*this);
    return document;
}

TUid CNGageApplication::AppDllUid() const
{
    return KUidNGageApp;
}
