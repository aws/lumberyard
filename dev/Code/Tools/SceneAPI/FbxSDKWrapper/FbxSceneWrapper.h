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

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FBX SDK Wrapper
// provides isolation between AUTODESK FBX SDK and FBX Serializer
// provides necessary APIs for FBX Serializer
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxAnimStackWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxAxisSystemWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxSystemUnitWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxSceneWrapper
        {
        public:
            FbxSceneWrapper();
            FbxSceneWrapper(FbxScene* fbxScene);
            virtual ~FbxSceneWrapper();

            virtual bool LoadSceneFromFile(const char* fileName);
            virtual bool LoadSceneFromFile(const AZStd::string& fileName);

            virtual AZStd::shared_ptr<FbxSystemUnitWrapper> GetSystemUnit() const;
            virtual AZStd::shared_ptr<FbxAxisSystemWrapper> GetAxisSystem() const;
            virtual FbxTimeWrapper GetTimeLineDefaultDuration() const;
            virtual const char* GetLastSavedApplicationName() const;
            virtual const char* GetLastSavedApplicationVersion() const;
            virtual const std::shared_ptr<FbxNodeWrapper> GetRootNode() const;
            virtual std::shared_ptr<FbxNodeWrapper> GetRootNode();
            virtual int GetAnimationStackCount() const;
            virtual const std::shared_ptr<FbxAnimStackWrapper> GetAnimationStackAt(int index) const;
            virtual void Clear();

            virtual FbxScene* GetFbxScene() const;

            static const char* s_defaultSceneName;

        protected:
            FbxScene* m_fbxScene;
            FbxManager* m_fbxManager;
            FbxImporter* m_fbxImporter;
            FbxIOSettings* m_fbxIOSettings;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
