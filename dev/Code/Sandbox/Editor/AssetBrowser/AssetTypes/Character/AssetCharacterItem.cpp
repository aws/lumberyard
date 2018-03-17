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
#include "AssetCharacterItem.h"
#include "../Model/AssetModelItem.h"
#include "AssetCharacterDatabase.h"
#include "AssetBrowser/AssetBrowserDialog.h"
#include "Util/PathUtil.h"
#include "Include/IAssetViewer.h"
#include "StringUtils.h"
#include "IMaterial.h"
#include "Controls/AssetViewer.h"
#include "AssetBrowserPreviewCharacterDlg.h"
#include "AssetBrowserPreviewCharacterDlgFooter.h"
#include "Controls/PreviewModelCtrl.h"
#include "IStreamEngine.h"
#include "IEditor.h"
#include "ITimeOfDay.h"

#include <QFile>
#include <QPainter>
#include <QTextStream>

UINT CAssetCharacterItem::s_frameBufferScreenshot[CAssetCharacterItem::kAssetDisplay_MaxThumbImageBufferSize];
QPointer<CAssetBrowserPreviewCharacterDlg> CAssetCharacterItem::s_modelPreviewDlgHeader;
QPointer<CAssetBrowserPreviewCharacterDlgFooter> CAssetCharacterItem::s_modelPreviewDlgFooter;
CPreviewModelCtrl* CAssetCharacterItem::s_pPreviewCtrl = NULL;

QImage CAssetCharacterItem::s_uncachedModelThumbBmp;
bool CAssetCharacterItem::s_bWireframe = false;
bool CAssetCharacterItem::s_bPhysics = false;
bool CAssetCharacterItem::s_bNormals = false;
float CAssetCharacterItem::s_fAmbience = AssetBrowser::kCharacterAssetAmbienceMultiplier;

CAssetCharacterItem::CAssetCharacterItem()
    : CAssetItem()
    , m_camTarget(0, 0, 0)
    , m_camRadius(1)
    , m_pObject(NULL)
    , m_pRenderer(gEnv->pRenderer)
    , m_bGrid(false)
    , m_bAxis(true)
    , m_clearColor(0.47f, 0.63f, 0.8f)
    , m_bRotate(false)
    , m_rotateAngle(0)
    , m_bShowObject(true)
    , m_bCacheCurrentThumbAngle(false)
    , m_fov(60)
    , m_rotationX(AssetBrowser::kDefaultCharacterRotationAngleX)
    , m_rotationY(AssetBrowser::kDefaultCharacterRotationAngleY)
    , m_translateX(0)
    , m_translateY(0)
    , m_camZoom(1)
    , m_lodCount(0)
    , m_triangleCount(0)
    , m_vertexCount(0)
    , m_submeshCount(0)
    , m_textureSize(0)
    , m_physicsTriCount(0)
    , m_mtlCount(0)
    , m_physicsSize(0)
    , m_bSplitLODs(false)
    , m_bIsLod(false)
    , m_bCameraUpdate(false)
    , m_dragCreationMode(eAssetCharacterItemDragCreationMode_AsBrush)
{
    m_flags |= eFlag_CanBeDraggedInViewports | eFlag_CanBeMovedAfterDroppedIntoViewport;
    m_aabb.Reset();
    m_crtRotation.CreateIdentity();
    memset(m_triCountLOD, 0, sizeof(m_triCountLOD));
    memset(m_vertCountLOD, 0, sizeof(m_vertCountLOD));
    memset(m_subMeshCountLOD, 0, sizeof(m_subMeshCountLOD));
    memset(m_meshSizeLOD, 0, sizeof(m_meshSizeLOD));

    // If the static/global variables weren't initialized, we initialize them while creating the first item.
    // TODO: this code should be moved to the database initialization, or to some common initialization area.
    if (!AssetViewer::s_bChrStaticsInitialized)
    {
        // If s_assetThumbTimeOfDay, it will already give us a message in the log file, so no need for any special handling here.
        AssetViewer::s_chrAssetThumbTimeOfDay = GetISystem()->LoadXmlFromFile("Editor/asset_thumbnail.tod");
        // Even if the initialization failed, we don't want to try it over and over again, as we would just fail over and over again.
        AssetViewer::s_bChrStaticsInitialized = true;
    }

    if (s_uncachedModelThumbBmp.isNull())
    {
        s_uncachedModelThumbBmp.load("Editor/UI/Icons/asset_character.png");
    }

    m_pUncachedThumbBmp = &s_uncachedModelThumbBmp;
}

CAssetCharacterItem::~CAssetCharacterItem()
{
}

