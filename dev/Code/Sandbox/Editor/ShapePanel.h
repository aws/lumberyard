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

#ifndef CRYINCLUDE_EDITOR_SHAPEPANEL_H
#define CRYINCLUDE_EDITOR_SHAPEPANEL_H
#pragma once

#include "Controls/PickObjectButton.h"
#include "Controls/ToolButton.h"

#include "EntityPanel.h"
#include "EditTool.h"

#include <QWidget>

class CAITerritoryObject;
class CShapeObject;
class QStringListModel;
class QCheckBox;
class QLabel;
class QItemSelection;

namespace Ui
{
    class ShapePanel;
    class ShapeMultySelPanel;
    class NavigationAreaPanel;
    class RopePanel;
}

class CEditShapeObjectTool
    : public CEditTool
{
    Q_OBJECT
public:
    CEditShapeObjectTool();

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    virtual void SetUserData(const char* key, void* userData);

    virtual void BeginEditParams(IEditor* ie, int flags) {};
    virtual void EndEditParams() {};

    virtual void Display(DisplayContext& dc) {};
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    bool IsNeedToSkipPivotBoxForObjects()   override {  return true; }

protected:
    virtual ~CEditShapeObjectTool();
    void DeleteThis() { delete this; };

private:
    CShapeObject* m_shape;
    bool m_modifying;
    QPoint m_mouseDownPos;
    Vec3 m_pointPos;
};

class CMergeShapeObjectsTool
    : public CEditTool
{
    Q_OBJECT
public:
    CMergeShapeObjectsTool();

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    virtual void SetUserData(const char* key, void* userData);
    virtual void BeginEditParams(IEditor* ie, int flags) {};
    virtual void EndEditParams() {};
    virtual void Display(DisplayContext& dc) {};
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);

protected:
    virtual ~CMergeShapeObjectsTool();
    void DeleteThis() { delete this; };

    int m_curPoint;
    CShapeObject* m_shape;

private:
};

class CSplitShapeObjectTool
    : public CEditTool
{
    Q_OBJECT
public:
    CSplitShapeObjectTool();

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    virtual void SetUserData(const char* key, void* userData);
    virtual void BeginEditParams(IEditor* ie, int flags) {};
    virtual void EndEditParams() {};
    virtual void Display(DisplayContext& dc) {};
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);

protected:
    virtual ~CSplitShapeObjectTool();
    void DeleteThis() { delete this; };

private:
    CShapeObject* m_shape;
    int m_curPoint;
};

class ShapeEditSplitPanel
{
public:
    void Initialize(QCheckBox* useTransformGizmoCheck, QEditorToolButton* editButton, QEditorToolButton* splitButton, QLabel* numberOfPointsLabel);

    virtual void SetShape(CShapeObject* shape);

    CShapeObject* GetShape() const { return m_shape; }

protected:
    QCheckBox* m_useTransformGizmoCheck;
    QEditorToolButton* m_editButton;
    QEditorToolButton* m_splitButton;
    QLabel* m_numberOfPointsLabel;

    CShapeObject* m_shape;
};

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
// CShapePanel dialog
class SANDBOX_API CShapePanel
    : public QWidget
    , public ShapeEditSplitPanel
    , public IPickObjectCallback
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    Q_OBJECT

public:
    CShapePanel(QWidget* parent = nullptr);     // standard constructor
    virtual ~CShapePanel();

    void SetShape(CShapeObject* shape) override;

protected:
    void OnBnClickedSelect();
    void OnBnClickedRemove();
    void OnBnClickedReverse();
    void OnBnClickedReset();

    void OnEntitySelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    // Ovverriden from IPickObjectCallback
    virtual void OnPick(CBaseObject* picked);
    virtual bool OnPickFilter(CBaseObject* picked);
    virtual void OnCancelPick();

    void ReloadEntities();

    QScopedPointer<Ui::ShapePanel> m_ui;

    QStringListModel* m_model;
};

//////////////////////////////////////////////////////////////////////////////
//
// CShapeMultySelPanel dialog
//////////////////////////////////////////////////////////////////////////////
class CShapeMultySelPanel
    : public QWidget
{
    Q_OBJECT

public:
    CShapeMultySelPanel(QWidget* pParent = nullptr);    // standard constructor
    virtual ~CShapeMultySelPanel();

protected:
    QScopedPointer<Ui::ShapeMultySelPanel> m_ui;
};


//////////////////////////////////////////////////////////////////////////
class CRopePanel
    : public QWidget
    , public ShapeEditSplitPanel
{
    Q_OBJECT

public:
    CRopePanel(QWidget* parent = nullptr);

protected:
    QScopedPointer<Ui::RopePanel> m_ui;
};


//////////////////////////////////////////////////////////////////////////
class CAITerritoryPanel
    : public CEntityPanel
{
    Q_OBJECT
public:
    CAITerritoryPanel(QWidget* pParent = nullptr);   // standard constructor

    void UpdateAssignedAIsPanel();

protected slots:
    void OnSelectAssignedAIs();
};


//////////////////////////////////////////////////////////////////////////
class CNavigationAreaPanel
    : public QWidget
    , public ShapeEditSplitPanel
{
    Q_OBJECT

public:
    CNavigationAreaPanel(QWidget* parent = nullptr);     // standard constructor

protected:
    QScopedPointer<Ui::NavigationAreaPanel> m_ui;
};

#endif // CRYINCLUDE_EDITOR_SHAPEPANEL_H
