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

#define AZ_LOADSCREENCOMPONENT_ENABLED      (1)

#if AZ_LOADSCREENCOMPONENT_ENABLED

#include <AzCore/Component/ComponentBus.h>

class LoadScreenInterface
    : public AZ::ComponentBus
{
public:
    using MutexType = AZStd::recursive_mutex;

    //! Invoked when the load screen should be updated and rendered. Single threaded loading only.
    virtual void UpdateAndRender() = 0;

    //! Invoked when the game load screen should become visible.
    virtual void GameStart() = 0;

    //! Invoked when the level load screen should become visible.
    virtual void LevelStart() = 0;

    //! Invoked when the load screen should be paused.
    virtual void Pause() = 0;

    //! Invoked when the load screen should be resumed.
    virtual void Resume() = 0;

    //! Invoked when the load screen should be stopped.
    virtual void Stop() = 0;

    //! Invoked to find out if loading screen is playing.
    virtual bool IsPlaying() = 0;
};
using LoadScreenBus = AZ::EBus<LoadScreenInterface>;

//! Interface for notifying load screen providers that specific load events are happening.
//! This is meant to notify systems to connect/disconnect to the LoadScreenUpdateNotificationBus if necessary.
struct LoadScreenNotifications
    : public AZ::EBusTraits
{
    //! Invoked when the game/engine loading starts. Returns true if any provider handles this.
    virtual bool NotifyGameLoadStart(bool usingLoadingThread) = 0;

    //! Invoked when level loading starts. Returns true if any provider handles this.
    virtual bool NotifyLevelLoadStart(bool usingLoadingThread) = 0;

    //! Invoked when loading finishes.
    virtual void NotifyLoadEnd() = 0;
};
using LoadScreenNotificationBus = AZ::EBus<LoadScreenNotifications>;

//! Interface for triggering load screen updates and renders. Has different methods for single threaded vs multi threaded.
//! This is a separate bus from the LoadScreenNotificationBus to avoid threading issues and to allow implementers to conditionally attach
//! from inside LoadScreenNotificationBus::NotifyGameLoadStart/NotifyLevelLoadStart
struct LoadScreenUpdateNotifications
    : public AZ::EBusTraits
{
    /**
     * Values to help you set when a particular handler is notified to update/render.
    */
    enum LoadScreenOrder : int
    {
        ORDER_FIRST = 0,       ///< First position in the render handler order.

        ORDER_VIDEO = 300,     ///< Suggested render handler position for video components.

        ORDER_DEFAULT = 500,     ///< Default render handler position.

        ORDER_UI = 1000,    ///< Suggested render handler position for UI components.

        ORDER_LAST = 100000,  ///< Last position in the render handler order.
    };

    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;

    struct BusHandlerOrderCompare
    {
        AZ_FORCE_INLINE bool operator()(LoadScreenUpdateNotifications* left, LoadScreenUpdateNotifications* right) const
        {
            return left->GetRenderOrder() < right->GetRenderOrder();
        }
    };

    //! Invoked when the load screen should be updated and rendered. Single threaded loading only.
    virtual void UpdateAndRender(float deltaTimeInSeconds) = 0;

    //! Invoked when the load screen should be updated. Multi-threaded loading only.
    virtual void LoadThreadUpdate(float deltaTimeInSeconds) = 0;

    //! Invoked when the load screen should be updated. Multi-threaded loading only.
    virtual void LoadThreadRender() = 0;

    //! Returns a value used to decide when this should be rendered compared to other handlers. Lower numbers are rendered first.
    virtual int GetRenderOrder() const { return ORDER_DEFAULT; }
};
using LoadScreenUpdateNotificationBus = AZ::EBus<LoadScreenUpdateNotifications>;

#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
