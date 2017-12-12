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

// Description : Header file for the class implementing IAssetDisplayItem
//               interface  It declares the headers of the actual used
//               functions


#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MODEL_ASSETMODELITEM_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MODEL_ASSETMODELITEM_H
#pragma once
#include "AssetBrowser/AssetBrowserCommon.h"
#include "AssetBrowserPreviewModelDlg.h"
#include "AssetBrowserPreviewModelDlgFooter.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include <IShader.h>
#include "Util/GdiUtil.h"
#include "Controls/PreviewModelCtrl.h"
#include <QPointer>

class CAssetDatabaseModelViewport;

namespace AssetBrowser
{
    static const float kDefaultModelRotationAngleX = 10.0f;
    static const float kDefaultModelRotationAngleY = 70.0f;
    static const float kModelAssetAmbienceMultiplyer = 7.0f;
};

namespace AssetViewer
{
    static const float kInteractiveRenderModelTranslateMultiplier = 0.02f;
    static const float kInteractiveRenderModelRotateMultiplier = 0.02f;
    static const float kInteractiveRenderModelZoomMultiplier = 0.006f;
    static const UINT kModelAssetWireframeToggleKey = 'W';
    static const UINT kModelAssetPhysicsToggleKey = 'P';
    static const UINT kModelAssetNormalsToggleKey = 'N';
    static const UINT kModelAssetStatsToggleKey = 'S';
    static const float kModelPreviewCameraZoomFactor = 0.1f;
    static const int kAssetModelViewportPulloutHeight = 600;
    static const int kAssetModelViewportPulloutWidth = 800;

    static XmlNodeRef s_chrAssetThumbTimeOfDay = NULL;
    static bool s_bChrStaticsInitialized =  false;
}


class CAssetModelItem
    : public CAssetItem
{
public:

    friend class CAssetBrowserPreviewModelDlg;
    friend class CAssetBrowserPreviewModelDlgFooter;

    static const int    kAssetDisplay_MaxThumbImageBufferSize = 2048 * 2048;

    enum EAssetModelItemDragCreationMode
    {
        eAssetModelItemDragCreationMode_AsBrush,
        eAssetModelItemDragCreationMode_AsGeomEntity
    };

    CAssetModelItem();
    ~CAssetModelItem();

    QVariant GetAssetFieldValue(const char* pFieldName) const;
    bool Cache();
    void CacheCurrentThumbAngle();
    void PrepareThumbTextInfo();
    void OnBeginPreview(QWidget* hQuickPreviewWnd) override;
    void OnEndPreview();
    QWidget* GetCustomPreviewPanelHeader(QWidget* pParentWnd) override;
    QWidget* GetCustomPreviewPanelFooter(QWidget* pParentWnd) override;
    void PreviewRender(QWidget* hRenderWindow, const QRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, int aMouseWheelDelta, UINT aKeyFlags) override;
    void OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags);
    bool Render(QWidget* hRenderWindow, const QRect& rstViewport, bool bCacheThumbnail = false);
    void* CreateInstanceInViewport(float aX, float aY, float aZ);
    bool MoveInstanceInViewport(const void* pDraggedObject, float aX, float aY, float aZ);
    void AbortCreateInstanceInViewport(const void* pDraggedObject);
    void CacheFieldsInfoForLoadedStatObj(IStatObj* pStatObj);
    void CheckIfItsLod();
    void ToXML(XmlNodeRef& node) const;
    void FromXML(const XmlNodeRef& node);
    void ResetView();
    bool LoadModel();

    void CreateViewport(QWidget* hRenderWindow, const QRect& rc, bool bThumbnail = false);
    static void SetAssetModelViewport(CPreviewModelCtrl* pView);
    static CPreviewModelCtrl* GetAssetModelViewport();

    // from IUnknown - Inherited through IClassDesc.
    HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

protected:
    void SetCamera(CCamera& cam, const QRect& rcViewportRect);
    void CalculateCameraPosition();
    void DrawTextOnReportImage(QPaintDevice* abm) const;
    void MakeLODsTrisString(QString& rOutValue) const;
    void MakeLODsVertsString(QString& rOutValue) const;
    void MakeLODsSubMeshesString(QString& rOutValue) const;
    void MakeLODsMeshSizeString(QString& rOutValue) const;
    void CacheThumbnail(QWidget* hRenderWindow, const QRect& rc);

    Vec3 m_camTarget;
    float m_camRadius, m_camZoom;
    CCamera m_camera;
    AABB m_aabb;
    Quat m_crtRotation;
    UINT m_lodCount, m_triangleCount, m_vertexCount, m_submeshCount, m_mtlCount;
    UINT m_textureSize, m_nRef, m_physicsTriCount, m_physicsSize;
    UINT m_triCountLOD[MAX_STATOBJ_LODS_NUM];
    UINT m_vertCountLOD[MAX_STATOBJ_LODS_NUM];
    UINT m_subMeshCountLOD[MAX_STATOBJ_LODS_NUM];
    UINT m_meshSizeLOD[MAX_STATOBJ_LODS_NUM];
    IStatObj* m_pObject;
    IRenderer* m_pRenderer;
    ColorF m_clearColor;
    bool m_bRotate;
    bool m_bIsLod;
    bool m_bGrid;
    bool m_bAxis;
    bool m_bShowObject;
    bool m_bSplitLODs;
    bool m_bCameraUpdate;
    bool m_bCacheCurrentThumbAngle;
    float m_rotateAngle;
    float m_fov, m_rotationX, m_rotationY, m_translateX, m_translateY;
    EAssetModelItemDragCreationMode m_dragCreationMode;

    static QImage s_uncachedModelThumbBmp;
    static bool s_bWireframe;
    static bool s_bPhysics;
    static bool s_bNormals;
    static float s_fAmbience;
    static UINT s_frameBufferScreenshot[kAssetDisplay_MaxThumbImageBufferSize];
    static QPointer<CAssetBrowserPreviewModelDlg> s_modelPreviewDlgHeader;
    static QPointer<CAssetBrowserPreviewModelDlgFooter> s_modelPreviewDlgFooter;
    static CPreviewModelCtrl* s_pPreviewCtrl;
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MODEL_ASSETMODELITEM_H
