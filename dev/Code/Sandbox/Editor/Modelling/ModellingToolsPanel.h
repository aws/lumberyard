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

// Description : Polygon creation edit tool.

#ifndef CRYINCLUDE_EDITOR_MODELLING_MODELLINGTOOLSPANEL_H
#define CRYINCLUDE_EDITOR_MODELLING_MODELLINGTOOLSPANEL_H
#pragma once

#include <QScopedPointer>
#include <QWidget>

/////////////////////////////////////////////////////////////////////////////
// CModellingToolsPanel dialog
class QPushButton;
class QTimer;

namespace Ui {
    class CModellingToolsPanel;
}

class CModellingToolsPanel
    : public QWidget
{
    Q_OBJECT
    // Construction
public:
    CModellingToolsPanel(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CModellingToolsPanel();

    // Implementation
protected slots:
    void CreateButtons();
    void OnButtonPressed();
    void UncheckAll();
    void OnTimer();

private:
    QTimer* m_timer;
    QString m_currentToolName;
    QVector<QPushButton*> m_pushButtons;
    QScopedPointer<Ui::CModellingToolsPanel> ui;
};


#endif // CRYINCLUDE_EDITOR_MODELLING_MODELLINGTOOLSPANEL_H