HRESULT STDMETHODCALLTYPE CAssetCharacterItem::QueryInterface(const IID& riid, void** ppvObj)
{
    if (riid == __uuidof(IAssetItem))
    {
        *ppvObj = this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CAssetCharacterItem::AddRef()
{
    return ++m_ref;
}

ULONG STDMETHODCALLTYPE CAssetCharacterItem::Release()
{
    if ((--m_ref) == 0)
    {
        FreeData();
        delete this;
        return 0;
    }
    else
    {
        return m_ref;
    }
}

bool CAssetCharacterItem::LoadModel()
{
    m_pObject = gEnv->p3DEngine->LoadStatObjUnsafeManualRef((m_strRelativePath + m_strFilename).toUtf8().data(), 0, 0, true);

    if (!m_pObject)
    {
        return false;
    }

    m_pObject->AddRef();
    m_aabb.min = m_pObject->GetBoxMin();
    m_aabb.max = m_pObject->GetBoxMax();

    return true;
}

bool CAssetCharacterItem::Cache()
{
    if (m_flags & eFlag_Cached)
    {
        return true;
    }

    if (!LoadModel())
    {
        return false;
    }

    IStatObj::SStatistics stats;

    stats.pTextureSizer = gEnv->pSystem->CreateSizer();
    m_pObject->GetStatistics(stats);

    if (!stats.nIndices || !stats.nVertices)
    {
        SAFE_RELEASE(stats.pTextureSizer);
        SAFE_RELEASE(m_pObject);
        return false;
    }

    m_lodCount = stats.nLods;
    m_triangleCount = stats.nIndices / 3;
    m_vertexCount = stats.nVertices;
    m_submeshCount = stats.nSubMeshCount;
    m_nRef = stats.nNumRefs;
    m_physicsTriCount = stats.nPhysPrimitives;
    m_physicsSize = (stats.nPhysProxySize + 512);
    m_bSplitLODs = stats.bSplitLods;
    m_textureSize = stats.pTextureSizer->GetTotalSize();
    m_mtlCount = 0;

    for (int i = 0; i < MAX_STATOBJ_LODS_NUM; ++i)
    {
        IStatObj* pLodObj = m_pObject->GetLodObject(i);
        IStatObj::SStatistics lodStats;

        m_triCountLOD[i] = stats.nIndicesPerLod[i] / 3;

        if (pLodObj)
        {
            pLodObj->GetStatistics(lodStats);
            m_vertCountLOD[i] = lodStats.nVertices;
            m_subMeshCountLOD[i] = lodStats.nSubMeshCount;
            m_meshSizeLOD[i] = lodStats.nMeshSize;
        }
    }

    SAFE_RELEASE(stats.pTextureSizer);

    GatherDependenciesInfo();
    PrepareThumbTextInfo();
    GetOwnerDatabase()->OnMetaDataChange(this);
    Render(
        &CAssetBrowserDialog::Instance()->GetAssetViewer(),
        QRect(0, 0, gSettings.sAssetBrowserSettings.nThumbSize, gSettings.sAssetBrowserSettings.nThumbSize),
        true);
    CAssetItem::Cache();
    m_cachedThumbBmp = QImage();
    SAFE_RELEASE(m_pObject);
    gEnv->p3DEngine->LockCGFResources();
    gEnv->p3DEngine->UnlockCGFResources();
    gEnv->p3DEngine->Update();
    gEnv->pSystem->GetStreamEngine()->Update();

#ifdef ASSET_BROWSER_USE_CGF_STREAMING
    return false; // we did not fully cached yet
#else
    SetFlag(IAssetItem::eFlag_Cached, true);
    return true;
#endif
}

void CAssetCharacterItem::GatherDependenciesInfo()
{
    size_t subMtls = m_pObject->GetMaterial()->GetSubMtlCount();

    m_dependencies.clear();

    if (!subMtls)
    {
        _smart_ptr<IMaterial> pMtl = NULL;
        pMtl = m_pObject->GetMaterial();

        // fill up dependencies
        if (pMtl)
        {
            IRenderShaderResources* pShaderRes = pMtl->GetShaderItem().m_pShaderResources;

            m_dependencies["Materials"].insert(pMtl->GetName());

            if (pShaderRes)
            {
                for (auto iter = pShaderRes->GetTexturesResourceMap()->begin(); iter != pShaderRes->GetTexturesResourceMap()->end(); ++iter)
                {
                    SEfResTexture* pTex = &(iter->second);
                    m_dependencies["Textures"].insert(pTex->m_Name.c_str());
                }
            }
        }
    }
    else
    {
        for (size_t s = 0; s < subMtls; ++s)
        {
            _smart_ptr<IMaterial> pMtl = m_pObject->GetMaterial()->GetSubMtl(s);

            // fill up dependencies
            if (pMtl)
            {
                IRenderShaderResources* pShaderRes = pMtl->GetShaderItem().m_pShaderResources;

                m_dependencies["Materials"].insert(pMtl->GetName());

                if (pShaderRes)
                {
                    for (auto iter = pShaderRes->GetTexturesResourceMap()->begin(); iter != pShaderRes->GetTexturesResourceMap()->end(); ++iter)
                    {
                        if (SEfResTexture* pTex = &(iter->second))
                        {
                            m_dependencies["Textures"].insert(pTex->m_Name.c_str());
                        }
                    }
                }
            }
        }
    }

    m_mtlCount = m_dependencies["Materials"].size();
}

void CAssetCharacterItem::CacheCurrentThumbAngle()
{
    m_bCacheCurrentThumbAngle = true;
    ForceCache();
    m_bCacheCurrentThumbAngle = false;
}

void CAssetCharacterItem::OnBeginPreview(QWidget* hQuickPreviewWnd)
{
    if (!LoadModel())
    {
        return;
    }

    const QRect rc = hQuickPreviewWnd->rect();
    CreateViewport(hQuickPreviewWnd, rc);

    if (s_pPreviewCtrl)
    {
        s_pPreviewCtrl->show();
        s_pPreviewCtrl->SetObject(m_pObject);
        s_pPreviewCtrl->Update(true);
        m_pObject->AddRef();
        CalculateCameraPosition();
        SetCamera(m_camera, rc);
    }

    ResetView();
}

void CAssetCharacterItem::OnEndPreview()
{
    if (s_pPreviewCtrl)
    {
        s_pPreviewCtrl->ReleaseObject();
        s_pPreviewCtrl->hide();
        SAFE_RELEASE(m_pObject);
    }
}

QWidget* CAssetCharacterItem::GetCustomPreviewPanelHeader(QWidget* pParentWnd)
{
    if (!CAssetCharacterItem::s_modelPreviewDlgHeader)
    {
        CAssetCharacterItem::s_modelPreviewDlgHeader = new CAssetBrowserPreviewCharacterDlg(pParentWnd);
    }

    CAssetCharacterItem::s_modelPreviewDlgHeader->m_pModel = this;

    CAssetCharacterItem::s_modelPreviewDlgHeader->Init();

    return CAssetCharacterItem::s_modelPreviewDlgHeader;
}

QWidget* CAssetCharacterItem::GetCustomPreviewPanelFooter(QWidget* pParentWnd)
{
    if (!CAssetCharacterItem::s_modelPreviewDlgFooter)
    {
        CAssetCharacterItem::s_modelPreviewDlgFooter = new CAssetBrowserPreviewCharacterDlgFooter(pParentWnd);
    }
    if (!CAssetCharacterItem::s_modelPreviewDlgHeader)
    {
        CAssetCharacterItem::s_modelPreviewDlgHeader = new CAssetBrowserPreviewCharacterDlg;
    }

    CAssetCharacterItem::s_modelPreviewDlgFooter->Init();
    CAssetCharacterItem::s_modelPreviewDlgHeader->SetFooter(s_modelPreviewDlgFooter);

    return CAssetCharacterItem::s_modelPreviewDlgFooter;
}

void CAssetCharacterItem::PreviewRender(
    QWidget* hRenderWindow,
    const QRect& rstViewport,
    int aMouseX, int aMouseY,
    int aMouseDeltaX, int aMouseDeltaY,
    int aMouseWheelDelta, UINT aKeyFlags)
{
    // this way we can zoom and pan in the same time
    if ((aKeyFlags & MK_SHIFT) || (aKeyFlags & MK_CONTROL))
    {
        if (aKeyFlags & MK_SHIFT)
        {
            m_camZoom += (float) aMouseDeltaX * AssetViewer::kInteractiveRenderModelZoomMultiplier;
        }

        if (aKeyFlags & MK_CONTROL)
        {
            m_translateX += (float) aMouseDeltaX * AssetViewer::kInteractiveRenderModelTranslateMultiplier;
            m_translateY += (float) aMouseDeltaY * AssetViewer::kInteractiveRenderModelTranslateMultiplier;
        }
    }
    else
    {
        m_rotationY += (float) aMouseDeltaX * AssetViewer::kInteractiveRenderModelRotateMultiplier;
        m_rotationX += (float) aMouseDeltaY * AssetViewer::kInteractiveRenderModelRotateMultiplier;
    }

    m_camZoom -= AssetViewer::kModelPreviewCameraZoomFactor * (float)aMouseWheelDelta / WHEEL_DELTA;

    Render(hRenderWindow, rstViewport, false);
}

void CAssetCharacterItem::OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags)
{
    if (bKeyDown && aChar == AssetViewer::kModelAssetWireframeToggleKey)
    {
        CAssetCharacterItem::s_bWireframe = !CAssetCharacterItem::s_bWireframe;
    }

    if (bKeyDown && aChar == AssetViewer::kModelAssetPhysicsToggleKey)
    {
        CAssetCharacterItem::s_bPhysics = !CAssetCharacterItem::s_bPhysics;
    }

    if (bKeyDown && aChar == AssetViewer::kModelAssetNormalsToggleKey)
    {
        CAssetCharacterItem::s_bNormals = !CAssetCharacterItem::s_bNormals;
    }
}

bool CAssetCharacterItem::Render(QWidget* hRenderWindow, const QRect& rstViewport, bool bCacheThumbnail)
{
    if (!m_pObject)
    {
        return false;
    }

    if (m_pObject->m_eStreamingStatus != ecss_Ready)
    {
        return false;
    }

    gEnv->p3DEngine->Update();
    gEnv->pSystem->GetStreamEngine()->Update();

    if (s_pPreviewCtrl && !bCacheThumbnail)
    {
        s_pPreviewCtrl->setGeometry(rstViewport);
        s_pPreviewCtrl->EnableWireframeRendering(s_bWireframe);
        s_pPreviewCtrl->SetAmbientMultiplier(s_fAmbience);
        s_pPreviewCtrl->SetShowNormals(s_bNormals);
        s_pPreviewCtrl->SetShowPhysics(s_bPhysics);
        s_pPreviewCtrl->SetShowRenderInfo(false);
    }

    if (bCacheThumbnail)
    {
        CacheThumbnail(hRenderWindow, rstViewport);
    }

    return true;
}

void CAssetCharacterItem::CacheThumbnail(QWidget* hRenderWindow, const QRect& rc)
{
    CreateViewport(hRenderWindow, rc, true);

    XmlNodeRef oldTod = GetISystem()->CreateXmlNode();
    assert(oldTod);
    ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
    assert(pTimeOfDay);

    if (s_pPreviewCtrl)
    {
        float fTime = 0;
        ITimeOfDay::SAdvancedInfo advInfo;

        if (pTimeOfDay && AssetViewer::s_chrAssetThumbTimeOfDay != NULL)
        {
            fTime = pTimeOfDay->GetTime();
            pTimeOfDay->Serialize(oldTod, false);
            pTimeOfDay->Serialize(AssetViewer::s_chrAssetThumbTimeOfDay, true);
            pTimeOfDay->GetAdvancedInfo(advInfo);
        }

        s_pPreviewCtrl->show();
        s_pPreviewCtrl->SetObject(m_pObject);
        s_pPreviewCtrl->SetAmbientMultiplier(s_fAmbience);
        s_pPreviewCtrl->SetGrid(false);
        s_pPreviewCtrl->SetAxis(false);
        m_pObject->AddRef();

        if (!m_bCacheCurrentThumbAngle)
        {
            CalculateCameraPosition();
            SetCamera(m_camera, rc);
        }

        gEnv->p3DEngine->Update();
        gEnv->pSystem->GetStreamEngine()->Update();
        s_pPreviewCtrl->Update();
        s_pPreviewCtrl->repaint();
        s_pPreviewCtrl->hide();

        SaveThumbnail(s_pPreviewCtrl);

        s_pPreviewCtrl->ReleaseObject();

        if (pTimeOfDay && AssetViewer::s_chrAssetThumbTimeOfDay != NULL)
        {
            pTimeOfDay->Serialize(oldTod, true);
        }
    }

    m_flags |= eFlag_ThumbnailLoaded;
}

void CAssetCharacterItem::CreateViewport(QWidget* hRenderWindow, const QRect& rc, bool bThumbnail)
{
    if (s_pPreviewCtrl && s_pPreviewCtrl->parent() != hRenderWindow)
    {
        delete s_pPreviewCtrl;
        s_pPreviewCtrl = nullptr;
    }

    if (!s_pPreviewCtrl)
    {
        s_pPreviewCtrl = new CPreviewModelCtrl(hRenderWindow);
        s_pPreviewCtrl->setGeometry(rc);
        s_pPreviewCtrl->SetGrid(true);
        s_pPreviewCtrl->EnableUpdate(true);
        s_pPreviewCtrl->show();
    }
}

void CAssetCharacterItem::CalculateCameraPosition()
{
    AABB        box(m_aabb);
    Vec3        stCameraDirection(0.0f, 0.0f, 0.0f);
    float       fCameraDistance(0);
    float       afAABBDimensions[3];
    float       afRatios[3];
    float   fSmallestRatio(FLT_MAX);
    float       afCameraDistances[2];
    float       fSign(-1.0f);
    int32       nCount(0);
    int32       nSmallestRationIndex(0);
    int32       nCameraDimensionIndex(0);
    // If m_fov==45+180*K, FPE.
    float       fHalfFovTangent(tanf(DEG2RAD(m_fov / 2.0f)));

    for (nCount = 0; nCount < 3; ++nCount)
    {
        afAABBDimensions[nCount] = box.max[nCount] - box.min[nCount];
    }

    for (nCount = 0; nCount < 3; ++nCount)
    {
        if (afAABBDimensions[(nCount + 1) % 3] != 0)
        {
            float dim = afAABBDimensions[(nCount + 1) % 3];
            const float kEpsilon = 0.00001f;

            if (dim < -kEpsilon && dim > kEpsilon)
            {
                afRatios[nCount] = fabs(fabs(afAABBDimensions[nCount] / dim) - 1.0f);
            }
            else
            {
                afRatios[nCount] = 1.0f;
            }
        }
        else
        {
            afRatios[nCount] = FLT_MAX;
        }
    }

    for (nCount = 0; nCount < 3; ++nCount)
    {
        if (fSmallestRatio < afRatios[nCount])
        {
            fSmallestRatio = afRatios[nCount];
            nSmallestRationIndex = nCount;
        }
    }

    nSmallestRationIndex = 2;
    nCameraDimensionIndex = (nSmallestRationIndex + 2) % 3;

    if (((nCameraDimensionIndex + 1) % 3) == 2)
    {
        fSign *= fSign;
    }

    stCameraDirection[nCameraDimensionIndex] = 1.0f * fSign;

    // We want to look at the center of the front face of the AABB.
    m_camTarget = (box.max + box.min) * 0.5f;

    afCameraDistances[0] = m_camTarget[(nCameraDimensionIndex + 1) % 3] - m_aabb.min[(nCameraDimensionIndex + 1) % 3];
    afCameraDistances[1] = m_camTarget[(nCameraDimensionIndex + 2) % 3] - m_aabb.min[(nCameraDimensionIndex + 2) % 3];

    fCameraDistance = box.GetRadius();
    stCameraDirection.Set(0.0f, -1.0f, 0.0f);

    if (fCameraDistance < m_camera.GetNearPlane())
    {
        fCameraDistance = m_camera.GetNearPlane();
    }

    // grow a little bit, further from camera center, add padding
    fCameraDistance *= 1.8f;
    Vec3 pos = stCameraDirection;
    Matrix33 rot;

    rot.SetIdentity();
    rot.SetRotationXYZ(Ang3(m_rotationX, 0, -m_rotationY));
    stCameraDirection = -rot.TransformVector(pos);
    Matrix34 tm = Matrix33::CreateRotationVDir(stCameraDirection, 0);

    Vec3 vLeft = Vec3(-1, 0, 0);
    Vec3 vUp = Vec3(0, 0, 1);

    vLeft = tm.TransformVector(vLeft);
    vUp = tm.TransformVector(vUp);

    Vec3 panVector = vUp * m_translateY + vLeft * m_translateX;
    m_camTarget += panVector;
    tm.SetTranslation(m_camTarget - stCameraDirection * fCameraDistance * m_camZoom + panVector);
    m_camera.SetMatrix(tm);
    m_camRadius = fCameraDistance;
    m_bCameraUpdate = true;
}

void* CAssetCharacterItem::CreateInstanceInViewport(float x, float y, float z)
{
    CBaseObject* pObj = GetIEditor()->GetObjectManager()->NewObject("Brush", 0, GetRelativePath() + GetFilename());
    GetIEditor()->GetObjectManager()->BeginEditParams(pObj, OBJECT_CREATE);
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedBrushes);
    pObj->SetPos(Vec3(x, y, z));
    GetIEditor()->GetObjectManager()->EndEditParams();

    return pObj;
}

