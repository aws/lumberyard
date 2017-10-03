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
#include "Vector.h"


namespace MCore
{
    /**
     * The Vector3 attribute class.
     * This attribute represents one Vector3.
     */
    class MCORE_API AttributeVector3
        : public Attribute
    {
        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x00000006
        };

        static AttributeVector3* Create();
        static AttributeVector3* Create(const Vector3& value);
        static AttributeVector3* Create(float x, float y, float z);

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(&mValue); }
        MCORE_INLINE uint32 GetRawDataSize() const                  { return sizeof(Vector3); }
        bool GetSupportsRawDataPointer() const override             { return true; }

        // adjust values
        MCORE_INLINE const Vector3& GetValue() const                { return mValue; }
        MCORE_INLINE void SetValue(const Vector3& value)            { mValue = value; }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeVector3::Create(mValue); }
        Attribute* CreateInstance(void* destMemory) override        { return new(destMemory) AttributeVector3(); }
        const char* GetTypeString() const override                  { return "AttributeVector3"; }
        bool InitFrom(const Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            mValue = static_cast<const AttributeVector3*>(other)->GetValue();
            return true;
        }
        bool InitFromString(const String& valueString) override
        {
            if (valueString.CheckIfIsValidVector3() == false)
            {
                return false;
            }
            mValue = valueString.ToVector3();
            return true;
        }
        bool ConvertToString(String& outString) const override      { outString.FromVector3(mValue); return true; }
        uint32 GetClassSize() const override                        { return sizeof(AttributeVector3); }
        uint32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_VECTOR3; }
        void Scale(float scaleFactor) override                      { mValue *= scaleFactor; }


    private:
        Vector3     mValue;     /**< The Vector3 value. */

        AttributeVector3()
            : Attribute(TYPE_ID)                    { mValue.Set(0.0f, 0.0f, 0.0f); }
        AttributeVector3(const Vector3& value)
            : Attribute(TYPE_ID)
            , mValue(value)     { }
        ~AttributeVector3() { }

        uint32 GetDataSize() const override                         { return sizeof(Vector3); }

        // read from a stream
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override
        {
            MCORE_UNUSED(version);

            // read the value
            Vector3 streamValue;
            if (stream->Read(&streamValue, sizeof(Vector3)) == 0)
            {
                return false;
            }

            // convert endian
            Endian::ConvertVector3(&streamValue, streamEndianType);
            mValue = streamValue;

            return true;
        }

        // write to a stream
        bool WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const override
        {
            Vector3 streamValue = mValue;
            Endian::ConvertVector3To(&streamValue, targetEndianType);
            if (stream->Write(&streamValue, sizeof(Vector3)) == 0)
            {
                return false;
            }

            return true;
        }
    };
}   // namespace MCore
