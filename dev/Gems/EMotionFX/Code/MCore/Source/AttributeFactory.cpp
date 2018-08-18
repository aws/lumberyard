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
#include "StandardHeaders.h"
#include "AttributeFactory.h"
#include "LogManager.h"

// include default types
#include "AttributeFloat.h"
#include "AttributeInt32.h"
#include "AttributeString.h"
#include "AttributeBool.h"
#include "AttributeVector2.h"
#include "AttributeVector3.h"
#include "AttributeVector4.h"
#include "AttributeQuaternion.h"
#include "AttributeColor.h"
#include "AttributePointer.h"


namespace MCore
{
    // constructor
    AttributeFactory::AttributeFactory()
    {
        mRegistered.SetMemoryCategory(MCORE_MEMCATEGORY_ATTRIBUTEFACTORY);
        RegisterStandardTypes();
    }


    // destructor
    AttributeFactory::~AttributeFactory()
    {
        UnregisterAllAttributes();
    }


    // unregister all attributes
    void AttributeFactory::UnregisterAllAttributes(bool delFromMem)
    {
        // delete all attributes from memory
        if (delFromMem)
        {
            const uint32 numAttributes = mRegistered.GetLength();
            for (uint32 i = 0; i < numAttributes; ++i)
            {
                delete mRegistered[i];
            }
        }

        // clear the array
        mRegistered.Clear();
    }


    // register a given attribute
    void AttributeFactory::RegisterAttribute(Attribute* attribute)
    {
        // check first if the type hasn't already been registered
        const uint32 attribIndex = FindAttributeIndexByType(attribute->GetType());
        if (attribIndex != MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("MCore::AttributeFactory::RegisterAttribute() - There is already an attribute of the same type registered (typeID %d vs %d - typeString '%s' vs '%s')", attribute->GetType(), mRegistered[attribIndex]->GetType(), attribute->GetTypeString(), mRegistered[attribIndex]->GetTypeString());
            return;
        }

        // register it
        mRegistered.Add(attribute);
    }


    // unregister a given attribute
    void AttributeFactory::UnregisterAttribute(Attribute* attribute, bool delFromMem)
    {
        // check first if the type hasn't already been registered
        const uint32 attribIndex = FindAttributeIndexByType(attribute->GetType());
        if (attribIndex == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("MCore::AttributeFactory::UnregisterAttribute() - No attribute with the given type found (typeID=%d - typeString='%s'", attribute->GetType(), attribute->GetTypeString());
            return;
        }

        // delete it from memory
        if (delFromMem)
        {
            delete mRegistered[attribIndex];
        }

        // remove it from the array
        mRegistered.Remove(attribIndex);
    }


    // find an attribute by a given ID
    uint32 AttributeFactory::FindAttributeIndexByType(uint32 typeID) const
    {
        // iterate over all registered attribs
        const uint32 numAttributes = mRegistered.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            if (mRegistered[i]->GetType() == typeID) // we found one with the same type
            {
                return i;
            }
        }

        // no attribute of this type found
        return MCORE_INVALIDINDEX32;
    }


    // find by type string
    uint32 AttributeFactory::FindAttributeIndexByTypeString(const char* typeString) const
    {
        // iterate over all registered attribs
        const uint32 numAttributes = mRegistered.GetLength();
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            if (strcmp(mRegistered[i]->GetTypeString(), typeString) == 0) // we found one with the same type
            {
                return i;
            }
        }

        // no attribute of this type found
        return MCORE_INVALIDINDEX32;
    }


    // create an attribute of a given type
    Attribute* AttributeFactory::CreateAttributeByType(uint32 typeID) const
    {
        // find the attribute
        const uint32 attribIndex = FindAttributeIndexByType(typeID);
        if (attribIndex == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        // create and return a clone
        return mRegistered[attribIndex]->Clone();
    }


    // create an attribute from a given type string
    Attribute* AttributeFactory::CreateAttributeByTypeString(const char* typeString) const
    {
        // find the attribute
        const uint32 attribIndex = FindAttributeIndexByTypeString(typeString);
        if (attribIndex == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        // create and return a clone
        return mRegistered[attribIndex]->Clone();
    }


    // create from another attribute
    Attribute* AttributeFactory::CreateAttributeByAttribute(const Attribute* attributeType) const
    {
        return CreateAttributeByType(attributeType->GetType());
    }


    // register standard types
    void AttributeFactory::RegisterStandardTypes()
    {
        // reserve some memory to prevent reallocs
        mRegistered.Reserve(32);

        // register the standard attribute types
        RegisterAttribute(new AttributeFloat());
        RegisterAttribute(new AttributeInt32());
        RegisterAttribute(new AttributeString());
        RegisterAttribute(new AttributeBool());
        RegisterAttribute(new AttributeVector2());
        RegisterAttribute(new AttributeVector3());
        RegisterAttribute(new AttributeVector4());
        RegisterAttribute(new AttributeQuaternion());
        RegisterAttribute(new AttributeColor());
        RegisterAttribute(new AttributePointer());
    }
}   // namespace MCore
