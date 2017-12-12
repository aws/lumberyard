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
#include "Attribute.h"
#include "AttributeFactory.h"
#include "AttributePool.h"
#include "AttributeString.h"
#include "AttributeSettings.h"
#include "UnicodeStringIterator.h"
#include "UnicodeCharacter.h"

namespace MCore
{
    // constructor
    Attribute::Attribute(uint32 typeID)
    {
        mTypeID = typeID;
        mParent = nullptr;
    }


    // destructor
    Attribute::~Attribute()
    {
    }


    // destroy the attribute
    void Attribute::Destroy(bool lock)
    {
        if (lock)
        {
            GetAttributePool().Free(this);
        }
        else
        {
            GetAttributePool().FreeWithoutLock(this);
        }
    }


    // the version we write into the stream
    uint8 Attribute::GetStreamWriteVersion() const
    {
        return 1;
    }


    // get the stream size
    uint32 Attribute::GetStreamSize() const
    {
        return sizeof(uint8) + GetDataSize(); // one byte extra for the version number
    }


    // equal operator
    Attribute& Attribute::operator=(const Attribute& other)
    {
        if (&other != this)
        {
            InitFrom(&other);
        }
        return *this;
    }


    // get the full attribute size
    uint32 Attribute::GetFullAttributeSize(Attribute* attribute)
    {
        uint32 result = 0;

        result += sizeof(uint32); // attribute type
        result += sizeof(uint32); // attribute size

        if (attribute)
        {
            result += attribute->GetStreamSize(); // version + data
        }
        return result;
    }


    // write the attribute
    bool Attribute::Write(Stream* stream, Endian::EEndianType targetEndianType) const
    {
        const uint8 version = GetStreamWriteVersion();
        if (stream->Write(&version, sizeof(uint8)) == 0)
        {
            return false;
        }

        return WriteData(stream, targetEndianType);
    }


    // read the attribute
    bool Attribute::Read(Stream* stream, Endian::EEndianType sourceEndianType)
    {
        // read the version
        uint8 version;
        if (stream->Read(&version, sizeof(uint8)) == 0)
        {
            return false;
        }

        // read the data
        const bool result = ReadData(stream, sourceEndianType, version);
        if (result == false)
        {
            return false;
        }

        return true;
    }


    // write the header
    bool Attribute::WriteHeader(Stream* stream, Endian::EEndianType targetEndianType, const Attribute* attribute)
    {
        // write the attribute type (base it on the default value type)
        uint32 attributeType = (attribute) ? attribute->GetType() : MCORE_INVALIDINDEX32; // invalid index writes a dummy
        Endian::ConvertUnsignedInt32To(&attributeType, targetEndianType);
        if (stream->Write(&attributeType, sizeof(uint32)) == 0)
        {
            return false;
        }

        // write the attribute size
        uint32 attributeSize = (attribute) ? attribute->GetStreamSize() : 0;
        Endian::ConvertUnsignedInt32To(&attributeSize, targetEndianType);
        if (stream->Write(&attributeSize, sizeof(uint32)) == 0)
        {
            return false;
        }

        return true;
    }


