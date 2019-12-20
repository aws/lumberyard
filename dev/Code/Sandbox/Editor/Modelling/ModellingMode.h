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

// Description : Modelling mode edit tool allows editing of geometry.


#ifndef CRYINCLUDE_EDITOR_MODELLING_MODELLINGMODE_H
#define CRYINCLUDE_EDITOR_MODELLING_MODELLINGMODE_H
#pragma once

// {6113E29E-613D-414c-9FB9-581EA46D1F2C}
DEFINE_GUID(MODELLING_MODE_GUID, 0x6113e29e, 0x613d, 0x414c, 0x9f, 0xb9, 0x58, 0x1e, 0xa4, 0x6d, 0x1f, 0x2c);

#include "Objects/SubObjSelection.h"

class CBaseObject;

/*!
*   CVertexMode is an abstract base class for All Editing   Tools supported by Editor.
*   Edit tools handle specific editing modes in viewports.
*/
class CModellingModeTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CModellingModeTool();

    static const GUID& GetClassID() { return MODELLING_MODE_GUID; }

    // Registration function.
    static void RegisterTool(CRegistrationContext& rc);

    //////////////////////////////////////////////////////////////////////////
    // CEditTool implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams();
    virtual void Display(struct DisplayContext& dc);

    virtual bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual bool OnSetCursor(CViewport* vp) { return false; };

    virtual void OnManipulatorDrag(CViewport* view, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const Vec3& value) override;

    bool IsNeedMoveTool() { return true; };

    static void PySetSelectionType(const char* modeName);
    static const char* Debug_GetSelectionType();

protected:
    enum ECommandMode
    {
        NothingMode = 0,
        ScrollZoomMode,
        SelectMode,
        MoveMode,
        RotateMode,
        ScaleMode,
        ScrollMode,
        ZoomMode,
    };

    virtual ~CModellingModeTool();
    bool OnLButtonDown(CViewport* view, int nFlags, const QPoint& point);
    bool OnLButtonDblClk(CViewport* view, int nFlags, const QPoint& point);
    bool OnLButtonUp(CViewport* view, int nFlags, const QPoint& point);
    bool OnMouseMove(CViewport* view, int nFlags, const QPoint& point);
    void SetCommandMode(ECommandMode mode) { m_commandMode = mode; }
    ECommandMode GetCommandMode() const { return m_commandMode; }

    //! Ctrl-Click in move mode to move selected objects to given pos.
    void MoveSelectionToPos(CViewport* view, Vec3& pos);
    void SetObjectCursor(CViewport* view, CBaseObject* hitObj, bool bChangeNow = false);

    virtual void DeleteThis() { delete this; };

    using CEditTool::HitTest;
    bool HitTest(CViewport* view, const QPoint& point, const QRect& rc, int nFlags);
    void SetSelectType(ESubObjElementType type);

protected:
    static void Command_ModellingMode();

    void UpdateSelectionGizmo();
    bool GetSelectionReferenceFrame(Matrix34& refFrame);

    //////////////////////////////////////////////////////////////////////////
    QPoint m_cMouseDownPos;
    ECommandMode m_commandMode;

    CBaseObject* m_pMouseOverObject;
    _smart_ptr<CBaseObject> m_pHighlightedObject;

    // Selected objects.
    typedef std::vector<_smart_ptr<CBaseObject> > SelectedObjectList;
    SelectedObjectList m_selectedObjects;
    static ESubObjElementType m_currSelectionType;

    std::vector<Vec3> m_xformedVertces;

    int m_selectionTypePanelId;
    int m_selectionPanelId;
    int m_displayPanelId;

    static CModellingModeTool* m_pModellingModeTool;
};

namespace AzToolsFramework
{
    //! A component to reflect scriptable commands for the Editor
    class ModellingModeFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(ModellingModeFuncsHandler, "{5A0FBA2C-0918-4897-A2B8-23DCAF883E61}")

        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

} // namespace AzToolsFramework

#endif // CRYINCLUDE_EDITOR_MODELLING_MODELLINGMODE_H
