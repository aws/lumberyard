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

// Description : Implementation of AssetSoundItem.h


#include "StdAfx.h"
#include "AssetSoundItem.h"
#include "Util/MemoryBlock.h"
#include "Util/Image.h"
#include "Util/ImageUtil.h"
#include "Util/PathUtil.h"
#include "Include/IAssetItemDatabase.h"
#include "IRenderer.h"
#include "Include/IAssetViewer.h"
#include "ImageExtensionHelper.h"
#include "Objects/EntityObject.h"

#include <QVariant>

namespace AssetBrowser
{
    const char* kSoundImageFilename = "Editor/UI/Icons/asset_sound.png";
};

QImage CAssetSoundItem::s_uncachedSoundThumbBmp;

CAssetSoundItem::CAssetSoundItem()
    : CAssetItem()
{
    // Audio: probably very obsolete!
    //m_soundId = 0;
    m_nSoundLengthMsec = 0;
    m_bLoopingSound = false;
    m_flags = eFlag_UseGdiRendering;
    m_flags |= eFlag_CanBeDraggedInViewports | eFlag_CanBeMovedAfterDroppedIntoViewport;

    if (s_uncachedSoundThumbBmp.isNull())
    {
        s_uncachedSoundThumbBmp.load(AssetBrowser::kSoundImageFilename);
    }

    m_pUncachedThumbBmp = &s_uncachedSoundThumbBmp;
}

CAssetSoundItem::~CAssetSoundItem()
{
    // empty, call FreeData first
}

HRESULT STDMETHODCALLTYPE CAssetSoundItem::QueryInterface(const IID& riid, void** ppvObj)
{
    if (riid == __uuidof(IAssetItem))
    {
        *ppvObj = this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CAssetSoundItem::AddRef()
{
    return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetSoundItem::Release()
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

QVariant CAssetSoundItem::GetAssetFieldValue(const char* pFieldName) const
{
    if (AssetViewer::IsFieldName(pFieldName, "thumbToolTipText"))
    {
        QString tagsText = CAssetItem::GetAssetFieldValue("tags").toString();

        return QString::fromLatin1("Path: %1\nLength(ms): %2\nLooping: %3\nTags: %4\n")
                   .arg(m_strRelativePath)
                   .arg(m_nSoundLengthMsec)
                   .arg((m_bLoopingSound ? "Yes" : "No"))
                   .arg(tagsText);
    }
    else if (AssetViewer::IsFieldName(pFieldName, "thumbOneLineText") && m_nSoundLengthMsec)
    {
        return QString::fromLatin1("[ Length: %1 ms ]").arg(m_nSoundLengthMsec);
    }
    else if (AssetViewer::IsFieldName(pFieldName, "errors"))
    {
        QString str;

        //TODO: pre-bake in the caching code, as a member
        if (m_nSoundLengthMsec <= 1)
        {
            str += QStringLiteral("WARNING: Sound length (msec) is zero\n");
        }

        return str;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "length"))
    {
        return m_nSoundLengthMsec;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "loopsound"))
    {
        return m_bLoopingSound;
    }

    // else, check if the common fields are requested
    return CAssetItem::GetAssetFieldValue(pFieldName);
}

void CAssetSoundItem::LoadSound()
{
}

bool CAssetSoundItem::Cache()
{
    if (m_flags & eFlag_Cached)
    {
        return true;
    }

    LoadSound();
    ISystem* pSystem = GetISystem();
    SetFlag(eFlag_Cached, true);
    GetOwnerDatabase()->OnMetaDataChange(this);

    if (0 == m_nSoundLengthMsec)
    {
        SetFlag(eFlag_Invalid, true);
    }

    return true;
}

void CAssetSoundItem::OnBeginPreview(QWidget* hQuickPreviewWnd)
{
    LoadSound();
}

void CAssetSoundItem::OnEndPreview()
{
}

void CAssetSoundItem::PreviewRender(
    QWidget* hRenderWindow,
    const QRect& rstViewport,
    int aMouseX, int aMouseY,
    int aMouseDeltaX, int aMouseDeltaY,
    int aMouseWheelDelta, UINT aKeyFlags)
{
}

void* CAssetSoundItem::CreateInstanceInViewport(float aX, float aY, float aZ)
{
    CBaseObject* pObj = GetIEditor()->NewObject("Entity", "SoundEventSpot");

    if (!pObj)
    {
        return NULL;
    }

    QString fullFilename = GetRelativePath() + GetFilename();
    CEntityObject* pObjEnt = (CEntityObject*)pObj;

    pObjEnt->SetEntityPropertyString("soundName", fullFilename);

    GetIEditor()->GetObjectManager()->BeginEditParams(pObj, OBJECT_CREATE);
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedEntities);
    pObj->SetPos(Vec3(aX, aY, aZ));
    GetIEditor()->GetObjectManager()->EndEditParams();

    return pObj;
}

bool CAssetSoundItem::MoveInstanceInViewport(const void* pDraggedObject, float aX, float aY, float aZ)
{
    CBaseObject* pObj = (CBaseObject*)pDraggedObject;

    if (pObj)
    {
        pObj->SetPos(Vec3(aX, aY, aZ));
        return true;
    }

    return false;
}

void CAssetSoundItem::AbortCreateInstanceInViewport(const void* pDraggedObject)
{
    CBaseObject* pObj = (CBaseObject*)pDraggedObject;

    if (pObj)
    {
        GetIEditor()->GetObjectManager()->DeleteObject(pObj);
    }
}

void CAssetSoundItem::OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags)
{
}

void CAssetSoundItem::ToXML(XmlNodeRef& node) const
{
    node->setTag("Sound");
    QString fileName = m_strRelativePath + m_strFilename;
    node->setAttr("fileName", fileName.toUtf8().data());
    node->setAttr("filesize", m_nFileSize);
    node->setAttr("dccFilename", m_strDccFilename.toUtf8().data());
    node->setAttr("length", m_nSoundLengthMsec);
    node->setAttr("loopsound", m_bLoopingSound);
    // we give the timestamp, since the sound is inside fmod projects, not a real file
    node->setAttr("timeStamp", "1");
}

void CAssetSoundItem::FromXML(const XmlNodeRef& node)
{
    assert(node->isTag("Sound"));

    if (node->isTag("Sound") == false)
    {
        return;
    }

    node->getAttr("filesize", m_nFileSize);
    node->getAttr("length", m_nSoundLengthMsec);
    node->getAttr("loopsound", m_bLoopingSound);
    const char* dccstr = NULL;
    node->getAttr("dccFilename", &dccstr);

    if (dccstr)
    {
        m_strDccFilename = dccstr;
    }

    SetFlag(eFlag_Cached);
}