    // read the header
    bool Attribute::ReadHeader(Stream* stream, Endian::EEndianType endianType, Attribute** outAttribute)
    {
        // read the attribute type
        uint32 attributeType;
        if (stream->Read(&attributeType, sizeof(uint32)) == 0)
        {
            return false;
        }
        Endian::ConvertUnsignedInt32(&attributeType, endianType);

        // read the attribute size
        uint32 attributeSize;
        if (stream->Read(&attributeSize, sizeof(uint32)) == 0)
        {
            return false;
        }
        Endian::ConvertUnsignedInt32(&attributeSize, endianType);

        // if the size is zero, we dont need an attribute
        if (attributeSize == 0)
        {
            GetAttributePool().Free(*outAttribute);
            *outAttribute = nullptr;
            return true;
        }

        // if the attribute is a nullptr pointer, create a new one
        if (*outAttribute == nullptr)
        {
            Attribute* newAttribute = GetAttributeFactory().CreateAttributeByType(attributeType);
            if (newAttribute)
            {
                *outAttribute = newAttribute;
            }
            else
            {
                LogError("Attribute::ReadHeader() - Failed to create the attribute of type id %d (hex: 0x%x)", attributeType, attributeType);
                *outAttribute = nullptr;
                return false;
            }
        }
        else // the attribute object already exists, see if we can reuse it
        {
            // if the types are different
            if ((*outAttribute)->GetType() != attributeType)
            {
                // delete the existing one first
                GetAttributePool().Free(*outAttribute);
                Attribute* newAttribute = GetAttributeFactory().CreateAttributeByType(attributeType);
                if (newAttribute)
                {
                    *outAttribute = newAttribute;
                }
                else
                {
                    *outAttribute = nullptr;
                    LogError("Attribute::ReadHeader() - Failed to create the attribute of type id %d when trying to convert its type (hex: 0x%x)", attributeType, attributeType);
                    return false;
                }
            }
        }

        return true;
    }


    // write the extended attribute (type, size, version and data)
    bool Attribute::WriteFullAttribute(Stream* stream, Endian::EEndianType targetEndianType, const Attribute* attribute)
    {
        // try to write the header
        if (WriteHeader(stream, targetEndianType, attribute) == false)
        {
            return false;
        }

        // write the data (version + data)
        if (attribute)
        {
            return attribute->Write(stream, targetEndianType);
        }

        return true;
    }


    // read an extended attribute (type, size, version and data)
    bool Attribute::ReadFullAttribute(Stream* stream, Endian::EEndianType endianType, Attribute** outAttribute)
    {
        // try to write the header
        if (ReadHeader(stream, endianType, outAttribute) == false)
        {
            return false;
        }

        // write the data (version + data)
        if (*outAttribute)
        {
            return (*outAttribute)->Read(stream, endianType);
        }

        return true;
    }


