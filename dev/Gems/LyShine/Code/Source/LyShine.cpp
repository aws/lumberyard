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
#include "StdAfx.h"

#include "LyShine.h"

#include "Draw2d.h"

#include "UiCanvasComponent.h"
#include "UiCanvasManager.h"
#include "LyShineDebug.h"
#include "UiElementComponent.h"
#include "UiTransform2dComponent.h"
#include "UiImageComponent.h"
#include "UiTextComponent.h"
#include "UiButtonComponent.h"
#include "UiCheckboxComponent.h"
#include "UiSliderComponent.h"
#include "UiTextInputComponent.h"
#include "UiScrollBarComponent.h"
#include "UiScrollBoxComponent.h"
#include "UiFaderComponent.h"
#include "UiMaskComponent.h"
#include "UiLayoutColumnComponent.h"
#include "UiLayoutRowComponent.h"
#include "UiLayoutGridComponent.h"
#include "UiTooltipComponent.h"
#include "UiTooltipDisplayComponent.h"
#include "UiDynamicLayoutComponent.h"
#include "UiDynamicScrollBoxComponent.h"
#include "Script/UiCanvasNotificationLuaBus.h"
#include "Script/UiCanvasLuaBus.h"
#include "Script/UiElementLuaBus.h"
#include "Sprite.h"
#include "UiSerialize.h"
#include "UiRenderer.h"

#include <LyShine/Bus/UiDraggableBus.h>
#include <LyShine/Bus/UiDropTargetBus.h>

#if defined(LYSHINE_INTERNAL_UNIT_TEST)
#include "TextMarkup.h"
#endif

#include <AzCore/Math/Crc.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

#include "Animation/UiAnimationSystem.h"
#include "World/UiCanvasAssetRefComponent.h"
#include "World/UiCanvasProxyRefComponent.h"
#include "World/UiCanvasOnMeshComponent.h"


//! \brief Simple utility class for LyShine funtionality in Lua.
//!
//! Functionality unrelated to UI, such as showing the mouse cursor, should
//! eventually be moved into other modules (for example, mouse cursor functionality
//! should be moved to input, which matches more closely how FG modules are
//! organized).
class LyShineLua
{
public:
    static void ShowMouseCursor(bool visible)
    {
        static bool sShowCursor = false;

        if (visible)
        {
            if (!sShowCursor)
            {
                sShowCursor = true;
                gEnv->pHardwareMouse->IncrementCounter();
            }
        }
        else
        {
            if (sShowCursor)
            {
                sShowCursor = false;
                gEnv->pHardwareMouse->DecrementCounter();
            }
        }
    }
};

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(LyShineLua, "{2570D3B3-2D18-4DB1-A0DE-E017A2F491D1}");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CLyShine::CLyShine(ISystem* system)
    : m_system(system)
    , m_draw2d(new CDraw2d)
    , m_uiRenderer(new UiRenderer)
    , m_uiCanvasManager(new UiCanvasManager)
{
    // Reflect the Deprecated Lua buses using the behavior context.
    // Thuis support will be removed at some point
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        EBUS_EVENT_RESULT(behaviorContext, AZ::ComponentApplicationBus, GetBehaviorContext);
        if (behaviorContext)
        {
            behaviorContext->Class<LyShineLua>("LyShineLua")
                ->Attribute(AZ_CRC("ScriptCanvasIgnore", 0x67a88f02), true)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Method("ShowMouseCursor", &LyShineLua::ShowMouseCursor)
            ;

            UiCanvasNotificationLuaProxy::Reflect(behaviorContext);
            UiCanvasLuaProxy::Reflect(behaviorContext);
            UiElementLuaProxy::Reflect(behaviorContext);
        }
    }

    CSprite::Initialize();
    LyShineDebug::Initialize();
    UiElementComponent::Initialize();
    UiCanvasComponent::Initialize();

    UiAnimationSystem::StaticInitialize();

    m_system->GetIRenderer()->AddRenderDebugListener(this);

    m_system->GetIInput()->AddEventListener(this);

    // These are internal Amazon components, so register them so that we can send back their names to our metrics collection
    // IF YOU ARE A THIRDPARTY WRITING A GEM, DO NOT REGISTER YOUR COMPONENTS WITH EditorMetricsComponentRegistrationBus
    // This is internal Amazon code, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
    AZStd::vector<AZ::Uuid> componentUuidsForMetricsCollection
    {
        azrtti_typeid<UiCanvasAssetRefComponent>(),
        azrtti_typeid<UiCanvasProxyRefComponent>(),
        azrtti_typeid<UiCanvasOnMeshComponent>(),
        azrtti_typeid<UiCanvasComponent>(),
        azrtti_typeid<UiElementComponent>(),
        azrtti_typeid<UiTransform2dComponent>(),
        azrtti_typeid<UiImageComponent>(),
        azrtti_typeid<UiTextComponent>(),
        azrtti_typeid<UiButtonComponent>(),
        azrtti_typeid<UiCheckboxComponent>(),
        azrtti_typeid<UiSliderComponent>(),
        azrtti_typeid<UiTextInputComponent>(),
        azrtti_typeid<UiScrollBarComponent>(),
        azrtti_typeid<UiScrollBoxComponent>(),
        azrtti_typeid<UiFaderComponent>(),
        azrtti_typeid<UiMaskComponent>(),
        azrtti_typeid<UiLayoutColumnComponent>(),
        azrtti_typeid<UiLayoutRowComponent>(),
        azrtti_typeid<UiLayoutGridComponent>(),
    };
    EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, componentUuidsForMetricsCollection);



