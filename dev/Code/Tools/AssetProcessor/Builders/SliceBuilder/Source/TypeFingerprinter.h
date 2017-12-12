/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_map.h>

namespace SliceBuilder
{
    /**
     * A fingerprint based on a type's reflected data in the AZ::SerializeContext.
     */
    using TypeFingerprint = size_t;

    static const TypeFingerprint InvalidTypeFingerprint = 0;

    /**
     * Generates fingerprints for each type known to the AZ::SerializeContext.
     */
    class TypeFingerprinter
    {
    public:
        void CreateFingerprintsForAllTypes(const AZ::SerializeContext& serializeContext);

        inline TypeFingerprint GetFingerprint(const AZ::TypeId& typeId) const
        {
            auto found = m_typeFingerprints.find(typeId);
            if (found != m_typeFingerprints.end())
            {
                return found->second;
            }
            return InvalidTypeFingerprint;
        }

    private:

        TypeFingerprint CreateFingerprint(const AZ::SerializeContext& serializeContext, const AZ::SerializeContext::ClassData& classData);

        AZStd::unordered_map<AZ::TypeId, TypeFingerprint> m_typeFingerprints;
    };
} // namespace SliceBuilder