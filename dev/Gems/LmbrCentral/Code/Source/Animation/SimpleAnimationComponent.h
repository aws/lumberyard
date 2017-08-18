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

#include <ICryAnimation.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>

#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>

#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Animation/SimpleAnimationComponentBus.h>

namespace LmbrCentral
{
    /*!
     * \class SimpleAnimator
     * \brief Provides animation facilities to both Editor Simple Animation Component and Simple Animation Component
     */
    class SimpleAnimator
        : public MeshComponentNotificationBus::Handler
        , public AZ::TickBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:

        SimpleAnimator()
            : m_activeAnimatedLayerManager(nullptr)
        {
        }

        /**
        * Activates the Animator to be used for a particular character Instance
        * @param characterInstance The character instance for the mesh assigned to the component controlling this Animator
        * @param entityId The Entity id of the Component this Animator is servicing
        */
        void Activate(ICharacterInstance* characterInstance, AZ::EntityId entityId);

        /**
        * Starts the indicated animation
        * @param animatedLayer An AnimatedLayer that indicates parameters for the animation to be started
        * @return A Result indicating whether or not the animation was successfully started
        */
        SimpleAnimationComponentRequests::Result StartAnimation(const AnimatedLayer& animatedLayer);

        /**
        * Starts an animation as configured by the indicated Animated Layer
        * @param animatedLayer A Set of AnimatedLayers that indicates parameters for the animation to be started (key'ed on the layer id)
        * @return A Result indicating whether or not the animation was successfully started
        */
        SimpleAnimationComponentRequests::Result StartAnimations(const  AnimatedLayer::AnimatedLayerSet& animatedLayer);

        /**
        * Stops animations on all Active animated layers
        * @param animatedLayerIds Bitset indicating all layers that have animations that should be stopped
        * @param blendOutTime Time that the animations take to blend out
        * @return A Result indicating whether the animations were stopped properly or not
        */
        SimpleAnimationComponentRequests::Result StopAnimations(const SimpleAnimationComponentRequests::LayerMask& animatedLayerIds, float blendOutTime);

        /**
        * Stops animations on the indicated layer
        * @param animatedLayerId Id of the layer on which animations are to be stopped
        * @param blendOutTime Time that the animations take to blend out
        * @return A Result indicating whether the animations were stopped properly or not
        */
        SimpleAnimationComponentRequests::Result StopAnimation(const AnimatedLayer::LayerId animatedLayerId, float blendOutTime);

        /**
        * Sets the playback speed for a layer.
        * @param animatedLayerId Id of the layer to change.
        * @param playbackSpeed (1.0 = normal speed) to set for the specified layer.
        * @return A Result indicating whether or not the speed was changed.
        */
        SimpleAnimationComponentRequests::Result SetPlaybackSpeed(AnimatedLayer::LayerId animatedLayerId, float playbackSpeed);

        /**
        * Sets the playback speed for a layer.
        * @param animatedLayerId Id of the layer to change.
        * @param weight [0..1] to set on the specified layer.
        * @return A Result indicating whether or not the speed was changed.
        */
        SimpleAnimationComponentRequests::Result SetPlaybackWeight(AnimatedLayer::LayerId layerId, float weight);

        /**
        * Stops all currently active animations
        * @return Result indicating whether animations were stopped properly or not
        */
        SimpleAnimationComponentRequests::Result StopAllAnimations();

        /**
        * Deactivates this animator, decouples from the character instance but not from the entity id
        * Disconnects from the Mesh component events bus
        */
        void Deactivate();

        //////////////////////////////////////////////////////////////////////////
        // MeshcomponentEvents Bus Handler implementation
        /**
        * Bus event received when the mesh attached to the mesh component on this entity is created or changes.
        */
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;

        /**
        * Bus event received when the mesh attached to the mesh component on this entity is destroyed
        */
        void OnMeshDestroyed() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////
        // Transform notification bus handler
        /// Called when the local transform of the entity has changed.
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;
        //////////////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        /*!
         * \class AnimatedLayerManager
         * \brief Maintains the set of Active animated layers, is responsible for actually activating animations on the character instance
         */
        class AnimatedLayerManager
        {
        public:

            friend void SimpleAnimator::OnTick(float, AZ::ScriptTimePoint);

            /**
            * Constructor for ActiveAnimatedLayers
            * @param characterInstance character instance for the mesh being managed
            * @param entityId The Entity id of the Animator this AnimatedLayerManager is servicing
            */
            AnimatedLayerManager(ICharacterInstance* characterInstance, AZ::EntityId entityId);

            /**
            * Adds an animated layer to the active set
            * @param animatedLayer The animated layer to be activated
            * @return A Result indicating whether the layer was activated or not
            */
            SimpleAnimationComponentRequests::Result ActivateAnimatedLayer(const AnimatedLayer& animatedLayer);

