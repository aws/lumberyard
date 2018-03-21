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
#include "ClipVolumeObject.h"
#include "ClipVolumeObjectPanel.h"
#include "Objects/ShapeObject.h"
#include "Core/Model.h"
#include "Core/BrushHelper.h"
#include "Tools/ClipVolumeTool.h"
#include "ToolFactory.h"
#include "Viewport.h"
#include <IChunkFile.h>
#include <IEntityHelper.h>
#include "Components/IComponentClipVolume.h"
#include <AzCore/std/parallel/atomic.h>


AZStd::atomic_uint s_nGlobalFileClipVolumeID = {0};

ClipVolumeObject::ClipVolumeObject()
{
    m_pEditClipVolumePanel = NULL;
    m_nEditClipVolumeRollUpID = 0;

    m_entityClass = "ClipVolume";
    m_nUniqFileIdForExport = (s_nGlobalFileClipVolumeID++);

    EnableReload(false);

    AddVariable(mv_filled, "Filled");
}

ClipVolumeObject::~ClipVolumeObject()
{
}

IStatObj* ClipVolumeObject::GetIStatObj()
{
    _smart_ptr<IStatObj> pStatObj;
    if (CD::ModelCompiler* pBaseBrush = GetCompiler())
    {
        pBaseBrush->GetIStatObj(&pStatObj);
    }

    return pStatObj.get();
}

void ClipVolumeObject::Serialize(CObjectArchive& ar)
{
    DesignerBaseObject<CEntityObject>::Serialize(ar);

    if (ar.bLoading)
    {
        if (!GetModel())
        {
            SetModel(new CD::Model);
        }
        GetModel()->SetModeFlag(CD::eDesignerMode_FrameRemainAfterDrill);

        if (XmlNodeRef brushNode = ar.node->findChild("Brush"))
        {
            GetModel()->Serialize(brushNode, ar.bLoading, ar.bUndo);
        }

        if (ar.bUndo)
        {
            if (GetCompiler())
            {
                GetCompiler()->DeleteAllRenderNodes();
            }
        }

        CD::UpdateAll(MainContext());
    }
    else
    {
        if (GetCompiler())
        {
            XmlNodeRef brushNode = ar.node->newChild("Brush");
            GetModel()->Serialize(brushNode, ar.bLoading, ar.bUndo);
        }
    }
}

bool ClipVolumeObject::CreateGameObject()
{
    DesignerBaseObject<CEntityObject>::CreateGameObject();

    if (m_pEntity)
    {
        auto gameFileName = GenerateGameFilename().toUtf8();
        SEntitySpawnParams spawnParams;
        spawnParams.pUserData = (void*)gameFileName.data();

        IComponentClipVolumePtr clipVolumeComponent = m_pEntity->GetOrCreateComponent<IComponentClipVolume>();
        clipVolumeComponent->InitComponent(m_pEntity, spawnParams);

        UpdateGameResource();
    }

    return m_pEntity != NULL;
}

void ClipVolumeObject::GetLocalBounds(AABB& bbox)
{
    bbox.Reset();

    if (CD::Model* pModel = GetModel())
    {
        bbox = pModel->GetBoundBox();
    }
}

void ClipVolumeObject::Display(DisplayContext& dc)
{
    if (IStatObj* pStatObj = GetIStatObj())
    {
        if (IsFrozen())
        {
            dc.SetFreezeColor();
        }

        QColor lineColor = GetColor();
        QColor solidColor = GetColor();

        if (IsSelected())
        {
            lineColor = dc.GetSelectedColor();
        }
        else if (IsHighlighted() && gSettings.viewports.bHighlightMouseOverGeometry)
        {
            lineColor = QColor(255, 255, 0);
        }

        SGeometryDebugDrawInfo dd;
        dd.tm = GetWorldTM();
        dd.bExtrude = false;
        dd.color = ColorB(solidColor.red(), solidColor.green(), solidColor.blue());
        dd.lineColor = ColorB(lineColor.red(), lineColor.green(), lineColor.blue());

        const float fAlpha = (IsSelected() || mv_filled) ? 0.3f : 0;
        dd.color.a = uint8(fAlpha * 255);
        dd.lineColor.a = 255;

        if (IsSelected())
        {
            dd.color.a = uint8(0.15f * 255);
            dd.lineColor.a = uint8(0.15f * 255);
            ;

            dc.DepthTestOff();
            pStatObj->DebugDraw(dd);

            dd.color.a = uint8(0.25f * 255);
            dd.lineColor.a = uint8(1.0f * 255);
            ;

            dc.DepthTestOn();
            pStatObj->DebugDraw(dd);
        }
        else
        {
            pStatObj->DebugDraw(dd);
        }
    }

    DrawDefault(dc);
}

