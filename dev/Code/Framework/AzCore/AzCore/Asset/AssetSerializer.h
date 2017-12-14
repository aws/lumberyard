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

#include <AzCore/Asset/AssetCommon.h>

namespace AZ {
    struct Uuid;

    namespace Data
    {
        template<typename T>
        class Asset;
    }   // namespace Data

    /*
     * Returns the serialization UUID for Asset class
     */
    const Uuid& GetAssetClassId();

    /// Generic IDataSerializer specialization for Asset<T>
    /// This is used internally by the object stream because assets need
    /// special handling during serialization
    class AssetSerializer
        : public SerializeContext::IDataSerializer
    {
    public:
        /// Store the class data into a stream.
        size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian = false) override;

        /// Convert binary data to text
        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override;

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override;

        /// Load the class data from a stream.
        bool Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian /*= false*/) override;

        bool CompareValueData(const void* lhs, const void* rhs) override;

        // Even though Asset<T> is a template class, we don't actually care about its underlying asset type
        // during serialization, so all types will share the same instance of the serializer.
        static AssetSerializer  s_serializer;
    };

    /*
     * Generic serialization descriptor for all Assets of all types.
     */
    template<typename T>
    struct SerializeGenericTypeInfo< Data::Asset<T> >
    {
        typedef typename Data::Asset<T> ThisType;

        class Factory
            : public SerializeContext::IObjectFactory
        {
        public:
            void* Create(const char* name) override
            {
                (void)name;
                AZ_Assert(false, "Asset<T> %s should be stored by value!", name);
                return nullptr;
            }

            void Destroy(void*) override
            {
                // do nothing
            }
        };

        class GenericClassGenericAsset
            : public GenericClassInfo
        {
        public:
            GenericClassGenericAsset()
            {
                m_classData = SerializeContext::ClassData::Create<ThisType>("Asset", GetAssetClassId(), &m_factory, &AssetSerializer::s_serializer);
                m_classData.m_version = 1;
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<T>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return GetAssetClassId();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return GetAssetClassId();
            }

            void Reflect(SerializeContext*)
            {
            }

            static GenericClassGenericAsset* Instance()
            {
                static GenericClassGenericAsset s_instance;
                return &s_instance;
            }

            Factory m_factory;
            SerializeContext::ClassData m_classData;
        };

        static GenericClassInfo* GetGenericInfo()
        {
            return GenericClassGenericAsset::Instance();
        }

        static const Uuid& GetClassTypeId()
        {
            return GenericClassGenericAsset::Instance()->m_classData.m_typeId;
        }
    };
}   // namespace AZ