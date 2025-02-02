/* @file ngage_application.cpp
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
