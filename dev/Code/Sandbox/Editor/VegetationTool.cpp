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


#include "StdAfx.h"
#include "VegetationTool.h"
#include "Viewport.h"
#include "VegetationPanel.h"
#include "Terrain/Heightmap.h"
#include "VegetationMap.h"
#include "VegetationObject.h"
#include "Objects/DisplayContext.h"
#include "Objects/BaseObject.h"
#include "PanelPreview.h"
#include "Material/Material.h"
#include "Include/ITransformManipulator.h"
#include "QtUI/WaitCursor.h"
#include "I3DEngine.h"
#include "IPhysics.h"
#include "MainWindow.h"

#include "Util/BoostPythonHelpers.h"

#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include <InitGuid.h>
#endif

#include <QInputDialog>
#include <AzCore/Casting/numeric_cast.h>

#include "QtUtil.h"

#include <QMessageBox>

// {D25B8229-7FE7-45d4-8AC5-CD6DA1365879}
DEFINE_GUID(VEGETATION_TOOL_GUID, 0xd25b8229, 0x7fe7, 0x45d4, 0x8a, 0xc5, 0xcd, 0x6d, 0xa1, 0x36, 0x58, 0x79);

REGISTER_PYTHON_COMMAND(CVegetationTool::Command_Activate, terrain, activate_vegetation_tool, "Activates the vegetation tool.");

float CVegetationTool::m_brushRadius = 1;

//////////////////////////////////////////////////////////////////////////
CVegetationTool::CVegetationTool()
{
    m_pClassDesc = GetIEditor()->GetClassFactory()->FindClass(VEGETATION_TOOL_GUID);
    m_panelId = 0;
    m_panel = 0;
    m_panelPreview = 0;
    m_panelPreviewId = 0;

    m_pointerPos(0, 0, 0);
    m_mouseOverPaintableSurface = false;

    m_vegetationMap = GetIEditor()->GetVegetationMap();

    GetIEditor()->ClearSelection();

    m_bPaintingMode = false;
    m_bPlaceMode = true;

    m_opMode = OPMODE_NONE;

    m_isAffectedByBrushes = false;
    m_instanceLimitMessageActive = false;

    SetStatusText("Click to Place or Remove Vegetation");
}

