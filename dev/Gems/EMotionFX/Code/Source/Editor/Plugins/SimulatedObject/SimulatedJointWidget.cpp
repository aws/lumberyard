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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SimulatedObjectCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/ObjectEditor.h>
#include <Editor/Plugins/SimulatedObject/SimulatedJointWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectColliderWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>
#include <Editor/SimulatedObjectHelpers.h>
#include <Editor/SkeletonModel.h>
#include <MCore/Source/StringConversions.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>


namespace EMotionFX
{
    class SimulatedObjectPropertyNotify
        : public AzToolsFramework::IPropertyEditorNotify
    {
    public:
        // this function gets called each time you are about to actually modify
        // a property (not when the editor opens)
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;

        // this function gets called each time a property is actually modified
        // (not just when the editor appears), for each and every change - so
        // for example, as a slider moves.  its meant for undo state capture.
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*pNode*/) override {}

        // this funciton is called when some stateful operation begins, such as
        // dragging starts in the world editor or such in which case you don't
        // want to blow away the tree and rebuild it until editing is complete
        // since doing so is flickery and intensive.
        void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*pNode*/) override {}
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;

        // this will cause the current undo operation to complete, sealing it
        // and beginning a new one if there are further edits.
        void SealUndoStack() override {}
    private:
        MCore::CommandGroup m_commandGroup;
    };

    void SimulatedObjectPropertyNotify::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        if (!m_commandGroup.IsEmpty())
        {
            return;
        }

        const AzToolsFramework::InstanceDataNode* parent = pNode->GetParent();
        if (parent && parent->GetSerializeContext()->CanDowncast(parent->GetClassMetadata()->m_typeId, azrtti_typeid<EMotionFX::SimulatedObject>(), parent->GetClassMetadata()->m_azRtti, nullptr))
        {
            const size_t instanceCount = pNode->GetNumInstances();
            m_commandGroup.SetGroupName(AZStd::string::format("Adjust simulated object%s", instanceCount > 1 ? "s" : ""));
            for (size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
            {
                const SimulatedObject* simulatedObject = static_cast<SimulatedObject*>(parent->GetInstance(instanceIndex));
                const SimulatedObjectSetup* simulatedObjectSetup = simulatedObject->GetSimulatedObjectSetup();
                const AZ::u32 actorId = simulatedObjectSetup->GetActor()->GetID();
                const size_t objectIndex = simulatedObjectSetup->FindSimulatedObjectIndex(simulatedObject).GetValue();

                CommandAdjustSimulatedObject* command = aznew CommandAdjustSimulatedObject(actorId, objectIndex);
                m_commandGroup.AddCommand(command);

                const void* instance = pNode->GetInstance(instanceIndex);

                const AZ::SerializeContext::ClassElement* elementData = pNode->GetElementMetadata();
                if (elementData->m_nameCrc == AZ_CRC("objectName", 0xd403db79))
                {
                    command->SetOldObjectName(*static_cast<const AZStd::string*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("gravityFactor", 0x23584906))
                {
                    command->SetOldGravityFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("stiffnessFactor", 0xbf262b07))
                {
                    command->SetOldStiffnessFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("dampingFactor", 0x388a3234))
                {
                    command->SetOldDampingFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("colliderTags", 0xb337393d))
                {
                    command->SetOldColliderTags(*static_cast<const AZStd::vector<AZStd::string>*>(instance));
                }
            }
        }
        else if (parent && parent->GetSerializeContext()->CanDowncast(parent->GetClassMetadata()->m_typeId, azrtti_typeid<EMotionFX::SimulatedJoint>(), parent->GetClassMetadata()->m_azRtti, nullptr))
        {
            const size_t instanceCount = pNode->GetNumInstances();
            m_commandGroup.SetGroupName(AZStd::string::format("Adjust simulated joint%s", instanceCount > 1 ? "s" : ""));
            for (size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
            {
                const SimulatedJoint* simulatedJoint = static_cast<SimulatedJoint*>(parent->GetInstance(instanceIndex));
                const SimulatedObject* simulatedObject = simulatedJoint->GetSimulatedObject();
                const SimulatedObjectSetup* simulatedObjectSetup = simulatedObject->GetSimulatedObjectSetup();
                const AZ::u32 actorId = simulatedObjectSetup->GetActor()->GetID();
                const size_t objectIndex = simulatedObjectSetup->FindSimulatedObjectIndex(simulatedObject).GetValue();
                const size_t jointIndex = simulatedJoint->CalculateSimulatedJointIndex().GetValue();

                CommandAdjustSimulatedJoint* command = aznew CommandAdjustSimulatedJoint(actorId, objectIndex, jointIndex);
                m_commandGroup.AddCommand(command);

                const void* instance = pNode->GetInstance(instanceIndex);

                const AZ::SerializeContext::ClassElement* elementData = pNode->GetElementMetadata();
                if (elementData->m_nameCrc == AZ_CRC("coneAngleLimit", 0x355562ed))
                {
                    command->SetOldConeAngleLimit(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("mass", 0x6c035b66))
                {
                    command->SetOldMass(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("stiffness", 0x89379000))
                {
                    command->SetOldStiffness(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("damping", 0x440e3a6a))
                {
                    command->SetOldDamping(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("gravityFactor", 0x23584906))
                {
                    command->SetOldGravityFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("friction", 0x120c180b))
                {
                    command->SetOldFriction(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("pinned", 0xe527e5e7))
                {
                    command->SetOldPinned(*static_cast<const bool*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("colliderExclusionTags", 0xdbaea6e9))
                {
                    command->SetOldColliderExclusionTags(*static_cast<const AZStd::vector<AZStd::string>*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("autoExcludeMode", 0x8e8f8066))
                {
                    command->SetOldAutoExcludeMode(*static_cast<const SimulatedJoint::AutoExcludeMode*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("autoExcludeGeometric", 0x1aa4f9b6))
                {
                    command->SetOldGeometricAutoExclusion(*static_cast<const bool*>(instance));
                }
            }
        }
    }

    void SimulatedObjectPropertyNotify::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
    {
        const AzToolsFramework::InstanceDataNode* parent = pNode->GetParent();
        if (!m_commandGroup.IsEmpty() && parent && parent->GetSerializeContext()->CanDowncast(parent->GetClassMetadata()->m_typeId, azrtti_typeid<EMotionFX::SimulatedObject>(), parent->GetClassMetadata()->m_azRtti, nullptr))
        {
            const size_t instanceCount = pNode->GetNumInstances();
            for (size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
            {
                CommandAdjustSimulatedObject* command = static_cast<CommandAdjustSimulatedObject*>(m_commandGroup.GetCommand(instanceIndex));

                const void* instance = pNode->GetInstance(instanceIndex);

                const AZ::SerializeContext::ClassElement* elementData = pNode->GetElementMetadata();
                if (elementData->m_nameCrc == AZ_CRC("objectName", 0xd403db79))
                {
                    command->SetObjectName(*static_cast<const AZStd::string*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("gravityFactor", 0x23584906))
                {
                    command->SetGravityFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("stiffnessFactor", 0xbf262b07))
                {
                    command->SetStiffnessFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("dampingFactor", 0x388a3234))
                {
                    command->SetDampingFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("colliderTags", 0xb337393d))
                {
                    AZStd::string commandString;

                    const EMotionFX::SimulatedObject* simulatedObject = static_cast<const EMotionFX::SimulatedObject*>(parent->GetInstance(instanceIndex));
                    const AZStd::vector<AZStd::string>& colliderTags = simulatedObject->GetColliderTags();

                    AZStd::vector<AZStd::string> colliderExclusionTags;
                    const AZStd::vector<SimulatedJoint*>& simulatedJoints = simulatedObject->GetSimulatedJoints();
                    for (const SimulatedJoint* simulatedJoint : simulatedJoints)
                    {
                        // Copy the current exclusion tags to a temporary buffer.
                        colliderExclusionTags = simulatedJoint->GetColliderExclusionTags();

                        // Remove all tags that are no longer part of the collider tags of the simulated object.
                        bool changed = false;
                        colliderExclusionTags.erase(AZStd::remove_if(colliderExclusionTags.begin(), colliderExclusionTags.end(),
                            [&colliderTags, &changed](const AZStd::string& tag)->bool
                            {
                                if (AZStd::find(colliderTags.begin(), colliderTags.end(), tag) == colliderTags.end())
                                {
                                    // The exclusion tag is not part of the collider tags in the simulated object.
                                    // Remove the tag from the exclusion tags.
                                    changed = true;
                                    return true;
                                }

                                return false;
                            }),
                            colliderExclusionTags.end());

                        if (changed)
                        {
                            const SimulatedObjectSetup* simulatedObjectSetup = simulatedObject->GetSimulatedObjectSetup();
                            const AZ::u32 actorId = simulatedObjectSetup->GetActor()->GetID();
                            const size_t objectIndex = simulatedObjectSetup->FindSimulatedObjectIndex(simulatedObject).GetValue();
                            const size_t jointIndex = simulatedJoint->CalculateSimulatedJointIndex().GetValue();
                            const AZStd::string colliderExclusionTagString = MCore::ConstructStringSeparatedBySemicolons(colliderExclusionTags);

                            commandString = AZStd::string::format("%s -%s %d -%s %d -%s %d -%s \"%s\"",
                                CommandAdjustSimulatedJoint::s_commandName,
                                CommandAdjustSimulatedJoint::s_actorIdParameterName, actorId,
                                CommandAdjustSimulatedJoint::s_objectIndexParameterName, objectIndex,
                                CommandAdjustSimulatedJoint::s_jointIndexParameterName, jointIndex,
                                CommandAdjustSimulatedJoint::s_colliderExclusionTagsParameterName, colliderExclusionTagString.c_str());
                            m_commandGroup.AddCommandString(commandString);
                        }
                    }

                    command->SetColliderTags(*static_cast<const AZStd::vector<AZStd::string>*>(instance));
                }
            }
        }
        else if (!m_commandGroup.IsEmpty() && parent && parent->GetSerializeContext()->CanDowncast(parent->GetClassMetadata()->m_typeId, azrtti_typeid<EMotionFX::SimulatedJoint>(), parent->GetClassMetadata()->m_azRtti, nullptr))
        {
            const size_t instanceCount = pNode->GetNumInstances();
            for (size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
            {
                CommandAdjustSimulatedJoint* command = static_cast<CommandAdjustSimulatedJoint*>(m_commandGroup.GetCommand(instanceIndex));

                const void* instance = pNode->GetInstance(instanceIndex);

                const AZ::SerializeContext::ClassElement* elementData = pNode->GetElementMetadata();
                if (elementData->m_nameCrc == AZ_CRC("coneAngleLimit", 0x355562ed))
                {
                    command->SetConeAngleLimit(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("mass", 0x6c035b66))
                {
                    command->SetMass(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("stiffness", 0x89379000))
                {
                    command->SetStiffness(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("damping", 0x440e3a6a))
                {
                    command->SetDamping(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("gravityFactor", 0x23584906))
                {
                    command->SetGravityFactor(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("friction", 0x120c180b))
                {
                    command->SetFriction(*static_cast<const float*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("pinned", 0xe527e5e7))
                {
                    command->SetPinned(*static_cast<const bool*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("colliderExclusionTags", 0xdbaea6e9))
                {
                    command->SetColliderExclusionTags(*static_cast<const AZStd::vector<AZStd::string>*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("autoExcludeMode", 0x8e8f8066))
                {
                    command->SetAutoExcludeMode(*static_cast<const SimulatedJoint::AutoExcludeMode*>(instance));
                }
                else if (elementData->m_nameCrc == AZ_CRC("autoExcludeGeometric", 0x1aa4f9b6))
                {
                    command->SetGeometricAutoExclusion(*static_cast<const bool*>(instance));
                }
            }
        }

        AZStd::string result;
        CommandSystem::GetCommandManager()->ExecuteCommandGroup(m_commandGroup, result);
        m_commandGroup.Clear();
    }

    ///////////////////////////////////////////////////////////////////////////

    const int SimulatedJointWidget::s_jointLabelSpacing = 17;
    const int SimulatedJointWidget::s_jointNameSpacing = 90;

    SimulatedJointWidget::SimulatedJointWidget(SimulatedObjectWidget* plugin, QWidget* parent)
        : QScrollArea(parent)
        , m_plugin(plugin)
        , m_noSelectionWidget(new QWidget(this))
        , m_contentsWidget(new QWidget(this))
        , m_removeButton(new QPushButton("Remove from simulated object", this))
        , m_simulatedObjectEditor(nullptr)
        , m_simulatedJointEditor(nullptr)
        , m_simulatedObjectEditorCard(new AzQtComponents::Card(this))
        , m_simulatedJointEditorCard(new AzQtComponents::Card(this))
        , m_propertyNotify(AZStd::make_unique<SimulatedObjectPropertyNotify>())
    {
        connect(m_removeButton, &QPushButton::clicked, this, &SimulatedJointWidget::RemoveSelectedSimulatedJoint);

        QLabel* noSelectionLabel = new QLabel("Select joints from the Skeleton Outliner and add it to the simulated object using the right-click menu");
        noSelectionLabel->setWordWrap(true);

        QVBoxLayout* noSelectionLayout = new QVBoxLayout(m_noSelectionWidget);
        noSelectionLayout->addWidget(noSelectionLabel, Qt::AlignTop);

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.")
        m_simulatedObjectEditor = new ObjectEditor(serializeContext, m_propertyNotify.get());
        m_simulatedJointEditor = new ObjectEditor(serializeContext, m_propertyNotify.get());

        QWidget* jointCardContents = new QWidget(this);
        QVBoxLayout* jointCardLayout = new QVBoxLayout(jointCardContents);
        jointCardLayout->addWidget(m_simulatedJointEditor);
        jointCardLayout->addWidget(m_removeButton);

        m_simulatedObjectEditorCard->setContentWidget(m_simulatedObjectEditor);
        m_simulatedJointEditorCard->setContentWidget(jointCardContents);

        m_colliderWidget = new QWidget();
        QVBoxLayout* colliderWidgetLayout = new QVBoxLayout(m_colliderWidget);

        if (ColliderHelpers::AreCollidersReflected())
        {
            SimulatedObjectColliderWidget* colliderWidget = new SimulatedObjectColliderWidget();
            colliderWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            colliderWidget->CreateGUI();

            colliderWidgetLayout->addWidget(colliderWidget);
        }
        else
        {
            QLabel* noColliders = new QLabel(
                "To adjust the properties of the Simulated Object Colliders, "
                "enable the PhysX gem via the Project Configurator");

            colliderWidgetLayout->addWidget(noColliders);
        }

        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (skeletonModel)
        {
            connect(&skeletonModel->GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, [this]()
            {
                // Show the collider widget as the joint in the skeleton
                // model was the last thing selected
                m_contentsWidget->hide();
                m_noSelectionWidget->hide();
                m_colliderWidget->show();
            });
        }

        // Contents widget
        m_contentsWidget->setVisible(false);
        QVBoxLayout* contentsLayout = new QVBoxLayout(m_contentsWidget);
        contentsLayout->setSpacing(ColliderContainerWidget::s_layoutSpacing);
        contentsLayout->addWidget(m_simulatedObjectEditorCard);
        contentsLayout->addWidget(m_simulatedJointEditorCard);

        QWidget* scrolledWidget = new QWidget();
        QVBoxLayout* mainLayout = new QVBoxLayout(scrolledWidget);
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->setMargin(0);
        mainLayout->addWidget(m_contentsWidget);
        mainLayout->addWidget(m_noSelectionWidget);
        mainLayout->addWidget(m_colliderWidget);

        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        setWidget(scrolledWidget);
        setWidgetResizable(true);

        SimulatedObjectModel* model = m_plugin->GetSimulatedObjectModel();
        connect(model->GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &SimulatedJointWidget::UpdateDetailsView);
        connect(model, &QAbstractItemModel::dataChanged, m_simulatedObjectEditor, &ObjectEditor::InvalidateValues);
        connect(model, &QAbstractItemModel::dataChanged, m_simulatedJointEditor, &ObjectEditor::InvalidateValues);
    }

    SimulatedJointWidget::~SimulatedJointWidget() = default;

    void SimulatedJointWidget::UpdateDetailsView(const QItemSelection& selected, const QItemSelection& deselected)
    {
        AZ_UNUSED(selected)
        AZ_UNUSED(deselected)

        const SimulatedObjectModel* model = m_plugin->GetSimulatedObjectModel();
        const QItemSelectionModel* selectionModel = model->GetSelectionModel();
        const QModelIndexList& selectedIndexes = selectionModel->selectedIndexes();

        if (selectedIndexes.empty())
        {
            m_noSelectionWidget->setVisible(true);
            m_contentsWidget->setVisible(false);
            m_simulatedObjectEditor->ClearInstances(true);
            m_simulatedJointEditor->ClearInstances(true);
            return;
        }

        m_simulatedObjectEditor->ClearInstances(false);
        m_simulatedJointEditor->ClearInstances(false);

        QString jointName;
        QString objectName;

        AZStd::unordered_map<AZ::Uuid, AZStd::vector<void*>> typeIdToAggregateInstance;
        for (const QModelIndex& modelIndex : selectedIndexes)
        {
            if (modelIndex.column() != 0)
            {
                continue;
            }
            void* object = modelIndex.data(SimulatedObjectModel::ROLE_JOINT_PTR).value<SimulatedJoint*>();
            AZ::Uuid typeId = azrtti_typeid<SimulatedJoint>();
            ObjectEditor* objectEditor = m_simulatedJointEditor;

            if (!object)
            {
                object = modelIndex.data(SimulatedObjectModel::ROLE_OBJECT_PTR).value<SimulatedObject*>();
                typeId = azrtti_typeid<SimulatedObject>();
                objectEditor = m_simulatedObjectEditor;
                if (objectName.isNull())
                {
                    objectName = modelIndex.data().toString();
                }
            }
            else if (jointName.isNull())
            {
                jointName = modelIndex.data().toString();
            }

            if (object)
            {
                auto foundAggregate = typeIdToAggregateInstance.find(typeId);
                if (foundAggregate != typeIdToAggregateInstance.end())
                {
                    objectEditor->AddInstance(object, typeId, foundAggregate->second[0]);
                    foundAggregate->second.emplace_back(object);
                }
                else
                {
                    objectEditor->AddInstance(object, typeId);
                    typeIdToAggregateInstance.emplace(typeId, AZStd::vector<void*> {object});
                }
            }
        }

        auto selectedSimulatedObjects = typeIdToAggregateInstance.find(azrtti_typeid<SimulatedObject>());
        if (selectedSimulatedObjects != typeIdToAggregateInstance.end())
        {
            m_simulatedObjectEditorCard->show();
            const size_t numSelectedObjects = selectedSimulatedObjects->second.size();
            if (numSelectedObjects == 1)
            {
                m_simulatedObjectEditorCard->setTitle(objectName);
            }
            else
            {
                m_simulatedObjectEditorCard->setTitle(QString("%1 Simulated Object%2").arg(numSelectedObjects).arg(numSelectedObjects > 1 ? "s" : ""));
            }
        }
        else
        {
            m_simulatedObjectEditorCard->hide();
        }

        auto selectedSimulatedJoints = typeIdToAggregateInstance.find(azrtti_typeid<SimulatedJoint>());
        if (selectedSimulatedJoints != typeIdToAggregateInstance.end())
        {
            m_simulatedJointEditorCard->show();
            const size_t numSelectedJoints = selectedSimulatedJoints->second.size();
            if (numSelectedJoints == 1)
            {
                m_simulatedJointEditorCard->setTitle(jointName);
            }
            else
            {
                m_simulatedJointEditorCard->setTitle(QString("%1 Simulated Joint%2").arg(numSelectedJoints).arg(numSelectedJoints > 1 ? "s" : ""));
            }
        }
        else
        {
            m_simulatedJointEditorCard->hide();
        }

        m_noSelectionWidget->setVisible(selectedIndexes.empty());
        m_contentsWidget->setVisible(!selectedIndexes.empty());

        // Hide the collider widget as the joint in the Simulated Object widget
        // was the last thing selected
        m_colliderWidget->hide();
    }

    void SimulatedJointWidget::RemoveSelectedSimulatedJoint() const
    {
        const SimulatedObjectModel* model = m_plugin->GetSimulatedObjectModel();
        SimulatedObjectHelpers::RemoveSimulatedJoints(model->GetSelectionModel()->selectedRows(0), false);
    }
} // namespace EMotionFX
