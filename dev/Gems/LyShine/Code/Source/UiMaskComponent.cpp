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
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetupAfterRenderingComponents(Pass pass)
{
    if (m_enableMasking)
    {
        // masking is enabled so on the first pass we want to increment the stencil ref stored
        // in the UiRenderer and used by all normal rendering. On the second pass we want to
        // decrement it so it is back to what it was before rendering the children of this mask
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
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetupAfterRenderingChildren(bool& isSecondComponentsPassRequired)
{
    // When we are doing masking we need to request a second pass of rendering the components
    // in order to decrement the stencil buffer
    if (m_enableMasking || m_drawMaskVisualInFrontOfChildren)
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
        // Right now we only do a check for the rectangle. If the point is outside of the rectacngle
        // then it is masked.
        // In the future we will add the option to check the alpha of the mask texture for interaction masking
        bool isInRect = false;
        EBUS_EVENT_ID_RESULT(isInRect, GetEntityId(), UiTransformBus, IsPointInRect, point);
        if (!isInRect)
        {
            isMasked = true;
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
            ->Field("MaskInteraction", &UiMaskComponent::m_maskInteraction);

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
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiMaskBus>("UiMaskBus")
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
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
