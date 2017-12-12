#pragma once

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

#include <AzCore/Math/Transform.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneBuilderConfiguration.h>

namespace AZ
{
    class Vector3;

    namespace FbxSDKWrapper
    {
        class FbxSceneWrapper;
    }

    namespace SceneAPI
    {
        class FbxSceneSystem
        {
        public:
            FBX_SCENE_BUILDER_API FbxSceneSystem();

            FBX_SCENE_BUILDER_API void Set(const FbxSDKWrapper::FbxSceneWrapper& fbxScene);
            FBX_SCENE_BUILDER_API void SwapVec3ForUpAxis(Vector3& swapVector) const;
            FBX_SCENE_BUILDER_API void SwapTransformForUpAxis(Transform& inOutTransform) const;
            FBX_SCENE_BUILDER_API void ConvertUnit(Vector3& scaleVector) const;
            FBX_SCENE_BUILDER_API void ConvertUnit(Transform& inOutTransform) const;
            FBX_SCENE_BUILDER_API void ConvertBoneUnit(Transform& inOutTransform) const;

        protected:
            float m_unitSizeInMeters;
            AZStd::unique_ptr<Transform> m_adjustTransform;
            AZStd::unique_ptr<Transform> m_adjustTransformInverse;
        };
    }
};