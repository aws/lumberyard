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
#include "MemoryManager.h"
#include "Endian.h"


namespace MCore
{
    // forward declarations
    class AttributeSettings;
    class Stream;


    class MCORE_API AttributeSettingsSet
    {
        MCORE_MEMORYOBJECTCATEGORY(AttributeSettingsSet, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_ATTRIBUTES);

    public:
        static AttributeSettingsSet* Create();
        void Destroy();

        MCORE_INLINE uint32 GetNumAttributes() const                                { return mAttributes.GetLength(); }
        MCORE_INLINE AttributeSettings* GetAttribute(uint32 index) const            { return mAttributes[index]; }
        MCORE_INLINE void SetAttribute(uint32 index, AttributeSettings* settings)   { mAttributes[index] = settings; }

        void AddAttribute(AttributeSettings* settings);
        void InsertAttribute(uint32 index, AttributeSettings* settings);

        void RemoveAttributeByIndex(uint32 index, bool delFromMem = true);
        bool RemoveAttributeByInternalName(const char* name, bool delFromMem = true);
        bool RemoveAttributeByInternalNameID(uint32 id, bool delFromMem = true);
        bool RemoveAttributeByName(const char* name, bool delFromMem = true);
        bool RemoveAttributeByNameID(uint32 id, bool delFromMem = true);
        void RemoveAllAttributes(bool delFromMem = true);
        void Reserve(uint32 numAttributes);
        void Resize(uint32 numAttributes);
        void Swap(uint32 firstIndex, uint32 secondIndex);

        uint32 FindAttributeIndexByInternalNameID(uint32 id) const;
        uint32 FindAttributeIndexByInternalName(const char* name) const;
        bool CheckIfHasAttributeWithInternalName(const char* name) const;
        bool CheckIfHasAttributeWithInternalNameID(uint32 nameID) const;
        uint32 FindAttributeIndexByNameID(uint32 id) const;
        uint32 FindAttributeIndexByName(const char* name) const;
        bool CheckIfHasAttributeWithName(const char* name) const;
        bool CheckIfHasAttributeWithNameID(uint32 nameID) const;

        void Merge(const AttributeSettingsSet& other, bool overwriteExisting = false);
        void CopyFrom(const AttributeSettingsSet& other);
        void Log();

        bool Write(Stream* stream, Endian::EEndianType targetEndianType) const;
        bool Read(Stream* stream, Endian::EEndianType sourceEndian);
        uint32 CalcStreamSize() const;

    private:
        Array<AttributeSettings*>   mAttributes;

        AttributeSettingsSet();
        ~AttributeSettingsSet();
    };
}   // namespace MCore
