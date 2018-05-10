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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/ObjectStream.h>

#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/IO/FileIO.h>

#include <AzCore/Debug/Profiler.h>

namespace AZ
{
    namespace Utils
    {
        bool LoadObjectFromStreamInPlace(IO::GenericStream& stream, AZ::SerializeContext* context, const SerializeContext::ClassData* objectClassData, void* targetPointer, const FilterDescriptor& filterDesc)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            AZ_Assert(objectClassData, "Class data is required.");

            if (!context)
            {
                EBUS_EVENT_RESULT(context, ComponentApplicationBus, GetSerializeContext);
                AZ_Assert(context, "No serialize context");
            }

            if (!context)
            {
                return false;
            }

            AZ_Assert(targetPointer, "You must provide a target pointer");

            bool foundSuccess = false;
            typedef AZStd::function<void(void**, const SerializeContext::ClassData**, const Uuid&, SerializeContext*)> CreationCallback;
            auto handler = [&targetPointer, objectClassData, &foundSuccess](void** instance, const SerializeContext::ClassData** classData, const Uuid& classId, SerializeContext*)
                {
                    if (classId == objectClassData->m_typeId)
                    {
                        foundSuccess = true;
                        if (instance)
                        {
                            // The ObjectStream will ask us for the address of the target to load into, so provide it.
                            *instance = targetPointer;
                        }
                        if (classData)
                        {
                            // The ObjectStream will ask us for the class data of the target being loaded into, so provide it if needed.
                            // This allows us to load directly into a generic object (templated containers, strings, etc).
                            *classData = objectClassData;
                        }
                    }
                };
            bool readSuccess = ObjectStream::LoadBlocking(&stream, *context, ObjectStream::ClassReadyCB(), filterDesc, CreationCallback(handler, AZ::OSStdAllocator()));

            AZ_Warning("Serialization", readSuccess, "LoadObjectFromStreamInPlace: Stream did not deserialize correctly");
            AZ_Warning("Serialization", foundSuccess, "LoadObjectFromStreamInPlace: Did not find the expected type in the stream");

            return readSuccess && foundSuccess;
        }

        bool LoadObjectFromStreamInPlace(IO::GenericStream& stream, AZ::SerializeContext* context, const Uuid& targetClassId, void* targetPointer, const FilterDescriptor& filterDesc)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            if (!context)
            {
                EBUS_EVENT_RESULT(context, ComponentApplicationBus, GetSerializeContext);
                AZ_Assert(context, "No serialize context");
            }

            if (!context)
            {
                return false;
            }

            const SerializeContext::ClassData* classData = context->FindClassData(targetClassId);
            if (!classData)
            {
                AZ_Error("Serialization", false,
                         "Unable to locate class data for uuid \"%s\". This object cannot be serialized as a root element. "
                         "Make sure the Uuid is valid, or if this is a generic type, use the override that takes a ClassData pointer instead.",
                         targetClassId.ToString<AZStd::string>().c_str());
                return false;
            }

