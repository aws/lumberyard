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

// Description : Implementation of AssetTextureDatabase.h

#include "StdAfx.h"
#include "AssetTextureDatabase.h"
#include "AssetTextureItem.h"
#include "ITexture.h"
#include "IRenderer.h"
#include "ImageExtensionHelper.h"
#include "Include/IAssetViewer.h"
#include "StringUtils.h"
#include "Util/IndexedFiles.h"
#include "TextureAssetDbFilterDlg.h"

REGISTER_CLASS_DESC(CAssetTextureDatabase);

CAssetTextureDatabase::CAssetTextureDatabase()
    : CAssetItemDatabase()
{
    // add fields
    static const int kFilenameColWidth = 150;
    static const int kDccFilenameColWidth = 50;
    static const int kFileSizeColWidth = 50;
    static const int kRelativePathColWidth = 50;
    static const int kWidthColWidth = 50;
    static const int kHeightColWidth = 50;
    static const int kMipsColWidth = 50;
    static const int kFormatColWidth = 100;
    static const int kTypeColWidth = 50;
    static const int kUsedInLevelColWidth = 40;
    static const int kLoadedInLevelColWidth = 40;
    static const int kTagsColWidth = 60;

    m_assetFields.push_back(SAssetField("filename", "Filename", SAssetField::eType_String, kFilenameColWidth));
    m_assetFields.push_back(SAssetField("filesize", "Filesize (Bytes)", SAssetField::eType_Int32, kFileSizeColWidth));
    m_assetFields.push_back(SAssetField("relativepath", "Path", SAssetField::eType_String, kRelativePathColWidth));
    m_assetFields.push_back(SAssetField("width", "Width", SAssetField::eType_Int32, kWidthColWidth));
    m_assetFields.push_back(SAssetField("height", "Height", SAssetField::eType_Int32, kHeightColWidth));
    m_assetFields.push_back(SAssetField("mips", "MipCount", SAssetField::eType_Int32, kMipsColWidth));
    m_assetFields.push_back(SAssetField("format", "Format", SAssetField::eType_String, kFormatColWidth));
    m_assetFields.push_back(SAssetField("type", "Type", SAssetField::eType_String, kTypeColWidth));
    m_assetFields.push_back(SAssetField("texture_usage", "Usage", SAssetField::eType_String, kTypeColWidth));
    m_assetFields.push_back(SAssetField("usedinlevel", "Used in level", SAssetField::eType_Bool, kUsedInLevelColWidth));
    m_assetFields.push_back(SAssetField("loadedinlevel", "Loaded in level", SAssetField::eType_Bool, kLoadedInLevelColWidth));
    m_assetFields.push_back(SAssetField("dccfilename", "DCC Filename", SAssetField::eType_String, kDccFilenameColWidth));
    m_assetFields.push_back(SAssetField("tags", "Tags", SAssetField::eType_String, kTagsColWidth));

    {
        SAssetField* pField = GetAssetFieldByName("width");

        pField->m_bUseEnumValues = true;
        pField->m_enumValues.push_back("4");
        pField->m_enumValues.push_back("8");
        pField->m_enumValues.push_back("16");
        pField->m_enumValues.push_back("32");
        pField->m_enumValues.push_back("64");
        pField->m_enumValues.push_back("128");
        pField->m_enumValues.push_back("256");
        pField->m_enumValues.push_back("512");
        pField->m_enumValues.push_back("1024");
        pField->m_enumValues.push_back("2048");
    }

    {
        SAssetField* pField = GetAssetFieldByName("height");

        pField->m_bUseEnumValues = true;
        pField->m_enumValues.push_back("4");
        pField->m_enumValues.push_back("8");
        pField->m_enumValues.push_back("16");
        pField->m_enumValues.push_back("32");
        pField->m_enumValues.push_back("64");
        pField->m_enumValues.push_back("128");
        pField->m_enumValues.push_back("256");
        pField->m_enumValues.push_back("512");
        pField->m_enumValues.push_back("1024");
        pField->m_enumValues.push_back("2048");
    }

    {
        SAssetField* pField = GetAssetFieldByName("mips");

        pField->m_bUseEnumValues = true;
        pField->m_enumValues.push_back("1");
        pField->m_enumValues.push_back("2");
        pField->m_enumValues.push_back("3");
        pField->m_enumValues.push_back("4");
        pField->m_enumValues.push_back("5");
        pField->m_enumValues.push_back("6");
        pField->m_enumValues.push_back("7");
        pField->m_enumValues.push_back("8");
        pField->m_enumValues.push_back("9");
        pField->m_enumValues.push_back("10");
    }
}

