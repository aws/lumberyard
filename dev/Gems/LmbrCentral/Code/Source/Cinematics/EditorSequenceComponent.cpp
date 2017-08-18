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
#include "StdAfx.h"
#include "EditorSequenceComponent.h"
#include "EditorSequenceAgentComponent.h"

#include "Objects/EntityObject.h"
#include "TrackView/TrackViewSequenceManager.h"

#include <AzCore/Math/Uuid.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <LmbrCentral/Cinematics/SequenceAgentComponentBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace LmbrCentral
{
    /*static*/ AZ::ScriptTimePoint EditorSequenceComponent::s_lastPropertyRefreshTime;
    /*static*/ const double        EditorSequenceComponent::s_refreshPeriodMilliseconds = 200.0;  // 5 Hz refresh rate
    /*static*/ const int           EditorSequenceComponent::s_invalidSequenceId = -1;

    namespace ClassConverters
    {
        static bool UpVersionAnimationData(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode&);
    } // namespace ClassConverters

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    EditorSequenceComponent::~EditorSequenceComponent()
    {
        IEditor* editor = nullptr;
        EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);
        if (editor)
        {
            IAnimSequence* sequence = editor->GetMovieSystem()->FindSequenceById(m_sequenceId);
            ITrackViewSequenceManager* pSequenceManager = editor->GetSequenceManager();

            if (sequence && pSequenceManager && pSequenceManager->GetSequenceByName(sequence->GetName()))
            {
                pSequenceManager->OnDeleteSequenceObject(sequence->GetName());
            }
        }

        if (m_sequence)
        {
            m_sequence = nullptr;
            m_sequenceId = s_invalidSequenceId;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    /*static*/ void EditorSequenceComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<AnimSerialize::AnimationData>()
                ->Field("SerializedString", &AnimSerialize::AnimationData::m_serializedData)
                ->Version(1, &ClassConverters::UpVersionAnimationData);

            serializeContext->Class<EditorSequenceComponent, EditorComponentBase>()
                ->Field("AnimationData", &EditorSequenceComponent::m_serializedAnimData)
                ->Version(3);

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<EditorSequenceComponent>(
                    "Sequence", "Plays Cinematic Animations")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Cinematics")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Sequence.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Sequence.png")
                      //->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                        ->Attribute(AZ::Edit::Attributes::AddableByUser, false)     // SequenceAgents are only added by TrackView
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
        
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<EditorSequenceComponent>()->RequestBus("SequenceComponentRequestBus");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void EditorSequenceComponent::Init()
    {
        EditorComponentBase::Init();
        m_sequence = nullptr;
        m_sequenceId = s_invalidSequenceId;
        IEditor* editor = nullptr;
        EBUS_EVENT_RESULT(editor, AzToolsFramework::EditorRequests::Bus, GetEditor);

        if (editor)
        {
            // request the creation an IAnimSequence object from TrackView and set its owner to this Director
            m_sequence = editor->GetSequenceManager()->OnCreateSequenceObject(m_entity->GetName().c_str(), false);
            if (m_sequence)
            {
                m_sequence->SetOwner(GetEntityId());
                m_sequenceId = m_sequence->GetId();
            }

            // If we're deserializing the Director from disk, the m_serializedAnimData.m_serializedData will have data in it.
            // Deserialize the m_sequence with this data.
            const char* buffer = m_serializedAnimData.m_serializedData.c_str();
            size_t size = m_serializedAnimData.m_serializedData.length();
            if (size > 0)
            {
                XmlNodeRef xmlArchive = gEnv->pSystem->LoadXmlFromBuffer(buffer, size);

                XmlNodeRef sequenceNode = xmlArchive->findChild("Sequence");
                if (sequenceNode != NULL)
                {
                    // check of sequence ID collision and resolve if needed
                    XmlNodeRef idNode = xmlArchive->findChild("ID");
                    if (idNode)
                    {
                        idNode->getAttr("value", m_sequenceId);
                        if (editor->GetMovieSystem()->FindSequenceById(m_sequenceId))  // A collision found!
                        {
                            uint32 newId = editor->GetMovieSystem()->GrabNextSequenceId();
                            // TODO - need to call ar.AddSequenceIdMapping(m_sequenceId, newId);
                            m_sequenceId = newId;
                        }
                    }

                    // deserialize into m_sequence
                    m_sequence->Serialize(sequenceNode, true, true, m_sequenceId);

                    // clear the serializationData now that we've deserialized it
                    m_serializedAnimData.m_serializedData.clear();

                    // Notify Trackview of the load
                    ITrackViewSequence* pTrackViewSequence = editor->GetSequenceManager()->GetSequenceByName(m_sequence->GetName());
                    if (pTrackViewSequence)
                    {
                        pTrackViewSequence->Load();
                    }
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void EditorSequenceComponent::Activate()
    {
        EditorComponentBase::Activate();

        LmbrCentral::EditorSequenceComponentRequestBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::SequenceComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void EditorSequenceComponent::Deactivate()
    {
        LmbrCentral::EditorSequenceComponentRequestBus::Handler::BusDisconnect();
        LmbrCentral::SequenceComponentRequestBus::Handler::BusDisconnect();

        // disconnect from TickBus if we're connected (which would only happen if we deactivated during a pending property refresh)
        AZ::TickBus::Handler::BusDisconnect();

        EditorComponentBase::Deactivate();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void EditorSequenceComponent::AddEntityToAnimate(AZ::EntityId entityToAnimate)
    {
        LmbrCentral::EditorSequenceAgentComponent* agentComponent = nullptr;
        auto component = AzToolsFramework::FindComponent<EditorSequenceAgentComponent>::OnEntity(entityToAnimate);
        if (component)
        {
            agentComponent = static_cast<LmbrCentral::EditorSequenceAgentComponent*>(component);
        }
        else
        {
            // #TODO LY-21846: Use "SequenceAgentComponentService" to find component, rather than specific component-type.
            auto addComponentResult = AzToolsFramework::AddComponents<EditorSequenceAgentComponent>::ToEntities(entityToAnimate);

            if (addComponentResult.IsSuccess())
            {
                // We need to register our Entity and Component Ids with the SequenceAgentComponent so we can communicate over EBuses with it.
                // We can't do this registration over an EBus because we haven't registered with it yet - do it with pointers? Is this safe?
                agentComponent = static_cast<LmbrCentral::EditorSequenceAgentComponent*>(addComponentResult.GetValue()[entityToAnimate].m_componentsAdded[0]);
            }
        }

        AZ_Assert(agentComponent, "EditorSequenceComponent::AddEntityToAnimate unable to create or find sequenceAgentComponent.")
        // Notify the SequenceAgentComponent that we're connected to it - after this call, all communication with the Agent is over an EBus
        agentComponent->ConnectSequence(GetEntityId());
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void EditorSequenceComponent::RemoveEntityToAnimate(AZ::EntityId removedEntityId)
    {
        const LmbrCentral::SequenceAgentEventBusId ebusId(GetEntityId(), removedEntityId);

        // Notify the SequenceAgentComponent that we're disconnecting from it
        EBUS_EVENT_ID(ebusId, LmbrCentral::SequenceAgentComponentRequestBus, DisconnectSequence);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void EditorSequenceComponent::GetAllAnimatablePropertiesForComponent(IAnimNode::AnimParamInfos& properties, AZ::EntityId animatedEntityId, AZ::ComponentId componentId)
    {
        const LmbrCentral::SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);

        LmbrCentral::EditorSequenceAgentComponentRequestBus::Event(ebusId, &LmbrCentral::EditorSequenceAgentComponentRequestBus::Events::GetAllAnimatableProperties, properties, componentId);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void EditorSequenceComponent::GetAnimatableComponents(AZStd::vector<AZ::ComponentId>& componentIds, AZ::EntityId animatedEntityId)
    {
        const LmbrCentral::SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);

        EBUS_EVENT_ID(ebusId, LmbrCentral::EditorSequenceAgentComponentRequestBus, GetAnimatableComponents, componentIds);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Uuid EditorSequenceComponent::GetAnimatedAddressTypeId(const AZ::EntityId& animatedEntityId, const LmbrCentral::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress)
    {
        AZ::Uuid typeId = AZ::Uuid::CreateNull();
        const LmbrCentral::SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);

        LmbrCentral::SequenceAgentComponentRequestBus::EventResult(typeId, ebusId, &LmbrCentral::SequenceAgentComponentRequestBus::Events::GetAnimatedAddressTypeId, animatableAddress);

        return typeId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void EditorSequenceComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        SequenceComponent *gameSequenceComponent = gameEntity->CreateComponent<SequenceComponent>();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void EditorSequenceComponent::OnBeforeSave()
    {
        if (m_sequence)
        {
            // serialize to a string and store for AZ::Serialization on component save
            XmlNodeRef animationNode = GetISystem()->CreateXmlNode("SequenceAnimation");
            XmlNodeRef sequenceNode = animationNode->newChild("Sequence");
            m_sequence->Serialize(sequenceNode, false);

            // The id should be saved here again so that it can be restored
            // early enough. This is required for a proper saving of FG nodes like
            // the 'TrackEvent' node with the new id-based sequence identification.
            XmlNodeRef idNode = animationNode->newChild("ID");
            idNode->setAttr("value", m_sequence->GetId());

            m_serializedAnimData.m_serializedData = animationNode->getXML();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    EAnimValue EditorSequenceComponent::GetValueType(const AZStd::string& animatableAddress)
    {
        // TODO: look up type from BehaviorContext Property
        return eAnimValue_Float;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    bool EditorSequenceComponent::SetAnimatedPropertyValue(const AZ::EntityId& animatedEntityId, const LmbrCentral::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress, const AnimatedValue& value)
    {
        const LmbrCentral::SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);
        bool changed = false;
        bool animatedEntityIsSelected = false;

        // put this component on the TickBus to refresh propertyGrids if it is Selected (and hence it's values will be shown in the EntityInspector)
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(animatedEntityIsSelected, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsSelected, animatedEntityId);
        if (animatedEntityIsSelected && !AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }

        EBUS_EVENT_ID_RESULT(changed, ebusId, LmbrCentral::SequenceAgentComponentRequestBus, SetAnimatedPropertyValue, animatableAddress, value);

        return changed;
    }

    void EditorSequenceComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        // refresh the property displays at a lower refresh rate
        if ((time.GetMilliseconds() - s_lastPropertyRefreshTime.GetMilliseconds()) > s_refreshPeriodMilliseconds)
        {
            s_lastPropertyRefreshTime = time;

            // refresh
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::Bus::Events::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);

            // disconnect from tick bus now that we've refreshed
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    void EditorSequenceComponent::GetAnimatedPropertyValue(AnimatedValue& returnValue, const AZ::EntityId& animatedEntityId, const LmbrCentral::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress)
    {
        const LmbrCentral::SequenceAgentEventBusId ebusId(GetEntityId(), animatedEntityId);
        float retVal = .0f;

        EBUS_EVENT_ID(ebusId, LmbrCentral::SequenceAgentComponentRequestBus, GetAnimatedPropertyValue, returnValue, animatableAddress);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    bool EditorSequenceComponent::MarkEntityLayerAsDirty() const
    {
        bool retSuccess = false;
        AZ::Entity* entity = nullptr;

        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, GetEntityId());
        if (entity)
        {
            IBaseObject* entityObject = nullptr;

            EBUS_EVENT_ID_RESULT(entityObject, GetEntityId(), AzToolsFramework::ComponentEntityEditorRequestBus, GetSandboxObject);
            if (entityObject)
            {
                entityObject->SetLayerModified();
                retSuccess = true;
            }
        }
        return retSuccess;
    }

    //=========================================================================
    namespace ClassConverters
    {
        // recursively traverses XML tree rooted at node converting transform nodes. Returns true if any node was converted.
        static bool convertTransformXMLNodes(XmlNodeRef node)
        {
            bool nodeConverted = false;

            // recurse through children
            for (int i = node->getChildCount(); --i >= 0;)
            {
                if (convertTransformXMLNodes(node->getChild(i)))
                {
                    nodeConverted = true;
                }
            }

            XmlString nodeType;
            if (node->isTag("Node") && node->getAttr("Type", nodeType) && nodeType == "Component")
            {
                XmlString componentTypeId;
                if (node->getAttr("ComponentTypeId", componentTypeId) && componentTypeId == "{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0}")   // Type Uuid ToolsTransformComponentTypeId
                {
                    static const char* paramTypeName = "paramType";
                    static const char* paramUserValueName = "paramUserValue";
                    static const char* virtualPropertyName = "virtualPropertyName";

                    // go through child nodes. Convert previous Position, Rotation or Scale tracks ByString to enumerated param types
                    for (const XmlNodeRef& childNode : node)
                    {
                        XmlString paramType;
                        if (childNode->isTag("Track") && childNode->getAttr(paramTypeName, paramType) && paramType == "ByString")
                        {
                            XmlString paramUserValue;
                            if (childNode->getAttr(paramUserValueName, paramUserValue) && paramUserValue == "Position")
                            {
                                childNode->setAttr(paramTypeName, "Position");
                                childNode->setAttr(virtualPropertyName, "Position");
                                childNode->delAttr(paramUserValueName);
                                nodeConverted = true;
                            }
                            else if (childNode->getAttr(paramUserValueName, paramUserValue) && paramUserValue == "Rotation")
                            {
                                childNode->setAttr(paramTypeName, "Rotation");
                                childNode->setAttr(virtualPropertyName, "Rotation");
                                childNode->delAttr(paramUserValueName);
                                nodeConverted = true;
                            }
                            else if (childNode->getAttr(paramUserValueName, paramUserValue) && paramUserValue == "Scale")
                            {
                                childNode->setAttr(paramTypeName, "Scale");
                                childNode->setAttr(virtualPropertyName, "Scale");
                                childNode->delAttr(paramUserValueName);
                                nodeConverted = true;
                            }
                        }
                    }
                }
            }

            return nodeConverted;
        }

        static bool UpVersionAnimationData(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() == 0)
            {

                // upgrade V0 to V1 - change "Position", "Rotation", "Scale" anim params in Transform Component Nodes from eAnimParamType_ByString to
                // eAnimParamType_Position, eAnimParamType_Rotation, eAnimParamType_Scale respectively
                int serializedAnimStringIdx = classElement.FindElement(AZ::Crc32("SerializedString"));
                if (serializedAnimStringIdx == -1)
                {
                    AZ_Error("Serialization", false, "Failed to find 'SerializedString' element.");
                    return false;
                }

                AZStd::string serializedAnimString;
                classElement.GetSubElement(serializedAnimStringIdx).GetData(serializedAnimString);

                const char* buffer = serializedAnimString.c_str();
                size_t size = serializedAnimString.length();
                if (size > 0)
                {
                    XmlNodeRef xmlArchive = gEnv->pSystem->LoadXmlFromBuffer(buffer, size);

                    // recursively traverse and convert through all nodes
                    if (convertTransformXMLNodes(xmlArchive))
                    {
                        // if a node was converted, replace the classElement Data with the converted XML
                        serializedAnimString = xmlArchive->getXML();
                        classElement.GetSubElement(serializedAnimStringIdx).SetData(context, serializedAnimString);
                    }
                }
            }

            return true;
        }
    } // namespace ClassConverters
} // namespace LmbrCentral