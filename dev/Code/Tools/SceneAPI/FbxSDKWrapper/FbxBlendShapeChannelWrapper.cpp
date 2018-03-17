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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Debug/Trace.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/FbxSDKWrapper/FbxBlendShapeChannelWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxBlendShapeChannelWrapper::FbxBlendShapeChannelWrapper(FbxBlendShapeChannel* fbxBlendShapeChannel)
            : m_fbxBlendShapeChannel(fbxBlendShapeChannel)
        {
            AZ_Assert(fbxBlendShapeChannel, "Invalid FbxSkin input to initialize FbxBlendShapeChannelWrapper");
        }

        FbxBlendShapeChannelWrapper::~FbxBlendShapeChannelWrapper()
        {
            m_fbxBlendShapeChannel = nullptr;
        }

        const char* FbxBlendShapeChannelWrapper::GetName() const
        {
            return m_fbxBlendShapeChannel->GetName();
        }

        //The engine currently only supports one target shape.  If there are more than one,
        //code will ultimately end up just using the max index returned by this function. 
        int FbxBlendShapeChannelWrapper::GetTargetShapeCount() const
        {
            return m_fbxBlendShapeChannel->GetTargetShapeCount();
        }

        //While not strictly true that the target shapes are meshes, for the purposes of the engine's
        //current runtime they must be meshes. 
        AZStd::shared_ptr<const FbxMeshWrapper> FbxBlendShapeChannelWrapper::GetTargetShape(int index) const
        {
            FbxShape* fbxShape = m_fbxBlendShapeChannel->GetTargetShape(index);
            FbxGeometry* fbxGeom = fbxShape? fbxShape->GetBaseGeometry() : nullptr;

            if (fbxGeom && fbxGeom->GetAttributeType() == FbxNodeAttribute::EType::eMesh)
            {
                return AZStd::make_shared<FbxMeshWrapper>(static_cast<FbxMesh*>(fbxGeom));
            }
            else
            {
                return nullptr;
            }
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
