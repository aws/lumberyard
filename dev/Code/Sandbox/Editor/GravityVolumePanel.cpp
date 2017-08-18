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

#include "StdAfx.h"
#include "GravityVolumePanel.h"
#include <ui_GravityVolumePanel.h>

#include "Viewport.h"
#include "Objects/GravityVolumeObject.h"

//////////////////////////////////////////////////////////////////////////
class CEditGravityVolumeObjectTool
    : public CEditTool
{
    Q_OBJECT
public:
    CEditGravityVolumeObjectTool();

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    virtual void SetUserData(const char* key, void* userData);

    virtual void BeginEditParams(IEditor* ie, int flags) {};
    virtual void EndEditParams() {};

    virtual void Display(DisplayContext& dc) {};
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) { return false; };

protected:
    virtual ~CEditGravityVolumeObjectTool();
    // Delete itself.
    void DeleteThis() { delete this; };

private:
    CGravityVolumeObject* m_GravityVolume;
    int m_currPoint;
    bool m_modifying;
    QPoint m_mouseDownPos;
    Vec3 m_pointPos;
};

//////////////////////////////////////////////////////////////////////////
CEditGravityVolumeObjectTool::CEditGravityVolumeObjectTool()
{
    m_GravityVolume = 0;
    m_currPoint = -1;
    m_modifying = false;
}

//////////////////////////////////////////////////////////////////////////
void CEditGravityVolumeObjectTool::SetUserData(const char* key, void* userData)
{
    m_GravityVolume = (CGravityVolumeObject*)userData;
    assert(m_GravityVolume != 0);

    // Modify GravityVolume undo.
    if (!CUndo::IsRecording())
    {
        CUndo("Modify GravityVolume");
        m_GravityVolume->StoreUndo("GravityVolume Modify");
    }

    m_GravityVolume->SelectPoint(-1);
}

//////////////////////////////////////////////////////////////////////////
CEditGravityVolumeObjectTool::~CEditGravityVolumeObjectTool()
{
    if (m_GravityVolume)
    {
        m_GravityVolume->SelectPoint(-1);
    }
    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->CancelUndo();
    }
}

bool CEditGravityVolumeObjectTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        GetIEditor()->SetEditTool(0);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEditGravityVolumeObjectTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (!m_GravityVolume)
    {
        return false;
    }

    if (event == eMouseLDown)
    {
        m_mouseDownPos = point;
    }

    if (event == eMouseLDown || event == eMouseMove || event == eMouseLDblClick || event == eMouseLUp)
    {
        const Matrix34& GravityVolumeTM = m_GravityVolume->GetWorldTM();

        float dist;

        Vec3 raySrc, rayDir;
        view->ViewToWorldRay(point, raySrc, rayDir);

        // Find closest point on the GravityVolume.
        int p1, p2;
        Vec3 intPnt;
        m_GravityVolume->GetNearestEdge(raySrc, rayDir, p1, p2, dist, intPnt);

        float fGravityVolumeCloseDistance = GravityVolume_CLOSE_DISTANCE * view->GetScreenScaleFactor(intPnt) * 0.01f;


        if ((flags & MK_CONTROL) && !m_modifying)
        {
            // If control we are editing edges..
            if (p1 >= 0 && p2 >= 0 && dist < fGravityVolumeCloseDistance + view->GetSelectionTolerance())
            {
                // Cursor near one of edited GravityVolume edges.
                view->ResetCursor();
                if (event == eMouseLDown)
                {
                    view->CaptureMouse();
                    m_modifying = true;
                    GetIEditor()->BeginUndo();
                    if (GetIEditor()->IsUndoRecording())
                    {
                        m_GravityVolume->StoreUndo("Make Point");
                    }

                    // If last edge, insert at end.
                    if (p2 == 0)
                    {
                        p2 = -1;
                    }

                    // Create new point between nearest edge.
                    // Put intPnt into local space of GravityVolume.
                    intPnt = GravityVolumeTM.GetInverted().TransformPoint(intPnt);

                    int index = m_GravityVolume->InsertPoint(p2, intPnt);
                    m_GravityVolume->SelectPoint(index);

                    // Set construction plane for view.
                    m_pointPos = GravityVolumeTM.TransformPoint(m_GravityVolume->GetPoint(index));
                    Matrix34 tm;
                    tm.SetIdentity();
                    tm.SetTranslation(m_pointPos);
                    view->SetConstructionMatrix(COORDS_LOCAL, tm);
                }
            }
            return true;
        }

        int index = m_GravityVolume->GetNearestPoint(raySrc, rayDir, dist);
        if (index >= 0 && dist < fGravityVolumeCloseDistance + view->GetSelectionTolerance())
        {
            // Cursor near one of edited GravityVolume points.
            view->ResetCursor();
            if (event == eMouseLDown)
            {
                if (!m_modifying)
                {
                    m_GravityVolume->SelectPoint(index);
                    m_modifying = true;
                    view->CaptureMouse();
                    GetIEditor()->BeginUndo();

                    // Set construction plance for view.
                    m_pointPos = GravityVolumeTM.TransformPoint(m_GravityVolume->GetPoint(index));
                    Matrix34 tm;
                    tm.SetIdentity();
                    tm.SetTranslation(m_pointPos);
                    view->SetConstructionMatrix(COORDS_LOCAL, tm);
                }
            }

            //GetNearestEdge

            if (event == eMouseLDblClick)
            {
                m_modifying = false;
                m_GravityVolume->RemovePoint(index);
                m_GravityVolume->SelectPoint(-1);
            }
        }
        else
        {
            if (event == eMouseLDown)
            {
                m_GravityVolume->SelectPoint(-1);
            }
        }

        if (m_modifying && event == eMouseLUp)
        {
            // Accept changes.
            m_modifying = false;
            //m_GravityVolume->SelectPoint( -1 );
            view->ReleaseMouse();
            m_GravityVolume->CalcBBox();

            if (GetIEditor()->IsUndoRecording())
            {
                GetIEditor()->AcceptUndo("GravityVolume Modify");
            }
        }

        if (m_modifying && event == eMouseMove)
        {
            // Move selected point point.
            Vec3 p1 = view->MapViewToCP(m_mouseDownPos);
            Vec3 p2 = view->MapViewToCP(point);
            Vec3 v = view->GetCPVector(p1, p2);

            if (m_GravityVolume->GetSelectedPoint() >= 0)
            {
                Vec3 wp = m_pointPos;
                Vec3 newp = wp + v;
                if (GetIEditor()->GetAxisConstrains() == AXIS_TERRAIN)
                {
                    // Keep height.
                    newp = view->MapViewToCP(point);
                    //float z = wp.z - GetIEditor()->GetTerrainElevation(wp.x,wp.y);
                    //newp.z = GetIEditor()->GetTerrainElevation(newp.x,newp.y) + z;
                    //newp.z = GetIEditor()->GetTerrainElevation(newp.x,newp.y) + GravityVolume_Z_OFFSET;
                    newp.z += GravityVolume_Z_OFFSET;
                }

                if (newp.x != 0 && newp.y != 0 && newp.z != 0)
                {
                    newp = view->SnapToGrid(newp);

                    // Put newp into local space of GravityVolume.
                    Matrix34 invGravityVolumeTM = GravityVolumeTM;
                    invGravityVolumeTM.Invert();
                    newp = invGravityVolumeTM.TransformPoint(newp);

                    if (GetIEditor()->IsUndoRecording())
                    {
                        m_GravityVolume->StoreUndo("Move Point");
                    }
                    m_GravityVolume->SetPoint(m_GravityVolume->GetSelectedPoint(), newp);
                }
            }
        }
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// CGravityVolumePanel dialog

//////////////////////////////////////////////////////////////////////////
CGravityVolumePanel::CGravityVolumePanel(QWidget* pParent /* = nullptr */)
    : QWidget(pParent)
    , ui(new Ui::CGravityVolumePanel)
{
    ui->setupUi(this);
}

CGravityVolumePanel::~CGravityVolumePanel()
{
}

void CGravityVolumePanel::SetGravityVolume(CGravityVolumeObject* GravityVolume)
{
    assert(GravityVolume);
    m_GravityVolume = GravityVolume;

    if (GravityVolume->GetPointCount() > 1)
    {
        ui->EDIT_SHAPE->SetToolClass(&CEditGravityVolumeObjectTool::staticMetaObject, "object", m_GravityVolume);
    }
    else
    {
        ui->EDIT_SHAPE->setEnabled(false);
    }

    ui->NUM_POINTS_LABEL->setText(tr("Num Points: %1").arg(GravityVolume->GetPointCount()));
}

//////////////////////////////////////////////////////////////////////////
void CGravityVolumePanel::SelectPoint(int index)
{
    if (index < 0)
    {
        ui->SELECTED_POINT->setText(tr("Selected Point: no selection"));
    }
    else
    {
        ui->SELECTED_POINT->setText(tr("Selected Point: %1").arg(index + 1));
    }
}

#include <GravityVolumePanel.moc>