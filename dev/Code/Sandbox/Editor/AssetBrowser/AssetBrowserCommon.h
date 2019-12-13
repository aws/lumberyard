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

// Description : Common code interface implementation for the asset database

#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERCOMMON_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERCOMMON_H
#pragma once
#include <QtGlobal>
#include "Include/IAssetItemDatabase.h"
#include "Include/IAssetItem.h"
#include "Include/IAssetViewer.h"
#include "AssetBrowserManager.h"
#include "Util/GdiUtil.h"

class CPreviewModelCtrl;

namespace AssetViewer
{
    const int kMinWidthForOverlayText = 256;
    const int kOverlayTextTopMargin = 10;
    const int kOverlayTextLeftMargin = 10;
};

namespace AssetBrowserCommon
{
    extern const char* kThumbnailsRoot;
}

// Description:
//      The base implementation for the IAssetItemDatabase interface
class CAssetItemDatabase
    : public IAssetItemDatabase
{
public:
    CAssetItemDatabase();
    virtual ~CAssetItemDatabase();
    virtual void PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db);
    virtual void FreeData();
    virtual void Refresh();
    virtual const char* GetDatabaseName() const;
    virtual const char* GetSupportedExtensions() const;
    virtual TAssetFields& GetAssetFields();
    virtual SAssetField* GetAssetFieldByName(const char* pFieldName);
    virtual TFilenameAssetMap& GetAssets();
    virtual IAssetItem* GetAsset(const char* pAssetFilename);
    virtual void ApplyFilters(const TAssetFieldFiltersMap& rFieldFilters);
    virtual void ClearFilters();
    virtual QWidget* CreateDbFilterDialog(QWidget* pParent, IAssetViewer* pViewerCtrl) override;
    virtual void UpdateDbFilterDialogUI(QWidget* pDlg) override;
    virtual void OnAssetBrowserOpen(){}
    virtual void OnAssetBrowserClose(){}
    virtual const char* GetTransactionFilename() const;
    virtual bool AddMetaDataChangeListener(MetaDataChangeListener callBack);
    virtual bool RemoveMetaDataChangeListener(MetaDataChangeListener callBack);
    virtual void OnMetaDataChange(const IAssetItem* pAssetItem);
    virtual HRESULT STDMETHODCALLTYPE   QueryInterface(const IID& riid, void** ppvObj);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

protected:
    ULONG m_ref;
    TFilenameAssetMap m_assets;
    TAssetFields m_assetFields;
    std::vector<MetaDataChangeListener> m_metaDataChangeListeners;
};

// Description:
//      Base implementation of the IAssetItem interface
class CAssetItem
    : public IAssetItem
{
public:
    CAssetItem();
    virtual ~CAssetItem();
    virtual uint32 GetHash() const;
    virtual void SetHash(uint32 hash);
    virtual IAssetItemDatabase* GetOwnerDatabase() const;
    virtual void SetOwnerDatabase(IAssetItemDatabase* piOwnerDatabase);
    virtual const TAssetDependenciesMap& GetDependencies() const;
    virtual void SetFileSize(quint64 aSize);
    virtual quint64 GetFileSize() const;
    virtual void SetFilename(const char* pName);
    virtual QString GetFilename() const;
    virtual void SetRelativePath(const char* pPath);
    virtual QString GetRelativePath() const;
    virtual void SetFileExtension(const char* pExt);
    virtual QString GetFileExtension() const;
    virtual UINT GetFlags() const;
    virtual void SetFlags(UINT aFlags);
    virtual void SetFlag(EAssetFlags aFlag, bool bSet = true);
    virtual bool IsFlagSet(EAssetFlags aFlag) const;
    virtual void SetIndex(UINT aIndex);
    virtual UINT GetIndex() const;
    virtual QVariant GetAssetFieldValue(const char* pFieldName) const;
    virtual bool SetAssetFieldValue(const char* pFieldName, void* pSrc);
    virtual void GetDrawingRectangle(QRect& rstDrawingRectangle) const;
    virtual void SetDrawingRectangle(const QRect& crstDrawingRectangle);
    virtual bool HitTest(int nX, int nY) const;
    virtual bool HitTest(const QRect& roTestRect) const;
    virtual void* CreateInstanceInViewport(float aX, float aY, float aZ);
    virtual bool MoveInstanceInViewport(const void* pDraggedObject, float aNewX, float aNewY, float aNewZ);
    virtual void AbortCreateInstanceInViewport(const void* pDraggedObject);
    virtual bool Cache();
    virtual bool ForceCache();
    virtual bool LoadThumbnail();
    virtual void UnloadThumbnail();
    virtual void FreeData();
    virtual void OnBeginPreview(QWidget* hQuickPreviewWnd);
    virtual void OnEndPreview();
    virtual QWidget* GetCustomPreviewPanelHeader(QWidget* pParentWnd);
    virtual QWidget* GetCustomPreviewPanelFooter(QWidget* pParentWnd);
    virtual void PreviewRender(
        QWidget* hRenderWindow,
        const QRect& rstViewport,
        int aMouseX, int aMouseY,
        int aMouseDeltaX, int aMouseDeltaY,
        int aMouseWheelDelta, UINT aKeyFlags);
    virtual void OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags);
    virtual void OnThumbClick(const QPoint& point, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    virtual void OnThumbDblClick(const QPoint& point, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    virtual bool DrawThumbImage(QPainter* painter, const QRect& rRect);
    virtual void ToXML(XmlNodeRef& node) const;
    virtual void FromXML(const XmlNodeRef& node);
    virtual HRESULT STDMETHODCALLTYPE   QueryInterface(const IID& riid, void** ppvObj);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

protected:
    virtual void DrawTextOnReportImage(QPaintDevice* abm) const;
    void SaveThumbnail(CPreviewModelCtrl* pPreviewCtrl);

    ULONG m_ref;
    uint64 m_hash;
    QString m_strFilename;
    QString m_strDccFilename;
    QString m_strExtension;
    QString m_strRelativePath;
    quint64 m_nFileSize;
    volatile UINT m_flags;
    QRect m_oDrawingRectangle;
    QImage m_cachedThumbBmp;
    QImage* m_pUncachedThumbBmp;
    IAssetItemDatabase* m_pOwnerDatabase;
    QWidget* m_hPreviewWidget;
    UINT m_assetIndex;
    TAssetDependenciesMap m_dependencies;
    QString m_errorText, m_toolTipText, m_toolTipOneLineText;
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSETBROWSERCOMMON_H
