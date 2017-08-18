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
#include "UVMappingMainWnd.h"
#include "QViewportSettings.h"
#include "Core/BrushHelper.h"
#include "Core/Model.h"
#include "Material/Material.h"
#include "QViewport.h"
#include <QBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QMenuBar>

using namespace CD;

UVMappingMainWnd::UVMappingMainWnd()
{
    GetIEditor()->RegisterNotifyListener(this);

    QWidget* centralWidget = new QWidget();
    setCentralWidget(centralWidget);
    centralWidget->setContentsMargins(0, 0, 0, 0);

    QBoxLayout* pLayout = new QBoxLayout(QBoxLayout::TopToBottom, centralWidget);

    m_pToolBar = new QToolBar();
    m_pToolBar->setIconSize(QSize(32, 32));
    m_pToolBar->setStyleSheet("QToolBar { border: 0px }");
    m_pToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_pToolBar->setFloatable(false);
    pLayout->addWidget(m_pToolBar, 0);

    setContentsMargins(0, 0, 0, 0);
    m_pViewport = new QViewport(0);

    SViewportState viewState = m_pViewport->GetState();
    viewState.cameraTarget.SetFromVectors(Vec3(1, 0, 0), Vec3(0, 0, -1), Vec3(0, 1, 0), Vec3(0.5f, 0.5f, 75.0f));
    m_pViewport->SetState(viewState);

    SViewportSettings viewportSettings = m_pViewport->GetSettings();
    viewportSettings.rendering.fps = false;
    viewportSettings.grid.showGrid = true;
    viewportSettings.grid.spacing  = 0.5f;
    viewportSettings.camera.moveSpeed = 0.004f;
    viewportSettings.camera.zoomSpeed = 12.5f;
    viewportSettings.camera.showViewportOrientation = false;
    viewportSettings.camera.transformRestraint = eCameraTransformRestraint_Rotation;
    viewportSettings.camera.fov = 1;
    m_pViewport->SetSettings(viewportSettings);
    m_pViewport->SetSceneDimensions(Vec3(100.0f, 100.0f, 100.0f));

    connect(m_pViewport, SIGNAL(SignalRender(const SRenderContext&)), this, SLOT(OnRender(const SRenderContext&)));
    pLayout->addWidget(m_pViewport, 1);
    setLayout(pLayout);

    m_pPolygonMesh = new PolygonMesh;
    std::vector<BrushVec3> rectangle(4);
    rectangle[0] = BrushVec3(0, 0, 0);
    rectangle[1] = BrushVec3(1, 0, 0);
    rectangle[2] = BrushVec3(1, 1, 0);
    rectangle[3] = BrushVec3(0, 1, 0);
    m_pPolygonMesh->SetPolygon(new CD::Polygon(rectangle), true);

    Matrix34 polygonTM = Matrix34::CreateIdentity();
    polygonTM.SetTranslation(Vec3(0, 0, 1.0f));
    m_pPolygonMesh->SetWorldTM(polygonTM);

    UpdateObject();
}

UVMappingMainWnd::~UVMappingMainWnd()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

void UVMappingMainWnd::OnRender(const SRenderContext& rc)
{
    IRenderAuxGeom* aux = gEnv->pRenderer->GetIRenderAuxGeom();

    aux->SetRenderFlags(e_Mode3D | e_AlphaBlended | e_FillModeSolid | e_CullModeNone | e_DepthWriteOff | e_DepthTestOn);

    aux->DrawLine(Vec3(0, -1.5f, 0), ColorB(0, 0, 0), Vec3(0, 1.5f, 0), ColorB(0, 0, 0), 2.0f);
    aux->DrawLine(Vec3(-1.5f, 0, 0), ColorB(0, 0, 0), Vec3(1.5f, 0, 0), ColorB(0, 0, 0), 2.0f);

    if (m_pObject)
    {
        m_pPolygonMesh->GetRenderNode()->Render(*rc.renderParams, *rc.passInfo);
    }

    RenderUnwrappedMesh(rc);
}

void UVMappingMainWnd::OnViewportUpdate()
{
    if (m_pViewport)
    {
        m_pViewport->Update();
    }
}

void UVMappingMainWnd::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (event == eNotify_OnIdleUpdate)
    {
        OnViewportUpdate();
    }
    else if (event == eNotify_OnSelectionChange)
    {
        UpdateObject();
    }
}

void UVMappingMainWnd::UpdateObject()
{
    std::vector<DesignerObject*> objects = CD::GetSelectedDesignerObjects();
    if (!objects.empty())
    {
        m_pObject = objects[0];
        if (m_pObject->GetMaterial())
        {
            m_pPolygonMesh->SetMaterialName(m_pObject->GetMaterial()->GetMatInfo()->GetName());
        }
        else
        {
            m_pPolygonMesh->SetMaterialName("");
        }
    }
    else
    {
        m_pObject = NULL;
        m_pPolygonMesh->SetMaterialName("");
    }
}

void UVMappingMainWnd::RenderUnwrappedMesh(const SRenderContext& rc)
{
    if (!m_pObject)
    {
        return;
    }

    CD::Model* pModel = m_pObject->GetModel();
    for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        RenderUnwrappedPolygon(rc, pModel->GetPolygon(i));
    }
}

void UVMappingMainWnd::RenderUnwrappedPolygon(const SRenderContext& rc, const PolygonPtr pPolygon)
{
    if (!pPolygon)
    {
        return;
    }

    for (int i = 0, iEdgeCount(pPolygon->GetEdgeCount()); i < iEdgeCount; ++i)
    {
        const CD::SEdge& edge = pPolygon->GetEdgeIndexPair(i);
    }
}

#include <UIs/UVMappingMainWnd.moc>