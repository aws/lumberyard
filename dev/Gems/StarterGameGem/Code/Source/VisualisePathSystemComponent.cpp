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
#include "VisualisePathSystemComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Math/MathUtils.h>

#include <IRenderAuxGeom.h>
#include <MathConversion.h>


namespace StarterGameGem
{
	VisualisePathSystemComponent* g_vpsc_instance = nullptr;

	VisualisePathSystemComponent* VisualisePathSystemComponent::GetInstance()
	{ 
		return g_vpsc_instance;
	}

	void VisualisePathSystemComponent::Reflect(AZ::ReflectContext* context)
	{
		if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
		{
			serializeContext->Class<VisualisePathSystemComponent, AZ::Component>()
				->Version(1)
				->SerializerForEmptyClass()
			;

			if (AZ::EditContext* editContext = serializeContext->GetEditContext())
			{
				editContext->Class<VisualisePathSystemComponent>("Visualise Path System", "Renders lines for given paths for debugging")
					->ClassElement(AZ::Edit::ClassElements::EditorData, "")
						->Attribute(AZ::Edit::Attributes::Category, "Game")
						->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
				;
			}
		}

		if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
		{
			behaviorContext->EBus<VisualisePathSystemRequestBus>("VisualisePathSystemRequestBus")
                ->Event("AddPath", &VisualisePathSystemRequestBus::Events::AddPath)
                ->Event("ClearPath", &VisualisePathSystemRequestBus::Events::ClearPath)
			;
		}
	}

	void VisualisePathSystemComponent::Activate()
	{
#if defined(_DEBUG) || defined(_PROFILE)
		VisualisePathSystemRequestBus::Handler::BusConnect();
		AZ::TickBus::Handler::BusConnect();
#endif

		g_vpsc_instance = this;
	}

	void VisualisePathSystemComponent::Deactivate()
	{
#if defined(_DEBUG) || defined(_PROFILE)
		VisualisePathSystemRequestBus::Handler::BusDisconnect();
#endif
		//AZ::TickBus::Handler::BusDisconnect();

		g_vpsc_instance = nullptr;
	}

	void VisualisePathSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
	{
		IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
		SAuxGeomRenderFlags flags;
		flags.SetAlphaBlendMode(e_AlphaNone);
		flags.SetCullMode(e_CullModeBack);
		flags.SetDepthTestFlag(e_DepthTestOn);
		flags.SetFillMode(e_FillModeSolid);
		renderAuxGeom->SetRenderFlags(flags);

		static ICVar* const cvarDraw = gEnv->pConsole->GetCVar("ai_debugDrawPaths");
		if (cvarDraw)
		{
			int val = cvarDraw->GetIVal();
			if (val > 0)
			{
				Vec3 raised = Vec3(0.0f, 0.0, 0.5f);
				for (AZStd::list<Path>::const_iterator it = m_paths.begin(); it != m_paths.end(); ++it)
				{
					Vec3* positions = new Vec3[it->path.size() + 1];
					int i = 0;
					for (i; i < it->path.size(); ++i)
					{
						positions[i] = it->path[i] + raised;
					}

					// If the value is 2 then draw a line from the entity to the start and end
					// points of the line as well (so we can easily determine who the line belongs
					// to).
					if (val == 2)
					{
						AZ::Transform tm = AZ::Transform::CreateIdentity();
						AZ::TransformBus::EventResult(tm, it->entity, &AZ::TransformInterface::GetWorldTM);
						positions[i] = AZVec3ToLYVec3(tm.GetTranslation());
						++i;	// we've added another point (the +1 from when we created the array)
					}

					renderAuxGeom->DrawPolyline(positions, i, val == 2, ColorB(0, 255, 0, 255));
				}
			}
		}
	}

    void VisualisePathSystemComponent::AddPath(const AZ::EntityId& id, const StarterGameNavigationComponentNotifications::StarterGameNavPath& path)
    {
#if defined(_DEBUG) || defined(_PROFILE)
        Path* newPath = this->GetPath(id);
        if (newPath == nullptr)
        {
            m_paths.push_front();
            newPath = &m_paths.front();
            newPath->entity = id;
        }

        newPath->path.clear();

        for (AZStd::vector<AZ::Vector3>::const_iterator it = path.m_points.begin(); it != path.m_points.end(); ++it)
        {
            newPath->path.push_back(AZVec3ToLYVec3(*it));
        }
#endif
    }

    void VisualisePathSystemComponent::ClearPath(const AZ::EntityId& id)
    {
#if defined(_DEBUG) || defined(_PROFILE)
        AZStd::list<Path>::iterator it = m_paths.begin();
        for (it; it != m_paths.end(); ++it)
        {
            if (it->entity == id)
            {
                // The EntityId is used as the key so there should only be one in the vector.
                m_paths.erase(it);
                break;
            }
        }
#endif
    }

	VisualisePathSystemComponent::Path* VisualisePathSystemComponent::GetPath(const AZ::EntityId& id)
	{
		AZStd::list<Path>::iterator it = m_paths.begin();
		for (it; it != m_paths.end(); ++it)
		{
			if (it->entity == id)
			{
				break;
			}
		}
		
		if (it == m_paths.end())
			return nullptr;
		else
			return &(*it);
	}

} // namespace StarterGameGem


