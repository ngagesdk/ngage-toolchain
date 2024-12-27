/* @file ngage_appview.h
 *
 * Nokia N-Gage toolchain demo application.
 *
 * CC0 http://creativecommons.org/publicdomain/zero/1.0/
 * SPDX-License-Identifier: CC0-1.0
 *
 */

#ifndef NGAGE_APPVIEW_H
#define NGAGE_APPVIEW_H

#include <coecntrl.h>

class CNGageAppView : public CCoeControl
{
public:
    static CNGageAppView* NewL(const TRect& aRect);
    static CNGageAppView* NewLC(const TRect& aRect);

    ~CNGageAppView();

public:
    void Draw(const TRect& aRect) const;

private:
    void ConstructL(const TRect& aRect);

    CNGageAppView();
};

#endif /* NGAGE_APPVIEW_H */
