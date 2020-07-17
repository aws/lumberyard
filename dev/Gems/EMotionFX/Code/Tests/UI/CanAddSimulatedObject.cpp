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

#include <gtest/gtest.h>

#include <QPushButton>
#include <QAction>
#include <QtTest>

#include <Tests/UI/UIFixture.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/AutoRegisteredActor.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>
#include <Editor/InputDialogValidatable.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>

#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/PhysicsSetupUtils.h>

namespace EMotionFX
{
    class CanAddSimulatedObjectFixture
        : public UIFixture
    {
    public:
        void TearDown() override
        {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            UIFixture::TearDown();
        }

        void RecursiveGetAllChildren(QTreeView* treeView, const QModelIndex& index, QModelIndexList& outIndicies)
        {
            outIndicies.push_back(index);
            for (int i = 0; i < treeView->model()->rowCount(index); ++i)
            {
                RecursiveGetAllChildren(treeView, index.child(i, 0), outIndicies);
            }
        }

        void CreateSimulateObject(const char* objectName)
        {
            // Select the newly created actor
            {
                AZStd::string result;
                EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("Select -actorID 0", result)) << result.c_str();
            }

            // Change the Editor mode to Simulated Objects
            EMStudio::GetMainWindow()->ApplicationModeChanged("SimulatedObjects");

            // Find the Simulated Object Manager and its button
            m_simulatedObjectWidget = static_cast<EMotionFX::SimulatedObjectWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SimulatedObjectWidget::CLASS_ID));
            ASSERT_TRUE(m_simulatedObjectWidget) << "Simulated Object plugin not found!";

            QPushButton* addSimulatedObjectButton = m_simulatedObjectWidget->GetDockWidget()->findChild<QPushButton*>("addSimulatedObjectButton");

            // Send the left button click directly to the button
            QTest::mouseClick(addSimulatedObjectButton, Qt::LeftButton);

            // In the Input Dialog set the name of the object and close the dialog
            EMStudio::InputDialogValidatable* inputDialog = qobject_cast<EMStudio::InputDialogValidatable*>(FindTopLevelWidget("newSimulatedObjectDialog"));
            ASSERT_NE(inputDialog, nullptr) << "Cannot find input dialog.";

            inputDialog->SetText(objectName);
            inputDialog->accept();

            // There is one and only one simulated objects
            ASSERT_EQ(m_actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), 1);
            // Check it has the correct name
            EXPECT_STREQ(m_actor->GetSimulatedObjectSetup()->GetSimulatedObject(0)->GetName().c_str(), objectName);

        }

    protected:
        AutoRegisteredActor m_actor;
        EMotionFX::SimulatedObjectWidget* m_simulatedObjectWidget = nullptr;
    };

    TEST_F(CanAddSimulatedObjectFixture, CanAddSimulatedObject)
    {
        RecordProperty("test_case_id", "C13048820");

        m_actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(1, "CanAddSimulatedObjectActor");

        CreateSimulateObject("New simulated object");
    }

    TEST_F(UIFixture, CanAddSimulatedObjectWithJoints)
    {
        RecordProperty("test_case_id", "C13048818");

        AutoRegisteredActor actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(2, "CanAddSimulatedObjectWithJointsActor");

        ActorInstance* actorInstance = ActorInstance::Create(actor.get());

        EMStudio::GetMainWindow()->ApplicationModeChanged("SimulatedObjects");

        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(AZStd::string{"Select -actorInstanceID "} + AZStd::to_string(actorInstance->GetID()), result)) << result.c_str();
        }

        auto simulatedObjectWidget = static_cast<EMotionFX::SimulatedObjectWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SimulatedObjectWidget::CLASS_ID));
        ASSERT_TRUE(simulatedObjectWidget) << "Simulated Object plugin not found!";

        auto skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        QTreeView* treeView = skeletonOutliner->GetDockWidget()->findChild<QTreeView*>("skeletonOutlinerTreeView");
        const QAbstractItemModel* model = treeView->model();

        const QModelIndex rootJointIndex = model->index(0, 0);
        ASSERT_TRUE(rootJointIndex.isValid()) << "Unable to find a model index for the root joint of the actor";

        treeView->selectionModel()->select(rootJointIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);

        treeView->scrollTo(rootJointIndex);
        BringUpContextMenu(treeView, treeView->visualRect(rootJointIndex));

        QMenu* contextMenu = skeletonOutliner->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        EXPECT_TRUE(contextMenu);
        QAction* simulatedObjectAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(simulatedObjectAction, contextMenu, "Simulated object"));
        QMenu* simulatedObjectMenu = simulatedObjectAction->menu();
        EXPECT_TRUE(simulatedObjectMenu);
        QAction* addSelectedJointAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(addSelectedJointAction, simulatedObjectMenu, "Add selected joint"));
        QMenu* addSelectedJointMenu = addSelectedJointAction->menu();
        EXPECT_TRUE(addSelectedJointMenu);
        QAction* newSimulatedObjectAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(newSimulatedObjectAction, addSelectedJointMenu, "<New simulated object>"));
        newSimulatedObjectAction->trigger();

        EMStudio::InputDialogValidatable* inputDialog = qobject_cast<EMStudio::InputDialogValidatable*>(FindTopLevelWidget("newSimulatedObjectDialog"));
        ASSERT_NE(inputDialog, nullptr) << "Cannot find input dialog.";

        inputDialog->SetText("New simulated object");
        inputDialog->accept();

        ASSERT_EQ(actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), 1);
        const auto simulatedObject = actor->GetSimulatedObjectSetup()->GetSimulatedObject(0);
        EXPECT_STREQ(simulatedObject->GetName().c_str(), "New simulated object");
        EXPECT_EQ(simulatedObject->GetNumSimulatedRootJoints(), 1);
        EXPECT_EQ(simulatedObject->GetNumSimulatedJoints(), 1);
        EXPECT_STREQ(actor->GetSkeleton()->GetNode(simulatedObject->GetSimulatedJoint(0)->GetSkeletonJointIndex())->GetName(), "rootJoint");
        EXPECT_STREQ(actor->GetSkeleton()->GetNode(simulatedObject->GetSimulatedRootJoint(0)->GetSkeletonJointIndex())->GetName(), "rootJoint");

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        actorInstance->Destroy();
    }

    TEST_F(CanAddSimulatedObjectFixture, CanAddSimulatedObjectWithJointsAndName)
    {
        RecordProperty("test_case_id", "C13048820a");

        m_actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(5, "CanAddSimulatedObjectActor");

        CreateSimulateObject("sim1");

        // Get the Skeleton Outliner and find the model relating to its treeview
        auto skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        QTreeView* treeView = skeletonOutliner->GetDockWidget()->findChild<QTreeView*>("skeletonOutlinerTreeView");
        const QAbstractItemModel* model = treeView->model();

        // Find the 3rd joint in the TreeView and select it
        const QModelIndex jointIndex = model->index(0, 3);
        ASSERT_TRUE(jointIndex.isValid()) << "Unable to find a model index for the root joint of the actor";

        treeView->selectionModel()->select(jointIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);

        treeView->scrollTo(jointIndex);

        // Open the Right Click Context Menu
        const QRect rect = treeView->visualRect(jointIndex);
        EXPECT_TRUE(rect.isValid());
        BringUpContextMenu(treeView, rect);

        // Trace down the sub Menus to <New simulated object> and select it
        QMenu* contextMenu = skeletonOutliner->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        EXPECT_TRUE(contextMenu);
        QAction* simulatedObjectAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(simulatedObjectAction, contextMenu, "Simulated object"));
        QMenu* simulatedObjectMenu = simulatedObjectAction->menu();
        EXPECT_TRUE(simulatedObjectMenu);
        QAction* addSelectedJointAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(addSelectedJointAction, simulatedObjectMenu, "Add selected joint"));
        QMenu* addSelectedJointMenu = addSelectedJointAction->menu();
        EXPECT_TRUE(addSelectedJointMenu);
        QAction* newSimulatedObjectAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(newSimulatedObjectAction, addSelectedJointMenu, "<New simulated object>"));
        newSimulatedObjectAction->trigger();
        // Set the name in the Dialog Box and test it.
        EMStudio::InputDialogValidatable* inputDialog = qobject_cast<EMStudio::InputDialogValidatable*>(FindTopLevelWidget("newSimulatedObjectDialog"));
        ASSERT_NE(inputDialog, nullptr) << "Cannot find input dialog.";

        inputDialog->SetText("sim2");
        inputDialog->accept();

        ASSERT_EQ(m_actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), 2);
        const auto simulatedObject = m_actor->GetSimulatedObjectSetup()->GetSimulatedObject(1);
        EXPECT_STREQ(simulatedObject->GetName().c_str(), "sim2");
    }

    TEST_F(CanAddSimulatedObjectFixture, CanAddColliderToSimulatedObject)
    {
        RecordProperty("test_case_id", "C13048816");

        m_actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(5, "CanAddSimulatedObjectActor");

        CreateSimulateObject("sim1");

        // Get the Skeleton Outliner and find the model relating to its treeview
        auto skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        QTreeView* treeView = skeletonOutliner->GetDockWidget()->findChild<QTreeView*>("skeletonOutlinerTreeView");
        const QAbstractItemModel* model = treeView->model();

        // Find the 3rd joint in the TreeView and select it
        const QModelIndex jointIndex = model->index(0, 3);
        ASSERT_TRUE(jointIndex.isValid()) << "Unable to find a model index for the root joint of the actor";

        treeView->selectionModel()->select(jointIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);

        treeView->scrollTo(jointIndex);

        // Open the Right Click Context Menu
        const QRect rect = treeView->visualRect(jointIndex);
        EXPECT_TRUE(rect.isValid());
        BringUpContextMenu(treeView, rect);

        // Trace down the sub Menus to Add Collider and select it
        QMenu* contextMenu = skeletonOutliner->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        EXPECT_TRUE(contextMenu);
        QAction* simulatedObjectColliderAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(simulatedObjectColliderAction, contextMenu, "Simulated object collider"));
        QMenu* simulatedObjectColliderMenu = simulatedObjectColliderAction->menu();
        EXPECT_TRUE(simulatedObjectColliderMenu);
        QAction* addSelectedAddColliderAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(addSelectedAddColliderAction, simulatedObjectColliderMenu, "Add collider"));
        QMenu* addSelectedColliderMenu = addSelectedAddColliderAction->menu();
        QAction* addCapsuleColliderAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(addCapsuleColliderAction, addSelectedColliderMenu, "Add capsule"));

        size_t numCapsuleColliders = PhysicsSetupUtils::CountColliders(m_actor.get(), PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Capsule);
        EXPECT_EQ(numCapsuleColliders, 0);

        addCapsuleColliderAction->trigger();

        // Check that a collider has been added.
        size_t numCollidersAfterAddCapsule = PhysicsSetupUtils::CountColliders(m_actor.get(), PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Capsule);
        ASSERT_EQ(numCollidersAfterAddCapsule, numCapsuleColliders + 1) << "Capsule collider not added.";

        QAction* addSphereColliderAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(addSphereColliderAction, addSelectedColliderMenu, "Add sphere"));

        const size_t numSphereColliders = PhysicsSetupUtils::CountColliders(m_actor.get(), PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Sphere);
        EXPECT_EQ(numSphereColliders, 0);

        addSphereColliderAction->trigger();

        // Check that a second collider has been added.
        const size_t numCollidersAfterAddSphere = PhysicsSetupUtils::CountColliders(m_actor.get(), PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Sphere);
        ASSERT_EQ(numCollidersAfterAddSphere, numSphereColliders + 1) << "Sphere collider not added.";
    }

    TEST_F(CanAddSimulatedObjectFixture, CanRemoveSimulatedObject)
    {
        RecordProperty("test_case_id", "C13048821");

        m_actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(1, "CanRemoveSimulatedObjectActor");

        CreateSimulateObject("TestObject1");

        // Get the Simulated Object and find the model relating to its treeview
        EMotionFX::SimulatedObjectWidget* simulatedObjectWidget = static_cast<EMotionFX::SimulatedObjectWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SimulatedObjectWidget::CLASS_ID));
        ASSERT_TRUE(simulatedObjectWidget);
        QTreeView* treeView = simulatedObjectWidget->GetDockWidget()->findChild<QTreeView*>("EMFX.SimulatedObjectWidget.TreeView");
        ASSERT_TRUE(treeView);
        const SimulatedObjectModel* model = reinterpret_cast<SimulatedObjectModel*>(treeView->model());
        const QModelIndex index = model->index(0, 0);

        treeView->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        treeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        treeView->scrollTo(index);

        BringUpContextMenu(treeView, treeView->visualRect(index));
        QMenu* contextMenu = simulatedObjectWidget->GetDockWidget()->findChild<QMenu*>("EMFX.SimulatedObjectWidget.ContextMenu");
        EXPECT_TRUE(contextMenu);
        QAction* removeObjectAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(removeObjectAction, contextMenu, "Remove object"));
        removeObjectAction->trigger();

        ASSERT_EQ(m_actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), 0);
    }

    TEST_F(CanAddSimulatedObjectFixture, CanAddColliderToSimulatedObjectFromInspector)
    {
        RecordProperty("test_case_id", "C20385259");

        m_actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(5, "CanAddSimulatedObjectActor");

        CreateSimulateObject("sim1");

        // Get the Skeleton Outliner and find the model relating to its treeview
        auto skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        ASSERT_TRUE(skeletonOutliner);
        QTreeView* treeView = skeletonOutliner->GetDockWidget()->findChild<QTreeView*>("skeletonOutlinerTreeView");
        ASSERT_TRUE(treeView);
        const QAbstractItemModel* model = treeView->model();

        QModelIndexList indexList;
        RecursiveGetAllChildren(treeView, model->index(0, 0), indexList);

        SelectIndexes(indexList, treeView, 3, 3);

        QDockWidget* simulatedObjectInspectorDock = EMStudio::GetMainWindow()->findChild<QDockWidget*>("SimulatedObjectWidget::m_simulatedObjectInspectorDock");
        ASSERT_TRUE(simulatedObjectInspectorDock);
        QPushButton* addColliderButton = simulatedObjectInspectorDock->findChild<QPushButton*>("EMFX.SimulatedObjectColliderWidget.AddColliderButton");
        ASSERT_TRUE(addColliderButton);
        // Send the left button click directly to the button
        QTest::mouseClick(addColliderButton, Qt::LeftButton);

        const size_t numCapsuleColliders = PhysicsSetupUtils::CountColliders(m_actor.get(), PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Capsule);
        EXPECT_EQ(numCapsuleColliders, 0);

        const size_t numSphereColliders = PhysicsSetupUtils::CountColliders(m_actor.get(), PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Sphere);
        EXPECT_EQ(numSphereColliders, 0);

        QMenu* contextMenu = addColliderButton->findChild<QMenu*>("EMFX.AddColliderButton.ContextMenu");
        EXPECT_TRUE(contextMenu);

        QAction* addCapsuleAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(addCapsuleAction, contextMenu, "Add capsule"));
        addCapsuleAction->trigger();
        const size_t numCollidersAfterAddCapsule = PhysicsSetupUtils::CountColliders(m_actor.get(), PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Capsule);
        ASSERT_EQ(numCollidersAfterAddCapsule, numCapsuleColliders + 1) << "Capsule collider not added.";

        QAction* addSphereAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(addSphereAction, contextMenu, "Add sphere"));
        addSphereAction->trigger();
        const size_t numCollidersAfterAddSphere = PhysicsSetupUtils::CountColliders(m_actor.get(), PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Sphere);
        ASSERT_EQ(numCollidersAfterAddSphere, numSphereColliders + 1) << "Sphere collider not added.";
    }

    TEST_F(CanAddSimulatedObjectFixture, CanAddMultipleJointsToSimulatedObject)
    {
        RecordProperty("test_case_id", "C13048818");

        m_actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(7, "CanAddSimulatedObjectActor");

        CreateSimulateObject("ANY");

        // Get the Skeleton Outliner and find the model relating to its treeview
        auto skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        ASSERT_TRUE(skeletonOutliner) << "Can't find SkeletonOutlinerPlugin";
        QTreeView* treeView = skeletonOutliner->GetDockWidget()->findChild<QTreeView*>("skeletonOutlinerTreeView");
        ASSERT_TRUE(treeView) << "Skeleton Treeview not found";
        const QAbstractItemModel* model = treeView->model();

        QModelIndexList indexList;
        RecursiveGetAllChildren(treeView, model->index(0, 0), indexList);

        SelectIndexes(indexList, treeView, 3, 5);

        // Open the Right Click Context Menu
        const QRect rect = treeView->visualRect(indexList[4]);
        EXPECT_TRUE(rect.isValid());
        BringUpContextMenu(treeView, rect);

        // Trace down the sub Menus to <New simulated object> and select it
        QMenu* contextMenu = skeletonOutliner->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        EXPECT_TRUE(contextMenu) << "Context Menu not found";
        QAction* simulatedObjectAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(simulatedObjectAction, contextMenu, "Simulated object"));
        QMenu* simulatedObjectMenu = simulatedObjectAction->menu();
        EXPECT_TRUE(simulatedObjectMenu) << "Simulated Object Menu not found";
        QAction* addSelectedJointAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(addSelectedJointAction, simulatedObjectMenu, "Add selected joints"));
        QMenu* addSelectedJointMenu = addSelectedJointAction->menu();
        EXPECT_TRUE(addSelectedJointMenu) << "Add selected joints menu not found";
        QAction* newSimulatedObjectAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(newSimulatedObjectAction, addSelectedJointMenu, "ANY")) << "Can't find named simulated object in menu";
        newSimulatedObjectAction->trigger();

        const EMotionFX::SimulatedObject* simulatedObject = m_actor->GetSimulatedObjectSetup()->GetSimulatedObject(0);
        EXPECT_EQ(simulatedObject->GetNumSimulatedRootJoints(), 1);
        EXPECT_EQ(simulatedObject->GetNumSimulatedJoints(), 3);
    }
} // namespace EMotionFX
