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

#include <LyShine/Bus/UiParticleEmitterBus.h>
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

        // Vec2 (still used in UI Animation sequence splines)
        {
            if (serializeContext)
            {
                serializeContext->Class<Vec2>()->
                    Field("x", &Vec2::x)->
                    Field("y", &Vec2::y);
            }
        }

        // Vec3 (possibly no longer used)
        {
            if (serializeContext)
            {
                serializeContext->Class<Vec3>()->
                    Field("x", &Vec3::x)->
                    Field("y", &Vec3::y)->
                    Field("z", &Vec3::z);
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
            }

            if (behaviorContext)
            {
                behaviorContext->Class<UiTransform2dInterface::Anchors>("UiAnchors")
                    ->Constructor<>()
                    ->Constructor<float, float, float, float>()
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &UiAnchorsScriptConstructor)
                    ->Property("left", BehaviorValueProperty(&UiTransform2dInterface::Anchors::m_left))
                    ->Property("top", BehaviorValueProperty(&UiTransform2dInterface::Anchors::m_top))
                    ->Property("right", BehaviorValueProperty(&UiTransform2dInterface::Anchors::m_right))
                    ->Property("bottom", BehaviorValueProperty(&UiTransform2dInterface::Anchors::m_bottom))
                    ->Method("SetLeft", [](UiTransform2dInterface::Anchors* thisPtr, float left) { thisPtr->m_left = left; })
                    ->Method("SetTop", [](UiTransform2dInterface::Anchors* thisPtr, float top) { thisPtr->m_top = top; })
                    ->Method("SetRight", [](UiTransform2dInterface::Anchors* thisPtr, float right) { thisPtr->m_right = right; })
                    ->Method("SetBottom", [](UiTransform2dInterface::Anchors* thisPtr, float bottom) { thisPtr->m_bottom = bottom; })
                    ->Method("SetAnchors", [](UiTransform2dInterface::Anchors* thisPtr, float left, float top, float right, float bottom)
                    {
                        thisPtr->m_left = left;
                        thisPtr->m_top = top;
                        thisPtr->m_right = right;
                        thisPtr->m_bottom = bottom;
                    });
            }
        }

        // ParticleColorKeyframe
        {
            if (serializeContext)
            {
                serializeContext->Class<UiParticleEmitterInterface::ParticleColorKeyframe>()
                    ->Field("Time", &UiParticleEmitterInterface::ParticleColorKeyframe::time)
                    ->Field("Color", &UiParticleEmitterInterface::ParticleColorKeyframe::color)
                    ->Field("InTangent", &UiParticleEmitterInterface::ParticleColorKeyframe::inTangent)
                    ->Field("OutTangent", &UiParticleEmitterInterface::ParticleColorKeyframe::outTangent);
            }
        }

        // ParticleFloatKeyframe
        {
            if (serializeContext)
            {
                serializeContext->Class<UiParticleEmitterInterface::ParticleFloatKeyframe>()
                    ->Field("Time", &UiParticleEmitterInterface::ParticleFloatKeyframe::time)
                    ->Field("Multiplier", &UiParticleEmitterInterface::ParticleFloatKeyframe::multiplier)
                    ->Field("InTangent", &UiParticleEmitterInterface::ParticleFloatKeyframe::inTangent)
                    ->Field("OutTangent", &UiParticleEmitterInterface::ParticleFloatKeyframe::outTangent);
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
            }

            if (behaviorContext)
            {
                behaviorContext->Class<UiTransform2dInterface::Offsets>("UiOffsets")
                    ->Constructor<>()
                    ->Constructor<float, float, float, float>()
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &UiOffsetsScriptConstructor)
                    ->Property("left", BehaviorValueProperty(&UiTransform2dInterface::Offsets::m_left))
                    ->Property("top", BehaviorValueProperty(&UiTransform2dInterface::Offsets::m_top))
                    ->Property("right", BehaviorValueProperty(&UiTransform2dInterface::Offsets::m_right))
                    ->Property("bottom", BehaviorValueProperty(&UiTransform2dInterface::Offsets::m_bottom))
                    ->Method("SetLeft", [](UiTransform2dInterface::Offsets* thisPtr, float left) { thisPtr->m_left = left; })
                    ->Method("SetTop", [](UiTransform2dInterface::Offsets* thisPtr, float top) { thisPtr->m_top = top; })
                    ->Method("SetRight", [](UiTransform2dInterface::Offsets* thisPtr, float right) { thisPtr->m_right = right; })
                    ->Method("SetBottom", [](UiTransform2dInterface::Offsets* thisPtr, float bottom) { thisPtr->m_bottom = bottom; })
                    ->Method("SetOffsets", [](UiTransform2dInterface::Offsets* thisPtr, float left, float top, float right, float bottom)
                    {
                        thisPtr->m_left = left;
                        thisPtr->m_top = top;
                        thisPtr->m_right = right;
                        thisPtr->m_bottom = bottom;
                    });
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
            }

            if (behaviorContext)
            {
                behaviorContext->Class<UiLayoutInterface::Padding>("UiPadding")
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Property("left", BehaviorValueProperty(&UiLayoutInterface::Padding::m_left))
                    ->Property("right", BehaviorValueProperty(&UiLayoutInterface::Padding::m_right))
                    ->Property("top", BehaviorValueProperty(&UiLayoutInterface::Padding::m_top))
                    ->Property("bottom", BehaviorValueProperty(&UiLayoutInterface::Padding::m_bottom))
                    ->Method("SetLeft", [](UiLayoutInterface::Padding* thisPtr, int left) { thisPtr->m_left = left; })
                    ->Method("SetTop", [](UiLayoutInterface::Padding* thisPtr, int top) { thisPtr->m_top = top; })
                    ->Method("SetRight", [](UiLayoutInterface::Padding* thisPtr, int right) { thisPtr->m_right = right; })
                    ->Method("SetBottom", [](UiLayoutInterface::Padding* thisPtr, int bottom) { thisPtr->m_bottom = bottom; })
                    ->Method("SetPadding", [](UiLayoutInterface::Padding* thisPtr, int left, int top, int right, int bottom)
                    {
                        thisPtr->m_left = left;
                        thisPtr->m_top = top;
                        thisPtr->m_right = right;
                        thisPtr->m_bottom = bottom;
                    });
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
                ->Event("GetHorizontalChildAlignment", &UiLayoutBus::Events::GetHorizontalChildAlignment)
                ->Event("SetHorizontalChildAlignment", &UiLayoutBus::Events::SetHorizontalChildAlignment)
                ->Event("GetVerticalChildAlignment", &UiLayoutBus::Events::GetVerticalChildAlignment)
                ->Event("SetVerticalChildAlignment", &UiLayoutBus::Events::SetVerticalChildAlignment)
                ->Event("GetIgnoreDefaultLayoutCells", &UiLayoutBus::Events::GetIgnoreDefaultLayoutCells)
                ->Event("SetIgnoreDefaultLayoutCells", &UiLayoutBus::Events::SetIgnoreDefaultLayoutCells);
        }
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
            if (!UiElementComponent::MoveEntityAndDescendantsToListAndReplaceWithEntityId(context, rootEntityNode, -1, copiedEntities))
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
