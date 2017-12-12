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
#include "Matrix4.h"


namespace MCore
{
    /**
     * The Matrix attribute class.
     * This attribute represents one Matrix.
     */
    class MCORE_API AttributeMatrix
        : public Attribute
    {
        friend class AttributeFactory;
    public:
        enum
        {
            TYPE_ID = 0x00000009
        };

        static AttributeMatrix* Create();
        static AttributeMatrix* Create(const Matrix& value);

        // adjust values
        MCORE_INLINE const Matrix& GetValue() const                 { return mValue; }
        MCORE_INLINE void SetValue(const Matrix& value)             { mValue = value; }

        MCORE_INLINE uint8* GetRawDataPointer()                     { return reinterpret_cast<uint8*>(&mValue); }
        MCORE_INLINE uint32 GetRawDataSize() const                  { return sizeof(Matrix); }
        bool GetSupportsRawDataPointer() const override             { return true; }

        // overloaded from the attribute base class
        Attribute* Clone() const override                           { return AttributeMatrix::Create(mValue); }
        Attribute* CreateInstance(void* destMemory) override        { return new(destMemory) AttributeMatrix(); }
        const char* GetTypeString() const override                  { return "AttributeMatrix"; }
        bool InitFrom(const Attribute* other) override
        {
            if (other->GetType() != TYPE_ID)
            {
                return false;
            }
            mValue = static_cast<const AttributeMatrix*>(other)->GetValue();
            return true;
        }
        bool InitFromString(const String& valueString) override
        {
            if (valueString.CheckIfIsValidMatrix() == false)
            {
                return false;
            }
            mValue = valueString.ToMatrix();
            return true;
        }
        bool ConvertToString(String& outString) const override      { outString.FromMatrix(mValue); return true; }
        uint32 GetClassSize() const override                        { return sizeof(AttributeMatrix); }
        uint32 GetDefaultInterfaceType() const override             { return ATTRIBUTE_INTERFACETYPE_DEFAULT; }

    private:
        Matrix  mValue;     /**< The Matrix value. */

        AttributeMatrix()
            : Attribute(TYPE_ID)                    { mValue.Identity(); }
        AttributeMatrix(const Matrix& value)
            : Attribute(TYPE_ID)
            , mValue(value)     { }
        ~AttributeMatrix() {}

        uint32 GetDataSize() const override                         { return sizeof(Matrix); }

        // read from a stream
        bool ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version) override
        {
            MCORE_UNUSED(version);

            // read the value
            Matrix streamValue;
            if (stream->Read(streamValue.m16, sizeof(float) * 16) == 0)
            {
                return false;
            }

            // convert endian
            Endian::ConvertFloat(streamValue.m16, streamEndianType, 16);
            mValue = streamValue;

            return true;
        }


        // write to a stream
        bool WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const override
        {
            Matrix streamValue = mValue;
            Endian::ConvertFloatTo(streamValue.m16, targetEndianType, 16);
            if (stream->Write(streamValue.m16, sizeof(float) * 16) == 0)
            {
                return false;
            }

            return true;
        }
    };
}   // namespace MCore