bool CAssetCharacterItem::MoveInstanceInViewport(const void* pDraggedObject, float aNewX, float aNewY, float aNewZ)
{
    CBaseObject* pObj = (CBaseObject*)pDraggedObject;

    if (pObj)
    {
        pObj->SetPos(Vec3(aNewX, aNewY, aNewZ));
        return true;
    }

    return false;
}

void CAssetCharacterItem::AbortCreateInstanceInViewport(const void* pDraggedObject)
{
    CBaseObject* pObj = (CBaseObject*)pDraggedObject;

    if (pObj)
    {
        GetIEditor()->GetObjectManager()->DeleteObject(pObj);
    }
}

void CAssetCharacterItem::SetCamera(CCamera& cam, const QRect& rcViewportRect)
{
    if (!m_bCameraUpdate)
    {
        return;
    }

    int w = rcViewportRect.width();
    int h = rcViewportRect.height();

    if (s_pPreviewCtrl)
    {
        Vec3 lookdir = m_camTarget - cam.GetPosition();
        lookdir.normalize();
        s_pPreviewCtrl->SetCameraLookAt(2.0f, lookdir);
    }

    m_camera.SetPosition(cam.GetPosition());
    m_camera.SetFrustum(w, h, DEG2RAD(m_fov), m_camera.GetNearPlane(), m_camera.GetFarPlane());
    m_camera.SetViewPort(rcViewportRect.left(), rcViewportRect.top(), rcViewportRect.width(), rcViewportRect.height());
    m_pRenderer->SetCamera(m_camera);

    m_bCameraUpdate = false;
}

