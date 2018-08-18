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

#include <AzCore/Component/Component.h>
#include <StaticDataInterface.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h> 
#include <AzCore/std/parallel/mutex.h>

#include <StaticData/StaticDataBus.h>
#include <DynamicContent/DynamicContentBus.h>


namespace CloudCanvas
{
    namespace StaticData
    {
        class StaticDataAsset
        {
        public:
            // Generic Static Data asset
            AZ_TYPE_INFO(StaticDataAsset, "{9555866C-A706-4B07-9A61-413163ADB93F}");
        };

        using StaticDataInterfacePtr = AZStd::shared_ptr<const StaticDataInterface>;
        using StaticDataMapType = AZStd::unordered_map<StaticDataTagType, StaticDataInterfacePtr>;
        using StaticDataExtensionList = AZStd::vector<StaticDataExtension>;

        static const char* csvDir = "staticdata/csv";

        class StaticDataManager
            : public StaticDataRequestBus::Handler, 
            public CloudCanvas::DynamicContent::DynamicContentUpdateBus::Handler,
            public AZ::Component
        {
        public:
            AZ_COMPONENT(StaticDataManager, "{CA11D812-4BCD-4A02-A569-040B84633BC9}");

            StaticDataManager();
            ~StaticDataManager() override;

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void Reflect(AZ::ReflectContext* reflection);

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            void Init() override;
            void Activate() override;
            void Deactivate() override;


            ReturnInt GetIntValue(const char* tagName, const char* structName, const char* fieldName, bool& wasSuccess) override;
            ReturnStr GetStrValue(const char* tagName, const char* structName, const char* fieldName, bool& wasSuccess) override;
            ReturnDouble GetDoubleValue(const char* tagName, const char* structName, const char* fieldName, bool& wasSuccess) override;

            bool ReloadTagType(const char* tagName) override;
            void LoadRelativeFile(const char* relativeFile) override;

            StaticDataTypeList GetDataTypeList() override;

            enum class StaticDataType
            {
                NONE = 0,
                CSV = 1,
            };

            StaticDataInterfacePtr GetDataType(const char* tagName) const;
            AZStd::mutex& GetDataMutex() const { return m_dataMutex; }

            StaticDataType GetTypeFromExtension(const AZStd::string& extensionStr) const;
            AZStd::string GetExtensionFromFile(const char* fileName) const;
            StaticDataType GetTypeFromFile(const char* fileName) const;
            StaticDataTagType GetTagFromFile(const char* fileName) const;

            void NewContentReady(const AZStd::string& filePath) override;
            void NewPakContentReady(const AZStd::string& pakFileName) override;
            void RequestsCompleted() override;

            static AZStd::string ResolveAndSanitize(const char* dirName);
            static void MakeEndInSlash(AZStd::string& someString);
            static AZStd::string GetDirectoryFromFullPath(const AZStd::string& filePath);

        private:
            StaticDataManager(const StaticDataManager&) = delete;
            using StaticDataInterfacePtrInternal = AZStd::shared_ptr<StaticDataInterface>;

            ReturnInt GetIntValueInternal(const char* tagName, const char* structName, const char* fieldName, bool& wasSuccess) const;
            ReturnStr GetStrValueInternal(const char* tagName, const char* structName, const char* fieldName, bool& wasSuccess) const;
            ReturnDouble GetDoubleValueInternal(const char* tagName, const char* structName, const char* fieldName, bool& wasSuccess) const;

            bool ReloadType(const char* tagName);

            void LoadAssetAndUserDirectory(const char* dirName, const char* extensionType, StaticDataType dataType);
            void LoadDirectoryDataType(const char* dirName, const char* fileExtension, StaticDataType dataType);

            bool LoadAll();

            void AddExtensionType(const char* extensionStr, StaticDataType dataType);

            StaticDataInterfacePtrInternal CreateInterface(StaticDataType dataType, const char* initData, const char* tagName);
            void RemoveInterface(const char* tagName);
            void SetInterface(const char* tagName, StaticDataInterfacePtrInternal someInterface);

            StaticDataExtensionList GetExtensionsForDirectory(const char* dirName) const;
            void AddExtensionForDirectory(const char* dirName, const char* extensionName);

            void GetFilesForExtension(const char* dirName, const char* extensionName, StaticDataFileSet& addSet) const;
            StaticDataFileSet GetFilesForDirectory(const char* dirName) override;

            bool IsLoadedData(const AZStd::string& filePath) const;
            bool HasDirectoryAndExtension(const AZStd::string& directoryString, const AZStd::string& extensionString) const;

            StaticDataMapType m_data;

            AZStd::unordered_map<StaticDataTagType, StaticDataType> m_extensionToTypeMap;
            AZStd::unordered_map<AZStd::string, StaticDataExtensionList> m_directoryToExtensionMap;

            mutable AZStd::mutex m_dataMutex;

            mutable AZStd::mutex m_extensionToTypeMutex;
            mutable AZStd::mutex m_directoryToExtensionMutex;

            static AZ::EntityId m_moduleEntity;
        };
    }
}