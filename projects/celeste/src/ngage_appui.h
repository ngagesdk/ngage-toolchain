/* @file ngage_appui.h
 *
 * Nokia N-Gage toolchain demo application.
 *
 * CC0 http://creativecommons.org/publicdomain/zero/1.0/
 * SPDX-License-Identifier: CC0-1.0
 *
 */

#ifndef NGAGE_APPUI_H
#define NGAGE_APPUI_H

#include <aknappui.h>

class CNGageAppView;

class CNGageAppUi : public CAknAppUi
{
public:
    void ConstructL();

    CNGageAppUi();
    ~CNGageAppUi();

public:
    void HandleCommandL(TInt aCommand);

private:
    CNGageAppView* iAppView;
};

#endif /* NGAGE_APPUI_H */
