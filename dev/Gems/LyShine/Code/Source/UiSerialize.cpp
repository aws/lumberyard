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
#include "UiSerialize.h"

#include <LyShine/UiAssetTypes.h>
#include <LyShine/IDraw2d.h>
#include <LyShine/UiBase.h>
#include "UiInteractableComponent.h"
#include <LyShine/UiSerializeHelpers.h>

#include <LyShine/Bus/UiImageBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>

#include <UiElementComponent.h>
#include <UiCanvasComponent.h>
#include <UiLayoutGridComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Slice/SliceComponent.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// NAMESPACE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace UiSerialize
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    class CryStringTCharSerializer
        : public AZ::SerializeContext::IDataSerializer
    {
        /// Return the size of binary buffer necessary to store the value in binary format
        size_t  GetRequiredBinaryBufferSize(const void* classPtr) const
        {
            const CryStringT<char>* string = reinterpret_cast<const CryStringT<char>*>(classPtr);
            return string->length() + 1;
        }

        /// Store the class data into a stream.
        size_t Save(const void* classPtr, AZ::IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
        {
            const CryStringT<char>* string = reinterpret_cast<const CryStringT<char>*>(classPtr);
            const char* data = string->c_str();

            return static_cast<size_t>(stream.Write(string->length() + 1, reinterpret_cast<const void*>(data)));
        }

        size_t DataToText(AZ::IO::GenericStream& in, AZ::IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            size_t len = in.GetLength();
            char* buffer = static_cast<char*>(azmalloc(len));
            in.Read(in.GetLength(), reinterpret_cast<void*>(buffer));

            AZStd::string outText = buffer;
            azfree(buffer);

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        size_t TextToData(const char* text, unsigned int textVersion, AZ::IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
        {
            (void)textVersion;

            size_t len = strlen(text) + 1;
            stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            return static_cast<size_t>(stream.Write(len, reinterpret_cast<const void*>(text)));
        }

        bool Load(void* classPtr, AZ::IO::GenericStream& stream, unsigned int /*version*/, bool isDataBigEndian /*= false*/) override
        {
            CryStringT<char>* string = reinterpret_cast<CryStringT<char>*>(classPtr);

            size_t len = stream.GetLength();
            char* buffer = static_cast<char*>(azmalloc(len));

            stream.Read(len, reinterpret_cast<void*>(buffer));

            *string = buffer;
            azfree(buffer);
            return true;
        }

        bool CompareValueData(const void* lhs, const void* rhs) override
        {
            return AZ::SerializeContext::EqualityCompareHelper<CryStringT<char> >::CompareValues(lhs, rhs);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    void UiOffsetsScriptConstructor(UiTransform2dInterface::Offsets* thisPtr, AZ::ScriptDataContext& dc)
    {
        int numArgs = dc.GetNumArguments();

        const int noArgsGiven = 0;
        const int allArgsGiven = 4;

        switch (numArgs)
        {
        case noArgsGiven:
        {
            *thisPtr = UiTransform2dInterface::Offsets();
        }
        break;

        case allArgsGiven:
        {
            if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
            {
                float left = 0;
                float top = 0;
                float right = 0;
                float bottom = 0;
                dc.ReadArg(0, left);
                dc.ReadArg(1, top);
                dc.ReadArg(2, right);
                dc.ReadArg(3, bottom);
                *thisPtr = UiTransform2dInterface::Offsets(left, top, right, bottom);
            }
            else
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 4 arguments to UiOffsets(), all must be numbers!");
            }
        }
        break;

        default:
        {
            dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "UiOffsets() accepts only 0 or 4 arguments, not %d!", numArgs);
        }
        break;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void UiAnchorsScriptConstructor(UiTransform2dInterface::Anchors* thisPtr, AZ::ScriptDataContext& dc)
    {
        int numArgs = dc.GetNumArguments();

        const int noArgsGiven = 0;
        const int allArgsGiven = 4;

        switch (numArgs)
        {
        case noArgsGiven:
        {
            *thisPtr = UiTransform2dInterface::Anchors();
        }
        break;

        case allArgsGiven:
        {
            if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
            {
                float left = 0;
                float top = 0;
                float right = 0;
                float bottom = 0;
                dc.ReadArg(0, left);
                dc.ReadArg(1, top);
                dc.ReadArg(2, right);
                dc.ReadArg(3, bottom);
                *thisPtr = UiTransform2dInterface::Anchors(left, top, right, bottom);
            }
            else
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 4 arguments to UiAnchors(), all must be numbers!");
            }
        }
        break;

        default:
        {
            dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "UiAnchors() accepts only 0 or 4 arguments, not %d!", numArgs);
        }
        break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ReflectUiTypes(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<ColorF>()->
                Field("r", &ColorF::r)->
                Field("g", &ColorF::g)->
                Field("b", &ColorF::b)->
                Field("a", &ColorF::a);

            serializeContext->Class<ColorB>()->
                Field("r", &ColorB::r)->
                Field("g", &ColorB::g)->
                Field("b", &ColorB::b)->
                Field("a", &ColorB::a);
        }

        // Vec2
        {
            if (serializeContext)
            {
                serializeContext->Class<Vec2>()->
                    Field("x", &Vec2::x)->
                    Field("y", &Vec2::y);

                // The only purpose of this EditContext stuff is to make things work in Push to Slice
                AZ::EditContext* ec = serializeContext->GetEditContext();
                if (ec)
                {
                    auto editInfo = ec->Class<Vec2>(0, "");
                    editInfo->DataElement(0, &Vec2::x, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &Vec2::y, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                }
            }
        }

        // Vec3
        {
            if (serializeContext)
            {
                serializeContext->Class<Vec3>()->
                    Field("x", &Vec3::x)->
                    Field("y", &Vec3::y)->
                    Field("z", &Vec3::z);

                // The only purpose of this EditContext stuff is to make things work in Push to Slice
                AZ::EditContext* ec = serializeContext->GetEditContext();
                if (ec)
                {
                    auto editInfo = ec->Class<Vec3>(0, "");
                    editInfo->DataElement(0, &Vec3::x, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &Vec3::y, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &Vec3::z, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                }
            }
        }

        // Anchors
        {
            if (serializeContext)
            {
                serializeContext->Class<UiTransform2dInterface::Anchors>()->
                    Field("left", &UiTransform2dInterface::Anchors::m_left)->
                    Field("top", &UiTransform2dInterface::Anchors::m_top)->
                    Field("right", &UiTransform2dInterface::Anchors::m_right)->
                    Field("bottom", &UiTransform2dInterface::Anchors::m_bottom);

                // The only purpose of this EditContext stuff is to make things work in Push to Slice
                AZ::EditContext* ec = serializeContext->GetEditContext();
                if (ec)
                {
                    auto editInfo = ec->Class<UiTransform2dInterface::Anchors>(0, "");
                    editInfo->DataElement(0, &UiTransform2dInterface::Anchors::m_left, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &UiTransform2dInterface::Anchors::m_top, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &UiTransform2dInterface::Anchors::m_right, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &UiTransform2dInterface::Anchors::m_bottom, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                }
            }

            if (behaviorContext)
            {
                behaviorContext->Class<UiTransform2dInterface::Anchors>("UiAnchors")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Constructor<>()
                    ->Constructor<float, float, float, float>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &UiAnchorsScriptConstructor)
                    ->Property("left", BehaviorValueProperty(&UiTransform2dInterface::Anchors::m_left))
                    ->Property("top", BehaviorValueProperty(&UiTransform2dInterface::Anchors::m_top))
                    ->Property("right", BehaviorValueProperty(&UiTransform2dInterface::Anchors::m_right))
                    ->Property("bottom", BehaviorValueProperty(&UiTransform2dInterface::Anchors::m_bottom));
            }
        }

        // Offsets
        {
            if (serializeContext)
            {
                serializeContext->Class<UiTransform2dInterface::Offsets>()->
                    Field("left", &UiTransform2dInterface::Offsets::m_left)->
                    Field("top", &UiTransform2dInterface::Offsets::m_top)->
                    Field("right", &UiTransform2dInterface::Offsets::m_right)->
                    Field("bottom", &UiTransform2dInterface::Offsets::m_bottom);

                // The only purpose of this EditContext stuff is to make things work in Push to Slice
                AZ::EditContext* ec = serializeContext->GetEditContext();
                if (ec)
                {
                    auto editInfo = ec->Class<UiTransform2dInterface::Offsets>(0, "");
                    editInfo->DataElement(0, &UiTransform2dInterface::Offsets::m_left, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &UiTransform2dInterface::Offsets::m_top, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &UiTransform2dInterface::Offsets::m_right, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &UiTransform2dInterface::Offsets::m_bottom, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                }
            }

            if (behaviorContext)
            {
                behaviorContext->Class<UiTransform2dInterface::Offsets>("UiOffsets")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Constructor<>()
                    ->Constructor<float, float, float, float>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &UiOffsetsScriptConstructor)
                    ->Property("left", BehaviorValueProperty(&UiTransform2dInterface::Offsets::m_left))
                    ->Property("top", BehaviorValueProperty(&UiTransform2dInterface::Offsets::m_top))
                    ->Property("right", BehaviorValueProperty(&UiTransform2dInterface::Offsets::m_right))
                    ->Property("bottom", BehaviorValueProperty(&UiTransform2dInterface::Offsets::m_bottom));
            }
        }

        // Padding
        {
            if (serializeContext)
            {
                serializeContext->Class<UiLayoutInterface::Padding>()->
                    Field("left", &UiLayoutInterface::Padding::m_left)->
                    Field("top", &UiLayoutInterface::Padding::m_top)->
                    Field("right", &UiLayoutInterface::Padding::m_right)->
                    Field("bottom", &UiLayoutInterface::Padding::m_bottom);

                // The only purpose of this EditContext stuff is to make things work in Push to Slice
                AZ::EditContext* ec = serializeContext->GetEditContext();
                if (ec)
                {
                    auto editInfo = ec->Class<UiLayoutInterface::Padding>(0, "");
                    editInfo->DataElement(0, &UiLayoutInterface::Padding::m_left, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &UiLayoutInterface::Padding::m_top, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &UiLayoutInterface::Padding::m_right, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &UiLayoutInterface::Padding::m_bottom, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                }
            }

            if (behaviorContext)
            {
                behaviorContext->Class<UiLayoutInterface::Padding>("UiPadding")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Property("left", BehaviorValueProperty(&UiLayoutInterface::Padding::m_left))
                    ->Property("right", BehaviorValueProperty(&UiLayoutInterface::Padding::m_right))
                    ->Property("top", BehaviorValueProperty(&UiLayoutInterface::Padding::m_top))
                    ->Property("bottom", BehaviorValueProperty(&UiLayoutInterface::Padding::m_bottom));
            }
        }

        // UiLayout enums
        {
            if (behaviorContext)
            {
                behaviorContext->Enum<(int)UiLayoutInterface::HorizontalOrder::LeftToRight>("eUiHorizontalOrder_LeftToRight")
                    ->Enum<(int)UiLayoutInterface::HorizontalOrder::RightToLeft>("eUiHorizontalOrder_RightToLeft")
                    ->Enum<(int)UiLayoutInterface::VerticalOrder::TopToBottom>("eUiVerticalOrder_TopToBottom")
                    ->Enum<(int)UiLayoutInterface::VerticalOrder::BottomToTop>("eUiVerticalOrder_BottomToTop");
            }
        }

        // IDraw2d enums
        {
            if (behaviorContext)
            {
                behaviorContext->Enum<(int)IDraw2d::HAlign::Left>("eUiHAlign_Left")
                    ->Enum<(int)IDraw2d::HAlign::Center>("eUiHAlign_Center")
                    ->Enum<(int)IDraw2d::HAlign::Right>("eUiHAlign_Right")
                    ->Enum<(int)IDraw2d::VAlign::Top>("eUiVAlign_Top")
                    ->Enum<(int)IDraw2d::VAlign::Center>("eUiVAlign_Center")
                    ->Enum<(int)IDraw2d::VAlign::Bottom>("eUiVAlign_Bottom");
            }
        }

        if (serializeContext)
        {
            serializeContext->Class<CryStringT<char> >()->
                Serializer(&AZ::Serialize::StaticInstance<CryStringTCharSerializer>::s_instance);

            serializeContext->Class<PrefabFileObject>()
                ->Version(2, &PrefabFileObject::VersionConverter)
                ->Field("RootEntity", &PrefabFileObject::m_rootEntityId)
                ->Field("Entities", &PrefabFileObject::m_entities);

            serializeContext->Class<AnimationData>()
                ->Version(1)
                ->Field("SerializeString", &AnimationData::m_serializeData);

            // deprecate old classes that no longer exist
            serializeContext->ClassDeprecate("UiCanvasEditor", "{65682E87-B573-435B-88CB-B4C12B71EEEE}");
            serializeContext->ClassDeprecate("ImageAsset", "{138E471A-F3AE-404A-9075-EDC7488C97FC}");

            AzFramework::SimpleAssetReference<LyShine::FontAsset>::Register(*serializeContext);
            AzFramework::SimpleAssetReference<LyShine::CanvasAsset>::Register(*serializeContext);

            UiInteractableComponent::Reflect(serializeContext);
        }

        if (behaviorContext)
        {
            UiInteractableComponent::Reflect(behaviorContext);

            behaviorContext->EBus<UiLayoutBus>("UiLayoutBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("GetHorizontalChildAlignment", &UiLayoutBus::Events::GetHorizontalChildAlignment)
                ->Event("SetHorizontalChildAlignment", &UiLayoutBus::Events::SetHorizontalChildAlignment)
                ->Event("GetVerticalChildAlignment", &UiLayoutBus::Events::GetVerticalChildAlignment)
                ->Event("SetVerticalChildAlignment", &UiLayoutBus::Events::SetVerticalChildAlignment)
                ->Event("GetIgnoreDefaultLayoutCells", &UiLayoutBus::Events::GetIgnoreDefaultLayoutCells)
                ->Event("SetIgnoreDefaultLayoutCells", &UiLayoutBus::Events::SetIgnoreDefaultLayoutCells);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to change all owned UI elements to be referenced by EntityId
    // and to copy the owned UI element entities' DataElementNodes to a container
    bool MoveEntityAndDescendantsToListAndReplaceWithEntityId(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& elementNode,
        AZStd::vector<AZ::SerializeContext::DataElementNode>& entities)
    {
        // Find the UiElementComponent on this entity
        AZ::SerializeContext::DataElementNode* elementComponentNode =
            LyShine::FindComponentNode(elementNode, UiElementComponent::TYPEINFO_Uuid());
        if (!elementComponentNode)
        {
            return false;
        }

        // We must process the children first so that when we make a copy of this entity to the entities list
        // it will already have had its child entities replaced with entity IDs

        // find the m_children field
        int childrenIndex = elementComponentNode->FindElement(AZ_CRC("Children", 0xa197b1ba));
        if (childrenIndex == -1)
        {
            return false;
        }
        AZ::SerializeContext::DataElementNode& childrenNode = elementComponentNode->GetSubElement(childrenIndex);

        // iterate through children and recursively call this function
        int numChildren = childrenNode.GetNumSubElements();
        for (int childIndex = 0; childIndex < numChildren; ++childIndex)
        {
            AZ::SerializeContext::DataElementNode& childElementNode = childrenNode.GetSubElement(childIndex);
            MoveEntityAndDescendantsToListAndReplaceWithEntityId(context, childElementNode, entities);
        }

        // the children list has now been processed so it will now just contain entity IDs
        // Now copy this node (elementNode) to the list we are building and then replace it
        // with an Entity ID node

        // copy this node to the list
        entities.push_back(elementNode);

        // Remember the name of this node (it could be "element" or "RootElement" for example)
        AZStd::string elementFieldName = elementNode.GetNameString();

        // Find the EntityId node within this entity
        int entityIdIndex = elementNode.FindElement(AZ_CRC("Id", 0xbf396750));
        if (entityIdIndex == -1)
        {
            return false;
        }
        AZ::SerializeContext::DataElementNode& elementIdNode = elementNode.GetSubElement(entityIdIndex);

        // Find the sub node of the EntityID that actually stores the u64 and make a copy of it
        int u64Index = elementIdNode.FindElement(AZ_CRC("id", 0xbf396750));
        if (u64Index == -1)
        {
            return false;
        }
        AZ::SerializeContext::DataElementNode u64Node = elementIdNode.GetSubElement(u64Index);

        // Convert this node (which was an entire Entity) into just an EntityId, keeping the same
        // node name as it had
        elementNode.Convert<AZ::EntityId>(context, elementFieldName.c_str());

        // copy in the subNode that stores the actual u64 (that we saved a copy of above)
        int newEntityIdIndex = elementNode.AddElement(u64Node);

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool PrefabFileObject::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() == 1)
        {
            // this is an old UI prefab (prior to UI Slices). We need to move all of the owned child entities into a
            // separate list and have the references to them be via entity ID

            // Find the m_rootEntity in the PrefabFileObject, in the old format this is an entity,
            // we will replace it with an entityId
            int rootEntityIndex = classElement.FindElement(AZ_CRC("RootEntity", 0x3cead042));
            if (rootEntityIndex == -1)
            {
                return false;
            }
            AZ::SerializeContext::DataElementNode& rootEntityNode = classElement.GetSubElement(rootEntityIndex);

            // All UI element entities will be copied to this container and then added to the m_childEntities list
            AZStd::vector<AZ::SerializeContext::DataElementNode> copiedEntities;

            // recursively process the root element and all of its child elements, copying their child entities to the
            // entities container and replacing them with EntityIds
            if (!MoveEntityAndDescendantsToListAndReplaceWithEntityId(context, rootEntityNode, copiedEntities))
            {
                return false;
            }

            // Create the child entities member (which is a generic vector)
            using entityVector = AZStd::vector<AZ::Entity*>;
            AZ::SerializeContext::ClassData* classData = AZ::SerializeGenericTypeInfo<entityVector>::GetGenericInfo()->GetClassData();
            int entitiesIndex = classElement.AddElement(context, "Entities", *classData);
            if (entitiesIndex == -1)
            {
                return false;
            }
            AZ::SerializeContext::DataElementNode& entitiesNode = classElement.GetSubElement(entitiesIndex);

            // now add all of the copied entities to the entities vector node
            for (AZ::SerializeContext::DataElementNode& entityElement : copiedEntities)
            {
                entityElement.SetName("element");   // all elements in the Vector should have this name
                entitiesNode.AddElement(entityElement);
            }
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to VersionConverter to move three state actions from the derived interactable
    // to the interactable base class
    bool MoveToInteractableStateActions(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& srcClassElement,
        const char* stateActionsElementName,
        const char* colorElementName,
        const char* alphaElementName,
        const char* spriteElementName)
    {
        // Note, we can assume that srcClassElement will stay in the same place in memory during this function
        // But the base class (and everything in it) will move around in memory as we remove elements from
        // srcClassElement. So it is improtant not to hold only any indicies or references to the base class.
        int interactableBaseClassIndex = srcClassElement.FindElement(AZ_CRC("BaseClass1", 0xd4925735));

        // Add a new element for the state actions.
        int stateActionsIndex = srcClassElement.GetSubElement(interactableBaseClassIndex)
                .AddElement<AZStd::vector<UiInteractableStateAction*> >(context, stateActionsElementName);
        if (stateActionsIndex == -1)
        {
            // Error adding the new sub element
            AZ_Error("Serialization", false, "AddElement failed for %s", stateActionsElementName);
            return false;
        }

        {
            interactableBaseClassIndex = srcClassElement.FindElement(AZ_CRC("BaseClass1", 0xd4925735));
            AZ::SerializeContext::DataElementNode& dstClassElement = srcClassElement.GetSubElement(interactableBaseClassIndex);

            stateActionsIndex = dstClassElement.FindElement(AZ_CRC(stateActionsElementName));
            AZ::SerializeContext::DataElementNode& stateActionsNode = dstClassElement.GetSubElement(stateActionsIndex);

            int colorIndex = stateActionsNode.AddElement<UiInteractableStateColor*>(context, "element");
            AZ::SerializeContext::DataElementNode& colorNode = stateActionsNode.GetSubElement(colorIndex);

            if (!LyShine::MoveElement(context, srcClassElement, colorNode, colorElementName, "Color"))
            {
                return false;
            }

            {
                // In the latest version of UiInteractableStateColor the color is an AZ::Color but in the
                // version we are converting from (before UiInteractableStateColor existed) colors were stored
                // as Vector3. Since the UiInteractableStateColor we just created will be at the latest version
                // we need to convert the color to an AZ::Color now.
                // Note that indices will have changed since MoveElement was called.
                interactableBaseClassIndex = srcClassElement.FindElement(AZ_CRC("BaseClass1", 0xd4925735));
                AZ::SerializeContext::DataElementNode& dstBaseClassElement = srcClassElement.GetSubElement(interactableBaseClassIndex);
                stateActionsIndex = dstBaseClassElement.FindElement(AZ_CRC(stateActionsElementName));
                AZ::SerializeContext::DataElementNode& dstStateActionsNode = dstBaseClassElement.GetSubElement(stateActionsIndex);
                colorIndex = dstStateActionsNode.FindElement(AZ_CRC("element"));
                AZ::SerializeContext::DataElementNode& dstColorNode = dstStateActionsNode.GetSubElement(colorIndex);

                if (!LyShine::ConvertSubElementFromVector3ToAzColor(context, dstColorNode, "Color"))
                {
                    return false;
                }
            }
        }

        {
            interactableBaseClassIndex = srcClassElement.FindElement(AZ_CRC("BaseClass1", 0xd4925735));
            AZ::SerializeContext::DataElementNode& dstClassElement = srcClassElement.GetSubElement(interactableBaseClassIndex);

            stateActionsIndex = dstClassElement.FindElement(AZ_CRC(stateActionsElementName));
            AZ::SerializeContext::DataElementNode& stateActionsNode = dstClassElement.GetSubElement(stateActionsIndex);

            int alphaIndex = stateActionsNode.AddElement<UiInteractableStateAlpha*>(context, "element");
            AZ::SerializeContext::DataElementNode& alphaNode = stateActionsNode.GetSubElement(alphaIndex);
            if (!LyShine::MoveElement(context, srcClassElement, alphaNode, alphaElementName, "Alpha"))
            {
                return false;
            }
        }

        {
            interactableBaseClassIndex = srcClassElement.FindElement(AZ_CRC("BaseClass1", 0xd4925735));
            AZ::SerializeContext::DataElementNode& dstClassElement = srcClassElement.GetSubElement(interactableBaseClassIndex);

            stateActionsIndex = dstClassElement.FindElement(AZ_CRC(stateActionsElementName));
            AZ::SerializeContext::DataElementNode& stateActionsNode = dstClassElement.GetSubElement(stateActionsIndex);

            int spriteIndex = stateActionsNode.AddElement<UiInteractableStateSprite*>(context, "element");
            AZ::SerializeContext::DataElementNode& spriteNode = stateActionsNode.GetSubElement(spriteIndex);
            if (!LyShine::MoveElement(context, srcClassElement, spriteNode, spriteElementName, "Sprite"))
            {
                return false;
            }
        }

        // if the field did not exist then we do not report an error
        return true;
    }
}
