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

#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/utils.h>

namespace CloudCanvas
{
    namespace StaticData
    {

        using ReturnInt = int;
        using ReturnStr = AZStd::string;
        using ReturnDouble = double;

        class StaticDataUpdateGroup
            : public AZ::EBusTraits
        {
        public:
            virtual void TypeReloaded(const AZStd::string& reloadType) {}
            virtual void StaticDataFileAdded(const AZStd::string& filePath) {}

            virtual ~StaticDataUpdateGroup() {}
        };

        typedef AZ::EBus<StaticDataUpdateGroup> StaticDataUpdateBus;

        class StaticDataInterface
        {
        public:
            virtual ReturnInt GetIntValue(const char* structName, const char* fieldName, bool& wasSuccess) const = 0;
            virtual ReturnStr GetStrValue(const char* structName, const char* fieldName, bool& wasSuccess) const = 0;
            virtual ReturnDouble GetDoubleValue(const char* structName, const char* fieldName, bool& wasSuccess) const = 0;

            friend class StaticDataManager;
        protected:
            virtual bool LoadData(const char* dataBuffer) = 0;
        };
    }
}