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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/std/containers/bitset.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace LmbrCentral
{
    /*!
    * An animated layer contains all data that is required to configure a layer with an animation and play it
    */
    class AnimatedLayer
    {
    public:

        AZ_TYPE_INFO(AnimatedLayer, "{147EAB48-2D6E-41CF-8414-CEABF3F1E59B}")
        AZ_CLASS_ALLOCATOR(AnimatedLayer, AZ::SystemAllocator, 0);

        using LayerId = AZ::s32;

        //! Indicates the maximum number of active animated layers that could be present
        static const AZ::u32 s_maxActiveAnimatedLayers = 16;
        static const LayerId s_invalidLayerId = static_cast<LayerId>(-1);

        LayerId GetLayerId() const
        {
            return m_layerId;
        }

        const AZStd::string& GetAnimationName() const
        {
            return m_animationName;
        }

        bool IsLooping() const
        {
            return m_looping;
        }

        float GetPlaybackSpeed() const
        {
            return m_playbackSpeed;
        }

        float GetTransitionTime() const
        {
            return m_transitionTime;
        }
        
        bool GetAnimationDrivenRootMotion() const
        {
            return m_animDrivenRootMotion;
        }

        bool InterruptIfAlreadyPlaying() const
        {
            return m_interruptIfAlreadyPlaying;
        }

        float GetLayerWeight() const
        {
            return m_layerWeight;
        }

        AnimatedLayer(const AZStd::string& animationName, LayerId layerId = s_invalidLayerId, bool looping = false, float playbackSpeed = 1.0f, float transitionTime = 0.2f, bool interruptIfAlreadyPlaying = false, float layerWeight = 1.0f, bool animDrivenRootMotion = false);
        AnimatedLayer(AZ::ScriptDataContext&);
        AnimatedLayer() {}

        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        /*!
        * Hashing and comparison functions for unordered sets of animated layers
        */
        struct HashAnimatedLayerKey
        {
            AZStd::size_t operator()(const AnimatedLayer& layer) const
            {
                return AZStd::hash<AnimatedLayer::LayerId>()(layer.GetLayerId());
            }
        };

        struct HashAnimatedKeyComparator
        {
            bool operator()(const AnimatedLayer& left, const AnimatedLayer& right) const
            {
                return left.GetLayerId() == right.GetLayerId();
            }
        };

        using AnimatedLayerSet = AZStd::unordered_set<AnimatedLayer, HashAnimatedLayerKey, HashAnimatedKeyComparator>;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        using AnimationNameList = AZStd::vector<AZStd::string>;

        void SetEntityId(AZ::EntityId entityId)
        {
            m_entityId = entityId;
        }

        //////////////////////////////////////////////////////////////////////////

        /**
        * Compares Animated Layers
        * @param otherAnimatedLayer The animated layer you want to compare to
        * @return indicates whether the layers are the same or not
        */
        bool operator==(const AnimatedLayer& otherAnimatedLayer) const
        {
            if (m_animationName.compare(otherAnimatedLayer.GetAnimationName()) != 0)
            {
                return false;
            }
            else if (m_looping != otherAnimatedLayer.IsLooping())
            {
                return false;
            }
            else if (AZ::IsClose(m_playbackSpeed,otherAnimatedLayer.GetPlaybackSpeed(),std::numeric_limits<float>::epsilon()))
            {
                return false;
            }
            else if (AZ::IsClose(m_transitionTime,otherAnimatedLayer.GetTransitionTime(),std::numeric_limits<float>::epsilon()))
            {
                return false;
            }
            else if (m_layerId != otherAnimatedLayer.GetLayerId())
            {
                return false;
            }
            else if (AZ::IsClose(m_layerWeight,otherAnimatedLayer.GetLayerWeight(),std::numeric_limits<float>::epsilon()))
            {
                return false;
            }
            return true;
        }

    private:

        //! Indicates the id of this layer [0 s_maxActiveAnimatedLayers). A negative value indicates an invalid layer
        LayerId m_layerId = 0;

        //! Indicates the name (alias as defined in the charparams file) of the animation that is to be played on this layer
        AZStd::string m_animationName;

        /*!
        * Indicates whether the animation should loop after its finished playing or not.
        * If set, this animation will continue to loop on this layer until stopped.
        */
        bool m_looping = true;

        //! Indicates the speed at which this animation will playback
        float m_playbackSpeed = 1.0f;

        //! Indicates the time it takes to transition from this animation to the next
        float m_transitionTime = 0.15f;

        //! Indicates if a request to play an already-playing animation should interrupt itself
        bool m_interruptIfAlreadyPlaying = false;

        //! Indicates the weight of animations played on this layer
        float m_layerWeight = 1.0f;

        //! Enables animation-driven root motion during playback of this animation
        bool m_animDrivenRootMotion = false;

        //! Animations that are available to be used with this animation layer
        AZStd::vector<AZStd::string> GetAvailableAnims();

        //! Entity Id used to fetch available anims for the property grid
        //! NOTE : Only used in the Editor
        AZ::EntityId m_entityId;
    };


    class AnimationInformationEventGroup
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = AZ::EntityId;

        virtual const AnimatedLayer::AnimationNameList* GetAvailableAnimationsList() = 0;
    };

    using AnimationInformationBus = AZ::EBus<AnimationInformationEventGroup>;

    /*!
    * Services provided by the simple animation component
    * The Simple Animation component provides basic animation functionality for the entity. If the entity
    * has a mesh component with a skinned mesh attached (.chr or .cdf), the Simple Animation component
    * will provide a list of all valid animations as specified in the associated .chrparams file.The
    * Simple Animation component does not provide interaction with Mannequin and should be used for
    * light-weight environment or background animation.
    */
    class SimpleAnimationComponentRequests
        : public AZ::ComponentBus
    {
    public:
        enum class Result
        {
            Success,
            SuccessWithErrors,
            Failure,
            AnimationAlreadyPlaying,
            AnimationNotFound,
            NoAnimationPlayingOnLayer
        };

        //! Helper type used to indicate a set of layers to various parts of the anim component
        using LayerMask = AZStd::bitset<AnimatedLayer::s_maxActiveAnimatedLayers>;

        /*!
        * Plays the default animations along with default looping and speed parameters that were set up as a part of this component
        * The component allows for multiple layers to be set up with defaults and this method allows the playback of all those animations
        * in one shot
        * @return a Result indicating whether the animations were started successfully or not
        */
        virtual Result StartDefaultAnimations() { return Result::Failure; }

        /*!
        * Plays the animation with the specified name
        * @param name The name of the animation to play
        * @param layerId The layer in which to play the animation
        * @return A Result indicating whether the animation was started or not
        */
        virtual Result StartAnimationByName(const char* name, AnimatedLayer::LayerId layerId)
        {
            AnimatedLayer layer(name, layerId);
            return StartAnimation(layer);
        }

        /*!
        * Plays the animation as configured by the animatedLayer
        * @param animatedLayer A Layer configured with the animation that is to be played on it
        * @return A Result indicating whether the animation was started or not
        */
        virtual Result StartAnimation(const AnimatedLayer& animatedLayer) { return Result::Failure; }

        /*!
        * Plays a set of animations as configured by each "AnimatedLayer" in the animationSet
        * @param animationSet An AnimatedLayer::AnimatedLayerSet containing animations to be kicked off simultaneously
        * @return A Result indicating whether the set of animations was started or not
        */
        virtual Result StartAnimationSet(const AnimatedLayer::AnimatedLayerSet& animationSet) { return Result::Failure; }

        /*!
        * Stops all animations that are being played on all layers
        * @return A Result indicating whether all animations were stopped or not
        */
        virtual Result StopAllAnimations() { return Result::Failure; }

        /*!
        * Stops the animation currently playing on the indicated layer
        * @param layerId Identifier for the layer that is to stop its animation (0 - AnimatedLayer::s_maxActiveAnimatedLayers)
        * @param blendOutTime Time that the animations take to blend out
        * @return A Result indicating whether the animation on the indicated layer was stopped or not
        */
        virtual Result StopAnimationsOnLayer(AnimatedLayer::LayerId layerId, float blendOutTime) { return Result::Failure; }

        /*!
        * Changes the playback speed for a particular layer.
        * @param layerId Identifier for the layer whose speed should be changed.
        * @return A Result indicating whether the animation on the indicated layer was updated or not. A failure likely indicated
        *         that no animation is playing on the specified layer.
        */
        virtual Result SetPlaybackSpeed(AnimatedLayer::LayerId layerId, float playbackSpeed) { return Result::Failure; }

        /*!
        * Changes the weight for the specified layer.
        * @param layerId Identifier for the layer whose weight should be changed.
        * @return A Result indicating whether the animation on the indicated layer was updated or not. A failure likely indicated
        *         that no animation is playing on the specified layer.
        */
        virtual Result SetPlaybackWeight(AnimatedLayer::LayerId layerId, float weight) { return Result::Failure; }

        /*!
        * Stops animations currently playing on all indicated layers
        * @param layerIds A bitset indicating layers that need to stop animating
        * @param blendOutTime Time that the animations take to blend out
        * @return A Result indicating whether the animation on the indicated layer was stopped or not
        */
        virtual Result StopAnimationsOnLayers(LayerMask layerIds, float blendOutTime) { return Result::Failure; }
    };

    // Bus to service the Simple animation component event group
    using SimpleAnimationComponentRequestBus = AZ::EBus<SimpleAnimationComponentRequests>;


    /*!
    *  Notifications sent by the simple animation component
    */
    class SimpleAnimationComponentNotifications
        : public AZ::ComponentBus
    {
    public:

        /*!
        * Informs all listeners about an animation being started on the indicated layer
        * @param animatedLayer AnimatedLayer indicating the animation and the parameters that were used to start the animation
        */
        virtual void OnAnimationStarted(const AnimatedLayer& animatedLayer) {}

        /*!
        * Informs all listeners about an animation being stopped on the indicated layer
        * @param animatedLayer AnimatedLayer indicating the animation and the parameters that identify the animation that was stopped
        */
        virtual void OnAnimationStopped(const AnimatedLayer::LayerId animatedLayer) {}
    };

    // Bus to service the Simple animation component notification event group
    using SimpleAnimationComponentNotificationBus = AZ::EBus<SimpleAnimationComponentNotifications>;

    // define SimpleAnimationComponentTypeId GUID here so we can reference it outside of LmbrCentral