void ClipVolumeObject::UpdateGameResource()
{
    DynArray<Vec3> meshFaces;

    if (m_pEntity)
    {
        if (IComponentClipVolumePtr pVolume = m_pEntity->GetOrCreateComponent<IComponentClipVolume>())
        {
            if (IStatObj* pStatObj = GetIStatObj())
            {
                if (IRenderMesh* pRenderMesh = pStatObj->GetRenderMesh())
                {
                    pRenderMesh->LockForThreadAccess();
                    if (pRenderMesh->GetIndicesCount() && pRenderMesh->GetVerticesCount())
                    {
                        meshFaces.reserve(pRenderMesh->GetIndicesCount());

                        int posStride;
                        const uint8* pPositions = (uint8*)pRenderMesh->GetPosPtr(posStride, FSL_READ);
                        const vtx_idx* pIndices = pRenderMesh->GetIndexPtr(FSL_READ);

                        TRenderChunkArray& Chunks = pRenderMesh->GetChunks();
                        for (int nChunkId = 0; nChunkId < Chunks.size(); nChunkId++)
                        {
                            CRenderChunk* pChunk = &Chunks[nChunkId];
                            if (!(pChunk->m_nMatFlags & MTL_FLAG_NODRAW))
                            {
                                for (uint  i = pChunk->nFirstIndexId; i < pChunk->nFirstIndexId + pChunk->nNumIndices; ++i)
                                {
                                    meshFaces.push_back(*alias_cast<Vec3*>(pPositions + pIndices[i] * posStride));
                                }
                            }
                        }
                    }

                    pRenderMesh->UnlockStream(VSF_GENERAL);
                    pRenderMesh->UnLockForThreadAccess();

                    pVolume->UpdateRenderMesh(pStatObj->GetRenderMesh(), meshFaces);
                }
            }
        }
    }

    UpdateCollisionData(meshFaces);
    InvalidateTM(0);   // BBox will most likely have changed
}

void ClipVolumeObject::UpdateCollisionData(const DynArray<Vec3>& meshFaces)
{
    struct SEdgeCompare
    {
        enum ComparisonResult
        {
            eResultLess = 0,
            eResultEqual,
            eResultGreater
        };

        ComparisonResult pointCompare(const Vec3& a, const Vec3& b) const
        {
            for (int i = 0; i < 3; ++i)
            {
                if (a[i] < b[i])
                {
                    return eResultLess;
                }
                else if (a[i] > b[i])
                {
                    return eResultGreater;
                }
            }

            return eResultEqual;
        }

        bool operator()(const Lineseg& a, const Lineseg& b) const
        {
            ComparisonResult cmpResult = pointCompare(a.start, b.start);
            if (cmpResult == eResultEqual)
            {
                cmpResult = pointCompare(a.end, b.end);
            }

            return cmpResult == eResultLess;
        }
    };

    std::set<Lineseg, SEdgeCompare> processedEdges;

    for (int i = 0; i < meshFaces.size(); i += 3)
    {
        for (int e = 0; e < 3; ++e)
        {
            Lineseg edge = Lineseg(
                    meshFaces[i + e],
                    meshFaces[i + (e + 1) % 3]
                    );

            if (SEdgeCompare().pointCompare(edge.start, edge.end) == SEdgeCompare::eResultGreater)
            {
                std::swap(edge.start, edge.end);
            }

            if (processedEdges.find(edge) == processedEdges.end())
            {
                processedEdges.insert(edge);
            }
        }
    }

    m_MeshEdges.clear();
    m_MeshEdges.insert(m_MeshEdges.begin(), processedEdges.begin(), processedEdges.end());
}

