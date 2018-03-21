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

// Description : Implementation of AssetMaterialItem.h

#include "StdAfx.h"
#include "AssetMaterialItem.h"
#include "Util/MemoryBlock.h"
#include "Util/Image.h"
#include "Util/ImageUtil.h"
#include "Util/PathUtil.h"
#include "Include/IAssetItemDatabase.h"
#include "IMaterial.h"
#include "IRenderer.h"
#include "Include/IAssetViewer.h"
#include "ImageExtensionHelper.h"
#include "Controls/PreviewModelCtrl.h"
#include "IEditor.h"
#include "IStreamEngine.h"
#include "Material/MaterialManager.h"
#include "AssetBrowser/AssetBrowserDialog.h"

namespace AssetBrowser
{
    const char* kMaterialImageFilename = "Editor/UI/Icons/asset_material.png";
    const char* kMaterialPreviewBackTexture = "Editor/Materials/Stripes.dds";
    const char* kMaterialPreviewModelFile = "Editor/Objects/MtlSphere.cgf";
};

QImage CAssetMaterialItem::s_uncachedMaterialThumbBmp;
CPreviewModelCtrl* CAssetMaterialItem::s_pPreviewCtrl = NULL;

CAssetMaterialItem::CAssetMaterialItem()
    : CAssetItem()
{
    m_flags = eFlag_UseGdiRendering;

    if (s_uncachedMaterialThumbBmp.isNull())
    {
        s_uncachedMaterialThumbBmp.load(AssetBrowser::kMaterialImageFilename);
    }

    m_pUncachedThumbBmp = &s_uncachedMaterialThumbBmp;
    m_pMaterial = NULL;
}

CAssetMaterialItem::~CAssetMaterialItem()
{
    // empty, call FreeData first
}

