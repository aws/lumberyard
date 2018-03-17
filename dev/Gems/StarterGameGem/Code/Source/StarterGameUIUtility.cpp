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

#include "StarterGameGem_precompiled.h"
#include "StarterGameUIUtility.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <MathConversion.h>
#include <AZCore/Component/TransformBus.h>
//#include "UiElementComponent.h"
#include <UiCanvasManager.h>
#include <UiTransform2dComponent.h>
#include <UiFaderComponent.h>
#include <UiScrollBarComponent.h>
#include <UiSliderComponent.h>
#include <UiTextComponent.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
//#include "LyShine/Code/Source/UiElementComponent.h"
#include <UiElementComponent.h>

namespace StarterGameGem
{
    void StarterGameUIUtility::UIFaderControl(const AZ::EntityId& canvasEntityID, const int& faderID, const float &fadeValue, const float& fadeTime)
    {
        if (!canvasEntityID.IsValid())
        {
            // AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIFaderControl: %s Component: is not valid\n", canvasEntityID.ToString());
            return;
        }

        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, faderID);
        if (element)
        {
            const AZ::Entity::ComponentArrayType& components = element->GetComponents();
            UiFaderComponent* faderComp = NULL;
            for (int count = 0; count < components.size(); ++count)
            {
                faderComp = azdynamic_cast<UiFaderComponent*>(element->GetComponents()[count]);

                if (faderComp != NULL)
                {
                    break;
                }
            }

            if (faderComp)
            {
				// fade from the value that i am at to the value that i want over the time specified.
				// so i need to calculate the "speed" of the fade
				const float currentFade = faderComp->GetFadeValue();
				float fadeSpeed = (fadeTime == 0) ? 0 : (abs(currentFade - fadeValue) / fadeTime);
                faderComp->Fade(fadeValue, fadeSpeed);
            }
            else
            {
                //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIFaderControl: %s Component: couldn't find a fader component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
            }
        }
        else
        {
            //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIFaderControl: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
        }
    }

    void StarterGameUIUtility::UIScrollControl(const AZ::EntityId& canvasEntityID, const int& scrollID, const float &value)
    {
        if (!canvasEntityID.IsValid())
        {
            // AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIScrollControl: %s Component: is not valid\n", canvasEntityID.ToString());
            return;
        }

        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, scrollID);
        if (element)
        {
            const AZ::Entity::ComponentArrayType& components = element->GetComponents();
            UiScrollBarComponent* scrollComp = NULL;
            for (int count = 0; count < components.size(); ++count)
            {
                scrollComp = azdynamic_cast<UiScrollBarComponent*>(element->GetComponents()[count]);

                if (scrollComp != NULL)
                {
                    break;
                }
            }

            if (scrollComp)
            {
                scrollComp->SetValue(value);
            }
            else
            {
                //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIScrollControl: %s Component: couldn't find a scroll component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
            }
        }
        else
        {
            //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIScrollControl: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
        }
    }

    void StarterGameUIUtility::UISliderControl(const AZ::EntityId& canvasEntityID, const int& sliderID, const float &value)
    {
        if (!canvasEntityID.IsValid())
        {
            // AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: is not valid\n", canvasEntityID.ToString());
            return;
        }

        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, sliderID);
        if (element)
        {
            const AZ::Entity::ComponentArrayType& components = element->GetComponents();
            UiSliderComponent* sliderComp = NULL;
            for (int count = 0; count < components.size(); ++count)
            {
                sliderComp = azdynamic_cast<UiSliderComponent*>(element->GetComponents()[count]);

                if (sliderComp != NULL)
                {
                    break;
                }
            }

            if (sliderComp)
            {
                sliderComp->SetValue(value);
            }
            else
            {
                //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: couldn't find a slider component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
            }
        }
        else
        {
            //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
        }
    }

    void StarterGameUIUtility::UIElementMover(const AZ::EntityId& canvasEntityID, const int& elementID, const AZ::Vector2& value)
    {
        if (!canvasEntityID.IsValid())
        {
            // AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementMover: %s Component: is not valid\n", canvasEntityID.ToString());
            return;
        }

        AZ::Entity* element = nullptr;
        EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, elementID);
        if (element)
        {
            const AZ::Entity::ComponentArrayType& components = element->GetComponents();
            UiTransform2dComponent* transformComp = NULL;
            for (int count = 0; count < components.size(); ++count)
            {
                transformComp = azdynamic_cast<UiTransform2dComponent*>(element->GetComponents()[count]);

                if (transformComp != NULL)
                {
                    break;
                }
            }

            if (transformComp)
            {
                UiTransform2dInterface::Offsets offs(transformComp->GetOffsets());
                offs.m_left = offs.m_right = value.GetX();
                offs.m_top = offs.m_bottom = value.GetY();
                transformComp->SetOffsets(offs);
            }
            else
            {
                //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementMover: %s Component: couldn't find a transform component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
            }
        }
        else
        {
            //AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementMover: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
        }
    }

	void StarterGameUIUtility::UIElementScaler(const AZ::EntityId& canvasEntityID, const int& elementID, const AZ::Vector2& value)
	{
		if (!canvasEntityID.IsValid())
		{
			// AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementScaler: %s Component: is not valid\n", canvasEntityID.ToString());
			return;
		}

		AZ::Entity* element = nullptr;
		EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, elementID);
		if (element)
		{
			const AZ::Entity::ComponentArrayType& components = element->GetComponents();
			UiTransform2dComponent* transformComp = NULL;
			for (int count = 0; count < components.size(); ++count)
			{
				transformComp = azdynamic_cast<UiTransform2dComponent*>(element->GetComponents()[count]);

				if (transformComp != NULL)
				{
					break;
				}
			}

			if (transformComp)
			{
				transformComp->SetScale(value);
			}
			else
			{
				//AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementScaler: %s Component: couldn't find a transform component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
			}
		}
		else
		{
			//AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementScaler: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
		}
	}

	void StarterGameUIUtility::UIElementRotator(const AZ::EntityId& canvasEntityID, const int& elementID, const float& value)
	{
		if (!canvasEntityID.IsValid())
		{
			// AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementRotator: %s Component: is not valid\n", canvasEntityID.ToString());
			return;
		}

		AZ::Entity* element = nullptr;
		EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, elementID);
		if (element)
		{
			const AZ::Entity::ComponentArrayType& components = element->GetComponents();
			UiTransform2dComponent* transformComp = NULL;
			for (int count = 0; count < components.size(); ++count)
			{
				transformComp = azdynamic_cast<UiTransform2dComponent*>(element->GetComponents()[count]);

				if (transformComp != NULL)
				{
					break;
				}
			}

			if (transformComp)
			{
				transformComp->SetZRotation(value);
			}
			else
			{
				//AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementRotator: %s Component: couldn't find a transform component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
			}
		}
		else
		{
			//AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UIElementRotator: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
		}
	}

	void StarterGameUIUtility::UITextSetter(const AZ::EntityId& canvasEntityID, const int& textID, const AZStd::string& value)
	{
		if (!canvasEntityID.IsValid())
		{
			// AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: is not valid\n", canvasEntityID.ToString());
			return;
		}

		AZ::Entity* element = nullptr;
		EBUS_EVENT_ID_RESULT(element, canvasEntityID, UiCanvasBus, FindElementById, textID);
		if (element)
		{
			const AZ::Entity::ComponentArrayType& components = element->GetComponents();
			UiTextComponent* textComp = NULL;
			for (int count = 0; count < components.size(); ++count)
			{
				textComp = azdynamic_cast<UiTextComponent*>(element->GetComponents()[count]);

				if (textComp != NULL)
				{
					break;
				}
			}

			if (textComp)
			{
				textComp->SetTextWithFlags(value, UiTextInterface::SetEscapeMarkup);
			}
			else
			{
				//AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: couldn't find a transform component on UI element with ID: %d\n", canvasEntityID.ToString(), faderID);
			}
		}
		else
		{
			//AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: couldn't find an image component with ID: %d\n", canvasEntityID.ToString(), faderID);
		}
	}

    bool StarterGameUIUtility::GetElementPosOnScreen(const AZ::EntityId& entityId, AZ::Vector2& value)
    {
        if (!entityId.IsValid())
        {
            // AZ_Warning("StarterGame", false, VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "StarterGameUtility::UISliderControl: %s Component: is not valid\n", canvasEntityID.ToString());
            return false;
        }

        AZ::Transform transform;
        EBUS_EVENT_ID_RESULT(transform, entityId, AZ::TransformBus, GetWorldTM);

        const CCamera& camera = gEnv->pRenderer->GetCamera();
        Vec3 screenPos;
        if (camera.Project(AZVec3ToLYVec3(transform.GetPosition()), screenPos, Vec2i(0, 0), Vec2i(0, 0)))
        {
            value.SetX(screenPos.x);
            value.SetY(screenPos.y); // the height is z
            return true;
        }
        return false;
    }

	AZ::EntityId StarterGameUIUtility::CloneElement(const AZ::EntityId& sourceEntityID, const AZ::EntityId& parentEntityID)
	{
		AZ::Entity* sourceEntity = nullptr;
		EBUS_EVENT_RESULT(sourceEntity, AZ::ComponentApplicationBus, FindEntity, sourceEntityID);

		AZ::Entity* parentEntity = nullptr;
		EBUS_EVENT_RESULT(parentEntity, AZ::ComponentApplicationBus, FindEntity, parentEntityID);

		AZ::EntityId canvasEntityId;
		EBUS_EVENT_ID_RESULT(canvasEntityId, sourceEntityID, UiElementBus, GetCanvasEntityId);

		AZ::Entity* clonedElement = nullptr;
		EBUS_EVENT_ID_RESULT(clonedElement, canvasEntityId, UiCanvasBus, CloneElement, sourceEntity, parentEntity);

		return clonedElement->GetId();
	}
	
	void StarterGameUIUtility::DeleteElement(const AZ::EntityId& entityID)
	{
		AZ::Entity* element = nullptr;
		EBUS_EVENT_RESULT(element, AZ::ComponentApplicationBus, FindEntity, entityID);
		if (element)
		{
			EBUS_EVENT_ID(element->GetId(), UiElementBus, DestroyElement);
		}
	}

    void StarterGameUIUtility::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<StarterGameUIUtility>("StarterGameUIUtility")
                ->Method("GetElementPosOnScreen", &GetElementPosOnScreen)
                ->Method("UIFaderControl", &UIFaderControl)
                ->Method("UIScrollControl", &UIScrollControl)
                ->Method("UISliderControl", &UISliderControl)
                ->Method("UIElementMover", &UIElementMover)
                ->Method("UIElementScaler", &UIElementScaler)
                ->Method("UIElementRotator", &UIElementRotator)
                ->Method("UITextSetter", &UITextSetter)
				->Method("CloneElement", &CloneElement)
 				->Method("DeleteElement", &DeleteElement)
				->Constant("ButtonType_Invalid", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_Invalid))
				->Constant("ButtonType_Blank", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_Blank))
				->Constant("ButtonType_Text", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_Text))
				->Constant("ButtonType_A", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_A))
				->Constant("ButtonType_B", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_B))
				->Constant("ButtonType_X", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_X))
				->Constant("ButtonType_Y", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_Y))
				->Constant("ButtonType_DUp", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_DUp))
				->Constant("ButtonType_DDown", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_DDown))
				->Constant("ButtonType_DLeft", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_DLeft))
				->Constant("ButtonType_DRight", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_DRight))
				->Constant("ButtonType_Cross", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_Cross))
				->Constant("ButtonType_Circle", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_Circle))
				->Constant("ButtonType_Square", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_Square))
				->Constant("ButtonType_Triangle", BehaviorConstant(StarterGameUIButtonType::StarterGameUIButtonType_Triangle))
				->Constant("ActionType_Invalid", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_Invalid))
				->Constant("ActionType_Start", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_Start))
				->Constant("ActionType_NavLeft", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_NavLeft))
				->Constant("ActionType_NavRight", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_NavRight))
				->Constant("ActionType_NavUp", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_NavUp))
				->Constant("ActionType_NavDown", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_NavDown))
				->Constant("ActionType_Confirm", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_Confirm))
				->Constant("ActionType_CancelBack", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_CancelBack))
				->Constant("ActionType_TabLeft", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_TabLeft))
				->Constant("ActionType_TabRight", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_TabRight))
				->Constant("ActionType_Pause", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_Pause))
				->Constant("ActionType_SlideLeft", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_SlideLeft))
				->Constant("ActionType_SlideRight", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_SlideRight))
				->Constant("ActionType_Use", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_Use))
 				->Constant("ActionType_TimeOfDayNext", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_TimeOfDayNext))
				->Constant("ActionType_TimeOfDayPrev", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_TimeOfDayPrev))
				->Constant("ActionType_AAModeNext", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_AAModeNext))
				->Constant("ActionType_AAModePrev", BehaviorConstant(StarterGameUIActionType::StarterGameUIActionType_AAModePrev))
			;
        
			behaviorContext->Class<TrackerParams>("TrackerParams")
				->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
				->Property("trackedItem", BehaviorValueProperty(&TrackerParams::m_trackedItem))
				->Property("graphic", BehaviorValueProperty(&TrackerParams::m_graphic))
				->Property("visible", BehaviorValueProperty(&TrackerParams::m_visible))
				;
		}
    }

}
