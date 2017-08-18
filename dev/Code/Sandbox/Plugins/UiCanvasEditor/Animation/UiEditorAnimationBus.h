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

#include <AzCore/EBus/EBus.h>
#include <LyShine/UiBase.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that the UI Editor Animation Window needs to implement
//! (e.g. UiAnimViewDialog)

class CUiAnimViewDialog;
class CUiAnimViewSequence;
class CUiAnimationContext;
struct IUiAnimationSystem;

class UiEditorAnimationInterface
    : public AZ::EBusTraits
{
public: // member functions

    virtual ~UiEditorAnimationInterface(){}

    //! Get the animation context for the UI animation window
    virtual CUiAnimationContext* GetAnimationContext() = 0;

    //! Get the active UI animation system, this is the animation system for the active canvas
    virtual IUiAnimationSystem* GetAnimationSystem() = 0;

    //! Get the active UI animation sequence in the UI Animation Window
    virtual CUiAnimViewSequence* GetCurrentSequence() = 0;

    //! Called when the active canvas in the UI Editor window changes so that the UI Editor
    //! animation window can update to show the correct sequences.
    virtual void ActiveCanvasChanged() = 0;

    //! Called when a canvas is loaded in the UI Editor window changes so that the UI Editor
    //! animation window can update to show the correct sequences
    virtual void CanvasLoaded() = 0;

    //! Called when a canvas is about to unloaded/destroyed in the UI Editor window so that
    //! the UI Editor animation window can update to show the correct sequences
    virtual void CanvasUnloading() = 0;

public: // static member functions

    static const char* GetUniqueName() { return "UiEditorAnimationInterface"; }
};

typedef AZ::EBus<UiEditorAnimationInterface> UiEditorAnimationBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Listener class that any UI Editor Animation class can implement to get notifications

class UiEditorAnimListenerInterface
    : public AZ::EBusTraits
{
public: // member functions

    virtual ~UiEditorAnimListenerInterface(){}

    //! Called when the active canvas in the UI Editor window changes.
    //! When this is called the UiEditorAnimationBus may be used to get the new active canvas
    virtual void OnActiveCanvasChanged() = 0;

    //! Called when a canvas is loaded in the UI Editor. OnActiveCanvasChanged will typically be
    //! called after this
    virtual void OnCanvasLoaded() = 0;

    //! Called when a canvas is about to unloaded/destroyed in the UI Editor window
    virtual void OnCanvasUnloading() = 0;

    //! Called when UI elements have been deleted from or re-added to the canvas
    //! This requires the sequences to be updated
    virtual void OnUiElementsDeletedOrReAdded() {};

public: // static member functions

    static const char* GetUniqueName() { return "UiEditorAnimListenerInterface"; }
};

typedef AZ::EBus<UiEditorAnimListenerInterface> UiEditorAnimListenerBus;
