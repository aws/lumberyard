/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "precompiled.h"
#include "Rotate.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <ScriptCanvas/Core/GraphBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Entity
        {
            const char* Rotate::k_setEntityName("Entity");
            const char* Rotate::k_setAnglesName("Euler Angles");

            void Rotate::Reflect(AZ::ReflectContext* reflection)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
                if (serializeContext)
                {
                    serializeContext->Class<Rotate, Node>()
                        ->Version(3)
                        ;

                    AZ::EditContext* editContext = serializeContext->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<Rotate>("Rotate by Euler Angles", "Rotates the specified entity in world space using Euler angles.")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Entity/Transform")
                            ->Attribute(AZ::Edit::Attributes::CategoryStyle, ".method")
                            ->Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "MethodNodeTitlePalette")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Rotate.png")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ;
                    }
                }
            }

            void Rotate::OnInit()
            {
                AddSlot("In", "", SlotType::ExecutionIn);
                AddSlot("Out", "", SlotType::ExecutionOut);
                AddInputDatumSlot(k_setEntityName, "The entity to apply the rotation on.", Datum::eOriginality::Original, ScriptCanvas::SelfReferenceId);
                AddInputDatumSlot(k_setAnglesName, "Euler angles, Pitch/Yaw/Roll.", Data::Type::Vector3(), Datum::eOriginality::Original);
            }

            void Rotate::OnInputSignal(const SlotId&)
            {
                AZ::EntityId targetEntity;
                if (auto input = GetInput(GetSlotId(k_setEntityName)))
                {
                    if (auto entityId = input->GetAs<AZ::EntityId>())
                    {
                        if (entityId->IsValid())
                        {
                            targetEntity = *entityId;
                        }
                    }
                }

                if (!targetEntity.IsValid())
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Invalid entity specified");
                    return;
                }

                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, targetEntity);
                if (entity)
                {
                    AZ::Vector3 angles = AZ::Vector3::CreateZero();
                    if (entity->GetState() == AZ::Entity::ES_ACTIVE)
                    {
                        if (auto input = GetInput(GetSlotId(k_setAnglesName)))
                        {
                            if (auto eulerAngles = input->GetAs<AZ::Vector3>())
                            {
                                angles = *eulerAngles;
                            }
                        }

                        AZ::Transform rotation(
                              AZ::Transform::CreateRotationX(AZ::DegToRad(angles.GetX()))
                            * AZ::Transform::CreateRotationY(AZ::DegToRad(angles.GetY()))
                            * AZ::Transform::CreateRotationZ(AZ::DegToRad(angles.GetZ()))
                        );

                        AZ::Transform currentTransform = AZ::Transform::CreateIdentity();
                        AZ::TransformBus::EventResult(currentTransform, targetEntity, &AZ::TransformInterface::GetWorldTM);
                        
                        auto position = currentTransform.GetTranslation();
                        AZ::Transform newTransform = rotation * currentTransform;
                        newTransform.SetTranslation(position);
                        AZ::TransformBus::Event(targetEntity, &AZ::TransformInterface::SetWorldTM, newTransform);
                    }
                }

                SignalOutput(GetSlotId("Out"));
            }
        }
    }
}