            /**
            * Deactivates the indicated Animated layer
            * @param layerId Layer id of layer to be deactivated
            * @param blendOutTime Time that the animations take to blend out
            * @return A Result indicating  whether the layer was deactivated or not
            */
            SimpleAnimationComponentRequests::Result DeactivateAnimatedLayer(AnimatedLayer::LayerId layerId,float blendOutTime);

            /**
            * Stops animations on all layers for this character
            * @return A result indicating whether the animations were stopped or not
            */
            SimpleAnimationComponentRequests::Result DeactivateAllAnimatedLayers();

            /**
            * @param layerId
            * @return true if the layer is active.
            */
            bool IsLayerActive(AnimatedLayer::LayerId layerId) const;

            /**
            * Retrieve associated character instance.
            */
            ICharacterInstance* GetCharacter() { return m_characterInstance; }

            ~AnimatedLayerManager();

        private:

            /**
            * Gets an active animated layer for the indicated layer id
            * @param layerId Layer id to be fetched
            * @return AnimatedLayer if one is active at the provided layer id, null otherwise
            */
            const AnimatedLayer* GetActiveAnimatedLayer(AnimatedLayer::LayerId layerId) const;

            //! The character instance for the mesh being Animated
            ICharacterInstance* m_characterInstance = nullptr;

            // The Entity id of the Component this Animator is servicing
            AZ::EntityId m_attachedEntityId;

            //! Stores the currently active animations
            AZStd::map<AnimatedLayer::LayerId, AnimatedLayer> m_activeAnimatedLayers;
        };
        //////////////////////////////////////////////////////////////////////////

    private:

        //! Manages and animates active Animated Layers
        std::unique_ptr<AnimatedLayerManager> m_activeAnimatedLayerManager = nullptr;

        // The Entity id of the Component this Animator is servicing
        AZ::EntityId m_attachedEntityId;

        //! Stores the location of this entity, is updated whenever the entity moves
        AZ::Transform m_currentEntityLocation;

        //! Character instance of the mesh being animated
        ICharacterInstance* m_meshCharacterInstance = nullptr;

        /*!
        * Updates the character instance when a new mesh gets attached to the mesh component
        * or the attached mesh is removed.
        * Is responsible for stopping all currently active animations on the old character instance
        * and updating the Animator to refer to a character instance attached to the new mesh.
        */
        void UpdateCharacterInstance();
    };

    class SimpleAnimationComponent
        : public AZ::Component
        , public SimpleAnimationComponentRequestBus::Handler
        , public MeshComponentNotificationBus::Handler
    {
    public:

        friend class EditorSimpleAnimationComponent;

        AZ_COMPONENT(SimpleAnimationComponent, SimpleAnimationComponentTypeId);
        SimpleAnimationComponent() = default;
        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentNotificationBus interface implementation
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void OnMeshDestroyed() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SimpleAnimationComponentRequestBus interface implementation
        SimpleAnimationComponentRequests::Result StartDefaultAnimations() override;
        SimpleAnimationComponentRequests::Result StartAnimation(const AnimatedLayer& animatedLayer) override;
        SimpleAnimationComponentRequests::Result StartAnimationSet(const AnimatedLayer::AnimatedLayerSet& animationSet) override;
        SimpleAnimationComponentRequests::Result StopAllAnimations() override;
        SimpleAnimationComponentRequests::Result StopAnimationsOnLayer(AnimatedLayer::LayerId layerId, float blendOutTime) override;
        SimpleAnimationComponentRequests::Result SetPlaybackSpeed(AnimatedLayer::LayerId layerId, float playbackSpeed) override;
        SimpleAnimationComponentRequests::Result SetPlaybackWeight(AnimatedLayer::LayerId layerId, float weight) override;
        SimpleAnimationComponentRequests::Result StopAnimationsOnLayers(LayerMask layerIds, float blendOutTime) override;
        //////////////////////////////////////////////////////////////////////////

    protected:

        void LinkToCharacterInstance(ICharacterInstance* characterInstance);

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
        SimpleAnimationComponent(const SimpleAnimationComponent&) = delete;
        /*
        * Reflects default animation settings for multiple layers as configured in the editor
        * This will be EMPTY post load , Only used for reflection
        */
        AZStd::list<AnimatedLayer> m_defaultAnimationSettings;

        //! Set that stores default animation settings for multiple layers as configured in the editor
        AnimatedLayer::AnimatedLayerSet m_defaultAnimLayerSet;

        //! Provides animation services to the component
        SimpleAnimator m_animator;

        //! Tracks whether the mesh asset attached to the skinned mesh is ready for animation.  We will queue animation requests until it is
        bool m_isMeshAssetReady = false;

        //! Stores the animations requested while the skinned mesh asset was unavailable
        AZStd::vector<AnimatedLayer> m_animationsQueuedBeforeAssetReady;
    };
} // namespace LmbrCentral