CAssetTextureDatabase::~CAssetTextureDatabase()
{
    // emtpty, call FreeData() first
}

HRESULT STDMETHODCALLTYPE CAssetTextureDatabase::QueryInterface(const IID& riid, void** ppvObj)
{
    if (riid == __uuidof(IAssetItemDatabase))
    {
        *ppvObj = this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CAssetTextureDatabase::AddRef()
{
    return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetTextureDatabase::Release()
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

const char* CAssetTextureDatabase::GetSupportedExtensions() const
{
    return "dds,tif,bmp,gif,jpg,jpeg,jpe,tga,png";
};

const char* CAssetTextureDatabase::GetDatabaseName() const
{
    return "Textures";
}

IAssetItem* CAssetTextureDatabase::GetAsset(const char* pAssetFilename)
{
    IAssetItem* asset = CAssetItemDatabase::GetAsset(pAssetFilename);
    if (asset)
    {
        return asset;
    }

    // ensure that the input file is an image, in case this db is being queried for
    // a material or some other asset with the same name as a texture
    if (IResourceCompilerHelper::IsSourceImageFormatSupported(pAssetFilename))
    {
        static const int numFormats = IResourceCompilerHelper::GetNumEngineImageFormats();
        for (int formatIdx = 0; formatIdx < numFormats; ++formatIdx)
        {
            const char* formatExt = IResourceCompilerHelper::GetEngineImageFormat(formatIdx, false);
            QString texFilename = Path::ReplaceExtension(pAssetFilename, formatExt);
            asset = CAssetItemDatabase::GetAsset(texFilename.toUtf8().data());
            if (asset)
            {
                break;
            }
        }
    }

    return asset;
}

void CAssetTextureDatabase::Refresh()
{
    FreeData();

    QString szTextureName;
    QString strExtension;
    QString strFilename;
    IFileUtil::FileArray cFiles;
    int nTotalFiles = 0;
    int nCurrentFile = 0;
    QString strIntermediateFilename;
    QString strOutputTextureName, strPathOnly, strFileNameOnly;
    CCryFile file;
    CAssetTextureItem* poTextureDatabaseItem = NULL;

    // search for DDS files
    QStringList tags;

    for (unsigned int i = 0; i < IResourceCompilerHelper::GetNumEngineImageFormats(); ++i)
    {
        tags.push_back(IResourceCompilerHelper::GetEngineImageFormat(i, false));
    }
    CIndexedFiles::GetDB().GetFilesWithTags(cFiles, tags);

    // for every file, check if is a texture...
    nTotalFiles = cFiles.size();

    for (nCurrentFile = 0; nCurrentFile < nTotalFiles; ++nCurrentFile)
    {
        IFileUtil::FileDesc& rstFileDescriptor = cFiles[nCurrentFile];
        strIntermediateFilename = rstFileDescriptor.filename.toLower();
        Path::ConvertBackSlashToSlash(strIntermediateFilename);
        strOutputTextureName = strIntermediateFilename;

        if (Path::GetExt(strOutputTextureName) != "dds")
        {
            continue;
        }

        // no need for temp files.
        if (rstFileDescriptor.filename.contains("~~~TempFile~~~"))
        {
            continue;
        }

        poTextureDatabaseItem = new CAssetTextureItem();

        if (!poTextureDatabaseItem)
        {
            return;
        }

        strFileNameOnly = Path::GetFile(strOutputTextureName);
        strPathOnly = Path::GetPath(strOutputTextureName);
        strExtension = Path::GetExt(strOutputTextureName);
        poTextureDatabaseItem->SetFileSize(rstFileDescriptor.size);
        poTextureDatabaseItem->SetFilename(strFileNameOnly.toUtf8().data());
        poTextureDatabaseItem->SetRelativePath(strPathOnly.toUtf8().data());
        poTextureDatabaseItem->SetOwnerDatabase(this);
        poTextureDatabaseItem->SetFileExtension(strExtension.toUtf8().data());
        poTextureDatabaseItem->SetFlag(IAssetItem::eFlag_Visible, true);
        poTextureDatabaseItem->SetHash(AssetBrowser::HashStringSbdm(strOutputTextureName.toUtf8().data()));
        m_assets[strOutputTextureName] = poTextureDatabaseItem;
    }

    // clear up the file array
    cFiles.clear();
    tags.clear();
    for (unsigned int i = 0; i < IResourceCompilerHelper::GetNumSourceImageFormats(); ++i)
    {
        tags.push_back(IResourceCompilerHelper::GetSourceImageFormat(i, false));
    }
    CIndexedFiles::GetDB().GetFilesWithTags(cFiles, tags);

    // for every file, check if is a texture...
    nTotalFiles = cFiles.size();

    for (nCurrentFile = 0; nCurrentFile < nTotalFiles; ++nCurrentFile)
    {
        IFileUtil::FileDesc& rstFileDescriptor = cFiles[nCurrentFile];
        strIntermediateFilename = rstFileDescriptor.filename.toLower();
        Path::ConvertBackSlashToSlash(strIntermediateFilename);
        strOutputTextureName = strIntermediateFilename;

        if (!IResourceCompilerHelper::IsSourceImageFormatSupported(strOutputTextureName.toUtf8().data()))
        {
            continue;
        }

        // no need for temp files.
        if (strOutputTextureName.contains("~~~TempFile~~~"))
        {
            continue;
        }

        strFilename = strOutputTextureName;
        strExtension = Path::GetExt(strOutputTextureName);
        // textures without names are not important for us.
        szTextureName = rstFileDescriptor.filename;

        if (szTextureName.isNull())
        {
            continue;
        }

        // textures which name starts with '$' are dynamic textures or in any way textures that
        // don't interest us.
        if (szTextureName[0] == '$')
        {
            continue;
        }

        strFileNameOnly = Path::GetFile(strOutputTextureName);
        strPathOnly = Path::GetPath(strOutputTextureName);

        poTextureDatabaseItem = new CAssetTextureItem();

        if (!poTextureDatabaseItem)
        {
            return;
        }

        poTextureDatabaseItem->SetFileSize(rstFileDescriptor.size);
        poTextureDatabaseItem->SetFilename(strFileNameOnly.toUtf8().data());
        poTextureDatabaseItem->SetRelativePath(strPathOnly.toUtf8().data());
        poTextureDatabaseItem->SetOwnerDatabase(this);
        poTextureDatabaseItem->SetFileExtension(strExtension.toUtf8().data());
        poTextureDatabaseItem->SetFlag(IAssetItem::eFlag_Visible, true);
        poTextureDatabaseItem->SetHash(AssetBrowser::HashStringSbdm(strOutputTextureName.toUtf8().data()));
        m_assets[strIntermediateFilename] = poTextureDatabaseItem;
    }
}

void CAssetTextureDatabase::CacheFieldsInfoForAlreadyLoadedAssets()
{
    ISystem* pSystem = GetISystem();
    IRenderer* pRenderer = pSystem->GetIRenderer();

    SRendererQueryGetAllTexturesParam query;
    pRenderer->EF_Query(EFQ_GetAllTextures, query);

    if (query.pTextures)
    {
        for (int i = 0; i < query.numTextures; ++i)
        {
            ITexture* pTexture = query.pTextures[i];
            int nTexSize = pTexture->GetDataSize();
            if (nTexSize > 0)
            {
                QString textureName = pTexture->GetName();
                Path::ConvertSlashToBackSlash(textureName);
                TFilenameAssetMap::iterator itr = m_assets.find(textureName);

                if (m_assets.end() != itr)
                {
                    static_cast<CAssetTextureItem*>(itr->second)->CacheFieldsInfoForLoadedTex(pTexture);
                }
            }
        }
    }

    pRenderer->EF_Query(EFQ_GetAllTexturesRelease, query);
}

const char* CAssetTextureDatabase::GetTransactionFilename() const
{
    return "textureAssetTransactions.xml";
}

QWidget* CAssetTextureDatabase::CreateDbFilterDialog(QWidget* pParent, IAssetViewer* pViewerCtrl)
{
    CTextureAssetDbFilterDlg* pDlg = new CTextureAssetDbFilterDlg(pParent);
    pDlg->SetAssetViewer(pViewerCtrl);

    return pDlg;
}

void CAssetTextureDatabase::UpdateDbFilterDialogUI(QWidget* pDlg)
{
    CTextureAssetDbFilterDlg* pFilterDlg = (CTextureAssetDbFilterDlg*)pDlg;

    pFilterDlg->UpdateFilterUI();
}

void CAssetTextureDatabase::PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db)
{
    assert(db->isTag(GetDatabaseName()));

    for (int i = 0; i < db->getChildCount(); ++i)
    {
        XmlNodeRef entry = db->getChild(i);
        QString fileName = entry->getAttr("fileName");
        TFilenameAssetMap::iterator assetIt = m_assets.find(fileName);
        bool bAssetFound = m_assets.end() != assetIt;

        if (bAssetFound)
        {
            assetIt->second->FromXML(entry);
        }
    }
}