QVariant CAssetCharacterItem::GetAssetFieldValue(const char* pFieldName) const
{
    if (AssetViewer::IsFieldName(pFieldName, "thumbToolTipText"))
    {
        return m_toolTipText;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "thumbOneLineText"))
    {
        return m_toolTipOneLineText;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "islod"))
    {
        return m_bIsLod;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "trianglecount"))
    {
        return m_triangleCount;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "vertexcount"))
    {
        return m_vertexCount;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "lodcount"))
    {
        return m_lodCount;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "submeshcount"))
    {
        return m_submeshCount;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "bbox_x"))
    {
        return m_aabb.GetSize().x;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "bbox_y"))
    {
        return m_aabb.GetSize().y;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "bbox_z"))
    {
        return m_aabb.GetSize().z;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "nref"))
    {
        return m_nRef;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "texturesize"))
    {
        return m_textureSize;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "phytricount"))
    {
        return m_physicsTriCount;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "physize"))
    {
        return m_physicsSize;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "splitlods"))
    {
        return m_bSplitLODs;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "lodstricount"))
    {
        QString lodsTris;
        MakeLODsTrisString(lodsTris);
        return lodsTris;
    }

    // else, check if the common fields are requested
    return CAssetItem::GetAssetFieldValue(pFieldName);
}