bool ClipVolumeObject::HitTest(HitContext& hc)
{
    const float DistanceThreshold = 5.0f; // distance threshold for edge collision detection (in pixels)

    Matrix34 inverseWorldTM = GetWorldTM().GetInverted();
    Vec3 raySrc = inverseWorldTM.TransformPoint(hc.raySrc);
    Vec3 rayDir = inverseWorldTM.TransformVector(hc.rayDir).GetNormalized();

    AABB bboxLS;
    GetLocalBounds(bboxLS);

    bool bHit = false;
    Vec3 vHitPos;

    if (Intersect::Ray_AABB(raySrc, rayDir, bboxLS, vHitPos))
    {
        if (hc.b2DViewport)
        {
            vHitPos = GetWorldTM().TransformPoint(vHitPos);
            bHit = true;
        }
        else if (hc.camera)
        {
            Matrix44A mView, mProj;
            mathMatrixPerspectiveFov(&mProj, hc.camera->GetFov(), hc.camera->GetProjRatio(), hc.camera->GetNearPlane(), hc.camera->GetFarPlane());
            mathMatrixLookAt(&mView, hc.camera->GetPosition(), hc.camera->GetPosition() + hc.camera->GetViewdir(), hc.camera->GetUp());

            Matrix44A mWorldViewProj = GetWorldTM().GetTransposed() * mView * mProj;

            int viewportWidth, viewportHeight;
            hc.view->GetDimensions(&viewportWidth, &viewportHeight);
            Vec3 testPosClipSpace = Vec3(2.0f * hc.point2d.x() / (float)viewportWidth  - 1.0f, -2.0f * hc.point2d.y() / (float)viewportHeight + 1.0f, 0);

            const float minDist = sqr(DistanceThreshold / viewportWidth * 2.0f) + sqr(DistanceThreshold / viewportHeight * 2.0f);
            const float aspect = viewportHeight / (float)viewportWidth;

            for (int e = 0; e < m_MeshEdges.size(); ++e)
            {
                Vec4 e0 = Vec4(m_MeshEdges[e].start, 1.0f) * mWorldViewProj;
                Vec4 e1 = Vec4(m_MeshEdges[e].end,   1.0f) * mWorldViewProj;

                if (e0.w < hc.camera->GetNearPlane() && e1.w < hc.camera->GetNearPlane())
                {
                    continue;
                }
                else if (e0.w < hc.camera->GetNearPlane() || e1.w < hc.camera->GetNearPlane())
                {
                    if (e0.w > e1.w)
                    {
                        std::swap(e0, e1);
                    }

                    // Push vertex slightly in front of near plane
                    float t = (hc.camera->GetNearPlane() + 1e-5 - e0.w) / (e1.w - e0.w);
                    e0 += (e1 - e0) * t;
                }

                Lineseg edgeClipSpace = Lineseg(Vec3(e0 / e0.w), Vec3(e1 / e1.w));

                float t;
                Distance::Point_Lineseg2D<float>(testPosClipSpace, edgeClipSpace, t);
                Vec3 closestPoint = edgeClipSpace.GetPoint(t);

                const float closestDistance = sqr(testPosClipSpace.x - closestPoint.x) + sqr(testPosClipSpace.y - closestPoint.y) * aspect;
                if (closestDistance < minDist)
                {
                    // unproject to world space. NOTE: CCamera::Unproject uses clip space coordinate system (no y-flip)
                    Vec3 v0 = Vec3((edgeClipSpace.start.x * 0.5 + 0.5) * viewportWidth, (edgeClipSpace.start.y * 0.5 + 0.5) * viewportHeight, edgeClipSpace.start.z);
                    Vec3 v1 = Vec3((edgeClipSpace.end.x   * 0.5 + 0.5) * viewportWidth, (edgeClipSpace.end.y   * 0.5 + 0.5) * viewportHeight, edgeClipSpace.end.z);

                    hc.camera->Unproject(Vec3::CreateLerp(v0, v1, t), vHitPos);
                    bHit = true;

                    break;
                }
            }
        }
    }

    if (bHit)
    {
        hc.dist = hc.raySrc.GetDistance(vHitPos);
        hc.object = this;
        return true;
    }

    return false;
}

