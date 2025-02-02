/* @file ngage_document.h
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
