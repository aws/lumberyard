#pragma once

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

#include <stdint.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>
#include <SceneAPI/SceneCore/Containers/Views/View.h>
#include <SceneAPI/SceneCore/Containers/Views/ConvertIterator.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>

namespace AZ
{
    namespace IO
    {
        class GenericStream;
    }

    namespace SceneAPI
    {
        namespace Containers
        {
            // Scene manifests hold arbitrary meta data about a scene in a dictionary-like fashion.
            //      This can include data such as export groups.
            class SceneManifest
            {
                friend class SceneManifestContainer;
            public:
                AZ_CLASS_ALLOCATOR_DECL

                AZ_RTTI(SceneManifest, "{9274AD17-3212-4651-9F3B-7DCCB080E467}");
                
                SCENE_CORE_API virtual ~SceneManifest();
                
                SCENE_CORE_API static AZStd::shared_ptr<const DataTypes::IManifestObject> SceneManifestConstDataConverter(
                    const AZStd::shared_ptr<DataTypes::IManifestObject>& value);

                using Index = size_t;
                static const Index s_invalidIndex = static_cast<Index>(-1);

                using StorageHash = const DataTypes::IManifestObject *;
                using StorageLookup = AZStd::unordered_map<StorageHash, Index>;

                using ValueStorageType = AZStd::shared_ptr<DataTypes::IManifestObject>;
                using ValueStorage = AZStd::vector<ValueStorageType>;
                using ValueStorageData = Views::View<ValueStorage::const_iterator>;

                using ValueStorageConstDataIteratorWrapper = Views::ConvertIterator<ValueStorage::const_iterator,
                            decltype(SceneManifestConstDataConverter(AZStd::shared_ptr<DataTypes::IManifestObject>()))>;
                using ValueStorageConstData = Views::View<ValueStorageConstDataIteratorWrapper>;

                SCENE_CORE_API void Clear();
                inline bool IsEmpty() const;

                inline bool AddEntry(const AZStd::shared_ptr<DataTypes::IManifestObject>& value);
                SCENE_CORE_API bool AddEntry(AZStd::shared_ptr<DataTypes::IManifestObject>&& value);
                inline bool RemoveEntry(const AZStd::shared_ptr<DataTypes::IManifestObject>& value);
                SCENE_CORE_API bool RemoveEntry(const DataTypes::IManifestObject* const value);

                inline size_t GetEntryCount() const;
                inline AZStd::shared_ptr<DataTypes::IManifestObject> GetValue(Index index);
                inline AZStd::shared_ptr<const DataTypes::IManifestObject> GetValue(Index index) const;
                // Finds the index of the given manifest object. A nullptr or invalid object will return s_invalidIndex.
                inline Index FindIndex(const AZStd::shared_ptr<DataTypes::IManifestObject>& value) const;
                // Finds the index of the given manifest object. A nullptr or invalid object will return s_invalidIndex.
                SCENE_CORE_API Index FindIndex(const DataTypes::IManifestObject* const value) const;

                inline ValueStorageData GetValueStorage();
                inline ValueStorageConstData GetValueStorage() const;

                // If SceneManifest if part of a class normal reflection will work as expected, but if it's directly serialized
                //      additional arguments for the common serialization functions are required. This function is utility uses
                //      the correct arguments to load the SceneManifest from a stream in-place.
                SCENE_CORE_API bool LoadFromStream(IO::GenericStream& stream, SerializeContext* context = nullptr);
                // If SceneManifest if part of a class normal reflection will work as expected, but if it's directly serialized
                //      additional arguments for the common serialization functions are required. This function is utility uses
                //      the correct arguments to save the SceneManifest to a stream.
                SCENE_CORE_API bool SaveToStream(IO::GenericStream& stream, SerializeContext* context = nullptr) const;

                /**
                 * Save manifest to file. Overwrites the file in case it already exists and creates a new file if not.
                 * @param serializeContext If no serialize context was specified, it will get the serialize context from the application component bus.
                 * @result True in case saving went all fine, false if an error occured.
                 */
                SCENE_CORE_API bool SaveToFile(const char* filename, SerializeContext* serializeContext = nullptr);

                SCENE_CORE_API static void Reflect(ReflectContext* context);
                static bool VersionConverter(SerializeContext& context, SerializeContext::DataElementNode& node);

            private:
                struct SerializeEvents : public SerializeContext::IEventHandler
                {
                    void OnWriteBegin(void* classPtr) override;
                    void OnWriteEnd(void* classPtr) override;
                };
                
                StorageLookup m_storageLookup;
                ValueStorage m_values;
            };
        } // Containers
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/Containers/SceneManifest.inl>
