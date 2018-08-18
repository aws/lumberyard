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

// Description : Implementation of AssetBrowserCommon.h


#include "StdAfx.h"
#include "AssetBrowserCommon.h"
#include "Include/IAssetViewer.h"
#include "StringUtils.h"
#include "Util/MemoryBlock.h"
#include "Util/Image.h"
#include "Util/ImageUtil.h"
#include "Util/PathUtil.h"
#include "Include/IAssetItemDatabase.h"
#include "Include/IAssetViewer.h"
#include "ImageExtensionHelper.h"
#include "AssetBrowserManager.h"
#include "Controls/PreviewModelCtrl.h"

#include <QPainter>
#include <QVariant>

#if defined (Q_OS_WIN)
#include <QtWinExtras/qwinfunctions.h>
#endif

namespace AssetBrowser
{
    const char* kThumbnailsRoot = "AssetBrowser/Thumbs/";
}

inline char WildcardCmpChar(bool bCaseSensitive, char aChar)
{
    return (bCaseSensitive ? aChar : tolower(aChar));
}

static bool WildcardCompareString(const char* pWildcard, const char* pText, bool bCaseSensitive = false)
{
    const char* cp = NULL, * mp = NULL;

    while ((*pText) && (*pWildcard != '*'))
    {
        if ((WildcardCmpChar(bCaseSensitive, *pWildcard) != WildcardCmpChar(bCaseSensitive, *pText))
            && (*pWildcard != '?'))
        {
            return false;
        }

        pWildcard++;
        pText++;
    }

    while (*pText)
    {
        if (*pWildcard == '*')
        {
            if (!*(++pWildcard))
            {
                return true;
            }

            mp = pWildcard;
            cp = pText + 1;
        }
        else if ((WildcardCmpChar(bCaseSensitive, *pWildcard) == WildcardCmpChar(bCaseSensitive, *pText))
                 || (*pWildcard == '?'))
        {
            pWildcard++;
            pText++;
        }
        else
        {
            pWildcard = mp;
            pText = cp++;
        }
    }

    while (*pWildcard == '*')
    {
        pWildcard++;
    }

    return (!*pWildcard);
}

static bool SearchTextWithWildcard(const char* pFindWhat, const char* pSearchWhere)
{
    // no wildcards found, we should search if contains
    if (!strstr(pFindWhat, "*") && !strstr(pFindWhat, "?"))
    {
        return CryStringUtils::stristr(pSearchWhere, pFindWhat);
    }

    return WildcardCompareString(pFindWhat, pSearchWhere);
}

CAssetItemDatabase::CAssetItemDatabase()
{
    m_ref = 1;
}

CAssetItemDatabase::~CAssetItemDatabase()
{
    // emtpty, call FreeData() first
}

void CAssetItemDatabase::PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db)
{
}

void CAssetItemDatabase::FreeData()
{
    if (m_assets.empty())
    {
        return;
    }

    Log("Release database %s asset items...", GetDatabaseName());

    for (TFilenameAssetMap::iterator iter = m_assets.begin(), iterEnd = m_assets.end(); iter != iterEnd; ++iter)
    {
        SAFE_RELEASE(iter->second);
    }

    m_assets.clear();
}

const char* CAssetItemDatabase::GetSupportedExtensions() const
{
    return "";
};

IAssetItemDatabase::TAssetFields& CAssetItemDatabase::GetAssetFields()
{
    return m_assetFields;
}

SAssetField* CAssetItemDatabase::GetAssetFieldByName(const char* pFieldName)
{
    for (size_t i = 0, iCount = m_assetFields.size(); i < iCount; ++i)
    {
        if (m_assetFields[i].m_fieldName == pFieldName)
        {
            return &m_assetFields[i];
        }
    }

    return NULL;
}

const char* CAssetItemDatabase::GetDatabaseName() const
{
    return "";
}

void CAssetItemDatabase::Refresh()
{
    // empty
}

IAssetItemDatabase::TFilenameAssetMap&  CAssetItemDatabase::GetAssets()
{
    return m_assets;
}

IAssetItem* CAssetItemDatabase::GetAsset(const char* pAssetFilename)
{
    QString assetPath(pAssetFilename);
    Path::ConvertBackSlashToSlash(assetPath);
    TFilenameAssetMap::iterator iter = m_assets.find(assetPath);

    if (iter != m_assets.end())
    {
        return iter->second;
    }

    return NULL;
}

