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
#include "SplinePanel.h"

#include "Viewport.h"
#include "Objects/SplineObject.h"
#include "Include/ITransformManipulator.h"
#include "MeasurementSystem/MeasurementSystem.h"


//////////////////////////////////////////////////////////////////////////
void CEditSplineObjectTool::SetUserData(const char* key, void* userData)
{
    m_pSpline = (CSplineObject*)userData;
    assert(m_pSpline != 0);

    m_pSpline->SetEditMode(true);

    // Modify Spline undo.
    if (!CUndo::IsRecording())
    {
        CUndo ("Modify Spline");
        m_pSpline->StoreUndo("Spline Modify");
    }

    if (GetIEditor()->GetEditMode() == eEditModeSelect)
    {
        GetIEditor()->SetEditMode(eEditModeMove);
    }

    SelectPoint(-1);
}


//////////////////////////////////////////////////////////////////////////
CEditSplineObjectTool::~CEditSplineObjectTool()
{
    if (m_pSpline)
    {
        m_pSpline->SetEditMode(false);
        SelectPoint(-1);
    }
    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->CancelUndo();
    }
}


//////////////////////////////////////////////////////////////////////////
bool CEditSplineObjectTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        GetIEditor()->SetEditTool(0);
    }
    else if (nChar == VK_DELETE)
    {
        int index = m_pSpline->GetSelectedPoint();
        if (index >= 0)
        {
            m_pSpline->SelectPoint(-1);
            m_pSpline->RemovePoint(index);
        }
    }
    return true;
}


//////////////////////////////////////////////////////////////////////////
void CEditSplineObjectTool::SelectPoint(int index)
{
    if (index < 0)
    {
        SetCursor(STD_CURSOR_DEFAULT, true);
    }

    if (!m_pSpline)
    {
        return;
    }

    m_pSpline->SelectPoint(index);
}


//////////////////////////////////////////////////////////////////////////
void CEditSplineObjectTool::OnManipulatorDrag(CViewport* pView, ITransformManipulator* pManipulator, QPoint& point0, QPoint& point1, const Vec3& value)
{
    // get world/local coordinate system setting.
    RefCoordSys coordSys = GetIEditor()->GetReferenceCoordSys();
    int editMode = GetIEditor()->GetEditMode();

    // get current axis constrains.
    if (editMode == eEditModeMove)
    {
        m_modifying = true;
        GetIEditor()->RestoreUndo();
        const Matrix34& splineTM = m_pSpline->GetWorldTM();
        Matrix34 invSplineTM = splineTM;
        invSplineTM.Invert();

        int index = m_pSpline->GetSelectedPoint();
        Vec3 pos = m_pSpline->GetPoint(index);

        Vec3 wp = splineTM.TransformPoint(pos);
        Vec3 newPos = wp + value;

        if ((Qt::ControlModifier & QApplication::queryKeyboardModifiers()) == 0 && pView->GetAxisConstrain() == AXIS_TERRAIN)
        {
            float height = wp.z - GetIEditor()->GetTerrainElevation(wp.x, wp.y);
            newPos.z = GetIEditor()->GetTerrainElevation(newPos.x, newPos.y) + height;
        }

        if (GetIEditor()->IsUndoRecording())
        {
            m_pSpline->StoreUndo("Move Point");
        }

        newPos = invSplineTM.TransformPoint(newPos);
        m_pSpline->SetPoint(m_pSpline->GetSelectedPoint(), newPos);
    }
}


