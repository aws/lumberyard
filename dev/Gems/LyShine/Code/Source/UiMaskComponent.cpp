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
#include "UiMaskComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "IRenderer.h"
#include <LyShine/IUiRenderer.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiVisualBus.h>

static const char* s_maskIncrProfileMarker = "UI_MASK_STENCIL_INCR";
static const char* s_maskDecrProfileMarker = "UI_MASK_STENCIL_DECR";

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiMaskComponent::UiMaskComponent()
    : m_enableMasking(true)
    , m_drawMaskVisualBehindChildren(false)
    , m_drawMaskVisualInFrontOfChildren(false)
    , m_useAlphaTest(false)
    , m_maskInteraction(true)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiMaskComponent::~UiMaskComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetupBeforeRenderingComponents(Pass pass)
{
    if (IUiRenderer::Get()->IsRenderingToMask())
    {
        // We are in the process of rendering a child element into the mask for another Mask component.
        // Additional masking while rendering a mask is not supported.
        AZ_Warning("UI", false, "An element with a Mask component is being used as a Child Mask Element.");
        return;
    }

    if (pass == Pass::First)
    {
        // record the base state at the start of the render for this element
        m_priorBaseState = IUiRenderer::Get()->GetBaseState();
    }

    // If using alpha test for drawing the renderable components on this element then we turn on
    // alpha test as a pre-render step
    int alphaTest = 0;
    if (m_useAlphaTest)
    {
        alphaTest = GS_ALPHATEST_GREATER;
    }

    // if either of the draw flags are checked then we may want to draw the renderable component(s)
    // on this element, otherwise use the color mask to stop them rendering
    int colorMask = GS_COLMASK_NONE;
    if ((m_drawMaskVisualBehindChildren && pass == Pass::First) ||
        (m_drawMaskVisualInFrontOfChildren && pass == Pass::Second))
    {
        colorMask = 0;
    }

    if (m_enableMasking)
    {
        // masking is enabled so we want to setup to increment (first pass) or decrement (second pass)
        // the stencil buff when rendering the renderable component(s) on this element
        int passOp = 0;
        if (pass == Pass::First)
        {
            passOp = STENCOP_PASS(FSS_STENCOP_INCR);
            gEnv->pRenderer->PushProfileMarker(s_maskIncrProfileMarker);
        }
        else
        {
            passOp = STENCOP_PASS(FSS_STENCOP_DECR);
            gEnv->pRenderer->PushProfileMarker(s_maskDecrProfileMarker);
        }

        // set up for stencil write
        const uint32 stencilRef = IUiRenderer::Get()->GetStencilRef();
        const uint32 stencilMask = 0xFF;
        const uint32 stencilWriteMask = 0xFF;
        const int32 stencilState = STENC_FUNC(FSS_STENCFUNC_EQUAL)
            | STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP) | passOp;
        gEnv->pRenderer->SetStencilState(stencilState, stencilRef, stencilMask, stencilWriteMask);

        // Set the base state that should be used when rendering the renderable component(s) on this
        // element
        IUiRenderer::Get()->SetBaseState(m_priorBaseState | GS_STENCIL | alphaTest | colorMask);
    }
    else
    {
        // masking is not enabled

        // Even if not masking we still use alpha test (if checked). This is primarily to help the user to
        // visualize what their alpha tested mask looks like.
        if (colorMask || alphaTest)
        {
            IUiRenderer::Get()->SetBaseState(m_priorBaseState | colorMask | alphaTest);
        }
    }

    if (m_childMaskElement.IsValid() && pass == Pass::First)
    {
        // There is a child mask element. Remember whether it is enabled since we disable it during normal rendering
        // of the child elements.
        EBUS_EVENT_ID_RESULT(m_priorChildMaskElementIsEnabled, m_childMaskElement, UiElementBus, IsEnabled);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetupAfterRenderingComponents(Pass pass)
{
    if (IUiRenderer::Get()->IsRenderingToMask())
    {
        // We are in the process of rendering a child element into the mask for another Mask component.
        // Additional masking while rendering a mask is not supported.
        return;
    }

    if (m_childMaskElement.IsValid())
    {
        // We have already rendered any visual component on this entity into the stencil buffer.
        // But we have a child mask element, which is an additional entity that gets rendered into the
        // stencil buffer.

        // if the child mask element was disabled then we don't want to render it as part of the mask
        // This allows the game to programatically enable and disable parts of the mask
        if (m_priorChildMaskElementIsEnabled)
        {
            // enable the rendering of the child mask element
            EBUS_EVENT_ID(m_childMaskElement, UiElementBus, SetIsEnabled, true);

            // Render the child mask element, this can render a whole hierarchy into the stencil buffer
            // as part of the mask.
            IUiRenderer::Get()->SetIsRenderingToMask(true);
            EBUS_EVENT_ID(m_childMaskElement, UiElementBus, RenderElement, false, false);
            IUiRenderer::Get()->SetIsRenderingToMask(false);
        }
    }

    if (m_enableMasking)
    {
        // Masking is enabled so on the first pass we want to increment the stencil ref stored
        // in the UiRenderer and used by all normal rendering, this is so that it matches the
        // increments to the stencil buffer that we have just done by rendering the mask.
        // On the second pass we want to decrement the stencil ref so it is back to what it
        // was before rendering the normal children of this mask element.
        if (pass == Pass::First)
        {
            IUiRenderer::Get()->IncrementStencilRef();
            gEnv->pRenderer->PopProfileMarker(s_maskIncrProfileMarker);
        }
        else
        {
            IUiRenderer::Get()->DecrementStencilRef();
            gEnv->pRenderer->PopProfileMarker(s_maskDecrProfileMarker);
        }

        // turn off stencil write and turn on stencil test
        const uint32 stencilRef = IUiRenderer::Get()->GetStencilRef();
        const uint32 stencilMask = 0xFF;
        const uint32 stencilWriteMask = 0x00;
        const int32 stencilState = STENC_FUNC(FSS_STENCFUNC_EQUAL)
            | STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP) | STENCOP_PASS(FSS_STENCOP_KEEP);
        gEnv->pRenderer->SetStencilState(stencilState, stencilRef, stencilMask, stencilWriteMask);

        if (pass == Pass::First)
        {
            // first pass, turn on stencil test for drawing children
            IUiRenderer::Get()->SetBaseState(m_priorBaseState | GS_STENCIL);
        }
        else
        {
            // second pass, set base state back to what it was before rendering this element
            IUiRenderer::Get()->SetBaseState(m_priorBaseState);
        }
    }
    else
    {
        // masking is not enabled
        // remove any color mask or alpha test that we set in pre-render
        IUiRenderer::Get()->SetBaseState(m_priorBaseState);
    }

    if (m_childMaskElement.IsValid())
    {
        if (pass == Pass::First)
        {
            // disable the rendering of the child mask with the other children
            // There will always be a second pass if there is a child mask element
            EBUS_EVENT_ID(m_childMaskElement, UiElementBus, SetIsEnabled, false);
        }
        else
        {
            // re-enable the rendering of the child mask (if it was enabled before we changed it)
            // This allows the game code to turn the child mask element on and off if so desired.
            EBUS_EVENT_ID(m_childMaskElement, UiElementBus, SetIsEnabled, m_priorChildMaskElementIsEnabled);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetupAfterRenderingChildren(bool& isSecondComponentsPassRequired)
{
    if (IUiRenderer::Get()->IsRenderingToMask())
    {
        // We are in the process of rendering a child element into the mask for another Mask component.
        // Additional masking while rendering a mask is not supported.
        return;
    }

    // When we are doing masking we need to request a second pass of rendering the components
    // in order to decrement the stencil buffer
    if (m_enableMasking || m_drawMaskVisualInFrontOfChildren || m_childMaskElement.IsValid())
    {
        isSecondComponentsPassRequired = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::GetIsMaskingEnabled()
{
    return m_enableMasking;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetIsMaskingEnabled(bool enableMasking)
{
    m_enableMasking = enableMasking;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::GetIsInteractionMaskingEnabled()
{
    return m_maskInteraction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetIsInteractionMaskingEnabled(bool enableInteractionMasking)
{
    m_maskInteraction = enableInteractionMasking;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::GetDrawBehind()
{
    return m_drawMaskVisualBehindChildren;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetDrawBehind(bool drawMaskVisualBehindChildren)
{
    m_drawMaskVisualBehindChildren = drawMaskVisualBehindChildren;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::GetDrawInFront()
{
    return m_drawMaskVisualInFrontOfChildren;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetDrawInFront(bool drawMaskVisualInFrontOfChildren)
{
    m_drawMaskVisualInFrontOfChildren = drawMaskVisualInFrontOfChildren;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::GetUseAlphaTest()
{
    return m_useAlphaTest;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetUseAlphaTest(bool useAlphaTest)
{
    m_useAlphaTest = useAlphaTest;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::IsPointMasked(AZ::Vector2 point)
{
    bool isMasked = false;

    // it is never masked if the flag to mask interactions is not checked
    if (m_maskInteraction)
    {
        // Initially consider it outside all of the mask visuals. If it is inside any we return false.
        isMasked = true;

        // Right now we only do a check for the rectangle. If the point is outside of the rectangle
        // then it is masked.
        // In the future we will add the option to check the alpha of the mask texture for interaction masking

        // first check this element if there is a visual component
        if (UiVisualBus::FindFirstHandler(GetEntityId()))
        {
            bool isInRect = false;
            EBUS_EVENT_ID_RESULT(isInRect, GetEntityId(), UiTransformBus, IsPointInRect, point);
            if (isInRect)
            {
                return false;
            }
        }

        // If there is a child mask element
        if (m_childMaskElement.IsValid())
        {
            // if it has a Visual component check if the point is in its rect
            if (UiVisualBus::FindFirstHandler(m_childMaskElement))
            {
                bool isInRect = false;
                EBUS_EVENT_ID_RESULT(isInRect, m_childMaskElement, UiTransformBus, IsPointInRect, point);
                if (isInRect)
                {
                    return false;
                }
            }

            // get any descendants of the child mask element that have visual components
            LyShine::EntityArray childMaskElements;
            EBUS_EVENT_ID(m_childMaskElement, UiElementBus, FindDescendantElements, 
                [](const AZ::Entity* descendant) { return UiVisualBus::FindFirstHandler(descendant->GetId()) != nullptr; },
                childMaskElements);

            // if the point is in any of their rects then it is not masked out
            for (auto child : childMaskElements)
            {
                bool isInRect = false;
                EBUS_EVENT_ID_RESULT(isInRect, child->GetId(), UiTransformBus, IsPointInRect, point);
                if (isInRect)
                {
                    return false;
                }
            }
        }
    }

    return isMasked;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiMaskComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiMaskComponent, AZ::Component>()
            ->Version(1)
            ->Field("EnableMasking", &UiMaskComponent::m_enableMasking)
            ->Field("DrawBehind", &UiMaskComponent::m_drawMaskVisualBehindChildren)
            ->Field("DrawInFront", &UiMaskComponent::m_drawMaskVisualInFrontOfChildren)
            ->Field("UseAlphaTest", &UiMaskComponent::m_useAlphaTest)
            ->Field("MaskInteraction", &UiMaskComponent::m_maskInteraction)
            ->Field("ChildMaskElement", &UiMaskComponent::m_childMaskElement);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiMaskComponent>("Mask", "A component that masks child elements using its visual component");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiMask.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiMask.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiMaskComponent::m_enableMasking, "Enable masking",
                "When checked, only the parts of child elements that are revealed by the mask will be seen.");

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiMaskComponent::m_drawMaskVisualBehindChildren, "Draw behind",
                "Check this box to draw the mask visual behind the child elements.");

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiMaskComponent::m_drawMaskVisualInFrontOfChildren, "Draw in front",
                "Check this box to draw the mask in front of the child elements.");

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiMaskComponent::m_useAlphaTest, "Use alpha test",
                "Check this box to use the alpha channel in the mask visual's texture to define the mask.");

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiMaskComponent::m_maskInteraction, "Mask interaction",
                "Check this box to prevent children hidden by the mask from getting input events.");

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiMaskComponent::m_childMaskElement, "Child mask element",
                "A child element that is rendered as part of the mask.")
                ->Attribute(AZ::Edit::Attributes::EnumValues, &UiMaskComponent::PopulateChildEntityList);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiMaskBus>("UiMaskBus")
            ->Event("GetIsMaskingEnabled", &UiMaskBus::Events::GetIsMaskingEnabled)
            ->Event("SetIsMaskingEnabled", &UiMaskBus::Events::SetIsMaskingEnabled)
            ->Event("GetIsInteractionMaskingEnabled", &UiMaskBus::Events::GetIsInteractionMaskingEnabled)
            ->Event("SetIsInteractionMaskingEnabled", &UiMaskBus::Events::SetIsInteractionMaskingEnabled)
            ->Event("GetDrawBehind", &UiMaskBus::Events::GetDrawBehind)
            ->Event("SetDrawBehind", &UiMaskBus::Events::SetDrawBehind)
            ->Event("GetDrawInFront", &UiMaskBus::Events::GetDrawInFront)
            ->Event("SetDrawInFront", &UiMaskBus::Events::SetDrawInFront)
            ->Event("GetUseAlphaTest", &UiMaskBus::Events::GetUseAlphaTest)
            ->Event("SetUseAlphaTest", &UiMaskBus::Events::SetUseAlphaTest);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::Activate()
{
    UiRenderControlBus::Handler::BusConnect(m_entity->GetId());
    UiMaskBus::Handler::BusConnect(m_entity->GetId());
    UiInteractionMaskBus::Handler::BusConnect(m_entity->GetId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::Deactivate()
{
    UiRenderControlBus::Handler::BusDisconnect();
    UiMaskBus::Handler::BusDisconnect();
    UiInteractionMaskBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiMaskComponent::EntityComboBoxVec UiMaskComponent::PopulateChildEntityList()
{
    EntityComboBoxVec result;

    // add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(AZ::EntityId()), "<None>"));

    // Get a list of all child elements
    LyShine::EntityArray matchingElements;
    EBUS_EVENT_ID(GetEntityId(), UiElementBus, FindDescendantElements,
        [](const AZ::Entity* entity) { return true; },
        matchingElements);

    // add their names to the StringList and their IDs to the id list
    for (auto childEntity : matchingElements)
    {
        result.push_back(AZStd::make_pair(AZ::EntityId(childEntity->GetId()), childEntity->GetName()));
    }

    return result;
}
