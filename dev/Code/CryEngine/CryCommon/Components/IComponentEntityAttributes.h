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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTENTITYATTRIBUTES_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTENTITYATTRIBUTES_H
#pragma once

#include "BoostHelpers.h"

struct IEntityAttribute;

DECLARE_SMART_POINTERS(IEntityAttribute)

typedef DynArray<IEntityAttributePtr> TEntityAttributeArray;

//! Interface for the attribute storage component.
//! This component exists to store IEntityAttributes for its entity.
struct IComponentEntityAttributes
    : public IComponent
{
    DECLARE_COMPONENT_TYPE("ComponentEntityAttributes", 0x46C2E70ECD91414E, 0xB3E70FC12825208A)

    IComponentEntityAttributes() {}
    virtual ~IComponentEntityAttributes() {}

    virtual void SetAttributes(const TEntityAttributeArray& attributes) = 0;
    virtual TEntityAttributeArray& GetAttributes() = 0;
    virtual const TEntityAttributeArray& GetAttributes() const = 0;
};

DECLARE_COMPONENT_POINTERS(IComponentEntityAttributes)

namespace EntityAttributeUtils
{
    //////////////////////////////////////////////////////////////////////////
    // Description:
    //    Clone array of entity attributes.
    //////////////////////////////////////////////////////////////////////////
    inline void CloneAttributes(const TEntityAttributeArray& src, TEntityAttributeArray& dst)
    {
        dst.clear();
        dst.reserve(src.size());
        for (TEntityAttributeArray::const_iterator iAttribute = src.begin(), iEndAttribute = src.end(); iAttribute != iEndAttribute; ++iAttribute)
        {
            dst.push_back((*iAttribute)->Clone());
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //    Find entity attribute by name.
    //////////////////////////////////////////////////////////////////////////
    inline IEntityAttributePtr FindAttribute(TEntityAttributeArray& attributes, const char* name)
    {
        CRY_ASSERT(name != NULL);
        if (name != NULL)
        {
            for (TEntityAttributeArray::iterator iAttribute = attributes.begin(), iEndAttribute = attributes.end(); iAttribute != iEndAttribute; ++iAttribute)
            {
                if (strcmp((*iAttribute)->GetName(), name) == 0)
                {
                    return *iAttribute;
                }
            }
        }
        return IEntityAttributePtr();
    }

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //    Find entity attribute by name.
    //////////////////////////////////////////////////////////////////////////
    inline IEntityAttributeConstPtr FindAttribute(const TEntityAttributeArray& attributes, const char* name)
    {
        CRY_ASSERT(name != NULL);
        if (name != NULL)
        {
            for (TEntityAttributeArray::const_iterator iAttribute = attributes.begin(), iEndAttribute = attributes.end(); iAttribute != iEndAttribute; ++iAttribute)
            {
                if (strcmp((*iAttribute)->GetName(), name) == 0)
                {
                    return *iAttribute;
                }
            }
        }
        return IEntityAttributePtr();
    }
}

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTENTITYATTRIBUTES_H