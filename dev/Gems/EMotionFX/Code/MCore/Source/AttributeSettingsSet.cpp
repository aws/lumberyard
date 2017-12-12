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
#include "AttributeSettingsSet.h"
#include "StringIDGenerator.h"
#include "LogManager.h"
#include "UnicodeString.h"
#include "Stream.h"
#include "AttributeSettings.h"


namespace MCore
{
    // constructor
    AttributeSettingsSet::AttributeSettingsSet()
    {
        mAttributes.SetMemoryCategory(MCORE_MEMCATEGORY_ATTRIBUTES);
    }


    // destructor
    AttributeSettingsSet::~AttributeSettingsSet()
    {
        RemoveAllAttributes();
    }


    // create
    AttributeSettingsSet* AttributeSettingsSet::Create()
    {
        return new AttributeSettingsSet();
    }


    // destroy this class object
    void AttributeSettingsSet::Destroy()
    {
        delete this;
    }


    // add an attribute
    void AttributeSettingsSet::AddAttribute(AttributeSettings* settings)
    {
        MCORE_ASSERT(CheckIfHasAttributeWithInternalNameID(settings->GetInternalNameID()) == false);
        mAttributes.Add(settings);
    }


    // remove a given attribute by its index
    void AttributeSettingsSet::RemoveAttributeByIndex(uint32 index, bool delFromMem)
    {
        if (delFromMem)
        {
            mAttributes[index]->Destroy();
        }

        mAttributes.Remove(index);
    }


    // remove a given attribute by its name
    bool AttributeSettingsSet::RemoveAttributeByInternalName(const char* name, bool delFromMem)
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
    bool AttributeSettingsSet::RemoveAttributeByInternalNameID(uint32 id, bool delFromMem)
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
    bool AttributeSettingsSet::RemoveAttributeByName(const char* name, bool delFromMem)
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
    bool AttributeSettingsSet::RemoveAttributeByNameID(uint32 id, bool delFromMem)
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
    void AttributeSettingsSet::RemoveAllAttributes(bool delFromMem)
    {
        if (delFromMem)
        {
            const uint32 numAttributes = mAttributes.GetLength();
            for (uint32 i = 0; i < numAttributes; ++i)
            {
                mAttributes[i]->Destroy();
            }
        }

        mAttributes.Clear();
    }


    // reserve attribute memory
    void AttributeSettingsSet::Reserve(uint32 numAttributes)
    {
        mAttributes.Reserve(numAttributes);
    }


    // resize attribute memory
    void AttributeSettingsSet::Resize(uint32 numAttributes)
    {
        mAttributes.Resize(numAttributes);
    }


    // swap two attributes
    void AttributeSettingsSet::Swap(uint32 firstIndex, uint32 secondIndex)
    {
        mAttributes.Swap(firstIndex, secondIndex);
    }