void CAssetItemDatabase::ApplyTagFilters(const TAssetFieldFiltersMap& rFieldFilters, CAssetBrowserManager::StrVector& assetList)
{
    CAssetBrowserManager::StrVector tags;

    for (auto iter = rFieldFilters.begin(), iterEnd = rFieldFilters.end(); iter != iterEnd; ++iter)
    {
        const SAssetField& field = iter->second;

        if (field.m_fieldName == "tags")
        {
            if (field.m_filterCondition != SAssetField::eCondition_Contains)
            {
                continue;
            }

            QString filterValue(field.m_filterValue);

            if (filterValue.isEmpty())
            {
                continue;
            }

            tags.push_back(filterValue);
        }
    }

    for (auto item = tags.begin(), end = tags.end(); item != end; ++item)
    {
        if (item->isEmpty())
        {
            continue;
        }

        CAssetBrowserManager::Instance()->GetAssetsForTag(assetList, (*item));
        CAssetBrowserManager::Instance()->GetAssetsWithDescription(assetList, (*item));
    }
}

void CAssetItemDatabase::ApplyFilters(const TAssetFieldFiltersMap& rFieldFilters)
{
    CAssetBrowserManager::StrVector assetList;
    TFilenameAssetMap tagFilteredAssetList;
    bool bFoundAssetTags = false;

    // loop through all field filters and abort if one of them does not comply
    for (auto iter = rFieldFilters.begin(), iterEnd = rFieldFilters.end(); iter != iterEnd; ++iter)
    {
        const SAssetField& field = iter->second;

        if (field.m_fieldName == "tags")
        {
            ApplyTagFilters(rFieldFilters, assetList);
            bFoundAssetTags = !assetList.empty();
            break;
        }
    }

    if (bFoundAssetTags)
    {
        for (auto item = assetList.begin(), end = assetList.end(); item != end; ++item)
        {
            for (auto iterAsset = m_assets.begin(), iterAssetsEnd = m_assets.end(); iterAsset != iterAssetsEnd; ++iterAsset)
            {
                bool found = false;
                IAssetItem* pAsset = iterAsset->second;

                if (iterAsset->first.compare(*item, Qt::CaseInsensitive) == 0)
                {
                    found = true;
                }

                pAsset->SetFlag(IAssetItem::eFlag_Visible, found);
            }
        }

        return;
    }

    std::map<QString, char*> cFieldFiltersValueRawData, cFieldFiltersMinValueRawData, cFieldFiltersMaxValueRawData;
    bool bAssetIsVisible;
    bool bAssetIsVisibleByPostFilters;
    bool bAssetIsVisibleByFilters;
    TAssetFieldFiltersMap::const_iterator iterFullSearchText = rFieldFilters.find("fullsearchtext");
    bool bHasFullSearchText = iterFullSearchText != rFieldFilters.end();

    for (auto iterAsset = m_assets.begin(), iterAssetsEnd = m_assets.end(); iterAsset != iterAssetsEnd; ++iterAsset)
    {
        IAssetItem* pAsset = iterAsset->second;
        pAsset->SetFlag(IAssetItem::eFlag_Visible, true);
        bAssetIsVisibleByPostFilters = true;
        bAssetIsVisibleByFilters = true;
        bAssetIsVisible = true;

        // loop through all field filters and abort if one of them does not comply
        for (auto iter = rFieldFilters.begin(), iterEnd = rFieldFilters.end(); iter != iterEnd; ++iter)
        {
            bAssetIsVisible = true;

            // if asset is not visible even after "post" filters, just abort, its hidden
            if (!bAssetIsVisibleByPostFilters)
            {
                break;
            }

            const SAssetField& field = iter->second;

            if (field.m_fieldName == "")
            {
                continue;
            }

            // skip special field, treated after this loop
            if (field.m_fieldName == "fullsearchtext")
            {
                continue;
            }

            // if this field is not from all databases
            if (!field.m_parentDatabaseName.isEmpty())
            {
                // skip fields that are not handled by this database
                if (field.m_parentDatabaseName != GetDatabaseName())
                {
                    continue;
                }
            }

            SAssetField::EAssetFilterCondition filterCondition = field.m_filterCondition;

            switch (field.m_fieldType)
            {
            case SAssetField::eType_None:
            {
                assert(!"eType_None is not a permitted type for a field, you must initialize it to a field type");
                continue;
            }

            case SAssetField::eType_Bool:
            {
                bool filterValue;
                QVariant assetFieldValueVariant;

                filterValue = (field.m_filterValue == "Yes");

                assetFieldValueVariant = pAsset->GetAssetFieldValue(field.m_fieldName.toUtf8().data());
                bool assetFieldValue = assetFieldValueVariant.toBool();

                if (assetFieldValueVariant.isNull())
                {
                    continue;
                }

                switch (filterCondition)
                {
                case SAssetField::eCondition_Equal:
                    bAssetIsVisible = (assetFieldValue == filterValue);
                    break;

                case SAssetField::eCondition_Greater:
                    bAssetIsVisible = (assetFieldValue > filterValue);
                    break;

                case SAssetField::eCondition_Less:
                    bAssetIsVisible = (assetFieldValue < filterValue);
                    break;

                case SAssetField::eCondition_GreaterOrEqual:
                    bAssetIsVisible = (assetFieldValue >= filterValue);
                    break;

                case SAssetField::eCondition_LessOrEqual:
                    bAssetIsVisible = (assetFieldValue <= filterValue);
                    break;

                case SAssetField::eCondition_Not:
                    bAssetIsVisible = (assetFieldValue != filterValue);
                    break;

                case SAssetField::eCondition_InsideRange:
                    break;// no sense to use range for a bool field
                }

                break;
            }

            case SAssetField::eType_Int8:
            {
                char filterValue, assetFieldValue;

                filterValue = field.m_filterValue.toInt();
                QVariant assetFieldValueVariant = pAsset->GetAssetFieldValue(field.m_fieldName.toUtf8().data());
                assetFieldValue = assetFieldValueVariant.toInt();

                if (assetFieldValueVariant.isNull())
                {
                    continue;
                }

                switch (filterCondition)
                {
                case SAssetField::eCondition_Equal:
                    bAssetIsVisible = (assetFieldValue == filterValue);
                    break;

                case SAssetField::eCondition_Greater:
                    bAssetIsVisible = (assetFieldValue > filterValue);
                    break;

                case SAssetField::eCondition_Less:
                    bAssetIsVisible = (assetFieldValue < filterValue);
                    break;

                case SAssetField::eCondition_GreaterOrEqual:
                    bAssetIsVisible = (assetFieldValue >= filterValue);
                    break;

                case SAssetField::eCondition_LessOrEqual:
                    bAssetIsVisible = (assetFieldValue <= filterValue);
                    break;

                case SAssetField::eCondition_Not:
                    bAssetIsVisible = (assetFieldValue != filterValue);
                    break;

                case SAssetField::eCondition_InsideRange:
                {
                    char maxFilterValue = field.m_maxFilterValue.toInt();
                    bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue);
                    break;
                }
                }

                break;
            }

            case SAssetField::eType_Int16:
            {
                short int filterValue, assetFieldValue;

                filterValue = field.m_filterValue.toInt();
                QVariant assetFieldValueVariant = pAsset->GetAssetFieldValue(field.m_fieldName.toUtf8().data());
                assetFieldValue = assetFieldValueVariant.toInt();

                if (assetFieldValueVariant.isNull())
                {
                    continue;
                }

                switch (filterCondition)
                {
                case SAssetField::eCondition_Equal:
                    bAssetIsVisible = (assetFieldValue == filterValue);
                    break;

                case SAssetField::eCondition_Greater:
                    bAssetIsVisible = (assetFieldValue > filterValue);
                    break;

                case SAssetField::eCondition_Less:
                    bAssetIsVisible = (assetFieldValue < filterValue);
                    break;

                case SAssetField::eCondition_GreaterOrEqual:
                    bAssetIsVisible = (assetFieldValue >= filterValue);
                    break;

                case SAssetField::eCondition_LessOrEqual:
                    bAssetIsVisible = (assetFieldValue <= filterValue);
                    break;

                case SAssetField::eCondition_Not:
                    bAssetIsVisible = (assetFieldValue != filterValue);
                    break;

                case SAssetField::eCondition_InsideRange:
                {
                    short int maxFilterValue = field.m_maxFilterValue.toInt();
                    bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue);
                    break;
                }
                }

                break;
            }

            case SAssetField::eType_Int32:
            {
                int filterValue, assetFieldValue;

                filterValue = field.m_filterValue.toInt();
                QVariant assetFieldValueVariant = pAsset->GetAssetFieldValue(field.m_fieldName.toUtf8().data());
                assetFieldValue = assetFieldValueVariant.toInt();

                if (assetFieldValueVariant.isNull())
                {
                    continue;
                }

                switch (filterCondition)
                {
                case SAssetField::eCondition_Equal:
                    bAssetIsVisible = (assetFieldValue == filterValue);
                    break;

                case SAssetField::eCondition_Greater:
                    bAssetIsVisible = (assetFieldValue > filterValue);
                    break;

                case SAssetField::eCondition_Less:
                    bAssetIsVisible = (assetFieldValue < filterValue);
                    break;

                case SAssetField::eCondition_GreaterOrEqual:
                    bAssetIsVisible = (assetFieldValue >= filterValue);
                    break;

                case SAssetField::eCondition_LessOrEqual:
                    bAssetIsVisible = (assetFieldValue <= filterValue);
                    break;

                case SAssetField::eCondition_Not:
                    bAssetIsVisible = (assetFieldValue != filterValue);
                    break;

                case SAssetField::eCondition_InsideRange:
                {
                    int maxFilterValue = field.m_maxFilterValue.toInt();
                    bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue);
                    break;
                }
                }

                break;
            }

            case SAssetField::eType_Int64:
            {
                qint64 filterValue, assetFieldValue;

                filterValue = field.m_filterValue.toLongLong();

                QVariant assetFieldValueVariant = pAsset->GetAssetFieldValue(field.m_fieldName.toUtf8().data());
                assetFieldValue = assetFieldValueVariant.toLongLong();

                if (assetFieldValueVariant.isNull())
                {
                    continue;
                }

                switch (filterCondition)
                {
                case SAssetField::eCondition_Equal:
                    bAssetIsVisible = (assetFieldValue == filterValue);
                    break;

                case SAssetField::eCondition_Greater:
                    bAssetIsVisible = (assetFieldValue > filterValue);
                    break;

                case SAssetField::eCondition_Less:
                    bAssetIsVisible = (assetFieldValue < filterValue);
                    break;

                case SAssetField::eCondition_GreaterOrEqual:
                    bAssetIsVisible = (assetFieldValue >= filterValue);
                    break;

                case SAssetField::eCondition_LessOrEqual:
                    bAssetIsVisible = (assetFieldValue <= filterValue);
                    break;

                case SAssetField::eCondition_Not:
                    bAssetIsVisible = (assetFieldValue != filterValue);
                    break;

                case SAssetField::eCondition_InsideRange:
                {
                    qint64 maxFilterValue = field.m_maxFilterValue.toLongLong();
                    bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue);
                    break;
                }
                }

                break;
            }

            case SAssetField::eType_Float:
            {
                float filterValue, assetFieldValue;

                filterValue = field.m_filterValue.toDouble();

                QVariant assetFieldValueVariant = pAsset->GetAssetFieldValue(field.m_fieldName.toUtf8().data());
                assetFieldValue = assetFieldValueVariant.toDouble();

                if (assetFieldValueVariant.isNull())
                {
                    continue;
                }

                switch (filterCondition)
                {
                case SAssetField::eCondition_Equal:
                    bAssetIsVisible = (assetFieldValue == filterValue);
                    break;

                case SAssetField::eCondition_Greater:
                    bAssetIsVisible = (assetFieldValue > filterValue);
                    break;

                case SAssetField::eCondition_Less:
                    bAssetIsVisible = (assetFieldValue < filterValue);
                    break;

                case SAssetField::eCondition_GreaterOrEqual:
                    bAssetIsVisible = (assetFieldValue >= filterValue);
                    break;

                case SAssetField::eCondition_LessOrEqual:
                    bAssetIsVisible = (assetFieldValue <= filterValue);
                    break;

                case SAssetField::eCondition_Not:
                    bAssetIsVisible = (assetFieldValue != filterValue);
                    break;

                case SAssetField::eCondition_InsideRange:
                {
                    float maxFilterValue = field.m_maxFilterValue.toDouble();
                    bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue);
                    break;
                }
                }

                break;
            }

            case SAssetField::eType_Double:
            {
                double filterValue, assetFieldValue;

                filterValue = field.m_filterValue.toDouble();

                QVariant assetFieldValueVariant = pAsset->GetAssetFieldValue(field.m_fieldName.toUtf8().data());
                assetFieldValue = assetFieldValueVariant.toDouble();

                if (assetFieldValueVariant.isNull())
                {
                    continue;
                }

                switch (filterCondition)
                {
                case SAssetField::eCondition_Equal:
                    bAssetIsVisible = (assetFieldValue == filterValue);
                    break;

                case SAssetField::eCondition_Greater:
                    bAssetIsVisible = (assetFieldValue > filterValue);
                    break;

                case SAssetField::eCondition_Less:
                    bAssetIsVisible = (assetFieldValue < filterValue);
                    break;

                case SAssetField::eCondition_GreaterOrEqual:
                    bAssetIsVisible = (assetFieldValue >= filterValue);
                    break;

                case SAssetField::eCondition_LessOrEqual:
                    bAssetIsVisible = (assetFieldValue <= filterValue);
                    break;

                case SAssetField::eCondition_Not:
                    bAssetIsVisible = (assetFieldValue != filterValue);
                    break;

                case SAssetField::eCondition_InsideRange:
                {
                    double maxFilterValue = field.m_maxFilterValue.toDouble();
                    bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue);
                    break;
                }
                }

                break;
            }

            case SAssetField::eType_String:
            {
                if (field.m_fieldName == "tags")
                {
                    if (!field.m_filterValue.isEmpty())
                    {
                        bool foundInTagAssetList = false;

                        for (auto item = assetList.begin(), end = assetList.end(); item != end; ++item)
                        {
                            if (iterAsset->first == (*item))
                            {
                                foundInTagAssetList = true;
                            }
                        }

                        if (!foundInTagAssetList)
                        {
                            bAssetIsVisible = false;
                        }

                        break;
                    }
                    else
                    {
                        bAssetIsVisible = true;
                        break;
                    }
                }

                QVariant assetFieldValueVariant = pAsset->GetAssetFieldValue(field.m_fieldName.toUtf8().data());
                QString assetFieldValue = assetFieldValueVariant.toString();
                const QString& filterValue = field.m_filterValue;

                if (assetFieldValueVariant.isNull())
                {
                    break;
                }

                switch (filterCondition)
                {
                case SAssetField::eCondition_Contains:
                    bAssetIsVisible = SearchTextWithWildcard(filterValue.toUtf8().data(), assetFieldValue.toUtf8().data());
                    break;

                case SAssetField::eCondition_ContainsOneOfTheWords:
                {
                    QStringList words;

                    SplitString(filterValue, words, ' ');

                    for (size_t w = 0, wCount = words.size(); w < wCount; ++w)
                    {
                        bAssetIsVisible = SearchTextWithWildcard(words[w].toUtf8().data(), assetFieldValue.toUtf8().data());

                        // break if we find one word which is contained by the field value
                        if (bAssetIsVisible)
                        {
                            break;
                        }
                    }

                    break;
                }

                case SAssetField::eCondition_StartsWith:
                    bAssetIsVisible = (assetFieldValue.startsWith(filterValue));
                    break;

                case SAssetField::eCondition_EndsWith:
                    bAssetIsVisible = (assetFieldValue.endsWith(filterValue));
                    break;

                case SAssetField::eCondition_Equal:
                    bAssetIsVisible = (assetFieldValue == filterValue);
                    break;

                case SAssetField::eCondition_Greater:
                    bAssetIsVisible = (assetFieldValue > filterValue);
                    break;

                case SAssetField::eCondition_Less:
                    bAssetIsVisible = (assetFieldValue < filterValue);
                    break;

                case SAssetField::eCondition_GreaterOrEqual:
                    bAssetIsVisible = (assetFieldValue >= filterValue);
                    break;

                case SAssetField::eCondition_LessOrEqual:
                    bAssetIsVisible = (assetFieldValue <= filterValue);
                    break;

                case SAssetField::eCondition_Not:
                    bAssetIsVisible = (assetFieldValue != filterValue);
                    break;

                case SAssetField::eCondition_InsideRange:
                {
                    bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= field.m_maxFilterValue);
                    break;
                }
                }

                break;
            }
            }

            //
            // if this field is an 'post filter', then remember its status
            //
            if (field.m_bPostFilter)
            {
                if (!bAssetIsVisible)
                {
                    bAssetIsVisibleByPostFilters = false;
                }
            }
            else
            {
                if (!bAssetIsVisible)
                {
                    bAssetIsVisibleByFilters = false;
                }
            }
        }

        //
        // check special 'fullsearchtext' field (this is provided when the user searches in the full text search edit box in the asset browser)
        //

        bool bAssetIsVisibleByFullTextSearch = true;

        if (bHasFullSearchText && iterFullSearchText->second.m_filterValue != "")
        {
            QVariant assetFieldValueVariant = pAsset->GetAssetFieldValue("filename");
            QString assetFieldValue = assetFieldValueVariant.toString();

            // get the filename asset field value to check against
            if (!assetFieldValueVariant.isNull())
            {
                bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard(iterFullSearchText->second.m_filterValue.toUtf8().data(), assetFieldValue.toUtf8().data());

                // lets try searching without extension
                if (!bAssetIsVisibleByFullTextSearch)
                {
                    QString fileExt = Path::GetExt(assetFieldValue);

                    assetFieldValue.replace(("." + fileExt), "");
                    bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard(iterFullSearchText->second.m_filterValue.toUtf8().data(), assetFieldValue.toUtf8().data());
                }
            }

            // try path then
            if (!bAssetIsVisibleByFullTextSearch)
            {
                assetFieldValueVariant = pAsset->GetAssetFieldValue("relativepath");
                assetFieldValue = assetFieldValueVariant.toString();

                // get the asset relative path field value to check against
                if (!assetFieldValueVariant.isNull())
                {
                    bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard(iterFullSearchText->second.m_filterValue.toUtf8().data(), assetFieldValue.toUtf8().data());
                    QString assetFilename = pAsset->GetAssetFieldValue("filename").toString();
                    QString tmp;

                    // lets try searching with whole path+filename (no ext)
                    if (!bAssetIsVisibleByFullTextSearch)
                    {
                        QString fileExt = Path::GetExt(assetFieldValue);

                        tmp = assetFilename;
                        tmp.replace(("." + fileExt), "");
                        tmp = assetFieldValue + tmp;
                        bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard(iterFullSearchText->second.m_filterValue.toUtf8().data(), tmp.toUtf8().data());

                        // lets try searching with whole path+filename+ext
                        if (!bAssetIsVisibleByFullTextSearch)
                        {
                            assetFieldValue += assetFilename;
                            bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard(iterFullSearchText->second.m_filterValue.toUtf8().data(), assetFieldValue.toUtf8().data());
                        }
                    }
                }
            }

            if (!bAssetIsVisibleByFullTextSearch)
            {
                QStringList words;

                // prepare the tags words
                SplitString(iterFullSearchText->second.m_filterValue, words, ' ');

                assetFieldValueVariant = pAsset->GetAssetFieldValue("tags");
                assetFieldValue = assetFieldValueVariant.toString();

                // get the tags asset field value to check against
                if (!assetFieldValueVariant.isNull())
                {
                    // search the tags
                    for (size_t w = 0, wCount = words.size(); w < wCount; ++w)
                    {
                        bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard(words[w].toUtf8().data(), assetFieldValue.toUtf8().data());

                        // break if we find one word which is contained by the field value
                        if (bAssetIsVisibleByFullTextSearch)
                        {
                            break;
                        }
                    }
                }
            }
        }

        bAssetIsVisible = bAssetIsVisibleByFilters && bAssetIsVisibleByPostFilters && bAssetIsVisibleByFullTextSearch;
        pAsset->SetFlag(IAssetItem::eFlag_Visible, bAssetIsVisible);
    }
}

