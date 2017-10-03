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
#include "Material.h"
#include <MCore/Source/StringIDGenerator.h>
#include <MCore/Source/AttributeSet.h>


namespace EMotionFX
{
    // constructor
    Material::Material(const char* name)
    {
        SetName(name);
        mAttributeSet = MCore::AttributeSet::Create();
    }


    // destructor
    Material::~Material()
    {
        if (mAttributeSet)
        {
            mAttributeSet->Destroy();
        }
    }


    // create a material
    Material* Material::Create(const char* name)
    {
        return new Material(name);
    }


    // set the material name
    void Material::SetName(const char* name)
    {
        // calculate the ID
        mNameID = MCore::GetStringIDGenerator().GenerateIDForString(name);
    }


    // return the material name
    const char* Material::GetName() const
    {
        return MCore::GetStringIDGenerator().GetName(mNameID).AsChar();
    }


    // return the material name as a string
    const MCore::String& Material::GetNameString() const
    {
        return MCore::GetStringIDGenerator().GetName(mNameID);
    }


    // return a clone
    Material* Material::Clone() const
    {
        Material* newMat = Material::Create(GetName());
        newMat->mAttributeSet->InitFrom(mAttributeSet);
        return newMat;
    }
} // namespace EMotionFX