//////////////////////////////////////////////////////////////////////////
bool CEditSplineObjectTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (!m_pSpline)
    {
        return false;
    }

    if (event == eMouseLDown)
    {
        m_mouseDownPos = point;
    }

    if (event == eMouseLDown || event == eMouseMove || event == eMouseLDblClick || event == eMouseLUp)
    {
        const Matrix34& splineTM = m_pSpline->GetWorldTM();

        Vec3 raySrc, rayDir;
        view->ViewToWorldRay(point, raySrc, rayDir);

        // Find closest point on the spline.
        int p1, p2;
        float dist;
        Vec3 intPnt(0, 0, 0);
        m_pSpline->GetNearestEdge(raySrc, rayDir, p1, p2, dist, intPnt);

        float fSplineCloseDistance = kSplinePointSelectionRadius * view->GetScreenScaleFactor(intPnt) * kClickDistanceScaleFactor;

        if ((flags & MK_CONTROL) && !m_modifying)
        {
            // If control we are editing edges..
            if (p1 >= 0 && p2 >= 0 && dist < fSplineCloseDistance + view->GetSelectionTolerance())
            {
                // Cursor near one of edited Spline edges.
                view->ResetCursor();
                if (event == eMouseLDown)
                {
                    view->CaptureMouse();
                    m_modifying = true;
                    GetIEditor()->BeginUndo();
                    if (GetIEditor()->IsUndoRecording())
                    {
                        m_pSpline->StoreUndo("Make Point");
                    }

                    // If last edge, insert at end.
                    if (p2 == 0)
                    {
                        p2 = -1;
                    }

                    // Create new point between nearest edge.
                    // Put intPnt into local space of Spline.
                    intPnt = splineTM.GetInverted().TransformPoint(intPnt);

                    int index = m_pSpline->InsertPoint(p2, intPnt);
                    SelectPoint(index);

                    // Set construction plane for view.
                    m_pointPos = splineTM.TransformPoint(m_pSpline->GetPoint(index));
                    Matrix34 tm;
                    tm.SetIdentity();
                    tm.SetTranslation(m_pointPos);
                    view->SetConstructionMatrix(COORDS_LOCAL, tm);
                }
            }
            return true;
        }

        int index = m_pSpline->GetNearestPoint(raySrc, rayDir, dist);
        if (index >= 0 && dist < fSplineCloseDistance + view->GetSelectionTolerance())
        {
            // Cursor near one of edited Spline points.
            SetCursor(GetIEditor()->GetEditMode() ? STD_CURSOR_MOVE : STD_CURSOR_HIT);

            if (event == eMouseLDown)
            {
                if (!m_modifying)
                {
                    if (CMeasurementSystem::GetMeasurementSystem().ProcessedLButtonClick(index))
                    {
                        return false;
                    }

                    SelectPoint(index);

                    GetIEditor()->BeginUndo();

                    ITransformManipulator* pManip = GetIEditor()->GetTransformManipulator();
                    if (pManip)
                    {
                        pManip->MouseCallback(view, event, point, flags);
                    }

                    // Set construction plance for view.
                    m_pointPos = splineTM.TransformPoint(m_pSpline->GetPoint(index));
                    Matrix34 tm;
                    tm.SetIdentity();
                    tm.SetTranslation(m_pointPos);
                    view->SetConstructionMatrix(COORDS_LOCAL, tm);
                }
            }

            if (event == eMouseLDblClick)
            {
                if (CMeasurementSystem::GetMeasurementSystem().ProcessedDblButtonClick(m_pSpline->GetPointCount()))
                {
                    return false;
                }

                m_modifying = false;
                m_pSpline->RemovePoint(index);
                SelectPoint(-1);
            }
        }
        else
        {
            SetCursor(STD_CURSOR_DEFAULT);

            if (event == eMouseLDown)
            {
                SelectPoint(-1);
            }
        }

        if (m_modifying && event == eMouseLUp)
        {
            // Accept changes.
            m_modifying = false;
            view->ReleaseMouse();
            m_pSpline->CalcBBox(); // TODO: Avoid and make CalcBBox() private
            m_pSpline->OnUpdate();

            if (GetIEditor()->IsUndoRecording())
            {
                GetIEditor()->AcceptUndo("Spline Modify");
            }
        }

        return true;
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////
void CEditSplineObjectTool::SetCursor(EStdCursor cursor, bool bForce)
{
    CViewport* pViewport = GetIEditor()->GetActiveView();
    if ((m_curCursor != cursor || bForce) && pViewport)
    {
        m_curCursor = cursor;
        pViewport->SetCurrentCursor(m_curCursor);
    }
}

//////////////////////////////////////////////////////////////////////////
CSplitSplineObjectTool::CSplitSplineObjectTool()
{
    m_pSpline = 0;
    m_curPoint = -1;
}

//////////////////////////////////////////////////////////////////////////
void CSplitSplineObjectTool::SetUserData(const char* key, void* userData)
{
    m_curPoint = -1;
    m_pSpline = (CSplineObject*)userData;
    assert(m_pSpline != 0);

    // Modify Spline undo.
    if (!CUndo::IsRecording())
    {
        CUndo("Modify Spline");
        m_pSpline->StoreUndo("Spline Modify");
    }
}

//////////////////////////////////////////////////////////////////////////
CSplitSplineObjectTool::~CSplitSplineObjectTool()
{
    //if (m_pSpline)
    //m_pSpline->SetSplitPoint( -1,Vec3(0,0,0), -1 );
    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->CancelUndo();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CSplitSplineObjectTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        GetIEditor()->SetEditTool(0);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CSplitSplineObjectTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (!m_pSpline)
    {
        return false;
    }

    if (event == eMouseLDown || event == eMouseMove)
    {
        const Matrix34& shapeTM = m_pSpline->GetWorldTM();

        float dist;
        Vec3 raySrc, rayDir;
        view->ViewToWorldRay(point, raySrc, rayDir);

        // Find closest point on the shape.
        int p1, p2;
        Vec3 intPnt;
        m_pSpline->GetNearestEdge(raySrc, rayDir, p1, p2, dist, intPnt);

        float fShapeCloseDistance = kSplinePointSelectionRadius * view->GetScreenScaleFactor(intPnt) * kClickDistanceScaleFactor;

        // If control we are editing edges..
        if (p1 >= 0 && p2 > 0 && dist < fShapeCloseDistance + view->GetSelectionTolerance())
        {
            view->SetCurrentCursor(STD_CURSOR_HIT);
            // Put intPnt into local space of shape.
            intPnt = shapeTM.GetInverted().TransformPoint(intPnt);

            if (event == eMouseLDown)
            {
                GetIEditor()->BeginUndo();
                m_pSpline->Split(p2, intPnt);
                if (GetIEditor()->IsUndoRecording())
                {
                    GetIEditor()->AcceptUndo("Split Spline");
                }
                GetIEditor()->SetEditTool(0);
            }
        }
        else
        {
            view->ResetCursor();
        }

        return true;
    }
    return false;
}

CInsertSplineObjectTool::CInsertSplineObjectTool()
{
}

CInsertSplineObjectTool::~CInsertSplineObjectTool()
{
}

bool CInsertSplineObjectTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        GetIEditor()->SetEditTool(0);
    }
    return true;
}