    // find an attribute by its ID
    uint32 AttributeSettingsSet::FindAttributeIndexByInternalNameID(uint32 id) const
    {
        const uint32 numAttribs = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            if (mAttributes[i]->GetInternalNameID() == id)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find an attribute by its name
    uint32 AttributeSettingsSet::FindAttributeIndexByInternalName(const char* name) const
    {
        const uint32 numAttribs = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            if (mAttributes[i]->GetInternalNameString() == name)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find an attribute by its ID
    uint32 AttributeSettingsSet::FindAttributeIndexByNameID(uint32 id) const
    {
        const uint32 numAttribs = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            if (mAttributes[i]->GetNameID() == id)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find an attribute by its name
    uint32 AttributeSettingsSet::FindAttributeIndexByName(const char* name) const
    {
        const uint32 numAttribs = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            if (mAttributes[i]->GetNameString().CheckIfIsEqualNoCase(name))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // check if there is an attribute with the given name
    bool AttributeSettingsSet::CheckIfHasAttributeWithInternalName(const char* name) const
    {
        return (FindAttributeIndexByInternalName(name) != MCORE_INVALIDINDEX32);
    }


    // check if there is an attribute with the given type
    bool AttributeSettingsSet::CheckIfHasAttributeWithInternalNameID(uint32 id) const
    {
        return (FindAttributeIndexByInternalNameID(id) != MCORE_INVALIDINDEX32);
    }


    // check if there is an attribute with the given name
    bool AttributeSettingsSet::CheckIfHasAttributeWithName(const char* name) const
    {
        return (FindAttributeIndexByName(name) != MCORE_INVALIDINDEX32);
    }


    // check if there is an attribute with the given type
    bool AttributeSettingsSet::CheckIfHasAttributeWithNameID(uint32 id) const
    {
        return (FindAttributeIndexByNameID(id) != MCORE_INVALIDINDEX32);
    }


    // merge this attribute set with another
    void AttributeSettingsSet::Merge(const AttributeSettingsSet& other, bool overwriteExisting)
    {
        const uint32 numAttribs = other.GetNumAttributes();
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            const AttributeSettings* curAttribute = other.mAttributes[i];

            // find the attribute in the current set
            const uint32 index = FindAttributeIndexByInternalNameID(curAttribute->GetInternalNameID());

            // if the attribute doesn't exist inside the current set, add it
            if (index == MCORE_INVALIDINDEX32)
            {
                mAttributes.Add(curAttribute->Clone());
                continue;
            }

            // the item already exists
            if (overwriteExisting == false)
            {
                continue;
            }

            // overwrite the data
            mAttributes[index]->InitFrom(curAttribute);
        }
    }


    // copy over attributes
    void AttributeSettingsSet::CopyFrom(const AttributeSettingsSet& other)
    {
        // remove all attributes
        RemoveAllAttributes();

        // copy over all attributes
        const uint32 numAttribs = other.GetNumAttributes();
        mAttributes.Reserve(numAttribs);
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            mAttributes.Add(other.mAttributes[i]->Clone());
        }
    }


    // log the data inside the set
    void AttributeSettingsSet::Log()
    {
        String valueString;
        const uint32 numAttribs = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            LogDetailedInfo("#%d - internalName='%s' - name='%s' - interfaceType=%d", i, mAttributes[i]->GetInternalName(), mAttributes[i]->GetName(), mAttributes[i]->GetInterfaceType());
        }
    }


    // write the set to a stream
    bool AttributeSettingsSet::Write(Stream* stream, Endian::EEndianType targetEndianType) const
    {
        // write the set version
        uint8 version = 1;
        if (stream->Write(&version, sizeof(uint8)) == 0)
        {
            return false;
        }

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
            if (mAttributes[i]->Write(stream, targetEndianType) == false)
            {
                return false;
            }
        }

        return true;
    }


    // read the set from a stream (this clear the current contents first)
    bool AttributeSettingsSet::Read(Stream* stream, Endian::EEndianType sourceEndian)
    {
        // read the set version
        uint8 version;
        if (stream->Read(&version, sizeof(uint8)) == 0)
        {
            return false;
        }

        MCORE_ASSERT(version == 1); // only supports version 1

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
            // create the new attribute setting and try to read it
            AttributeSettings* newAttribute = AttributeSettings::Create();
            if (newAttribute->Read(stream, sourceEndian) == false)
            {
                newAttribute->Destroy();
                return false;
            }
            else
            {
                mAttributes.Add(newAttribute);
            }
        }

        return true;
    }


    // calculate the stream size
    uint32 AttributeSettingsSet::CalcStreamSize() const
    {
        uint32 totalSize = 0;

        totalSize += sizeof(uint8); // the version
        totalSize += sizeof(uint32); // numAttributes

        // for all attributes
        const uint32 numAttributes = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            totalSize += mAttributes[i]->CalcStreamSize();
        }

        return totalSize;
    }


    // insert a given attribute
    void AttributeSettingsSet::InsertAttribute(uint32 index, AttributeSettings* settings)
    {
        mAttributes.Insert(index, settings);
    }
}   // namespace MCore
