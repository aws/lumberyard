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

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            AZ_CLASS_ALLOCATOR_IMPL(SceneManifest, AZ::SystemAllocator, 0)

            SceneManifest::~SceneManifest()
            {
            }
            
            void SceneManifest::Clear()
            {
                m_storageLookup.clear();
                m_values.clear();
            }

            bool SceneManifest::AddEntry(AZStd::shared_ptr<DataTypes::IManifestObject>&& value)
            {
                auto itValue = m_storageLookup.find(value.get());
                if (itValue != m_storageLookup.end())
                {
                    AZ_TracePrintf(Utilities::WarningWindow, "Manifest Object has already been registered with the manifest.");
                    return false;
                }

                Index index = aznumeric_caster(m_values.size());
                m_storageLookup[value.get()] = index;
                m_values.push_back(AZStd::move(value));

                AZ_Assert(m_values.size() == m_storageLookup.size(), 
                    "SceneManifest values and storage-lookup tables have gone out of lockstep (%i vs %i)", 
                    m_values.size(), m_storageLookup.size());
                return true;
            }

            bool SceneManifest::RemoveEntry(const DataTypes::IManifestObject* const value)
            {
                auto storageLookupIt = m_storageLookup.find(value);
                if (storageLookupIt == m_storageLookup.end())
                {
                    AZ_Assert(false, "Value not registered in SceneManifest.");
                    return false;
                }

                size_t index = storageLookupIt->second;

                m_values.erase(m_values.begin() + index);
                m_storageLookup.erase(storageLookupIt);

                for (auto& entry : m_storageLookup)
                {
                    if (entry.second > index)
                    {
                        entry.second--;
                    }
                }

                return true;
            }

            SceneManifest::Index SceneManifest::FindIndex(const DataTypes::IManifestObject* const value) const
            {
                auto it = m_storageLookup.find(value);
                return it != m_storageLookup.end() ? (*it).second : s_invalidIndex;
            }

            bool SceneManifest::LoadFromStream(IO::GenericStream& stream, SerializeContext* context)
            {
                // Merging of manifests probably introduces more problems than it would solve. In particular there's a high risk of names
                //      colliding. If merging is every required a dedicated function would be preferable that would allow some measure of
                //      control over the merging behavior. To avoid merging manifests, always clear out the old manifest.
                Clear();

                // Gems can be removed, causing the setting for manifest objects in the the Gem to not be registered. Instead of failing
                //      to load the entire manifest, just ignore those values.
                ObjectStream::FilterDescriptor loadFilter(&AZ::Data::AssetFilterNoAssetLoading, ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
                return Utils::LoadObjectFromStreamInPlace<SceneManifest>(stream, *this, context, loadFilter);
            }
            
            bool SceneManifest::SaveToStream(IO::GenericStream& stream, SerializeContext* context) const
            {
                return Utils::SaveObjectToStream<SceneManifest>(stream, ObjectStream::ST_XML, this, context);
            }


            bool SceneManifest::SaveToFile(const char* filename, SerializeContext* serializeContext)
            {
                AZ_TraceContext("SceneManifest", filename);
                bool result = false;

                IO::SystemFile manifestFile;
                if (manifestFile.Open(filename, IO::SystemFile::SF_OPEN_READ_WRITE | IO::SystemFile::SF_OPEN_CREATE))
                {
                    IO::SystemFileStream manifestFileStream(&manifestFile, false);
                    AZ_Assert(manifestFileStream.IsOpen(), "Manifest file stream is not open.");

                    if (!serializeContext)
                    {
                        EBUS_EVENT_RESULT(serializeContext, ComponentApplicationBus, GetSerializeContext);
                        if (!serializeContext)
                        {
                            AZ_Error("SceneManifest", false, "No serialization context was created.");
                            manifestFileStream.Close();
                            manifestFile.Close();
                            return false;
                        }
                    }

                    result = SaveToStream(manifestFileStream, serializeContext);
                    manifestFileStream.Close();
                    manifestFile.Close();
                    result = true;
                }
                else
                {
                    AZ_Error("SceneManifest", false, "Cannot open manifest file for writing.");
                }

                return result;
            }


            AZStd::shared_ptr<const DataTypes::IManifestObject> SceneManifest::SceneManifestConstDataConverter(
                const AZStd::shared_ptr<DataTypes::IManifestObject>& value)
            {
                return AZStd::shared_ptr<const DataTypes::IManifestObject>(value);
            }

            void SceneManifest::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<SceneManifest>()
                    ->Version(1, &SceneManifest::VersionConverter)
                    ->EventHandler<SerializeEvents>()
                    ->Field("values", &SceneManifest::m_values);
            }

            bool SceneManifest::VersionConverter(SerializeContext& context, SerializeContext::DataElementNode& node)
            {
                if (node.GetVersion() != 0)
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Unable to upgrade SceneManifest from version %i.", node.GetVersion());
                    return false;
                }

                // Copy out the original values.
                AZStd::vector<SerializeContext::DataElementNode> values;
                values.reserve(node.GetNumSubElements());
                for (int i = 0; i < node.GetNumSubElements(); ++i)
                {
                    // The old format stored AZStd::pair<AZStd::string, AZStd::shared_ptr<IManifestObjets>>. All this
                    //      data is still used, but needs to be move to the new location. The shared ptr needs to be
                    //      moved into the new container, while the name needs to be moved to the group name.
                    
                    SerializeContext::DataElementNode& pairNode = node.GetSubElement(i);
                    // This is the original content of the shared ptr. Using the shared pointer directly caused
                    //      registration issues so it's extracting the data the shared ptr was storing instead.
                    SerializeContext::DataElementNode& elementNode = pairNode.GetSubElement(1).GetSubElement(0);
                    
                    SerializeContext::DataElementNode& nameNode = pairNode.GetSubElement(0);
                    AZStd::string name;
                    if (nameNode.GetData(name))
                    {
                        elementNode.AddElementWithData<AZStd::string>(context, "name", name);
                    }
                    // It's better not to set a default name here as the default behaviors will take care of that
                    //      will have more information to work with.

                    values.push_back(elementNode);
                }

                // Delete old values
                for (int i = 0; i < node.GetNumSubElements(); ++i)
                {
                    node.RemoveElement(i);
                }

                // Put stored values back
                int vectorIndex = node.AddElement<ValueStorage>(context, "values");
                SerializeContext::DataElementNode& vectorNode = node.GetSubElement(vectorIndex);
                for (SerializeContext::DataElementNode& value : values)
                {
                    value.SetName("element");
                    
                    // Put in a blank shared ptr to be filled with a value stored from "values".
                    int valueIndex = vectorNode.AddElement<ValueStorageType>(context, "element");
                    SerializeContext::DataElementNode& pointerNode = vectorNode.GetSubElement(valueIndex);
                    
                    // Type doesn't matter as it will be overwritten by the stored value.
                    pointerNode.AddElement<int>(context, "element");
                    pointerNode.GetSubElement(0) = value;
                }

                AZ_TracePrintf(Utilities::WarningWindow,
                    "The SceneManifest has been updated from version %i. It's recommended to save the updated file.", node.GetVersion());
                return true;
            }

            void SceneManifest::SerializeEvents::OnWriteBegin(void* classPtr)
            {
                SceneManifest* manifest = reinterpret_cast<SceneManifest*>(classPtr);
                manifest->Clear();
            }

            void SceneManifest::SerializeEvents::OnWriteEnd(void* classPtr)
            {
                SceneManifest* manifest = reinterpret_cast<SceneManifest*>(classPtr);

                auto end = AZStd::remove_if(manifest->m_values.begin(), manifest->m_values.end(), 
                    [](const ValueStorageType& entry) -> bool
                    {
                        return !entry;
                    });
                manifest->m_values.erase(end, manifest->m_values.end());

                for (size_t i = 0; i < manifest->m_values.size(); ++i)
                {
                    Index index = aznumeric_caster(i);
                    manifest->m_storageLookup[manifest->m_values[i].get()] = index;
                }
            }
        } // Containers
    } // SceneAPI
} // AZ