void CAssetItemDatabase::ClearFilters()
{
    for (TFilenameAssetMap::iterator iterAsset = m_assets.begin(), iterAssetsEnd = m_assets.end(); iterAsset != iterAssetsEnd; ++iterAsset)
    {
        IAssetItem* pAsset = iterAsset->second;

        pAsset->SetFlag(IAssetItem::eFlag_Visible, true);
    }
}

QWidget* CAssetItemDatabase::CreateDbFilterDialog(QWidget* pParent, IAssetViewer* pViewerCtrl)
{
    return NULL;
}

void CAssetItemDatabase::UpdateDbFilterDialogUI(QWidget* pDlg)
{
}

const char* CAssetItemDatabase::GetTransactionFilename() const
{
    return "commonTransactions.xml";
}

bool CAssetItemDatabase::AddMetaDataChangeListener(IAssetItemDatabase::MetaDataChangeListener callBack)
{
    return stl::push_back_unique(m_metaDataChangeListeners, callBack);
}

bool CAssetItemDatabase::RemoveMetaDataChangeListener(IAssetItemDatabase::MetaDataChangeListener callBack)
{
    return stl::find_and_erase(m_metaDataChangeListeners, callBack);
}

void CAssetItemDatabase::OnMetaDataChange(const IAssetItem* pAssetItem)
{
    for (size_t i = 0; i < m_metaDataChangeListeners.size(); ++i)
    {
        m_metaDataChangeListeners[i](pAssetItem);
    }
}

