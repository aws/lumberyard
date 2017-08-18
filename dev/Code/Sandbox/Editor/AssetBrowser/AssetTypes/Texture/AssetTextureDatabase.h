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

#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_TEXTURE_ASSETTEXTUREDATABASE_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_TEXTURE_ASSETTEXTUREDATABASE_H
#pragma once
#include "AssetBrowser/AssetBrowserCommon.h"

class CAssetTextureDatabase
    : public CAssetItemDatabase
    , public IClassDesc
{
public:
    CAssetTextureDatabase();
    ~CAssetTextureDatabase();

    void CacheFieldsInfoForAlreadyLoadedAssets();
    void PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db);
    void Refresh();
    const char* GetDatabaseName() const;
    const char* GetSupportedExtensions() const;
    const char* GetTransactionFilename() const;
    QWidget* CreateDbFilterDialog(QWidget* pParent, IAssetViewer* pViewerCtrl) override;
    void UpdateDbFilterDialogUI(QWidget* pDlg) override;


    virtual IAssetItem* GetAsset(const char* pAssetFilename);

    //////////////////////////////////////////////////////////////////////////
    // From IClassDesc
    //////////////////////////////////////////////////////////////////////////
    virtual ESystemClassID SystemClassID()
    {
        return ESYSTEM_CLASS_ASSET_DISPLAY;
    };
    REFGUID ClassID()
    {
        // {509EE025-855B-441b-B56E-D4949180968A}
        static const GUID guid = {
            0x509ee025, 0x855b, 0x441b, { 0xb5, 0x6e, 0xd4, 0x94, 0x91, 0x80, 0x96, 0x8a }
        };
        return guid;
    }

    virtual QString ClassName()
    {
        return "Asset Item Texture";
    };
    virtual QString Category()
    {
        return "Asset Item DB";
    };
    virtual void ShowAbout() {};

    //////////////////////////////////////////////////////////////////////////
    // From IUnknown - Inherited through IClassDesc.
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE   QueryInterface(const IID& riid, void** ppvObj);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_TEXTURE_ASSETTEXTUREDATABASE_H
