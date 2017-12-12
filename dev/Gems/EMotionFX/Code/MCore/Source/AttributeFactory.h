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
#include "Array.h"
#include "Attribute.h"

namespace MCore
{
    /**
     * The attribute factory, which is used to create attribute objects.
     */
    class MCORE_API AttributeFactory
    {
    public:
        AttributeFactory();
        ~AttributeFactory();

        //
        void UnregisterAllAttributes(bool delFromMem = true);
        void RegisterAttribute(Attribute* attribute);
        void UnregisterAttribute(Attribute* attribute, bool delFromMem = true);
        void RegisterStandardTypes();

        MCORE_INLINE uint32 GetNumRegisteredAttributes() const                  { return mRegistered.GetLength(); }
        MCORE_INLINE Attribute* GetRegisteredAttribute(uint32 index) const      { return mRegistered[index]; }

        uint32 FindAttributeIndexByType(uint32 typeID) const;
        uint32 FindAttributeIndexByTypeString(const char* typeString) const;

        Attribute* CreateAttributeByType(uint32 typeID) const;
        Attribute* CreateAttributeByTypeString(const char* typeString) const;
        Attribute* CreateAttributeByAttribute(const Attribute* attributeType) const;

    private:
        MCore::Array<Attribute*>    mRegistered;
    };
}   // namespace MCore
