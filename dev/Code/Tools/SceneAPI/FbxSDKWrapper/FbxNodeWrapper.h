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

#include <SceneAPI/FbxSDKWrapper/FbxTimeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxMeshWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxPropertyWrapper.h>
#include <AzCore/Math/Transform.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxMaterialWrapper;
        class FbxAnimLayerWrapper;
        class FbxAnimCurveWrapper;

        class FbxNodeWrapper
        {
        public:
            enum CurveNodeComponent
            {
                Component_X,
                Component_Y,
                Component_Z
            };

            FbxNodeWrapper(FbxNode* fbxNode);
            virtual ~FbxNodeWrapper();

            FbxNode* GetFbxNode();

            virtual int GetMaterialCount() const;
            virtual const char* GetMaterialName(int index) const;
            virtual const std::shared_ptr<FbxMaterialWrapper> GetMaterial(int index) const;
            virtual const std::shared_ptr<FbxMeshWrapper> GetMesh() const;
            virtual const std::shared_ptr<FbxPropertyWrapper> FindProperty(const char* name) const;
            virtual bool IsBone() const;
            virtual const char* GetName() const;
            virtual AZ::u64 GetUniqueId() const;

            virtual Transform EvaluateGlobalTransform();
            virtual Vector3 EvaluateLocalTranslation();
            virtual Vector3 EvaluateLocalTranslation(FbxTimeWrapper& time);
            virtual Transform EvaluateLocalTransform();
            virtual Transform EvaluateLocalTransform(FbxTimeWrapper& time);
            virtual Vector3 EvaluateLocalRotation();
            virtual Vector3 EvaluateLocalRotation(FbxTimeWrapper& time);
            virtual Vector3 GetGeometricTranslation() const;
            virtual Vector3 GetGeometricScaling() const;
            virtual Vector3 GetGeometricRotation() const;
            virtual Transform GetGeometricTransform() const;
            virtual const AZStd::shared_ptr<FbxAnimCurveWrapper> GetLocalTranslationCurve(const AZStd::shared_ptr<FbxAnimLayerWrapper>& layer, CurveNodeComponent component) const;
            virtual const AZStd::shared_ptr<FbxAnimCurveWrapper> GetLocalRotationCurve(const AZStd::shared_ptr<FbxAnimLayerWrapper>& layer, CurveNodeComponent component) const;

            virtual int GetChildCount() const;
            virtual const std::shared_ptr<FbxNodeWrapper> GetChild(int childIndex) const;

            virtual bool IsAnimated() const;

        protected:
            FbxNodeWrapper() = default;

            FbxNode* m_fbxNode;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
