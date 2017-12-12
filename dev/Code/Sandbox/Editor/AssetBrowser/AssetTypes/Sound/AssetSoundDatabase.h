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

// Description : The sound asset database class, which holds and searches
//               for sound asset in the game data folder


#ifndef CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_SOUND_ASSETSOUNDDATABASE_H
#define CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_SOUND_ASSETSOUNDDATABASE_H
#pragma once
#include "AssetBrowser/AssetBrowserCommon.h"

class CAssetSoundDatabase
    : public CAssetItemDatabase
    , public IClassDesc
{
public:
    CAssetSoundDatabase();
    ~CAssetSoundDatabase();
    void PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db);
    void FreeData();
    void Refresh();
    const char* GetDatabaseName() const;
    const char* GetSupportedExtensions() const;
    const char* GetTransactionFilename() const;
    QWidget* CreateDbFilterDialog(QWidget* pParent, IAssetViewer* pViewerCtrl) override;
    void UpdateDbFilterDialogUI(QWidget* pDlg) override;
    void CollectCachedEventgroup(XmlNodeRef& gr, const QString& Block, const QString& Path, int level);

    //////////////////////////////////////////////////////////////////////////
    // From IClassDesc
    //////////////////////////////////////////////////////////////////////////
    virtual ESystemClassID SystemClassID()
    {
        return ESYSTEM_CLASS_ASSET_DISPLAY;
    };
    REFGUID ClassID()
    {
        // {31491504-39F5-4D85-9049-15940B0CCA0C}
        static const GUID guid =
        {
            0x31491504, 0x39f5, 0x4d85, { 0x90, 0x49, 0x15, 0x94, 0xb, 0xc, 0xca, 0xc }
        };

        return guid;
    }

    virtual QString ClassName()
    {
        return "Asset Item Sound";
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

#endif // CRYINCLUDE_EDITOR_ASSET_BROWSER_ASSET_TYPES_SOUND_ASSETSOUNDDATABASE_H
