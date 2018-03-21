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
#include "LyShine_precompiled.h"
#include "UiCanvasManager.h"

#include "UiCanvasFileObject.h"
#include "UiCanvasComponent.h"
#include "UiElementComponent.h"
#include "UiTransform2dComponent.h"
#include "UiSerialize.h"
#include "UiGameEntityContext.h"

#include <IRenderer.h>
#include <CryPath.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/Bus/UiNavigationBus.h>
#include <LyShine/Bus/UiTooltipDisplayBus.h>
#include <LyShine/Bus/UiEntityContextBus.h>
#include <LyShine/IUiRenderer.h>
#include <LyShine/UiSerializeHelpers.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/time.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Input/Channels/InputChannel.h>

#include "Animation/UiAnimationSystem.h"

#include <LyShine/Bus/UiCursorBus.h>
#include <LyShine/Bus/World/UiCanvasOnMeshBus.h>
#include <LyShine/Bus/World/UiCanvasRefBus.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
// Anonymous namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Compare function used for sorting
    bool CompareCanvasDrawOrder(UiCanvasComponent* a, UiCanvasComponent* b)
    {
        return a->GetDrawOrder() < b->GetDrawOrder();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Transform the pathname so that a) it works for opening a file that could be in a Gem or in
    // a pak file, and b) so that it is in a consistent form that can be used for string comparison
    string NormalizePath(const string& pathname)
    {
        AZStd::string normalizedPath(pathname.c_str());
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, normalizedPath);
        return normalizedPath.c_str();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasManager::UiCanvasManager()
    : m_latestViewportSize(UiCanvasComponent::s_defaultCanvasSize)
{
    UiCanvasManagerBus::Handler::BusConnect();
    UiCanvasOrderNotificationBus::Handler::BusConnect();
    AzFramework::AssetCatalogEventBus::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasManager::~UiCanvasManager()
{
    UiCanvasManagerBus::Handler::BusDisconnect();
    UiCanvasOrderNotificationBus::Handler::BusDisconnect();
    AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();

    // destroy ALL the loaded canvases, whether loaded in game or in Editor
    for (auto canvas : (m_loadedCanvases))
    {
        delete canvas->GetEntity();
    }

    for (auto canvas : (m_loadedCanvasesInEditor))
    {
        delete canvas->GetEntity();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::CreateCanvas()
{
    // Prevent in-game canvas from being created when we are in the editor in a simulation mode
    // but not in game mode (ex. AI/Physics mode or Preview mode)
    if (gEnv && gEnv->IsEditor() && gEnv->IsEditing())
    {
        return AZ::EntityId();
    }

    UiGameEntityContext* entityContext = new UiGameEntityContext();

    UiCanvasComponent* canvasComponent = UiCanvasComponent::CreateCanvasInternal(entityContext, false);

    m_loadedCanvases.push_back(canvasComponent);
    SortCanvasesByDrawOrder();

    // The game entity context needs to know its corresponding canvas entity for instantiating dynamic slices
    entityContext->SetCanvasEntity(canvasComponent->GetEntityId());

    // When we create a canvas in game we want it to have the correct viewport size from the first frame rather
    // than having to wait a frame to have it updated
    canvasComponent->SetTargetCanvasSize(true, m_latestViewportSize);

    return canvasComponent->GetEntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::LoadCanvas(const AZStd::string& assetIdPathname)
{
    // Prevent canvas from being loaded when we are in the editor in a simulation mode
    // but not in game mode (ex. AI/Physics mode or Preview mode).
    // NOTE: Normal Preview mode load does not come through here since we clone the canvas rather than load it
    if (gEnv && gEnv->IsEditor() && gEnv->IsEditing())
    {
        return AZ::EntityId();
    }

    UiGameEntityContext* entityContext = new UiGameEntityContext();

    string pathname(assetIdPathname.c_str());
    AZ::EntityId canvasEntityId = LoadCanvasInternal(pathname, false, "", entityContext);

    if (!canvasEntityId.IsValid())
    {
        delete entityContext;
    }
    else
    {
        // The game entity context needs to know its corresponding canvas entity for instantiating dynamic slices
        entityContext->SetCanvasEntity(canvasEntityId);

        EBUS_EVENT(UiCanvasManagerNotificationBus, OnCanvasLoaded, canvasEntityId);
    }

    return canvasEntityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::UnloadCanvas(AZ::EntityId canvasEntityId)
{
    ReleaseCanvasDeferred(canvasEntityId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::FindLoadedCanvasByPathName(const AZStd::string& assetIdPathname)
{
    // this is only used for finding canvases loaded in game
    UiCanvasComponent* canvasComponent = FindCanvasComponentByPathname(assetIdPathname.c_str());
    return canvasComponent ? canvasComponent->GetEntityId() : AZ::EntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasManager::CanvasEntityList UiCanvasManager::GetLoadedCanvases()
{
    CanvasEntityList list;
    for (auto canvasComponent : m_loadedCanvases)
    {
        list.push_back(canvasComponent->GetEntityId());
    }

    return list;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::OnCanvasDrawOrderChanged(AZ::EntityId canvasEntityId)
{
    SortCanvasesByDrawOrder();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
{
    // get AssetInfo from asset id
    AZ::Data::AssetInfo assetInfo;
    EBUS_EVENT_RESULT(assetInfo, AZ::Data::AssetCatalogRequestBus, GetAssetInfoById, assetId);
    if (assetInfo.m_assetType != LyShine::CanvasAsset::TYPEINFO_Uuid())
    {
        // this is not a UI canvas asset
        return;
    }

    // get pathname from asset id
    AZStd::string assetPath;
    EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, assetId);

    CanvasList reloadedCanvases;  // keep track of the reloaded canvases and add them to m_loadedCanvases after the loop
    AZStd::vector<AZ::EntityId> unloadedCanvases;  // also keep track of any canvases that fail to reload and are unloaded

    // loop over all canvases loaded in game and reload any canvases loaded from this canvas asset
    // NOTE: this could be improved by using AssetId for the comparison rather than pathnames
    string adjustedSearchName = NormalizePath(assetPath.c_str());
    auto iter = m_loadedCanvases.begin();
    while (iter != m_loadedCanvases.end())
    {
        UiCanvasComponent* canvasComponent = *iter;
        string adjustedName = NormalizePath(canvasComponent->GetPathname());
        if (adjustedSearchName == adjustedName)
        {
            AZ::EntityId existingCanvasEntityId = canvasComponent->GetEntityId();

            //Before unloading the existing canvas, make a copy of its mapping table
            AZ::SliceComponent::EntityIdToEntityIdMap existingRemapTable = canvasComponent->GetEditorToGameEntityIdMap();

            // unload the canvas, just deleting the canvas entity does this
            AZ::Entity* existingCanvasEntity = canvasComponent->GetEntity();
            delete existingCanvasEntity;

            // Remove the deleted canvas entry in the m_loadedCanvases vector and move the iterator to the next
            iter = m_loadedCanvases.erase(iter);

            // reload canvas with the same entity IDs (except for new entities, deleted entities etc)
            UiGameEntityContext* entityContext = new UiGameEntityContext();
            string pathname(assetPath.c_str());
            UiCanvasComponent* newCanvasComponent = UiCanvasComponent::LoadCanvasInternal(pathname, false, pathname, entityContext, &existingRemapTable, existingCanvasEntityId);

            if (!newCanvasComponent)
            {
                delete entityContext;
                unloadedCanvases.push_back(existingCanvasEntityId);
            }
            else
            {
                // The game entity context needs to know its corresponding canvas entity for instantiating dynamic slices
                entityContext->SetCanvasEntity(newCanvasComponent->GetEntityId());
                reloadedCanvases.push_back(newCanvasComponent);
            }
        }
        else
        {
            ++iter; // we did not delete this canvas - move to next in list
        }
    }

    // add the successfully reloaded canvases at the end
    m_loadedCanvases.insert(m_loadedCanvases.end(), reloadedCanvases.begin(), reloadedCanvases.end());

    // In case any draw orders changed resort
    SortCanvasesByDrawOrder();

    // notify any listeners of any UI canvases that were reloaded
    for (auto reloadedCanvasComponent : reloadedCanvases)
    {
        EBUS_EVENT(UiCanvasManagerNotificationBus, OnCanvasReloaded, reloadedCanvasComponent->GetEntityId());
    }

    // notify any listeners of any UI canvases that were unloaded
    for (auto unloadedCanvas : unloadedCanvases)
    {
        EBUS_EVENT(UiCanvasManagerNotificationBus, OnCanvasUnloaded, unloadedCanvas);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::CreateCanvasInEditor(UiEntityContext* entityContext)
{
    UiCanvasComponent* canvasComponent = UiCanvasComponent::CreateCanvasInternal(entityContext, true);

    m_loadedCanvasesInEditor.push_back(canvasComponent);

    return canvasComponent->GetEntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::LoadCanvasInEditor(const string& assetIdPathname, const string& sourceAssetPathname, UiEntityContext* entityContext)
{
    return LoadCanvasInternal(assetIdPathname, true, sourceAssetPathname, entityContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::ReloadCanvasFromXml(const AZStd::string& xmlString, UiEntityContext* entityContext)
{
    // Load new canvas from xml
    AZ::IO::MemoryStream memoryStream(xmlString.c_str(), xmlString.size());
    AZ::Entity* rootSliceEntity = nullptr;
    AZ::Entity* newCanvasEntity = UiCanvasFileObject::LoadCanvasEntitiesFromStream(memoryStream, rootSliceEntity);
    if (newCanvasEntity && rootSliceEntity)
    {
        // Find the old canvas to replace
        UiCanvasComponent* oldCanvasComponent = nullptr;
        for (auto canvas : (m_loadedCanvasesInEditor))
        {
            if (canvas->GetEntityId() == newCanvasEntity->GetId())
            {
                oldCanvasComponent = canvas;
                break;
            }
        }

        AZ_Assert(oldCanvasComponent, "Canvas not found");
        if (oldCanvasComponent)
        {
            LyShine::CanvasId oldCanvasId = oldCanvasComponent->GetCanvasId();
            string oldPathname = oldCanvasComponent->GetPathname();
            AZ::Matrix4x4 oldCanvasToViewportMatrix = oldCanvasComponent->GetCanvasToViewportMatrix();

            // Delete the old canvas. We assume this is for editor
            ReleaseCanvas(oldCanvasComponent->GetEntityId(), true);

            // Complete initialization of new canvas. We assume this is for editor
            UiCanvasComponent* newCanvasComponent = UiCanvasComponent::FixupReloadedCanvasForEditorInternal(
                    newCanvasEntity, rootSliceEntity, entityContext, oldCanvasId, oldPathname);

            newCanvasComponent->SetCanvasToViewportMatrix(oldCanvasToViewportMatrix);

            // Add new canvas to the list of loaded canvases
            m_loadedCanvasesInEditor.push_back(newCanvasComponent);

            return newCanvasComponent->GetEntityId();
        }
        else
        {
            delete newCanvasEntity;
        }
    }

    return AZ::EntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::ReleaseCanvas(AZ::EntityId canvasEntityId, bool forEditor)
{
    AZ::Entity* canvasEntity = nullptr;
    EBUS_EVENT_RESULT(canvasEntity, AZ::ComponentApplicationBus, FindEntity, canvasEntityId);
    AZ_Assert(canvasEntity, "Canvas entity not found by ID");

    if (canvasEntity)
    {
        UiCanvasComponent* canvasComponent = canvasEntity->FindComponent<UiCanvasComponent>();
        AZ_Assert(canvasComponent, "Canvas entity has no canvas component");

        if (canvasComponent)
        {
            if (forEditor)
            {
                stl::find_and_erase(m_loadedCanvasesInEditor, canvasComponent);
                delete canvasEntity;
            }
            else
            {
                stl::find_and_erase(m_loadedCanvases, canvasComponent);
                delete canvasEntity;

                EBUS_EVENT(UiCanvasManagerNotificationBus, OnCanvasUnloaded, canvasEntityId);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::ReleaseCanvasDeferred(AZ::EntityId canvasEntityId)
{
    AZ::Entity* canvasEntity = nullptr;
    EBUS_EVENT_RESULT(canvasEntity, AZ::ComponentApplicationBus, FindEntity, canvasEntityId);
    AZ_Assert(canvasEntity, "Canvas entity not found by ID");

    if (canvasEntity)
    {
        UiCanvasComponent* canvasComponent = canvasEntity->FindComponent<UiCanvasComponent>();
        AZ_Assert(canvasComponent, "Canvas entity has no canvas component");

        if (canvasComponent)
        {
            // Remove canvas component from list of loaded canvases
            stl::find_and_erase(m_loadedCanvases, canvasComponent);

            // Deactivate elements of the canvas
            canvasComponent->DeactivateElements();

            // Deactivate the canvas element
            canvasEntity->Deactivate();

            EBUS_EVENT(UiCanvasManagerNotificationBus, OnCanvasUnloaded, canvasEntityId);

            // Queue UI canvas deletion until next tick. This prevents deleting a UI canvas from an active entity within that UI canvas
            AZStd::function<void()> destroyCanvas = [canvasEntityId]()
            {
                AZ::Entity* queuedCanvasEntity = nullptr;
                EBUS_EVENT_RESULT(queuedCanvasEntity, AZ::ComponentApplicationBus, FindEntity, canvasEntityId);
                delete queuedCanvasEntity;
            };

            AZ::TickBus::QueueFunction(destroyCanvas);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::FindCanvasById(LyShine::CanvasId id)
{
    // this is only used for finding canvases loaded in game
    for (auto canvas : m_loadedCanvases)
    {
        if (canvas->GetCanvasId() == id)
        {
            return canvas->GetEntityId();
        }
    }
    return AZ::EntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::SetTargetSizeForLoadedCanvases(AZ::Vector2 viewportSize)
{
    for (auto canvas : m_loadedCanvases)
    {
        canvas->SetTargetCanvasSize(true, viewportSize);
    }

    m_latestViewportSize = viewportSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::UpdateLoadedCanvases(float deltaTime)
{
    // Update all the canvases loaded in game
    for (auto canvas : m_loadedCanvases)
    {
        canvas->UpdateCanvas(deltaTime, true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::RenderLoadedCanvases()
{
    // Render all the canvases loaded in game
    // canvases loaded in editor are rendered by the viewport window

    bool displayBounds = false;
#ifndef EXCLUDE_DOCUMENTATION_PURPOSE
    // If the console variable is set then display the element bounds
    // We use deferred render for the bounds so that they draw on top of everything else
    // this only works when running in-game
    if (UiCanvasComponent::CV_ui_DisplayElemBounds)
    {
        displayBounds = true;
    }
#endif

    // clear the stencil buffer before rendering the loaded canvases - required for masking
    // NOTE: We want to use ClearTargetsImmediately instead of ClearTargetsLater since we will not be setting the render target
    ColorF viewportBackgroundColor(0, 0, 0, 0); // if clearing color we want to set alpha to zero also
    gEnv->pRenderer->ClearTargetsImmediately(FRT_CLEAR_STENCIL, viewportBackgroundColor);

    for (auto canvas : m_loadedCanvases)
    {
        if (!canvas->GetIsRenderToTexture())
        {
            // Rendering in game full screen so the viewport size and target canvas size are the same
            AZ::Vector2 viewportSize = canvas->GetTargetCanvasSize();

            canvas->RenderCanvas(true, viewportSize, displayBounds);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::DestroyLoadedCanvases(bool keepCrossLevelCanvases)
{
    // Delete all the canvases loaded in game (but not loaded in editor)
    for (auto iter = m_loadedCanvases.begin(); iter != m_loadedCanvases.end(); ++iter)
    {
        auto canvas = *iter;
        bool removed = false;

        if (!(keepCrossLevelCanvases && canvas->GetKeepLoadedOnLevelUnload()))
        {
            // no longer used by game so delete the canvas
            delete canvas->GetEntity();
            *iter = nullptr;    // mark for removal from container
        }
    }

    // now remove the nullptr entries
    m_loadedCanvases.erase(
        std::remove(m_loadedCanvases.begin(), m_loadedCanvases.end(), nullptr),
        m_loadedCanvases.end());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasManager::HandleInputEventForLoadedCanvases(const AzFramework::InputChannel& inputChannel)
{
    // reverse iterate over the loaded canvases so that the front most canvas gets first chance to
    // handle the event
    bool areAnyInWorldInputCanvasesLoaded = false;

    // Take a snapshot of the input channel instead of just passing through the channel itself.
    // This is necessary because UI input is currently simulated in the editor's UI Preview mode
    // by constructing 'fake' input events, which we can do with snapshots but not input channels.
    // Long term we should look to update the AzFramework input system while in UI Editor Preview
    // mode so that it works exactly the same as in-game input, but this is a larger task for later.
    const AzFramework::InputChannel::Snapshot inputSnapshot(inputChannel);

    // De-normalize the position (if any) of the input event, as the UI system expects it relative
    // to the viewport from here on.
    AZ::Vector2 viewportPos(0.0f, 0.0f);
    const AzFramework::InputChannel::PositionData2D* positionData2D = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
    if (positionData2D)
    {
        viewportPos.SetX(positionData2D->m_normalizedPosition.GetX() * m_latestViewportSize.GetX());
        viewportPos.SetY(positionData2D->m_normalizedPosition.GetY() * m_latestViewportSize.GetY());
    }

    // Get the active modifier keys (if any) of the input event. Will only exist for keyboard keys.
    const AzFramework::ModifierKeyStates* modifierKeyStates = inputChannel.GetCustomData<AzFramework::ModifierKeyStates>();
    const AzFramework::ModifierKeyMask activeModifierKeys = modifierKeyStates ?
        modifierKeyStates->GetActiveModifierKeys() :
        AzFramework::ModifierKeyMask::None;

    for (auto iter = m_loadedCanvases.rbegin(); iter != m_loadedCanvases.rend(); ++iter)
    {
        UiCanvasComponent* canvas = *iter;

        if (canvas->GetIsRenderToTexture() && canvas->GetIsPositionalInputSupported())
        {
            // keep track of whether any canvases are rendering to texture. Positional events for these
            // are ignored in HandleInputEvent and handled later in this function by HandleInputEventForInWorldCanvases
            areAnyInWorldInputCanvasesLoaded = true;
        }

        if (canvas->HandleInputEvent(inputSnapshot, &viewportPos, activeModifierKeys))
        {
            return true;
        }
    }

    // if there are any canvases loaded that are rendering to texture we handle them seperately after the screen canvases
    // only do this for input events that are actually associated with a position
    if (areAnyInWorldInputCanvasesLoaded && positionData2D)
    {
        if (HandleInputEventForInWorldCanvases(inputSnapshot, viewportPos))
        {
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasManager::HandleTextEventForLoadedCanvases(const AZStd::string& textUTF8)
{
    // reverse iterate over the loaded canvases so that the front most canvas gets first chance to
    // handle the event
    for (auto iter = m_loadedCanvases.rbegin(); iter != m_loadedCanvases.rend(); ++iter)
    {
        if ((*iter)->HandleTextEvent(textUTF8))
        {
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::SortCanvasesByDrawOrder()
{
    std::stable_sort(m_loadedCanvases.begin(), m_loadedCanvases.end(), CompareCanvasDrawOrder);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent* UiCanvasManager::FindCanvasComponentByPathname(const string& name)
{
    UiCanvasComponent* match = nullptr;
    string adjustedSearchName = NormalizePath(name);
    for (auto canvas : (m_loadedCanvases))
    {
        string adjustedName = NormalizePath(canvas->GetPathname());
        if (adjustedSearchName == adjustedName)
        {
            return canvas;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent* UiCanvasManager::FindEditorCanvasComponentByPathname(const string& name)
{
    UiCanvasComponent* match = nullptr;
    string adjustedSearchName = NormalizePath(name);
    for (auto canvas : (m_loadedCanvasesInEditor))
    {
        string adjustedName = NormalizePath(canvas->GetPathname());
        if (adjustedSearchName == adjustedName)
        {
            return canvas;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasManager::HandleInputEventForInWorldCanvases(const AzFramework::InputChannel::Snapshot& inputSnapshot, const AZ::Vector2& viewportPos)
{
    // First we need to construct a ray from the either the center of the screen or the mouse position.
    // This requires knowledge of the camera
    // for initial testing we will just use a ray in the center of the viewport
    CCamera& cam = GetISystem()->GetViewCamera();

    // construct a ray from the camera position in the view direction of the camera
    const float rayLength = 5000.0f;
    Vec3 rayOrigin(cam.GetPosition());
    Vec3 rayDirection = cam.GetViewdir() * rayLength;

    // If the mouse cursor is visible we will assume that the ray should be in the direction of the
    // mouse pointer. This is a temporary solution. A better solution is to be able to configure the
    // LyShine system to say how ray input should be handled.
    bool isCursorVisible = false;
    UiCursorBus::BroadcastResult(isCursorVisible, &UiCursorInterface::IsUiCursorVisible);
    if (isCursorVisible)
    {
        // for some reason Unproject seems to work when given the viewport pos with (0,0) at the
        // bottom left as opposed to the top left - even though that function specifically sets top left
        // to (0,0).
        const float viewportYInverted = cam.GetViewSurfaceZ() - viewportPos.GetY();

        // Unproject to get the screen position in world space, use arbitrary Z that is within the depth range
        Vec3 flippedViewportPos(viewportPos.GetX(), viewportYInverted, 0.5f);
        Vec3 unprojectedPos;
        cam.Unproject(flippedViewportPos, unprojectedPos);

        // We want a vector relative to the camera origin
        Vec3 rayVec = unprojectedPos - rayOrigin;

        // we want to ensure that the ray is a certain length so normalize it and scale it
        rayVec.NormalizeSafe();
        rayDirection = rayVec * rayLength;
    }

    // do a ray world intersection test
    ray_hit rayhit;
    // NOTE: these flags may need some tuning. After a fix from physics setup rwi_colltype_any may work.
    unsigned int flags = rwi_stop_at_pierceable | rwi_ignore_noncolliding;
    if (gEnv->pPhysicalWorld->RayWorldIntersection(rayOrigin, rayDirection, ent_all, flags, &rayhit, 1))
    {
        // if the ray collided with a component entity then call a bus on that entity to process the event
        const AZ::EntityId entityId = AZ::EntityId(rayhit.pCollider->GetForeignData(PHYS_FOREIGN_ID_COMPONENT_ENTITY));
        if (entityId.IsValid())
        {
            // first get the UI canvas entity from the hit entity - we do this to see if it supports
            // automatic input.
            AZ::EntityId canvasEntityId;
            EBUS_EVENT_ID_RESULT(canvasEntityId, entityId, UiCanvasRefBus, GetCanvas);

            if (canvasEntityId.IsValid())
            {
                // Checkif the UI canvas referenced by the hit entity supports automatic input
                bool doesCanvasSupportInput = false;
                EBUS_EVENT_ID_RESULT(doesCanvasSupportInput, canvasEntityId, UiCanvasBus, GetIsPositionalInputSupported);

                if (doesCanvasSupportInput)
                {
                    // set the hit details to the hit entity, it will convert into canvas coords and send to canvas
                    bool result = false;
                    EBUS_EVENT_ID_RESULT(result, entityId, UiCanvasOnMeshBus, ProcessRayHitInputEvent, inputSnapshot, rayhit);
                    if (result)
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::LoadCanvasInternal(const string& assetIdPathname, bool forEditor, const string& sourceAssetPathname, UiEntityContext* entityContext,
    const AZ::SliceComponent::EntityIdToEntityIdMap* previousRemapTable, AZ::EntityId previousCanvasId)
{
    string pathToOpen;
    if (forEditor)
    {
        // If loading from the editor we load the source asset path (just in case it is not in the cache)
        // Eventually we may refactor so the LyShine never accesses the source assets and perhaps pass
        // in a buffer from the editor.
        pathToOpen = sourceAssetPathname;
    }
    else
    {
        // If loading in game this could be a path that a developer typed into a flow graph node.
        // However, it has to be a valid asset ID path. E.g. it can be resolved from the asset root
        // since at runtime we cannot convert from an arbitrary dev asset path to an asset ID
        pathToOpen = assetIdPathname;
    }

    string assetIdPath = assetIdPathname;

    const string canvasExtension("uicanvas");

    const char* extension = PathUtil::GetExt(pathToOpen.c_str());
    if (!extension || extension[0] == '\0')
    {
        // empty path - add the uicanvas extension
        pathToOpen = PathUtil::ReplaceExtension(pathToOpen, canvasExtension);
        assetIdPath = PathUtil::ReplaceExtension(assetIdPath, canvasExtension);
    }
    else
    {
        string lowerCaseExt = PathUtil::ToLower(extension);
        if (lowerCaseExt != "uicanvas")
        {
            // Invalid extension
            AZ_Warning("UI", false, "Given UI canvas path \"%s\" has an invalid extension. Replacing extension with \"%s\".",
                pathToOpen.c_str(), canvasExtension.c_str());
            pathToOpen = PathUtil::ReplaceExtension(pathToOpen, canvasExtension);
            assetIdPath = PathUtil::ReplaceExtension(assetIdPath, canvasExtension);
        }
    }

    // if the canvas is already loaded in the editor and we are running in game then we clone the
    // editor version so that the user can test their canvas without saving it
    UiCanvasComponent* canvasComponent = FindEditorCanvasComponentByPathname(assetIdPath);
    if (canvasComponent)
    {
        // this canvas is already loaded in the editor
        if (forEditor)
        {
            // should never load a canvas in Editor if it is already loaded. The Editor should avoid loading the
            // same canvas twice in Editor. If the game is running it is not possible to load a canvas
            // from the editor.
            gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,
                pathToOpen.c_str(),
                "UI canvas file: %s is already loaded",
                pathToOpen.c_str());
            return AZ::EntityId();
        }
        else
        {
            // we are loading from the game, the canvas is already open in the editor, so
            // we clone the canvas that is open in the editor.
            canvasComponent = canvasComponent->CloneAndInitializeCanvas(entityContext, assetIdPath);
        }
    }
    else
    {
        // not already loaded in editor, attempt to load...
        canvasComponent = UiCanvasComponent::LoadCanvasInternal(pathToOpen, forEditor, assetIdPath, entityContext, previousRemapTable, previousCanvasId);
    }

    if (canvasComponent)
    {
        // canvas loaded OK (or cloned from Editor canvas OK)

        // add to the list of loaded canvases
        if (forEditor)
        {
            m_loadedCanvasesInEditor.push_back(canvasComponent);
        }
        else
        {
            m_loadedCanvases.push_back(canvasComponent);
            SortCanvasesByDrawOrder();
        }
    }

    return (canvasComponent) ? canvasComponent->GetEntityId() : AZ::EntityId();
}