//////////////////////////////////////////////////////////////////////////
CVegetationTool::~CVegetationTool()
{
    m_pointerPos(0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::BeginEditParams(IEditor* ie, int flags)
{
    if (!m_panelId)
    {
        WaitCursor wait;
        m_panel = new CVegetationPanel(this);
        m_panelId = GetIEditor()->AddRollUpPage(ROLLUP_TERRAIN, "Vegetation", m_panel);

        if (gSettings.bPreviewGeometryWindow)
        {
            m_panelPreview = new CPanelPreview;
            m_panelPreviewId = GetIEditor()->AddRollUpPage(ROLLUP_TERRAIN, "Object Preview", m_panelPreview);
            m_panel->SetPreviewPanel(m_panelPreview);
        }

        MainWindow::instance()->setFocus();

        SetModified(false, AABB());
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::EndEditParams()
{
    ClearThingSelection();
    if (m_panelPreviewId)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_TERRAIN, m_panelPreviewId);
        m_panelPreviewId = 0;
        m_panelPreview = 0;
    }
    if (m_panelId)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_TERRAIN, m_panelId);
        m_panel = 0;
        m_panelId = 0;
    }
    GetIEditor()->SetStatusText("Ready");
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::RefreshPanel(bool isReload)
{
    if (m_panel)
    {
        if (isReload)
        {
            m_panel->ReloadObjects();
        }
        else
        {
            m_panel->UpdateAllObjectsInTree();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// Specific mouse events handlers.
bool CVegetationTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    m_mouseDownPos = point;


    m_opMode = OPMODE_NONE;
    if (m_bPaintingMode)
    {
        m_opMode = OPMODE_PAINT;

        if (nFlags & MK_CONTROL)
        {
            m_bPlaceMode = false;
        }
        else
        {
            m_bPlaceMode = true;
        }

        GetIEditor()->BeginUndo();
        view->CaptureMouse();
        // If the paint fails we need to handle the button release ourselves because
        // the failure message box will steal the mouse events
        if (!PaintBrush())
        {
            OnLButtonUp(view, nFlags, point);
        }
    }
    else
    {
        Matrix34 tm;
        tm.SetIdentity();
        tm.SetTranslation(view->ViewToWorld(point));
        view->SetConstructionMatrix(COORDS_LOCAL, tm);

        m_opMode = OPMODE_SELECT;

        if (nFlags & MK_SHIFT)
        {
            PlaceThing();
            m_opMode = OPMODE_MOVE;
            GetIEditor()->BeginUndo();
        }
        else
        {
            bool bCtrl = nFlags & MK_CONTROL;
            bool bAlt = Qt::AltModifier & QApplication::queryKeyboardModifiers();
            bool bModifySelection = bCtrl || bAlt;

            CVegetationInstance* pObj = SelectThingAtPoint(view, point, true);
            if (pObj)
            {
                if (!pObj->m_boIsSelected && !bModifySelection)
                {
                    ClearThingSelection();
                }

                SelectThing(pObj, true, true);

                if (bAlt && bCtrl)
                {
                    m_opMode = OPMODE_ROTATE;
                }
                else if (bAlt)
                {
                    m_opMode = OPMODE_SCALE;
                }
                else if (GetIEditor()->GetEditMode() == eEditModeMove)
                {
                    m_opMode = OPMODE_MOVE;
                }

                GetIEditor()->BeginUndo();
            }
            else if (!bModifySelection)
            {
                ClearThingSelection();
            }
        }
        view->CaptureMouse();
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationTool::IsThingSelected(CVegetationInstance* pObj)
{
    if (!pObj)
    {
        return false;
    }

    for (int i = 0; i < m_selectedThings.size(); i++)
    {
        if (pObj == m_selectedThings[i].pInstance)
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (GetIEditor()->IsUndoRecording())
    {
        if (m_opMode == OPMODE_MOVE)
        {
            GetIEditor()->AcceptUndo("Vegetation Move");
        }
        else if (m_opMode == OPMODE_SCALE)
        {
            GetIEditor()->AcceptUndo("Vegetation Scale");
        }
        else if (m_opMode == OPMODE_ROTATE)
        {
            GetIEditor()->AcceptUndo("Vegetation Rotate");
        }
        else if (m_opMode == OPMODE_PAINT)
        {
            GetIEditor()->AcceptUndo("Vegetation Paint");
        }
        else
        {
            GetIEditor()->CancelUndo();
        }
    }
    if (m_opMode == OPMODE_SELECT)
    {
        QRect rect = view->GetSelectionRectangle();
        if (!rect.isEmpty())
        {
            SelectThingsInRect(view, rect, true);
        }
    }
    m_opMode = OPMODE_NONE;

    view->ReleaseMouse();

    // Reset selected region.
    view->ResetSelectionRegion();
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_opMode == OPMODE_NONE)
    {
        m_bPlaceMode = !(nFlags & MK_CONTROL);
    }
    else if (m_opMode == OPMODE_PAINT)
    {
        if (nFlags & MK_LBUTTON)
        {
            PaintBrush();
        }
    }
    else if (m_opMode == OPMODE_SELECT)
    {
        // Define selection.
        view->SetSelectionRectangle(QRect(m_mouseDownPos, point));
        QRect rect = view->GetSelectionRectangle();

        if (!rect.isEmpty())
        {
            if (!(nFlags & MK_CONTROL))
            {
                ClearThingSelection();
            }
            SelectThingsInRect(view, rect, true);
        }
    }
    else if (m_opMode == OPMODE_MOVE)
    {
        GetIEditor()->RestoreUndo();
        // Define selection.
        int axis = GetIEditor()->GetAxisConstrains();
        Vec3 p1 = view->MapViewToCP(m_mouseDownPos);
        Vec3 p2 = view->MapViewToCP(point);
        Vec3 v = view->GetCPVector(p1, p2);
        MoveSelected(view, v, (axis == AXIS_TERRAIN));
    }
    else if (m_opMode == OPMODE_SCALE)
    {
        GetIEditor()->RestoreUndo();
        // Define selection.
        int y = m_mouseDownPos.y() - point.y();
        if (y != 0)
        {
            float fScale = 1.0f + (float)y / 100.0f;
            ScaleSelected(fScale);
        }
    }
    else if (m_opMode == OPMODE_ROTATE)
    {
        GetIEditor()->RestoreUndo();
        // Define selection.
        int y = m_mouseDownPos.y() - point.y();
        if (y != 0)
        {
            const float fRadianStepSizePerPixel = 1.0f / 255.0f;
            RotateSelected(Vec3(0, 0, fRadianStepSizePerPixel * y));
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    m_isAffectedByBrushes = false;
    GetSelectedObjects(m_selectedObjects);
    if (!m_selectedObjects.empty() && m_selectedObjects[0]->IsAffectedByBrushes())
    {
        m_isAffectedByBrushes = true;
    }

    bool mouseCollidingWithTerrain = false;
    bool mouseCollidingWithObject = false;

    m_pointerPos = view->ViewToWorld(point, &mouseCollidingWithTerrain, !m_isAffectedByBrushes, m_isAffectedByBrushes, false /* collideWithRenderMesh */, &mouseCollidingWithObject);
    m_mousePos = point;

    m_mouseOverPaintableSurface = mouseCollidingWithTerrain || mouseCollidingWithObject;

    bool bProcessed = false;
    if (event == eMouseLDown)
    {
        bProcessed = OnLButtonDown(view, flags, point);
    }
    else if (event == eMouseLUp)
    {
        bProcessed = OnLButtonUp(view, flags, point);
    }
    else if (event == eMouseMove)
    {
        bProcessed = OnMouseMove(view, flags, point);
    }

    GetIEditor()->SetMarkerPosition(m_pointerPos);

    if (flags & MK_CONTROL)
    {
        //swap x/y
        int unitSize = GetIEditor()->GetHeightmap()->GetUnitSize();
        float slope = GetIEditor()->GetHeightmap()->GetSlope(m_pointerPos.y / unitSize, m_pointerPos.x / unitSize);
        char szNewStatusText[512];
        sprintf_s(szNewStatusText, "Slope: %g", slope);
        GetIEditor()->SetStatusText(szNewStatusText);
    }
    else
    {
#ifdef AZ_PLATFORM_APPLE
        GetIEditor()->SetStatusText("Shift: Place New  ⌘: Add To Selection  ⌥: Scale Selected  ⌥⌘: Rotate Selected DEL: Delete Selected");
#else
        GetIEditor()->SetStatusText("Shift: Place New  Ctrl: Add To Selection  Alt: Scale Selected  Alt+Ctrl: Rotate Selected DEL: Delete Selected");
#endif
    }

    m_prevMousePos = point;

    // Not processed.
    return bProcessed;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::Display(DisplayContext& dc)
{
    if (GetIEditor()->GetActiveView()->GetAdvancedSelectModeFlag())
    {
        // TODO: optimize, instead of rendering ALL vegetation objects
        // one should render only the visible ones.
        std::vector<CVegetationInstance*> instances;
        m_vegetationMap->GetAllInstances(instances);

        for (int i = 0; i < instances.size(); ++i)
        {
            CVegetationInstance*&   poVegetationInstance = instances[i];

            if (!poVegetationInstance)
            {
                continue;
            }

            const CVegetationObject* pVegetationObject = poVegetationInstance->object;

            if (pVegetationObject && pVegetationObject->IsHidden())
            {
                continue;
            }

            Vec3 wp = poVegetationInstance->pos;

            dc.SetColor(QColor(255, 255, 255));

            if (poVegetationInstance->m_boIsSelected)
            {
                dc.SetColor(dc.GetSelectedColor());
            }
            uint32 nPrevState = dc.GetState();
            dc.DepthTestOff();
            float r = dc.view->GetScreenScaleFactor(wp) * 0.006f;
            dc.DrawWireBox(wp - Vec3(r, r, r), wp + Vec3(r, r, r));
            dc.SetState(nPrevState);
        }
    }


    if (!m_bPaintingMode)
    {
        if (dc.flags & DISPLAY_2D)
        {
            return;
        }

        // Single object 3D display mode.
        for (int i = 0; i < m_selectedThings.size(); i++)
        {
            CVegetationInstance*    rpoCurrentSelectedInstance = m_selectedThings[i].pInstance;
            // The object may become invalid if the vegetation object was destroyed.
            if (!rpoCurrentSelectedInstance->object)
            {
                continue;
            }

            const CVegetationObject* pVegetationObject = rpoCurrentSelectedInstance->object;

            if (pVegetationObject && pVegetationObject->IsHidden())
            {
                continue;
            }

            float radius = rpoCurrentSelectedInstance->object->GetObjectSize() * rpoCurrentSelectedInstance->scale;
            if (m_selectedThings[i].transform_failed)
            {
                dc.SetColor(1, 0, 0, 1); // red
            }
            else
            {
                dc.SetColor(0, 1, 0, 1); // green
            }

            dc.DrawTerrainCircle(rpoCurrentSelectedInstance->pos, radius / 2.0f, 0.1f);
            dc.SetColor(0, 0, 1.0f, 0.7f);
            dc.DrawTerrainCircle(rpoCurrentSelectedInstance->pos, 10.0f, 0.1f);
            float aiRadius = rpoCurrentSelectedInstance->object->GetAIRadius() * rpoCurrentSelectedInstance->scale;
            if (aiRadius > 0.0f)
            {
                dc.SetColor(1, 0, 1.0f, 0.7f);
                dc.DrawTerrainCircle(rpoCurrentSelectedInstance->pos, aiRadius, 0.1f);
            }
        }
    }
    else if (m_mouseOverPaintableSurface)
    {
        // Brush paint mode.

        if (dc.flags & DISPLAY_2D)
        {
            QPoint p1 = dc.view->WorldToView(m_pointerPos);
            QPoint p2 = dc.view->WorldToView(m_pointerPos + Vec3(m_brushRadius, 0, 0));
            float radius = sqrtf((p2.x() - p1.x()) * (p2.x() - p1.x()) + (p2.y() - p1.y()) * (p2.y() - p1.y()));
            dc.SetColor(0, 1, 0, 1);
            dc.DrawWireCircle2d(p1, radius, 0);
            return;
        }

        dc.SetColor(0, 1, 0, 1);

        if (m_isAffectedByBrushes)
        {
            dc.DrawCircle(m_pointerPos, m_brushRadius);
            dc.SetColor(0, 0, 1, 1);
            Vec3 posUpper = m_pointerPos + Vec3(0, 0, 0.1f + m_brushRadius / 2);
            dc.DrawCircle(posUpper, m_brushRadius);
            dc.DrawLine(m_pointerPos, posUpper);
        }
        else
        {
            dc.DrawTerrainCircle(m_pointerPos, m_brushRadius, 0.2f);
        }

        float col[4] = { 1, 1, 1, 0.8f };
        if (m_bPlaceMode)
        {
            dc.renderer->DrawLabelEx(m_pointerPos + Vec3(0, 0, 1), 1.0f, col, true, true, "Place");
        }
        else
        {
            dc.renderer->DrawLabelEx(m_pointerPos + Vec3(0, 0, 1), 1.0f, col, true, true, "Remove");
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    bool bProcessed = false;
    float brushRadius = m_brushRadius;
    if (nChar == VK_CONTROL && m_opMode == OPMODE_NONE)
    {
        m_bPlaceMode = false;
    }

    if (nChar == VK_OEM_6)
    {
        if (brushRadius < 300)
        {
            brushRadius += 1;
        }
        bProcessed = true;
    }
    if (nChar == VK_OEM_4)
    {
        if (brushRadius > 1)
        {
            brushRadius -= 1;
        }
        bProcessed = true;
    }
    if (nChar == VK_DELETE)
    {
        CUndo undo("Vegetation Delete");
        if (!m_selectedThings.empty())
        {
            if (QMessageBox::question(QApplication::activeWindow(), QString(), "Delete Selected Instances?") == QMessageBox::Yes)
            {
                AABB box;
                box.Reset();

                m_vegetationMap->StoreBaseUndo();
                for (auto& selectedItem : m_selectedThings)
                {
                    box.Add(selectedItem.pInstance->pos);
                    m_vegetationMap->DeleteObjInstance(selectedItem.pInstance);
                }

                ClearThingSelection(false);

                if (m_panel)
                {
                    m_panel->UpdateAllObjectsInTree();
                }


                SetModified(true, box);

                m_vegetationMap->StoreBaseUndo();

                // force shadowmap to be recalculated when vegetation is removed
                auto shadowCacheCVar = gEnv->pConsole->GetCVar("e_ShadowsCacheUpdate");
                if (shadowCacheCVar != nullptr)
                {
                    // if e_ShadowsCacheUpdate is set to 1, the shadowmap will be recalculated
                    // it'll be set back to 0 once the recomputations have taken place.
                    shadowCacheCVar->Set(1);
                }
            }
        }
        bProcessed = true;
    }

    if (nChar == VK_ESCAPE)
    {
        if (m_bPaintingMode)
        {
            SetPaintingMode(false);
            bProcessed = true;
        }
        if (m_selectedThings.size() > 0)
        {
            int num_selected = m_selectedThings.size();
            // de-select all the things that were successfully transformed (ie. moved) ... but
            ClearThingSelection(false, true);
            if (num_selected ==  m_selectedThings.size())
            {
                // ... if all were unsuccessful, de-select them all
                ClearThingSelection(false, false);
            }
            bProcessed = true;
        }
    }
    else
    if (bProcessed && m_panel)
    {
        SetBrushRadius(brushRadius);
    }
    return bProcessed;
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationTool::OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_CONTROL && m_opMode == OPMODE_NONE)
    {
        m_bPlaceMode = true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::GetSelectedObjects(std::vector<CVegetationObject*>& objects)
{
    objects.clear();
    for (int i = 0; i < m_vegetationMap->GetObjectCount(); i++)
    {
        CVegetationObject* pObj = m_vegetationMap->GetObject(i);
        if (pObj->IsSelected())
        {
            objects.push_back(pObj);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationTool::PaintBrush()
{
    if (!m_mouseOverPaintableSurface)
    {
        return true;
    }

    GetSelectedObjects(m_selectedObjects);

    QRect rc(QPoint(m_pointerPos.x - m_brushRadius, m_pointerPos.y - m_brushRadius), 
             QSize(m_brushRadius * 2, m_brushRadius * 2));

    AABB updateRegion;
    updateRegion.min = m_pointerPos - Vec3(m_brushRadius, m_brushRadius, m_brushRadius);
    updateRegion.max = m_pointerPos + Vec3(m_brushRadius, m_brushRadius, m_brushRadius);
    bool success = true;
    if (m_bPlaceMode)
    {
        for (int i = 0; i < m_selectedObjects.size(); i++)
        {
            int numInstances = m_selectedObjects[i]->GetNumInstances();
            success = m_vegetationMap->PaintBrush(rc, true, m_selectedObjects[i], &m_pointerPos);
            // If number of instances changed.
            if (numInstances != m_selectedObjects[i]->GetNumInstances() && m_panel)
            {
                m_panel->UpdateObjectInTree(m_selectedObjects[i]);
            }

            if (!success)
            {
                OnPaintBrushFailed();
                break;
            }
        }
    }
    else
    {
        m_vegetationMap->StoreBaseUndo(CVegetationMap::eStoreUndo_Begin);
        for (int i = 0; i < m_selectedObjects.size(); i++)
        {
            int numInstances = m_selectedObjects[i]->GetNumInstances();
            m_vegetationMap->ClearBrush(rc, true, m_selectedObjects[i]);
            // If number of instances changed.
            if (numInstances != m_selectedObjects[i]->GetNumInstances() && m_panel)
            {
                m_panel->UpdateObjectInTree(m_selectedObjects[i]);
            }
        }
        m_vegetationMap->StoreBaseUndo(CVegetationMap::eStoreUndo_End);
    }

    SetModified(true, updateRegion);

    return success;
}

void CVegetationTool::OnPaintBrushFailed()
{
    // Bail out if we already have a dialog active (e.g. the dialog was thrown on mouse
    // click, but this will get triggered if you drag the mouse before the dialog
    // has been exec'd)
    if (m_instanceLimitMessageActive)
    {
        return;
    }

    // Display error modal dialog if we reached the limit of vegetation instances
    // Need to delay the exec so that we don't block the mouse callback, since it
    // comes from the RenderViewport that will start assert'ing since the PostWidgetRendering
    // call won't be invoked at the proper time
    QTimer::singleShot(0, this, [this] {
        m_instanceLimitMessageActive = true;
        QMessageBox* box = new QMessageBox(AzToolsFramework::GetActiveWindow());
        box->setWindowTitle(QObject::tr("Limit reached"));
        box->setText(QObject::tr("The maximum limit of vegetation instances has been reached."));
        box->setIcon(QMessageBox::Critical);
        QObject::connect(box, &QMessageBox::finished, this, [this, box]() {
            m_instanceLimitMessageActive = false;
            box->deleteLater();
        });
        box->exec();
    });
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::PlaceThing()
{
    GetSelectedObjects(m_selectedObjects);

    CUndo undo("Vegetation Place");
    if (!m_selectedObjects.empty())
    {
        CVegetationInstance* thing = m_vegetationMap->PlaceObjectInstance(m_pointerPos, m_selectedObjects[0]);
        // If number of instances changed.
        if (thing)
        {
            ClearThingSelection();
            SelectThing(thing);
            if (m_panel)
            {
                m_panel->UpdateObjectInTree(m_selectedObjects[0]);
            }

            GetIEditor()->ShowTransformManipulator(true);
            UpdateTransformManipulator();

            AABB updateRegion;
            updateRegion.min = m_pointerPos - Vec3(1, 1, 1);
            updateRegion.max = m_pointerPos + Vec3(1, 1, 1);
            SetModified(true, updateRegion);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::Distribute()
{
    GetSelectedObjects(m_selectedObjects);

    for (int i = 0; i < m_selectedObjects.size(); i++)
    {
        int numInstances = m_selectedObjects[i]->GetNumInstances();

        QRect rc(0, 0, m_vegetationMap->GetSize(), m_vegetationMap->GetSize());
        bool success = m_vegetationMap->PaintBrush(rc, false, m_selectedObjects[i]);

        if (numInstances != m_selectedObjects[i]->GetNumInstances() && m_panel)
        {
            m_panel->UpdateObjectInTree(m_selectedObjects[i]);
        }

        if (!success)
        {
            OnPaintBrushFailed();
            break;
        }
    }
    if (!m_selectedObjects.empty())
    {
        SetModified();
    }

    ClearThingSelection(false);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::Clear()
{
    GetSelectedObjects(m_selectedObjects);

    if (m_selectedObjects.empty())
    {
        return;
    }

    for (int i = 0; i < m_selectedObjects.size(); i++)
    {
        int numInstances = m_selectedObjects[i]->GetNumInstances();

        QRect rc(0, 0, m_vegetationMap->GetSize(), m_vegetationMap->GetSize());
        m_vegetationMap->ClearBrush(rc, false, m_selectedObjects[i]);

        if (numInstances != m_selectedObjects[i]->GetNumInstances() && m_panel)
        {
            m_panel->UpdateObjectInTree(m_selectedObjects[i]);
        }
    }

    SetModified();
    ClearThingSelection(false);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::DoRandomRotate()
{
    GetSelectedObjects(m_selectedObjects);
    for (int i = 0; i < m_selectedObjects.size(); i++)
    {
        m_vegetationMap->RandomRotateInstances(m_selectedObjects[i]);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::DoClearRotate()
{
    GetSelectedObjects(m_selectedObjects);
    for (int i = 0; i < m_selectedObjects.size(); i++)
    {
        m_vegetationMap->ClearRotateInstances(m_selectedObjects[i]);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::HideSelectedObjects(bool bHide)
{
    GetSelectedObjects(m_selectedObjects);

    for (int i = 0; i < m_selectedObjects.size(); i++)
    {
        m_vegetationMap->HideObject(m_selectedObjects[i], bHide);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::RemoveSelectedObjects()
{
    GetSelectedObjects(m_selectedObjects);

    GetIEditor()->BeginUndo();

    for (int i = 0; i < m_selectedObjects.size(); i++)
    {
        int numInstances = m_selectedObjects[i]->GetNumInstances();
        m_vegetationMap->RemoveObject(m_selectedObjects[i]);

        if (numInstances != m_selectedObjects[i]->GetNumInstances() && m_panel)
        {
            m_panel->UpdateObjectInTree(m_selectedObjects[i]);
        }
    }
    GetIEditor()->AcceptUndo("Remove Brush");

    if (!m_selectedObjects.empty())
    {
        SetModified();
    }

    ClearThingSelection();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::SetPaintingMode(bool bPaintingMode)
{
    ClearThingSelection(false);
    m_bPaintingMode = bPaintingMode;
    if (m_panel)
    {
        m_panel->UpdateUI();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::ClearThingSelection(bool bUnselectGroups, bool bLeaveFailedTransformSelected)
{
    for (int i = 0; i < m_selectedThings.size(); i++)
    {
        if (!bLeaveFailedTransformSelected || !m_selectedThings[i].transform_failed)
        {
            CVegetationInstance* thing = m_selectedThings[i].pInstance;
            if (thing->pRenderNode)
            {
                thing->pRenderNode->SetRndFlags(ERF_SELECTED, false);
            }
            thing->m_boIsSelected = false;
        }
    }

    if (bLeaveFailedTransformSelected)
    {
        m_selectedThings.erase(std::remove_if(m_selectedThings.begin(), m_selectedThings.end(), [](const SelectedThing& vegetation) { return !vegetation.transform_failed; }),
            m_selectedThings.end());
        UpdateTransformManipulator();
    }
    else
    {
        m_selectedThings.clear();
        GetIEditor()->ShowTransformManipulator(false);
    }

    if (bUnselectGroups && m_panel)
    {
        m_panel->UnselectAll();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::SelectThing(CVegetationInstance* thing, bool bSelObject, bool bShowManipulator)
{
    // If already selected or hidden
    if (thing->m_boIsSelected || thing->object->IsHidden())
    {
        return;
    }

    if (thing->pRenderNode)
    {
        thing->pRenderNode->SetRndFlags(ERF_SELECTED, gSettings.viewports.bHighlightSelectedVegetation);
    }

    thing->m_boIsSelected = true;
    m_selectedThings.push_back(SelectedThing(thing));

    if (m_panel && bSelObject)
    {
        if (!thing->object->IsSelected())
        {
            m_panel->SelectObject(thing->object, (m_selectedThings.size() > 1));
        }
    }

    float fAngle = 0;
    float fScale = 1.0f;

    int numThings = (int)m_selectedThings.size();
    Vec3 pos(0, 0, 0);
    for (int i = 0; i < numThings; i++)
    {
        pos += m_selectedThings[i].pInstance->pos;
    }

    if (numThings > 0)
    {
        pos = pos / numThings;
    }

    if (numThings == 1)
    {
        fAngle = (m_selectedThings[0].pInstance->angle / 255.0f) * g_PI;
        fScale = m_selectedThings[0].pInstance->scale;
    }

    if (bShowManipulator)
    {
        GetIEditor()->ShowTransformManipulator(true);
        UpdateTransformManipulator();
    }
}

//////////////////////////////////////////////////////////////////////////
CVegetationInstance* CVegetationTool::SelectThingAtPoint(CViewport* view, const QPoint& point, bool bNotSelect, bool bShowManipulator)
{
    Vec3 raySrc, rayDir;
    view->ViewToWorldRay(point, raySrc, rayDir);

    if (view->GetAdvancedSelectModeFlag())
    {
        std::vector<CVegetationInstance*> instances;
        m_vegetationMap->GetAllInstances(instances);

        int   nMinimumDistanceIndex(-1);
        float fCurrentSquareDistance(FLT_MAX);
        float fNextSquareDistance(0);
        CVegetationInstance*    poSelectedInstance(NULL);

        for (int i = 0; i < instances.size(); ++i)
        {
            // Hit test helper.
            CVegetationInstance*&   rpoCurrentInstance = instances[i];

            Vec3 origin = rpoCurrentInstance->pos;

            Vec3 w = origin - raySrc;
            w = rayDir.Cross(w);
            float d = w.GetLengthSquared();

            float radius = view->GetScreenScaleFactor(origin) * 0.008f;

            if (d < radius * radius)
            {
                fNextSquareDistance = raySrc.GetDistance(origin);
                if (fCurrentSquareDistance > fNextSquareDistance)
                {
                    fCurrentSquareDistance = fNextSquareDistance;
                    poSelectedInstance = rpoCurrentInstance;
                }
            }
        }
        if (poSelectedInstance != NULL)
        {
            if (!bNotSelect)
            {
                SelectThing(poSelectedInstance, true, bShowManipulator);
            }
            return poSelectedInstance;
        }
        else
        {
            return NULL;
        }
    }



    bool bCollideTerrain = false;
    Vec3 pos = view->ViewToWorld(point, &bCollideTerrain, true);
    float fTerrainHitDistance = raySrc.GetDistance(pos);

    IPhysicalWorld* pPhysics = GetIEditor()->GetSystem()->GetIPhysicalWorld();
    if (pPhysics)
    {
        int objTypes = ent_static;
        int flags = rwi_stop_at_pierceable | rwi_ignore_terrain_holes;
        ray_hit hit;
        int col = pPhysics->RayWorldIntersection(raySrc, rayDir * 1000.0f, objTypes, flags, &hit, 1);
        if (hit.dist > 0 && !hit.bTerrain && hit.dist < fTerrainHitDistance)
        {
            pe_status_pos statusPos;
            hit.pCollider->GetStatus(&statusPos);
            if (!statusPos.pos.IsZero(0.1f))
            {
                pos = statusPos.pos;
            }
        }
    }

    // Find closest thing to this point.
    CVegetationInstance* obj = m_vegetationMap->GetNearestInstance(pos, 1.0f);
    if (obj)
    {
        if (!bNotSelect)
        {
            SelectThing(obj, true, bShowManipulator);
        }
        return obj;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::MoveSelected(CViewport* view, const Vec3& offset, bool bFollowTerrain)
{
    if (m_selectedThings.empty())
    {
        return;
    }
    AABB box;
    box.Reset();
    Vec3 newPos, oldPos;
    for (int i = 0; i < m_selectedThings.size(); i++)
    {
        oldPos = m_selectedThings[i].pInstance->pos;

        box.Add(oldPos);

        newPos = oldPos + offset;
        newPos = view->SnapToGrid(newPos);

        const CVegetationObject* pObject = m_selectedThings[i].pInstance->object;

        if (pObject && pObject->IsAffectedByBrushes())
        {
            float ofz = 1.0f + (abs(offset.x) + abs(offset.y)) / 2;
            if (ofz > 10.0f)
            {
                ofz = 10.0f;
            }
            Vec3 posUpper = newPos + Vec3(0, 0, ofz);
            newPos.z = m_vegetationMap->CalcHeightOnBrushes(newPos, posUpper);
        }
        else if (pObject->IsAffectedByTerrain())
        {
            // Stick to terrain.
            if (I3DEngine* engine = GetIEditor()->GetSystem()->GetI3DEngine())
            {
                newPos.z = engine->GetTerrainElevation(newPos.x, newPos.y);
            }
        }

        if (!m_vegetationMap->MoveInstance(m_selectedThings[i].pInstance, newPos))
        {
            m_selectedThings[i].transform_failed = true;
        }
        else
        {
            m_selectedThings[i].transform_failed = false;
        }
        box.Add(newPos);
    }

    UpdateTransformManipulator();

    box.min = box.min - Vec3(1, 1, 1);
    box.max = box.max + Vec3(1, 1, 1);
    SetModified(true, box);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::ScaleSelected(float fScale)
{
    if (fScale <= 0.01f)
    {
        return;
    }

    if (!m_selectedThings.empty())
    {
        AABB box;
        box.Reset();
        int numThings = m_selectedThings.size();
        for (int i = 0; i < numThings; i++)
        {
            m_vegetationMap->RecordUndo(m_selectedThings[i].pInstance);
            m_selectedThings[i].pInstance->scale *= fScale;

            // Force this object to be placed on terrain again.
            m_vegetationMap->MoveInstance(m_selectedThings[i].pInstance, m_selectedThings[i].pInstance->pos);

            box.Add(m_selectedThings[i].pInstance->pos);
        }
        SetModified(true, box);
    }
}


//////////////////////////////////////////////////////////////////////////
void CVegetationTool::RotateSelected(const Vec3& rot)
{
    if (!m_selectedThings.empty())
    {
        AABB box;
        box.Reset();
        int numThings = m_selectedThings.size();
        for (int i = 0; i < numThings; i++)
        {
            CVegetationInstance& thing = *m_selectedThings[i].pInstance;
            m_vegetationMap->RecordUndo(&thing);

            Matrix34 tm;
            CVegetationObject* pObj = thing.object;
            if (pObj->IsAlignToTerrain())
            {
                Matrix34 origTm;
                origTm.SetRotationZ(rot.z);
                tm.SetRotationZ(thing.angle);
                tm = origTm * tm;
            }
            else
            {
                Matrix34 origTm = Matrix34::CreateRotationXYZ(Ang3(thing.angleX, thing.angleY, thing.angle));
                tm = Matrix34::CreateRotationXYZ(Ang3(rot));
                tm = origTm * tm;
            }

            Ang3 ang(tm);
            thing.angleX = ang.x;
            thing.angleY = ang.y;
            thing.angle  = ang.z;

            m_vegetationMap->MoveInstance(&thing, thing.pos);

            box.Add(thing.pos);
        }
        SetModified(true, box);

        UpdateTransformManipulator();
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::ScaleObjects()
{
    float fScale = 1;
    if (m_selectedThings.size() == 1)
    {
        fScale = m_selectedThings[0].pInstance->scale;
    }

    bool ok = false;
    int fractionalDigitCount = 5;
    fScale = aznumeric_caster(QInputDialog::getDouble(nullptr, QObject::tr("Scale Selected Object(s)"), QStringLiteral(""), fScale, 0.01, std::numeric_limits<float>::max(), fractionalDigitCount, &ok));
    if (!ok)
    {
        return;
    }

    // dirty bounding box
    AABB modifiedBox;
    modifiedBox.Reset();

    if (!m_selectedThings.empty())
    {
        int numThings = m_selectedThings.size();
        for (int i = 0; i < numThings; i++)
        {
            CVegetationInstance* thing = m_selectedThings[i].pInstance;
            m_vegetationMap->RecordUndo(thing);
            if (numThings > 1)
            {
                thing->scale *= fScale;
            }
            else
            {
                thing->scale = fScale;
            }

            // invalidate the area occupied by this instance
            IRenderNode* pRenderNode = thing->pRenderNode;
            if (pRenderNode != NULL)
            {
                modifiedBox.Add(pRenderNode->GetBBox());
            }

            // Move it ( this also forces object to be placed on terrain again )
            m_vegetationMap->MoveInstance(thing, thing->pos);

            // invalidate the area occupied by this instance
            pRenderNode = m_selectedThings[i].pInstance->pRenderNode;
            if (pRenderNode != NULL)
            {
                modifiedBox.Add(pRenderNode->GetBBox());
            }
        }
    }
    else
    {
        GetSelectedObjects(m_selectedObjects);
        for (int i = 0; i < m_selectedObjects.size(); i++)
        {
            AABB box;
            m_vegetationMap->ScaleObjectInstances(m_selectedObjects[i], fScale, &box);
            modifiedBox.Add(box);
        }
    }

    // mark the dirty area
    if (!modifiedBox.IsEmpty())
    {
        SetModified(true, modifiedBox);
    }
}

/////////////////////////////////////////////////////////////////////////
void CVegetationTool::SelectThingsInRect(CViewport* view, const QRect& rect, bool bShowManipulator)
{
    AABB box;
    std::vector<CVegetationInstance*> things;
    m_vegetationMap->GetAllInstances(things);
    if (things.size() > 0)
    {
        if (m_panel)
        {
            m_panel->UnselectAll();
        }
    }
    for (int i = 0; i < (int)things.size(); i++)
    {
        Vec3 pos = things[i]->pos;
        box.min.Set(pos.x - 0.1f, pos.y - 0.1f, pos.z - 0.1f);
        box.max.Set(pos.x + 0.1f, pos.y + 0.1f, pos.z + 0.1f);
        if (!view->IsBoundsVisible(box))
        {
            continue;
        }
        QPoint pnt = view->WorldToView(things[i]->pos);
        if (rect.contains(pnt))
        {
            SelectThing(things[i], true, bShowManipulator);
        }
    }
}

/////////////////////////////////////////////////////////////////////////
void CVegetationTool::SetBrushRadius(float r)
{
    if (r > 300.f)
    {
        r = 300.f;
    }
    else if (r < 1.f)
    {
        r = 1.f;
    }

    m_brushRadius = r;

    if (m_panel)
    {
        m_panel->SetBrush(r);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::Command_Activate()
{
    CEditTool* pTool = GetIEditor()->GetEditTool();
    if (pTool && qobject_cast<CVegetationTool*>(pTool))
    {
        // Already active.
        return;
    }

    GetIEditor()->SelectRollUpBar(ROLLUP_TERRAIN);

    // This needs to be done after the terrain tab is selected, because in
    // Cry-Free mode the terrain tool could be closed, whereas in legacy
    // mode the rollupbar is never deleted, it's only hidden
    pTool = new CVegetationTool;
    GetIEditor()->SetEditTool(pTool);
}

const GUID& CVegetationTool::GetClassID()
{
    return VEGETATION_TOOL_GUID;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::RegisterTool(CRegistrationContext& rc)
{
    rc.pClassFactory->RegisterClass(new CQtViewClass<CVegetationTool>("EditTool.VegetationPaint", "Terrain", ESYSTEM_CLASS_EDITTOOL));

    CommandManagerHelper::RegisterCommand(rc.pCommandManager, "edit_tool", "vegetation_activate", "", "", functor(CVegetationTool::Command_Activate));
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::OnManipulatorDrag(CViewport* view, ITransformManipulator* pManipulator, QPoint& point0, QPoint& point1, const Vec3& value)
{
    // get world/local coordinate system setting.
    RefCoordSys coordSys = GetIEditor()->GetReferenceCoordSys();
    int editMode = GetIEditor()->GetEditMode();

    // get current axis constrains.
    if (editMode == eEditModeMove)
    {
        GetIEditor()->RestoreUndo();
        MoveSelected(view, value, true);
    }
    if (editMode == eEditModeRotate)
    {
        GetIEditor()->RestoreUndo();
        RotateSelected(value);
    }
    if (editMode == eEditModeScale)
    {
        GetIEditor()->RestoreUndo();

        float fScale = max(value.x, value.y);
        fScale = max(fScale, value.z);
        ScaleSelected(fScale);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationTool::IsNeedMoveTool()
{
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::MoveSelectedInstancesToCategory(QString category)
{
    int numThings = m_selectedThings.size();
    for (int i = 0; i < numThings; i++)
    {
        CVegetationObject* object = m_selectedThings[i].pInstance->object;
        if (QString::compare(object->GetCategory(), category))
        {
            CVegetationObject* newObject = m_vegetationMap->CreateObject(object);
            if (newObject)
            {
                newObject->SetCategory(category);
                for (int j = i; j < numThings; j++)
                {
                    if (m_selectedThings[j].pInstance->object == object)
                    {
                        object->SetNumInstances(object->GetNumInstances() - 1);
                        m_selectedThings[j].pInstance->object = newObject;
                        newObject->SetNumInstances(newObject->GetNumInstances() + 1);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationTool::SetModified(bool bWithBounds, const AABB& bounds)
{
    if (bWithBounds)
    {
        GetIEditor()->UpdateViews(eUpdateStatObj, &bounds);
    }
    else
    {
        GetIEditor()->UpdateViews(eUpdateStatObj);
    }

    AABB box = bounds;
}


//////////////////////////////////////////////////////////////////////////
void CVegetationTool::UpdateTransformManipulator()
{
    if (m_selectedThings.empty())
    {
        return;
    }

    Matrix34A tm;
    tm.SetIdentity();

    if (m_selectedThings.size() == 1)
    {
        CVegetationInstance* thing = m_selectedThings[0].pInstance;

        CVegetationObject* pObj = thing->object;
        if (pObj->IsAlignToTerrain())
        {
            tm.SetRotationZ(thing->angle);
        }
        else
        {
            tm = Matrix34::CreateRotationXYZ(Ang3(thing->angleX, thing->angleY, thing->angle));
        }
        tm.SetTranslation(thing->pos);
    }
    else
    {
        Vec3 pos(0, 0, 0);
        for (int i = 0; i < m_selectedThings.size(); ++i)
        {
            pos += m_selectedThings[i].pInstance->pos;
        }
        tm.SetTranslation(pos / m_selectedThings.size());
    }

    ITransformManipulator* pManip = GetIEditor()->GetTransformManipulator();
    if (pManip)
    {
        pManip->SetTransformation(COORDS_LOCAL, tm);
        pManip->SetTransformation(COORDS_PARENT, tm);
        pManip->SetTransformation(COORDS_USERDEFINED, tm);
    }
}

#include <VegetationTool.moc>