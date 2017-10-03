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

// include the required headers
#include "StandardHeaders.h"
#include "AttributeSet.h"
#include "Attribute.h"
#include "AttributeSettings.h"
#include "Stream.h"
#include "StringIDGenerator.h"
#include "LogManager.h"
#include "UnicodeString.h"
#include "AttributeBool.h"
#include "AttributeFloat.h"
#include "AttributeString.h"
#include "AttributeInt32.h"
#include "AttributeQuaternion.h"
#include "AttributeMatrix.h"
#include "AttributeColor.h"
#include "AttributeVector2.h"
#include "AttributeVector3.h"
#include "AttributeVector4.h"
#include "AttributePool.h"
#include "AttributeFactory.h"
#include "MCoreSystem.h"
#include "CommandLine.h"
#include "AttributePool.h"

namespace MCore
{
    // a Set....Attribute implementation macro
#define MCORE_ATTRIBUTESET_DECLARE_SET(AttributeType)                                                                                \
    const uint32 index = FindAttributeIndexByInternalName(internalName);                                                             \
    if (index == MCORE_INVALIDINDEX32)                                                                                               \
    {                                                                                                                                \
        if (createIfNotExists)                                                                                               \
        {                                                                                                                            \
            Attribute* attribute = GetMCore().GetAttributePool().RequestNew(AttributeType::TYPE_ID);                                    \
            static_cast<AttributeType*>(attribute)->SetValue(value);                                                                 \
            AttributeSettings* settings = AttributeSettings::Create(internalName);                                                   \
            settings->SetParent(this);                                                                                               \
            settings->SetDefaultValue(attribute->Clone());                                                                           \
            settings->SetName(internalName);                                                                                         \
            AddAttribute(settings, attribute);                                                                                       \
            return true;                                                                                                             \
        }                                                                                                                            \
        else{                                                                                                                        \
            return false; }                                                                                                          \
    }                                                                                                                                \
                                                                                                                                     \
    Attribute* attribute = mAttributes[index].mValue;                                                                                \
    if (attribute->GetType() == AttributeType::TYPE_ID)                                                                              \
    {                                                                                                                                \
        AttributeType* castAttribute = static_cast<AttributeType*>(attribute);                                                       \
        castAttribute->SetValue(value);                                                                                              \
    }                                                                                                                                \
    else                                                                                                                             \
    {                                                                                                                                \
        LogWarning("MCore::AttributeSet::Set....Attribute() - The existing attribute '%s' is not of the right type.", internalName); \
        return false;                                                                                                                \
    }                                                                                                                                \
    return true;


    // a quick Get....Attribute implementation
#define MCORE_ATTRIBUTESET_DECLARE_GET(AttributeType)                      \
    const uint32 index = FindAttributeIndexByInternalName(internalName);   \
    if (index == MCORE_INVALIDINDEX32) {                                   \
        return defaultValue; }                                             \
                                                                           \
    Attribute* attribute = mAttributes[index].mValue;                      \
    if (attribute->GetType() != AttributeType::TYPE_ID) {                  \
        return defaultValue; }                                             \
                                                                           \
    AttributeType* castAttribute = static_cast<AttributeType*>(attribute); \
    return castAttribute->GetValue();


    //--------------------------------------------------------

    // constructor
    AttributeSet::AttributeSet()
        : Attribute(TYPE_ID)
    {
        mAttributes.SetMemoryCategory(MCORE_MEMCATEGORY_ATTRIBUTES);
    }