    // find by value pointer
    uint32 Attribute::FindAttributeIndexByValuePointer(const Attribute* attribute) const
    {
        const uint32 numChildren = GetNumChildAttributes();
        for (uint32 i = 0; i < numChildren; ++i)
        {
            if (GetChildAttribute(i) == attribute)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by name
    uint32 Attribute::FindAttributeIndexByName(const char* name) const
    {
        const uint32 numChildren = GetNumChildAttributes();
        for (uint32 i = 0; i < numChildren; ++i)
        {
            if (GetChildAttributeSettings(i)->GetNameString().CheckIfIsEqualNoCase(name))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by internal name
    uint32 Attribute::FindAttributeIndexByInternalName(const char* name) const
    {
        const uint32 numChildren = GetNumChildAttributes();
        for (uint32 i = 0; i < numChildren; ++i)
        {
            if (GetChildAttributeSettings(i)->GetInternalNameString().CheckIfIsEqualNoCase(name))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by internal name id
    uint32 Attribute::FindAttributeIndexByInternalNameID(uint32 nameID) const
    {
        const uint32 numChildren = GetNumChildAttributes();
        for (uint32 i = 0; i < numChildren; ++i)
        {
            if (GetChildAttributeSettings(i)->GetInternalNameID() == nameID)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find by name id
    uint32 Attribute::FindAttributeIndexByNameID(uint32 nameID) const
    {
        const uint32 numChildren = GetNumChildAttributes();
        for (uint32 i = 0; i < numChildren; ++i)
        {
            if (GetChildAttributeSettings(i)->GetNameID() == nameID)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find an attribute by its hierarchical name
    Attribute* Attribute::FindAttributeHierarchical(const char* namePath, AttributeSettings** outSettings, bool lookUpRef) const
    {
        //LogInfo("***** Looking up attribute: %s", namePath);
        Attribute* container = nullptr;
        const uint32 attributeIndex = FindAttributeIndexHierarchical(namePath, &container);
        if (attributeIndex != MCORE_INVALIDINDEX32)
        {
            AttributeSettings* resultSettings = container->GetChildAttributeSettings(attributeIndex);
            bool isReferencingOtherAttribute = false;

            if (resultSettings)
            {
                isReferencingOtherAttribute = resultSettings->GetReferencesOtherAttribute();
            }

            // check if we are referencing another attribute, and if this is a string attribute, as then the string value it stores
            // will represent the hierarchical name of the attribute we should get the value from
            Attribute* result = container->GetChildAttribute(attributeIndex);
            if (isReferencingOtherAttribute && result->GetType() == AttributeString::TYPE_ID && lookUpRef)
            {
                // try to extract it from the attribute name it stores
                AttributeSettings* refSettings = nullptr;
                Attribute* refResult = FindAttributeHierarchical(static_cast<AttributeString*>(result)->GetValue().AsChar(), &refSettings, lookUpRef);
                if (refResult && refSettings)
                {
                    if (outSettings)
                    {
                        *outSettings = refSettings;
                    }

                    return refResult;
                }
                else
                {
                    if (outSettings)
                    {
                        *outSettings = resultSettings;
                    }

                    return result;
                }
            }
            else // we're not referencing or its not a string, just output what we found
            {
                if (outSettings)
                {
                    *outSettings = resultSettings;
                }

                return result;
            }
        }

        return nullptr;
    }


    // find an attribute by a hierarchical name such as "materialList.plasticMaterial.specularColor.rgb"
    uint32 Attribute::FindAttributeIndexHierarchical(const char* namePath, Attribute** outContainer) const
    {
        char currentInternalName[256]; // maximum buffer for an internal name
        char currentIndexString[16];
        currentInternalName[0] = '\0';
        currentIndexString[0] = '\0';

        bool    lastIsDot   = false;
        bool    readingIndex = false;
        uint32  startIndex  = 0;
        uint32  endIndex    = 0;
        uint32  result      = MCORE_INVALIDINDEX32;
        uint32  indexStringStart = MCORE_INVALIDINDEX32;
        uint32  indexStringEnd = MCORE_INVALIDINDEX32;
        uint32  childIndex  = MCORE_INVALIDINDEX32;

        Attribute* currentAttribute = const_cast<Attribute*>(this);

        // process the whole string
        UnicodeStringIterator iterator((const char*)namePath);
        while (iterator.GetHasReachedEnd() == false)
        {
            UnicodeCharacter c = iterator.GetNextCharacter();
            if (lastIsDot)
            {
                startIndex = c.GetIndex();
            }

            bool readyToLookup = false;

            const bool isOpenBracket = (c == UnicodeCharacter('['));
            const bool isCloseBracket = (c == UnicodeCharacter(']'));

            const bool isDot = (c == UnicodeCharacter::dot);
            if (isDot)
            {
                endIndex = c.GetIndex();
            }

            if (c == UnicodeCharacter('['))
            {
                indexStringStart = iterator.GetIndex();
                endIndex = c.GetIndex();
                readingIndex = true;
            }

            if (iterator.GetHasReachedEnd() && readingIndex == false)
            {
                endIndex = iterator.GetIndex();
            }

            // if we finished reading the array index value
            if (isCloseBracket && readingIndex)
            {
                readingIndex = false;
                indexStringEnd = c.GetIndex();
                const uint32 numChars = indexStringEnd - indexStringStart;

                if (numChars >= 16)
                {
                    LogError("MCore::Attribute::FindAttributeIndexHierarchical() - The element array number string is over 16 characters, cannot proceed! (namePath=%s)", namePath);
                    *outContainer = nullptr;
                    return MCORE_INVALIDINDEX32;
                }
                MCORE_ASSERT(numChars < 16);

                azstrncpy(currentIndexString, 16, &namePath[indexStringStart], numChars);

                currentIndexString[numChars] = '\0';

                childIndex = static_cast<uint32>(atoi(currentIndexString));
                readyToLookup = true;
                //MCore::LogInfo("*** ARRAY INDEX FOUND = %s (%d)  --> %s[%d]", currentIndexString, childIndex, currentInternalName, childIndex);
            }

            // if we can build the name of the attribute
            if ((c == UnicodeCharacter::dot || (iterator.GetHasReachedEnd() && readingIndex == false) || (isOpenBracket && readingIndex)) && currentInternalName[0] == '\0')
            {
                const uint32 numCharacters = endIndex - startIndex;
                if (numCharacters >= 256)
                {
                    LogError("MCore::Attribute::FindAttributeIndexHierarchical() - The internal name of the current attribute is over 256 characters, cannot proceed (namePath=%s)!", namePath);
                    *outContainer = nullptr;
                    return MCORE_INVALIDINDEX32;
                }
                MCORE_ASSERT(numCharacters < 256);

                if (numCharacters > 0)
                {
                    // create the name buffer
                    azstrncpy(currentInternalName, 256, &namePath[startIndex], numCharacters);

                    currentInternalName[numCharacters] = '\0';

                    if (iterator.GetHasReachedEnd() || isOpenBracket == false)
                    {
                        readyToLookup = true;
                    }
                }
                else
                {
                    currentInternalName[0] = '\0';
                }
            }

            // if we are ready to output
            if (readyToLookup)
            {
                //LogInfo("INTERNAL NAME = %s  (index=%d)", currentInternalName, childIndex);

                startIndex = iterator.GetIndex();

                // if we have a name
                if (currentInternalName[0] != '\0')
                {
                    // search for it
                    uint32 attributeIndex = currentAttribute->FindAttributeIndexByInternalName(currentInternalName);
                    if (attributeIndex != MCORE_INVALIDINDEX32)
                    {
                        if (iterator.GetHasReachedEnd() == false)
                        {
                            currentAttribute = currentAttribute->GetChildAttribute(attributeIndex);
                            if (childIndex != MCORE_INVALIDINDEX32)
                            {
                                const uint32 numChildAttributes = currentAttribute->GetNumChildAttributes();
                                if (childIndex >= numChildAttributes)
                                {
                                    LogError("MCore::Attribute::FindAttributeIndexHierarchical() - The child index for the array lookup (%s[%d]) is out of range (requestedIndex=%d numChildAttributes=%d namePath=%s].", currentInternalName, childIndex, childIndex, numChildAttributes, namePath);
                                    *outContainer = nullptr;
                                    MCORE_ASSERT(numChildAttributes > childIndex);  // we go out of range
                                    return MCORE_INVALIDINDEX32;
                                }

                                currentAttribute = currentAttribute->GetChildAttribute(childIndex);
                            }
                        }
                        else
                        {
                            if (childIndex != MCORE_INVALIDINDEX32)
                            {
                                currentAttribute = currentAttribute->GetChildAttribute(attributeIndex);
                                const uint32 numChildAttributes = currentAttribute->GetNumChildAttributes();
                                if (childIndex >= numChildAttributes)
                                {
                                    LogError("MCore::Attribute::FindAttributeIndexHierarchical() - The child index for the array lookup (%s[%d]) is out of range (requestedIndex=%d numChildAttributes=%d namePath=%s].", currentInternalName, childIndex, childIndex, numChildAttributes, namePath);
                                    *outContainer = nullptr;
                                    MCORE_ASSERT(numChildAttributes > childIndex);  // we go out of range
                                    return MCORE_INVALIDINDEX32;
                                }

                                *outContainer = currentAttribute;
                                return childIndex;
                            }
                            else
                            {
                                *outContainer = currentAttribute;
                                return attributeIndex;
                            }
                        }
                    }
                    else
                    {
                        *outContainer = nullptr;
                        return MCORE_INVALIDINDEX32;
                    }

                    currentInternalName[0] = '\0';
                }
                else // there is no current internal name, so we use just a [5] style index
                {
                    MCORE_ASSERT(childIndex != MCORE_INVALIDINDEX32);

                    const uint32 numChildAttributes = currentAttribute->GetNumChildAttributes();
                    if (childIndex >= numChildAttributes)
                    {
                        LogError("MCore::Attribute::FindAttributeIndexHierarchical() - The child index for the array lookup is out of range (requestedIndex=%d numChildAttributes=%d namePath=%s]", childIndex, numChildAttributes, namePath);
                        *outContainer = nullptr;
                        MCORE_ASSERT(numChildAttributes > childIndex);  // we go out of range
                        return MCORE_INVALIDINDEX32;
                    }

                    if (iterator.GetHasReachedEnd() == false)
                    {
                        currentAttribute = currentAttribute->GetChildAttribute(childIndex);
                    }
                    else
                    {
                        *outContainer = currentAttribute;
                        return childIndex;
                    }
                }

                childIndex = MCORE_INVALIDINDEX32;
            }

            lastIsDot = isDot;
        }

        *outContainer = nullptr;
        return result;
    }


    // set the parent
    void Attribute::SetParent(Attribute* parent)
    {
        mParent = parent;
    }


    // get the parent
    Attribute* Attribute::GetParent() const
    {
        return mParent;
    }


    // recursively update all parent pointers
    void Attribute::UpdateChildParentPointers(bool recursive)
    {
        const uint32 numChildren = GetNumChildAttributes();
        for (uint32 i = 0; i < numChildren; ++i)
        {
            Attribute* curChild = GetChildAttribute(i);
            AttributeSettings* curChildSettings = GetChildAttributeSettings(i);

            if (curChild)
            {
                curChild->SetParent(this);
            }

            if (curChildSettings)
            {
                curChildSettings->SetParent(this);
            }

            if (recursive)
            {
                curChild->UpdateChildParentPointers(true);
            }
        }
    }


    // find the attribute settings for this attribute
    AttributeSettings* Attribute::FindAttributeSettings() const
    {
        uint32 childIndex = (mParent) ? mParent->FindAttributeIndexByValuePointer(const_cast<Attribute*>(this)) : MCORE_INVALIDINDEX32;
        AttributeSettings* settings = (childIndex != MCORE_INVALIDINDEX32) ? mParent->GetChildAttributeSettings(childIndex) : nullptr;
        return settings;
    }


    // return a string with the hierarchical name
    String Attribute::BuildHierarchicalName() const
    {
        String result;
        BuildHierarchicalName(result);
        return result;
    }


    // build a hierarchical string like "materialList.myMaterial.specularColor.rgb"
    // arrays or nameless attributes look like this: "materialList.myMaterial.parameters[10]" where 10 would be the zero-based index into the child array of parameters
    void Attribute::BuildHierarchicalName(String& outString) const
    {
        outString.Clear();
        outString.Reserve(64);

        String temp;

        // work our way up towards the root
        const Attribute* curAttribute = this;
        while (curAttribute)
        {
            Attribute* parentAttribute = curAttribute->GetParent();
            AttributeSettings* curSettings = curAttribute->FindAttributeSettings();

            bool hasName = false;
            if (curSettings)
            {
                hasName = (curSettings->GetInternalNameString().GetLength() > 0);
                if (hasName == false && parentAttribute)
                {
                    const uint32 attributeIndex = parentAttribute->FindAttributeIndexByValuePointer(curAttribute);
                    temp.Format("[%d]", attributeIndex);
                    outString.Insert(0, temp.AsChar());
                }

                outString.Insert(0, curSettings->GetInternalName());
            }

            if (parentAttribute && parentAttribute->GetParent() && hasName)
            {
                outString.Insert(0, ".");
            }

            curAttribute = parentAttribute;
        }
    }
}   // namespace MCore
