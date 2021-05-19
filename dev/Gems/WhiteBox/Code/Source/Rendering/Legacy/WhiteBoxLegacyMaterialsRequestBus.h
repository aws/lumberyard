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

#pragma once

#include <CryCommon/smartptr.h>

#include <AzCore/EBus/EBus.h>

struct IMaterial;

namespace WhiteBox
{
    //! Look-up for White Box materials.
    struct WhiteBoxLegacyMaterialKey
    {
        AZ::u32 m_tint;
        AZ::Crc32 m_texture;
    };

    //! Interface to store and retrieve White Box materials.
    class WhiteBoxLegacyMaterialsRequests : public AZ::EBusTraits
    {
    public:
        virtual _smart_ptr<IMaterial> FindMaterial(const WhiteBoxLegacyMaterialKey& materialKey) = 0;
        virtual void AddMaterial(const WhiteBoxLegacyMaterialKey& materialKey, _smart_ptr<IMaterial> material) = 0;

    protected:
        ~WhiteBoxLegacyMaterialsRequests() = default;
    };

    inline bool operator==(const WhiteBoxLegacyMaterialKey& lhs, const WhiteBoxLegacyMaterialKey& rhs)
    {
        return lhs.m_texture == rhs.m_texture && lhs.m_tint == rhs.m_tint;
    }

    inline bool operator!=(const WhiteBoxLegacyMaterialKey& lhs, const WhiteBoxLegacyMaterialKey& rhs)
    {
        return !(lhs == rhs);
    }

    using WhiteBoxLegacyMaterialsRequestBus = AZ::EBus<WhiteBoxLegacyMaterialsRequests>;
} // namespace WhiteBox

namespace AZStd
{
    template<>
    struct hash<WhiteBox::WhiteBoxLegacyMaterialKey>
    {
        inline size_t operator()(const WhiteBox::WhiteBoxLegacyMaterialKey& materialKey) const
        {
            AZStd::size_t result = 0;
            AZStd::hash_combine(result, static_cast<AZ::u32>(materialKey.m_texture));
            AZStd::hash_combine(result, materialKey.m_tint);
            return result;
        }
    };
} // namespace AZStd
