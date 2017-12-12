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
#ifdef MOTIONCANVAS_GEM_ENABLED

#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <AzCore/Memory/Memory.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Behaviors
        {
            class EFXMotionGroupBehavior
                : public Events::ManifestMetaInfoBus::Handler
                , public Events::AssetImportRequestBus::Handler
            {
            public:
                AZ_CLASS_ALLOCATOR_DECL

                EFXMotionGroupBehavior();
                ~EFXMotionGroupBehavior() override;

                // ManifestMetaInfo
                void GetCategoryAssignments(CategoryRegistrationList& categories, const Containers::Scene& scene) override;
                void InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target) override;
                
                // AssetImportRequest
                Events::ProcessingResult UpdateManifest(Containers::Scene& scene, ManifestAction action,
                    RequestingApplication requester) override;
                
            private:
                Events::ProcessingResult BuildDefault(Containers::Scene& scene) const;
                Events::ProcessingResult UpdateEFXMotionGroupBehaviors(Containers::Scene& scene) const;

                bool SceneHasMotionGroup(const Containers::Scene& scene) const;

                static const int s_preferredTabOrder;
            };
        } // Behaviors
    } // SceneAPI
} // AZ
#endif //MOTIONCANVAS_GEM_ENABLED