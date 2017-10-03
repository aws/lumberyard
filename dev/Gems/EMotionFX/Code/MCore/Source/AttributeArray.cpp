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

// include required headers
#include "AttributeArray.h"
#include "AttributePool.h"
#include "AttributeFactory.h"
#include "AttributeSettings.h"
#include "CommandLine.h"

namespace MCore
{
    // default constructor
    AttributeArray::AttributeArray()
        : Attribute(TYPE_ID)
    {
        mSettings       = nullptr;
        mElementTypeID  = MCORE_INVALIDINDEX32;
    }


    // destructor
    AttributeArray::~AttributeArray()
    {
        Clear(true, false);
        mSettings->Destroy();
    }


    // create a clone
    Attribute* AttributeArray::Clone() const
    {
        Attribute* clone = AttributeArray::Create(mElementTypeID, mValues);
        AttributeArray* arrayClone = static_cast<AttributeArray*>(clone);
        arrayClone->InitFrom(this);
        return arrayClone;
    }


    // static extended create
    AttributeArray* AttributeArray::Create(uint32 elementAttributeTypeID, const Array<Attribute*>& values, uint32 interfaceTypeID, bool cloneThem)
    {
        AttributeArray* result = static_cast<AttributeArray*>(GetAttributePool().RequestNew(AttributeArray::TYPE_ID));
        result->mSettings = AttributeSettings::Create("");
        result->mSettings->SetInterfaceType(interfaceTypeID);
        result->mSettings->SetParent(result);
        result->mElementTypeID = elementAttributeTypeID;
        result->SetAttributes(values, cloneThem);
        return result;
    }


    // create the array
    AttributeArray* AttributeArray::Create(uint32 elementAttributeTypeID, uint32 interfaceTypeID)
    {
        AttributeArray* result = static_cast<AttributeArray*>(GetAttributePool().RequestNew(AttributeArray::TYPE_ID));
        result->mSettings = AttributeSettings::Create("");
        result->mSettings->SetInterfaceType(interfaceTypeID);
        result->mSettings->SetParent(result);
        result->mElementTypeID = elementAttributeTypeID;
        return result;
    }


    // set the array at once
    void AttributeArray::SetAttributes(const Array<Attribute*>& values, bool cloneThem)
    {
        // if we're trying to copy the same thing do nothing
        if (values.GetPtr() == mValues.GetPtr())
        {
            return;
        }

        // clear existing contents
        Clear();

        // auto set the element ID if it hasn't yet been set
        if (mElementTypeID == MCORE_INVALIDINDEX32 && values.GetLength() > 0)
        {
            mElementTypeID = values[0]->GetType();
        }

        if (mSettings->GetInterfaceType() == ATTRIBUTE_INTERFACETYPE_DEFAULT && values.GetLength() > 0)
        {
            mSettings->SetInterfaceType(values[0]->GetDefaultInterfaceType());
        }

        // clone the values
        if (cloneThem)
        {
            const uint32 numValues = values.GetLength();
            mValues.Resize(numValues);
            for (uint32 i = 0; i < numValues; ++i)
            {
                MCORE_ASSERT(values[i]->GetType() == mElementTypeID);
                mValues[i] = values[i]->Clone();
                mValues[i]->SetParent(this);
            }
        }
        else
        {
            mValues = values;
            UpdateChildParentPointers(false);
        }
        /*
            //--------------------------
            LogError("**********************************************");
            String temp;
            if (ConvertToString(temp) == false)
                LogError("FAILED to convert to string");

            LogError( temp );

            if (InitFromString(temp) == false)
                LogError("FAILED to init from string");

            LogError("**********************************************");
            //--------------------------
        */
    }


    // clear the array
    void AttributeArray::Clear(bool delFromMem, bool lock)
    {
        const uint32 numValues = mValues.GetLength();
        for (uint32 i = 0; i < numValues; ++i)
        {
            mValues[i]->SetParent(nullptr);
            if (delFromMem)
            {
                mValues[i]->Destroy(lock);
            }
        }

        mValues.Clear();
    }


    // initialize from another attribute
    bool AttributeArray::InitFrom(const Attribute* other)
    {
        if (other == this)
        {
            return true;
        }

        if (other->GetType() != TYPE_ID)
        {
            return false;
        }

        // clone the values
        const AttributeArray* otherArray = static_cast<const AttributeArray*>(other);

        // clear the min and max etc
        mSettings->SetDefaultValue(nullptr, true);
        mSettings->SetMinValue(nullptr, true);
        mSettings->SetMaxValue(nullptr, true);

        if (otherArray->mSettings)
        {
            mSettings->InitFrom(otherArray->mSettings);
        }

        mElementTypeID = otherArray->mElementTypeID;

        SetAttributes(otherArray->GetAttributes());

        return true;
    }


