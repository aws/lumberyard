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

// Description : Header file for the class implementing IAssetDisplay
//               interface. It declares the headers of the actual used
//               functions.

#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MODEL_ASSETMODELDATABASE_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MODEL_ASSETMODELDATABASE_H
#pragma once
#include "AssetBrowser/AssetBrowserCommon.h"

// Description:
//      Implements the database which handles CGF model assets
class CAssetModelDatabase
    : public CAssetItemDatabase
    , public IClassDesc
{
public:
    CAssetModelDatabase();
    ~CAssetModelDatabase();
    void CacheFieldsInfoForAlreadyLoadedAssets();
    void PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db);
    void Refresh();
    const char* GetDatabaseName() const;
    const char* GetSupportedExtensions() const;
    const char* GetTransactionFilename() const;
    QWidget* CreateDbFilterDialog(QWidget* pParent, IAssetViewer* pViewerCtrl) override;
    void UpdateDbFilterDialogUI(QWidget* pDlg) override;

    // from IClassDesc
    virtual ESystemClassID SystemClassID()
    {
        return ESYSTEM_CLASS_ASSET_DISPLAY;
    };
    REFGUID ClassID()
    {
        // {8EA6C7BB-484A-4f84-8E3F-1607A537E6E3}
        static const GUID guid =
        {
            0x8ea6c7bb, 0x484a, 0x4f84, { 0x8e, 0x3f, 0x16, 0x7, 0xa5, 0x37, 0xe6, 0xe3 }
        };
        return guid;
    }

    virtual QString ClassName()
    {
        return "Asset Item Model";
    };
    virtual QString Category()
    {
        return "Asset Item DB";
    };
    virtual void ShowAbout() {};

    // from IUnknown - Inherited through IClassDesc.
    HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MODEL_ASSETMODELDATABASE_H