HRESULT STDMETHODCALLTYPE CAssetItemDatabase::QueryInterface(const IID& riid, void** ppvObj)
{
    if (riid == __uuidof(IAssetItemDatabase) /* && m_pIntegrator*/)
    {
        *ppvObj = this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CAssetItemDatabase::AddRef()
{
    return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetItemDatabase::Release()
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

//---

CAssetItem::CAssetItem()
    : m_flags(0)
    , m_nFileSize(0)
    , m_oDrawingRectangle(0, 0, 0, 0)
    , m_pOwnerDatabase(NULL)
    , m_ref(1)
    , m_assetIndex(0)
    , m_hPreviewWidget(0)
    , m_pUncachedThumbBmp(0)
{
}

CAssetItem::~CAssetItem()
{
    // empty, call FreeData first
}

void CAssetItem::FreeData()
{
}

uint32 CAssetItem::GetHash() const
{
    return m_hash;
}

void CAssetItem::SetHash(uint32 hash)
{
    m_hash = hash;
}

IAssetItemDatabase* CAssetItem::GetOwnerDatabase() const
{
    return m_pOwnerDatabase;
}

void CAssetItem::SetOwnerDatabase(IAssetItemDatabase* piOwnerDisplayDatabase)
{
    m_pOwnerDatabase = piOwnerDisplayDatabase;
}

const IAssetItem::TAssetDependenciesMap& CAssetItem::GetDependencies() const
{
    return m_dependencies;
}

void CAssetItem::SetFileSize(quint64 aSize)
{
    m_nFileSize = aSize;
}

void CAssetItem::SetFileExtension(const char* pExt)
{
    m_strExtension = pExt;
}

QString CAssetItem::GetFileExtension() const
{
    return m_strExtension;
}

quint64 CAssetItem::GetFileSize() const
{
    return m_nFileSize;
}

void CAssetItem::SetFilename(const char* pName)
{
    m_strFilename = pName;
}

QString CAssetItem::GetFilename() const
{
    return m_strFilename;
}

void CAssetItem::SetRelativePath(const char* pPath)
{
    m_strRelativePath = pPath;
}

QString CAssetItem::GetRelativePath() const
{
    return m_strRelativePath;
}

UINT CAssetItem::GetFlags() const
{
    return m_flags;
}

void CAssetItem::SetFlags(UINT aFlags)
{
    m_flags = aFlags;
}

void CAssetItem::SetFlag(EAssetFlags aFlag, bool bSet)
{
    if (bSet)
    {
        m_flags |= aFlag;
    }
    else
    {
        m_flags &= ~aFlag;
    }
}

bool CAssetItem::IsFlagSet(EAssetFlags aFlag) const
{
    return 0 != (m_flags & (UINT)aFlag);
}

void CAssetItem::SetIndex(UINT aIndex)
{
    m_assetIndex = aIndex;
}

UINT CAssetItem::GetIndex() const
{
    return m_assetIndex;
}

QVariant CAssetItem::GetAssetFieldValue(const char* pFieldName) const
{
    if (AssetViewer::IsFieldName(pFieldName, "filename"))
    {
        return m_strFilename;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "dccfilename"))
    {
        return m_strDccFilename;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "relativepath"))
    {
        return m_strRelativePath;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "extension"))
    {
        return m_strExtension;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "filesize"))
    {
        return m_nFileSize;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "fullfilepath"))
    {
        return m_strRelativePath + m_strFilename;
    }
    else if (AssetViewer::IsFieldName(pFieldName, "usedinlevel"))
    {
        return (m_flags & eFlag_UsedInLevel);
    }
    else if (AssetViewer::IsFieldName(pFieldName, "tags"))
    {
        QString path = GetRelativePath();
        path += GetFilename();

        QString description;
        CAssetBrowserManager::Instance()->GetAssetDescription(path, description);

        CAssetBrowserManager::StrVector tags;
        CAssetBrowserManager::Instance()->GetTagsForAsset(tags, path);

        QString result = description;

        for (size_t i = 0; i < tags.size(); ++i)
        {
            result += ",";
            result += tags[i];
        }

        return result;
    }

    return QVariant();
}

