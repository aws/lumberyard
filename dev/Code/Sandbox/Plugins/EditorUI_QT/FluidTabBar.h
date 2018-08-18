/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef FLUIDTABBAR_H
#define FLUIDTABBAR_H

#include "api.h"
#include <QTabBar>

class EDITOR_QT_UI_API FluidTabBar
    : public QTabBar
{
    Q_OBJECT

public:
    FluidTabBar(QWidget* p);

    //Ensure the width of the tab bar is not dependent on the contents
    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
    virtual void paintEvent(QPaintEvent*) override;

    void CloseTab(QString name);
    void CloseAll();
    void CloseAllBut(int keepOpenIdx);

private:
    void AddCloseButtonToCurrentOnly();
    void AddCloseButton(int tabIndex);
    void RemoveCloseButton(int tabindex);
};

#endif