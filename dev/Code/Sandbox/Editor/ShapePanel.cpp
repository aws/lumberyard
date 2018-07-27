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
#include "ShapePanel.h"

#include "Viewport.h"
#include "Objects/AIWave.h"
#include "Objects/ShapeObject.h"

#include <QGroupBox>
#include <QPushButton>
#include <ui_ShapePanel.h>
#include <ui_ShapeMultySelPanel.h>
#include <ui_NavigationAreaPanel.h>
#include <ui_RopePanel.h>

#include <QStringListModel>
#include <QItemSelectionModel>

//////////////////////////////////////////////////////////////////////////
CEditShapeObjectTool::CEditShapeObjectTool()
{
    m_shape = 0;
    m_modifying = false;
}

//////////////////////////////////////////////////////////////////////////
void CEditShapeObjectTool::SetUserData(const char* key, void* userData)
{
    m_shape = (CShapeObject*)userData;
    assert(m_shape != 0);

    // Modify shape undo.
    if (!CUndo::IsRecording())
    {
        CUndo("Modify Shape");
        m_shape->StoreUndo("Shape Modify");
    }

    m_shape->SelectPoint(-1);
}

//////////////////////////////////////////////////////////////////////////
CEditShapeObjectTool::~CEditShapeObjectTool()
{
    if (m_shape)
    {
        m_shape->SelectPoint(-1);
    }
    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->CancelUndo();
    }
}

