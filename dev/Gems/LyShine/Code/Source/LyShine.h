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

#include <IRenderer.h>
#include <LyShine/ILyShine.h>
#include <LyShine/Bus/UiCursorBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Input/Events/InputTextEventListener.h>

#if defined(_DEBUG)
//#define LYSHINE_INTERNAL_UNIT_TEST
#endif

#if !defined(_DEBUG) && defined(LYSHINE_INTERNAL_UNIT_TEST)
#error "Internal unit test enabled on non-debug build! Please disable."
#endif

class CDraw2d;
class UiRenderer;
class UiCanvasManager;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! CLyShine is the full implementation of the ILyShine interface
class CLyShine
    : public ILyShine
    , public IRenderDebugListener
    , public UiCursorBus::Handler
    , public AzFramework::InputChannelEventListener
    , public AzFramework::InputTextEventListener
{
public:

    //! Create the LyShine object, the given system pointer is stored internally
    CLyShine(ISystem* system);

    // ILyShine

    ~CLyShine() override;

    void Release() override;

    IDraw2d* GetDraw2d() override;
    IUiRenderer* GetUiRenderer() override;

    AZ::EntityId CreateCanvas() override;
    AZ::EntityId LoadCanvas(const string& assetIdPathname) override;
    AZ::EntityId CreateCanvasInEditor(UiEntityContext* entityContext) override;
    AZ::EntityId LoadCanvasInEditor(const string& assetIdPathname, const string& sourceAssetPathname, UiEntityContext* entityContext) override;
    AZ::EntityId ReloadCanvasFromXml(const AZStd::string& xmlString, UiEntityContext* entityContext) override;
    AZ::EntityId FindCanvasById(LyShine::CanvasId id) override;
    AZ::EntityId FindLoadedCanvasByPathName(const string& assetIdPathname) override;

    void ReleaseCanvas(AZ::EntityId canvas, bool forEditor) override;

    ISprite* LoadSprite(const string& pathname) override;
    ISprite* CreateSprite(const string& renderTargetName) override;

    void PostInit() override;

    void SetViewportSize(AZ::Vector2 viewportSize) override;
    void Update(float deltaTimeInSeconds) override;
    void Render() override;
    void ExecuteQueuedEvents() override;

    void Reset() override;
    void OnLevelUnload() override;

    // ~ILyShine

    // IRenderDebugListener

    //! Renders any debug displays currently enabled for the UI system
    void OnDebugDraw() override;

    // ~IRenderDebugListener

    // UiCursorInterface
    void IncrementVisibleCounter() override;
    void DecrementVisibleCounter() override;
    bool IsUiCursorVisible() override;
    void SetUiCursor(const char* cursorImagePath) override;
    AZ::Vector2 GetUiCursorPosition() override;
    // ~UiCursorInterface

    // InputChannelEventListener
    bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;
    // ~InputChannelEventListener

    // InputTextEventListener
    bool OnInputTextEventFiltered(const AZStd::string& textUTF8) override;
    // ~InputTextEventListener

private: // member functions

    AZ_DISABLE_COPY_MOVE(CLyShine);

    void RenderUiCursor();

private: // data

    ISystem* m_system;     // store a pointer to system rather than relying on env.pSystem

    std::unique_ptr<CDraw2d> m_draw2d;  // using a pointer rather than an instance to avoid including Draw2d.h
    std::unique_ptr<UiRenderer> m_uiRenderer;  // using a pointer rather than an instance to avoid including Draw2d.h

    std::unique_ptr<UiCanvasManager> m_uiCanvasManager;

    ITexture* m_uiCursorTexture;
    int m_uiCursorVisibleCounter;
};
