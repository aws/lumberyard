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

// Description : Implements AssetMaterialDatabase.h


#include "stdafx.h"
#include "AssetMaterialDatabase.h"
#include "AssetMaterialItem.h"
#include "IMaterial.h"
#include "IRenderer.h"
#include "ImageExtensionHelper.h"
#include "Include/IAssetViewer.h"
#include "StringUtils.h"
#include "Util/IndexedFiles.h"
#include "MaterialAssetDbFilterDlg.h"

REGISTER_CLASS_DESC(CAssetMaterialDatabase);

CAssetMaterialDatabase::CAssetMaterialDatabase()
    : CAssetItemDatabase()
{
    // add fields
    static const int kFilenameColWidth = 150;
    static const int kDccFilenameColWidth = 50;
    static const int kFileSizeColWidth = 50;
    static const int kRelativePathColWidth = 50;
    static const int kUsedInLevelColWidth = 40;
    static const int kLoadedInLevelColWidth = 40;
    static const int kTagsColWidth = 60;

    m_assetFields.push_back(SAssetField("filename", "Filename", SAssetField::eType_String, kFilenameColWidth));
    m_assetFields.push_back(SAssetField("relativepath", "Path", SAssetField::eType_String, kRelativePathColWidth));
    m_assetFields.push_back(SAssetField("usedinlevel", "Used in level", SAssetField::eType_Bool, kUsedInLevelColWidth));
    m_assetFields.push_back(SAssetField("loadedinlevel", "Loaded in level", SAssetField::eType_Bool, kLoadedInLevelColWidth));
    m_assetFields.push_back(SAssetField("dccfilename", "DCC Filename", SAssetField::eType_String, kDccFilenameColWidth));
    m_assetFields.push_back(SAssetField("tags", "Tags", SAssetField::eType_String, kTagsColWidth));
}

CAssetMaterialDatabase::~CAssetMaterialDatabase()
{
    // empty, call FreeData first
}

void CAssetMaterialDatabase::PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db)
{
    assert(db->isTag(GetDatabaseName()));
    int foundAssets = 0;

    for (int i = 0; i < db->getChildCount(); ++i)
    {
        XmlNodeRef entry = db->getChild(i);
        const char* fileName = entry->getAttr("fileName");
        TFilenameAssetMap::iterator assetIt = m_assets.find(fileName);
        bool bAssetFound = m_assets.end() != assetIt;

        if (bAssetFound)
        {
            assetIt->second->FromXML(entry);
            foundAssets++;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CAssetMaterialDatabase::QueryInterface(const IID& riid, void** ppvObj)
{
    if (riid == __uuidof(IAssetItemDatabase))
    {
        *ppvObj = this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CAssetMaterialDatabase::AddRef()
{
    return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetMaterialDatabase::Release()
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

void CAssetMaterialDatabase::FreeData()
{
    CAssetItemDatabase::FreeData();
}

const char* CAssetMaterialDatabase::GetSupportedExtensions() const
{
    return "mtl";
};

const char* CAssetMaterialDatabase::GetTransactionFilename() const
{
    return "materialAssetTransactions.xml";
}

QWidget* CAssetMaterialDatabase::CreateDbFilterDialog(QWidget* pParent, IAssetViewer* pViewerCtrl)
{
    auto pDlg = new CMaterialAssetDbFilterDlg(pParent);
    pDlg->SetAssetViewer(pViewerCtrl);

    return pDlg;
}

void CAssetMaterialDatabase::UpdateDbFilterDialogUI(QWidget* pDlg)
{
    CMaterialAssetDbFilterDlg* pFilterDlg = (CMaterialAssetDbFilterDlg*)pDlg;

    pFilterDlg->UpdateFilterUI();
}

const char* CAssetMaterialDatabase::GetDatabaseName() const
{
    return "Materials";
}

void CAssetMaterialDatabase::Refresh()
{
    FreeData();

    QString strExtension;
    IFileUtil::FileArray cFiles;
    int nTotalFiles = 0;
    int nCurrentFile = 0;
    QString strIntermediateFilename;
    QString strOutputMaterialName, strPathOnly, strFileNameOnly;
    CAssetMaterialItem* poMaterialDatabaseItem = NULL;

    // search for Material files
    QStringList tags;

    tags.push_back("mtl");
    CIndexedFiles::GetDB().GetFilesWithTags(cFiles, tags);

    nTotalFiles = cFiles.size();

    for (nCurrentFile = 0; nCurrentFile < nTotalFiles; ++nCurrentFile)
    {
        IFileUtil::FileDesc& rstFileDescriptor = cFiles[nCurrentFile];

        // if not a real .mtl file
        if (Path::GetExt(rstFileDescriptor.filename) != "mtl")
        {
            continue;
        }

        strIntermediateFilename = rstFileDescriptor.filename.toLower();
        strOutputMaterialName = strIntermediateFilename;
        Path::ConvertBackSlashToSlash(strOutputMaterialName);
        strFileNameOnly = Path::GetFile(strOutputMaterialName);
        strPathOnly = Path::GetPath(strOutputMaterialName);

        poMaterialDatabaseItem = new CAssetMaterialItem();

        if (!poMaterialDatabaseItem)
        {
            return;
        }

        poMaterialDatabaseItem->SetFileSize(rstFileDescriptor.size);
        poMaterialDatabaseItem->SetFilename(strFileNameOnly.toUtf8().data());
        poMaterialDatabaseItem->SetRelativePath(strPathOnly.toUtf8().data());
        poMaterialDatabaseItem->SetOwnerDatabase(this);
        poMaterialDatabaseItem->SetFileExtension(strExtension.toUtf8().data());
        poMaterialDatabaseItem->SetFlag(IAssetItem::eFlag_Visible, true);
        poMaterialDatabaseItem->SetHash(AssetBrowser::HashStringSbdm(strOutputMaterialName.toUtf8().data()));
        m_assets[strOutputMaterialName] = poMaterialDatabaseItem;
    }
}