bool CAssetItem::SetAssetFieldValue(const char* pFieldName, void* pSrc)
{
    return false;
}

void CAssetItem::GetDrawingRectangle(QRect& rstDrawingRectangle) const
{
    rstDrawingRectangle = m_oDrawingRectangle;
}

void    CAssetItem::SetDrawingRectangle(const QRect& crstDrawingRectangle)
{
    m_oDrawingRectangle = crstDrawingRectangle;
}

bool CAssetItem::Cache()
{
    QString strUserFolder = Path::GetResolvedUserSandboxFolder();

    const QString str = QString::fromUtf8("%1%2%3.jpg").arg(strUserFolder, QString(AssetBrowser::kThumbnailsRoot), QString::number(m_hash));

    return m_cachedThumbBmp.save(str);
}

bool CAssetItem::ForceCache()
{
    SetFlag(eFlag_Cached, false);
    SetFlag(eFlag_ThumbnailLoaded, false);
    return Cache();
}

bool CAssetItem::LoadThumbnail()
{
    if (IsFlagSet(eFlag_ThumbnailLoaded))
    {
        return true;
    }

    QString strUserFolder = Path::GetResolvedUserSandboxFolder();

    const QString str = QString::fromUtf8("%1%2%3.jpg").arg(strUserFolder, QString(AssetBrowser::kThumbnailsRoot), QString::number(m_hash));

    if (m_cachedThumbBmp.load(str))
    {
        SetFlag(eFlag_ThumbnailLoaded, true);
        return true;
    }

    SetFlag(eFlag_ThumbnailLoaded, false);

    return false;
}

