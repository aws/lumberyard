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
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <AzCore/std/containers/unordered_map.h>

namespace Aws
{
    namespace DynamoDB
    {
        namespace Model
        {
            class AttributeValue;
        }
    }
}

namespace LmbrAWS
{
    namespace DynamoDBUtils
    {
        Aws::DynamoDB::Model::AttributeValue MakeAttributeValue(const char* dataType, const char* valueStr);

        void GetDataEnumString(Aws::StringStream& ss);
        const char* GetDefaultEnumString();

        // Helper call for DynamoDB nodes to add condition expressions
        void AddAttributeRequirement(const char* requirementType, const char* requirementName, Aws::String& buildStr);

        static const char* const LABEL_EQUALS = "EQUALS";
        static const char* const LABEL_LESS_THAN = "LESS_THAN";
        static const char* const LABEL_LESS_THAN_OR_EQUALS = "LESS_THAN_OR_EQUALS";
        static const char* const LABEL_GREATER_THAN = "GREATER_THAN";
        static const char* const LABEL_GREATER_THAN_OR_EQUALS = "GREATER_THAN_OR_EQUALS";

        static const char* const DATA_TYPE_STRING = "string";
        static const char* const DATA_TYPE_NUMBER = "number";
        static const char* const DATA_TYPE_BOOL = "bool";

        Aws::String GetDynamoDBStringByLabel(const char* comparisonTypeLabel);
        void GetComparisonTypeEnumString(Aws::StringStream& ss);
        const char* GetDefaultComparisonString();
    } // namespace DynamoDBUtils
} // namespace LmbrAWS