void CAssetCharacterItem::CacheFieldsInfoForLoadedStatObj(IStatObj* pStatObj)
{
    IStatObj::SStatistics stats;
    stats.pTextureSizer = gEnv->pSystem->CreateSizer();

    pStatObj->GetStatistics(stats);

    if (!stats.nIndices || !stats.nVertices)
    {
        return;
    }

    m_aabb.min = pStatObj->GetBoxMin();
    m_aabb.max = pStatObj->GetBoxMax();

    m_lodCount = stats.nLods;
    m_triangleCount = stats.nIndices / 3;
    m_vertexCount = stats.nVertices;
    m_submeshCount = stats.nSubMeshCount;
    m_nRef = stats.nNumRefs;
    m_physicsTriCount = stats.nPhysPrimitives;
    m_physicsSize = (stats.nPhysProxySize + 512);
    m_bSplitLODs = stats.bSplitLods;
    m_textureSize = stats.pTextureSizer->GetTotalSize();

    for (int i = 0; i < MAX_STATOBJ_LODS_NUM; ++i)
    {
        m_triCountLOD[i] = stats.nIndicesPerLod[i] / 3;
    }

    stats.pTextureSizer->Release();

    size_t subMtls = pStatObj->GetMaterial()->GetSubMtlCount();
    _smart_ptr<IMaterial> pMtl = NULL;
    m_dependencies.clear();

    if (!subMtls)
    {
        pMtl = pStatObj->GetMaterial();

        // fill up dependencies
        if (pMtl)
        {
            IRenderShaderResources* pShaderRes = pMtl->GetShaderItem().m_pShaderResources;

            m_dependencies["Materials"].insert(pMtl->GetName());

            if (pShaderRes)
            {
                for (auto iter = pShaderRes->GetTexturesResourceMap()->begin(); iter != pShaderRes->GetTexturesResourceMap()->end(); ++iter)
                {
                    if (SEfResTexture* pTex = &(iter->second))
                    {
                        m_dependencies["Textures"].insert(pTex->m_Name.c_str());
                    }
                }
            }
        }
    }
    else
    {
        for (size_t s = 0; s < subMtls; ++s)
        {
            _smart_ptr<IMaterial> pMtl = pStatObj->GetMaterial()->GetSubMtl(s);

            // fill up dependencies
            if (pMtl)
            {
                IRenderShaderResources* pShaderRes = pMtl->GetShaderItem().m_pShaderResources;

                m_dependencies["Materials"].insert(pMtl->GetName());

                if (pShaderRes)
                {
                    for (auto iter = pShaderRes->GetTexturesResourceMap()->begin(); iter != pShaderRes->GetTexturesResourceMap()->end(); ++iter)
                    {
                        if (SEfResTexture* pTex = &(iter->second))
                        {
                            m_dependencies["Textures"].insert(pTex->m_Name.c_str());
                        }
                    }
                }
            }
        }
    }

    m_mtlCount = m_dependencies["Materials"].size();

    QString tagsText, lodTris, lod;
    MakeLODsTrisString(lodTris);
    PrepareThumbTextInfo();
    GetOwnerDatabase()->OnMetaDataChange(this);
}

