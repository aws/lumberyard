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

// Description : Places vegetation on terrain.


#ifndef CRYINCLUDE_EDITOR_VEGETATIONTOOL_H
#define CRYINCLUDE_EDITOR_VEGETATIONTOOL_H

#pragma once

class CVegetationObject;
struct CVegetationInstance;
class CVegetationMap;

//////////////////////////////////////////////////////////////////////////
class CVegetationTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CVegetationTool();

    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams();
    void RefreshPanel(bool isReload);

    virtual void Display(DisplayContext& dc);

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    void OnManipulatorDrag(CViewport* view, ITransformManipulator* pManipulator, QPoint& point0, QPoint& point1, const Vec3& value) override;

    // Key down.
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);

    void SetBrushRadius(float r);
    float GetBrushRadius() const { return m_brushRadius; };

    void SetPaintingMode(bool bPaintingMode);
    bool GetPaintingMode() { return m_bPaintingMode; }

    void Distribute();
    void Clear();

    void ScaleObjects();
    void DoRandomRotate();
    void DoClearRotate();

    void HideSelectedObjects(bool bHide);
    void RemoveSelectedObjects();

    bool PaintBrush();
    void PlaceThing();
    void GetSelectedObjects(std::vector<CVegetationObject*>& objects);

    void ClearThingSelection(bool bUnselectGroups = true, bool bLeaveFailedTransformSelected = false);

    static const GUID& GetClassID();
    static void RegisterTool(CRegistrationContext& rc);

    bool IsNeedMoveTool();
    bool IsNeedToSkipPivotBoxForObjects()   {   return true;    }

    void MoveSelectedInstancesToCategory(QString category);

    int GetCountSelectedInstances() { return m_selectedThings.size(); }

    void UpdateTransformManipulator();

    static void Command_Activate();

protected:
    virtual ~CVegetationTool();
    // Delete itself.
    void DeleteThis() { delete this; };

private:
    void SelectThing(CVegetationInstance* thing, bool bSelObject = true, bool bShowManipulator = false);
    CVegetationInstance* SelectThingAtPoint(CViewport* view, const QPoint& point, bool bNotSelect = false, bool bShowManipulator = false);
    bool IsThingSelected(CVegetationInstance* pObj);
    void SelectThingsInRect(CViewport* view, const QRect& rect, bool bShowManipulator = false);
    void MoveSelected(CViewport* view, const Vec3& offset, bool bFollowTerrain);
    void ScaleSelected(float fScale);
    void RotateSelected(const Vec3& rot);

    void SetModified(bool bWithBounds = false, const AABB& bounds = AABB());

    // Specific mouse events handlers.
    bool OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point);
    bool OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point);
    bool OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point);

    void OnPaintBrushFailed();


    QPoint m_mouseDownPos;
    QPoint m_mousePos;
    QPoint m_prevMousePos;
    Vec3 m_pointerPos;
    bool m_mouseOverPaintableSurface;
    static float m_brushRadius;

    bool m_bPlaceMode;
    bool m_bPaintingMode;

    enum OpMode
    {
        OPMODE_NONE,
        OPMODE_PAINT,
        OPMODE_SELECT,
        OPMODE_MOVE,
        OPMODE_SCALE,
        OPMODE_ROTATE
    };
    OpMode m_opMode;

    std::vector<CVegetationObject*> m_selectedObjects;

    CVegetationMap* m_vegetationMap;

    //! Selected vegetation instances
    struct SelectedThing
    {
        CVegetationInstance*    pInstance;
        bool                    transform_failed;
        SelectedThing(CVegetationInstance*  instance)
            : pInstance(instance)
            , transform_failed(false) {}

    private:
        SelectedThing(){}
    };

    std::vector<SelectedThing> m_selectedThings;

    int m_panelId;
    class CVegetationPanel* m_panel;
    int m_panelPreviewId;
    class CPanelPreview* m_panelPreview;

    bool m_isAffectedByBrushes;
    bool m_instanceLimitMessageActive;
};

namespace AzToolsFramework
{
    //! A component to reflect scriptable commands for the Editor
    class VegetationToolFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(VegetationToolFuncsHandler, "{8CB91B63-3BB7-45F5-AC31-B666A962F932}")

            static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

} // namespace AzToolsFramework

#endif // CRYINCLUDE_EDITOR_VEGETATIONTOOL_H