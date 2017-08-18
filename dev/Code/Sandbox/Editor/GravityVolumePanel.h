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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_GRAVITYVOLUMEPANEL_H
#define CRYINCLUDE_EDITOR_GRAVITYVOLUMEPANEL_H
#pragma once

#include "Controls/PickObjectButton.h"
#include "Controls/ToolButton.h"

#include <QWidget>
#include <QScopedPointer>

class CGravityVolumeObject;
namespace Ui {
    class CGravityVolumePanel;
}

#ifdef Q_MOC_RUN
class CEditGravityVolumeObjectTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CEditGravityVolumeObjectTool();
};
#endif

// CGravityVolumePanel dialog
class CGravityVolumePanel
    : public QWidget
{
public:
    CGravityVolumePanel(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CGravityVolumePanel();

    void SetGravityVolume(CGravityVolumeObject* GravityVolume);
    CGravityVolumeObject* GetGravityVolume() const { return m_GravityVolume; }

    void SelectPoint(int index);

protected:
    void OnBnClickedSelect();

    CGravityVolumeObject* m_GravityVolume;
    //CPickObjectButton m_pickButton;
    QScopedPointer<Ui::CGravityVolumePanel> ui;
};

#endif // CRYINCLUDE_EDITOR_GRAVITYVOLUMEPANEL_H