void CAssetCharacterItem::DrawTextOnReportImage(QPaintDevice* pd) const
{
    const QColor filenameShadowColor(0, 0, 0);
    const QColor filenameColor(255, 255, 0);
    const QColor otherInfosColor(0, 0, 0);
    QFont fontInfoTitle(QStringLiteral("Arial"), 9, QFont::Bold);
    QFont fontInfo(QStringLiteral("Arial"), 8, QFont::Bold);

    QPainter painter(pd);
    painter.setFont(fontInfoTitle);
    painter.setPen(filenameShadowColor);
    painter.drawText(AssetViewer::kOverlayTextLeftMargin + 1,
        AssetViewer::kOverlayTextTopMargin + 1, m_strFilename);
    painter.setPen(filenameColor);
    painter.drawText(AssetViewer::kOverlayTextLeftMargin,
        AssetViewer::kOverlayTextTopMargin, m_strFilename);

    QString lodsTris;

    MakeLODsTrisString(lodsTris);

    QString reportInfos = QString::fromLatin1("Path: %1\nTrianges: %2\nVertices: %3\nLODs: %4\nLODs tris: %5\nSplit LODs: %6\nPhysics tris: %7\nRefs: %8\n")
            .arg(m_strRelativePath)
            .arg(m_triangleCount)
            .arg(m_vertexCount)
            .arg(m_lodCount)
            .arg(lodsTris)
            .arg(m_bSplitLODs ? "Yes" : "No")
            .arg(m_physicsTriCount)
            .arg(m_nRef);

    const QSize titleSize = painter.fontMetrics().boundingRect(m_strFilename).size();
    const QRect rcTextInfo(QPoint(AssetViewer::kOverlayTextLeftMargin, titleSize.height() + 15), QPoint(pd->width(), pd->height()));
    painter.setPen(otherInfosColor);
    painter.setFont(fontInfo);
    painter.drawText(rcTextInfo, Qt::TextWordWrap, reportInfos);
}