            return LoadObjectFromStreamInPlace(stream, context, classData, targetPointer, filterDesc);
        }

        bool LoadObjectFromFileInPlace(const AZStd::string& filePath, const Uuid& targetClassId, void* destination, AZ::SerializeContext* context /*= nullptr*/, const FilterDescriptor& filterDesc /*= FilterDescriptor()*/)
        {
            AZ::IO::FileIOStream fileStream;
            if (!fileStream.Open(filePath.c_str(), IO::OpenMode::ModeRead | IO::OpenMode::ModeBinary))
            {
                return false;
            }

            return LoadObjectFromStreamInPlace(fileStream, context, targetClassId, destination, filterDesc);
        }

        void* LoadObjectFromStream(IO::GenericStream& stream, AZ::SerializeContext* context, const Uuid* targetClassId, const FilterDescriptor& filterDesc)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            if (!context)
            {
                EBUS_EVENT_RESULT(context, ComponentApplicationBus, GetSerializeContext);
                AZ_Assert(context, "No serialize context");
            }

            if (!context)
            {
                return nullptr;
            }

            void* loadedInstance = nullptr;
            AZ::Uuid loadedClassID = AZ::Uuid::CreateNull();

            bool success = ObjectStream::LoadBlocking(&stream, *context,
                    [&loadedInstance, &loadedClassID, targetClassId](void* classPtr, const Uuid& classId, const SerializeContext* serializeContext)
                    {
                        if (targetClassId)
                        {
                            void* instance = serializeContext->DownCast(classPtr, classId, *targetClassId);

                            // Given a valid object
                            if (instance)
                            {
                                AZ_Assert(!loadedInstance, "loadedInstance must be NULL, otherwise we are being invoked with multiple valid objects");
                                loadedInstance = instance;
                                loadedClassID = classId;
                                return;
                            }
                        }
                        else
                        {
                            if (!loadedInstance)
                            {
                                loadedInstance = classPtr;
                                loadedClassID = classId;
                                return;
                            }
                        }

                        auto classData = serializeContext->FindClassData(classId);
                        if (classData && classData->m_factory)
                        {
                            classData->m_factory->Destroy(classPtr);
                        }
                    },
                    filterDesc,
                    ObjectStream::InplaceLoadRootInfoCB()
                    );

            if (!success)
            {
                // if serialization failed, clean up by destroying the instance that was loaded.
                if (loadedInstance)
                {
                    const AZ::SerializeContext::ClassData* classData = context->FindClassData(loadedClassID);
                    if (classData && classData->m_factory)
                    {
                        classData->m_factory->Destroy(loadedInstance);
                    }
                }
                return nullptr;
            }

            return loadedInstance;
        }

        void* LoadObjectFromFile(const AZStd::string& filePath, const Uuid& targetClassId, SerializeContext* context, const FilterDescriptor& filterDesc, int /*platformFlags*/)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            AZ::IO::FileIOStream fileStream;           
            if (!fileStream.Open(filePath.c_str(), IO::OpenMode::ModeRead | IO::OpenMode::ModeBinary))
            {
                return nullptr;
            }

            void* loadedObject = LoadObjectFromStream(fileStream, context, &targetClassId, filterDesc);
            return loadedObject;
        }

        bool SaveObjectToStream(IO::GenericStream& stream, DataStream::StreamType streamType, const void* classPtr, const Uuid& classId, SerializeContext* context, const SerializeContext::ClassData* classData)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            if (!context)
            {
                EBUS_EVENT_RESULT(context, ComponentApplicationBus, GetSerializeContext);

                if(!context)
                {
                    AZ_Assert(false, "No serialize context");
                    return false;
                }
            }

            if (!classPtr)
            {
                AZ_Assert(false, "SaveObjectToStream: classPtr is null, object cannot be serialized.");
                return false;
            }

            AZ::ObjectStream* objectStream = AZ::ObjectStream::Create(&stream, *context, streamType);
            if (!objectStream)
            {
                return false;
            }

            if (!objectStream->WriteClass(classPtr, classId, classData))
            {
                objectStream->Finalize();
                return false;
            }

            if (!objectStream->Finalize())
            {
                return false;
            }

            return true;
        }

        bool SaveObjectToFile(const AZStd::string& filePath, DataStream::StreamType fileType, const void* classPtr, const Uuid& classId, SerializeContext* context, int platformFlags)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzCore);

            // \note This is ok for tools, but we should use the streamer to write objects directly (no memory store)
            AZStd::vector<AZ::u8> dstData;
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > dstByteStream(&dstData);

            if (!SaveObjectToStream(dstByteStream, fileType, classPtr, classId, context))
            {
                return false;
            }

            AZ::IO::SystemFile file;
            file.Open(filePath.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY, platformFlags);
            if (!file.IsOpen())
            {
                file.Close();
                return false;
            }

            file.Write(dstData.data(), dstData.size());

            file.Close();
            return true;
        }

        /*!
        \brief Finds any descendant DataElementNodes of the @classElement which match each Crc32 values in the supplied elementCrcQueue
        \param context SerializeContext used for looking up ClassData
        \param classElement Top level DataElementNode to begin comparison each the Crc32 queue
        \param elementCrcQueue Container of Crc32 values in the order in which DataElementNodes should be matched as the DataElementNode tree is traversed
        \return Vector of valid pointers to DataElementNodes which match the entire element Crc32 queue
        */
        AZStd::vector<AZ::SerializeContext::DataElementNode*> FindDescendantElements(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement,
            const AZStd::vector<AZ::Crc32>& elementCrcQueue)
        {
            AZStd::vector<AZ::SerializeContext::DataElementNode*> dataElementNodes;
            FindDescendantElements(context, classElement, dataElementNodes, elementCrcQueue.begin(), elementCrcQueue.end());

            return dataElementNodes;
        }

        /*!
        \brief Finds any descendant DataElementNodes of the @classElement which match each Crc32 values in the supplied elementCrcQueue
        \param context SerializeContext used for looking up ClassData
        \param classElement The current DataElementNode which will be compared against be to current top Crc32 value in the Crc32 queue
        \param dataElementNodes[out] Array to populate with a DataElementNode which was found by matching all Crc32 values in the Crc32 queue
        \param first The current front of the Crc32 queue
        \param last The end of the Crc32 queue
        */
        void FindDescendantElements(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement,
            AZStd::vector<AZ::SerializeContext::DataElementNode*>& dataElementNodes, AZStd::vector<AZ::Crc32>::const_iterator first, AZStd::vector<AZ::Crc32>::const_iterator last)
        {
            if (first == last)
            {
                return;
            }

            for (int i = 0; i < classElement.GetNumSubElements(); ++i)
            {
                auto& childElement = classElement.GetSubElement(i);
                if (*first == AZ::Crc32(childElement.GetName()))
                {
                    if (AZStd::distance(first, last) == 1)
                    {
                        dataElementNodes.push_back(&childElement);
                    }
                    else
                    {
                        FindDescendantElements(context, childElement, dataElementNodes, AZStd::next(first), last);
                    }
                }
            }
        }
    } // namespace Utils
} // namespace AZ

#endif // #ifndef AZ_UNITY_BUILD
