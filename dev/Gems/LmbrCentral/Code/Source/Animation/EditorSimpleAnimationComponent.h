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

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>

#include "SimpleAnimationComponent.h"

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace LmbrCentral
{
    class EditorSimpleAnimationComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private MeshComponentNotificationBus::Handler
        , private AnimationInformationBus::Handler
        , private IAnimationSetListener
    {
    public:

        AZ_COMPONENT(EditorSimpleAnimationComponent,
            EditorSimpleAnimationComponentTypeId,               // defined in SimpleAnimationComponentBus.h
            AzToolsFramework::Components::EditorComponentBase);
        EditorSimpleAnimationComponent() = default;
        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentNotificationBus
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;

        void OnMeshDestroyed() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AnimationInformationBus implementation
        const AnimatedLayer::AnimationNameList* GetAvailableAnimationsList() override;
        //////////////////////////////////////////////////////////////////////////

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        //////////////////////////////////////////////////////////////////////////
        // IAnimationSetListener implementation
        void OnAnimationSetReload() override;
        void OnAnimationSetAddAnimation(const char* animationPath, const char* animationName) override;
        void OnAnimationSetRemovedAnimation(const char* animationPath, const char* animationName) override;

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("AnimationService", 0x553f5760));
            provided.push_back(AZ_CRC("SimpleAnimationService", 0x8710444f));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("SkinnedMeshService", 0xac7cea96));
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("AnimationService", 0x553f5760));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        EditorSimpleAnimationComponent(const EditorSimpleAnimationComponent&) = delete;
        /*
        * Called by the property grid whenever the layers attached to this component change
        * Note : Only called when layers are added using the UI
        */
        void OnLayersChanged();

        void UpdateCharacterInstance();

        /*
        * Fetches all animations on the character instance that is attached to the mesh on this entity
        */
        void FetchCharacterAnimations(ICharacterInstance* characterInstance);

        //! Stores default animation settings for multiple layers as configured in the editor
        AZStd::list<AnimatedLayer> m_defaultAnimationSettings;

        //! Stores the list of animations that are available on the mesh that is on the entity that this component is attached to
        AnimatedLayer::AnimationNameList m_availableAnimations;

        //! Provides animation services to the component
        SimpleAnimator m_animator;
    };
} // namespace LmbrCentral
