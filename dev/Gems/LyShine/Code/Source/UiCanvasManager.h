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

#include <IGameFramework.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/RTTI/TypeInfo.h>

#include <LyShine/Bus/UiCanvasManagerBus.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiAnimationBus.h>
#include <LyShine/UiEntityContext.h>
#include <LyShine/UiComponentTypes.h>

#include "UiElementComponent.h"
#include "UiSerialize.h"
#include "Animation/UiAnimationSystem.h"

namespace AZ
{
    class SerializeContext;
}

namespace AzFramework
{
    class InputChannel;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiCanvasManager
    : protected UiCanvasManagerBus::Handler
    , protected UiCanvasOrderNotificationBus::Handler
{
public: // member functions

    //! Constructor, constructed by the LyShine class
    UiCanvasManager();
    ~UiCanvasManager() override;

public: // member functions

    // UiCanvasManagerBus interface implementation
    AZ::EntityId CreateCanvas() override;
    AZ::EntityId LoadCanvas(const AZStd::string& canvasPathname) override;
    void UnloadCanvas(AZ::EntityId canvasEntityId) override;
    AZ::EntityId FindLoadedCanvasByPathName(const AZStd::string& canvasPathname) override;
    CanvasEntityList GetLoadedCanvases() override;
    // ~UiCanvasManagerBus

    // UiCanvasOrderNotificationBus
    void OnCanvasDrawOrderChanged(AZ::EntityId canvasEntityId) override;
    // ~UiCanvasOrderNotificationBus

    AZ::EntityId CreateCanvasInEditor(UiEntityContext* entityContext);
    AZ::EntityId LoadCanvasInEditor(const string& assetIdPathname, const string& sourceAssetPathname, UiEntityContext* entityContext);
    AZ::EntityId ReloadCanvasFromXml(const AZStd::string& xmlString, UiEntityContext* entityContext);

    void ReleaseCanvas(AZ::EntityId canvas, bool forEditor);

    AZ::EntityId FindCanvasById(LyShine::CanvasId id);

    void SetTargetSizeForLoadedCanvases(AZ::Vector2 viewportSize);
    void UpdateLoadedCanvases(float deltaTimeInSeconds);
    void RenderLoadedCanvases();

    void DestroyLoadedCanvases(bool keepCrossLevelCanvases);

    // These functions handle events for all canvases loaded in the game
    bool HandleInputEventForLoadedCanvases(const AzFramework::InputChannel& inputChannel);
    bool HandleTextEventForLoadedCanvases(const AZStd::string& textUTF8);

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiCanvasManager);

    void SortCanvasesByDrawOrder();

    UiCanvasComponent* FindCanvasComponentByPathname(const string& name);
    UiCanvasComponent* FindEditorCanvasComponentByPathname(const string& name);

    bool HandleInputEventForInWorldCanvases(const AzFramework::InputChannel::Snapshot& inputSnapshot, const AZ::Vector2& viewportPos);

    AZ::EntityId LoadCanvasInternal(const string& assetIdPathname, bool forEditor, const string& sourceAssetPathname, UiEntityContext* entityContext);

private: // types

    typedef std::vector<UiCanvasComponent*> CanvasList;   //!< Sorted by draw order

private: // data

    CanvasList m_loadedCanvases;             // UI Canvases loaded in game
    CanvasList m_loadedCanvasesInEditor;     // UI Canvases loaded in editor

    AZ::Vector2 m_latestViewportSize;        // The most recent viewport size
};