HRESULT STDMETHODCALLTYPE CAssetMaterialItem::QueryInterface(const IID& riid, void** ppvObj)
{
    if (riid == __uuidof(IAssetItem))
    {
        *ppvObj = this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CAssetMaterialItem::AddRef()
{
    return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetMaterialItem::Release()
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

QVariant CAssetMaterialItem::GetAssetFieldValue(const char* pFieldName) const
{
    if (AssetViewer::IsFieldName(pFieldName, "thumbToolTipText"))
    {
        QString tagsText = CAssetItem::GetAssetFieldValue("tags").toString();
        return QString::fromLatin1("Path: %1\nTags: %2\n").arg(m_strRelativePath, tagsText);
    }
    else if (AssetViewer::IsFieldName(pFieldName, "thumbOneLineText"))
    {
        stack_string str;

        //TODO: pre-bake in the caching code, as a member
        str.Format("");
        return QString(str.c_str());
    }
    else if (AssetViewer::IsFieldName(pFieldName, "errors"))
    {
        return QString();
    }

    // else, check if the common fields are requested
    return CAssetItem::GetAssetFieldValue(pFieldName);
}

void CAssetMaterialItem::LoadMaterial()
{
    if (!m_pMaterial)
    {
        QString fullPath = m_strRelativePath + m_strFilename;
        m_pMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(fullPath.toUtf8().data(), false);
    }
}

bool CAssetMaterialItem::Cache()
{
    if (IsFlagSet(eFlag_Cached))
    {
        return true;
    }

    LoadMaterial();

    Render(
        &CAssetBrowserDialog::Instance()->GetAssetViewer(),
        QRect(0, 0, gSettings.sAssetBrowserSettings.nThumbSize, gSettings.sAssetBrowserSettings.nThumbSize),
        true);
    CAssetItem::Cache();
    m_cachedThumbBmp = QImage();

    SetFlag(eFlag_Cached, true);
    GetOwnerDatabase()->OnMetaDataChange(this);

    if (m_pMaterial)
    {
        m_pMaterial = 0;
    }

    return true;
}

void CAssetMaterialItem::OnBeginPreview(QWidget* hQuickPreviewWnd)
{
    LoadMaterial();

    if (!s_pPreviewCtrl)
    {
        hQuickPreviewWnd->setLayout(new QHBoxLayout);
        hQuickPreviewWnd->layout()->setMargin(0);

        s_pPreviewCtrl = new CPreviewModelCtrl(hQuickPreviewWnd);
        s_pPreviewCtrl->SetGrid(false);
        s_pPreviewCtrl->SetAxis(false);
        s_pPreviewCtrl->SetBackgroundTexture(AssetBrowser::kMaterialPreviewBackTexture);
        s_pPreviewCtrl->SetClearColor(ColorF(0, 0, 0));
        s_pPreviewCtrl->LoadFile(AssetBrowser::kMaterialPreviewModelFile);

        hQuickPreviewWnd->layout()->addWidget(s_pPreviewCtrl);
        s_pPreviewCtrl->show();
    }

    s_pPreviewCtrl->SetMaterial(m_pMaterial);
}

void CAssetMaterialItem::OnEndPreview()
{
    m_hPreviewWidget = 0;
    delete s_pPreviewCtrl, s_pPreviewCtrl = nullptr;
}

void CAssetMaterialItem::PreviewRender(
    QWidget* hRenderWindow,
    const QRect& rstViewport,
    int aMouseX, int aMouseY,
    int aMouseDeltaX, int aMouseDeltaY,
    int aMouseWheelDelta, UINT aKeyFlags)
{
}

bool CAssetMaterialItem::Render(QWidget* hRenderWindow, const QRect& rstViewport, bool bCacheThumbnail)
{
    if (!m_pMaterial)
    {
        return false;
    }

    if (!s_pPreviewCtrl)
    {
        s_pPreviewCtrl = new CPreviewModelCtrl(hRenderWindow);
        s_pPreviewCtrl->SetGrid(false);
        s_pPreviewCtrl->SetAxis(false);
        s_pPreviewCtrl->SetBackgroundTexture(AssetBrowser::kMaterialPreviewBackTexture);
        s_pPreviewCtrl->SetClearColor(ColorF(0, 0, 0));
        s_pPreviewCtrl->LoadFile(AssetBrowser::kMaterialPreviewModelFile);
        s_pPreviewCtrl->show();
    }
    else
    {
        s_pPreviewCtrl->setParent(hRenderWindow);
    }

    s_pPreviewCtrl->FitToScreen();
    s_pPreviewCtrl->SetMaterial(m_pMaterial);
    s_pPreviewCtrl->setGeometry(rstViewport);

    if (bCacheThumbnail)
    {
        CacheThumbnail(hRenderWindow, rstViewport);
    }

    return true;
}

void CAssetMaterialItem::CacheThumbnail(QWidget* hRenderWindow, const QRect& rc)
{
    if (s_pPreviewCtrl)
    {
        s_pPreviewCtrl->show();
        gEnv->p3DEngine->Update();
        gEnv->pSystem->GetStreamEngine()->Update();
        s_pPreviewCtrl->Update(true);
        s_pPreviewCtrl->repaint();
        s_pPreviewCtrl->FitToScreen();

        SaveThumbnail(s_pPreviewCtrl);

        m_flags |= eFlag_ThumbnailLoaded;

        delete s_pPreviewCtrl;
        s_pPreviewCtrl = nullptr;
    }
}

void CAssetMaterialItem::OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags)
{
}

void CAssetMaterialItem::ToXML(XmlNodeRef& node) const
{
    node->setTag("Material");
    QString fileName = m_strRelativePath + m_strFilename;
    node->setAttr("fileName", fileName.toUtf8().data());
    node->setAttr("filesize", m_nFileSize);
    node->setAttr("dccFilename", m_strDccFilename.toUtf8().data());
}

void CAssetMaterialItem::FromXML(const XmlNodeRef& node)
{
    assert(node->isTag("Material"));

    if (node->isTag("Material") == false)
    {
        return;
    }

    node->getAttr("filesize", m_nFileSize);
    const char* dccstr = NULL;
    node->getAttr("dccFilename", &dccstr);

    if (dccstr)
    {
        m_strDccFilename = dccstr;
    }

    SetFlag(eFlag_Cached);
}