bool CEditShapeObjectTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        GetIEditor()->SetEditTool(0);
    }
    else if (nChar == VK_DELETE)
    {
        if (m_shape)
        {
            int sel = m_shape->GetSelectedPoint();
            if (!m_modifying && sel >= 0 && sel < m_shape->GetPointCount())
            {
                GetIEditor()->BeginUndo();
                if (GetIEditor()->IsUndoRecording())
                {
                    m_shape->StoreUndo("Delete Point");
                }

                m_shape->RemovePoint(sel);
                m_shape->SelectPoint(-1);
                m_shape->CalcBBox();

                if (GetIEditor()->IsUndoRecording())
                {
                    GetIEditor()->AcceptUndo("Shape Modify");
                }
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEditShapeObjectTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (!m_shape)
    {
        return false;
    }

    if (event == eMouseLDown)
    {
        m_mouseDownPos = point;
    }

    if (event == eMouseLDown || event == eMouseMove || event == eMouseLDblClick || event == eMouseLUp)
    {
        const Matrix34& shapeTM = m_shape->GetWorldTM();

        /*
        float fShapeCloseDistance = SHAPE_CLOSE_DISTANCE;
        Vec3 pos = view->ViewToWorld( point );
        if (pos.x == 0 && pos.y == 0 && pos.z == 0)
        {
            // Find closest point on the shape.
            fShapeCloseDistance = SHAPE_CLOSE_DISTANCE * view->GetScreenScaleFactor(pos) * 0.01f;
        }
        else
            fShapeCloseDistance = SHAPE_CLOSE_DISTANCE * view->GetScreenScaleFactor(pos) * 0.01f;
        */


        float dist;

        Vec3 raySrc, rayDir;
        view->ViewToWorldRay(point, raySrc, rayDir);

        // Find closest point on the shape.
        int p1, p2;
        Vec3 intPnt;
        m_shape->GetNearestEdge(raySrc, rayDir, p1, p2, dist, intPnt);

        float fShapeCloseDistance = SHAPE_CLOSE_DISTANCE * view->GetScreenScaleFactor(intPnt) * 0.01f;


        if ((flags & MK_CONTROL) && !m_modifying)
        {
            // If control we are editing edges..
            if (p1 >= 0 && p2 >= 0 && dist < fShapeCloseDistance + view->GetSelectionTolerance())
            {
                // Cursor near one of edited shape edges.
                view->ResetCursor();
                if (event == eMouseLDown)
                {
                    view->CaptureMouse();
                    m_modifying = true;
                    GetIEditor()->BeginUndo();
                    if (GetIEditor()->IsUndoRecording())
                    {
                        m_shape->StoreUndo("Make Point");
                    }

                    // If last edge, insert at end.
                    if (p2 == 0)
                    {
                        p2 = -1;
                    }

                    // Create new point between nearest edge.
                    // Put intPnt into local space of shape.
                    intPnt = shapeTM.GetInverted().TransformPoint(intPnt);

                    int index = m_shape->InsertPoint(p2, intPnt, true);
                    m_shape->SelectPoint(index);

                    // Set construction plane for view.
                    m_pointPos = shapeTM.TransformPoint(m_shape->GetPoint(index));
                    Matrix34 tm;
                    tm.SetIdentity();
                    tm.SetTranslation(m_pointPos);
                    view->SetConstructionMatrix(COORDS_LOCAL, tm);
                }
            }
            return true;
        }

        int index = m_shape->GetNearestPoint(raySrc, rayDir, dist);
        if (dist > fShapeCloseDistance + view->GetSelectionTolerance())
        {
            index = -1;
        }
        bool    hitPoint(index != -1);

        // Edit the selected point based on the axis gizmo.
        if (m_shape->UseAxisHelper() && index == -1 && m_shape->GetSelectedPoint() != -1 && !m_modifying)
        {
            CAxisHelper&    axisHelper(m_shape->GetSelelectedPointAxisHelper());
            HitContext hc;
            hc.view = view;
            hc.point2d = point;
            view->ViewToWorldRay(point, hc.raySrc, hc.rayDir);

            Vec3    selectedPointPos = shapeTM.TransformPoint(m_shape->GetPoint(m_shape->GetSelectedPoint()));

            Matrix34    axis;
            axis.SetTranslationMat(selectedPointPos);
            if (axisHelper.HitTest(axis, GetIEditor()->GetGlobalGizmoParameters(), hc))
            {
                if (event == eMouseLDown)
                {
                    m_modifying = true;
                    view->CaptureMouse();
                    GetIEditor()->BeginUndo();

                    if (GetIEditor()->IsUndoRecording())
                    {
                        m_shape->StoreUndo("Move Point");
                    }

                    // Set construction plance for view.
                    m_pointPos = selectedPointPos;
                    Matrix34 tm;
                    tm.SetIdentity();
                    tm.SetTranslation(selectedPointPos);
                    view->SetConstructionMatrix(COORDS_LOCAL, tm);

                    // Hit axis gizmo.
                    GetIEditor()->SetAxisConstraints((AxisConstrains)hc.axis);
                    view->SetAxisConstrain(hc.axis);
                }
                index = m_shape->GetSelectedPoint();
                view->SetCurrentCursor(STD_CURSOR_MOVE);
            }
        }

        if (index >= 0)
        {
            // Cursor near one of edited shape points.
            if (event == eMouseLDown)
            {
                if (!m_modifying)
                {
                    m_shape->SelectPoint(index);
                    m_modifying = true;
                    view->CaptureMouse();
                    GetIEditor()->BeginUndo();

                    if (GetIEditor()->IsUndoRecording())
                    {
                        m_shape->StoreUndo("Move Point");
                    }

                    // Set construction plance for view.
                    m_pointPos = shapeTM.TransformPoint(m_shape->GetPoint(index));
                    Matrix34 tm;
                    tm.SetIdentity();
                    tm.SetTranslation(m_pointPos);
                    view->SetConstructionMatrix(COORDS_LOCAL, tm);
                }
            }

            //GetNearestEdge

            // Delete points with double click.
            // Test if the use hit the point too so that interacting with the
            // axis helper does not allow to delete points.
            if (event == eMouseLDblClick && hitPoint)
            {
                CUndo undo("Remove Point");
                m_modifying = false;
                m_shape->RemovePoint(index);
                m_shape->SelectPoint(-1);
            }

            if (hitPoint)
            {
                view->SetCurrentCursor(STD_CURSOR_HIT);
            }
        }
        else
        {
            if (event == eMouseLDown)
            {
                // Missed a point, deselect all.
                m_shape->SelectPoint(-1);
            }
            view->ResetCursor();
        }

        if (m_modifying && event == eMouseLUp)
        {
            // Accept changes.
            m_modifying = false;
            //          m_shape->SelectPoint( -1 );
            view->ReleaseMouse();
            m_shape->CalcBBox();

            if (GetIEditor()->IsUndoRecording())
            {
                GetIEditor()->AcceptUndo("Shape Modify");
            }

            m_shape->EndPointModify();
        }

        if (m_modifying && event == eMouseMove)
        {
            // Move selected point point.
            Vec3 p1 = view->MapViewToCP(m_mouseDownPos);
            Vec3 p2 = view->MapViewToCP(point);
            Vec3 v = view->GetCPVector(p1, p2);

            if (m_shape->GetSelectedPoint() >= 0)
            {
                Vec3 wp = m_pointPos;
                Vec3 newp = wp + v;
                if (GetIEditor()->GetAxisConstrains() == AXIS_TERRAIN)
                {
                    // Keep height.
                    newp = view->MapViewToCP(point);
                    //float z = wp.z - GetIEditor()->GetTerrainElevation(wp.x,wp.y);
                    //newp.z = GetIEditor()->GetTerrainElevation(newp.x,newp.y) + z;
                    //newp.z = GetIEditor()->GetTerrainElevation(newp.x,newp.y) + SHAPE_Z_OFFSET;
                    newp.z += m_shape->GetShapeZOffset();
                }

                if (newp.x != 0 && newp.y != 0 && newp.z != 0)
                {
                    newp = view->SnapToGrid(newp);

                    // Put newp into local space of shape.
                    Matrix34 invShapeTM = shapeTM;
                    invShapeTM.Invert();
                    newp = invShapeTM.TransformPoint(newp);

                    m_shape->SetPoint(m_shape->GetSelectedPoint(), newp);
                    m_shape->UpdatePrefab();
                }

                view->SetCurrentCursor(STD_CURSOR_MOVE);
            }
        }

        /*
        Vec3 raySrc,rayDir;
        view->ViewToWorldRay( point,raySrc,rayDir );
        CBaseObject *hitObj = GetIEditor()->GetObjectManager()->HitTest( raySrc,rayDir,view->GetSelectionTolerance() );
        */
        return true;
    }
    return false;
}







//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CSplitShapeObjectTool::CSplitShapeObjectTool()
{
    m_shape = 0;
    m_curPoint = -1;
}

//////////////////////////////////////////////////////////////////////////
void CSplitShapeObjectTool::SetUserData(const char* key, void* userData)
{
    m_curPoint = -1;
    m_shape = (CShapeObject*)userData;
    assert(m_shape != 0);

    // Modify shape undo.
    if (!CUndo::IsRecording())
    {
        CUndo("Modify Shape");
        m_shape->StoreUndo("Shape Modify");
    }
}

//////////////////////////////////////////////////////////////////////////
CSplitShapeObjectTool::~CSplitShapeObjectTool()
{
    if (m_shape)
    {
        m_shape->SetSplitPoint(-1, Vec3(0, 0, 0), -1);
    }
    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->CancelUndo();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CSplitShapeObjectTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        GetIEditor()->SetEditTool(0);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CSplitShapeObjectTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (!m_shape)
    {
        return false;
    }

    if (event == eMouseLDown || event == eMouseMove)
    {
        const Matrix34& shapeTM = m_shape->GetWorldTM();

        float dist;
        Vec3 raySrc, rayDir;
        view->ViewToWorldRay(point, raySrc, rayDir);

        // Find closest point on the shape.
        int p1, p2;
        Vec3 intPnt;
        m_shape->GetNearestEdge(raySrc, rayDir, p1, p2, dist, intPnt);

        float fShapeCloseDistance = SHAPE_CLOSE_DISTANCE * view->GetScreenScaleFactor(intPnt) * 0.01f;

        // If control we are editing edges..
        if (p1 >= 0 && p2 >= 0 && dist < fShapeCloseDistance + view->GetSelectionTolerance())
        {
            view->SetCurrentCursor(STD_CURSOR_HIT);
            // If last edge, insert at end.
            if (p2 == 0)
            {
                p2 = -1;
            }
            // Put intPnt into local space of shape.
            intPnt = shapeTM.GetInverted().TransformPoint(intPnt);

            if (event == eMouseMove)
            {
                if (m_curPoint == 0)
                {
                    m_shape->SetSplitPoint(p2, intPnt, 1);
                }
            }
            if (event == eMouseLDown)
            {
                Matrix34 ltm = m_shape->GetLocalTM();
                Vec3 shapePoint1 = intPnt;
                Vec3 newPos = m_shape->GetPos() + ltm.TransformVector(shapePoint1);

                m_curPoint++;
                m_curPoint = m_shape->SetSplitPoint(p2, intPnt, m_curPoint) - 1;
                if (m_curPoint == 1)
                {
                    GetIEditor()->BeginUndo();
                    m_shape->Split();
                    if (GetIEditor()->IsUndoRecording())
                    {
                        GetIEditor()->AcceptUndo("Split shape");
                    }
                    GetIEditor()->SetEditTool(0);
                }
            }
        }
        else
        {
            view->ResetCursor();
            if (m_curPoint > -1)
            {
                m_shape->SetSplitPoint(-2, Vec3(0, 0, 0), 0);
            }
        }
        return true;
    }
    return false;
}





//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////



void ShapeEditSplitPanel::Initialize(QCheckBox* useTransformGizmoCheck, QEditorToolButton* editButton, QEditorToolButton* splitButton, QLabel* numberOfPointsLabel)
{
    m_useTransformGizmoCheck = useTransformGizmoCheck;
    m_editButton = editButton;
    m_splitButton = splitButton;
    m_numberOfPointsLabel = numberOfPointsLabel;

    QSettings settings;
    useTransformGizmoCheck->setChecked(settings.value("Editor/ShapePanel_UseTransformGizmo", false).toBool());

    QObject::connect(m_useTransformGizmoCheck, &QCheckBox::toggled, m_useTransformGizmoCheck, [&](bool checked) {
        if (m_shape)
        {
            m_shape->SetUseAxisHelper(checked);
        }
    });
}

void ShapeEditSplitPanel::SetShape(CShapeObject* shape)
{
    assert(shape);
    m_shape = shape;

    if (shape->GetPointCount() > 1)
    {
        m_editButton->SetToolClass(&CEditShapeObjectTool::staticMetaObject, "object", m_shape);
        m_editButton->setEnabled(true);
    }
    else
    {
        m_editButton->setEnabled(true);
    }

    if (shape->GetPointCount() > 2)
    {
        m_splitButton->SetToolClass(&CSplitShapeObjectTool::staticMetaObject, "object", m_shape);
        m_splitButton->setEnabled(true);
    }
    else
    {
        m_splitButton->setEnabled(false);
    }

    m_numberOfPointsLabel->setText(QObject::tr("Num Points: %1").arg(shape->GetPointCount()));

    m_shape->SetUseAxisHelper(m_useTransformGizmoCheck->isChecked());
}

// CShapePanel dialog

//////////////////////////////////////////////////////////////////////////
CShapePanel::CShapePanel(QWidget* parent /* = nullptr */)
    : QWidget(parent)
    , m_ui(new Ui::ShapePanel())
    , m_model(new QStringListModel(this))
{
    m_ui->setupUi(this);
    Initialize(m_ui->useTransformGizmoCheck, m_ui->editButton, m_ui->splitButton, m_ui->numberOfPointsLabel);

    m_ui->targetEntitiesListView->setModel(m_model);

    m_ui->pickButton->SetPickCallback(this, "Pick Entity");
    connect(m_ui->reversePathButton, &QPushButton::clicked, this, &CShapePanel::OnBnClickedReverse);
    connect(m_ui->resetHeightButton, &QPushButton::clicked, this, &CShapePanel::OnBnClickedReset);
    connect(m_ui->removeButton, &QPushButton::clicked, this, &CShapePanel::OnBnClickedRemove);
    connect(m_ui->selectButton, &QPushButton::clicked, this, &CShapePanel::OnBnClickedSelect);
    connect(m_ui->targetEntitiesListView, &QListView::doubleClicked, this, &CShapePanel::OnBnClickedSelect);
}

CShapePanel::~CShapePanel()
{
    QSettings settings;
    settings.setValue("Editor/ShapePanel_UseTransformGizmo", m_ui->useTransformGizmoCheck->isChecked());
}

// CShapePanel message handlers

//////////////////////////////////////////////////////////////////////////
void CShapePanel::SetShape(CShapeObject* shape)
{
    ShapeEditSplitPanel::SetShape(shape);

    ReloadEntities();
}

//////////////////////////////////////////////////////////////////////////
void CShapePanel::OnPick(CBaseObject* picked)
{
    assert(m_shape);
    CUndo undo("[Shape] Add Entity");
    m_shape->AddEntity(picked);
    ReloadEntities();
    //  m_entityName.SetWindowText( picked->GetName() );
}

//////////////////////////////////////////////////////////////////////////
bool CShapePanel::OnPickFilter(CBaseObject* picked)
{
    assert(picked != 0);
    if (qobject_cast<CAITerritoryObject*>(m_shape))
    {
        return qobject_cast<CAIWaveObject*>(picked);
    }
    return picked->GetType() == OBJTYPE_ENTITY;
}

//////////////////////////////////////////////////////////////////////////
void CShapePanel::OnCancelPick()
{
}

//////////////////////////////////////////////////////////////////////////
void CShapePanel::OnBnClickedSelect()
{
    assert(m_shape);
    QModelIndex index = m_ui->targetEntitiesListView->currentIndex();
    if (index.isValid())
    {
        CBaseObject* obj = m_shape->GetEntity(index.row());
        if (obj)
        {
            GetIEditor()->ClearSelection();
            GetIEditor()->SelectObject(obj);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapePanel::ReloadEntities()
{
    if (!m_shape)
    {
        return;
    }

    disconnect(m_ui->targetEntitiesListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CShapePanel::OnEntitySelectionChanged);

    QStringList list;

    for (int i = 0; i < m_shape->GetEntityCount(); i++)
    {
        CBaseObject* obj = m_shape->GetEntity(i);
        if (obj)
        {
            list.append(obj->GetName());
        }
        else
        {
            list.append(QStringLiteral("<Null>"));
        }
    }

    m_model->setStringList(list);

    OnEntitySelectionChanged({}, {});
    connect(m_ui->targetEntitiesListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CShapePanel::OnEntitySelectionChanged);
}

//////////////////////////////////////////////////////////////////////////
void CShapePanel::OnBnClickedRemove()
{
    assert(m_shape);
    QModelIndex index = m_ui->targetEntitiesListView->currentIndex();
    if (index.isValid())
    {
        CUndo undo("[Shape] Remove Entity");
        m_shape->RemoveEntity(index.row());
        ReloadEntities();
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapePanel::OnBnClickedReverse()
{
    assert(m_shape);
    if (m_shape->GetPointCount() > 0)
    {
        CUndo undo("[Shape] Reverse Shape");
        m_shape->ReverseShape();
    }
}

//////////////////////////////////////////////////////////////////////////
void CShapePanel::OnBnClickedReset()
{
    assert(m_shape);
    CUndo undo("[Shape] Reset Height Shape");
    m_shape->ResetShape();
}

//////////////////////////////////////////////////////////////////////////
void CShapePanel::OnEntitySelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    bool selection = !selected.indexes().isEmpty();
    m_ui->removeButton->setEnabled(selection);
    m_ui->selectButton->setEnabled(selection);
}

//////////////////////////////////////////////////////////////////////////
CMergeShapeObjectsTool::CMergeShapeObjectsTool()
{
    m_curPoint = -1;
    m_shape = 0;
}

//////////////////////////////////////////////////////////////////////////
void CMergeShapeObjectsTool::SetUserData(const char* key, void* userData)
{
    /*
    // Modify shape undo.
    if (!CUndo::IsRecording())
    {
        CUndo ("Modify Shape");
        m_shape->StoreUndo( "Shape Modify" );
    }
    */
}

//////////////////////////////////////////////////////////////////////////
CMergeShapeObjectsTool::~CMergeShapeObjectsTool()
{
    if (m_shape)
    {
        m_shape->SetMergeIndex(-1);
    }
    m_shape = 0;
    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->CancelUndo();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CMergeShapeObjectsTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        GetIEditor()->SetEditTool(0);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CMergeShapeObjectsTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    //return true;
    if (event == eMouseLDown || event == eMouseMove)
    {
        CSelectionGroup* pSel = GetIEditor()->GetSelection();

        bool foundSel = false;

        for (int i = 0; i < pSel->GetCount(); i++)
        {
            CBaseObject* pObj = pSel->GetObject(i);
            if (!qobject_cast<CShapeObject*>(pObj))
            {
                continue;
            }

            CShapeObject* shape = (CShapeObject*) pObj;

            const Matrix34& shapeTM = shape->GetWorldTM();

            float dist;
            Vec3 raySrc, rayDir;
            view->ViewToWorldRay(point, raySrc, rayDir);

            // Find closest point on the shape.
            int p1, p2;
            Vec3 intPnt;
            shape->GetNearestEdge(raySrc, rayDir, p1, p2, dist, intPnt);

            float fShapeCloseDistance = SHAPE_CLOSE_DISTANCE * view->GetScreenScaleFactor(intPnt) * 0.01f;

            // If control we are editing edges..
            if (p1 >= 0 && p2 >= 0 && dist < fShapeCloseDistance + view->GetSelectionTolerance())
            {
                view->SetCurrentCursor(STD_CURSOR_HIT);
                if (event == eMouseLDown)
                {
                    if (m_curPoint == 0)
                    {
                        if (shape != m_shape)
                        {
                            m_curPoint++;
                        }
                        shape->SetMergeIndex(p1);
                    }

                    if (m_curPoint == -1)
                    {
                        m_shape = shape;
                        m_curPoint++;
                        shape->SetMergeIndex(p1);
                    }

                    if (m_curPoint == 1)
                    {
                        GetIEditor()->BeginUndo();
                        if (m_shape)
                        {
                            m_shape->Merge(shape);
                        }
                        if (GetIEditor()->IsUndoRecording())
                        {
                            GetIEditor()->AcceptUndo("Merge shapes");
                        }
                        GetIEditor()->SetEditTool(0);
                    }
                }
                foundSel = true;
                break;
            }
        }

        if (!foundSel)
        {
            view->ResetCursor();
        }

        return true;
    }
    return false;
}





//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// CShapeMultySelPanel dialog

//////////////////////////////////////////////////////////////////////////
CShapeMultySelPanel::CShapeMultySelPanel(QWidget* pParent /* = nullptr */)
    : QWidget(pParent)
    , m_ui(new Ui::ShapeMultySelPanel())
{
    m_ui->setupUi(this);
    m_ui->mergeButton->SetToolClass(&CMergeShapeObjectsTool::staticMetaObject, 0);
}

CShapeMultySelPanel::~CShapeMultySelPanel()
{
}


//////////////////////////////////////////////////////////////////////////
// RopePanel
//////////////////////////////////////////////////////////////////////////

CRopePanel::CRopePanel(QWidget* pParent /* = nullptr */)
    : QWidget(pParent)
    , m_ui(new Ui::RopePanel())
{
    m_ui->setupUi(this);
    Initialize(m_ui->useTransformGizmoCheck, m_ui->editButton, m_ui->splitButton, m_ui->numberOfPointsLabel);
}


//////////////////////////////////////////////////////////////////////////
// CTerritoryPanel
//////////////////////////////////////////////////////////////////////////

CAITerritoryPanel::CAITerritoryPanel(QWidget* pParent /* = nullptr */)
    : CEntityPanel(pParent)
{
    m_prototypeButton->disconnect(this);
    connect(m_prototypeButton, &QPushButton::clicked, this, &CAITerritoryPanel::OnSelectAssignedAIs);
}

//////////////////////////////////////////////////////////////////////////
void CAITerritoryPanel::OnSelectAssignedAIs()
{
    GetIEditor()->GetObjectManager()->SelectAssignedEntities();
}

void CAITerritoryPanel::UpdateAssignedAIsPanel()
{
    size_t nAssignedAIs = GetIEditor()->GetObjectManager()->NumberOfAssignedEntities();

    m_frame->setTitle(QString(tr("Assigned AIs: %1")).arg(nAssignedAIs));

    m_prototypeButton->setEnabled(nAssignedAIs);
}

//////////////////////////////////////////////////////////////////////////
CNavigationAreaPanel::CNavigationAreaPanel(QWidget* pParent /* = nullptr */)
    : QWidget(pParent)
    , m_ui(new Ui::NavigationAreaPanel())
{
    m_ui->setupUi(this);
    Initialize(m_ui->useTransformGizmoCheck, m_ui->editButton, m_ui->splitButton, m_ui->numberOfPointsLabel);
}

#include <ShapePanel.moc>

