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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_RAMPCONTROL_H
#define CRYINCLUDE_EDITOR_CONTROLS_RAMPCONTROL_H
#pragma once

#include <vector>
#include <QWidget>

// RampControl

#ifdef KDAP_PORT
static UINT NEAR WM_HANDLECHANGED = RegisterWindowMessage("HANDLECHANGED");
static UINT NEAR WM_HANDLEDELETED = RegisterWindowMessage("HANDLEDELETED");
static UINT NEAR WM_HANDLEADDED = RegisterWindowMessage("HANDLEADDED");
static UINT NEAR WM_HANDLESELECTED = RegisterWindowMessage("HANDLESELECTED");
static UINT NEAR WM_HANDLESELECTIONCLEARED = RegisterWindowMessage("HANDLESELECTIONCLEAR");
#endif

class QMenu;


class CRampControl
    : public QWidget
{
    Q_OBJECT
public:
    enum HandleFlags
    {
        hfSelected = 1 << 0,
        hfHotitem = 1 << 1,
    };

    struct HandleData
    {
        float percentage;
        int flags;
    };

private:
    #define ID_MENU_ADD             33000
    #define ID_MENU_SELECT          33001
    #define ID_MENU_DELETE          33002
    #define ID_MENU_SELECTALL       33003
    #define ID_MENU_SELECTNONE      33004
    #define ID_MENU_CUSTOM_BEGIN    33005
    #define ID_MENU_CUSTOM_END      33015

public:
    CRampControl(QWidget* parent = 0);
    virtual ~CRampControl();

    QSize sizeHint() const override;

signals:
    void handleAdded(int percent);
    void handleSelectionCleared();
    void handleSelected();
    //  void handleSelected(int percent);
    void handleChanged();
    void handleDeleted();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

    virtual void OnMenuCustom(UINT nID);

protected slots:
    void OnMenuAdd();
    void OnMenuSelect();
    void OnMenuDelete();
    void OnMenuSelectAll();
    void OnMenuSelectNone();

public:
    virtual void DrawRamp(QPainter& dc);
    virtual void DrawHandle(QPainter& dc, const HandleData& data);
    virtual void DrawBackground(QPainter& dc);
    virtual void DrawForeground(QPainter& dc);

    virtual void AddCustomMenuOptions(QMenu& menu);

    virtual float DeterminePercentage(const QPoint& point);
    virtual bool AddHandle(const float percent);
    virtual void SortHandles();
    static bool SortHandle(const CRampControl::HandleData& dataA, const CRampControl::HandleData& dataB);
    virtual void ClearSelectedHandles();
    virtual void Reset();
    virtual bool SelectHandle(const float percent);
    virtual bool DeSelectHandle(const float percent);
    virtual bool IsSelectedHandle(const float percent);
    virtual void SelectAllHandles();
    virtual float MoveHandle(HandleData& data, const float percent);
    virtual bool HitAnyHandles(const float percent);
    virtual bool HitTestHandle(const HandleData& data, const float percent);
    static bool CheckSelected(const HandleData& data);
    virtual void RemoveSelectedHandles();
    virtual void SelectNextHandle();
    virtual void SelectPreviousHandle();

    virtual const bool GetSelectedData(HandleData& data);
    virtual const int GetHandleCount();
    virtual const HandleData GetHandleData(const int idx);
    virtual const int GetHotHandleIndex();

    virtual void SetHotItem(const float percentage);

    virtual void SetMaxHandles(int max) { m_max = max; }
    virtual const int GetMaxHandles() { return m_max; }
    virtual void SetDrawPercentage(bool draw) { m_drawPercentage = draw; }

protected:
    std::vector<HandleData> m_handles;
    QPoint* m_ptLeftDown;
    QPoint m_menuPoint;
    UINT m_max;
    bool m_movedHandles;
    bool m_canMoveSelected;
    bool m_mouseMoving;
    bool m_drawPercentage;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_RAMPCONTROL_H
