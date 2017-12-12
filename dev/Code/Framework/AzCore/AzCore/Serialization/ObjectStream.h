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
#ifndef AZCORE_OBJECT_STREAM_H
#define AZCORE_OBJECT_STREAM_H

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/Asset/AssetCommon.h>

/**
 * ObjectStream provides load/save functionality of structured object data.
 * It works in conjunction with SerializeContext. After serializable classes
 * have been reflected in a SerializeContext, objects of such types can be
 * serialized in and out through the ObjectStream interface.
 *
 * Operation:
 * To serialize objects, a GenericStream and SerializeContext are provided to
 * the ObjectStream along with an operation callback and a completion callback.
 * When loading, each root object created by ObjectStream is passed to the ClassReadyCB
 * supplied to transfer ownership of the object to the user.
 * When saving, classWriterCB is called from the saving thread. The user then calls
 * ObjectStream::WriteClass() for each root object that needs to be written before returning
 * from the callback.
 * CompletionCB is called to indicate that the object stream operation is complete.
 */

namespace AZ
{
    namespace IO {
        class Stream;
        class GenericStream;
    }

    namespace ObjectStreamInternal {
        class ObjectStreamImpl;
    }

    class ObjectStream;

    /**
     * TEMP
     */
    class DataStream
    {
    public:
        enum StreamType
        {
            ST_XML,
            ST_JSON,
            ST_BINARY,
        };

        StreamType  GetType() const         { return m_type; }
        void        SetType(StreamType fmt) { m_type = fmt; }

    protected:
        StreamType  m_type;
    };

    /**
     * ObjectStream
     *
     */
    class ObjectStream
        : public DataStream
    {
    public:

        struct Descriptor
        {
        };

        /*
         * Handle used to cancel/query requests
         */
        class Handle
        {
            friend class ObjectStream;
            friend class ObjectStreamInternal::ObjectStreamImpl;

        public:
            Handle()
                : m_job(NULL) {}
            Handle(ObjectStream* job)
                : m_job(job) {}

            ObjectStream* m_job;
        };

        /**
         * Allows the user to provide information about the root element for in-place loading. Can be asked to provide once of two things
         * \param rootAddress when this is not null, you need to provide the address for inplace loading. If you return null the default object factory will be called (like if you
         * did not provide the callback)
         * \param classData if the class Uuid can't be find in the serializeContext (only in very special cases, like generics in-place loading), you will be asked to provide classData.
         * \param classId provided to you for information
         * \param context provided to you for information
         */
        typedef AZStd::function< void (void** /*rootAddress*/, const SerializeContext::ClassData** /*classData*/, const Uuid& /*classId*/, SerializeContext* /*context*/)> InplaceLoadRootInfoCB;

        /// Called for each root object loaded
        typedef AZStd::function< void (void* /*classPtr*/, const Uuid& /*classId*/, SerializeContext* /*context*/) > ClassReadyCB;

        /// Called to indicate that loading has completed
        typedef AZStd::function< void (Handle /*objectstream*/, bool /*success*/) > CompletionCB;

        enum FilterFlags
        {
            FILTERFLAG_IGNORE_UNKNOWN_CLASSES = 1 << 0,  /// If not set, an error is raised for each unknown class encountered during serialization.
        };

        struct FilterDescriptor
        {
            FilterDescriptor(const FilterDescriptor& rhs)
                : m_flags(rhs.m_flags)
                , m_assetCB(rhs.m_assetCB)
            {}
            FilterDescriptor(const Data::AssetFilterCB& assetFilterCB = nullptr, u32 filterFlags = 0)
                : m_flags(filterFlags)
                , m_assetCB(assetFilterCB)
            {}

            u32 m_flags;
            Data::AssetFilterCB m_assetCB;
        };

        /// Create objects from a stream. All processing happens in the caller thread. Returns true on success.
        static bool LoadBlocking(IO::GenericStream* stream, SerializeContext& sc, const ClassReadyCB& readyCB, const FilterDescriptor& filterDesc = FilterDescriptor(), const InplaceLoadRootInfoCB& inplaceRootInfo = InplaceLoadRootInfoCB());

        /// Create a new object stream for writing
        static ObjectStream* Create(IO::GenericStream* stream, SerializeContext& sc, DataStream::StreamType fmt);

        virtual bool WriteClass(const void* classPtr, const Uuid& classId, const SerializeContext::ClassData* classData = nullptr) = 0;

        /// returns true if successfully flushed and closed the object stream, false otherwise
        virtual bool Finalize() = 0;

        /// Cancel a request. To be implemented...
        static bool Cancel(Handle jobHandle);

        /// Writes a root object. Call this inside the ClassWriterCB.
        template<typename T>
        bool WriteClass(const T* obj, const char* elemName = nullptr);

        /// Default asset filter obeys the Asset<> holder's load flags.
        static bool AssetFilterDefault(const Data::Asset<Data::AssetData>& asset);

        /// SlicesOnly filter ignores all asset references except for slices.
        static bool AssetFilterSlicesOnly(const Data::Asset<Data::AssetData>& asset);

        /// NoAssetLoading filter ignores all asset references.
        static bool AssetFilterNoAssetLoading(const Data::Asset<Data::AssetData>& asset);

    protected:
        ObjectStream(SerializeContext* sc)
            : m_sc(sc)   { AZ_Assert(m_sc, "Creating an object stream with sc = NULL is pointless!"); }
        virtual ~ObjectStream() {}

        SerializeContext*                   m_sc;
    };

    template<typename T>
    bool ObjectStream::WriteClass(const T* obj, const char* elemName)
    {
        (void)elemName;
        AZ_Assert(!AZStd::is_pointer<T>::value, "Cannot serialize pointer-to-pointer as root element! This makes no sense!");
        // Call SaveClass with the potential pointer to derived class fully resolved.
        const void* classPtr = SerializeTypeInfo<T>::RttiCast(obj, SerializeTypeInfo<T>::GetRttiTypeId(obj));
        const Uuid& classId = SerializeTypeInfo<T>::GetUuid(obj);
        const SerializeContext::ClassData* classData = m_sc->FindClassData(classId, NULL, 0);
        if (!classData)
        {
            GenericClassInfo* genericClassInfo = SerializeGenericTypeInfo<T>::GetGenericInfo();
            classData = genericClassInfo ? genericClassInfo->GetClassData() : nullptr;
            if (classData)
            {
                char uuidStr[Uuid::MaxStringBuffer];
                SerializeGenericTypeInfo<T>::GetClassTypeId().ToString(uuidStr, Uuid::MaxStringBuffer, false);
                AZ_Error("Serializer", false, "Serialization of generic type (%s,%s) or a derivative as root element is not supported!!", classData->m_name, uuidStr);
            }
            else
            {
                AZ_Error("Serializer", false, "Class '%s' is not registered with the serializer!", SerializeTypeInfo<T>::GetRttiTypeName(obj));
            }
            return false;
        }
        return WriteClass(classPtr, classId);
    }
} // namespace AZ

#endif  // AZCORE_OBJECT_STREAM_H
#pragma once
