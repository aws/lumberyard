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

#ifndef CRYINCLUDE_EDITOR_SPLINEPANEL_H
#define CRYINCLUDE_EDITOR_SPLINEPANEL_H

#pragma once

#include <QWidget>
#include "Controls/ToolButton.h"

class CSplineObject;

// This constant is used with kSplinePointSelectionRadius and was found experimentally.
static const float kClickDistanceScaleFactor = 0.01f;

namespace Ui
{
    class SplinePanel;
}

#ifdef Q_MOC_RUN
class CEditSplineObjectTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CEditSplineObjectTool();
};
class CInsertSplineObjectTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CInsertSplineObjectTool();
};
class CMergeSplineObjectsTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CMergeSplineObjectsTool();
};
class CSplitSplineObjectTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CSplitSplineObjectTool();
};
#endif

class CSplinePanel
    : public QWidget
{
    Q_OBJECT

public:
    CSplinePanel(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CSplinePanel();

    void SetSpline(CSplineObject* pSpline);
    CSplineObject* GetSpline() const { return m_pSpline; }

    void Update();

protected slots:
    void OnDefaultWidth(int state);

protected:
    void OnInitDialog();

    QScopedPointer<Ui::SplinePanel> m_ui;

    CSplineObject* m_pSpline;
};

#endif // CRYINCLUDE_EDITOR_SPLINEPANEL_H