void CAssetItem::UnloadThumbnail()
{
    if (!IsFlagSet(eFlag_ThumbnailLoaded))
    {
        return;
    }

    SetFlag(eFlag_ThumbnailLoaded, false);
    m_cachedThumbBmp = QImage();
}

void CAssetItem::OnBeginPreview(QWidget* hQuickPreviewWndC)
{
    m_hPreviewWidget = hQuickPreviewWndC;
}

QWidget* CAssetItem::GetCustomPreviewPanelHeader(QWidget* pParentWnd)
{
    return nullptr;
}

QWidget* CAssetItem::GetCustomPreviewPanelFooter(QWidget* pParentWnd)
{
    return nullptr;
}

void CAssetItem::OnEndPreview()
{
    m_hPreviewWidget = 0;
}

void CAssetItem::PreviewRender(
    QWidget* hRenderWindow,
    const QRect& rstViewport,
    int aMouseX, int aMouseY,
    int aMouseDeltaX, int aMouseDeltaY,
    int aMouseWheelDelta, UINT aKeyFlags)
{
}

void CAssetItem::OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags)
{
}

void CAssetItem::OnThumbClick(const QPoint& point, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
}

void CAssetItem::OnThumbDblClick(const QPoint& point, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
}

bool CAssetItem::DrawThumbImage(QPainter* painter, const QRect& rRect)
{
    QImage* pSrcBmp = (m_flags & eFlag_ThumbnailLoaded) ? &m_cachedThumbBmp : m_pUncachedThumbBmp;

    if (!m_pUncachedThumbBmp)
    {
        return false;
    }

    painter->drawImage(rRect, *pSrcBmp);

    return true;
}

