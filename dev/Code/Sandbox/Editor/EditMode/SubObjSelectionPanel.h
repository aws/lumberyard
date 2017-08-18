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

#ifndef CRYINCLUDE_EDITOR_EDITMODE_SUBOBJSELECTIONPANEL_H
#define CRYINCLUDE_EDITOR_EDITMODE_SUBOBJSELECTIONPANEL_H
#pragma once

#include <QWidget>
#include <QScopedPointer>

// CSubObjSelectionPanel dialog
namespace Ui {
    class CSubObjSelectionPanel;
}
class CSubObjSelectionPanel
    : public QWidget
{
    Q_OBJECT

public:
    CSubObjSelectionPanel(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CSubObjSelectionPanel();

protected slots:
    void OnSelectByVertex();
    void OnIgnoreBackfacing();
    void onSoftSelection();
    void OnFallOff();

private:
    QScopedPointer<Ui::CSubObjSelectionPanel> ui;
};

#endif // CRYINCLUDE_EDITOR_EDITMODE_SUBOBJSELECTIONPANEL_H
