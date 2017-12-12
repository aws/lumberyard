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

#include <SceneAPI/SceneCore/Events/ExportProductList.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            ExportProduct::ExportProduct(const AZStd::string& filename, Uuid id, Data::AssetType assetType, s8 lod)
                : ExportProduct(AZStd::string(filename), id, assetType, lod)
            {
            }

            ExportProduct::ExportProduct(AZStd::string&& filename, Uuid id, Data::AssetType assetType, s8 lod)
                : m_filename(AZStd::move(filename))
                , m_id(id)
                , m_assetType(assetType)
                , m_lod(lod)
            {
            }

            ExportProduct::ExportProduct(ExportProduct&& rhs)
            {
                *this = AZStd::move(rhs);
            }

            ExportProduct& ExportProduct::operator=(ExportProduct&& rhs)
            {
                m_legacyFileNames = AZStd::move(rhs.m_legacyFileNames);
                m_filename = AZStd::move(rhs.m_filename);
                m_id = rhs.m_id;
                m_assetType = rhs.m_assetType;
                m_lod = rhs.m_lod;
                return *this;
            }

            ExportProduct& ExportProductList::AddProduct(const AZStd::string& filename, Uuid id, Data::AssetType assetType, s8 lod)
            {
                return AddProduct(AZStd::string(filename), id, assetType, lod);
            }

            ExportProduct& ExportProductList::AddProduct(AZStd::string&& filename, Uuid id, Data::AssetType assetType, s8 lod)
            {
                AZ_Assert(!filename.empty(), "A filename is required to register a product.");
                AZ_Assert(!id.IsNull(), "Provided guid is not valid");
                AZ_Assert(lod < 16, "Lod value has to be between 0 and 15 or s_LodNotUsed(-1) to disable.");

                size_t index = m_products.size();
                m_products.emplace_back(AZStd::move(filename), id, assetType, lod);
                return m_products[index];
            }

            const AZStd::vector<ExportProduct>& ExportProductList::GetProducts() const
            {
                return m_products;
            }
        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