#if defined(LYSHINE_INTERNAL_UNIT_TEST)
    TextMarkup::UnitTest();
    UiTextComponent::UnitTest(this);
    UiTextComponent::UnitTestLocalization(this);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CLyShine::~CLyShine()
{
    if (m_system->GetIInput())
    {
        m_system->GetIInput()->RemoveEventListener(this);
    }

    if (m_system->GetIRenderer())
    {
        m_system->GetIRenderer()->RemoveRenderDebugListener(this);
    }

    UiCanvasComponent::Shutdown();

    // must be done after UiCanvasComponent::Shutdown
    CSprite::Shutdown();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::Release()
{
    delete this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IDraw2d* CLyShine::GetDraw2d()
{
    return m_draw2d.get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IUiRenderer* CLyShine::GetUiRenderer()
{
    return m_uiRenderer.get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::CreateCanvas()
{
    return m_uiCanvasManager->CreateCanvas();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::LoadCanvas(const string& assetIdPathname)
{
    return m_uiCanvasManager->LoadCanvas(assetIdPathname.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::CreateCanvasInEditor(UiEntityContext* entityContext)
{
    return m_uiCanvasManager->CreateCanvasInEditor(entityContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::LoadCanvasInEditor(const string& assetIdPathname, const string& sourceAssetPathname, UiEntityContext* entityContext)
{
    return m_uiCanvasManager->LoadCanvasInEditor(assetIdPathname, sourceAssetPathname, entityContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::ReloadCanvasFromXml(const AZStd::string& xmlString, UiEntityContext* entityContext)
{
    return m_uiCanvasManager->ReloadCanvasFromXml(xmlString, entityContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::FindCanvasById(LyShine::CanvasId id)
{
    return m_uiCanvasManager->FindCanvasById(id);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId CLyShine::FindLoadedCanvasByPathName(const string& assetIdPathname)
{
    return m_uiCanvasManager->FindLoadedCanvasByPathName(assetIdPathname.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::ReleaseCanvas(AZ::EntityId canvas, bool forEditor)
{
    m_uiCanvasManager->ReleaseCanvas(canvas, forEditor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ISprite* CLyShine::LoadSprite(const string& pathname)
{
    return CSprite::LoadSprite(pathname);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ISprite* CLyShine::CreateSprite(const string& renderTargetName)
{
    return CSprite::CreateSprite(renderTargetName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::PostInit()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::SetViewportSize(AZ::Vector2 viewportSize)
{
    // Pass the viewport size to UiCanvasComponents
    m_uiCanvasManager->SetTargetSizeForLoadedCanvases(viewportSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::Update(float deltaTimeInSeconds)
{
    // Update all the canvases loaded in game
    m_uiCanvasManager->UpdateLoadedCanvases(deltaTimeInSeconds);

    // Execute events that have been queued during the canvas update
    ExecuteQueuedEvents();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::Render()
{
    if (!gEnv || !gEnv->pRenderer || gEnv->pRenderer->GetRenderType() == ERenderType::eRT_Null)
    {
        // if the renderer is not initialized or it is the null renderer (e.g. running as a server)
        // then do nothing
        return;
    }
        
    // we are rendering at the end of the frame, after tone mapping, so we should be writing sRGB values
    gEnv->pRenderer->SetSrgbWrite(true);

    // Render all the canvases loaded in game
    m_uiCanvasManager->RenderLoadedCanvases();

    m_draw2d->RenderDeferredPrimitives();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::ExecuteQueuedEvents()
{
    // Execute events that have been queued during the canvas update
    UiFaderNotificationBus::ExecuteQueuedEvents();
    UiAnimationNotificationBus::ExecuteQueuedEvents();

    // Execute events that have been queued during the input event handler
    UiDraggableNotificationBus::ExecuteQueuedEvents();  // draggable must be done before drop target
    UiDropTargetNotificationBus::ExecuteQueuedEvents();
    UiCanvasNotificationBus::ExecuteQueuedEvents();
    UiButtonNotificationBus::ExecuteQueuedEvents();
    UiInteractableNotificationBus::ExecuteQueuedEvents();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::Reset()
{
    // This is called when the game is terminated.

    // Reset the debug module - this should be called before DestroyLoadedCanvases since it
    // tracks the loaded debug canvas
    LyShineDebug::Reset();

    // Delete all the canvases in m_canvases map that are not open in the editor.
    m_uiCanvasManager->DestroyLoadedCanvases(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::OnLevelUnload()
{
    // This is called when a level is unloaded or a new level is initialized

    // Reset the debug module - this should be called before DestroyLoadedCanvases since it
    // tracks the loaded debug canvas
    LyShineDebug::Reset();

    // Delete all the canvases in m_canvases map that a not loaded in the editor and are not
    // marked to be kept between levels.
    m_uiCanvasManager->DestroyLoadedCanvases(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CLyShine::OnDebugDraw()
{
    LyShineDebug::RenderDebug();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
bool CLyShine::OnInputEvent(const SInputEvent& event)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

    // disable UI inputs when console is open except for a primary release
    // if we ignore the primary release when there is an active interactable then it will miss its release
    // which leaves it in a bad state. E.g. a drag operation will be left in flight and not properly
    // terminated.
    if (gEnv->pConsole->GetStatus())
    {
        bool isPrimaryRelease = false;
        if (event.keyId == eKI_Mouse1 || event.keyId == eKI_Touch0)
        {
            if (event.state == eIS_Released)
            {
                isPrimaryRelease = true;
            }
        }

        if (!isPrimaryRelease)
        {
            return false;
        }
    }

    // currently I can't find a way to get the unicode events when running in the editor,
    // so handle the keyboard events though this path when in the editor
    //if (event.state != eIS_UI)
    if (event.state != eIS_UI || (gEnv && gEnv->IsEditor()))
    {
        bool result = m_uiCanvasManager->HandleInputEventForLoadedCanvases(event);
        if (result)
        {
            // Execute events that have been queued during the input event handler
            ExecuteQueuedEvents();
        }
        
        return result;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CLyShine::OnInputEventUI(const SUnicodeEvent& event)
{
    // NOTE: this function is not yet implemented to do anything. It is a little confusing that
    // the input system distinguishes between regular events and UI events and which ones
    // we should be responding to. I think we may need to use this one for text input for text
    // entry fields.
    FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

    if (gEnv->pConsole->GetStatus()) // disable UI inputs when console is open
    {
        return false;
    }

    // keyboard events come through this path
    bool result = m_uiCanvasManager->HandleKeyboardEventForLoadedCanvases(event);
    if (result)
    {
        // Execute events that have been queued during the input event handler
        ExecuteQueuedEvents();
    }

    return false;
}
