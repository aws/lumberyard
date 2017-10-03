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

// initialize the box
// this creates an invalid box (with negative extents) so the IsValid method will return false
MCORE_INLINE void OBB::Init()
{
    mCenter.Set(0.0f, 0.0f, 0.0f);
    mExtents.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    mRotation.Identity();
}


// check if the OBB is valid
MCORE_INLINE bool OBB::CheckIfIsValid() const
{
    if (mExtents.x < 0.0f)
    {
        return false;
    }
    if (mExtents.y < 0.0f)
    {
        return false;
    }
    if (mExtents.z < 0.0f)
    {
        return false;
    }
    return true;
}

