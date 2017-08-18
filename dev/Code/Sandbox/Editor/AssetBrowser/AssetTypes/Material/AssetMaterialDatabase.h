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

// Description : The Material asset database class, which holds and searches
//               for Material asset in the game data folder


#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MATERIAL_ASSETMATERIALDATABASE_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MATERIAL_ASSETMATERIALDATABASE_H
#pragma once
#include "AssetBrowser/AssetBrowserCommon.h"

class CAssetMaterialDatabase
    : public CAssetItemDatabase
    , public IClassDesc
{
public:
    CAssetMaterialDatabase();
    ~CAssetMaterialDatabase();
    void PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db);
    void FreeData();
    void Refresh();
    const char* GetDatabaseName() const;
    const char* GetSupportedExtensions() const;
    const char* GetTransactionFilename() const;
    QWidget* CreateDbFilterDialog(QWidget* pParent, IAssetViewer* pViewerCtrl) override;
    void UpdateDbFilterDialogUI(QWidget* pDlg) override;

    //////////////////////////////////////////////////////////////////////////
    // From IClassDesc
    //////////////////////////////////////////////////////////////////////////
    virtual ESystemClassID SystemClassID()
    {
        return ESYSTEM_CLASS_ASSET_DISPLAY;
    };
    REFGUID ClassID()
    {
        // {C669476D-AB5D-4CB5-808B-8FAC7019DE30}
        static const GUID guid =
        {
            0xc669476d, 0xab5d, 0x4cb5, { 0x80, 0x8b, 0x8f, 0xac, 0x70, 0x19, 0xde, 0x30 }
        };
        return guid;
    }

    virtual QString ClassName()
    {
        return "Asset Item Material";
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

protected:
};
#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_MATERIAL_ASSETMATERIALDATABASE_H
