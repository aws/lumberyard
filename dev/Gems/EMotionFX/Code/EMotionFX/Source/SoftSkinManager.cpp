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
#include "SoftSkinManager.h"
#include "SoftSkinDeformer.h"


namespace EMotionFX
{
    // constructor
    SoftSkinManager::SoftSkinManager()
        : BaseObject()
    {
    }


    // create
    SoftSkinManager* SoftSkinManager::Create()
    {
        return new SoftSkinManager();
    }


    // create the deformer
    SoftSkinDeformer* SoftSkinManager::CreateDeformer(Mesh* mesh)
    {
        return SoftSkinDeformer::Create(mesh);
    }
} // namespace EMotionFX
