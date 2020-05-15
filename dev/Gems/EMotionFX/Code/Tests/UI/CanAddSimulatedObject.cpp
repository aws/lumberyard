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

namespace EMotionFX
{
    TEST_F(UIFixture, CanAddSimulatedObject)
    {
        RecordProperty("test_case_id", "C13048820");

        AutoRegisteredActor actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(1, "CanAddSimulatedObjectActor");

        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("Select -actorID 0", result)) << result.c_str();
        }

        EMStudio::GetMainWindow()->ApplicationModeChanged("SimulatedObjects");

        auto simulatedObjectWidget = static_cast<EMotionFX::SimulatedObjectWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SimulatedObjectWidget::CLASS_ID));
        ASSERT_TRUE(simulatedObjectWidget) << "Simulated Object plugin not found!";

        auto addSimulatedObjectButton = simulatedObjectWidget->GetDockWidget()->findChild<QPushButton*>("addSimulatedObjectButton");

        QTest::mouseClick(addSimulatedObjectButton, Qt::LeftButton);

        const QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
        auto inputDialogIt = AZStd::find_if(topLevelWidgets.begin(), topLevelWidgets.end(), [](const QWidget* widget) {
             return widget->objectName() == "newSimulatedObjectDialog";
        });
        ASSERT_NE(inputDialogIt, topLevelWidgets.end());
        auto inputDialog = qobject_cast<EMStudio::InputDialogValidatable*>(*inputDialogIt);
        inputDialog->SetText("New simulated object");
        inputDialog->accept();

        ASSERT_EQ(actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), 1);
        EXPECT_STREQ(actor->GetSimulatedObjectSetup()->GetSimulatedObject(0)->GetName().c_str(), "New simulated object");

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
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
        const QRect rect = treeView->visualRect(rootJointIndex);
        ASSERT_TRUE(rect.isValid());
        {
            QContextMenuEvent cme(QContextMenuEvent::Mouse, rect.center(), treeView->viewport()->mapTo(treeView->window(), rect.center()));
            QSpontaneKeyEvent::setSpontaneous(&cme);
            QApplication::instance()->notify(
                treeView->viewport(),
                &cme
            );
        }

        QMenu* contextMenu = skeletonOutliner->GetDockWidget()->findChild<QMenu*>("contextMenu");
        ASSERT_TRUE(contextMenu);
        const auto contextMenuActions = contextMenu->actions();
        auto simulatedObjectAction = AZStd::find_if(contextMenuActions.begin(), contextMenuActions.end(), [](const QAction* action) {
            return action->text() == "Simulated object";
        });
        ASSERT_NE(simulatedObjectAction, contextMenuActions.end());
        QMenu* simulatedObjectMenu = (*simulatedObjectAction)->menu();
        const auto simulatedObjectMenuActions = simulatedObjectMenu->actions();
        auto addSelectedJointAction = AZStd::find_if(simulatedObjectMenuActions.begin(), simulatedObjectMenuActions.end(), [](const QAction* action) {
            return action->text() == "Add selected joint";
        });
        ASSERT_NE(addSelectedJointAction, simulatedObjectMenuActions.end());
        QMenu* addSelectedJointMenu = (*addSelectedJointAction)->menu();
        const auto addSelectedJointMenuActions = addSelectedJointMenu->actions();
        auto newSimulatedObjectAction = AZStd::find_if(addSelectedJointMenuActions.begin(), addSelectedJointMenuActions.end(), [](const QAction* action) {
            return action->text() == "<New simulated object>";
        });
        ASSERT_NE(newSimulatedObjectAction, addSelectedJointMenuActions.end());
        (*newSimulatedObjectAction)->trigger();

        const QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
        auto inputDialogIt = AZStd::find_if(topLevelWidgets.begin(), topLevelWidgets.end(), [](const QWidget* widget) {
             return widget->objectName() == "newSimulatedObjectDialog";
        });
        ASSERT_NE(inputDialogIt, topLevelWidgets.end());
        auto inputDialog = qobject_cast<EMStudio::InputDialogValidatable*>(*inputDialogIt);
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
} // namespace EMotionFX