    // init from a string
    bool AttributeArray::InitFromString(const String& valueString)
    {
        // FORMAT: -settings { settingsString } -type typeIntegerID -numElements numberInt -0 { value } -1 { value }...
        Clear();

        MCore::String tempString;
        tempString.Reserve(256);
        CommandLine commandLine(valueString);

        if (mSettings == nullptr)
        {
            mSettings = AttributeSettings::Create("");
            mSettings->SetParent(this);
        }

        // get rid of old data
        mSettings->SetDefaultValue(nullptr);
        mSettings->SetMinValue(nullptr);
        mSettings->SetMaxValue(nullptr);

        // init the settings
        commandLine.GetValue("settings", "", &tempString);
        if (tempString.GetLength() > 0)
        {
            if (mSettings->InitFromString(tempString) == false)
            {
                LogError("MCore::AttributeArray::InitFromString() - Failed to init settings from a string.");
                Clear();
                return false;
            }
        }

        // read the element type ID
        mElementTypeID = static_cast<uint32>(commandLine.GetValueAsInt("type", -1));

        // read the number of attributes
        const uint32 numAttributes = static_cast<uint32>(commandLine.GetValueAsInt("numElements", 0));

        // read the attribute values
        MCore::String indexString;
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            indexString.FromInt(i);
            commandLine.GetValue(indexString, "", &tempString);

            // add the attribute and try to init it
            MCore::Attribute* attribute = AddAttribute();
            if (attribute->InitFromString(tempString) == false)
            {
                LogError("MCore::AttributeArray::InitFromString() - Failed to init attribute index %d from its value string (attribType=%s).", i, attribute->GetTypeString());
                Clear();
                return false;
            }
        }

