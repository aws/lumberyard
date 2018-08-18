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

// include required headers
#include "RenderViewWidget.h"
#include "../MainWindow.h"
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include "../EMStudioManager.h"
#include <MysticQt/Source/RecentFiles.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/CommandSystem/Source/ActorInstanceCommands.h>


namespace EMStudio
{
    // create the context menu
    void RenderViewWidget::OnContextMenuEvent(QWidget* renderWidget, bool ctrlPressed, int32 localMouseX, int32 localMouseY, QPoint globalMousePos, RenderPlugin* plugin, MCommon::Camera* camera)
    {
        MCORE_UNUSED(ctrlPressed);
        MCORE_UNUSED(localMouseX);
        MCORE_UNUSED(localMouseY);
        MCORE_UNUSED(camera);
        /*
            // find the actor instance under the cursor
            ActorInstance* actorInstance = nullptr;
            const MCore::Ray cameraRay = camera->Unproject(localMouseX, localMouseY);

            // iterate over all existing actor instances and find the one that is hit by the ray
            const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
            for (uint32 i=0; i<numActorInstances; ++i)
            {
                // get the current actor instance
                ActorInstance* currInstance = EMotionFX::GetActorManager().GetActorInstance(i);
                if (currInstance == nullptr)
                    continue;

                // check if actor instance is visible and gets rendered
                if (currInstance->GetIsVisible() == false || currInstance->GetRender() == false)
                    continue;

                // check for an intersection
                Vector3 intersect, normal;
                Node* intersectionNode = currInstance->IntersectsMesh(0, cameraRay, &intersect, &normal);
                if (intersectionNode)
                {
                    // set the actor instance
                    actorInstance = currInstance;

                    // get the currently selected actor instance
                    EMotionFX::ActorInstance* selectedInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
                    if (currInstance != selectedInstance)
                    {
                        // call selection commands, depending on selection state and ctrl state
                        String outResult;
                        CommandGroup commandGroup("Select actor instances", 2);
                        if (ctrlPressed == false)
                            commandGroup.AddCommandString( "Unselect -actorInstanceID SELECT_ALL" );

                        String command;
                        command = AZStd::string::format("Select -actorInstanceID %i", actorInstance->GetID());
                        commandGroup.AddCommandString( command.AsChar() );

                        // execute the commands
                        if(GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
                        {
                            if (outResult.GetIsEmpty() == false)
                                LogError( outResult.AsChar() );
                        }
                    }

                    // break look because instance has been found
                    break;
                }
            }
        */

        // read the maximum number of recent files
        QSettings settings(this);
        settings.beginGroup("EMotionFX");
        const int32 maxRecentFiles = settings.value("maxRecentFiles", 16).toInt(); // default to 16 recent files in case we start EMStudio the first time

        // get the main window from the emstudio manager
        MainWindow* mainWindow = GetManager()->GetMainWindow();

        // create the context menu
        QMenu menu(renderWidget);

        // add menu related to actor instance if at least one selected
        if (GetCommandManager()->GetCurrentSelection().GetNumSelectedActorInstances() > 0)
        {
            // action to reset transformation
            QAction* resetAction = menu.addAction("Reset Transform");

            // add seperator line
            menu.addSeparator();

            // action to unselect
            QAction* unselectAction = menu.addAction("Unselect All Actor Instances");

            // add seperator line
            menu.addSeparator();

            // action for visibility
            QAction* hideAction     = menu.addAction("Hide Selected Actor Instances");
            QAction* unhideAction   = menu.addAction("Unhide Selected Actor Instances");

            // add seperator line
            menu.addSeparator();

            // actions for cloning/removing
            QAction* cloneAction    = menu.addAction("Clone Selected Actor Instances");
            QAction* removeAction   = menu.addAction("Remove Selected Actor Instances");

            // add seperator line
            menu.addSeparator();

            // connect the context menu to the corresponding slots
            connect(resetAction,       SIGNAL(triggered()), renderWidget, SLOT(ResetToBindPose()));
            connect(unselectAction,    SIGNAL(triggered()), renderWidget, SLOT(UnselectSelectedActorInstances()));
            connect(hideAction,        SIGNAL(triggered()), renderWidget, SLOT(MakeSelectedActorInstancesInvisible()));
            connect(unhideAction,      SIGNAL(triggered()), renderWidget, SLOT(MakeSelectedActorInstancesVisible()));
            connect(cloneAction,       SIGNAL(triggered()), renderWidget, SLOT(CloneSelectedActorInstances()));
            connect(removeAction,      SIGNAL(triggered()), renderWidget, SLOT(RemoveSelectedActorInstances()));
        }

        // add actions for loading actors
        QAction* openAction         = menu.addAction("Open Actor");
        QAction* mergeAction        = menu.addAction("Merge Actor");

        // create recent actors sub menu
        MysticQt::RecentFiles recentActors;
        recentActors.Init(&menu, maxRecentFiles, "Recent Actors", "recentActorFiles");
        connect(&recentActors, SIGNAL(OnRecentFile(QAction*)), (QObject*)mainWindow, SLOT(OnRecentFile(QAction*)));

        // add a separator line
        menu.addSeparator();

        // for opening projects
        QAction* openProjectAction = menu.addAction("Open Workspace");

        // create recent projects sub menu
        MysticQt::RecentFiles recentProjects;
        recentProjects.Init(&menu, maxRecentFiles, "Recent Workspaces", "recentWorkspaces");
        connect(&recentProjects, SIGNAL(OnRecentFile(QAction*)), (QObject*)mainWindow, SLOT(OnRecentFile(QAction*)));

        // add a separator line
        menu.addSeparator();

        // add actions for actor instance transformations
        QMenu* modeMenu             = menu.addMenu("Transform");
        QAction* selectAction       = modeMenu->addAction("Selection Mode");
        QAction* translateAction    = modeMenu->addAction("Translate");
        QAction* rotateAction       = modeMenu->addAction("Rotate");
        QAction* scaleAction        = modeMenu->addAction("Scale");

        // add a separator line
        menu.addSeparator();

        // add sub menu for camera control
        menu.addMenu(GetCameraMenu());

        // set the icons of the context menu items
        openAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Open.png"));
        mergeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Open.png"));
        openProjectAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Open.png"));
        selectAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Rendering/Select.png"));
        translateAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Rendering/Translate.png"));
        rotateAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Rendering/Rotate.png"));
        scaleAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Rendering/Scale.png"));

        // connect the context menu to the corresponding slots
        connect(openAction,         SIGNAL(triggered()), (QObject*)mainWindow,  SLOT(OnFileOpenActor()));
        connect(mergeAction,        SIGNAL(triggered()), (QObject*)mainWindow,  SLOT(OnFileMergeActor()));
        connect(openProjectAction,  SIGNAL(triggered()), (QObject*)mainWindow,  SLOT(OnFileOpenWorkspace()));
        connect(selectAction,       SIGNAL(triggered()), (QObject*)plugin,      SLOT(SetSelectionMode()));
        connect(translateAction,    SIGNAL(triggered()), (QObject*)plugin,      SLOT(SetTranslationMode()));
        connect(rotateAction,       SIGNAL(triggered()), (QObject*)plugin,      SLOT(SetRotationMode()));
        connect(scaleAction,        SIGNAL(triggered()), (QObject*)plugin,      SLOT(SetScaleMode()));

        // show the menu at the given position
        menu.exec(globalMousePos);

        settings.endGroup();
    }
}
