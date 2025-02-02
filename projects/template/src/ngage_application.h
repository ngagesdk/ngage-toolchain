/* @file ngage_application.h
 *
 */

#ifndef NGAGE_APPLICATION_H
#define NGAGE_APPLICATION_H

#include <aknapp.h>

class CNGageApplication : public CAknApplication
{
public:
    TUid AppDllUid() const;

protected:
    CApaDocument* CreateDocumentL();
};

#endif /* NGAGE_APPLICATION_H */