bool ClipVolumeObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool res = DesignerBaseObject<CEntityObject>::Init(ie, prev, file);
    SetColor(RGB(0, 0, 255));

    if (m_pEntity)
    {
        m_pEntity->GetOrCreateComponent<IComponentClipVolume>();
    }

    if (prev)
    {
        ClipVolumeObject* pVolume = static_cast<ClipVolumeObject*>(prev);
        CD::ModelCompiler* pCompiler = pVolume->GetCompiler();
        CD::Model* pModel = pVolume->GetModel();

        if (pCompiler && pModel)
        {
            SetCompiler(new CD::ModelCompiler(*pCompiler));
            SetModel(new CD::Model(*pModel));
            UpdateAll(MainContext());
        }
    }

    if (CD::ModelCompiler* pCompiler = GetCompiler())
    {
        pCompiler->RemoveFlags(CD::eCompiler_Physicalize | CD::eCompiler_CastShadow);
    }

    return res;
}

void ClipVolumeObject::EntityLinked(const QString& name, GUID targetEntityId)
{
    if (targetEntityId != GUID_NULL)
    {
        CBaseObject* pObject = FindObject(targetEntityId);
        if (qobject_cast<CEntityObject*>(pObject))
        {
            CEntityObject* target = static_cast<CEntityObject*>(pObject);
            QString type = target->GetTypeDescription();

            if ((type.compare("Light", Qt::CaseInsensitive) == 0) || (type.compare("EnvironmentLight", Qt::CaseInsensitive) == 0) || (type.compare("EnvironmentLightTOD", Qt::CaseInsensitive) == 0))
            {
                CEntityScript* pScript = target->GetScript();
                IEntity* pEntity = target->GetIEntity();

                if (pScript && pEntity)
                {
                    pScript->CallOnPropertyChange(pEntity);
                }
            }
        }
    }
}

void ClipVolumeObject::BeginEditParams(IEditor* ie, int flags)
{
    if (!m_pEditClipVolumePanel)
    {
        m_pEditClipVolumePanel = new ClipVolumeObjectPanel(this);
        m_nEditClipVolumeRollUpID = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, tr("Edit Clipvolume"), m_pEditClipVolumePanel, false);

        if (GetModel()->IsEmpty())
        {
            m_pEditClipVolumePanel->GotoEditModeAsync();
        }
    }

    // note: deliberately skipping CEntityObject::BeginEditParams so Script and Entity panels are not shown
    CBaseObject::BeginEditParams(ie, flags);
}

//////////////////////////////////////////////////////////////////////////
void ClipVolumeObject::EndEditParams(IEditor* ie)
{
    CBaseObject::EndEditParams(ie);

    if (m_nEditClipVolumeRollUpID)
    {
        ie->RemoveRollUpPage(ROLLUP_OBJECTS, m_nEditClipVolumeRollUpID);
    }
    m_nEditClipVolumeRollUpID = 0;
    m_pEditClipVolumePanel = NULL;

    UpdateGameResource();
}


QString ClipVolumeObject::GenerateGameFilename() const
{
    return QString("%level%/Brush/clipvolume_%1.cgf").arg(m_nUniqFileIdForExport);
}

void ClipVolumeObject::ExportBspTree(IChunkFile* pChunkFile) const
{
    if (m_pEntity)
    {
        if (IComponentClipVolumePtr clipVolumeComponent = m_pEntity->GetComponent<IComponentClipVolume>())
        {
            if (IBSPTree3D* pBspTree = clipVolumeComponent->GetBspTree())
            {
                size_t nBufferSize = pBspTree->WriteToBuffer(NULL);

                auto pTreeData = new uint8[nBufferSize];
                pBspTree->WriteToBuffer(pTreeData);

                pChunkFile->AddChunk(ChunkType_BspTreeData, 0x1, eEndianness_Little, pTreeData, nBufferSize);

                delete [] pTreeData;
            }
        }
    }
}

#include <Objects/ClipVolumeObject.moc>