void CAssetCharacterItem::MakeLODsTrisString(QString& rOutValue) const
{
    rOutValue.clear();

    for (int lod = 0; lod < MAX_STATOBJ_LODS_NUM; ++lod)
    {
        if (m_triCountLOD[lod] <= 0)
        {
            break;
        }

        const QString count = QString::number(m_triCountLOD[lod]);

        if (lod > 0)
        {
            rOutValue += QStringLiteral(" / ");
        }

        rOutValue += count;
    }
}

void CAssetCharacterItem::MakeLODsVertsString(QString& rOutValue) const
{
    rOutValue.clear();

    for (int lod = 0; lod < MAX_STATOBJ_LODS_NUM; ++lod)
    {
        if (m_vertCountLOD[lod] <= 0)
        {
            break;
        }

        const QString count = QString::number(m_vertCountLOD[lod]);

        if (lod > 0)
        {
            rOutValue += QStringLiteral(" / ");
        }

        rOutValue += count;
    }
}

void CAssetCharacterItem::MakeLODsSubMeshesString(QString& rOutValue) const
{
    rOutValue = "";

    for (int lod = 0; lod < MAX_STATOBJ_LODS_NUM; ++lod)
    {
        if (m_subMeshCountLOD[lod] <= 0)
        {
            break;
        }

        const QString count = QString::number(m_subMeshCountLOD[lod]);

        if (lod > 0)
        {
            rOutValue += QStringLiteral(" / ");
        }

        rOutValue += count;
    }
}

void CAssetCharacterItem::MakeLODsMeshSizeString(QString& rOutValue) const
{
    rOutValue.clear();

    for (int lod = 0; lod < MAX_STATOBJ_LODS_NUM; ++lod)
    {
        if (m_meshSizeLOD[lod] <= 0)
        {
            break;
        }

        const QString count = QString::number(static_cast<float>(m_meshSizeLOD[lod]) / 1024, 'f', 1);

        if (lod > 0)
        {
            rOutValue += QStringLiteral(" / ");
        }

        rOutValue += count;
    }
}

