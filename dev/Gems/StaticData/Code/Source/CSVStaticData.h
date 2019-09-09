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
#include <StaticDataInterface.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace CloudCanvas
{
    namespace StaticData
    {
        using AttributeKeyType = AZStd::string;
        using AttributeValueType = AZStd::string;
        using AttributeMap = AZStd::unordered_map<AttributeKeyType, AttributeValueType>;
        using AttributeMapPtr = AZStd::shared_ptr<AttributeMap>;
        using DataVec = AZStd::vector<AttributeMapPtr>;
        using AttributeReturnType = AZStd::pair<bool, AttributeValueType>;
        using AttributeVec = AZStd::vector<AttributeValueType>;

        class CSVStaticData
            : public StaticDataInterface
        {
        public:
            CSVStaticData();
            virtual ~CSVStaticData() = default;

            virtual ReturnInt GetIntValue(const char* structName, const char* fieldName, bool& wasSuccess) const override;
            virtual ReturnStr GetStrValue(const char* structName, const char* fieldName, bool& wasSuccess) const override;
            virtual ReturnDouble GetDoubleValue(const char* structName, const char* fieldName, bool& wasSuccess) const override;

            size_t GetNumElements() const override;

            static AttributeValueType ParseFromStream(std::stringstream& inStream);
        protected:
            bool LoadData(const char* initBuffer) override;
        private:
            // Helpers to get attribute value data

            // Find the attribute map for the given key
            AttributeMapPtr GetAttributeMap(const char* keyName) const;

            // Find the attribute value for the given key and attribute combination
            AttributeReturnType GetAttributeValue(const char* keyName, const char* attributeName) const;

            AttributeKeyType m_keyName;
            AttributeVec m_attributes;
            DataVec m_dataVec;
        };
    }
}