bool CInsertSplineObjectTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (!m_pSpline || event != eMouseLDown)
    {
        return false;
    }

    Vec3 raySrc, rayDir;
    view->ViewToWorldRay(point, raySrc, rayDir);

    // Find closest point on the spline.
    int p1;
    int p2;
    float distance;
    Vec3 insertPosition(0, 0, 0);
    m_pSpline->GetNearestEdge(raySrc, rayDir, p1, p2, distance, insertPosition);

    float fSplineCloseDistance = kSplinePointSelectionRadius * view->GetScreenScaleFactor(insertPosition) * kClickDistanceScaleFactor;

    // If control we are editing edges..
    if (p1 >= 0 && p2 >= 0 && distance < fSplineCloseDistance + view->GetSelectionTolerance())
    {
        GetIEditor()->BeginUndo();

        // If last edge, insert at end.
        if (p2 == 0)
        {
            p2 = -1;
        }
        // Create new point between nearest edge & put insertPosition into local space of Spline.
        const Matrix34& splineTM = m_pSpline->GetWorldTM();
        insertPosition = splineTM.GetInverted().TransformPoint(insertPosition);

        int index = m_pSpline->InsertPoint(p2, insertPosition);

        SelectPoint(index);

        // Set construction plane for view.
        m_pointPos = splineTM.TransformPoint(m_pSpline->GetPoint(index));
        Matrix34 tm;
        tm.SetIdentity();
        tm.SetTranslation(m_pointPos);
        view->SetConstructionMatrix(COORDS_LOCAL, tm);

        if (GetIEditor()->IsUndoRecording())
        {
            GetIEditor()->AcceptUndo("Spline Insert Point");
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
CMergeSplineObjectsTool::CMergeSplineObjectsTool()
{
    m_curPoint = -1;
    m_pSpline = 0;
}

//////////////////////////////////////////////////////////////////////////
void CMergeSplineObjectsTool::SetUserData(const char* key, void* userData)
{
    m_pSpline = (CSplineObject*)userData;
    assert(m_pSpline != 0);

    // Modify Spline undo.
    if (!CUndo::IsRecording())
    {
        CUndo("Spline Merging");
        m_pSpline->StoreUndo("Spline Merging");
    }

    m_pSpline->SelectPoint(-1);
}

//////////////////////////////////////////////////////////////////////////
CMergeSplineObjectsTool::~CMergeSplineObjectsTool()
{
    if (m_pSpline)
    {
        m_pSpline->SetMergeIndex(-1);
    }
    m_pSpline = 0;
    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->CancelUndo();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CMergeSplineObjectsTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        GetIEditor()->SetEditTool(0);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CMergeSplineObjectsTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    //return true;
    if (event == eMouseLDown || event == eMouseMove)
    {
        HitContext hc;
        hc.view = view;
        hc.point2d = point;
        view->ViewToWorldRay(point, hc.raySrc, hc.rayDir);
        if (!GetIEditor()->GetObjectManager()->HitTest(hc))
        {
            return false;
        }

        CBaseObject* pObj = hc.object;

        if (!qobject_cast<CSplineObject*>(pObj))
        {
            return false;
        }

        CSplineObject* pSpline = (CSplineObject*) pObj;

        if (!(m_curPoint == -1 && pSpline == m_pSpline || m_curPoint != -1 && pSpline != m_pSpline))
        {
            return false;
        }

        const Matrix34& splineTM = pSpline->GetWorldTM();

        float dist;
        Vec3 raySrc, rayDir;
        view->ViewToWorldRay(point, raySrc, rayDir);

        // Find closest point on the Spline.
        int p1, p2;
        Vec3 intPnt;
        pSpline->GetNearestEdge(raySrc, rayDir, p1, p2, dist, intPnt);

        if (p1 < 0 || p2 <= 0)
        {
            return false;
        }

        float fShapeCloseDistance = kSplinePointSelectionRadius * view->GetScreenScaleFactor(intPnt) * kClickDistanceScaleFactor;

        if (dist > fShapeCloseDistance + view->GetSelectionTolerance())
        {
            return false;
        }

        int cnt = pSpline->GetPointCount();
        if (cnt < 2)
        {
            return false;
        }

        int p = pSpline->GetNearestPoint(raySrc, rayDir, dist);
        if (p != 0 && p != cnt - 1)
        {
            return false;
        }

        Vec3 pnt = splineTM.TransformPoint(pSpline->GetPoint(p));

        if (intPnt.GetDistance(pnt) > fShapeCloseDistance + view->GetSelectionTolerance())
        {
            return false;
        }

        view->SetCurrentCursor(STD_CURSOR_HIT);

        if (event == eMouseLDown)
        {
            if (m_curPoint == -1)
            {
                m_curPoint = p;
                m_pSpline->SetMergeIndex(p);
            }
            else
            {
                GetIEditor()->BeginUndo();
                if (m_pSpline)
                {
                    pSpline->SetMergeIndex(p);
                    m_pSpline->Merge(pSpline);
                }
                if (GetIEditor()->IsUndoRecording())
                {
                    GetIEditor()->AcceptUndo("Spline Merging");
                }
                GetIEditor()->SetEditTool(0);
            }
        }

        return true;
    }

    return false;
}





//////////////////////////////////////////////////////////////////////////
// CSplinePanel

#include <ui_SplinePanel.h>

//////////////////////////////////////////////////////////////////////////
CSplinePanel::CSplinePanel(QWidget* pParent /* = nullptr */)
    : QWidget(pParent)
    , m_ui(new Ui::SplinePanel)
    , m_pSpline(nullptr)
{
    OnInitDialog();
}

CSplinePanel::~CSplinePanel()
{
    CMeasurementSystem::GetMeasurementSystem().ShutdownMeasurementSystem();
}

// CSplinePanel message handlers

void CSplinePanel::OnInitDialog()
{
    m_ui->setupUi(this);

    auto valueChanged = static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

    connect(m_ui->angleSpin, valueChanged, [&](double value) { m_pSpline->SetPointAngle(value); });
    connect(m_ui->widthSpin, valueChanged, [&](double value) { m_pSpline->SetPointWidth(value); });
    connect(m_ui->defaultWidthCheck, &QCheckBox::stateChanged, this, &CSplinePanel::OnDefaultWidth);
    connect(m_ui->editSplineButton, &QEditorToolButton::clicked, []()
        {
		CMeasurementSystem::GetMeasurementSystem().ShutdownMeasurementSystem();
	} );
}

void CSplinePanel::SetSpline(CSplineObject* pSpline)
{
    assert(pSpline);
    m_pSpline = pSpline;
    m_ui->angleSpin->setRange(-m_pSpline->GetAngleRange(), m_pSpline->GetAngleRange());

    Update();
}


//////////////////////////////////////////////////////////////////////////
void CSplinePanel::Update()
{
    if (m_pSpline->GetPointCount() > 1)
    {
        m_ui->editSplineButton->setEnabled(true);
        m_ui->editSplineButton->SetToolClass(&CEditSplineObjectTool::staticMetaObject, "object", m_pSpline);
    }
    else
    {
        m_ui->editSplineButton->setEnabled(false);
    }

    m_ui->numberOfPointsLabel->setText(tr("Num Points: %1").arg(m_pSpline->GetPointCount()));

    if (m_pSpline->GetPointCount() >= 2)
    {
        m_ui->splitButton->SetToolClass(&CSplitSplineObjectTool::staticMetaObject, "object", m_pSpline);
        m_ui->splitButton->setEnabled(true);

        m_ui->insertButton->SetToolClass(&CInsertSplineObjectTool::staticMetaObject, "object", m_pSpline);
        m_ui->insertButton->setEnabled(true);

        m_ui->mergeButton->SetToolClass(&CMergeSplineObjectsTool::staticMetaObject, "object", m_pSpline);
        m_ui->mergeButton->setEnabled(true);
    }
    else
    {
        m_ui->splitButton->setEnabled(false);
        m_ui->insertButton->setEnabled(false);
        m_ui->mergeButton->setEnabled(false);
    }

    int index = m_pSpline->GetSelectedPoint();

    if (index < 0)
    {
        m_ui->angleSpin->setEnabled(false);
        m_ui->widthSpin->setEnabled(false);
        m_ui->defaultWidthCheck->setEnabled(false);
        m_ui->selectedPointLabel->setText(tr("Selected Point: no selection"));
    }
    else
    {
        m_ui->angleSpin->setEnabled(true);
        float val = m_pSpline->GetPointAngle();
        m_ui->angleSpin->setValue(val);

        m_ui->defaultWidthCheck->setEnabled(true);

        bool isDefault = m_pSpline->IsPointDefaultWidth();
        m_ui->defaultWidthCheck->setChecked(isDefault);
        m_ui->widthSpin->setEnabled(!isDefault);

        m_ui->selectedPointLabel->setText(tr("Selected Point: %1").arg(index + 1));
    }

    m_ui->widthSpin->setValue(m_pSpline->GetPointWidth());
}

void CSplinePanel::OnDefaultWidth(int state)
{
    m_ui->widthSpin->setEnabled(state == Qt::Unchecked);
    m_pSpline->PointDafaultWidthIs(state == Qt::Checked);
    m_ui->widthSpin->setValue(m_pSpline->GetPointWidth());
}

#include <SplinePanel.moc>
