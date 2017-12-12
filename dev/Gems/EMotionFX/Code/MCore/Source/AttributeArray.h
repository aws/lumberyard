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
#include "StandardHeaders.h"
#include "Attribute.h"

namespace MCore
{
    /**
     * The array attribute class, which is an array of attribute values.
     */
    class MCORE_API AttributeArray
        : public Attribute
    {
        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x000001aa
        };

        static AttributeArray* Create(uint32 elementAttributeTypeID, uint32 interfaceTypeID = ATTRIBUTE_INTERFACETYPE_DEFAULT);
        static AttributeArray* Create(uint32 elementAttributeTypeID, const Array<Attribute*>& values, uint32 interfaceTypeID = ATTRIBUTE_INTERFACETYPE_DEFAULT, bool cloneThem = true);

        // adjust values
        MCORE_INLINE const Array<Attribute*>& GetAttributes() const                 { return mValues; }
        void SetAttributes(const Array<Attribute*>& values, bool cloneThem = true);

        MCORE_INLINE uint8* GetRawDataPointer()                                     { return reinterpret_cast<uint8*>(&mValues); }
        MCORE_INLINE uint32 GetRawDataSize() const                                  { return sizeof(mValues); }
        bool GetSupportsRawDataPointer() const override                             { return false; }

        // overloaded from the attribute base class
        Attribute* Clone() const override;
        Attribute* CreateInstance(void* destMemory) override                        { return new(destMemory) AttributeArray(); }
        const char* GetTypeString() const override                                  { return "AttributeArray"; }
        uint8 GetStreamWriteVersion() const                                         { return 2; }
        bool InitFrom(const Attribute* other) override;
        bool InitFromString(const String& valueString) override;
        bool ConvertToString(String& outString) const override;
        uint32 GetClassSize() const override                                        { return sizeof(AttributeArray); }
        uint32 GetDefaultInterfaceType() const override;

        bool GetCanHaveChildren() const override                                    { return true; }
        uint32 GetNumChildAttributes() const override                               { return mValues.GetLength(); }
        Attribute* GetChildAttribute(uint32 index) const override                   { return mValues[index]; }
        AttributeSettings* GetChildAttributeSettings(uint32 index) const override   { MCORE_UNUSED(index); return mSettings; }

        uint32 FindAttributeIndexByValuePointer(const Attribute* attribute) const override;
        uint32 FindAttributeIndexByNameID(uint32 nameID) const override;
        uint32 FindAttributeIndexByInternalNameID(uint32 nameID) const override;
        uint32 FindAttributeIndexByName(const char* name) const override;
        uint32 FindAttributeIndexByInternalName(const char* name) const override;

        void Clear(bool delFromMem = true, bool lock = true);

        Attribute* AddAttribute();
        void AddAttribute(Attribute* attribute, bool cloneIt = true);
        void RemoveAttribute(uint32 index, bool delFromMem = true);
        void RemoveAttribute(Attribute* attribute, bool delFromMem = true);
        MCORE_INLINE Attribute* GetAttribute(uint32 index) const
        {
            if (mValues.GetLength() > index)
            {
                return mValues[index];
            }
            return nullptr;
        }
        MCORE_INLINE uint32 GetNumAttributes() const                                { return mValues.GetLength(); }

    private:
        MCore::AttributeSettings*   mSettings;
        Array<Attribute*>           mValues;
        uint32                      mElementTypeID;

        AttributeArray();
        ~AttributeArray();

        uint32 GetDataSize() const override;
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override;
        bool WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const override;
    };
}   // namespace MCore