bool CAssetItem::HitTest(int nX, int nY) const
{
    return m_oDrawingRectangle.contains(nX, nY);
}

bool CAssetItem::HitTest(const QRect& roTestRect) const
{
    return m_oDrawingRectangle.intersects(roTestRect);
}

void* CAssetItem::CreateInstanceInViewport(float aX, float aY, float aZ)
{
    return NULL;
}

bool CAssetItem::MoveInstanceInViewport(const void* pDraggedObject, float aNewX, float aNewY, float aNewZ)
{
    return false;
}

void CAssetItem::AbortCreateInstanceInViewport(const void* pDraggedObject)
{
}

void CAssetItem::DrawTextOnReportImage(QPaintDevice* rDestBmp) const
{
}
void CAssetItem::ToXML(XmlNodeRef& node) const
{
}

void CAssetItem::FromXML(const XmlNodeRef& node)
{
}

HRESULT CAssetItem::QueryInterface(const IID& riid, void** ppvObj)
{
    if (riid == __uuidof(IAssetItem) /* && m_pIntegrator*/)
    {
        *ppvObj = this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG CAssetItem::AddRef()
{
    return ++m_ref;
};

ULONG CAssetItem::Release()
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

void CAssetItem::SaveThumbnail(CPreviewModelCtrl* pPreviewCtrl)
{
    if (pPreviewCtrl)
    {
        CImageEx img;
        pPreviewCtrl->GetImageOffscreen(img,
            QSize(gSettings.sAssetBrowserSettings.nThumbSize,
            gSettings.sAssetBrowserSettings.nThumbSize));

        m_cachedThumbBmp = QImage(reinterpret_cast<uchar*>(img.GetData()),
            img.GetWidth(), img.GetHeight(), QImage::Format_ARGB32)
            .copy();

        img.Release();
    }
}