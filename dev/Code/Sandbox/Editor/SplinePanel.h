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
#include "EditTool.h"

class CSplineObject;

// This constant is used with kSplinePointSelectionRadius and was found experimentally.
static const float kClickDistanceScaleFactor = 0.01f;

namespace Ui
{
    class SplinePanel;
}

class CEditSplineObjectTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CEditSplineObjectTool()
        : m_pSpline(0)
        , m_currPoint(-1)
        , m_modifying(false)
        , m_curCursor(STD_CURSOR_DEFAULT)
    {}

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    bool TabletCallback(CViewport* view, ETabletEvent event, const QPoint& point, const STabletContext& tabletContext) override { return false; }

    void OnManipulatorDrag(CViewport* pView, ITransformManipulator* pManipulator, QPoint& point0, QPoint& point1, const Vec3& value) override;

    virtual void SetUserData(const char* key, void* userData);

    virtual void Display(DisplayContext& dc) {}
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);

    bool IsNeedMoveTool() override { return true; }
    bool IsNeedToSkipPivotBoxForObjects()   override {  return true; }

protected:
    virtual ~CEditSplineObjectTool();
    void DeleteThis() { delete this; }

    void SelectPoint(int index);
    void SetCursor(EStdCursor cursor, bool bForce = false);

    CSplineObject* m_pSpline;
    int m_currPoint;
    bool m_modifying;
    QPoint m_mouseDownPos;
    Vec3 m_pointPos;
    EStdCursor m_curCursor;
};

class CInsertSplineObjectTool
    : public CEditSplineObjectTool
{
    Q_OBJECT
public:
    CInsertSplineObjectTool();

    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;

protected:
    virtual ~CInsertSplineObjectTool();
};

class CMergeSplineObjectsTool
    : public CEditTool
{
    Q_OBJECT
public:
    CMergeSplineObjectsTool();

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    bool TabletCallback(CViewport* view, ETabletEvent event, const QPoint& point, const STabletContext& tabletContext) override { return false; }

    virtual void SetUserData(const char* key, void* userData);
    virtual void Display(DisplayContext& dc) {}
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);

protected:
    virtual ~CMergeSplineObjectsTool();
    void DeleteThis() { delete this; }

    int m_curPoint;
    CSplineObject* m_pSpline;
};

class CSplitSplineObjectTool
    : public CEditTool
{
    Q_OBJECT
public:
    CSplitSplineObjectTool();

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    bool TabletCallback(CViewport* view, ETabletEvent event, const QPoint& point, const STabletContext& tabletContext) override { return false; }

    virtual void SetUserData(const char* key, void* userData);
    virtual void Display(DisplayContext& dc) {};
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);

protected:
    virtual ~CSplitSplineObjectTool();
    void DeleteThis() { delete this; };

private:
    CSplineObject* m_pSpline;
    int m_curPoint;
};

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
