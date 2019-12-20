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
#include "AWS_precompiled.h"

// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/dynamodb/model/AttributeValue.h>
AZ_POP_DISABLE_WARNING

#include "DynamoDBUtils.h"

namespace LmbrAWS
{
    namespace DynamoDBUtils
    {
        Aws::DynamoDB::Model::AttributeValue MakeAttributeValue(const char* dataType, const char* valueStr)
        {
            Aws::DynamoDB::Model::AttributeValue data;

            // check default first
            if (!strcmp(dataType, DATA_TYPE_STRING))
            {
                data.SetS(valueStr);
            }
            else if (!strcmp(dataType, DATA_TYPE_NUMBER))
            {
                data.SetN(valueStr);
            }
            else if (!strcmp(dataType, DATA_TYPE_BOOL))
            {
                data.SetBool(!strcmp(valueStr, "1") || !azstricmp(valueStr, "true") || !azstricmp(valueStr, "y"));
            }
            else // Default back to string
            {
                data.SetS(valueStr);
            }
            return data;
        }

        void GetDataEnumString(Aws::StringStream& ss)
        {
            ss << "enum_string:";
            ss << DATA_TYPE_STRING << "=" << DATA_TYPE_STRING << ",";
            ss << DATA_TYPE_NUMBER << "=" << DATA_TYPE_NUMBER << ",";
            ss << DATA_TYPE_BOOL << "=" << DATA_TYPE_BOOL;
        }

        Aws::String GetDynamoDBStringByLabel(const char* comparisonTypeLabel)
        {
            if (std::strcmp(comparisonTypeLabel, LABEL_EQUALS) == 0)
            {
                return "=";
            }
            else if (std::strcmp(comparisonTypeLabel, LABEL_LESS_THAN) == 0)
            {
                return "<";
            }
            else if (std::strcmp(comparisonTypeLabel, LABEL_LESS_THAN_OR_EQUALS) == 0)
            {
                return "<=";
            }
            else if (std::strcmp(comparisonTypeLabel, LABEL_GREATER_THAN) == 0)
            {
                return ">";
            }
            else if (std::strcmp(comparisonTypeLabel, LABEL_GREATER_THAN_OR_EQUALS) == 0)
            {
                return ">=";
            }

            AZ_Assert(false, "No such comparison type mapping");
            return comparisonTypeLabel; // In case an old flow node still has the exact string (=, <, etc), allow backwards compatibility but still assert - it should be updated by the user
        }

        const char* GetDefaultComparisonString()
        {
            return LABEL_EQUALS;
        }

        const char* GetDefaultEnumString()
        {
            return DATA_TYPE_STRING;
        }

        void GetComparisonTypeEnumString(Aws::StringStream& ss)
        {
            ss << "enum_string:";
            ss << LABEL_EQUALS << "=" << LABEL_EQUALS << ",";
            ss << LABEL_LESS_THAN << "=" << LABEL_LESS_THAN << ",";
            ss << LABEL_LESS_THAN_OR_EQUALS << "=" << LABEL_LESS_THAN_OR_EQUALS << ",";
            ss << LABEL_GREATER_THAN << "=" << LABEL_GREATER_THAN << ",";
            ss << LABEL_GREATER_THAN_OR_EQUALS << "=" << LABEL_GREATER_THAN_OR_EQUALS;
        }

        void AddAttributeRequirement(const char* requirementType, const char* requirementName, Aws::String& buildStr)
        {
            if (buildStr.length())
            {
                buildStr += " AND ";
            }
            buildStr += requirementType;
            buildStr += " (";
            buildStr += requirementName;
            buildStr += ")";
        }
    } // namespace DynamoDBUtils
} // namespace LmbrAWS