void CAssetCharacterItem::PrepareThumbTextInfo()
{
    QString tagsText;
    QString lodTris, lodVerts, lodSubs, lodMeshSize, lod;

    tagsText = CAssetItem::GetAssetFieldValue("tags").toString();
    MakeLODsTrisString(lodTris);
    MakeLODsVertsString(lodVerts);
    MakeLODsSubMeshesString(lodSubs);
    MakeLODsMeshSizeString(lodMeshSize);

    m_toolTipText = QString::fromLatin1("Path: %1\nLODs: %2\nTriangles/LOD: %3\nVertices/LOD: %4\nSubMeshes/LOD: %5\nMeshSize/LOD: %6 (kB)\n"
            "BBox.X: %7\nBBox.Y: %8\nBBox.Z: %9\nFilesize: %10 kB\nTextures: %11\nMaterials: %d\nTags: %12\n")
            .arg(m_strRelativePath)
            .arg(m_lodCount)
            .arg(lodTris)
            .arg(lodVerts)
            .arg(lodSubs)
            .append(lodMeshSize)
            .arg(m_aabb.GetSize().x, 4, 'f').arg(m_aabb.GetSize().y, 4, 'f').arg(m_aabb.GetSize().z, 4, 'f')
            .arg(m_nFileSize / 1024.0f, 2, 'f')
            .arg(m_dependencies["Textures"].size())
            .arg(m_mtlCount)
            .arg(tagsText);
    m_toolTipOneLineText = QString::fromLatin1("[ Tris/LOD: %1 ]").arg(lodTris);
}

void CAssetCharacterItem::CheckIfItsLod()
{
    m_bIsLod = m_strFilename.contains(QStringLiteral("_lod"));
}

void CAssetCharacterItem::ToXML(XmlNodeRef& node) const
{
    node->setTag("Character");
    QString fileName = m_strRelativePath + m_strFilename;
    node->setAttr("fileName", fileName.toUtf8().data());
    XmlNodeRef aabbNode = node->newChild("aabb");
    aabbNode->setAttr("min", m_aabb.min);
    aabbNode->setAttr("max", m_aabb.max);

    node->setAttr("lodCount", m_lodCount);
    node->setAttr("triangleCount", m_triangleCount);
    node->setAttr("mtlcount", m_mtlCount);
    node->setAttr("vertexCount", m_vertexCount);
    node->setAttr("submeshCount", m_submeshCount);
    node->setAttr("nRef", m_nRef);
    node->setAttr("physicsTriCount", m_physicsTriCount);
    node->setAttr("physicsSize", m_physicsSize);
    node->setAttr("bSplitLODs", m_bSplitLODs);
    node->setAttr("textureSize", m_textureSize);

    for (int i = 0; i < MAX_STATOBJ_LODS_NUM; ++i)
    {
        XmlNodeRef triCountLODnode = node->newChild("triCountLOD");
        triCountLODnode->setAttr("lod", i);
        triCountLODnode->setAttr("triCount", m_triCountLOD[i]);
    }
}

void CAssetCharacterItem::FromXML(const XmlNodeRef& node)
{
    assert(node->isTag("Character"));

    if (node->isTag("Character") == false)
    {
        return;
    }

    XmlNodeRef aabbNode = node->findChild("aabb");

    if (aabbNode == NULL)
    {
        assert(0);
        return;
    }

    aabbNode->getAttr("min", m_aabb.min);
    aabbNode->getAttr("max", m_aabb.max);
    node->getAttr("lodCount", m_lodCount);
    node->getAttr("triangleCount", m_triangleCount);
    node->getAttr("mtlcount", m_mtlCount);
    node->getAttr("vertexCount", m_vertexCount);
    node->getAttr("submeshCount", m_submeshCount);
    node->getAttr("nRef", m_nRef);
    node->getAttr("physicsTriCount", m_physicsTriCount);
    node->getAttr("physicsSize", m_physicsSize);
    node->getAttr("bSplitLODs", m_bSplitLODs);
    node->getAttr("textureSize", m_textureSize);

    m_dependencies.clear();

    for (int i = 0; i < node->getChildCount(); ++i)
    {
        if (strcmp(node->getChild(i)->getTag(), "triCountLOD") == 0)
        {
            XmlNodeRef triCountLODnode = node->getChild(i);
            int index;
            triCountLODnode->getAttr("lod", index);
            triCountLODnode->getAttr("triCount", m_triCountLOD[index]);
        }

        //TODO: load dependencies
        //else if(strcmp(node->getChild(i)->getTag(), "dependency") == 0)
        //{
        //  XmlNodeRef dependencyNode = node->getChild(i);
        //  m_dependencies.push_back(dependencyNode->getAttr("path"));
        //}
    }

    PrepareThumbTextInfo();
    SetFlag(eFlag_Cached);
}

void CAssetCharacterItem::ResetView()
{
    m_camZoom = 1.0f;
    m_translateX = 0.0f;
    m_translateY = 0.0f;
    m_rotationX = AssetBrowser::kDefaultCharacterRotationAngleX;
    m_rotationY = AssetBrowser::kDefaultCharacterRotationAngleY;
    CalculateCameraPosition();
}

void CAssetCharacterItem::SetAssetCharacterPreviewCtrl(CPreviewModelCtrl* pView)
{
    s_pPreviewCtrl = pView;
}

CPreviewModelCtrl* CAssetCharacterItem::GetAssetCharacterPreviewCtrl()
{
    return s_pPreviewCtrl;
}
