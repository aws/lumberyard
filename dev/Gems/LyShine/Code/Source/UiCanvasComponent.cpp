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
#include "UiCanvasComponent.h"

#include "UiElementComponent.h"
#include "UiTransform2dComponent.h"
#include "UiSerialize.h"
#include "UiCanvasFileObject.h"
#include "UiGameEntityContext.h"
#include "UiNavigationHelpers.h"

#include <IRenderer.h>
#include <CryPath.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiInitializationBus.h>
#include <LyShine/Bus/UiNavigationBus.h>
#include <LyShine/Bus/UiTooltipDisplayBus.h>
#include <LyShine/Bus/UiLayoutBus.h>
#include <LyShine/Bus/UiEntityContextBus.h>
#include <LyShine/IUiRenderer.h>
#include <LyShine/UiSerializeHelpers.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/time.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>


#include "Animation/UiAnimationSystem.h"

#include <LyShine/Bus/World/UiCanvasOnMeshBus.h>
#include <LyShine/Bus/World/UiCanvasRefBus.h>

// This is the memory allocation for the static data member used for the debug console variable
AllocateConstIntCVar(UiCanvasComponent, CV_ui_DisplayElemBounds);

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiCanvasNotificationBus Behavior context handler class
class UiCanvasNotificationBusBehaviorHandler
    : public UiCanvasNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiCanvasNotificationBusBehaviorHandler, "{64014B4F-E12F-4839-99B0-426B5717DB44}", AZ::SystemAllocator,
        OnAction);

    void OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName) override
    {
        Call(FN_OnAction, entityId, actionName);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiCanvasInputNotificationBus Behavior context handler class
class UiCanvasInputNotificationBusBehaviorHandler
    : public UiCanvasInputNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiCanvasInputNotificationBusBehaviorHandler, "{76042EFA-0B61-4E7A-ACC8-296382D46881}", AZ::SystemAllocator,
        OnCanvasPrimaryPressed, OnCanvasPrimaryReleased, OnCanvasMultiTouchPressed, OnCanvasMultiTouchReleased, OnCanvasHoverStart, OnCanvasHoverEnd, OnCanvasEnterPressed, OnCanvasEnterReleased);

    void OnCanvasPrimaryPressed(AZ::EntityId entityId) override
    {
        Call(FN_OnCanvasPrimaryPressed, entityId);
    }

    void OnCanvasPrimaryReleased(AZ::EntityId entityId) override
    {
        Call(FN_OnCanvasPrimaryReleased, entityId);
    }

    void OnCanvasMultiTouchPressed(AZ::EntityId entityId, int multiTouchIndex) override
    {
        Call(FN_OnCanvasMultiTouchPressed, entityId, multiTouchIndex);
    }

    void OnCanvasMultiTouchReleased(AZ::EntityId entityId, int multiTouchIndex) override
    {
        Call(FN_OnCanvasMultiTouchReleased, entityId, multiTouchIndex);
    }

    void OnCanvasHoverStart(AZ::EntityId entityId) override
    {
        Call(FN_OnCanvasHoverStart, entityId);
    }

    void OnCanvasHoverEnd(AZ::EntityId entityId) override
    {
        Call(FN_OnCanvasHoverEnd, entityId);
    }

    void OnCanvasEnterPressed(AZ::EntityId entityId) override
    {
        Call(FN_OnCanvasEnterPressed, entityId);
    }

    void OnCanvasEnterReleased(AZ::EntityId entityId) override
    {
        Call(FN_OnCanvasEnterReleased, entityId);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiAnimationNotificationBus Behavior context handler class
class UiAnimationNotificationBusBehaviorHandler
    : public UiAnimationNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiAnimationNotificationBusBehaviorHandler, "{35D19FE8-5F31-426E-877A-8EEF3A42F99F}", AZ::SystemAllocator,
        OnUiAnimationEvent);

    void OnUiAnimationEvent(IUiAnimationListener::EUiAnimationEvent uiAnimationEvent, AZStd::string animSequenceName) override
    {
        Call(FN_OnUiAnimationEvent, uiAnimationEvent, animSequenceName);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Anonymous namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    LyShine::CanvasId s_lastCanvasId = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // test if the given text file starts with the given text string
    bool TestFileStartString(const string& pathname, const char* expectedStart)
    {
        // Open the file using CCryFile, this supports it being in the pak file or a standalone file
        CCryFile file;
        if (!file.Open(pathname, "r"))
        {
            return false;
        }

        // get the size of the file and the length of the expected start string
        size_t fileSize = file.GetLength();
        size_t expectedStartLen = strlen(expectedStart);

        // if the file is smaller than the expected start string then it is not a valid file
        if (fileSize < expectedStartLen)
        {
            return false;
        }

        // read in the length of the expected start string
        char* buffer = new char[expectedStartLen];
        size_t bytesRead = file.ReadRaw(buffer, expectedStartLen);

        // match is true if the string read from the file matches the expected start string
        bool match = strncmp(expectedStart, buffer, expectedStartLen) == 0;
        delete [] buffer;
        return match;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Check if the given file was saved using AZ serialization
    bool IsValidAzSerializedFile(const string& pathname)
    {
        return TestFileStartString(pathname, "<ObjectStream");
    }

    template<class T, class MapType>
    void ReuseOrGenerateNewIdsAndFixRefs(T* object, MapType& newIdMap, AZ::SerializeContext* context)
    {
        AZ::EntityUtils::ReplaceEntityIds(
            object,
            [&newIdMap](const AZ::EntityId& originalId, bool /*isEntityId*/) -> AZ::EntityId
            {
                auto findIt = newIdMap.find(originalId);
                if (findIt == newIdMap.end())
                {
                    AZ::EntityId newId = AZ::Entity::MakeId();
                    newIdMap.insert(AZStd::make_pair(originalId, newId));
                    return newId;
                }
                else
                {
                    return findIt->second; // return the previously remapped id
                }
            }, context);

        AZ::EntityUtils::ReplaceEntityRefs(
            object,
            [&newIdMap](const AZ::EntityId& originalId, bool /*isEntityId*/) -> AZ::EntityId
            {
                auto findIt = newIdMap.find(originalId);
                if (findIt == newIdMap.end())
                {
                    return originalId; // entityId is not being remapped
                }
                else
                {
                    return findIt->second; // return the remapped id
                }
            }, context);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// STATIC MEMBER DATA
////////////////////////////////////////////////////////////////////////////////////////////////////

const AZ::Vector2 UiCanvasComponent::s_defaultCanvasSize(1280.0f, 720.0f);

bool UiCanvasComponent::s_handleHoverInputEvents = true;

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent::UiCanvasComponent()
    : m_uniqueId(0)
    , m_rootElement()
    , m_lastElementId(0)
    , m_isPixelAligned(true)
    , m_canvasToViewportMatrix(AZ::Matrix4x4::CreateIdentity())
    , m_viewportToCanvasMatrix(AZ::Matrix4x4::CreateIdentity())
    , m_activeInteractableShouldStayActive(false)
    , m_allowInvalidatingHoverInteractableOnHoverInput(true)
    , m_isActiveInteractablePressed(false)
    , m_lastMousePosition(-1.0f, -1.0f)
    , m_id(++s_lastCanvasId)
    , m_drawOrder(0)
    , m_canvasSize(UiCanvasComponent::s_defaultCanvasSize)
    , m_targetCanvasSize(m_canvasSize)
    , m_uniformDeviceScale(1.0f)
    , m_isLoadedInGame(false)
    , m_keepLoadedOnLevelUnload(false)
    , m_enabled(true)
    , m_renderToTexture(false)
    , m_isSnapEnabled(false)
    , m_snapDistance(10.0f)
    , m_snapRotationDegrees(10.0f)
    , m_entityContext(nullptr)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent::~UiCanvasComponent()
{
    m_uiAnimationSystem.RemoveAllSequences();

    if (m_entityContext)
    {
        // deactivate all UI elements, this is so that we can detect improper deletion of UI elements by users
        // during game play
        DeactivateElements();

        // Destroy the entity context, this will delete all the UI elements
        m_entityContext->DestroyUiContext();
    }

    if (m_isLoadedInGame)
    {
        delete m_entityContext;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::UpdateCanvas(float deltaTime, bool isInGame)
{
    if (isInGame)
    {
        // Ignore update if we're not enabled
        if (!m_enabled)
        {
            return;
        }

        EBUS_EVENT_ID(m_rootElement, UiElementBus, UpdateElement, deltaTime);

        // update the animation system
        m_uiAnimationSystem.PreUpdate(deltaTime);
        m_uiAnimationSystem.PostUpdate(deltaTime);
    }

    SendRectChangeNotificationsAndRecomputeLayouts();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::RenderCanvas(bool isInGame, AZ::Vector2 viewportSize, bool displayBounds)
{
    // Ignore render ops if we're not enabled
    if (!m_enabled)
    {
        return;
    }

    IUiRenderer::Get()->BeginCanvasRender(viewportSize);
    EBUS_EVENT_ID(m_rootElement, UiElementBus, RenderElement, isInGame, displayBounds);
    IUiRenderer::Get()->EndCanvasRender();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const string& UiCanvasComponent::GetPathname()
{
    return m_pathname;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::CanvasId UiCanvasComponent::GetCanvasId()
{
    return m_id;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::u64 UiCanvasComponent::GetUniqueCanvasId()
{
    return m_uniqueId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiCanvasComponent::GetDrawOrder()
{
    return m_drawOrder;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetDrawOrder(int drawOrder)
{
    m_drawOrder = drawOrder;
    EBUS_EVENT(UiCanvasOrderNotificationBus, OnCanvasDrawOrderChanged, GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::GetKeepLoadedOnLevelUnload()
{
    return m_keepLoadedOnLevelUnload;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetKeepLoadedOnLevelUnload(bool keepLoaded)
{
    m_keepLoadedOnLevelUnload = keepLoaded;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::RecomputeChangedLayouts()
{
    SendRectChangeNotificationsAndRecomputeLayouts();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiCanvasComponent::GetNumChildElements()
{
    int numChildElements = 0;
    EBUS_EVENT_ID_RESULT(numChildElements, m_rootElement, UiElementBus, GetNumChildElements);
    return numChildElements;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiCanvasComponent::GetChildElement(int index)
{
    AZ::Entity* child = nullptr;
    EBUS_EVENT_ID_RESULT(child, m_rootElement, UiElementBus, GetChildElement, index);
    return child;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasComponent::GetChildElementEntityId(int index)
{
    AZ::EntityId childEntityId;
    EBUS_EVENT_ID_RESULT(childEntityId, m_rootElement, UiElementBus, GetChildEntityId, index);
    return childEntityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::EntityArray UiCanvasComponent::GetChildElements()
{
    LyShine::EntityArray childElements;
    EBUS_EVENT_ID_RESULT(childElements, m_rootElement, UiElementBus, GetChildElements);
    return childElements;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::vector<AZ::EntityId> UiCanvasComponent::GetChildElementEntityIds()
{
    AZStd::vector<AZ::EntityId> childElementEntityIds;
    EBUS_EVENT_ID_RESULT(childElementEntityIds, m_rootElement, UiElementBus, GetChildEntityIds);
    return childElementEntityIds;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiCanvasComponent::CreateChildElement(const LyShine::NameType& name)
{
    AZ::Entity* child = nullptr;
    EBUS_EVENT_ID_RESULT(child, m_rootElement, UiElementBus, CreateChildElement, name);
    return child;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiCanvasComponent::FindElementById(LyShine::ElementId id)
{
    AZ::Entity* element = nullptr;
    EBUS_EVENT_ID_RESULT(element, m_rootElement, UiElementBus, FindDescendantById, id);
    return element;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiCanvasComponent::FindElementByName(const LyShine::NameType& name)
{
    AZ::Entity* entity = nullptr;
    EBUS_EVENT_ID_RESULT(entity, m_rootElement, UiElementBus, FindDescendantByName, name);
    return entity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasComponent::FindElementEntityIdByName(const LyShine::NameType& name)
{
    AZ::EntityId entityId;
    EBUS_EVENT_ID_RESULT(entityId, m_rootElement, UiElementBus, FindDescendantEntityIdByName, name);
    return entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::FindElementsByName(const LyShine::NameType& name, LyShine::EntityArray& result)
{
    // find all elements with the given name
    EBUS_EVENT_ID(m_rootElement, UiElementBus, FindDescendantElements,
        [&name](const AZ::Entity* entity) { return name == entity->GetName(); },
        result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiCanvasComponent::FindElementByHierarchicalName(const LyShine::NameType& name)
{
    // start at the root
    AZ::Entity* currentEntity = GetRootElement();
    bool found = false;

    std::size_t lastPos = 0;
    while (currentEntity)
    {
        std::size_t pos = name.find('/', lastPos);
        if (pos == lastPos)
        {
            // skip over any double '/' characters or '/' characters at the start
            lastPos++;
        }
        else if (pos == LyShine::NameType::npos)
        {
            // '/' not found, use whole remaining string
            AZ::Entity* entity = nullptr;
            EBUS_EVENT_ID_RESULT(entity, currentEntity->GetId(), UiElementBus,
                FindChildByName, name.substr(lastPos));
            currentEntity = entity;

            if (currentEntity)
            {
                found = true;
            }
            break;
        }
        else
        {
            // use the part of the string between lastPos and pos (between the '/' characters)
            AZ::Entity* entity = nullptr;
            EBUS_EVENT_ID_RESULT(entity, currentEntity->GetId(), UiElementBus,
                FindChildByName, name.substr(lastPos, pos - lastPos));
            currentEntity = entity;
            lastPos = pos + 1;
        }
    }

    return (found) ? currentEntity : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::FindElements(AZStd::function<bool(const AZ::Entity*)> predicate, LyShine::EntityArray& result)
{
    // find all matching elements
    EBUS_EVENT_ID(m_rootElement, UiElementBus, FindDescendantElements, predicate, result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiCanvasComponent::PickElement(AZ::Vector2 point)
{
    AZ::Entity* element = nullptr;
    EBUS_EVENT_ID_RESULT(element, m_rootElement,
        UiElementBus, FindFrontmostChildContainingPoint, point, m_isLoadedInGame);
    return element;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::EntityArray UiCanvasComponent::PickElements(const AZ::Vector2& bound0, const AZ::Vector2& bound1)
{
    LyShine::EntityArray elements;
    EBUS_EVENT_ID_RESULT(elements, m_rootElement,
        UiElementBus, FindAllChildrenIntersectingRect, bound0, bound1, m_isLoadedInGame);
    return elements;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasComponent::FindInteractableToHandleEvent(AZ::Vector2 point)
{
    AZ::EntityId interactable;
    EBUS_EVENT_ID_RESULT(interactable, m_rootElement, UiElementBus, FindInteractableToHandleEvent, point);
    return interactable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::SaveToXml(const string& assetIdPathname, const string& sourceAssetPathname)
{
    PrepareAnimationSystemForCanvasSave();

    // We are saving to the dev assets (source) not the cache so we use the sourceAssetPathname to save
    // the file
    bool result = SaveCanvasToFile(sourceAssetPathname, AZ::ObjectStream::ST_XML);

    if (result)
    {
        // We store the asset ID so that we can tell if the same file is being loaded from the game
        m_pathname = assetIdPathname;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasInterface::ErrorCode UiCanvasComponent::CheckElementValidToSaveAsPrefab(AZ::Entity* entity)
{
    AZ_Assert(entity, "null entity ptr passed to SaveAsPrefab");

    // Check that none of the EntityId's in this entity or its children reference entities that
    // are not part of the prefab.
    // First make a list of all entityIds that will be in the prefab
    AZStd::vector<AZ::EntityId> entitiesInPrefab = GetEntityIdsOfElementAndDescendants(entity);

    // Next check all entity refs in the element to see if any are externel
    // We use ReplaceEntityRefs even though we don't want to change anything
    bool foundRefOutsidePrefab = false;
    AZ::SerializeContext* context = nullptr;
    EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(context, "No serialization context found");
    AZ::EntityUtils::ReplaceEntityRefs(entity, [&](const AZ::EntityId& key, bool /*isEntityId*/) -> AZ::EntityId
        {
            if (key.IsValid())
            {
                auto iter = AZStd::find(entitiesInPrefab.begin(), entitiesInPrefab.end(), key);
                if (iter == entitiesInPrefab.end())
                {
                    foundRefOutsidePrefab = true;
                }
            }
            return key; // always leave key unchanged
        }, context);

    if (foundRefOutsidePrefab)
    {
        return UiCanvasInterface::ErrorCode::PrefabContainsExternalEntityRefs;
    }

    return UiCanvasInterface::ErrorCode::NoError;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::SaveAsPrefab(const string& pathname, AZ::Entity* entity)
{
    AZ_Assert(entity, "null entity ptr passed to SaveAsPrefab");

    AZ::SerializeContext* context = nullptr;
    EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(context, "No serialization context found");

    // To be sure that we do not save an invalid prefab, if this entity contains entity references
    // outside of the prefab set them to invalid references
    // First make a list of all entityIds that will be in the prefab
    AZStd::vector<AZ::EntityId> entitiesInPrefab = GetEntityIdsOfElementAndDescendants(entity);

    // Next make a serializable object containing all the entities to save (in order to check for invalid refs)
    AZ::SliceComponent::InstantiatedContainer sourceObjects;
    for (const AZ::EntityId& id : entitiesInPrefab)
    {
        AZ::Entity* sourceEntity = nullptr;
        EBUS_EVENT_RESULT(sourceEntity, AZ::ComponentApplicationBus, FindEntity, id);
        if (sourceEntity)
        {
            sourceObjects.m_entities.push_back(sourceEntity);
        }
    }

    // clone all the objects in order to replace external references
    AZ::SliceComponent::InstantiatedContainer* clonedObjects = context->CloneObject(&sourceObjects);
    AZ::Entity* clonedRootEntity = clonedObjects->m_entities[0];

    // clear sourceObjects so that its destructor doesn't delete the objects that we cloned from
    sourceObjects.m_entities.clear();

    // use ReplaceEntityRefs to replace external references with invalid IDs
    // Note that we are not generating new IDs so we do not need to fixup internal references
    AZ::EntityUtils::ReplaceEntityRefs(clonedObjects, [&](const AZ::EntityId& key, bool /*isEntityId*/) -> AZ::EntityId
        {
            if (key.IsValid())
            {
                auto iter = AZStd::find(entitiesInPrefab.begin(), entitiesInPrefab.end(), key);
                if (iter == entitiesInPrefab.end())
                {
                    return AZ::EntityId();
                }
            }
            return key; // leave key unchanged
        }, context);

    // make a wrapper object around the prefab entity so that we have an opportunity to change what
    // is in a prefab file in future.
    UiSerialize::PrefabFileObject fileObject;
    fileObject.m_rootEntityId = clonedRootEntity->GetId();

    // add all of the entities that are not the root entity to a childEntities list
    for (auto descendant : clonedObjects->m_entities)
    {
        fileObject.m_entities.push_back(descendant);
    }

    bool result = AZ::Utils::SaveObjectToFile(pathname.c_str(), AZ::ObjectStream::ST_XML, &fileObject);

    // now delete the cloned entities we created, fixed up and saved
    delete clonedObjects;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiCanvasComponent::LoadFromPrefab(const string& pathname, bool makeUniqueName, AZ::Entity* optionalInsertionPoint)
{
    AZ::Entity* newEntity = nullptr;

    // Currently LoadObjectFromFile will hang if the file cannot be parsed
    // (LMBR-10078). So first check that it is in the right format
    if (!IsValidAzSerializedFile(pathname))
    {
        return nullptr;
    }

    // The top level object in the file is a wrapper object called PrefabFileObject
    // this is to give us more protection against changes to what we store in the file in future
    // NOTE: this read doesn't support pak files but that is OK because prefab files are an
    // editor only feature.
    UiSerialize::PrefabFileObject* fileObject =
        AZ::Utils::LoadObjectFromFile<UiSerialize::PrefabFileObject>(pathname.c_str());
    AZ_Assert(fileObject, "Failed to load prefab");

    if (fileObject)
    {
        // We want new IDs so generate them and fixup all references within the list of entities
        {
            AZ::SerializeContext* context = nullptr;
            EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
            AZ_Assert(context, "No serialization context found");

            AZ::SliceComponent::EntityIdToEntityIdMap entityIdMap;
            AZ::IdUtils::Remapper<AZ::EntityId>::GenerateNewIdsAndFixRefs(fileObject, entityIdMap, context);
        }

        // add all of the entities to this canvases EntityContext
        m_entityContext->AddUiEntities(fileObject->m_entities);

        EBUS_EVENT_RESULT(newEntity, AZ::ComponentApplicationBus, FindEntity, fileObject->m_rootEntityId);

        delete fileObject;    // we do not keep the file wrapper object around

        if (makeUniqueName)
        {
            AZ::EntityId parentEntityId;
            if (optionalInsertionPoint)
            {
                parentEntityId = optionalInsertionPoint->GetId();
            }
            AZStd::string uniqueName = GetUniqueChildName(parentEntityId, newEntity->GetName(), nullptr);
            newEntity->SetName(uniqueName);
        }

        UiElementComponent* elementComponent = newEntity->FindComponent<UiElementComponent>();
        AZ_Assert(elementComponent, "No element component found on prefab entity");

        AZ::Entity* parent = (optionalInsertionPoint) ? optionalInsertionPoint : GetRootElement();

        // recursively visit all the elements and set their canvas and parent pointers
        elementComponent->FixupPostLoad(newEntity, this, parent, true);

        // add this new entity as a child of the parent (insertionPoint or root)
        UiElementComponent* parentElementComponent = parent->FindComponent<UiElementComponent>();
        AZ_Assert(parentElementComponent, "No element component found on parent entity");
        parentElementComponent->AddChild(newEntity);
    }

    return newEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::FixupCreatedEntities(LyShine::EntityArray topLevelEntities, bool makeUniqueNamesAndIds, AZ::Entity* optionalInsertionPoint)
{
    if (makeUniqueNamesAndIds)
    {
        AZ::EntityId parentEntityId;
        if (optionalInsertionPoint)
        {
            parentEntityId = optionalInsertionPoint->GetId();
        }

        LyShine::EntityArray namedChildren;
        for (auto entity : topLevelEntities)
        {
            AZStd::string uniqueName = GetUniqueChildName(parentEntityId, entity->GetName(), &namedChildren);
            entity->SetName(uniqueName);
            namedChildren.push_back(entity);
        }
    }

    AZ::Entity* parent = (optionalInsertionPoint) ? optionalInsertionPoint : GetRootElement();

    for (auto entity : topLevelEntities)
    {
        UiElementComponent* elementComponent = entity->FindComponent<UiElementComponent>();
        AZ_Assert(elementComponent, "No element component found on prefab entity");

        // recursively visit all the elements and set their canvas and parent pointers
        elementComponent->FixupPostLoad(entity, this, parent, makeUniqueNamesAndIds);
    }

    if (m_isLoadedInGame)
    {
        // Call InGamePostActivate on all the created entities
        for (auto entity : topLevelEntities)
        {
            EBUS_EVENT_ID(entity->GetId(), UiInitializationBus, InGamePostActivate);

            LyShine::EntityArray descendantElements;
            EBUS_EVENT_ID(entity->GetId(), UiElementBus, FindDescendantElements,
                [](const AZ::Entity*) { return true; },
                descendantElements);

            for (AZ::Entity* child : descendantElements)
            {
                EBUS_EVENT_ID(child->GetId(), UiInitializationBus, InGamePostActivate);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::AddElement(AZ::Entity* element, AZ::Entity* parent, AZ::Entity* insertBefore)
{
    if (!parent)
    {
        parent = GetRootElement();
    }

    // add this new entity as a child of the parent (insertionPoint or root)
    UiElementComponent* parentElementComponent = parent->FindComponent<UiElementComponent>();
    AZ_Assert(parentElementComponent, "No element component found on parent entity");
    parentElementComponent->AddChild(element, insertBefore);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::ReinitializeElements()
{
    AZ::Entity* rootElement = GetRootElement();

    UiElementComponent* elementComponent = rootElement->FindComponent<UiElementComponent>();
    AZ_Assert(elementComponent, "No element component found on root element entity");

    elementComponent->FixupPostLoad(rootElement, this, nullptr, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiCanvasComponent::SaveToXmlString()
{
    PrepareAnimationSystemForCanvasSave();

    AZStd::string charBuffer;
    AZ::IO::ByteContainerStream<AZStd::string > charStream(&charBuffer);
    bool success = SaveCanvasToStream(charStream, AZ::ObjectStream::ST_XML);

    AZ_Assert(success, "Failed to serialize canvas entity to XML");
    return charBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiCanvasComponent::GetUniqueChildName(AZ::EntityId parentEntityId, AZStd::string baseName, const LyShine::EntityArray* includeChildren)
{
    // Get a list of children that the name needs to be unique to
    LyShine::EntityArray children;
    if (parentEntityId.IsValid())
    {
        EBUS_EVENT_ID_RESULT(children, parentEntityId, UiElementBus, GetChildElements);
    }
    else
    {
        children = GetChildElements();
    }

    if (includeChildren)
    {
        children.push_back(*includeChildren);
    }

    // First, check if base name is unique
    if (IsElementNameUnique(baseName, children))
    {
        return baseName;
    }

    // Count trailing digits in base name
    int i;
    for (i = baseName.length() - 1; i >= 0; i--)
    {
        if (!isdigit(baseName[i]))
        {
            break;
        }
    }
    int startDigitIndex = i + 1;
    int numDigits = baseName.length() - startDigitIndex;

    int suffix = 1;
    if (numDigits > 0)
    {
        // Set starting suffix
        suffix = AZStd::stoi(baseName.substr(startDigitIndex, numDigits));

        // Trim the digits from the base name
        baseName.erase(startDigitIndex, numDigits);
    }

    // Keep incrementing suffix until a unique name is found
    // NOTE: This could cause a performance issue when large copies are being made in a large canvas
    AZStd::string proposedChildName;
    do
    {
        ++suffix;

        proposedChildName = baseName;

        AZStd::string suffixString = AZStd::string::format("%d", suffix);

        // Append leading zeros
        int numLeadingZeros = (suffixString.length() < numDigits) ? numDigits - suffixString.length() : 0;
        for (int zeroes = 0; zeroes < numLeadingZeros; zeroes++)
        {
            proposedChildName.push_back('0');
        }

        // Append suffix
        proposedChildName.append(suffixString);
    } while (!IsElementNameUnique(proposedChildName, children));

    return proposedChildName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiCanvasComponent::CloneElement(AZ::Entity* sourceEntity, AZ::Entity* parentEntity)
{
    return CloneAndAddElementInternal(sourceEntity, parentEntity, nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasComponent::CloneElementEntityId(AZ::EntityId sourceEntityId, AZ::EntityId parentEntityId, AZ::EntityId insertBeforeId)
{
    AZ::EntityId result;

    AZ::Entity* sourceEntity = nullptr;
    EBUS_EVENT_RESULT(sourceEntity, AZ::ComponentApplicationBus, FindEntity, sourceEntityId);
    if (!sourceEntity)
    {
        AZ_Warning("UI", false, "CloneElementEntityId: Cannot find entity to clone.");
        return result;
    }

    AZ::Entity* parentEntity = nullptr;
    if (parentEntityId.IsValid())
    {
        EBUS_EVENT_RESULT(parentEntity, AZ::ComponentApplicationBus, FindEntity, parentEntityId);
        if (!parentEntity)
        {
            AZ_Warning("UI", false, "CloneElementEntityId: Cannot find parent entity.");
            return result;
        }
    }
    else
    {
        parentEntity = GetRootElement();
    }

    AZ::Entity* insertBeforeEntity = nullptr;
    if (insertBeforeId.IsValid())
    {
        EBUS_EVENT_RESULT(insertBeforeEntity, AZ::ComponentApplicationBus, FindEntity, insertBeforeId);
        if (!insertBeforeEntity)
        {
            AZ_Warning("UI", false, "CloneElementEntityId: Cannot find insertBefore entity.");
            return result;
        }
    }

    AZ::Entity* clonedEntity = CloneAndAddElementInternal(sourceEntity, parentEntity, insertBeforeEntity);

    if (clonedEntity)
    {
        result = clonedEntity->GetId();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiCanvasComponent::CloneCanvas(const AZ::Vector2& canvasSize)
{
    UiGameEntityContext* entityContext = new UiGameEntityContext();

    UiCanvasComponent* canvasComponent = CloneAndInitializeCanvas(entityContext, m_pathname, &canvasSize);
    AZ::Entity* newCanvasEntity = nullptr;
    if (canvasComponent)
    {
        newCanvasEntity = canvasComponent->GetEntity();
        canvasComponent->m_isLoadedInGame = true;

        // The game entity context needs to know its corresponding canvas entity for instantiating dynamic slices
        entityContext->SetCanvasEntity(newCanvasEntity->GetId());
    }
    else
    {
        delete entityContext;
    }

    return newCanvasEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetCanvasToViewportMatrix(const AZ::Matrix4x4& matrix)
{
    if (!m_canvasToViewportMatrix.IsClose(matrix))
    {
        m_canvasToViewportMatrix = matrix;
        m_viewportToCanvasMatrix = m_canvasToViewportMatrix.GetInverseTransform();
        EBUS_EVENT_ID(GetRootElement()->GetId(), UiTransformBus, SetRecomputeTransformFlag);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const AZ::Matrix4x4& UiCanvasComponent::GetCanvasToViewportMatrix()
{
    return m_canvasToViewportMatrix;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::GetViewportToCanvasMatrix(AZ::Matrix4x4& matrix)
{
    matrix = m_viewportToCanvasMatrix;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiCanvasComponent::GetCanvasSize()
{
    return m_targetCanvasSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetCanvasSize(const AZ::Vector2& canvasSize)
{
    m_canvasSize = canvasSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetTargetCanvasSize(bool isInGame, const AZ::Vector2& targetCanvasSize)
{
    if (m_renderToTexture)
    {
        // when a canvas is set to render to texture the target canvas size is always the authored canvas size
        SetTargetCanvasSizeAndUniformScale(isInGame, m_canvasSize);
    }
    else
    {
        SetTargetCanvasSizeAndUniformScale(isInGame, targetCanvasSize);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiCanvasComponent::GetUniformDeviceScale()
{
    return m_uniformDeviceScale;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::GetIsPixelAligned()
{
    return m_isPixelAligned;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetIsPixelAligned(bool isPixelAligned)
{
    m_isPixelAligned = isPixelAligned;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IUiAnimationSystem* UiCanvasComponent::GetAnimationSystem()
{
    return &m_uiAnimationSystem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::GetEnabled()
{
    return m_enabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetEnabled(bool enabled)
{
    const bool wasEnabled = m_enabled;
    m_enabled = enabled;
    if (!wasEnabled && m_enabled && m_isConsumingAllInputEvents)
    {
        EBUS_EVENT(UiCanvasBus, ClearAllInteractables);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::GetIsRenderToTexture()
{
    return m_renderToTexture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetIsRenderToTexture(bool isRenderToTexture)
{
    m_renderToTexture = isRenderToTexture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiCanvasComponent::GetRenderTargetName()
{
    return m_renderTargetName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetRenderTargetName(const AZStd::string& name)
{
    if (name != m_renderTargetName && !name.empty())
    {
        DestroyRenderTarget();
        m_renderTargetName = name;
        CreateRenderTarget();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::GetIsPositionalInputSupported()
{
    return m_isPositionalInputSupported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetIsPositionalInputSupported(bool isSupported)
{
    m_isPositionalInputSupported = isSupported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::GetIsConsumingAllInputEvents()
{
    return m_isConsumingAllInputEvents;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetIsConsumingAllInputEvents(bool isConsuming)
{
    m_isConsumingAllInputEvents = isConsuming;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::GetIsMultiTouchSupported()
{
    return m_isMultiTouchSupported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetIsMultiTouchSupported(bool isSupported)
{
    m_isMultiTouchSupported = isSupported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::GetIsNavigationSupported()
{
    return m_isNavigationSupported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetIsNavigationSupported(bool isSupported)
{
    m_isNavigationSupported = isSupported;
    SetFirstHoverInteractable();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::HandleInputEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot,
    const AZ::Vector2* viewportPos,
    AzFramework::ModifierKeyMask activeModifierKeys)
{
    // Ignore input events if we're not enabled
    if (!m_enabled)
    {
        return false;
    }

    if (AzFramework::InputDeviceKeyboard::IsKeyboardDevice(inputSnapshot.m_deviceId) ||
        AzFramework::InputDeviceVirtualKeyboard::IsVirtualKeyboardDevice(inputSnapshot.m_deviceId) ||
        AzFramework::InputDeviceGamepad::IsGamepadDevice(inputSnapshot.m_deviceId))
    {
        return HandleKeyInputEvent(inputSnapshot, activeModifierKeys) || m_isConsumingAllInputEvents;
    }
    else if (viewportPos)
    {
        if (!m_renderToTexture && m_isPositionalInputSupported)
        {
            if (HandleInputPositionalEvent(inputSnapshot, *viewportPos))
            {
                return true;
            }
        }
    }

    return m_isConsumingAllInputEvents;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::HandleTextEvent(const AZStd::string& textUTF8)
{
    // Ignore input events if we're not enabled
    if (!m_enabled)
    {
        return false;
    }

    if (m_activeInteractable.IsValid())
    {
        EBUS_EVENT_ID(m_activeInteractable, UiInteractableBus, HandleTextInput, textUTF8);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::HandleInputPositionalEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot,
    AZ::Vector2 viewportPos)
{
    if (AzFramework::InputDeviceMouse::IsMouseDevice(inputSnapshot.m_deviceId))
    {
        if (m_lastMousePosition != viewportPos)
        {
            // Check if the mouse position has been initialized
            if (m_lastMousePosition.GetX() >= 0.0f && m_lastMousePosition.GetY() >= 0.0f)
            {
                // Mouse moved, resume handling hover input events if there is no active interactable
                if (!m_activeInteractable.IsValid())
                {
                    s_handleHoverInputEvents = true;
                }
            }
            m_lastMousePosition = viewportPos;
        }
    }

    // Currently we are just interested in mouse events and the primary touch for hover events
    if (AzFramework::InputDeviceMouse::IsMouseDevice(inputSnapshot.m_deviceId) ||
        inputSnapshot.m_channelId == AzFramework::InputDeviceTouch::Touch::Index0)
    {
        if (s_handleHoverInputEvents)
        {
            HandleHoverInputEvent(viewportPos);
        }
    }

    // Currently we are just interested in mouse button 1 events and UI events here
    if (inputSnapshot.m_channelId == AzFramework::InputDeviceMouse::Button::Left ||
        inputSnapshot.m_channelId == AzFramework::InputDeviceTouch::Touch::Index0)
    {
        if (inputSnapshot.m_state == AzFramework::InputChannel::State::Began)
        {
            return HandlePrimaryPress(viewportPos);
        }
        else if (inputSnapshot.m_state == AzFramework::InputChannel::State::Ended)
        {
            if (inputSnapshot.m_channelId == AzFramework::InputDeviceTouch::Touch::Index0)
            {
                ClearHoverInteractable();
            }
            return HandlePrimaryRelease(viewportPos);
        }
    }
    // ...while all other events from touch devices should be treated as multi-touch
    else if (AzFramework::InputDeviceTouch::IsTouchDevice(inputSnapshot.m_deviceId))
    {
        const auto& touchIndexIt = AZStd::find(AzFramework::InputDeviceTouch::Touch::All.cbegin(),
                                               AzFramework::InputDeviceTouch::Touch::All.cend(),
                                               inputSnapshot.m_channelId);
        if (touchIndexIt != AzFramework::InputDeviceTouch::Touch::All.cend())
        {
            const int touchindex = static_cast<int>(touchIndexIt - AzFramework::InputDeviceTouch::Touch::All.cbegin());
            if (inputSnapshot.m_state == AzFramework::InputChannel::State::Began)
            {
                return HandleMultiTouchPress(viewportPos, touchindex);
            }
            else if (inputSnapshot.m_state == AzFramework::InputChannel::State::Ended)
            {
                return HandleMultiTouchRelease(viewportPos, touchindex);
            }
            else if (inputSnapshot.m_state == AzFramework::InputChannel::State::Updated)
            {
                return HandleMultiTouchUpdated(viewportPos, touchindex);
            }
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiCanvasComponent::GetMousePosition()
{
    return m_lastMousePosition;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasComponent::GetTooltipDisplayElement()
{
    return m_tooltipDisplayElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetTooltipDisplayElement(AZ::EntityId entityId)
{
    m_tooltipDisplayElement = entityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::GetIsSnapEnabled()
{
    return m_isSnapEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetIsSnapEnabled(bool enabled)
{
    m_isSnapEnabled = enabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiCanvasComponent::GetSnapDistance()
{
    return m_snapDistance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetSnapDistance(float distance)
{
    m_snapDistance = distance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiCanvasComponent::GetSnapRotationDegrees()
{
    return m_snapRotationDegrees;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetSnapRotationDegrees(float degrees)
{
    m_snapRotationDegrees = degrees;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::ForceActiveInteractable(AZ::EntityId interactableId, bool shouldStayActive, AZ::Vector2 point)
{
    SetHoverInteractable(interactableId);
    SetActiveInteractable(interactableId, shouldStayActive);
    m_lastMousePosition = point;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasComponent::GetHoverInteractable()
{
    return m_hoverInteractable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::ForceHoverInteractable(AZ::EntityId newHoverInteractable)
{
    SetHoverInteractable(newHoverInteractable);

    if (m_hoverInteractable.IsValid())
    {
        s_handleHoverInputEvents = false;
        m_allowInvalidatingHoverInteractableOnHoverInput = false;

        AZ::EntityId ancestorInteractable = FindAncestorInteractable(m_hoverInteractable);
        if (ancestorInteractable.IsValid())
        {
            // Send an event that the descendant interactable became the hover interactable via navigation
            EBUS_EVENT_ID(ancestorInteractable, UiInteractableBus, HandleDescendantReceivedHoverByNavigation, m_hoverInteractable);
        }

        CheckHoverInteractableAndAutoActivate();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::ClearAllInteractables()
{
    m_multiTouchInteractablesByTouchIndex.clear();
    ClearActiveInteractable();
    ClearHoverInteractable();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetTransformsNeedRecomputeFlag()
{
    m_transformsNeedRecompute = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::OnEntityDeactivated(const AZ::EntityId& entityId)
{
    AZ::EntityBus::Handler::BusDisconnect(entityId);

    if (entityId == m_hoverInteractable)
    {
        ClearHoverInteractable();

        // If we are using keyboard/gamepad navigation we should set a new hover interactable
        SetFirstHoverInteractable();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::StartSequence(const AZStd::string& sequenceName)
{
    IUiAnimSequence* sequence = m_uiAnimationSystem.FindSequence(sequenceName.c_str());
    if (sequence)
    {
        m_uiAnimationSystem.AddUiAnimationListener(sequence, this);
        m_uiAnimationSystem.PlaySequence(sequence, nullptr, false, false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::StopSequence(const AZStd::string& sequenceName)
{
    IUiAnimSequence* sequence = m_uiAnimationSystem.FindSequence(sequenceName.c_str());
    if (sequence)
    {
        m_uiAnimationSystem.StopSequence(sequence);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::AbortSequence(const AZStd::string& sequenceName)
{
    IUiAnimSequence* sequence = m_uiAnimationSystem.FindSequence(sequenceName.c_str());
    if (sequence)
    {
        m_uiAnimationSystem.AbortSequence(sequence);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::PauseSequence(const AZStd::string& sequenceName)
{
    IUiAnimSequence* sequence = m_uiAnimationSystem.FindSequence(sequenceName.c_str());
    if (sequence)
    {
        sequence->Pause();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::ResumeSequence(const AZStd::string& sequenceName)
{
    IUiAnimSequence* sequence = m_uiAnimationSystem.FindSequence(sequenceName.c_str());
    if (sequence)
    {
        sequence->Resume();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::ResetSequence(const AZStd::string& sequenceName)
{
    IUiAnimSequence* sequence = m_uiAnimationSystem.FindSequence(sequenceName.c_str());
    if (sequence)
    {
        sequence->Reset(true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiCanvasComponent::GetSequencePlayingSpeed(const AZStd::string& sequenceName)
{
    IUiAnimSequence* sequence = m_uiAnimationSystem.FindSequence(sequenceName.c_str());
    return m_uiAnimationSystem.GetPlayingSpeed(sequence);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetSequencePlayingSpeed(const AZStd::string& sequenceName, float speed)
{
    IUiAnimSequence* sequence = m_uiAnimationSystem.FindSequence(sequenceName.c_str());
    m_uiAnimationSystem.SetPlayingSpeed(sequence, speed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiCanvasComponent::GetSequencePlayingTime(const AZStd::string& sequenceName)
{
    IUiAnimSequence* sequence = m_uiAnimationSystem.FindSequence(sequenceName.c_str());
    return m_uiAnimationSystem.GetPlayingTime(sequence);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::IsSequencePlaying(const AZStd::string& sequenceName)
{
    IUiAnimSequence* sequence = m_uiAnimationSystem.FindSequence(sequenceName.c_str());
    if (sequence)
    {
        return m_uiAnimationSystem.IsPlaying(sequence);
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::ActiveCancelled()
{
    // currently we are only connected to one UiInteractableActiveNotificationBus so we know it is the
    // pressed interactable. If we could be connected to several we would need to change
    // the ActiveCancelled method to pass the EntityId.
    if (m_activeInteractable.IsValid())
    {
        UiInteractableActiveNotificationBus::Handler::BusDisconnect(m_activeInteractable);
        m_activeInteractable.SetInvalid();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// change the active interactable to the given one
void UiCanvasComponent::ActiveChanged(AZ::EntityId m_newActiveInteractable, bool shouldStayActive)
{
    // There should always be an active interactable at this point, disconnect from it
    if (m_activeInteractable.IsValid())
    {
        UiInteractableActiveNotificationBus::Handler::BusDisconnect(m_activeInteractable);
        m_activeInteractable.SetInvalid();
    }

    // The m_newActiveInteractable should always be valid but check anyway
    if (m_newActiveInteractable.IsValid())
    {
        m_activeInteractable = m_newActiveInteractable;
        UiInteractableActiveNotificationBus::Handler::BusConnect(m_activeInteractable);
        m_activeInteractableShouldStayActive = shouldStayActive;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::OnPreRender()
{
    bool displayBounds = false;
#ifndef EXCLUDE_DOCUMENTATION_PURPOSE
    // If the console variable is set then display the element bounds
    // We use deferred render for the bounds so that they draw on top of everything else
    // this only works when running in-game
    if (CV_ui_DisplayElemBounds)
    {
        displayBounds = true;
    }
#endif

    RenderCanvasToTexture(displayBounds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::OnUiAnimationEvent(EUiAnimationEvent uiAnimationEvent, IUiAnimSequence* pAnimSequence)
{
    // Queue the event to prevent deletions during the canvas update
    EBUS_QUEUE_EVENT_ID(GetEntityId(), UiAnimationNotificationBus, OnUiAnimationEvent, uiAnimationEvent, pAnimSequence->GetName());

    // Stop listening to events
    if ((uiAnimationEvent == EUiAnimationEvent::eUiAnimationEvent_Stopped) || (uiAnimationEvent == EUiAnimationEvent::eUiAnimationEvent_Aborted))
    {
        m_uiAnimationSystem.RemoveUiAnimationListener(pAnimSequence, this);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiCanvasComponent::GetRootElement() const
{
    AZ::Entity* rootEntity = nullptr;
    EBUS_EVENT_RESULT(rootEntity, AZ::ComponentApplicationBus, FindEntity, m_rootElement);
    return rootEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::ElementId UiCanvasComponent::GenerateId()
{
    return ++m_lastElementId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Vector2 UiCanvasComponent::GetTargetCanvasSize()
{
    return m_targetCanvasSize;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiCanvasComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        UiAnimationSystem::Reflect(serializeContext);

        serializeContext->Class<UiCanvasComponent, AZ::Component>()
            ->Version(3, &VersionConverter)
        // Not in properties pane
            ->Field("UniqueId", &UiCanvasComponent::m_uniqueId)
            ->Field("RootElement", &UiCanvasComponent::m_rootElement)
            ->Field("LastElement", &UiCanvasComponent::m_lastElementId)
            ->Field("CanvasSize", &UiCanvasComponent::m_canvasSize)
            ->Field("IsSnapEnabled", &UiCanvasComponent::m_isSnapEnabled)
        // Rendering group
            ->Field("DrawOrder", &UiCanvasComponent::m_drawOrder)
            ->Field("IsPixelAligned", &UiCanvasComponent::m_isPixelAligned)
            ->Field("RenderToTexture", &UiCanvasComponent::m_renderToTexture)
            ->Field("RenderTargetName", &UiCanvasComponent::m_renderTargetName)
        // Input group
            ->Field("IsPosInputSupported", &UiCanvasComponent::m_isPositionalInputSupported)
            ->Field("IsConsumingAllInput", &UiCanvasComponent::m_isConsumingAllInputEvents)
            ->Field("IsMultiTouchSupported", &UiCanvasComponent::m_isMultiTouchSupported)
            ->Field("IsNavigationSupported", &UiCanvasComponent::m_isNavigationSupported)
            ->Field("FirstHoverElement", &UiCanvasComponent::m_firstHoverInteractable)
            ->Field("AnimSystem", &UiCanvasComponent::m_uiAnimationSystem)
            ->Field("AnimationData", &UiCanvasComponent::m_serializedAnimationData)
        // Tooltips group
            ->Field("TooltipDisplayElement", &UiCanvasComponent::m_tooltipDisplayElement)
        // Editor settings
            ->Field("SnapDistance", &UiCanvasComponent::m_snapDistance)
            ->Field("SnapRotationDegrees", &UiCanvasComponent::m_snapRotationDegrees);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiCanvasComponent>("UI Canvas", "These are the properties of the UI canvas.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AddableByUser, false)
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiCanvas.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiCanvas.png")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Rendering")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::Default, &UiCanvasComponent::m_drawOrder, "Draw order", "The order, relative to other canvases, in which this canvas will draw (higher numbers on top).")
                ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1000.0f);
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiCanvasComponent::m_isPixelAligned, "Is pixel aligned", "When checked, all corners of all elements will be rounded to the nearest pixel.");
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiCanvasComponent::m_renderToTexture, "Render to texture", "When checked, the canvas is rendered to a texture instead of the full screen.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
            editInfo->DataElement(0, &UiCanvasComponent::m_renderTargetName, "Render target", "The name of the texture that is created when this canvas renders to a texture.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiCanvasComponent::m_renderToTexture);

            editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Input")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiCanvasComponent::m_isPositionalInputSupported, "Handle positional", "When checked, positional input (mouse/touch) will automatically be handled.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c));
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiCanvasComponent::m_isConsumingAllInputEvents, "Consume all input", "When checked, all input events will be consumed by this canvas while it is enabled.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiCanvasComponent::GetIsPositionalInputSupported);
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiCanvasComponent::m_isMultiTouchSupported, "Handle multi-touch", "When checked, multi-touch input will automatically be handled.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiCanvasComponent::GetIsPositionalInputSupported);
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiCanvasComponent::m_isNavigationSupported, "Handle navigation", "When checked, keyboard/gamepad events will automatically be used for navigation.");
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiCanvasComponent::m_firstHoverInteractable, "First focus elem", "The element to receive focus when the canvas loads.")
                ->Attribute("EnumValues", &UiCanvasComponent::PopulateNavigableEntityList);

            editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Tooltips")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiCanvasComponent::m_tooltipDisplayElement, "Tooltip display elem", "The element to be displayed when hovering over an interactable.")
                ->Attribute("EnumValues", &UiCanvasComponent::PopulateTooltipDisplayEntityList);

            editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Editor settings")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::Default, &UiCanvasComponent::m_snapDistance, "Snap distance", "The snap grid spacing.")
                ->Attribute(AZ::Edit::Attributes::Min, 1.0f);
            editInfo->DataElement(AZ::Edit::UIHandlers::Default, &UiCanvasComponent::m_snapRotationDegrees, "Snap rotation", "The degrees of rotation to snap to.")
                ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 359.0f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " degrees");
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiCanvasBus>("UiCanvasBus")
            ->Event("GetDrawOrder", &UiCanvasBus::Events::GetDrawOrder)
            ->Event("SetDrawOrder", &UiCanvasBus::Events::SetDrawOrder)
            ->Event("GetKeepLoadedOnLevelUnload", &UiCanvasBus::Events::GetKeepLoadedOnLevelUnload)
            ->Event("SetKeepLoadedOnLevelUnload", &UiCanvasBus::Events::SetKeepLoadedOnLevelUnload)
            ->Event("RecomputeChangedLayouts", &UiCanvasBus::Events::RecomputeChangedLayouts)
            ->Event("GetNumChildElements", &UiCanvasBus::Events::GetNumChildElements)
            ->Event("GetChildElement", &UiCanvasBus::Events::GetChildElementEntityId)
            ->Event("GetChildElements", &UiCanvasBus::Events::GetChildElementEntityIds)
            ->Event("FindElementByName", &UiCanvasBus::Events::FindElementEntityIdByName)
            ->Event("CloneElement", &UiCanvasBus::Events::CloneElementEntityId)
            ->Event("GetIsPixelAligned", &UiCanvasBus::Events::GetIsPixelAligned)
            ->Event("SetIsPixelAligned", &UiCanvasBus::Events::SetIsPixelAligned)
            ->Event("GetEnabled", &UiCanvasBus::Events::GetEnabled)
            ->Event("SetEnabled", &UiCanvasBus::Events::SetEnabled)
            ->Event("GetIsRenderToTexture", &UiCanvasBus::Events::GetIsRenderToTexture)
            ->Event("SetIsRenderToTexture", &UiCanvasBus::Events::SetIsRenderToTexture)
            ->Event("GetRenderTargetName", &UiCanvasBus::Events::GetRenderTargetName)
            ->Event("SetRenderTargetName", &UiCanvasBus::Events::SetRenderTargetName)
            ->Event("GetIsPositionalInputSupported", &UiCanvasBus::Events::GetIsPositionalInputSupported)
            ->Event("SetIsPositionalInputSupported", &UiCanvasBus::Events::SetIsPositionalInputSupported)
            ->Event("GetIsConsumingAllInputEvents", &UiCanvasBus::Events::GetIsConsumingAllInputEvents)
            ->Event("SetIsConsumingAllInputEvents", &UiCanvasBus::Events::SetIsConsumingAllInputEvents)
            ->Event("GetIsMultiTouchSupported", &UiCanvasBus::Events::GetIsMultiTouchSupported)
            ->Event("SetIsMultiTouchSupported", &UiCanvasBus::Events::SetIsMultiTouchSupported)
            ->Event("GetIsNavigationSupported", &UiCanvasBus::Events::GetIsNavigationSupported)
            ->Event("SetIsNavigationSupported", &UiCanvasBus::Events::SetIsNavigationSupported)
            ->Event("GetTooltipDisplayElement", &UiCanvasBus::Events::GetTooltipDisplayElement)
            ->Event("SetTooltipDisplayElement", &UiCanvasBus::Events::SetTooltipDisplayElement)
            ->Event("GetHoverInteractable", &UiCanvasBus::Events::GetHoverInteractable)
            ->Event("ForceHoverInteractable", &UiCanvasBus::Events::ForceHoverInteractable);

        behaviorContext->EBus<UiCanvasNotificationBus>("UiCanvasNotificationBus")
            ->Handler<UiCanvasNotificationBusBehaviorHandler>();

        behaviorContext->EBus<UiAnimationBus>("UiAnimationBus")
            ->Event("StartSequence", &UiAnimationBus::Events::StartSequence)
            ->Event("StopSequence", &UiAnimationBus::Events::StopSequence)
            ->Event("AbortSequence", &UiAnimationBus::Events::AbortSequence)
            ->Event("PauseSequence", &UiAnimationBus::Events::PauseSequence)
            ->Event("ResumeSequence", &UiAnimationBus::Events::ResumeSequence)
            ->Event("ResetSequence", &UiAnimationBus::Events::ResetSequence)
            ->Event("GetSequencePlayingSpeed", &UiAnimationBus::Events::GetSequencePlayingSpeed)
            ->Event("SetSequencePlayingSpeed", &UiAnimationBus::Events::SetSequencePlayingSpeed)
            ->Event("GetSequencePlayingTime", &UiAnimationBus::Events::GetSequencePlayingTime)
            ->Event("IsSequencePlaying", &UiAnimationBus::Events::IsSequencePlaying);

        behaviorContext->Enum<(int)IUiAnimationListener::EUiAnimationEvent::eUiAnimationEvent_Started>("eUiAnimationEvent_Started")
            ->Enum<(int)IUiAnimationListener::EUiAnimationEvent::eUiAnimationEvent_Stopped>("eUiAnimationEvent_Stopped")
            ->Enum<(int)IUiAnimationListener::EUiAnimationEvent::eUiAnimationEvent_Aborted>("eUiAnimationEvent_Aborted")
            ->Enum<(int)IUiAnimationListener::EUiAnimationEvent::eUiAnimationEvent_Updated>("eUiAnimationEvent_Updated");

        behaviorContext->EBus<UiAnimationNotificationBus>("UiAnimationNotificationBus")
            ->Handler<UiAnimationNotificationBusBehaviorHandler>();

        behaviorContext->EBus<UiCanvasInputNotificationBus>("UiCanvasInputNotificationBus")
            ->Handler<UiCanvasInputNotificationBusBehaviorHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::Initialize()
{
    s_handleHoverInputEvents = true;

#ifndef EXCLUDE_DOCUMENTATION_PURPOSE
    // Define a debug console variable that controls display of all element bounds when in game
    DefineConstIntCVar3("ui_DisplayElemBounds", CV_ui_DisplayElemBounds, 0, VF_CHEAT,
        "0=off, 1=display the UI element bounding boxes");
#endif // EXCLUDE_DOCUMENTATION_PURPOSE
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::Shutdown()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiCanvasComponent::Init()
{
    // We don't know whether we're in editor or game yet, but if we're in the editor
    // we need to know the authored canvas size to ensure certain properties are displayed
    // correctly in the editor window. If we're in game, the target canvas size will be
    // initialized to the viewport on the first render loop.
    m_targetCanvasSize = m_canvasSize;

    if (m_uniqueId == 0)
    {
        // Initialize unique Id
        m_uniqueId = CreateUniqueId();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::Activate()
{
    UiCanvasBus::Handler::BusConnect(m_entity->GetId());
    UiAnimationBus::Handler::BusConnect(m_entity->GetId());

    // Reconnect to buses that we connect to intermittently
    // This will only happen if we have been deactivated and reactivated at runtime
    if (m_hoverInteractable.IsValid())
    {
        AZ::EntityBus::Handler::BusConnect(m_hoverInteractable);
    }
    if (m_activeInteractable.IsValid())
    {
        UiInteractableActiveNotificationBus::Handler::BusConnect(m_activeInteractable);
    }

    // Note: this will create a render target even when the canvas is being used in the editor which is
    // unnecessary but harmless. It will not actually be used as a render target unless we are running in game.
    // An alternative would be to create in on first use.
    if (m_renderToTexture)
    {
        CreateRenderTarget();
    }

    m_layoutManager = new UiLayoutManager(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::Deactivate()
{
    UiCanvasBus::Handler::BusDisconnect();
    UiAnimationBus::Handler::BusDisconnect();

    // disconnect from any other buses we could be connected to
    if (m_hoverInteractable.IsValid() && AZ::EntityBus::Handler::BusIsConnectedId(m_hoverInteractable))
    {
        AZ::EntityBus::Handler::BusDisconnect(m_hoverInteractable);
    }
    if (m_activeInteractable.IsValid() && UiInteractableActiveNotificationBus::Handler::BusIsConnectedId(m_activeInteractable))
    {
        UiInteractableActiveNotificationBus::Handler::BusDisconnect(m_activeInteractable);
    }

    m_multiTouchInteractablesByTouchIndex.clear();

    if (m_renderToTexture)
    {
        DestroyRenderTarget();
    }

    delete m_layoutManager;
    m_layoutManager = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::HandleHoverInputEvent(AZ::Vector2 point)
{
    bool result = false;

    // We don't change the active interactable here. Some interactables may want to still be considered
    // pressed if the mouse moves outside their bounds while they are pressed.

    // However, the active interactable does influence how hover works, if there is an active
    // interactable then that is the only one that can be the hoverInteractable
    AZ::EntityId latestHoverInteractable;
    if (m_activeInteractable.IsValid())
    {
        // check if the mouse is hovering over the active interactable
        bool hoveringOnActive = false;
        EBUS_EVENT_ID_RESULT(hoveringOnActive, m_activeInteractable, UiTransformBus, IsPointInRect, point);

        if (hoveringOnActive)
        {
            latestHoverInteractable = m_activeInteractable;
        }
    }
    else
    {
        // there is no active interactable
        // find the interactable that the mouse is hovering over (if any)
        EBUS_EVENT_ID_RESULT(latestHoverInteractable, m_rootElement, UiElementBus, FindInteractableToHandleEvent, point);
    }

    if (latestHoverInteractable.IsValid())
    {
        m_allowInvalidatingHoverInteractableOnHoverInput = true;
    }

    if (m_hoverInteractable.IsValid() && m_hoverInteractable != latestHoverInteractable)
    {
        // we were hovering over an interactable but now we are hovering over nothing or a different interactable
        if (m_allowInvalidatingHoverInteractableOnHoverInput)
        {
            ClearHoverInteractable();
        }
    }

    if (latestHoverInteractable.IsValid() && !m_hoverInteractable.IsValid())
    {
        // we are now hovering over something and we aren't tracking that yet
        SetHoverInteractable(latestHoverInteractable);

        EBUS_EVENT_ID_RESULT(result, m_hoverInteractable, UiInteractableBus, IsHandlingEvents);
    }

    // if there is an active interactable then we send mouse position updates to that interactable
    if (m_activeInteractable.IsValid())
    {
        EBUS_EVENT_ID(m_activeInteractable, UiInteractableBus, InputPositionUpdate, point);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::HandleKeyInputEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot, AzFramework::ModifierKeyMask activeModifierKeys)
{
    bool result = false;

    // Allow the active interactable to handle the key input first
    if (m_activeInteractable.IsValid())
    {
        if (inputSnapshot.m_state == AzFramework::InputChannel::State::Began ||
            AzFramework::InputDeviceVirtualKeyboard::IsVirtualKeyboardDevice(inputSnapshot.m_deviceId)) // Virtual keyboard events don't have state
        {
            EBUS_EVENT_ID_RESULT(result, m_activeInteractable, UiInteractableBus, HandleKeyInputBegan, inputSnapshot, activeModifierKeys);
        }
    }

    if (!result && m_isNavigationSupported)
    {
        const UiNavigationHelpers::Command command = UiNavigationHelpers::MapInputChannelIdToUiNavigationCommand(inputSnapshot.m_channelId, activeModifierKeys);

        if (command != UiNavigationHelpers::Command::Unknown)
        {
            // Handle directional navigation input. Navigation is performed if there is no active interactable, or if the
            // active interactable is not pressed and is set to auto-activate

            bool handleDirectionalNavigation = false;
            if (!m_activeInteractable.IsValid())
            {
                handleDirectionalNavigation = true;
            }
            else if (!m_isActiveInteractablePressed)
            {
                // Check if the active interactable automatically goes to an active state
                EBUS_EVENT_ID_RESULT(handleDirectionalNavigation, m_activeInteractable, UiInteractableBus, GetIsAutoActivationEnabled);
            }

            if (handleDirectionalNavigation)
            {
                AZ::EntityId oldHoverInteractable = m_hoverInteractable;
                result = HandleNavigationInputEvent(command, inputSnapshot);
                if (m_hoverInteractable != oldHoverInteractable)
                {
                    s_handleHoverInputEvents = false;
                    m_allowInvalidatingHoverInteractableOnHoverInput = false;

                    AZ::EntityId ancestorInteractable = FindAncestorInteractable(m_hoverInteractable);
                    if (ancestorInteractable.IsValid())
                    {
                        // Send an event that the descendant interactable became the hover interactable via navigation
                        EBUS_EVENT_ID(ancestorInteractable, UiInteractableBus, HandleDescendantReceivedHoverByNavigation, m_hoverInteractable);
                    }

                    ClearActiveInteractable();

                    // Check if this hover interactable should automatically go to an active state
                    CheckHoverInteractableAndAutoActivate(oldHoverInteractable, command);
                }
            }

            if (!result)
            {
                // Handle enter input
                result = HandleEnterInputEvent(command, inputSnapshot);
            }

            if (!result)
            {
                // Handle back input
                result = HandleBackInputEvent(command, inputSnapshot);
            }

            if (!result)
            {
                // If there is any active or hover interactable then we consider this event handled.
                // Otherwise we can end up sending events to underlying canvases even though there
                // is an interactable in this canvas that should block the events
                if (m_activeInteractable.IsValid() || m_hoverInteractable.IsValid())
                {
                    result = true;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::HandleEnterInputEvent(UiNavigationHelpers::Command command, const AzFramework::InputChannel::Snapshot& inputSnapshot)
{
    bool result = false;

    if (command == UiNavigationHelpers::Command::Enter)
    {
        // The key is the Enter key. If there is any active or hover interactable then we consider this event handled.
        // Otherwise we can end up sending Enter events to underlying canvases even though there is an interactable
        // in this canvas that should block the events
        if (m_activeInteractable.IsValid() || m_hoverInteractable.IsValid())
        {
            result = true;
        }

        if (inputSnapshot.m_state == AzFramework::InputChannel::State::Began)
        {
            AZ::EntityId prevHoverInteractable = m_hoverInteractable;

            // Enter key was pressed. The press can activate an interactable and also deactivate an interactable

            // Check if there's an interactable to deactivate
            if (m_activeInteractable.IsValid() && m_activeInteractableShouldStayActive)
            {
                DeactivateInteractableByKeyInput(inputSnapshot);
            }
            else
            {
                // Check if there's a hover interactable to make active
                if (m_hoverInteractable.IsValid())
                {
                    // clear any active interactable
                    ClearActiveInteractable();

                    // if the hover interactable can handle enter pressed events then
                    // it becomes the currently pressed interactable for the canvas
                    bool handled = false;
                    bool shouldStayActive = false;
                    EBUS_EVENT_ID_RESULT(handled, m_hoverInteractable, UiInteractableBus, HandleEnterPressed, shouldStayActive);

                    if (handled)
                    {
                        SetActiveInteractable(m_hoverInteractable, shouldStayActive);

                        s_handleHoverInputEvents = false;
                        m_allowInvalidatingHoverInteractableOnHoverInput = false;

                        m_isActiveInteractablePressed = true;
                    }
                }
            }

            // Send a notification to listeners telling them who was just pressed (can be noone)
            EBUS_EVENT_ID(GetEntityId(), UiCanvasInputNotificationBus, OnCanvasEnterPressed, prevHoverInteractable);
        }
        else if (inputSnapshot.m_state == AzFramework::InputChannel::State::Ended)
        {
            AZ::EntityId prevActiveInteractable = m_activeInteractable;

            // Enter key has been released. Check if the active interactable should stay active
            if (m_activeInteractable.IsValid() && (m_activeInteractable == m_hoverInteractable))
            {
                EBUS_EVENT_ID(m_activeInteractable, UiInteractableBus, HandleEnterReleased);

                if (!m_activeInteractableShouldStayActive)
                {
                    ClearActiveInteractable();
                }
                else
                {
                    // Interactable should stay active, so check if if it has a descendant interactable
                    // that it should pass the hover to
                    CheckActiveInteractableAndPassHoverToDescendant();
                }

                m_isActiveInteractablePressed = false;
            }

            // Send a notification to listeners telling them who was just released (can be noone)
            EBUS_EVENT_ID(GetEntityId(), UiCanvasInputNotificationBus, OnCanvasEnterReleased, prevActiveInteractable);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::HandleBackInputEvent(UiNavigationHelpers::Command command, const AzFramework::InputChannel::Snapshot& inputSnapshot)
{
    bool result = false;

    if (command == UiNavigationHelpers::Command::Back)
    {
        if (inputSnapshot.m_state == AzFramework::InputChannel::State::Began)
        {
            // Back has two purposes:
            // 1. If there is an active interactable, and it's not set to auto-activate, pressing back deactivates the interactable
            // 2. If there is a hover interactable, and it's a child of another interactable, then pressing back moves focus from the child
            // to the parent

            // First check if there is an active interactable to deactivate
            if (m_activeInteractable.IsValid())
            {
                // Deactivate this interactable
                result = DeactivateInteractableByKeyInput(inputSnapshot);
            }
            else if (m_hoverInteractable.IsValid())
            {
                result = PassHoverToAncestorByKeyInput(inputSnapshot);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::HandleNavigationInputEvent(UiNavigationHelpers::Command command, const AzFramework::InputChannel::Snapshot& inputSnapshot)
{
    bool result = false;

    if (command == UiNavigationHelpers::Command::Up ||
        command == UiNavigationHelpers::Command::Down ||
        command == UiNavigationHelpers::Command::Left ||
        command == UiNavigationHelpers::Command::Right)
    {
        if (inputSnapshot.m_state == AzFramework::InputChannel::State::Began)
        {
            AZ::EntityId firstHoverInteractable = GetFirstHoverInteractable();

            // Find the interactable to navigate to
            if (!m_hoverInteractable.IsValid())
            {
                SetHoverInteractable(firstHoverInteractable);
            }
            else
            {
                AZ::EntityId curInteractable = m_hoverInteractable;
                while (curInteractable.IsValid())
                {
                    AZ::EntityId ancestorInteractable = UiNavigationHelpers::FindAncestorNavigableInteractable(curInteractable);

                    LyShine::EntityArray navigableElements;
                    UiNavigationHelpers::FindNavigableInteractables(ancestorInteractable.IsValid() ? ancestorInteractable : m_rootElement, curInteractable, navigableElements);

                    auto isValidInteractable = [](AZ::EntityId entityId)
                        {
                            bool isEnabled = false;
                            EBUS_EVENT_ID_RESULT(isEnabled, entityId, UiElementBus, IsEnabled);

                            bool canHandleEvents = false;
                            if (isEnabled)
                            {
                                EBUS_EVENT_ID_RESULT(canHandleEvents, entityId, UiInteractableBus, IsHandlingEvents);
                            }

                            return canHandleEvents;
                        };

                    AZ::EntityId nextEntityId = UiNavigationHelpers::GetNextElement(curInteractable, command,
                            navigableElements, firstHoverInteractable, isValidInteractable, ancestorInteractable);

                    if (nextEntityId.IsValid())
                    {
                        SetHoverInteractable(nextEntityId);
                        break;
                    }
                    else
                    {
                        // Check if parent interactable was auto-activated
                        bool autoActivated = false;
                        EBUS_EVENT_ID_RESULT(autoActivated, ancestorInteractable, UiInteractableBus, GetIsAutoActivationEnabled);
                        if (autoActivated)
                        {
                            curInteractable = ancestorInteractable;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }

            result = m_hoverInteractable.IsValid();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::DeactivateInteractableByKeyInput(const AzFramework::InputChannel::Snapshot& inputSnapshot)
{
    // Check if the active interactable automatically went to an active state. If it did
    // not automatically go into its active state, then we deactivate the active interactable.
    // Otherwise, the only way to deactivate the interactable is by navigating away from it
    // using the directional keys
    bool autoActivated = false;
    EBUS_EVENT_ID_RESULT(autoActivated, m_activeInteractable, UiInteractableBus, GetIsAutoActivationEnabled);

    if (!autoActivated)
    {
        // Clear the active interactable
        AZ::EntityId prevActiveInteractable = m_activeInteractable;
        ClearActiveInteractable();

        if (AzFramework::InputDeviceGamepad::IsGamepadDevice(inputSnapshot.m_deviceId))
        {
            SetHoverInteractable(prevActiveInteractable);

            s_handleHoverInputEvents = false;
            m_allowInvalidatingHoverInteractableOnHoverInput = false;
        }

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::PassHoverToAncestorByKeyInput(const AzFramework::InputChannel::Snapshot& inputSnapshot)
{
    bool result = false;

    // Check if the hover interactable has an ancestor that's also an interactable
    AZ::EntityId ancestorInteractable = UiNavigationHelpers::FindAncestorNavigableInteractable(m_hoverInteractable, true);
    if (ancestorInteractable.IsValid())
    {
        AZ::EntityId descendantInteractable = m_hoverInteractable;

        SetHoverInteractable(ancestorInteractable);

        EBUS_EVENT_ID(ancestorInteractable, UiInteractableBus, HandleReceivedHoverByNavigatingFromDescendant, descendantInteractable);

        result = true;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::HandlePrimaryPress(AZ::Vector2 point)
{
    bool result = false;

    // use the pressed position to select the interactable being pressed
    AZ::EntityId interactableEntity;
    EBUS_EVENT_ID_RESULT(interactableEntity, m_rootElement, UiElementBus, FindInteractableToHandleEvent, point);

    // Clear the previous active interactable if it's different from the new active interactable
    if (!interactableEntity.IsValid() || (interactableEntity != m_activeInteractable))
    {
        if (m_activeInteractable.IsValid())
        {
            ClearActiveInteractable();
        }
    }

    if (interactableEntity.IsValid() && !IsInteractableActiveOrPressed(interactableEntity))
    {
        // if there is an interactable at that point and it can handle pressed events then
        // it becomes the currently pressed interactable for the canvas
        bool handled = false;
        bool shouldStayActive = false;
        EBUS_EVENT_ID_RESULT(handled, interactableEntity, UiInteractableBus, HandlePressed, point, shouldStayActive);

        if (handled)
        {
            SetActiveInteractable(interactableEntity, shouldStayActive);
            m_isActiveInteractablePressed = true;
            result = true;
        }
    }

    // Resume handling hover input events
    s_handleHoverInputEvents = true;
    m_allowInvalidatingHoverInteractableOnHoverInput = true;

    // Send a notification to listeners telling them who was just pressed (can be noone)
    EBUS_EVENT_ID(GetEntityId(), UiCanvasInputNotificationBus, OnCanvasPrimaryPressed, interactableEntity);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::HandlePrimaryRelease(AZ::Vector2 point)
{
    bool result = false;

    AZ::EntityId prevActiveInteractable = m_activeInteractable;

    // touch was released, if there is a currently pressed interactable let it handle the release
    if (m_activeInteractable.IsValid())
    {
        EBUS_EVENT_ID(m_activeInteractable, UiInteractableBus, HandleReleased, point);

        if (!m_activeInteractableShouldStayActive)
        {
            UiInteractableActiveNotificationBus::Handler::BusDisconnect(m_activeInteractable);
            m_activeInteractable.SetInvalid();
        }

        m_isActiveInteractablePressed = false;

        result = true;
    }

    // Send a notification to listeners telling them who was just released
    EBUS_EVENT_ID(GetEntityId(), UiCanvasInputNotificationBus, OnCanvasPrimaryReleased, prevActiveInteractable);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::HandleMultiTouchPress(AZ::Vector2 point, int multiTouchIndex)
{
    bool result = false;

    if (m_isMultiTouchSupported)
    {
        AZ::EntityId interactableEntity;
        EBUS_EVENT_ID_RESULT(interactableEntity, m_rootElement, UiElementBus, FindInteractableToHandleEvent, point);

        if (interactableEntity.IsValid() && !IsInteractableActiveOrPressed(interactableEntity))
        {
            EBUS_EVENT_ID_RESULT(result, interactableEntity, UiInteractableBus, HandleMultiTouchPressed, point, multiTouchIndex);
            if (result)
            {
                m_multiTouchInteractablesByTouchIndex[multiTouchIndex] = interactableEntity;
            }
        }

        // Send a notification to listeners telling them who was just pressed (can be noone)
        EBUS_EVENT_ID(GetEntityId(), UiCanvasInputNotificationBus, OnCanvasMultiTouchPressed, interactableEntity, multiTouchIndex);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::HandleMultiTouchRelease(AZ::Vector2 point, int multiTouchIndex)
{
    bool result = false;

    if (m_isMultiTouchSupported)
    {
        // Get the corresponding interactable from the map before removing it. It should always already
        // exist in the map, but if not this will just insert an invalid entity id then remove it again
        AZ::EntityId multiTouchInteractable = m_multiTouchInteractablesByTouchIndex[multiTouchIndex];
        m_multiTouchInteractablesByTouchIndex.erase(multiTouchIndex);

        if (multiTouchInteractable.IsValid())
        {
            EBUS_EVENT_ID(multiTouchInteractable, UiInteractableBus, HandleMultiTouchReleased, point, multiTouchIndex);
            result = true;
        }

        // Send a notification to listeners telling them who was just released
        EBUS_EVENT_ID(GetEntityId(), UiCanvasInputNotificationBus, OnCanvasMultiTouchReleased, multiTouchInteractable, multiTouchIndex);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::HandleMultiTouchUpdated(AZ::Vector2 point, int multiTouchIndex)
{
    bool result = false;

    if (m_isMultiTouchSupported)
    {
        auto it = m_multiTouchInteractablesByTouchIndex.find(multiTouchIndex);
        if (it != m_multiTouchInteractablesByTouchIndex.end() && it->second.IsValid())
        {
            EBUS_EVENT_ID(it->second, UiInteractableBus, MultiTouchPositionUpdate, point, multiTouchIndex);
            result = true;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::IsInteractableActiveOrPressed(AZ::EntityId interactableId) const
{
    if (interactableId == m_activeInteractable)
    {
        return true;
    }

    for (const auto& multiTouchInteractableByTouchIndex : m_multiTouchInteractablesByTouchIndex)
    {
        if (interactableId == multiTouchInteractableByTouchIndex.second)
        {
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetHoverInteractable(AZ::EntityId newHoverInteractable)
{
    if (m_hoverInteractable != newHoverInteractable)
    {
        ClearHoverInteractable();

        m_hoverInteractable = newHoverInteractable;
        if (m_hoverInteractable.IsValid())
        {
            EBUS_EVENT_ID(m_hoverInteractable, UiInteractableBus, HandleHoverStart);
            EBUS_EVENT_ID(GetEntityId(), UiCanvasInputNotificationBus, OnCanvasHoverStart, m_hoverInteractable);

            // we want to know if this entity is deactivated or destroyed
            // (unlikely: while hovered over we can't be in edit mode, could happen from C++ interface though)
            AZ::EntityBus::Handler::BusConnect(m_hoverInteractable);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::ClearHoverInteractable()
{
    if (m_hoverInteractable.IsValid())
    {
        EBUS_EVENT_ID(m_hoverInteractable, UiInteractableBus, HandleHoverEnd);
        EBUS_EVENT_ID(GetEntityId(), UiCanvasInputNotificationBus, OnCanvasHoverEnd, m_hoverInteractable);
        AZ::EntityBus::Handler::BusDisconnect(m_hoverInteractable);
        m_hoverInteractable.SetInvalid();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetActiveInteractable(AZ::EntityId newActiveInteractable, bool shouldStayActive)
{
    if (m_activeInteractable != newActiveInteractable)
    {
        ClearActiveInteractable();

        m_activeInteractable = newActiveInteractable;
        if (m_activeInteractable.IsValid())
        {
            UiInteractableActiveNotificationBus::Handler::BusConnect(m_activeInteractable);
            m_activeInteractableShouldStayActive = shouldStayActive;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::ClearActiveInteractable()
{
    if (m_activeInteractable.IsValid())
    {
        EBUS_EVENT_ID(m_activeInteractable, UiInteractableBus, LostActiveStatus);
        UiInteractableActiveNotificationBus::Handler::BusDisconnect(m_activeInteractable);
        m_activeInteractable.SetInvalid();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::CheckHoverInteractableAndAutoActivate(AZ::EntityId prevHoverInteractable, UiNavigationHelpers::Command command)
{
    // Check if this hover interactable should automatically go to an active state
    bool autoActivate = false;
    EBUS_EVENT_ID_RESULT(autoActivate, m_hoverInteractable, UiInteractableBus, GetIsAutoActivationEnabled);
    if (autoActivate)
    {
        bool handled = false;
        EBUS_EVENT_ID_RESULT(handled, m_hoverInteractable, UiInteractableBus, HandleAutoActivation);

        if (handled)
        {
            SetActiveInteractable(m_hoverInteractable, true);
            CheckActiveInteractableAndPassHoverToDescendant(prevHoverInteractable, command);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::CheckActiveInteractableAndPassHoverToDescendant(AZ::EntityId prevHoverInteractable, UiNavigationHelpers::Command command)
{
    AZ::EntityId hoverInteractable;
    if (prevHoverInteractable.IsValid())
    {
        LyShine::EntityArray navigableElements;
        UiNavigationHelpers::FindNavigableInteractables(m_activeInteractable, AZ::EntityId(), navigableElements);

        if (!navigableElements.empty())
        {
            hoverInteractable = UiNavigationHelpers::SearchForNextElement(prevHoverInteractable, command, navigableElements, m_activeInteractable);
        }
    }

    if (!hoverInteractable.IsValid())
    {
        hoverInteractable = FindFirstHoverInteractable(m_activeInteractable);
    }

    if (hoverInteractable.IsValid())
    {
        // Send an event that the descendant interactable became the hover interactable via navigation
        EBUS_EVENT_ID(m_activeInteractable, UiInteractableBus, HandleDescendantReceivedHoverByNavigation, hoverInteractable);

        ClearActiveInteractable();
        SetHoverInteractable(hoverInteractable);
        CheckHoverInteractableAndAutoActivate(prevHoverInteractable, command);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasComponent::FindAncestorInteractable(AZ::EntityId entityId)
{
    AZ::EntityId parent;
    EBUS_EVENT_ID_RESULT(parent, entityId, UiElementBus, GetParentEntityId);
    while (parent.IsValid())
    {
        if (UiInteractableBus::FindFirstHandler(parent))
        {
            break;
        }

        AZ::EntityId newParent = parent;
        parent.SetInvalid();
        EBUS_EVENT_ID_RESULT(parent, newParent, UiElementBus, GetParentEntityId);
    }

    return parent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasComponent::GetFirstHoverInteractable()
{
    AZ::EntityId hoverInteractable;

    if (m_firstHoverInteractable.IsValid())
    {
        // Make sure that this interactable exists
        AZ::Entity* hoverEntity = nullptr;
        EBUS_EVENT_RESULT(hoverEntity, AZ::ComponentApplicationBus, FindEntity, m_firstHoverInteractable);

        if (hoverEntity)
        {
            if (UiNavigationHelpers::IsElementInteractableAndNavigable(m_firstHoverInteractable))
            {
                hoverInteractable = m_firstHoverInteractable;
            }
        }
    }

    if (!hoverInteractable.IsValid())
    {
        hoverInteractable = FindFirstHoverInteractable();
    }

    return hoverInteractable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasComponent::FindFirstHoverInteractable(AZ::EntityId parentElement)
{
    if (!parentElement.IsValid())
    {
        parentElement = m_rootElement;
    }

    LyShine::EntityArray navigableElements;
    UiNavigationHelpers::FindNavigableInteractables(parentElement, AZ::EntityId(), navigableElements);

    UiTransformInterface::Rect parentRect;
    AZ::Matrix4x4 transformFromViewport;
    if (parentElement != m_rootElement)
    {
        EBUS_EVENT_ID(parentElement, UiTransformBus, GetCanvasSpaceRectNoScaleRotate, parentRect);
        EBUS_EVENT_ID(parentElement, UiTransformBus, GetTransformFromViewport, transformFromViewport);
    }
    else
    {
        transformFromViewport = AZ::Matrix4x4::CreateIdentity();
        parentRect.Set(0.0f, m_targetCanvasSize.GetX(), 0.0f, m_targetCanvasSize.GetY());
    }

    // Go through the navigable elements and find the closest element to the top left of its parent
    float shortestDist = FLT_MAX;
    float shortestOutsideDist = FLT_MAX;
    AZ::EntityId closestElement;
    AZ::EntityId closestOutsideElement;
    for (auto navigableElement : navigableElements)
    {
        UiTransformInterface::RectPoints points;
        EBUS_EVENT_ID(navigableElement->GetId(), UiTransformBus, GetViewportSpacePoints, points);
        points = points.Transform(transformFromViewport);

        AZ::Vector2 topLeft = points.GetAxisAlignedTopLeft() - AZ::Vector2(parentRect.left, parentRect.top);
        AZ::Vector2 center = points.GetCenter();

        float dist = topLeft.GetLength();

        bool inside = (center.GetX() >= parentRect.left &&
                       center.GetX() <= parentRect.right &&
                       center.GetY() >= parentRect.top &&
                       center.GetY() <= parentRect.bottom);

        if (inside)
        {
            // Calculate a value from 0 to 1 representing how close the element is to the top of the screen
            float yDistValue = topLeft.GetY() / parentRect.GetHeight();

            // Calculate final distance value biased by y distance value
            const float distMultConstant = 1.0f;
            dist += dist * distMultConstant * yDistValue;

            if (dist < shortestDist)
            {
                shortestDist = dist;
                closestElement = navigableElement->GetId();
            }
        }
        else
        {
            if (dist < shortestOutsideDist)
            {
                shortestOutsideDist = dist;
                closestOutsideElement = navigableElement->GetId();
            }
        }
    }

    if (!closestElement.IsValid())
    {
        closestElement = closestOutsideElement;
    }

    return closestElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetFirstHoverInteractable()
{
    bool setHoverInteractable = false;

    if (s_handleHoverInputEvents)
    {
        // Check if there is a mouse or touch input device
        const AzFramework::InputDevice* mouseDevice = AzFramework::InputDeviceRequests::FindInputDevice(AzFramework::InputDeviceMouse::Id);
        const AzFramework::InputDevice* touchDevice = AzFramework::InputDeviceRequests::FindInputDevice(AzFramework::InputDeviceTouch::Id);
        if ((!mouseDevice || !mouseDevice->IsConnected()) &&
            (!touchDevice || !touchDevice->IsConnected()))
        {
            // No mouse or touch input device available so set a hover interactable
            setHoverInteractable = true;
        }
    }
    else
    {
        // Not handling hover input events so set a hover interactable
        setHoverInteractable = true;
    }

    if (setHoverInteractable)
    {
        AZ::EntityId hoverInteractable = GetFirstHoverInteractable();

        if (hoverInteractable.IsValid())
        {
            SetHoverInteractable(hoverInteractable);

            s_handleHoverInputEvents = false;
            m_allowInvalidatingHoverInteractableOnHoverInput = false;

            CheckHoverInteractableAndAutoActivate();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::PrepareAnimationSystemForCanvasSave()
{
    m_serializedAnimationData.m_serializeData.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::RestoreAnimationSystemAfterCanvasLoad(bool remapIds, UiElementComponent::EntityIdMap* entityIdMap)
{
    // NOTE: this is legacy code for loading old format animation data. The latest canvas format
    // uses the AZ serialization system for animation data.
    const char* buffer = m_serializedAnimationData.m_serializeData.c_str();
    size_t size = m_serializedAnimationData.m_serializeData.length();
    if (size > 0)
    {
        // found old format animation data
        // serialize back from loaded string and then clear string
        XmlNodeRef xmlNode = gEnv->pSystem->LoadXmlFromBuffer(buffer, size);

        m_uiAnimationSystem.Serialize(xmlNode, true);
        m_serializedAnimationData.m_serializeData.clear();
    }

    // go through the sequences and fixup the entity Ids
    // NOTE: for a latest format canvas these have probably already been remapped by ReplaceEntityRefs
    // This function will leave them alone if that are not in the remap table
    m_uiAnimationSystem.InitPostLoad(remapIds, entityIdMap);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent* UiCanvasComponent::CloneAndInitializeCanvas(UiEntityContext* entityContext, const string& assetIdPathname, const AZ::Vector2* canvasSize)
{
    UiCanvasComponent* canvasComponent = nullptr;

    // Clone the root slice entity
    // Do this in a way that handles this canvas being a Editor canvas.
    // If it is an editor canvas then slices will be flattened and Editor components will be
    // replaced with runtime components.
    AZ::Entity* clonedRootSliceEntity = nullptr;
    AZStd::string rootSliceBuffer;
    AZ::IO::ByteContainerStream<AZStd::string > rootSliceStream(&rootSliceBuffer);
    if (m_entityContext->SaveToStreamForGame(rootSliceStream, AZ::ObjectStream::ST_XML))
    {
        rootSliceStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        clonedRootSliceEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(rootSliceStream);
    }

    // Clone the canvas entity
    AZ::Entity* sourceCanvasEntity = GetEntity();
    AZ::Entity* clonedCanvasEntity = nullptr;
    AZStd::string canvasBuffer;
    AZ::IO::ByteContainerStream<AZStd::string > canvasStream(&canvasBuffer);
    if (m_entityContext->SaveCanvasEntityToStreamForGame(sourceCanvasEntity, canvasStream, AZ::ObjectStream::ST_XML))
    {
        canvasStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        clonedCanvasEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(canvasStream);
    }

    AZ_Assert(clonedCanvasEntity && clonedRootSliceEntity, "Failed to clone canvas");

    if (clonedCanvasEntity && clonedRootSliceEntity)
    {
        // complete initialization of cloned entities, we assume this is NOT for editor
        // since we only do this when using canvas in game that is already loaded in editor
        canvasComponent = FixupPostLoad(clonedCanvasEntity, clonedRootSliceEntity, false, entityContext, canvasSize);
    }

    if (canvasComponent)
    {
        canvasComponent->m_pathname = assetIdPathname;
        canvasComponent->m_isLoadedInGame = true;
    }

    return canvasComponent;
}

void UiCanvasComponent::DeactivateElements()
{
    AZ::SliceComponent::EntityList entities;
    AZ::SliceComponent* rootSlice = m_entityContext->GetRootSlice();

    bool result = rootSlice->GetEntities(entities);
    if (result)
    {
        for (AZ::Entity* entity : entities)
        {
            if (entity->GetState() == AZ::Entity::ES_ACTIVE)
            {
                entity->Deactivate();
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::vector<AZ::EntityId> UiCanvasComponent::GetEntityIdsOfElementAndDescendants(AZ::Entity* entity)
{
    AZStd::vector<AZ::EntityId> entitiesInPrefab;
    entitiesInPrefab.push_back(entity->GetId());

    LyShine::EntityArray descendantEntities;
    EBUS_EVENT_ID(entity->GetId(), UiElementBus, FindDescendantElements,
        [](const AZ::Entity*) { return true; }, descendantEntities);

    for (auto descendant : descendantEntities)
    {
        entitiesInPrefab.push_back(descendant->GetId());
    }

    return entitiesInPrefab;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SetTargetCanvasSizeAndUniformScale(bool isInGame, AZ::Vector2 canvasSize)
{
    AZ::Vector2 oldTargetCanvasSize = m_targetCanvasSize;
    float oldUniformDeviceScale = m_uniformDeviceScale;

    if (isInGame)
    {
        // Set the target canvas size to the canvas size specified by the caller
        m_targetCanvasSize = canvasSize;

        // set the uniform scale
        float viewportAspectRatio = m_targetCanvasSize.GetX() / m_targetCanvasSize.GetY();
        float canvasAspectRatio = m_canvasSize.GetX() / m_canvasSize.GetY();
        if (viewportAspectRatio > canvasAspectRatio)
        {
            // viewport is more wide-screen that the canvas. So scale so that the y dimensions fit
            m_uniformDeviceScale = m_targetCanvasSize.GetY() / m_canvasSize.GetY();
        }
        else
        {
            // viewport is less wide-screen that the canvas. So scale so that the x dimensions fit
            m_uniformDeviceScale = m_targetCanvasSize.GetX() / m_canvasSize.GetX();
        }
    }
    else
    {
        // While in the editor, the only resolution we care about is the canvas' authored
        // size, so we set that as our target size for display purposes.
        m_targetCanvasSize = m_canvasSize;
    }

    // if the target canvas size or the uniform device scale changed then this will affect the
    // element transforms so force them to recompute
    if (oldTargetCanvasSize != m_targetCanvasSize || oldUniformDeviceScale != m_uniformDeviceScale)
    {
        EBUS_EVENT_ID(GetRootElement()->GetId(), UiTransformBus, SetRecomputeTransformFlag);
        EBUS_EVENT(UiCanvasSizeNotificationBus, OnCanvasSizeOrScaleChange, GetEntityId());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::IsElementNameUnique(const AZStd::string& elementName, const LyShine::EntityArray& elements)
{
    for (auto element : elements)
    {
        if (element->GetName() == elementName)
        {
            return false;
        }
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent::EntityComboBoxVec UiCanvasComponent::PopulateNavigableEntityList()
{
    EntityComboBoxVec result;

    // Add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(), "<None>"));

    // Get a list of all navigable elements
    LyShine::EntityArray navigableElements;

    auto checkNavigable = [](const AZ::Entity* entity)
        {
            UiNavigationInterface::NavigationMode navigationMode = UiNavigationInterface::NavigationMode::None;
            EBUS_EVENT_ID_RESULT(navigationMode, entity->GetId(), UiNavigationBus, GetNavigationMode);
            return (navigationMode != UiNavigationInterface::NavigationMode::None);
        };

    FindElements(checkNavigable, navigableElements);

    // Sort the elements by name
    AZStd::sort(navigableElements.begin(), navigableElements.end(),
        [](const AZ::Entity* e1, const AZ::Entity* e2) { return e1->GetName() < e2->GetName(); });

    // Add their names to the StringList and their IDs to the id list
    for (auto navigableEntity : navigableElements)
    {
        result.push_back(AZStd::make_pair(navigableEntity->GetId(), navigableEntity->GetName()));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent::EntityComboBoxVec UiCanvasComponent::PopulateTooltipDisplayEntityList()
{
    EntityComboBoxVec result;

    // Add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(), "<None>"));

    // Get a list of all tooltip display elements
    LyShine::EntityArray tooltipDisplayElements;

    auto checkTooltipDisplay = [](const AZ::Entity* entity)
        {
            // Check for component on entity
            return UiTooltipDisplayBus::FindFirstHandler(entity->GetId()) ? true : false;
        };

    FindElements(checkTooltipDisplay, tooltipDisplayElements);

    // Sort the elements by name
    AZStd::sort(tooltipDisplayElements.begin(), tooltipDisplayElements.end(),
        [](const AZ::Entity* e1, const AZ::Entity* e2) { return e1->GetName() < e2->GetName(); });

    // Add their names to the StringList and their IDs to the id list
    for (auto tooltipDisplayEntity : tooltipDisplayElements)
    {
        result.push_back(AZStd::make_pair(tooltipDisplayEntity->GetId(), tooltipDisplayEntity->GetName()));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::CreateRenderTarget()
{
    if (m_canvasSize.GetX() <= 0 || m_canvasSize.GetY() <= 0)
    {
        gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,
            m_pathname.c_str(),
            "Invalid render target width/height for UI canvas: %s",
            m_pathname.c_str());
        return;
    }

    // Create a render target that this canvas will be rendered to.
    // The render target size is the canvas size.
    m_renderTargetHandle = gEnv->pRenderer->CreateRenderTarget(m_renderTargetName.c_str(),
            static_cast<int>(m_canvasSize.GetX()), static_cast<int>(m_canvasSize.GetY()), Clr_Empty, eTF_R8G8B8A8);

    if (m_renderTargetHandle <= 0)
    {
        gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,
            m_pathname.c_str(),
            "Failed to create render target for UI canvas: %s",
            m_pathname.c_str());
    }
    else
    {
        // Also create a depth surface to render the canvas to, we need depth for masking
        // since that uses the stencil buffer
        m_renderTargetDepthSurface = gEnv->pRenderer->CreateDepthSurface(
                static_cast<int>(m_canvasSize.GetX()), static_cast<int>(m_canvasSize.GetY()));

        // Register this canvas component as a game framework listener so that we can render
        // it to a texture on the PreRender event
        if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
        {
            gEnv->pGame->GetIGameFramework()->RegisterListener(this, "UiCanvasComponent", FRAMEWORKLISTENERPRIORITY_HUD);
        }
        else // CryAction has been moved to the optional CryLegacy Gem, so if it doesn't exist we need to do this instead
        {
            ISystem::CrySystemNotificationBus::Handler::BusConnect();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::DestroyRenderTarget()
{
    if (m_renderTargetHandle > 0)
    {
        if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
        {
            gEnv->pGame->GetIGameFramework()->UnregisterListener(this);
        }
        else // CryAction has been moved to the optional CryLegacy Gem, so if it doesn't exist we need to do this instead
        {
            ISystem::CrySystemNotificationBus::Handler::BusDisconnect();
        }
        gEnv->pRenderer->DestroyDepthSurface(m_renderTargetDepthSurface);
        m_renderTargetDepthSurface = nullptr;
        gEnv->pRenderer->DestroyRenderTarget(m_renderTargetHandle);
        m_renderTargetHandle = -1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::RenderCanvasToTexture(bool displayBounds)
{
    if (m_renderTargetHandle <= 0)
    {
        return;
    }

    ISystem* system = gEnv->pSystem;
    if (system && !gEnv->IsDedicated())
    {
        gEnv->pRenderer->SetRenderTarget(m_renderTargetHandle, m_renderTargetDepthSurface);

        // clear the render target before rendering to it
        // NOTE: the FRT_CLEAR_IMMEDIATE is required since we will have already set the render target
        // In theory we could call this before setting the render target without the immediate flag
        // but that doesn't work. Perhaps because FX_Commit is not called.
        ColorF viewportBackgroundColor(0, 0, 0, 0); // if clearing color we want to set alpha to zero also
        gEnv->pRenderer->ClearTargetsImmediately(FRT_CLEAR, viewportBackgroundColor);

        // we are writing to a linear texture
        gEnv->pRenderer->SetSrgbWrite(false);

        RenderCanvas(true, m_canvasSize, displayBounds);

        gEnv->pRenderer->SetRenderTarget(0); // restore render target
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::SaveCanvasToFile(const string& pathname, AZ::DataStream::StreamType streamType)
{
    // Note: This is ok for saving in tools, but we should use the streamer to write objects directly (no memory store)
    AZStd::vector<AZ::u8> dstData;
    AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > dstByteStream(&dstData);

    if (!SaveCanvasToStream(dstByteStream, streamType))
    {
        return false;
    }

    AZ::IO::SystemFile file;
    file.Open(pathname.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
    if (!file.IsOpen())
    {
        file.Close();
        return false;
    }

    file.Write(dstData.data(), dstData.size());

    file.Close();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::SaveCanvasToStream(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType)
{
    UiCanvasFileObject fileObject;
    fileObject.m_canvasEntity = this->GetEntity();

    fileObject.m_rootSliceEntity = m_entityContext->GetRootAssetEntity();

    if (!AZ::Utils::SaveObjectToStream<UiCanvasFileObject>(stream, streamType, &fileObject))
    {
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SendRectChangeNotificationsAndRecomputeLayouts()
{
    // Send canvas space rect change notifications. Handlers may mark
    // layouts for a recompute
    SendRectChangeNotifications();

    // Recompute invalid layouts
    if (m_layoutManager->HasMarkedLayouts())
    {
        m_layoutManager->RecomputeMarkedLayouts();

        // The layout recompute may have caused child size changes, so
        // send canvas space rect change notifications again
        // NOTE: this is expensive so we don't do it unless there were marked layouts
        SendRectChangeNotifications();

        // Remove the newly marked layouts since they have been marked due
        // to their parents recomputing them
        m_layoutManager->UnmarkAllLayouts();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::SendRectChangeNotifications()
{
    // lambda that simply sends the RecomputeTransformsAndSendNotifications message to an entity
    auto RecomputeAndNotify = [](const AZ::EntityId entity)
        {
            EBUS_EVENT_ID(entity, UiTransformBus, RecomputeTransformsAndSendNotifications);
        };

    // While we know there are transforms that need re-computing
    while (m_transformsNeedRecompute)
    {
        // clear the flag, it may get set again if some of the notification listeners change things
        m_transformsNeedRecompute = false;

        // Recompute the transforms on all elements and sent notifications of any changes in element rects
        EBUS_EVENT_ID(m_rootElement, UiElementBus, CallOnDescendantElements, RecomputeAndNotify);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasComponent::InitializeLayouts()
{
    m_layoutManager->ComputeLayoutForElementAndDescendants(GetRootElement()->GetId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiCanvasComponent::CloneAndAddElementInternal(AZ::Entity* sourceEntity, AZ::Entity* parentEntity, AZ::Entity* insertBeforeEntity)
{
    // first check that the given entity really is a UI element - i.e. it has a UiElementComponent
    UiElementComponent* sourceElementComponent = sourceEntity->FindComponent<UiElementComponent>();
    if (!sourceElementComponent)
    {
        AZ_Warning("UI", false, "CloneElement: The entity to be cloned must have an element component");
        return nullptr;
    }

    // also check that the given parent entity is part of this canvas (if one is specified)
    if (parentEntity)
    {
        AZ::EntityId parentCanvasId;
        EBUS_EVENT_ID_RESULT(parentCanvasId, parentEntity->GetId(), UiElementBus, GetCanvasEntityId);
        if (parentCanvasId != GetEntityId())
        {
            AZ_Warning("UI", false, "CloneElement: The parent entity must belong to this canvas");
            return nullptr;
        }
    }

    // If no parent entity specified then the parent is the root element
    AZ::Entity* parent = (parentEntity) ? parentEntity : GetRootElement();

    // also check that the given InsertBefore entity is a child of the parent
    if (insertBeforeEntity)
    {
        AZ::Entity* insertBeforeParent;
        EBUS_EVENT_ID_RESULT(insertBeforeParent, insertBeforeEntity->GetId(), UiElementBus, GetParent);
        if (insertBeforeParent != parent)
        {
            AZ_Warning("UI", false, "CloneElement: The insertBefore entity must be a child of the parent");
            return nullptr;
        }
    }

    AZ::SerializeContext* context = nullptr;
    EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(context, "No serialization context found");

    AZStd::vector<AZ::EntityId> entitiesToClone = GetEntityIdsOfElementAndDescendants(sourceEntity);

    AzFramework::EntityContext::EntityList clonedEntities;
    m_entityContext->CloneUiEntities(entitiesToClone, clonedEntities);

    AZ::Entity* clonedRootEntity = clonedEntities[0];

    UiElementComponent* elementComponent = clonedRootEntity->FindComponent<UiElementComponent>();
    AZ_Assert(elementComponent, "The cloned entity must have an element component");

    // recursively set the canvas and parent pointers
    elementComponent->FixupPostLoad(clonedRootEntity, this, parent, true);

    // add this new entity as a child of the parent (parentEntity or root)
    UiElementComponent* parentElementComponent = parent->FindComponent<UiElementComponent>();
    AZ_Assert(parentElementComponent, "No element component found on parent entity");
    parentElementComponent->AddChild(clonedRootEntity, insertBeforeEntity);

    if (m_isLoadedInGame)
    {
        // Call InGamePostActivate on all the created entities
        for (AZ::Entity* entity : clonedEntities)
        {
            EBUS_EVENT_ID(entity->GetId(), UiInitializationBus, InGamePostActivate);
        }
    }

    return clonedRootEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::u64 UiCanvasComponent::CreateUniqueId()
{
    AZ::u64 utcTime = AZStd::GetTimeUTCMilliSecond();
    uint32 r = cry_random_uint32();

    AZ::u64 id = utcTime << 32 | r;
    return id;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent* UiCanvasComponent::CreateCanvasInternal(UiEntityContext* entityContext, bool forEditor)
{
    // create a new empty canvas, give it a name to avoid serialization generating one based on
    // the ID (which in some cases caused diffs to fail in the editor)
    AZ::Entity* canvasEntity = aznew AZ::Entity("UiCanvasEntity");
    UiCanvasComponent* canvasComponent = canvasEntity->CreateComponent<UiCanvasComponent>();

    // Initialize the UiEntityContext
    canvasComponent->m_entityContext = entityContext;
    canvasComponent->m_entityContext->InitUiContext();

    // Give the canvas a unique identifier. Used for canvas metrics
    canvasComponent->m_uniqueId = CreateUniqueId();

    // This is the dummy root node of the canvas.
    // It needs an element component and a transform component.
    AZ::Entity* rootEntity = canvasComponent->m_entityContext->CreateEntity("_root");
    canvasComponent->m_rootElement = rootEntity->GetId();
    AZ_Assert(rootEntity, "Failed to create root element entity");

    rootEntity->Deactivate(); // so we can add components

    UiElementComponent* elementComponent = rootEntity->CreateComponent<UiElementComponent>();
    AZ_Assert(elementComponent, "Failed to add UiElementComponent to entity");
    elementComponent->SetCanvas(canvasComponent, canvasComponent->GenerateId());
    AZ::Component* transformComponent = rootEntity->CreateComponent<UiTransform2dComponent>();
    AZ_Assert(transformComponent, "Failed to add transform2d component to entity");

    rootEntity->Activate();  // re-activate

    // init the canvas entity (the canvas entity is not part of the EntityContext so is not automatically initialized)
    canvasEntity->Init();
    canvasEntity->Activate();

    canvasComponent->m_isLoadedInGame = !forEditor;

    return canvasComponent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent*  UiCanvasComponent::LoadCanvasInternal(const string& pathnameToOpen, bool forEditor, const string& assetIdPathname, UiEntityContext* entityContext,
    const AZ::SliceComponent::EntityIdToEntityIdMap* previousRemapTable, AZ::EntityId previousCanvasId)
{
    UiCanvasComponent* canvasComponent = nullptr;

    // Currently LoadObjectFromFile will hang if the file cannot be parsed
    // (LMBR-10078). So first check that it is in the right format
    if (IsValidAzSerializedFile(pathnameToOpen))
    {
        // Open a stream on the input path
        AZ::IO::FileIOStream stream(pathnameToOpen.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);
        if (!stream.IsOpen())
        {
            AZ_Warning("UI", false, "Cannot open UI canvas file \"%s\".", pathnameToOpen.c_str());
        }
        else
        {
            // Read in the canvas from the stream
            UiCanvasFileObject* canvasFileObject = UiCanvasFileObject::LoadCanvasFromStream(stream);
            AZ_Assert(canvasFileObject, "Failed to load canvas");

            if (canvasFileObject)
            {
                AZ::Entity* canvasEntity = canvasFileObject->m_canvasEntity;
                AZ::Entity* rootSliceEntity = canvasFileObject->m_rootSliceEntity;
                AZ_Assert(canvasEntity && rootSliceEntity, "Failed to load canvas");

                if (canvasEntity && rootSliceEntity)
                {
                    // file loaded OK

                    // no need to check if a canvas with this EntityId is already loaded since we are going
                    // to generate new entity IDs for all entities loaded from the file.

                    // complete initialization of loaded entities
                    canvasComponent = FixupPostLoad(canvasEntity, rootSliceEntity, forEditor, entityContext, nullptr, previousRemapTable, previousCanvasId);
                    if (canvasComponent)
                    {
                        // The canvas size may get reset on the first call to RenderCanvas to set the size to
                        // viewport size. So we'll recompute again on first render.
                        EBUS_EVENT_ID(canvasComponent->GetRootElement()->GetId(), UiTransformBus, SetRecomputeTransformFlag);

                        canvasComponent->m_pathname = assetIdPathname;
                        canvasComponent->m_isLoadedInGame = !forEditor;
                    }
                    else
                    {
                        // cleanup, don't delete rootSliceEntity, deleting the canvasEntity cleans up the EntityContext and root slice
                        delete canvasEntity;
                    }
                }

                // UiCanvasFileObject is a simple container for the canvas pointers, its destructor
                // doesn't destroy the canvas, but we need to delete it nonetheless to avoid leaking.
                delete canvasFileObject;
            }
        }
    }
    else
    {
        // this file is not a valid canvas file
        gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,
            pathnameToOpen.c_str(),
            "Invalid XML format or couldn't load file for UI canvas file: %s",
            pathnameToOpen.c_str());
    }

    return canvasComponent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent* UiCanvasComponent::FixupReloadedCanvasForEditorInternal(AZ::Entity* newCanvasEntity,
    AZ::Entity* rootSliceEntity, UiEntityContext* entityContext,
    LyShine::CanvasId existingId, const string& existingPathname)
{
    UiCanvasComponent* newCanvasComponent = FixupPostLoad(newCanvasEntity, rootSliceEntity, true, entityContext);
    if (newCanvasComponent)
    {
        newCanvasComponent->m_id = existingId;
        newCanvasComponent->m_pathname = existingPathname;
    }
    return newCanvasComponent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent* UiCanvasComponent::FixupPostLoad(AZ::Entity* canvasEntity, AZ::Entity* rootSliceEntity, bool forEditor, UiEntityContext* entityContext, const AZ::Vector2* canvasSize,
    const AZ::SliceComponent::EntityIdToEntityIdMap* previousRemapTable, AZ::EntityId previousCanvasId)
{
    // When we load in game we always generate new entity IDs.
    bool makeNewEntityIds = (forEditor) ? false : true;

    // When we load in the editor, check if there is another canvas open that has the same entityId
    if (forEditor)
    {
        AZ::Entity* foundEntity = nullptr;
        EBUS_EVENT_RESULT(foundEntity, AZ::ComponentApplicationBus, FindEntity, canvasEntity->GetId());
        if (foundEntity)
        {
            makeNewEntityIds = true;
        }
    }

    UiCanvasComponent* canvasComponent = canvasEntity->FindComponent<UiCanvasComponent>();
    AZ_Assert(canvasComponent, "No canvas component found on loaded entity");
    if (!canvasComponent)
    {
        return nullptr;     // unlikely to happen but perhaps possible if a non-canvas file was opened
    }

    // Initialize the entity context for the new canvas and init and activate all the entities
    // in the root slice
    canvasComponent->m_entityContext = entityContext;
    canvasComponent->m_entityContext->InitUiContext();

    if (previousRemapTable)
    {
        // We are reloading a canvas and we want to use the same entity ID mapping (from editor entity to game entity) as in the previously
        // loaded canvas. The code below is similar to what HandleLoadedRootSliceEntity does for remapping except that if the existing
        // mapping table already contains an entry for an editor entity ID it will use the existing mapping
        AZ::SliceComponent* newRootSlice = rootSliceEntity->FindComponent<AZ::SliceComponent>();

        AZ::SerializeContext* context = nullptr;
        EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(context, "No serialization context found");

        AZ::SliceComponent::EntityList entities;
        newRootSlice->GetEntities(entities);

        canvasComponent->m_editorToGameEntityIdMap = *previousRemapTable;
        AZ::SliceComponent::InstantiatedContainer entityContainer;
        entityContainer.m_entities = AZStd::move(entities);
        ReuseOrGenerateNewIdsAndFixRefs(&entityContainer, canvasComponent->m_editorToGameEntityIdMap, context);
        entities = AZStd::move(entityContainer.m_entities); // have to move these out or all the entities get deleted

        if (!canvasComponent->m_entityContext->HandleLoadedRootSliceEntity(rootSliceEntity, false, nullptr))
        {
            return nullptr;
        }
    }
    else
    {
        // we are not reloading a canvas so we let the EntityContext HandleLoadedRootSliceEntity do the entity ID remapping as usual
        if (!canvasComponent->m_entityContext->HandleLoadedRootSliceEntity(rootSliceEntity, makeNewEntityIds, &canvasComponent->m_editorToGameEntityIdMap))
        {
            return nullptr;
        }
    }

    // For the canvas entity itself, handle ID mapping and initialization
    {
        if (previousCanvasId.IsValid())
        {
            canvasEntity->SetId(previousCanvasId);
        }
        else if (makeNewEntityIds)
        {
            AZ::EntityId newId = AZ::Entity::MakeId();
            canvasEntity->SetId(newId);
        }

        // remap entity IDs such as m_rootElement and any entity IDs in the animation data
        if (makeNewEntityIds)
        {
            // new IDs were generated so we should fix up any internal EntityRefs
            AZ::SerializeContext* context = nullptr;
            EBUS_EVENT_RESULT(context, AZ::ComponentApplicationBus, GetSerializeContext);
            AZ_Assert(context, "No serialization context found");

            ReuseOrGenerateNewIdsAndFixRefs(canvasEntity, canvasComponent->m_editorToGameEntityIdMap, context);
        }

        canvasEntity->Init();
        canvasEntity->Activate();
    }

    AZ::Entity* rootElement = canvasComponent->GetRootElement();

    UiElementComponent* elementComponent = rootElement->FindComponent<UiElementComponent>();
    AZ_Assert(elementComponent, "No element component found on root element entity");

    // Need to remapIds too (actually I don't think this needs to remap anymore)
    canvasComponent->RestoreAnimationSystemAfterCanvasLoad(makeNewEntityIds, &canvasComponent->m_editorToGameEntityIdMap);

    bool fixupSuccess = elementComponent->FixupPostLoad(rootElement, canvasComponent, nullptr, false);
    if (!fixupSuccess)
    {
        return nullptr;
    }

    AZ::SliceComponent::EntityList entities;
    AZ::SliceComponent* rootSlice = canvasComponent->m_entityContext->GetRootSlice();

    bool result = rootSlice->GetEntities(entities);

    // Initialize the target canvas size and uniform scale
    // This should be done before calling InGamePostActivate so that the
    // canvas space rects of the elements are accurate
    AZ_Assert(gEnv->pRenderer, "Attempting to access IRenderer before it has been initialized");
    if (gEnv->pRenderer)
    {
        AZ::Vector2 targetCanvasSize;
        if (canvasSize)
        {
            targetCanvasSize = *canvasSize;
        }
        else
        {
            targetCanvasSize.SetX(static_cast<float>(gEnv->pRenderer->GetOverlayWidth()));
            targetCanvasSize.SetY(static_cast<float>(gEnv->pRenderer->GetOverlayHeight()));
        }
        canvasComponent->SetTargetCanvasSizeAndUniformScale(!forEditor, targetCanvasSize);
    }

    // Set this before calling InGamePostActivate on the created entities. InGamePostActivate could
    // call CloneElement which checks this flag
    canvasComponent->m_isLoadedInGame = !forEditor;

    // Initialize transform properties of children of layout elements
    canvasComponent->InitializeLayouts();

    if (!forEditor)
    {
        // Call InGamePostActivate on all the created entities when loading in game
        for (AZ::Entity* entity : entities)
        {
            EBUS_EVENT_ID(entity->GetId(), UiInitializationBus, InGamePostActivate);
        }
    }

    // Set the first hover interactable
    if (canvasComponent->m_isNavigationSupported)
    {
        canvasComponent->SetFirstHoverInteractable();
    }

    return canvasComponent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1 to 2:
    if (classElement.GetVersion() < 2)
    {
        // No need to actually convert anything because the CanvasFileObject takes care of it
        // But it makes sense to bump the version number because the m_rootElement is now an EntityId
        // rather than an Entity*
    }

    // conversion from version 2 to 3:
    // - Need to convert Vec2 to AZ::Vector2
    if (classElement.GetVersion() < 3)
    {
        if (!LyShine::ConvertSubElementFromVec2ToVector2(context, classElement, "CanvasSize"))
        {
            return false;
        }
    }

    return true;
}
