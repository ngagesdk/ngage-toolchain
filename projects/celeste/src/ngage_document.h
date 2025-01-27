/* @file ngage_document.h
 *
 * Nokia N-Gage toolchain demo application.
 *
 * CC0 http://creativecommons.org/publicdomain/zero/1.0/
 * SPDX-License-Identifier: CC0-1.0
 *
 */

#ifndef NGAGE_DOCUMENT_H
#define NGAGE_DOCUMENT_H

#include <akndoc.h>

class CNGageAppUi;
class CEikApplication;

class CNGageDocument : public CAknDocument
{
public:
    static CNGageDocument* NewL(CEikApplication& aApp);
    static CNGageDocument* NewLC(CEikApplication& aApp);

    ~CNGageDocument();

public:
    CEikAppUi* CreateAppUiL();

private:
    void ConstructL();
    CNGageDocument(CEikApplication& aApp);
};

#endif /* NGAGE_DOCUMENT_H */
