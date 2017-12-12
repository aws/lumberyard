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

// include headers
#include "VertexAttributeLayer.h"
#include <MCore/Source/StringIDGenerator.h>


namespace EMotionFX
{
    // constructor
    VertexAttributeLayer::VertexAttributeLayer(uint32 numAttributes, bool keepOriginals)
        : BaseObject()
    {
        mNumAttributes  = numAttributes;
        mKeepOriginals  = keepOriginals;
        mNameID         = MCore::GetStringIDGenerator().GenerateIDForString("");
    }


    // destructor
    VertexAttributeLayer::~VertexAttributeLayer()
    {
    }


    // returns true when we deal with the abstract vertex attribute layer class
    // not very elegant to do it this way, but it's probably the best way with this design
    bool VertexAttributeLayer::GetIsAbstractDataClass() const
    {
        return false;
    }


    // set the name
    void VertexAttributeLayer::SetName(const char* name)
    {
        mNameID = MCore::GetStringIDGenerator().GenerateIDForString(name);
    }


    // get the name
    const char* VertexAttributeLayer::GetName() const
    {
        return MCore::GetStringIDGenerator().GetName(mNameID).AsChar();
    }


    // get the name string
    const MCore::String& VertexAttributeLayer::GetNameString() const
    {
        return MCore::GetStringIDGenerator().GetName(mNameID);
    }


    // get the name ID
    uint32 VertexAttributeLayer::GetNameID() const
    {
        return mNameID;
    }
} // namespace EMotionFX
