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
#include <SliceBuilder/Source/TypeFingerprinter.h>

namespace SliceBuilder
{
    void TypeFingerprinter::CreateFingerprintsForAllTypes(const AZ::SerializeContext& serializeContext)
    {
        serializeContext.EnumerateAll(
            [this, &serializeContext](const AZ::SerializeContext::ClassData* classData, const AZ::TypeId&)
            {
                CreateFingerprint(serializeContext, *classData);
                return true;
            });
    }

    TypeFingerprint TypeFingerprinter::CreateFingerprint(const AZ::SerializeContext& serializeContext, const AZ::SerializeContext::ClassData& classData)
    {
        TypeFingerprint fingerprint = 0;

        AZStd::hash_combine(fingerprint, classData.m_typeId);
        AZStd::hash_combine(fingerprint, classData.m_version);

        for (const AZ::SerializeContext::ClassElement& element : classData.m_elements)
        {
            if (element.m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS)
            {
                // Incorporate base classes' fingerprints into this type's fingerprint.
                auto baseFingerprintFound = m_typeFingerprints.find(element.m_typeId);
                if (baseFingerprintFound != m_typeFingerprints.end())
                {
                    // base class already fingerprinted
                    AZStd::hash_combine(fingerprint, baseFingerprintFound->second);
                }
                else if (const AZ::SerializeContext::ClassData* baseClassData = serializeContext.FindClassData(element.m_typeId))
                {
                    // base class hasn't been fingerprinted yet
                    AZStd::hash_combine(fingerprint, CreateFingerprint(serializeContext, *baseClassData));
                }
                else
                {
                    // base class not found in serialize context
                    AZStd::hash_combine(fingerprint, element.m_typeId);
                }
            }
            else
            {
                // member is not a base class
                AZStd::hash_combine(fingerprint, element.m_typeId);
                AZStd::hash_range(fingerprint, element.m_name, element.m_name + strlen(element.m_name));
                AZStd::hash_combine(fingerprint, element.m_flags);
            }
        }

        m_typeFingerprints[classData.m_typeId] = fingerprint;
        return fingerprint;
    }
} // namespace SliceBuilder