    // destructor
    AttributeSet::~AttributeSet()
    {
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            mAttributes[i].mSettings->Destroy(false);
            mAttributes[i].mValue->Destroy(false);
        }
    }


    // create
    AttributeSet* AttributeSet::Create()
    {
        return static_cast<AttributeSet*>(GetAttributePool().RequestNew(AttributeSet::TYPE_ID));
    }


    //-----------------------
    // create a clone
    Attribute* AttributeSet::Clone() const
    {
        AttributeSet* newSet = AttributeSet::Create();
        newSet->CopyFrom(*this);
        return newSet;
    }


    // create an instance at a given location
    Attribute* AttributeSet::CreateInstance(void* destMemory)
    {
        return new(destMemory) AttributeSet();
    }


    // get the type string
    const char* AttributeSet::GetTypeString() const
    {
        return "AttributeSet";
    }


    // init from a string
    bool AttributeSet::InitFromString(const MCore::String& valueString)
    {
        // -numAttributes 10 -index { -settings { settingsString } -type typeString -value { valueString } }

        // remove all current attributes
        RemoveAllAttributes();

        // parse the valueString
        CommandLine rootCommandLine(valueString);

        MCore::String tempString;
        MCore::String tempStringB;
        MCore::String typeString;

        // get the number of attributes
        const uint32 numAttributes = static_cast<uint32>(rootCommandLine.GetValueAsInt("numAttributes", -1));
        if (numAttributes == MCORE_INVALIDINDEX32)
        {
            MCore::LogError("AttributeSet::InitFromString() - The input value string has no numAttributes defined.");
            return false;
        }

        // load all attributes
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            tempStringB.FromInt(i);
            rootCommandLine.GetValue(tempStringB.AsChar(), "", &tempString);

            if (tempString.GetLength() == 0)
            {
                RemoveAllAttributes();
                MCore::LogError("AttributeSet::InitFromString() - Failed to get the string data for attribute index %d.", i);
                return false;
            }

            // get the settings string
            CommandLine attribCommandLine(tempString);
            attribCommandLine.GetValue("settings", "", &tempString);
            if (tempString.GetLength() == 0)
            {
                RemoveAllAttributes();
                MCore::LogError("AttributeSet::InitFromString() - Failed to get the settings string data for attribute index %d.", i);
                return false;
            }

            // create the settings
            MCore::AttributeSettings* newSettings = MCore::AttributeSettings::Create();
            if (newSettings->InitFromString(tempString) == false)
            {
                MCore::LogError("AttributeSet::InitFromString() - Failed to init settings for attribute index %d.", i);
                RemoveAllAttributes();
                newSettings->Destroy();
                return false;
            }

            // check the type
            attribCommandLine.GetValue("type", "", &typeString);
            if (typeString.GetLength() == 0)
            {
                MCore::LogError("AttributeSet::InitFromString() - Failed to get the attribute value type for attribute index %d.", i);
                RemoveAllAttributes();
                newSettings->Destroy();
                return false;
            }

            // get the value
            attribCommandLine.GetValue("value", "", &tempStringB);
            if (tempStringB.GetLength() == 0 && attribCommandLine.CheckIfHasParameter("value") == false)
            {
                MCore::LogError("AttributeSet::InitFromString() - Failed to get the attribute value data for index %d.", i);
                RemoveAllAttributes();
                newSettings->Destroy();
                return false;
            }

            // create the attribute
            Attribute* value = GetAttributeFactory().CreateAttributeByTypeString(typeString.AsChar());
            if (value == nullptr)
            {
                MCore::LogError("AttributeSet::InitFromString() - Failed to create attribute of type '%s' for the value of attribute %s (index=%d).", typeString.AsChar(), newSettings->GetInternalName(), i);
                RemoveAllAttributes();
                newSettings->Destroy();
                return false;
            }

            // init the attribute from the given data string
            if (value->InitFromString(tempStringB) == false)
            {
                MCore::LogError("AttributeSet::InitFromString() - Failed to init the value attribute from its data string, for attribute of type '%s' with name '%s' (index=%d).", typeString.AsChar(), newSettings->GetInternalName(), i);
                RemoveAllAttributes();
                newSettings->Destroy();
                value->Destroy();
                return false;
            }

            // add the attribute
            AddAttribute(newSettings, value);
        }

        return true;
    }


    // convert to a string
    bool AttributeSet::ConvertToString(MCore::String& outString) const
    {
        // -numAttributes 10 -index { -settings { settingsString } -type typeString -value { valueString } }
        outString.Clear();
        outString.Reserve(4096);

        MCore::String tempString;
        const uint32 numAttributes = mAttributes.GetLength();
        outString.Reserve(numAttributes * 128);

        tempString.FromInt(mAttributes.GetLength());
        outString += "-numAttributes";
        outString += " ";
        outString += tempString;
        outString += " ";

        for (uint32 i = 0; i < numAttributes; ++i)
        {
            outString += "-";
            tempString.FromInt(i);
            outString += tempString;

            outString += " {";

            outString += "-settings ";
            outString += " {";

            if (mAttributes[i].mSettings->ConvertToString(tempString) == false)
            {
                MCore::LogError("AttributeSet::ConvertToString() - Failed to convert the attribute settings for attribute '%s' (index %d) to a string.", mAttributes[i].mSettings->GetInternalName(), i);
                outString.Clear();
                return false;
            }

            outString += tempString;
            outString += "} ";

            outString += "-type";
            outString += " ";
            outString += mAttributes[i].mValue->GetTypeString();
            outString += " ";

            if (mAttributes[i].mValue->ConvertToString(tempString) == false)
            {
                MCore::LogError("AttributeSet::ConvertToString() - Failed to convert the attribute value for attribute '%s' (index %d) to a string.", mAttributes[i].mSettings->GetInternalName(), i);
                outString.Clear();
                return false;
            }

            outString += "-value";
            outString += " {";
            outString += tempString;
            outString += "} ";

            outString += "} ";
        }

        return true;
    }


    // init from another attribute
    bool AttributeSet::InitFrom(const Attribute* other)
    {
        MCORE_ASSERT(other->GetType() == AttributeSet::TYPE_ID);
        AttributeSet* otherSet = static_cast<AttributeSet*>(const_cast<MCore::Attribute*>(other));
        CopyFrom(*otherSet);
        return true;
    }


    // get the class size
    uint32 AttributeSet::GetClassSize() const
    {
        return sizeof(AttributeSet);
    }


    // get the data size
    uint32 AttributeSet::GetDataSize() const
    {
        uint32 totalSize = 0;

        totalSize += sizeof(uint32); // numAttributes

        // for all attributes
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            // the settings
            if (mAttributes[i].mSettings)
            {
                totalSize += mAttributes[i].mSettings->CalcStreamSize();
            }

            // the value
            totalSize += Attribute::GetFullAttributeSize(mAttributes[i].mValue);
        }

        return totalSize;
    }


    // write the attribute data
    bool AttributeSet::WriteData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType) const
    {
        return WriteSet(stream, targetEndianType);
    }


    // read the attribute data
    bool AttributeSet::ReadData(MCore::Stream* stream, MCore::Endian::EEndianType streamEndianType, uint8 version)
    {
        MCORE_UNUSED(version);
        const bool result = ReadSet(stream, streamEndianType);

        /*
            //----------------------------
            AttributeSet* copy = MCore::AttributeSet::Create();
            copy->InitFrom( this );

            String tempString;
            if (copy->ConvertToString(tempString))
            {
                //OutputDebugStringA( "-----------------------------------------------------------------------------\n" );
                //OutputDebugStringA( tempString.AsChar() );
                //OutputDebugStringA( "\n" );
                //OutputDebugStringA( "-----------------------------------------------------------------------------\n" );

                Timer t;
                bool initResult = copy->InitFromString( tempString );
                double totalTime = t.GetTime();

                if (initResult == false)
                    MCore::LogInfo("********* FAILED TO INIT FROM STRING");
                else
                    MCore::LogInfo("****** SUCCESSFULLY INITIALIZED IN %f seconds (%.4f ms)", totalTime, totalTime * 1000.0);
            }
            else
                MCore::LogInfo("**************** FAILED TO CONVERT TO STRING");

            InitFrom( copy );

            copy->Destroy();
            //----------------------------
        */

        return result;
    }


    // support getting the raw pointer?
    bool AttributeSet::GetSupportsRawDataPointer() const
    {
        return false;
    }


    //-----------------------


    // add an attribute
    void AttributeSet::AddAttribute(AttributeSettings* settings, Attribute* attributeValue)
    {
        MCORE_ASSERT(CheckIfHasAttributeWithInternalNameID(settings->GetInternalNameID()) == false);
        mAttributes.AddEmpty();
        mAttributes.GetLast().mSettings = settings;
        mAttributes.GetLast().mValue    = attributeValue;
        settings->SetParent(this);
        attributeValue->SetParent(this);
    }


    // add an attribute
    void AttributeSet::AddAttribute(AttributeSettings* settings)
    {
        MCORE_ASSERT(CheckIfHasAttributeWithInternalNameID(settings->GetInternalNameID()) == false);
        mAttributes.AddEmpty();
        mAttributes.GetLast().mSettings = settings;
        mAttributes.GetLast().mValue    = settings->GetDefaultValue()->Clone();
        mAttributes.GetLast().mValue->SetParent(this);
        settings->SetParent(this);
    }


    // remove a given attribute by its index
    void AttributeSet::RemoveAttributeByIndex(uint32 index, bool delFromMem)
    {
        if (delFromMem)
        {
            mAttributes[index].mSettings->SetParent(nullptr);
            mAttributes[index].mSettings->Destroy();
            mAttributes[index].mValue->Destroy();
        }

        mAttributes[index].mValue->SetParent(nullptr);
        mAttributes[index].mSettings->SetParent(nullptr);
        mAttributes.Remove(index);
    }


    // remove a given attribute by its name
    bool AttributeSet::RemoveAttributeByInternalName(const char* name, bool delFromMem)
    {
        const uint32 index = FindAttributeIndexByInternalName(name);
        if (index != MCORE_INVALIDINDEX32)
        {
            RemoveAttributeByIndex(index, delFromMem);
            return true;
        }

        return false;
    }


    // remove a given attribute by its ID
    bool AttributeSet::RemoveAttributeByInternalNameID(uint32 id, bool delFromMem)
    {
        const uint32 index = FindAttributeIndexByInternalNameID(id);
        if (index != MCORE_INVALIDINDEX32)
        {
            RemoveAttributeByIndex(index, delFromMem);
            return true;
        }

        return false;
    }


    // remove a given attribute by its name
    bool AttributeSet::RemoveAttributeByName(const char* name, bool delFromMem)
    {
        const uint32 index = FindAttributeIndexByName(name);
        if (index != MCORE_INVALIDINDEX32)
        {
            RemoveAttributeByIndex(index, delFromMem);
            return true;
        }

        return false;
    }


    // remove a given attribute by its ID
    bool AttributeSet::RemoveAttributeByNameID(uint32 id, bool delFromMem)
    {
        const uint32 index = FindAttributeIndexByNameID(id);
        if (index != MCORE_INVALIDINDEX32)
        {
            RemoveAttributeByIndex(index, delFromMem);
            return true;
        }

        return false;
    }


    // remove all attributes
    void AttributeSet::RemoveAllAttributes(bool delFromMem)
    {
        if (delFromMem)
        {
            const uint32 numAttributes = mAttributes.GetLength();
            for (uint32 i = 0; i < numAttributes; ++i)
            {
                mAttributes[i].mSettings->SetParent(nullptr);
                mAttributes[i].mSettings->Destroy();
                mAttributes[i].mValue->SetParent(nullptr);
                mAttributes[i].mValue->Destroy();
            }
        }
        else
        {
            const uint32 numAttributes = mAttributes.GetLength();
            for (uint32 i = 0; i < numAttributes; ++i)
            {
                mAttributes[i].mValue->SetParent(nullptr);
                mAttributes[i].mSettings->SetParent(nullptr);
            }
        }

        mAttributes.Clear(true);
    }


    // reserve attribute memory
    void AttributeSet::Reserve(uint32 numAttributes)
    {
        mAttributes.Reserve(numAttributes);
    }


    // resize attribute memory
    void AttributeSet::Resize(uint32 numAttributes)
    {
        mAttributes.Resize(numAttributes);
    }


    // swap two attributes
    void AttributeSet::Swap(uint32 firstIndex, uint32 secondIndex)
    {
        mAttributes.Swap(firstIndex, secondIndex);
    }


    // check if there is an attribute with the given name
    bool AttributeSet::CheckIfHasAttributeWithInternalName(const char* name) const
    {
        return (FindAttributeIndexByInternalName(name) != MCORE_INVALIDINDEX32);
    }


    // check if there is an attribute with the given type
    bool AttributeSet::CheckIfHasAttributeWithInternalNameID(uint32 id) const
    {
        return (FindAttributeIndexByInternalNameID(id) != MCORE_INVALIDINDEX32);
    }


    // check if there is an attribute with the given name
    bool AttributeSet::CheckIfHasAttributeWithName(const char* name) const
    {
        return (FindAttributeIndexByName(name) != MCORE_INVALIDINDEX32);
    }


    // check if there is an attribute with the given type
    bool AttributeSet::CheckIfHasAttributeWithNameID(uint32 id) const
    {
        return (FindAttributeIndexByNameID(id) != MCORE_INVALIDINDEX32);
    }


    // merge this attribute set with another
    void AttributeSet::Merge(const AttributeSet& other, bool overwriteExisting, bool overwriteOnlyWhenSameType)
    {
        const uint32 numAttribs = other.GetNumAttributes();
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            const AttributeData& curAttribute = other.mAttributes[i];

            // find the attribute in the current set
            const uint32 index = FindAttributeIndexByInternalNameID(curAttribute.mSettings->GetInternalNameID());

            // if the attribute doesn't exist inside the current set, add it
            if (index == MCORE_INVALIDINDEX32)
            {
                mAttributes.AddEmpty();
                mAttributes.GetLast().mSettings = curAttribute.mSettings->Clone();
                mAttributes.GetLast().mSettings->SetParent(this);
                mAttributes.GetLast().mValue    = curAttribute.mValue->Clone();
                mAttributes.GetLast().mValue->SetParent(this);
                continue;
            }

            // the item already exists
            if (overwriteExisting == false)
            {
                continue;
            }

            // check if we need to overwrite only when the data types are the same
            const bool sameType = (mAttributes[index].mValue->GetType() == curAttribute.mValue->GetType());
            if (overwriteOnlyWhenSameType && sameType == false) // skip overwrite when data types are not equal
            {
                continue;
            }

            // overwrite the data
            mAttributes[index].mSettings->SetParent(this);
            mAttributes[index].mSettings->InitFrom(curAttribute.mSettings);
            mAttributes[index].mValue->SetParent(this);
            mAttributes[index].mValue->InitFrom(curAttribute.mValue);
        }
    }


    // copy over attributes
    void AttributeSet::CopyFrom(const AttributeSet& other)
    {
        // remove all attributes
        RemoveAllAttributes();

        // copy over all attributes
        const uint32 numAttribs = other.GetNumAttributes();
        mAttributes.Reserve(numAttribs);
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            const AttributeData& data = other.mAttributes[i];
            mAttributes.AddEmpty();
            mAttributes.GetLast().mSettings = data.mSettings->Clone();
            mAttributes.GetLast().mValue    = data.mValue->Clone();
            mAttributes.GetLast().mSettings->SetParent(this);
            mAttributes.GetLast().mValue->SetParent(this);
        }
    }


    // log the data inside the set
    void AttributeSet::Log()
    {
        String valueString;
        const uint32 numAttribs = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            // convert the value to a string
            if (mAttributes[i].mValue->ConvertToString(valueString) == false)
            {
                valueString = "<Failed to convert attribute to string>";
            }

            LogDetailedInfo("#%d - internalName='%s' - value=%s (type=%s)", i, GetAttributeSettings(i)->GetInternalName(), valueString.AsChar(), mAttributes[i].mValue->GetTypeString());
        }
    }


    // write the set to a stream
    bool AttributeSet::WriteSet(Stream* stream, Endian::EEndianType targetEndianType) const
    {
        // get the number of attributes in the set
        uint32 numAttributes = mAttributes.GetLength();
        Endian::ConvertUnsignedInt32To(&numAttributes, targetEndianType);
        if (stream->Write(&numAttributes, sizeof(uint32)) == 0)
        {
            return false;
        }

        // write all attributes
        numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            // write the attribute settings
            if (mAttributes[i].mSettings->Write(stream, targetEndianType) == false)
            {
                return false;
            }

            // write the attribute value
            if (Attribute::WriteFullAttribute(stream, targetEndianType, mAttributes[i].mValue) == false)
            {
                return false;
            }
        }

        return true;
    }


    // read the set from a stream (this clear the current contents first)
    bool AttributeSet::ReadSet(Stream* stream, Endian::EEndianType sourceEndian)
    {
        // read the number of attributes
        uint32 numAttributes;
        if (stream->Read(&numAttributes, sizeof(uint32)) == 0)
        {
            return false;
        }
        Endian::ConvertUnsignedInt32(&numAttributes, sourceEndian);

        // get rid of old attributes
        RemoveAllAttributes();
        mAttributes.Reserve(numAttributes);

        // read all attributes
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            AttributeData attributeData;

            // create the new attribute setting and try to read it
            attributeData.mSettings = AttributeSettings::Create();
            if (attributeData.mSettings->Read(stream, sourceEndian) == false)
            {
                attributeData.mSettings->Destroy();
                attributeData.mSettings = nullptr;
                return false;
            }

            attributeData.mSettings->SetParent(this);

            // read the attribute value
            if (Attribute::ReadFullAttribute(stream, sourceEndian, &attributeData.mValue) == false)
            {
                return false;
            }

            attributeData.mValue->SetParent(this);

            mAttributes.Add(attributeData);
        }

        return true;
    }


    // insert a given attribute
    void AttributeSet::InsertAttribute(uint32 index, AttributeSettings* settings)
    {
        AttributeData newEntry;
        newEntry.mSettings = settings;
        settings->SetParent(this);
        if (settings && settings->GetDefaultValue())
        {
            newEntry.mValue = settings->GetDefaultValue()->Clone();
            newEntry.mValue->SetParent(this);
        }

        mAttributes.Insert(index, newEntry);
    }


    // insert a given attribute, with specified value
    void AttributeSet::InsertAttribute(uint32 index, AttributeSettings* settings, Attribute* attributeValue)
    {
        AttributeData newEntry;
        newEntry.mSettings = settings;
        settings->SetParent(this);
        if (attributeValue == nullptr && settings && settings->GetDefaultValue())
        {
            newEntry.mValue = settings->GetDefaultValue()->Clone();
        }
        else
        {
            newEntry.mValue = attributeValue;
        }

        newEntry.mValue->SetParent(this);
        mAttributes.Insert(index, newEntry);
    }


    // find by name
    uint32 AttributeSet::FindAttributeIndexByName(const char* name) const
    {
        const uint32 numChildren = mAttributes.GetLength();
        for (uint32 i = 0; i < numChildren; ++i)
        {
            if (mAttributes[i].mSettings->GetNameString().CheckIfIsEqualNoCase(name))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by internal name
    uint32 AttributeSet::FindAttributeIndexByInternalName(const char* name) const
    {
        const uint32 numChildren = mAttributes.GetLength();
        for (uint32 i = 0; i < numChildren; ++i)
        {
            if (mAttributes[i].mSettings->GetInternalNameString().CheckIfIsEqualNoCase(name))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by value pointer
    uint32 AttributeSet::FindAttributeIndexByValuePointer(const Attribute* attribute) const
    {
        const uint32 numChildren = mAttributes.GetLength();
        for (uint32 i = 0; i < numChildren; ++i)
        {
            if (mAttributes[i].mValue == attribute)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by name id
    uint32 AttributeSet::FindAttributeIndexByNameID(uint32 nameID) const
    {
        const uint32 numChildren = mAttributes.GetLength();
        for (uint32 i = 0; i < numChildren; ++i)
        {
            if (mAttributes[i].mSettings->GetNameID() == nameID)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by internal name id
    uint32 AttributeSet::FindAttributeIndexByInternalNameID(uint32 nameID) const
    {
        const uint32 numChildren = mAttributes.GetLength();
        for (uint32 i = 0; i < numChildren; ++i)
        {
            if (mAttributes[i].mSettings->GetInternalNameID() == nameID)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // set helpers
    bool AttributeSet::SetFloatAttribute(const char* internalName, float value, bool createIfNotExists)                         { MCORE_ATTRIBUTESET_DECLARE_SET(AttributeFloat) }
    bool AttributeSet::SetInt32Attribute(const char* internalName, int32 value, bool createIfNotExists)                         { MCORE_ATTRIBUTESET_DECLARE_SET(AttributeInt32) }
    bool AttributeSet::SetBoolAttribute(const char* internalName, bool value, bool createIfNotExists)                           { MCORE_ATTRIBUTESET_DECLARE_SET(AttributeBool) }
    bool AttributeSet::SetQuaternionAttribute(const char* internalName, const Quaternion& value, bool createIfNotExists)        { MCORE_ATTRIBUTESET_DECLARE_SET(AttributeQuaternion) }
    bool AttributeSet::SetVector2Attribute(const char* internalName, const AZ::Vector2& value, bool createIfNotExists)          { MCORE_ATTRIBUTESET_DECLARE_SET(AttributeVector2) }
    bool AttributeSet::SetVector3Attribute(const char* internalName, const Vector3& value, bool createIfNotExists)              { MCORE_ATTRIBUTESET_DECLARE_SET(AttributeVector3) }
    bool AttributeSet::SetVector4Attribute(const char* internalName, const AZ::Vector4& value, bool createIfNotExists)          { MCORE_ATTRIBUTESET_DECLARE_SET(AttributeVector4) }
    bool AttributeSet::SetMatrixAttribute(const char* internalName, const Matrix& value, bool createIfNotExists)                { MCORE_ATTRIBUTESET_DECLARE_SET(AttributeMatrix) }
    bool AttributeSet::SetColorAttribute(const char* internalName, const RGBAColor& value, bool createIfNotExists)              { MCORE_ATTRIBUTESET_DECLARE_SET(AttributeColor) }
    bool AttributeSet::SetStringAttribute(const char* internalName, const char* value, bool createIfNotExists)                  { MCORE_ATTRIBUTESET_DECLARE_SET(AttributeString) }

    // get helpers
    const char* AttributeSet::GetStringAttribute(const char* internalName, const char* defaultValue)                            { MCORE_ATTRIBUTESET_DECLARE_GET(AttributeString) }
    float AttributeSet::GetFloatAttribute(const char* internalName, float defaultValue)                                         { MCORE_ATTRIBUTESET_DECLARE_GET(AttributeFloat) }
    int32 AttributeSet::GetInt32Attribute(const char* internalName, int32 defaultValue)                                         { MCORE_ATTRIBUTESET_DECLARE_GET(AttributeInt32) }
    bool AttributeSet::GetBoolAttribute(const char* internalName, bool defaultValue)                                            { MCORE_ATTRIBUTESET_DECLARE_GET(AttributeBool) }
    AZ::Vector2 AttributeSet::GetVector2Attribute(const char* internalName, const AZ::Vector2& defaultValue)                    { MCORE_ATTRIBUTESET_DECLARE_GET(AttributeVector2) }
    Vector3 AttributeSet::GetVector3Attribute(const char* internalName, const Vector3& defaultValue)                            { MCORE_ATTRIBUTESET_DECLARE_GET(AttributeVector3) }
    AZ::Vector4 AttributeSet::GetVector4Attribute(const char* internalName, const AZ::Vector4& defaultValue)                    { MCORE_ATTRIBUTESET_DECLARE_GET(AttributeVector4) }
    Matrix AttributeSet::GetMatrixAttribute(const char* internalName, const Matrix& defaultValue)                               { MCORE_ATTRIBUTESET_DECLARE_GET(AttributeMatrix) }
    Quaternion AttributeSet::GetQuaternionAttribute(const char* internalName, const Quaternion& defaultValue)                   { MCORE_ATTRIBUTESET_DECLARE_GET(AttributeQuaternion) }
    RGBAColor AttributeSet::GetColorAttribute(const char* internalName, const RGBAColor& defaultValue)                          { MCORE_ATTRIBUTESET_DECLARE_GET(AttributeColor) }
}   // namespace MCore
