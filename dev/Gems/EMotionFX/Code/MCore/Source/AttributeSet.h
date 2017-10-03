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

// include the required headers
#include <AzCore/Math/Vector2.h>
#include "StandardHeaders.h"
#include "MemoryManager.h"
#include "UnicodeString.h"
#include "Vector.h"
#include "Quaternion.h"
#include "Matrix4.h"
#include "Color.h"
#include "Endian.h"
#include "Attribute.h"


namespace MCore
{
    // forward declarations
    class Attribute;
    class AttributeSettings;
    class Stream;


    class MCORE_API AttributeSet
        : public Attribute
    {
        MCORE_MEMORYOBJECTCATEGORY(AttributeSet, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_ATTRIBUTES);

        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x00000111
        };

        static AttributeSet* Create();

        //------
        // overloaded from Attribute
        Attribute* Clone() const override;
        Attribute* CreateInstance(void* destMemory) override;
        const char* GetTypeString() const override;
        bool InitFromString(const String& valueString) override;
        bool ConvertToString(String& outString) const override;
        bool InitFrom(const Attribute* other) override;
        uint32 GetClassSize() const override;
        uint32 GetDataSize() const override;
        bool GetSupportsRawDataPointer() const;
        uint32 GetDefaultInterfaceType() const override                             { return ATTRIBUTE_INTERFACETYPE_PROPERTYSET; }

        bool GetCanHaveChildren() const override                                    { return true; }
        uint32 GetNumChildAttributes() const override                               { return mAttributes.GetLength(); }
        Attribute* GetChildAttribute(uint32 index) const override                   { MCORE_ASSERT(index < mAttributes.GetLength()); return mAttributes[index].mValue; }
        AttributeSettings* GetChildAttributeSettings(uint32 index) const override   { MCORE_ASSERT(index < mAttributes.GetLength()); return mAttributes[index].mSettings; }

        uint32 FindAttributeIndexByValuePointer(const Attribute* attribute) const override;
        uint32 FindAttributeIndexByNameID(uint32 nameID) const override;
        uint32 FindAttributeIndexByInternalNameID(uint32 nameID) const override;
        uint32 FindAttributeIndexByName(const char* name) const override;
        uint32 FindAttributeIndexByInternalName(const char* name) const override;

        //------

        MCORE_INLINE uint32 GetNumAttributes() const                                                    { return mAttributes.GetLength(); }
        MCORE_INLINE AttributeSettings* GetAttributeSettings(uint32 index) const                        { return mAttributes[index].mSettings; }
        MCORE_INLINE Attribute* GetAttributeValue(uint32 index) const                                   { return mAttributes[index].mValue; }
        MCORE_INLINE void SetAttributeSettings(uint32 index, AttributeSettings* settings)               { mAttributes[index].mSettings = settings; }
        MCORE_INLINE void SetAttributeValue(uint32 index, Attribute* value, bool delCurrent = true)
        {
            if (mAttributes[index].mValue != value && mAttributes[index].mValue && delCurrent)
            {
                mAttributes[index].mValue->Destroy();
            }
            mAttributes[index].mValue = value;
        }

        void AddAttribute(AttributeSettings* settings, Attribute* attributeValue);
        void AddAttribute(AttributeSettings* settings);
        void InsertAttribute(uint32 index, AttributeSettings* settings);
        void InsertAttribute(uint32 index, AttributeSettings* settings, Attribute* attributeValue);

        void RemoveAttributeByIndex(uint32 index, bool delFromMem = true);
        bool RemoveAttributeByInternalName(const char* name, bool delFromMem = true);
        bool RemoveAttributeByInternalNameID(uint32 id, bool delFromMem = true);
        bool RemoveAttributeByName(const char* name, bool delFromMem = true);
        bool RemoveAttributeByNameID(uint32 id, bool delFromMem = true);
        void RemoveAllAttributes(bool delFromMem = true);
        void Reserve(uint32 numAttributes);
        void Resize(uint32 numAttributes);
        void Swap(uint32 firstIndex, uint32 secondIndex);

        bool CheckIfHasAttributeWithInternalName(const char* name) const;
        bool CheckIfHasAttributeWithInternalNameID(uint32 nameID) const;
        bool CheckIfHasAttributeWithName(const char* name) const;
        bool CheckIfHasAttributeWithNameID(uint32 nameID) const;

        // set helpers
        bool SetStringAttribute(const char* internalName, const char* value, bool createIfNotExists = true);
        bool SetFloatAttribute(const char* internalName, float value, bool createIfNotExists = true);
        bool SetInt32Attribute(const char* internalName, int32 value, bool createIfNotExists = true);
        bool SetColorAttribute(const char* internalName, const RGBAColor& value, bool createIfNotExists = true);
        bool SetVector2Attribute(const char* internalName, const AZ::Vector2& value, bool createIfNotExists = true);
        bool SetVector3Attribute(const char* internalName, const Vector3& value, bool createIfNotExists = true);
        bool SetVector4Attribute(const char* internalName, const AZ::Vector4& value, bool createIfNotExists = true);
        bool SetMatrixAttribute(const char* internalName, const Matrix& value, bool createIfNotExists = true);
        bool SetQuaternionAttribute(const char* internalName, const Quaternion& value, bool createIfNotExists = true);
        bool SetBoolAttribute(const char* internalName, bool value, bool createIfNotExists = true);

        // get helpers
        const char* GetStringAttribute(const char* internalName, const char* defaultValue = "");
        float GetFloatAttribute(const char* internalName, float defaultValue = 0.0f);
        int32 GetInt32Attribute(const char* internalName, int32 defaultValue = 0);
        bool GetBoolAttribute(const char* internalName, bool defaultValue = false);
        AZ::Vector2 GetVector2Attribute(const char* internalName, const AZ::Vector2& defaultValue = AZ::Vector2(0.0f, 0.0f));
        Vector3 GetVector3Attribute(const char* internalName, const Vector3& defaultValue = Vector3(0.0f, 0.0f, 0.0f));
        AZ::Vector4 GetVector4Attribute(const char* internalName, const AZ::Vector4& defaultValue = AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f));
        Quaternion GetQuaternionAttribute(const char* internalName, const Quaternion& defaultValue = Quaternion());
        Matrix GetMatrixAttribute(const char* internalName, const Matrix& defaultValue = Matrix());   // TODO: the matrix default value currently is an uninitialized matrix!
        RGBAColor GetColorAttribute(const char* internalName, const RGBAColor& defaultValue = RGBAColor());

        void Merge(const AttributeSet& other, bool overwriteExisting = false, bool overwriteOnlyWhenSameType = true);
        void CopyFrom(const AttributeSet& other);
        void Log();

    private:
        struct MCORE_API AttributeData
        {
            AttributeSettings*  mSettings;
            Attribute*          mValue;

            AttributeData()
                : mSettings(nullptr)
                , mValue(nullptr) {}
            ~AttributeData() {}
        };

        Array<AttributeData>    mAttributes;

        AttributeSet();
        ~AttributeSet();

        bool WriteSet(Stream* stream, Endian::EEndianType targetEndianType) const;
        bool ReadSet(Stream* stream, Endian::EEndianType sourceEndian);

        // overloaded from Attribute base class
        bool WriteData(Stream* stream, Endian::EEndianType targetEndianType) const override;
        bool ReadData(Stream* stream, Endian::EEndianType streamEndianType, uint8 version) override;
    };
}   // namespace MCore
