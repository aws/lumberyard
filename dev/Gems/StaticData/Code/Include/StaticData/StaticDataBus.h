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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>
#include <StaticDataInterface.h>

namespace CloudCanvas
{
    namespace StaticData
    {
        using StringReturnType = AZStd::string;
        using StaticDataTagType = AZStd::string;
        using StaticDataExtension = AZStd::string;
        using StaticDataTypeList = AZStd::vector<StaticDataTagType>;
        using StaticDataFileSet = AZStd::unordered_set<AZStd::string>;

        class IStaticDataMonitor;

        class IStaticDataManager 
            : public AZ::ComponentBus
        {
        public:
            virtual ReturnInt GetIntValue(const char* tagName, const char* structName, const char* fieldName, bool& wasSuccess) = 0;
            virtual ReturnStr GetStrValue(const char* tagName, const char* structName, const char* fieldName, bool& wasSuccess) = 0;
            virtual ReturnDouble GetDoubleValue(const char* tagName, const char* structName, const char* fieldName, bool& wasSuccess) = 0;

            virtual bool ReloadTagType(const char* tagName) = 0;
            virtual void LoadRelativeFile(const char* relativeFile) = 0;

            virtual StaticDataTypeList GetDataTypeList() = 0;
            virtual StaticDataFileSet GetFilesForDirectory(const char* dirName) = 0;

            friend class StaticDataMonitor;
        };

        using StaticDataRequestBus = AZ::EBus<IStaticDataManager>;
    }
}