        return true;
    }


    // convert into a string
    bool AttributeArray::ConvertToString(String& outString) const//     { outString.FromVector4( Vector4(mValue.r, mValue.g, mValue.b, mValue.a) ); return true; }
    {
        // FORMAT: -settings { settingsString } -type typeIntegerID -numElements numberInt -0 { value } -1 { value }...

        String tempString;
        tempString.Reserve(256);
        outString.Clear(true);
        outString.Reserve(1024);

        // add the settings
        outString += "-settings ";
        outString += "{";
        if (mSettings->ConvertToString(tempString) == false)
        {
            LogError("MCore::AttributeArray::InitFromString() - Failed to convert the settings for to a string.");
            outString.Clear();
            return false;
        }
        outString += tempString;
        outString += "} ";

        // add the attribute type
        outString += "-type ";
        outString += MCore::String((uint32)mElementTypeID);
        outString += " ";

        outString += "-numElements ";
        outString += MCore::String((uint32)mValues.GetLength());
        outString += " ";

        // for all attributes
        const uint32 numAttribs = mValues.GetLength();
        for (uint32 a = 0; a < numAttribs; ++a)
        {
            // get the parameter value
            if (mValues[a]->ConvertToString(tempString) == false)
            {
                LogError("MCore::AttributeArray::InitFromString() - Failed to convert the value for attribute with index %d (type=%s) into a string (probably dont know how to handle the data type).", a, mValues[a]->GetTypeString());
                outString.Clear();
                return false;
            }
            else
            {
                outString += (a == 0) ? "" : " ";
                outString += "-";
                outString += String((uint32)a);
                outString += " {";
                outString += tempString.AsChar();
                outString += "}";
            }
        }

        return true;
    }


    // get the data size
    uint32 AttributeArray::GetDataSize() const
    {
        uint32 result = 0;

        result += sizeof(uint32); // element type ID
        result += mSettings->CalcStreamSize();
        result += sizeof(uint32); // number of values

        const uint32 numValues = mValues.GetLength();
        for (uint32 i = 0; i < numValues; ++i)
        {
            result += mValues[i]->GetStreamSize(); // use the stream size as we also write the version for each attribute value
        }
        return result;
    }


    // read from a stream
    bool AttributeArray::ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version)
    {
        if (version == 1) // num values + full attributes (types+values etc)
        {
            // read the number of values
            uint32 numValues;
            if (stream->Read(&numValues, sizeof(uint32)) == 0)
            {
                Clear();
                return false;
            }
            Endian::ConvertUnsignedInt32(&numValues, streamEndianType);

            // reserve space
            mValues.Resize(numValues);
            for (uint32 i = 0; i < numValues; ++i)
            {
                mValues[i] = nullptr;
            }

            // read each value and try to create the attribute
            for (uint32 i = 0; i < numValues; ++i)
            {
                if (ReadFullAttribute(stream, streamEndianType, &mValues[i]) == false)
                {
                    MCore::LogError("AttributeArray::ReadData() - Failed to read attribute #%d.", i);
                    Clear();
                    return false;
                }
            }
        }
        else
        if (version == 2) // typeid + settings + numvalues + attribute data values (no per attribute type)
        {
            // read the attribute type id
            uint32 attribType;
            if (stream->Read(&attribType, sizeof(uint32)) == 0)
            {
                MCore::LogError("AttributeArray::ReadData() - Failed to read the attribute type ID.");
                Clear();
                return false;
            }
            Endian::ConvertUnsignedInt32(&attribType, streamEndianType);
            mElementTypeID = attribType;

            // read the settings
            if (mSettings->Read(stream, streamEndianType) == false)
            {
                MCore::LogError("AttributeArray::ReadData() - Failed to read settings.");
                Clear();
                return false;
            }

            // read the number of attributes
            uint32 numValues;
            if (stream->Read(&numValues, sizeof(uint32)) == 0)
            {
                MCore::LogError("AttributeArray::ReadData() - Failed to read the number of values for array attribute with element type ID %d.", attribType);
                Clear();
                return false;
            }
            Endian::ConvertUnsignedInt32(&numValues, streamEndianType);

            // read the values
            for (uint32 i = 0; i < numValues; ++i)
            {
                MCore::Attribute* newAttribute = AddAttribute();
                if (newAttribute->Read(stream, streamEndianType) == false)
                {
                    MCore::LogError("AttributeArray::ReadData() - Failed to read the attribute value for array attribute (index=%d) with element type ID %d.", i, attribType);
                    Clear();
                    return false;
                }
            }
        }

        return true;
    }


    // write to a stream
    bool AttributeArray::WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const
    {
        // write the element type ID (attribute type ID)
        uint32 fileAttributeType = mElementTypeID;
        Endian::ConvertUnsignedInt32To(&fileAttributeType, targetEndianType);
        if (stream->Write(&fileAttributeType, sizeof(uint32)) == 0)
        {
            LogError("AttributeArray::WriteData() - Failed to write element attribute ID.");
            return false;
        }

        // write the settings
        if (mSettings->Write(stream, targetEndianType) == false)
        {
            LogError("AttributeArray::WriteData() - Failed to write settings data.");
            return false;
        }

        // write the number of attributes
        uint32 fileNumValues = mValues.GetLength();
        Endian::ConvertUnsignedInt32To(&fileNumValues, targetEndianType);
        if (stream->Write(&fileNumValues, sizeof(uint32)) == 0)
        {
            LogError("AttributeArray::WriteData() - Failed to write the number of values.");
            return false;
        }

        // write all values
        const uint32 numValues = mValues.GetLength();
        for (uint32 i = 0; i < numValues; ++i)
        {
            if (mValues[i]->Write(stream, targetEndianType) == false)
            {
                LogError("AttributeArray::WriteData() - Failed to write attribute #%d of type '%s'", i, mValues[i]->GetTypeString());
                return false;
            }
        }

        return true;
    }


    // add an attribute
    void AttributeArray::AddAttribute(Attribute* attribute, bool cloneIt)
    {
        if (cloneIt == false)
        {
            attribute->SetParent(this);
            mValues.Add(attribute);
        }
        else
        {
            MCore::Attribute* clone = attribute->Clone();
            clone->SetParent(this);
            mValues.Add(clone);
        }
    }


    // remove an attribute
    void AttributeArray::RemoveAttribute(uint32 index, bool delFromMem)
    {
        mValues[index]->SetParent(nullptr);
        if (delFromMem)
        {
            mValues[index]->Destroy();
        }

        mValues.Remove(index);
    }


    // remove by pointer
    void AttributeArray::RemoveAttribute(Attribute* attribute, bool delFromMem)
    {
        attribute->SetParent(nullptr);

        if (delFromMem)
        {
            attribute->Destroy();
        }

        mValues.RemoveByValue(attribute);
    }


    // return the default interface type
    uint32 AttributeArray::GetDefaultInterfaceType() const
    {
        return ATTRIBUTE_INTERFACETYPE_DEFAULT; // TODO: change this
    }


    // add a new attribute
    Attribute* AttributeArray::AddAttribute()
    {
        MCORE_ASSERT(mElementTypeID != MCORE_INVALIDINDEX32);
        MCore::Attribute* newAttrib = GetAttributeFactory().CreateAttributeByType(mElementTypeID);
        MCORE_ASSERT(newAttrib);
        newAttrib->SetParent(this);
        mValues.Add(newAttrib);
        return newAttrib;
    }


    // find by name
    uint32 AttributeArray::FindAttributeIndexByName(const char* name) const
    {
        MCORE_UNUSED(name);
        return MCORE_INVALIDINDEX32;
    }


    // find by internal name
    uint32 AttributeArray::FindAttributeIndexByInternalName(const char* name) const
    {
        MCORE_UNUSED(name);
        return MCORE_INVALIDINDEX32;
    }


    // find by value pointer
    uint32 AttributeArray::FindAttributeIndexByValuePointer(const Attribute* attribute) const
    {
        const uint32 numChildren = mValues.GetLength();
        for (uint32 i = 0; i < numChildren; ++i)
        {
            if (mValues[i] == attribute)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by name id
    uint32 AttributeArray::FindAttributeIndexByNameID(uint32 nameID) const
    {
        MCORE_UNUSED(nameID);
        return MCORE_INVALIDINDEX32;
    }


    // find by internal name id
    uint32 AttributeArray::FindAttributeIndexByInternalNameID(uint32 nameID) const
    {
        MCORE_UNUSED(nameID);
        return MCORE_INVALIDINDEX32;
    }
}   // namespace MCore
