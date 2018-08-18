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

// Description : Implements AssetSoundDatabase.h


#include "StdAfx.h"
#include "AssetSoundDatabase.h"
#include "AssetSoundItem.h"
#include "IRenderer.h"
#include "ImageExtensionHelper.h"
#include "Include/IAssetViewer.h"
#include "StringUtils.h"
#include "Util/IndexedFiles.h"
#include "SoundAssetDbFilterDlg.h"

REGISTER_CLASS_DESC(CAssetSoundDatabase);

CAssetSoundDatabase::CAssetSoundDatabase()
    : CAssetItemDatabase()
{
    // add fields
    static const int kFilenameColWidth = 150;
    static const int kDccFilenameColWidth = 50;
    static const int kFileSizeColWidth = 50;
    static const int kRelativePathColWidth = 50;
    static const int kLengthColWidth = 50;
    static const int kUsedInLevelColWidth = 40;
    static const int kLoadedInLevelColWidth = 40;
    static const int kTagsColWidth = 60;

    m_assetFields.push_back(SAssetField("filename", "Filename", SAssetField::eType_String, kFilenameColWidth));
    m_assetFields.push_back(SAssetField("relativepath", "Path", SAssetField::eType_String, kRelativePathColWidth));
    m_assetFields.push_back(SAssetField("length", "Length (Msec)", SAssetField::eType_Int32, kLengthColWidth));
    m_assetFields.push_back(SAssetField("loopsound", "Looping", SAssetField::eType_Bool, kLengthColWidth));
    m_assetFields.push_back(SAssetField("usedinlevel", "Used in level", SAssetField::eType_Bool, kUsedInLevelColWidth));
    m_assetFields.push_back(SAssetField("loadedinlevel", "Loaded in level", SAssetField::eType_Bool, kLoadedInLevelColWidth));
    m_assetFields.push_back(SAssetField("dccfilename", "DCC Filename", SAssetField::eType_String, kDccFilenameColWidth));
    m_assetFields.push_back(SAssetField("tags", "Tags", SAssetField::eType_String, kTagsColWidth));
}

CAssetSoundDatabase::~CAssetSoundDatabase()
{
    // empty, call FreeData first
}

void CAssetSoundDatabase::PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db)
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
HRESULT STDMETHODCALLTYPE CAssetSoundDatabase::QueryInterface(const IID& riid, void** ppvObj)
{
    if (riid == __uuidof(IAssetItemDatabase))
    {
        *ppvObj = this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CAssetSoundDatabase::AddRef()
{
    return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetSoundDatabase::Release()
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

void CAssetSoundDatabase::FreeData()
{
    CAssetItemDatabase::FreeData();
}

const char* CAssetSoundDatabase::GetSupportedExtensions() const
{
    return "fdp,fsb";
};

const char* CAssetSoundDatabase::GetTransactionFilename() const
{
    return "soundAssetTransactions.xml";
}

QWidget* CAssetSoundDatabase::CreateDbFilterDialog(QWidget* pParent, IAssetViewer* pViewerCtrl)
{
    CSoundAssetDbFilterDlg* pDlg = new CSoundAssetDbFilterDlg(pParent);
    pDlg->SetAssetViewer(pViewerCtrl);

    return pDlg;
}

void CAssetSoundDatabase::UpdateDbFilterDialogUI(QWidget* pDlg)
{
    CSoundAssetDbFilterDlg* pFilterDlg = (CSoundAssetDbFilterDlg*)pDlg;

    pFilterDlg->UpdateFilterUI();
}

const char* CAssetSoundDatabase::GetDatabaseName() const
{
    return "Sounds";
}

void CAssetSoundDatabase::CollectCachedEventgroup(XmlNodeRef& gr, const QString& Block, const QString& Path, int level)
{
    const char* pNameGr = "";
    XmlNodeRef name = gr->findChild("name");

    if (name)
    {
        pNameGr = name->getContent();
    }

    QString NewPath = Path + pNameGr;

    for (size_t j = 0, jCount = gr->getChildCount(); j < jCount; ++j)
    {
        XmlNodeRef ev = gr->getChild(j);

        if (!strcmp(ev->getTag(), "event"))
        {
            const char* pNameEv = "";
            XmlNodeRef name = ev->findChild("name");

            if (name)
            {
                pNameEv = name->getContent();
            }

            CAssetSoundItem*            poSoundDatabaseItem = NULL;
            poSoundDatabaseItem = new CAssetSoundItem();

            if (!poSoundDatabaseItem)
            {
                return;
            }

            QString fpath = Block;
            fpath += ":";
            fpath += NewPath;
            fpath += ":";

            poSoundDatabaseItem->SetFileSize(0);
            poSoundDatabaseItem->SetFilename(pNameEv);
            poSoundDatabaseItem->SetRelativePath(fpath.toUtf8().data());
            poSoundDatabaseItem->SetOwnerDatabase(this);
            poSoundDatabaseItem->SetFileExtension("fsb");
            poSoundDatabaseItem->SetFlag(IAssetItem::eFlag_Visible, true);
            fpath += pNameEv;
            poSoundDatabaseItem->SetHash(AssetBrowser::HashStringSbdm(fpath.toUtf8().data()));
            m_assets[fpath] = poSoundDatabaseItem;
        }
        else if (!strcmp(ev->getTag(), "eventgroup"))
        {
            CollectCachedEventgroup(ev, Block, NewPath + "/", level + 1);
        }
    }
}

void CAssetSoundDatabase::Refresh()
{
    FreeData();

    const char* szSoundName = NULL;
    QString strExtension;
    QString strFilename;
    IFileUtil::FileArray cFiles;
    int nTotalFiles = 0;
    int nCurrentFile = 0;
    QString strIntermediateFilename;
    QString strOutputSoundName, strPathOnly, strFileNameOnly;
    CCryFile file;
    CAssetSoundItem* poSoundDatabaseItem = NULL;

    // search for sound files
    QStringList tags;

    tags.push_back("fdp");
    CIndexedFiles::GetDB().GetFilesWithTags(cFiles, tags);

    nTotalFiles = cFiles.size();

    for (nCurrentFile = 0; nCurrentFile < nTotalFiles; ++nCurrentFile)
    {
        IFileUtil::FileDesc& rstFileDescriptor = cFiles[nCurrentFile];

        // if not a real .fdp file
        if (Path::GetExt(rstFileDescriptor.filename) != "fdp")
        {
            continue;
        }

        strIntermediateFilename = rstFileDescriptor.filename.toLower();
        strOutputSoundName = strIntermediateFilename;
        Path::ConvertBackSlashToSlash(strOutputSoundName);

        XmlNodeRef root = XmlHelpers::LoadXmlFromFile(strIntermediateFilename.toUtf8().data());

        char path[_MAX_PATH];
        azstrcpy(path, AZ_ARRAY_SIZE(path), strIntermediateFilename.toUtf8().data());
        char* ch;

        while (ch = strchr(path, '\\'))
        {
            *ch = '/';
        }

        if (ch = strrchr(path, '/'))
        {
            *ch = 0;
        }

        if (root)
        {
            const char* pName = "";

            XmlNodeRef name = root->findChild("name");

            if (name)
            {
                pName = name->getContent();
            }

            for (size_t i = 0, iCount = root->getChildCount(); i < iCount; ++i)
            {
                XmlNodeRef gr = root->getChild(i);

                if (!strcmp(gr->getTag(), "eventgroup"))
                {
                    CollectCachedEventgroup(gr, path, "", 1);
                }
            }
        }
    }
}
