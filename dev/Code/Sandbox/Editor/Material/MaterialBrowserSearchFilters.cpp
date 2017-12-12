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

#include "StdAfx.h"
#include "MaterialBrowserSearchFilters.h"
#include "MaterialBrowserFilterModel.h"
#include "Material.h"

SubMaterialSearchFilter::SubMaterialSearchFilter(const MaterialBrowserFilterModel* filterModel)
    : m_filterModel(filterModel)
{
    AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter::SetFilterPropagation(AzToolsFramework::AssetBrowser::AssetBrowserEntryFilter::PropagateDirection::Down);
}

void SubMaterialSearchFilter::SetFilterString(const QString& filterString)
{
    m_filterString = filterString;
}

QString SubMaterialSearchFilter::GetNameInternal() const
{
    return "SubMaterialSearchFilter";
}

bool SubMaterialSearchFilter::MatchInternal(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
{
    // All entries match if there is no search string
    if (m_filterString.isEmpty())
    {
        return true;
    }

    // Get the product material for the given asset browser entry
    AZ::Data::AssetId assetId = GetMaterialProductAssetIdFromAssetBrowserEntry(entry);
    if (assetId.IsValid() && m_filterModel)
    {
        CMaterialBrowserRecord record;
        bool found = m_filterModel->TryGetRecordFromAssetId(assetId, record);
        if (found)
        {
            // If there is a valid product material, check to see if any of its sub-materials match the search string
            if (record.m_material)
            {
                for (int i = 0; i < record.m_material->GetSubMaterialCount(); ++i)
                {
                    CMaterial* subMaterial = record.m_material->GetSubMaterial(i);
                    if (subMaterial)
                    {
                        // If any of the product sub-materials matches the string, return true for this entry
                        if (subMaterial->GetName().contains(m_filterString))
                        {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

#include <Material/MaterialBrowserSearchFilters.moc>