#define SimpleAnimationComponentTypeId "{FBB470EF-2288-4B62-B41F-D830DD4C5B98}"
#define EditorSimpleAnimationComponentTypeId "{D61BB108-5176-4FF8-B692-CC3EB5865F79}"
    //////////////////////////////////////////////////////////////////////////
    // AnimatedLayer Implementation

    inline AnimatedLayer::AnimatedLayer(const AZStd::string& animationName, LayerId layerId, bool looping, float playbackSpeed, float transitionTime, bool interruptIfAlreadyPlaying, float layerWeight, bool animDrivenRootMotion)
        : m_animationName(animationName)
        , m_looping(looping)
        , m_playbackSpeed(playbackSpeed)
        , m_transitionTime(transitionTime)
        , m_interruptIfAlreadyPlaying(interruptIfAlreadyPlaying)
        , m_layerWeight(layerWeight)
        , m_animDrivenRootMotion(animDrivenRootMotion)
    {
        if (layerId < 0 || layerId >= s_maxActiveAnimatedLayers)
        {
            m_layerId = s_invalidLayerId;
        }
        else
        {
            m_layerId = layerId;
        }
    }

    inline AnimatedLayer::AnimatedLayer(AZ::ScriptDataContext& context)
        : m_animationName("INVALID_ANIM_NAME")
        , m_looping(false)
        , m_playbackSpeed(1.f)
        , m_transitionTime(0.2f)
        , m_layerId(0)
        , m_interruptIfAlreadyPlaying(false)
        , m_layerWeight(1.0f)
        , m_animDrivenRootMotion(false)
    {
        // unpack parameters into a waterfall of default constructor arguments
        int numberOfArguments = context.GetNumArguments();
        switch (numberOfArguments)
        {
            case 8:
            {
                AZ_Warning("Script", context.IsBoolean(7), "Invalid Parameter 8, expects (animDrivenRootMotion:boolean)");
                context.ReadArg(7, m_animDrivenRootMotion);
            }
            case 7:
            {
                AZ_Warning("Script", context.IsNumber(6), "Invalid Parameter 7, expects (layerWeight:float)");
                context.ReadArg(6, m_layerWeight);
            }
            case 6:
            {
                AZ_Warning("Script", context.IsBoolean(5), "Invalid Parameter 6, expects (ignoreIfAlreadyPlaying:boolean)");
                context.ReadArg(5, m_interruptIfAlreadyPlaying);
            }
            case 5:
            {
                AZ_Warning("Script", context.IsNumber(4), "Invalid Parameter 5, expects (transitionTime:float)");
                context.ReadArg(4, m_transitionTime);
            }
            case 4:
            {
                AZ_Warning("Script", context.IsNumber(3), "Invalid Parameter 4, expects (playbackSpeed:float)");
                context.ReadArg(3, m_playbackSpeed);
            }
            case 3:
            {
                AZ_Warning("Script", context.IsBoolean(2), "Invalid Parameter 3, expects (looping:bool)");
                context.ReadArg(2, m_looping);
            }
            case 2:
            {
                AZ_Warning("Script", context.IsNumber(1), "Invalid Parameter 2, expects (layerId:number)");
                context.ReadArg(1, m_layerId);
                if (m_layerId < 0 || m_layerId >= s_maxActiveAnimatedLayers)
                {
                    AZ_Warning("Script", false, "Invalid Parameter 2, expects (layerId:number) between 0 and %d", s_maxActiveAnimatedLayers);
                    m_layerId = s_invalidLayerId;
                }
            }
            case 1:
            {
                AZ_Warning("Script", context.IsString(0), "Invalid Parameter 1, expects (animationName:String)");
                context.ReadArg(0, m_animationName);
            }
            case 0:
            {
                return;
            }
            default:
            {
                AZ_Warning("Script", false, "Invalid number of arguments to AnimatedLayer constructor expects (animationName,layerId,looping(optional),playbackSpeed(optional),transitionTime(optional),ignoreIfAlreadyPlaying(optional),layerWeight(optional))");
                return;
            }
        }
    }
} // namespace LmbrCentral
