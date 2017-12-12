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

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <SceneAPI/FbxSDKWrapper/FbxSceneWrapper.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneSystem.h>

namespace AZ
{
    namespace SceneAPI
    {
        FbxSceneSystem::FbxSceneSystem() :
            m_unitSizeInMeters(1.0f),
            m_adjustTransform(nullptr),
            m_adjustTransformInverse(nullptr)
        {
        }

        void FbxSceneSystem::Set(const FbxSDKWrapper::FbxSceneWrapper& fbxScene)
        {
            // Get unit conversion factor to meter.
            m_unitSizeInMeters = fbxScene.GetSystemUnit()->GetConversionFactorTo(FbxSDKWrapper::FbxSystemUnitWrapper::m);
            
            int sign = 0;
            FbxSDKWrapper::FbxAxisSystemWrapper::UpVector upVector = fbxScene.GetAxisSystem()->GetUpVector(sign);

            if (upVector != FbxSDKWrapper::FbxAxisSystemWrapper::Z && upVector != FbxSDKWrapper::FbxAxisSystemWrapper::Unknown)
            {
                m_adjustTransform.reset(new Transform(fbxScene.GetAxisSystem()->CalculateConversionTransform(FbxSDKWrapper::FbxAxisSystemWrapper::Z)));
                m_adjustTransformInverse.reset(new Transform(m_adjustTransform->GetInverseFull()));
            }
        }

        void FbxSceneSystem::SwapVec3ForUpAxis(Vector3& swapVector) const
        {
            if (m_adjustTransform)
            {
                swapVector = *m_adjustTransform * swapVector;
            }
        }

        void FbxSceneSystem::SwapTransformForUpAxis(Transform& inOutTransform) const
        {
            if (m_adjustTransform)
            {
                inOutTransform = *m_adjustTransform * inOutTransform * *m_adjustTransformInverse;
            }
        }

        void FbxSceneSystem::ConvertUnit(Vector3& scaleVector) const
        {
            scaleVector *= m_unitSizeInMeters;
        }

        void FbxSceneSystem::ConvertUnit(Transform& inOutTransform) const
        {
            Vector3 position = inOutTransform.GetPosition();
            position *= m_unitSizeInMeters;
            inOutTransform.SetPosition(position);
        }

        void FbxSceneSystem::ConvertBoneUnit(Transform& inOutTransform) const
        {


            // Need to scale translation explicitly as MultiplyByScale won't change the translation component
            // and we need to convert to meter unit
            Vector3 position = inOutTransform.GetPosition();
            position *= m_unitSizeInMeters;
            inOutTransform.SetPosition(position);
        }
    }
}