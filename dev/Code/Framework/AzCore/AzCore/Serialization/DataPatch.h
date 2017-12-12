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
#ifndef AZCORE_DATA_PATCH_FIELD_H
#define AZCORE_DATA_PATCH_FIELD_H

#include <AzCore/Asset/AssetCommon.h>
#include "ObjectStream.h"

namespace AZ
{
    struct Uuid;
    class ReflectContext;

    /**
    * Structure that contains patch data for a given class. The primary goal of this
    * object is to help with tools (slices and undo/redo), this structure is not recommended to
    * runtime due to efficiency. If you want dynamically configure objects, use data overlay which are more efficient and generic.
    */
    class DataPatch
    {
    public:
        AZ_CLASS_ALLOCATOR(DataPatch, SystemAllocator, 0)
        AZ_TYPE_INFO(DataPatch, "{3A8D5EC9-D70E-41CB-879C-DEF6A6D6ED03}")

        // Currently our address is an array of 64 bit values
        struct AddressType
            : public AZStd::vector<u64>
        {
            AZ_TYPE_INFO(AddressType, "{90752F2D-CBD3-4EE9-9CDD-447E797C8408}")
            /// used for hashing
            operator AZStd::size_t() const
            {
                return AZStd::hash_range(begin(), end());
            }
        };

        using PatchMap = AZStd::unordered_map<AddressType, AZStd::vector<AZ::u8>>;

        /**
         * Data addresses can be tagged with flags to affect how patches are created and applied.
         */
        using Flags = AZ::u8;

        /**
         * Bit fields for data flags.
         * These options affect how patches are created and applied.
         */
        enum Flag : Flags
        {
            ForceOverride = 1 << 0, ///< The data is always overridden. The target will never inherit this data from its source.
        };

        /**
         * Data flags, mapped by the address where the flags are set.
         * When patching, an address is affected not only by the flags set AT that address,
         * but also the flags set at any parent address.
         */
        using FlagsMap = AZStd::unordered_map<DataPatch::AddressType, Flags>;

        DataPatch();
        DataPatch(const DataPatch& rhs);
#ifdef AZ_HAS_RVALUE_REFS
        DataPatch(DataPatch&& rhs);
        DataPatch& operator=(DataPatch&& rhs);
#endif // AZ_HAS_RVALUE_REF
        DataPatch& operator=(const DataPatch& rhs);

        /**
         * Create a a patch structure which generate the delta (patcher) from source to the target type.
         *
         * \param source object we will use a base for the delta
         * \param souceClassId class of the source type, either the same as targetClassId or a base class
         * \param target object we want to have is we have source and apply the returned patcher.
         * \param targetClassId class of the target type, either the same as sourceClassId or a base class
         * \param flagsMap data flags, mapped by address, which may affect how the patch is created.
         * \param context if null we will grab the default serialize context.
         */
        bool Create(const void* source, const Uuid& souceClassId, const void* target, const Uuid& targetClassId, const FlagsMap& flagsMap = FlagsMap(), SerializeContext* context = nullptr);

        /// T and U should either be the same type a common base class
        template<class T, class U>
        bool Create(const T* source, const U* target, const FlagsMap& flagsMap = FlagsMap(), SerializeContext* context = nullptr)
        {
            const void* sourceClassPtr = SerializeTypeInfo<T>::RttiCast(source, SerializeTypeInfo<T>::GetRttiTypeId(source));
            const Uuid& sourceClassId = SerializeTypeInfo<T>::GetUuid(source);
            const void* targetClassPtr = SerializeTypeInfo<U>::RttiCast(target, SerializeTypeInfo<U>::GetRttiTypeId(target));
            const Uuid& targetClassId = SerializeTypeInfo<U>::GetUuid(target);
            return Create(sourceClassPtr, sourceClassId, targetClassPtr, targetClassId, flagsMap, context);
        }

        /**
         * Apply the patch to a source instance and generate a patched instance, from a source instance.
         * If patch can't be applied a null pointer is returned. Currently the only reason for that is if
         * the resulting class ID doesn't match the stored root of the patch.
         *
         * \param source pointer to the source instance.
         * \param sourceClassID id of the class \ref source is pointing to.
         * \param context if null we will grab the default serialize context.
         */
        void* Apply(const void* source, const Uuid& sourceClassId, SerializeContext* context = nullptr, const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor());

        // Apply specialization when the source and return type are the same.
        template<class T>
        T* Apply(const T* source, SerializeContext* context = nullptr, const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor())
        {
            const void* classPtr = SerializeTypeInfo<T>::RttiCast(source, SerializeTypeInfo<T>::GetRttiTypeId(source));
            const Uuid& classId = SerializeTypeInfo<T>::GetUuid(source);
            if (m_targetClassId == classId)
            {
                return reinterpret_cast<T*>(Apply(classPtr, classId, context, filterDesc));
            }
            else
            {
                return nullptr;
            }
        }

        template<class U, class T>
        U* Apply(const T* source, SerializeContext* context = nullptr, const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor())
        {
            // \note find class Data and check if we can do that cast ie. U* is a base class of m_targetClassID
            if (SerializeTypeInfo<U>::GetUuid() == m_targetClassId)
            {
                return reinterpret_cast<U*>(Apply(source, SerializeTypeInfo<T>::GetUuid(), context, filterDesc));
            }
            else
            {
                return nullptr;
            }
        }

        /**
         * Apply a patch on the top of the patch. Keep in mind that applying patches is order dependent.
         * \param patch patch to apply on the top of this.
         * \returns true of patch was applied or false, if this patch can't be applied (currently due to a different target class)
         */
        bool Apply(const DataPatch& patch);
        bool Apply(const DataPatch* patch)
        {
            if (patch)
            {
                return Apply(*patch);
            }

            return false;
        }

        /// \returns true if this is a valid patch.
        bool IsValid() const
        {
            return m_targetClassId != Uuid::CreateNull();
        }

        /// \returns true of the patch actually contains data for patching otherwise false.
        bool IsData() const
        {
            return !m_patch.empty();
        }

        /**
         * Reflect a patch for serialization.
         */
        static void Reflect(ReflectContext* context);

    protected:
        Uuid     m_targetClassId;
        PatchMap m_patch;
    };
}   // namespace AZ

#endif