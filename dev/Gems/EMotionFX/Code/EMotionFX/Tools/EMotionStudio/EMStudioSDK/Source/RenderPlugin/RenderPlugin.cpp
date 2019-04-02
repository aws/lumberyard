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

#include <AzCore/RTTI/ReflectContext.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/RenderPlugin/ManipulatorCallbacks.h>
#include <EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderLayouts.h>
#include <EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderPlugin.h>
#include <MysticQt/Source/KeyboardShortcutManager.h>
#include <QMessageBox>


namespace EMStudio
{
    // constructor
    RenderPlugin::RenderPlugin()
        : DockWidgetPlugin()
    {
        mActors.SetMemoryCategory(MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);
        mLayouts.SetMemoryCategory(MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);
        mViewWidgets.SetMemoryCategory(MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

        mIsVisible                      = true;
        mRenderUtil                     = nullptr;
        mUpdateCallback                 = nullptr;

        mUpdateRenderActorsCallback     = nullptr;
        mReInitRenderActorsCallback     = nullptr;
        mCreateActorInstanceCallback    = nullptr;
        mRemoveActorInstanceCallback    = nullptr;
        mSelectCallback                 = nullptr;
        mUnselectCallback               = nullptr;
        mClearSelectionCallback         = nullptr;
        mResetToBindPoseCallback        = nullptr;
        mAdjustActorInstanceCallback    = nullptr;

        mZoomInCursor                   = nullptr;
        mZoomOutCursor                  = nullptr;
        mTranslateCursor                = nullptr;
        mRotateCursor                   = nullptr;
        mNotAllowedCursor               = nullptr;

        mSelectionModeButton            = nullptr;
        mTranslationModeButton          = nullptr;
        mRotationModeButton             = nullptr;
        mScaleModeButton                = nullptr;

        mBaseLayout                     = nullptr;
        mToolbarLayout                  = nullptr;
        mToolbarWidget                  = nullptr;
        mRenderLayoutWidget             = nullptr;
        mActiveViewWidget               = nullptr;
        mSignalMapper                   = nullptr;
        mCurrentSelection               = nullptr;
        mCurrentLayout                  = nullptr;
        mFocusViewWidget                = nullptr;
        mMode                           = MODE_SELECTION;
        mFirstFrameAfterReInit          = false;

        mTranslateManipulator           = nullptr;
        mRotateManipulator              = nullptr;
        mScaleManipulator               = nullptr;
    }


    RenderPlugin::~RenderPlugin()
    {
        EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusDisconnect();

        SaveRenderOptions();
        CleanEMStudioActors();

        // Get rid of the OpenGL view widgets.
        // Don't delete them directly as there might be still paint events in the Qt message queue which will cause a crash.
        // deleteLater will make sure all events will be processed before actually destructing the object.
        const uint32 numViewWidgets = mViewWidgets.GetLength();
        for (uint32 i = 0; i < numViewWidgets; ++i)
        {
            mViewWidgets[i]->deleteLater();
        }

        // get rid of the layouts
        const uint32 numLayouts = mLayouts.GetLength();
        for (uint32 i = 0; i < numLayouts; ++i)
        {
            delete mLayouts[i];
        }
        mLayouts.Clear();

        // delete the gizmos
        GetManager()->RemoveTransformationManipulator(mTranslateManipulator);
        GetManager()->RemoveTransformationManipulator(mRotateManipulator);
        GetManager()->RemoveTransformationManipulator(mScaleManipulator);

        delete mTranslateManipulator;
        delete mRotateManipulator;
        delete mScaleManipulator;

        // get rid of the signal mapper
        delete mSignalMapper;

        // get rid of the cursors
        delete mZoomInCursor;
        delete mZoomOutCursor;
        delete mTranslateCursor;
        delete mRotateCursor;
        delete mNotAllowedCursor;

        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(mUpdateRenderActorsCallback, false);
        GetCommandManager()->RemoveCommandCallback(mReInitRenderActorsCallback, false);
        GetCommandManager()->RemoveCommandCallback(mCreateActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mResetToBindPoseCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustActorInstanceCallback, false);

        delete mUpdateRenderActorsCallback;
        delete mReInitRenderActorsCallback;
        delete mCreateActorInstanceCallback;
        delete mRemoveActorInstanceCallback;
        delete mSelectCallback;
        delete mUnselectCallback;
        delete mClearSelectionCallback;
        delete mResetToBindPoseCallback;
        delete mAdjustActorInstanceCallback;

        // get rid of the trace paths
        const uint32 numPaths = mTrajectoryTracePaths.GetLength();
        for (uint32 i = 0; i < numPaths; ++i)
        {
            delete mTrajectoryTracePaths[i];
        }
        mTrajectoryTracePaths.Clear();
    }


    // get rid of the render actors
    void RenderPlugin::CleanEMStudioActors()
    {
        // get rid of the actors
        const uint32 numActors = mActors.GetLength();
        for (uint32 i = 0; i < numActors; ++i)
        {
            if (mActors[i])
            {
                delete mActors[i];
            }
        }
        mActors.Clear();
    }


    // get rid of the given render actor
    bool RenderPlugin::DestroyEMStudioActor(EMotionFX::Actor* actor)
    {
        // get the corresponding emstudio actor
        EMStudioRenderActor* emstudioActor = FindEMStudioActor(actor);
        if (emstudioActor == nullptr)
        {
            MCore::LogError("Cannot destroy render actor. There is no render actor registered for this actor.");
            return false;
        }

        // get the index of the emstudio actor, we can be sure it is valid as else the emstudioActor pointer would be nullptr already
        const uint32 index = FindEMStudioActorIndex(emstudioActor);
        MCORE_ASSERT(index != MCORE_INVALIDINDEX32);

        // get rid of the emstudio actor
        delete emstudioActor;
        mActors.Remove(index);
        return true;
    }


    // finds the manipulator the is currently hit by the mouse within the current camera
    MCommon::TransformationManipulator* RenderPlugin::GetActiveManipulator(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY)
    {
        // get the current manipulator
        MCore::Array<MCommon::TransformationManipulator*>* transformationManipulators = GetManager()->GetTransformationManipulators();
        const uint32 numGizmos = transformationManipulators->GetLength();

        // init the active manipulator to nullptr
        MCommon::TransformationManipulator* activeManipulator = nullptr;
        float minCamDist = camera->GetFarClipDistance();
        bool activeManipulatorFound = false;

        // iterate over all gizmos and search for the hit one that is closest to the camera
        for (uint32 i = 0; i < numGizmos; ++i)
        {
            // get the current manipulator and check if it exists
            MCommon::TransformationManipulator* currentManipulator = transformationManipulators->GetItem(i);
            if (currentManipulator == nullptr || currentManipulator->GetIsVisible() == false)
            {
                continue;
            }

            // return the manipulator if selection is already locked
            if (currentManipulator->GetSelectionLocked())
            {
                activeManipulator = currentManipulator;
                activeManipulatorFound = true;
            }

            // check if manipulator is hit or if the selection is locked
            if (currentManipulator->Hit(camera, mousePosX, mousePosY))
            {
                // calculate the distance of the camera and the active manipulator
                float distance = (camera->GetPosition() - currentManipulator->GetPosition()).GetLength();
                if (distance < minCamDist && activeManipulatorFound == false)
                {
                    minCamDist = distance;
                    activeManipulator = currentManipulator;
                }
            }
            else
            {
                if (currentManipulator->GetSelectionLocked() == false)
                {
                    currentManipulator->SetMode(0);
                }
            }
        }

        // return the manipulator
        return activeManipulator;
    }


    // reinit the transformation manipulators
    void RenderPlugin::ReInitTransformationManipulators()
    {
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (mScaleManipulator == nullptr || mTranslateManipulator == nullptr || mRotateManipulator == nullptr)
        {
            return;
        }

        mTranslateManipulator->SetIsVisible(actorInstance && mMode == MODE_TRANSLATIONMANIPULATOR);
        mRotateManipulator->SetIsVisible(actorInstance && mMode == MODE_ROTATIONMANIPULATOR);
        mScaleManipulator->SetIsVisible(actorInstance && mMode == MODE_SCALEMANIPULATOR);

        if (actorInstance == nullptr)
        {
            return;
        }

        switch (mMode)
        {
        case RenderPlugin::MODE_TRANSLATIONMANIPULATOR:
        {
            mTranslateManipulator->Init(actorInstance->GetLocalSpaceTransform().mPosition);
            mTranslateManipulator->SetCallback(new TranslateManipulatorCallback(actorInstance, actorInstance->GetLocalSpaceTransform().mPosition));
            break;
        }
        case RenderPlugin::MODE_ROTATIONMANIPULATOR:
        {
            mRotateManipulator->Init(actorInstance->GetLocalSpaceTransform().mPosition);
            mRotateManipulator->SetCallback(new RotateManipulatorCallback(actorInstance, actorInstance->GetLocalSpaceTransform().mRotation));
            break;
        }
        case RenderPlugin::MODE_SCALEMANIPULATOR:
        {
            mScaleManipulator->Init(actorInstance->GetLocalSpaceTransform().mPosition);
            mScaleManipulator->SetCallback(new ScaleManipulatorCallback(actorInstance, actorInstance->GetLocalSpaceTransform().mScale));
            break;
        }
        default:
        {
            break;
        }
        }
    }


    void RenderPlugin::ZoomToJoints(EMotionFX::ActorInstance* actorInstance, const AZStd::vector<EMotionFX::Node*>& joints)
    {
        if (!actorInstance || joints.empty())
        {
            return;
        }

        MCore::AABB aabb;
        aabb.Init();

        const EMotionFX::Actor* actor = actorInstance->GetActor();
        const EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
        const EMotionFX::Pose* pose = actorInstance->GetTransformData()->GetCurrentPose();

        for (const EMotionFX::Node* joint : joints)
        {
            const AZ::Vector3 jointPosition = pose->GetWorldSpaceTransform(joint->GetNodeIndex()).mPosition;

            aabb.Encapsulate(jointPosition);

            const AZ::u32 childCount = joint->GetNumChildNodes();
            for (AZ::u32 i = 0; i < childCount; ++i)
            {
                EMotionFX::Node* childJoint = skeleton->GetNode(joint->GetChildIndex(i));
                const AZ::Vector3 childPosition = pose->GetWorldSpaceTransform(childJoint->GetNodeIndex()).mPosition;
                aabb.Encapsulate(childPosition);
            }
        }

        if (aabb.CheckIfIsValid())
        {
            aabb.Widen(aabb.CalcRadius());

            bool isFollowModeActive = false;
            const uint32 numViewWidgets = mViewWidgets.GetLength();
            for (AZ::u32 i = 0; i < numViewWidgets; ++i)
            {
                const RenderViewWidget* renderViewWidget = mViewWidgets[i];
                RenderWidget* current = renderViewWidget->GetRenderWidget();

                if (renderViewWidget->GetIsCharacterFollowModeActive())
                {
                    isFollowModeActive = true;
                }

                current->ViewCloseup(aabb, 1.0f);
            }

            if (isFollowModeActive)
            {
                QMessageBox::warning(mDock, "Please disable character follow mode", "Zoom to joints is only working in case character follow mode is disabled.\nPlease disable character follow mode in the render view menu: Camera -> Follow Mode", QMessageBox::Ok);
            }
        }
    }


    // try to locate the helper actor for a given instance
    RenderPlugin::EMStudioRenderActor* RenderPlugin::FindEMStudioActor(EMotionFX::ActorInstance* actorInstance, bool doubleCheckInstance)
    {
        // get the number of emstudio actors and iterate through them
        const uint32 numEMStudioRenderActors = mActors.GetLength();
        for (uint32 i = 0; i < numEMStudioRenderActors; ++i)
        {
            EMStudioRenderActor* EMStudioRenderActor = mActors[i];

            // is the parent actor of the instance the same as the one in the emstudio actor?
            if (EMStudioRenderActor->mActor == actorInstance->GetActor())
            {
                // double check if the actor instance is in the actor instance array inside the emstudio actor
                if (doubleCheckInstance)
                {
                    // now double check if the actor instance really is in the array of instances of this emstudio actor
                    const uint32 numActorInstances = EMStudioRenderActor->mActorInstances.GetLength();
                    for (uint32 a = 0; a < numActorInstances; ++a)
                    {
                        if (EMStudioRenderActor->mActorInstances[a] == actorInstance)
                        {
                            return EMStudioRenderActor;
                        }
                    }
                }
                else
                {
                    return EMStudioRenderActor;
                }
            }
        }

        return nullptr;
    }


    // try to locate the helper actor for a given one
    RenderPlugin::EMStudioRenderActor* RenderPlugin::FindEMStudioActor(EMotionFX::Actor* actor)
    {
        // get the number of emstudio actors and iterate through them
        const uint32 numEMStudioRenderActors = mActors.GetLength();
        for (uint32 i = 0; i < numEMStudioRenderActors; ++i)
        {
            EMStudioRenderActor* EMStudioRenderActor = mActors[i];

            // is the actor the same as the one in the emstudio actor?
            if (EMStudioRenderActor->mActor == actor)
            {
                return EMStudioRenderActor;
            }
        }

        return nullptr;
    }


    // get the index of the given emstudio actor
    uint32 RenderPlugin::FindEMStudioActorIndex(EMStudioRenderActor* EMStudioRenderActor)
    {
        // get the number of emstudio actors and iterate through them
        const uint32 numEMStudioRenderActors = mActors.GetLength();
        for (uint32 i = 0; i < numEMStudioRenderActors; ++i)
        {
            // compare the two emstudio actors and return the current index in case of success
            if (EMStudioRenderActor == mActors[i])
            {
                return i;
            }
        }

        // the emstudio actor has not been found
        return MCORE_INVALIDINDEX32;
    }


    void RegisterRenderPluginLayouts(RenderPlugin* renderPlugin)
    {
        // add the available render template layouts
        renderPlugin->AddLayout(new SingleRenderWidget());
        renderPlugin->AddLayout(new HorizontalDoubleRenderWidget());
        renderPlugin->AddLayout(new VerticalDoubleRenderWidget());
        renderPlugin->AddLayout(new QuadrupleRenderWidget());
        renderPlugin->AddLayout(new TripleBigLeft());
        renderPlugin->AddLayout(new TripleBigRight());
        renderPlugin->AddLayout(new TripleBigTop());
        renderPlugin->AddLayout(new TripleBigBottom());
    }


    // add the given render actor to the array
    void RenderPlugin::AddEMStudioActor(EMStudioRenderActor* emstudioActor)
    {
        // add the actor to the list and return success
        mActors.Add(emstudioActor);
    }


    void RenderPlugin::ReInit(bool resetViewCloseup)
    {
        uint32 numActors = EMotionFX::GetActorManager().GetNumActors();
        for (uint32 i = 0; i < numActors; ++i)
        {
            // get the current actor and the number of clones
            EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

            if (actor->GetIsOwnedByRuntime())
            {
                continue;
            }

            // try to find the corresponding emstudio actor
            EMStudioRenderActor* emstudioActor = FindEMStudioActor(actor);

            // in case there is no emstudio actor yet create one
            if (emstudioActor == nullptr)
            {
                CreateEMStudioActor(actor);
            }
        }

        //  uint32 numEMStudioActors = mActors.GetLength();
        for (uint32 i = 0; i < mActors.GetLength(); ++i)
        {
            EMStudioRenderActor* emstudioActor = mActors[i];
            EMotionFX::Actor* actor = emstudioActor->mActor;

            if (actor->GetIsOwnedByRuntime())
            {
                continue;
            }

            bool found = false;

            for (uint32 j = 0; j < numActors; ++j)
            {
                EMotionFX::Actor* curActor = EMotionFX::GetActorManager().GetActor(j);
                if (actor == curActor)
                {
                    found = true;
                }
            }

            if (found == false)
            {
                DestroyEMStudioActor(actor);
            }
        }
        //numEMStudioActors = mActors.GetLength();

        // get the number of actor instances and iterate through them
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().GetActorInstance(i);
            EMotionFX::Actor*           actor           = actorInstance->GetActor();
            EMStudioRenderActor*        emstudioActor   = FindEMStudioActor(actorInstance, false);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            // in case the actor instance has not been mapped to an emstudio actor yet, do this here
            if (emstudioActor == nullptr)
            {
                // get the number of emstudio actors and iterate through them
                //numEMStudioActors = mActors.GetLength();
                for (uint32 j = 0; j < mActors.GetLength(); ++j)
                {
                    EMStudioRenderActor* currentEMStudioActor = mActors[j];
                    if (actor == currentEMStudioActor->mActor)
                    {
                        emstudioActor = currentEMStudioActor;
                        break;
                    }
                }
            }

            MCORE_ASSERT(emstudioActor);

            // set the GL actor
            actorInstance->SetCustomData(emstudioActor->mRenderActor);

            // add the actor instance to the emstudio actor instances in case it is not in yet
            if (emstudioActor->mActorInstances.Find(actorInstance) == MCORE_INVALIDINDEX32)
            {
                emstudioActor->mActorInstances.Add(actorInstance);
            }
        }

        // get the number of emstudio actors and iterate through them
        //numEMStudioActors = mActors.GetLength();
        for (uint32 i = 0; i < mActors.GetLength(); ++i)
        {
            EMStudioRenderActor* emstudioActor = mActors[i];

            for (uint32 j = 0; j < emstudioActor->mActorInstances.GetLength();)
            {
                EMotionFX::ActorInstance* emstudioActorInstance = emstudioActor->mActorInstances[j];
                bool found = false;

                for (uint32 k = 0; k < numActorInstances; ++k)
                {
                    if (emstudioActorInstance == EMotionFX::GetActorManager().GetActorInstance(k))
                    {
                        found = true;
                        break;
                    }
                }

                if (found == false)
                {
                    emstudioActor->mActorInstances.Remove(j);
                }
                else
                {
                    j++;
                }
            }
        }

        mFirstFrameAfterReInit = true;

        // zoom the camera to the available character only in case we're dealing with a single instance
        if (resetViewCloseup && numActorInstances == 1)
        {
            ViewCloseup(false);
        }

        ReInitTransformationManipulators();
    }


    //--------------------------------------------------------------------------------------------------------
    // class EMStudioGLActor
    //--------------------------------------------------------------------------------------------------------

    // constructor
    RenderPlugin::EMStudioRenderActor::EMStudioRenderActor(EMotionFX::Actor* actor, RenderGL::GLActor* renderActor)
    {
        mRenderActor                = renderActor;
        mActor                      = actor;
        mNormalsScaleMultiplier     = 1.0f;
        mCharacterHeight            = 0.0f;
        mOffsetFromTrajectoryNode   = 0.0f;
        mMustCalcNormalScale        = true;

        // extract the bones from the actor and add it to the array
        actor->ExtractBoneList(0, &mBoneList);

        CalculateNormalScaleMultiplier();
    }


    // destructor
    RenderPlugin::EMStudioRenderActor::~EMStudioRenderActor()
    {
        // get the number of actor instances and iterate through them
        const uint32 numActorInstances = mActorInstances.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = mActorInstances[i];

            // only delete the actor instance in case it is still inside the actor manager
            // in case it is not present there anymore this means an undo command has already deleted it
            if (EMotionFX::GetActorManager().FindActorInstanceIndex(actorInstance) != MCORE_INVALIDINDEX32)
            {
                //actorInstance->Destroy();
            }
            // in case the actor instance is not valid anymore make sure to unselect the instance to avoid bad pointers
            else
            {
                CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
                selection.RemoveActorInstance(actorInstance);
            }
        }

        // only delete the actor in case it is still inside the actor manager
        // in case it is not present there anymore this means an undo command has already deleted it
        if (EMotionFX::GetActorManager().FindActorIndex(mActor) != MCORE_INVALIDINDEX32)
        {
            //mActor->Destroy();
        }
        // in case the actor is not valid anymore make sure to unselect it to avoid bad pointers
        else
        {
            CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
            selection.RemoveActor(mActor);
        }

        // get rid of the OpenGL actor
        if (mRenderActor)
        {
            mRenderActor->Destroy();
        }
    }


    void RenderPlugin::EMStudioRenderActor::CalculateNormalScaleMultiplier()
    {
        // calculate the max extent of the character
        EMotionFX::ActorInstance* actorInstance = EMotionFX::ActorInstance::Create(mActor);
        actorInstance->UpdateMeshDeformers(0.0f, true);

        MCore::AABB aabb;
        actorInstance->CalcMeshBasedAABB(0, &aabb);

        if (aabb.CheckIfIsValid() == false)
        {
            actorInstance->CalcNodeOBBBasedAABB(&aabb);
        }

        if (aabb.CheckIfIsValid() == false)
        {
            actorInstance->CalcNodeBasedAABB(&aabb);
        }

        mCharacterHeight = aabb.CalcHeight();
        mOffsetFromTrajectoryNode = aabb.GetMin().GetY() + (mCharacterHeight * 0.5f);

        actorInstance->Destroy();

        // scale the normals down to 1% of the character size, that looks pretty nice on all models
        mNormalsScaleMultiplier = aabb.CalcRadius() * 0.01f;
    }


    // zoom to characters
    void RenderPlugin::ViewCloseup(bool selectedInstancesOnly, RenderWidget* renderWidget, float flightTime)
    {
        uint32 i;

        MCore::AABB sceneAABB = GetSceneAABB(selectedInstancesOnly);

        if (sceneAABB.CheckIfIsValid())
        {
            // in case the given view widget parameter is nullptr apply it on all view widgets
            if (renderWidget == nullptr)
            {
                const uint32 numViewWidgets = mViewWidgets.GetLength();
                for (i = 0; i < numViewWidgets; ++i)
                {
                    RenderWidget* current = mViewWidgets[i]->GetRenderWidget();
                    current->ViewCloseup(sceneAABB, flightTime);
                }
            }
            // only apply it to the given view widget
            else
            {
                renderWidget->ViewCloseup(sceneAABB, flightTime);
            }
        }
    }


    // skip follow calcs
    void RenderPlugin::SetSkipFollowCalcs(bool skipFollowCalcs)
    {
        const uint32 numViewWidgets = mViewWidgets.GetLength();
        for (uint32 i = 0; i < numViewWidgets; ++i)
        {
            RenderWidget* current = mViewWidgets[i]->GetRenderWidget();
            current->SetSkipFollowCalcs(skipFollowCalcs);
        }
    }


    // remove the given view widget
    void RenderPlugin::RemoveViewWidget(RenderViewWidget* viewWidget)
    {
        mViewWidgets.RemoveByValue(viewWidget);
    }


    // remove all view widgets
    void RenderPlugin::ClearViewWidgets()
    {
        mViewWidgets.Clear();
    }


    // add a layout button
    QPushButton* RenderPlugin::AddLayoutButton(const char* imageFileName)
    {
        // create the button and add it to the layout
        QPushButton* button = new QPushButton(mToolbarWidget);

        button->setIcon(MysticQt::GetMysticQt()->FindIcon(imageFileName));
        button->setIconSize(QSize(32, 32)); // HACK REMOVE AND FIX ME
        button->setStyleSheet("border: 0px; border-radius: 0px");

        //button->setWhatsThis( layout->GetName() );
        button->setMinimumSize(36, 36); // HACK REMOVE AND FIX ME
        button->setMaximumSize(36, 36);

        mToolbarLayout->addWidget(button);
        return button;
    }

    void RenderPlugin::Reflect(AZ::ReflectContext* context)
    {
        RenderOptions::Reflect(context);
    }

    bool RenderPlugin::Init()
    {
        //mDock->setAllowedAreas(Qt::NoDockWidgetArea);

        // load the cursors
        mZoomInCursor       = new QCursor(QPixmap(AZStd::string(MysticQt::GetDataDir() + "Images/Rendering/ZoomInCursor.png").c_str()).scaled(32, 32));
        mZoomOutCursor      = new QCursor(QPixmap(AZStd::string(MysticQt::GetDataDir() + "Images/Rendering/ZoomOutCursor.png").c_str()).scaled(32, 32));
        //mTranslateCursor  = new QCursor( QPixmap(String(MysticQt::GetDataDir() + "Images/Rendering/TranslateCursor.png").c_str()) );
        //mRotateCursor     = new QCursor( QPixmap(String(MysticQt::GetDataDir() + "Images/Rendering/RotateCursor.png").c_str()) );
        //mNotAllowedCursor = new QCursor( QPixmap(String(MysticQt::GetDataDir() + "Images/Rendering/NotAllowedCursor.png").c_str()) );

        LoadRenderOptions();

        mSignalMapper       = new QSignalMapper(this);
        mCurrentSelection   = &GetCommandManager()->GetCurrentSelection();

        connect(mSignalMapper, static_cast<void (QSignalMapper::*)(const QString&)>(&QSignalMapper::mapped), this, &RenderPlugin::LayoutButtonPressed);
        connect(mDock, &MysticQt::DockWidget::visibilityChanged, this, &RenderPlugin::VisibilityChanged);

        // add the available render template layouts
        RegisterRenderPluginLayouts(this);

        // create the inner widget which contains the base layout
        mInnerWidget = new QWidget();
        mDock->SetContents(mInnerWidget);

        // the base layout contains the render layout templates on the left and the render views on the right
        mBaseLayout = new QHBoxLayout(mInnerWidget);
        mBaseLayout->setContentsMargins(0, 2, 2, 2);
        mBaseLayout->setSpacing(0);

        // the render template widget contains a vertical layout in which all the template buttons are
        mToolbarWidget = new QWidget(mInnerWidget);
        mToolbarLayout = new QVBoxLayout(mToolbarWidget);

        // make sure it does not use more space than required by the buttons
        mToolbarLayout->setSizeConstraint(QLayout::SetMaximumSize);
        mToolbarLayout->setSpacing(1);
        mToolbarLayout->setMargin(5);

        mSelectionModeButton = AddLayoutButton("Images/Rendering/Select.png");
        connect(mSelectionModeButton, &QPushButton::clicked, this, &RenderPlugin::SetSelectionMode);
        mTranslationModeButton = AddLayoutButton("Images/Rendering/Translate.png");
        connect(mTranslationModeButton, &QPushButton::clicked, this, &RenderPlugin::SetTranslationMode);
        mRotationModeButton = AddLayoutButton("Images/Rendering/Rotate.png");
        connect(mRotationModeButton, &QPushButton::clicked, this, &RenderPlugin::SetRotationMode);
        mScaleModeButton = AddLayoutButton("Images/Rendering/Scale.png");
        connect(mScaleModeButton, &QPushButton::clicked, this, &RenderPlugin::SetScaleMode);

        // get the render layout templates and iterate through them
        const uint32 numLayouts = mLayouts.GetLength();
        for (uint32 i = 0; i < numLayouts; ++i)
        {
            // get the current layout
            Layout* layout = mLayouts[i];

            // create the button and connect it to the signal mapper
            QPushButton* button = AddLayoutButton(layout->GetImageFileName());
            connect(button, &QPushButton::clicked, mSignalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
            mSignalMapper->setMapping(button, layout->GetName());
        }

        // add the render template widget to the base layout
        mBaseLayout->addWidget(mToolbarWidget);

        SetSelectionMode();


        // create and register the command callbacks only (only execute this code once for all plugins)
        mUpdateRenderActorsCallback     = new UpdateRenderActorsCallback(false);
        mReInitRenderActorsCallback     = new ReInitRenderActorsCallback(false);
        mCreateActorInstanceCallback    = new CreateActorInstanceCallback(false);
        mRemoveActorInstanceCallback    = new RemoveActorInstanceCallback(false);
        mSelectCallback                 = new SelectCallback(false);
        mUnselectCallback               = new UnselectCallback(false);
        mClearSelectionCallback         = new ClearSelectionCallback(false);
        mResetToBindPoseCallback        = new CommandResetToBindPoseCallback(false);
        mAdjustActorInstanceCallback    = new AdjustActorInstanceCallback(false);
        GetCommandManager()->RegisterCommandCallback("UpdateRenderActors", mUpdateRenderActorsCallback);
        GetCommandManager()->RegisterCommandCallback("ReInitRenderActors", mReInitRenderActorsCallback);
        GetCommandManager()->RegisterCommandCallback("CreateActorInstance", mCreateActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveActorInstance", mRemoveActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);
        GetCommandManager()->RegisterCommandCallback("ResetToBindPose", mResetToBindPoseCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustActorInstance", mAdjustActorInstanceCallback);

        // initialize the gizmos
        mTranslateManipulator   = (MCommon::TranslateManipulator*)GetManager()->AddTransformationManipulator(new MCommon::TranslateManipulator(70.0f, false));
        mScaleManipulator       = (MCommon::ScaleManipulator*)GetManager()->AddTransformationManipulator(new MCommon::ScaleManipulator(70.0f, false));
        mRotateManipulator      = (MCommon::RotateManipulator*)GetManager()->AddTransformationManipulator(new MCommon::RotateManipulator(70.0f, false));

        // set the default rendering layout
        LayoutButtonPressed(mRenderOptions.GetLastUsedLayout().c_str());

        EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusConnect();
        return true;
    }


    void RenderPlugin::SaveRenderOptions()
    {
        AZStd::string renderOptionsFilename(GetManager()->GetAppDataFolder());
        renderOptionsFilename += "EMStudioRenderOptions.cfg";
        QSettings settings(renderOptionsFilename.c_str(), QSettings::IniFormat, this);

        // save the general render options
        mRenderOptions.Save(&settings);

        AZStd::string groupName;
        if (mCurrentLayout)
        {
            // get the number of render views and iterate through them
            const uint32 numRenderViews = mViewWidgets.GetLength();
            for (uint32 i = 0; i < numRenderViews; ++i)
            {
                RenderViewWidget* renderView = mViewWidgets[i];

                groupName = AZStd::string::format("%s_%i", mCurrentLayout->GetName(), i);

                settings.beginGroup(groupName.c_str());
                renderView->SaveOptions(&settings);
                settings.endGroup();
            }
        }
    }


    void RenderPlugin::LoadRenderOptions()
    {
        AZStd::string renderOptionsFilename(GetManager()->GetAppDataFolder());
        renderOptionsFilename += "EMStudioRenderOptions.cfg";
        QSettings settings(renderOptionsFilename.c_str(), QSettings::IniFormat, this);
        mRenderOptions = RenderOptions::Load(&settings);

        AZStd::string groupName;
        if (mCurrentLayout)
        {
            // get the number of render views and iterate through them
            const uint32 numRenderViews = mViewWidgets.GetLength();
            for (uint32 i = 0; i < numRenderViews; ++i)
            {
                RenderViewWidget* renderView = mViewWidgets[i];

                groupName = AZStd::string::format("%s_%i", mCurrentLayout->GetName(), i);

                settings.beginGroup(groupName.c_str());
                renderView->LoadOptions(&settings);
                settings.endGroup();
            }
        }

        //SetAspiredRenderingFPS(mRenderOptions.mAspiredRenderFPS);
    }


    void RenderPlugin::SetSelectionMode()
    {
        mMode = MODE_SELECTION;
        mSelectionModeButton->setStyleSheet("border-radius: 0px; border: 1px solid rgb(244, 156, 28);");
        mTranslationModeButton->setStyleSheet("border-radius: 0px; border: 0px;");
        mRotationModeButton->setStyleSheet("border-radius: 0px; border: 0px;");
        mScaleModeButton->setStyleSheet("border-radius: 0px; border: 0px;");

        if (mTranslateManipulator)
        {
            mTranslateManipulator->SetIsVisible(false);
        }

        if (mScaleManipulator)
        {
            mScaleManipulator->SetIsVisible(false);
        }

        if (mRotateManipulator)
        {
            mRotateManipulator->SetIsVisible(false);
        }
    }


    void RenderPlugin::SetTranslationMode()
    {
        mMode = MODE_TRANSLATIONMANIPULATOR;
        mSelectionModeButton->setStyleSheet("border-radius: 0px; border: 0px;");
        mTranslationModeButton->setStyleSheet("border-radius: 0px; border: 1px solid rgb(244, 156, 28);");
        mRotationModeButton->setStyleSheet("border-radius: 0px; border: 0px;");
        mScaleModeButton->setStyleSheet("border-radius: 0px; border: 0px;");
        ReInitTransformationManipulators();
    }


    void RenderPlugin::SetRotationMode()
    {
        mMode = MODE_ROTATIONMANIPULATOR;
        mSelectionModeButton->setStyleSheet("border-radius: 0px; border: 0px;");
        mTranslationModeButton->setStyleSheet("border-radius: 0px; border: 0px;");
        mRotationModeButton->setStyleSheet("border-radius: 0px; border: 1px solid rgb(244, 156, 28);");
        mScaleModeButton->setStyleSheet("border-radius: 0px; border: 0px;");
        ReInitTransformationManipulators();
    }


    void RenderPlugin::SetScaleMode()
    {
        mMode = MODE_SCALEMANIPULATOR;
        mSelectionModeButton->setStyleSheet("border-radius: 0px; border: 0px;");
        mTranslationModeButton->setStyleSheet("border-radius: 0px; border: 0px;");
        mRotationModeButton->setStyleSheet("border-radius: 0px; border: 0px;");
        mScaleModeButton->setStyleSheet("border-radius: 0px; border: 1px solid rgb(244, 156, 28);");
        ReInitTransformationManipulators();
    }


    void RenderPlugin::VisibilityChanged(bool visible)
    {
        mIsVisible = visible;
    }


    void RenderPlugin::UpdateActorInstances(float timePassedInSeconds)
    {
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            // Ignore actors not owned by the tool.
            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            RenderGL::GLActor* renderActor = static_cast<RenderGL::GLActor*>(actorInstance->GetCustomData());
            if (!renderActor)
            {
                continue;
            }

            UpdateActorInstance(actorInstance, timePassedInSeconds);
        }
    }


    // handle timer event
    void RenderPlugin::ProcessFrame(float timePassedInSeconds)
    {
        // skip rendering in case we want to avoid updating any 3d views
        if (GetManager()->GetAvoidRendering() || mIsVisible == false)
        {
            return;
        }

        // update EMotion FX, but don't render
        EMotionFX::GetAnimGraphManager().SetAnimGraphVisualizationEnabled(false);
        UpdateActorInstances(timePassedInSeconds);

        const uint32 numViewWidgets = mViewWidgets.GetLength();
        for (uint32 i = 0; i < numViewWidgets; ++i)
        {
            RenderWidget* renderWidget = mViewWidgets[i]->GetRenderWidget();

            if (mFirstFrameAfterReInit == false)
            {
                renderWidget->GetCamera()->Update(timePassedInSeconds);
            }

            if (mFirstFrameAfterReInit)
            {
                mFirstFrameAfterReInit = false;
            }

            // redraw
            renderWidget->Update();
        }
    }


    // get the AABB containing all actor instances in the scene
    MCore::AABB RenderPlugin::GetSceneAABB(bool selectedInstancesOnly)
    {
        MCore::AABB finalAABB;
        CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();

        if (mUpdateCallback)
        {
            mUpdateCallback->SetEnableRendering(false);
        }

        // update EMotion FX, but don't render
        EMotionFX::GetEMotionFX().Update(0.0f);

        if (mUpdateCallback)
        {
            mUpdateCallback->SetEnableRendering(true);
        }

        // get the number of actor instances and iterate through them
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the actor instance and update its transformations and meshes
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            // only take the selected instances into account if wanted
            if (selectedInstancesOnly)
            {
                if (selection.CheckIfHasActorInstance(actorInstance) == false)
                {
                    continue;
                }
            }

            // get the mesh based AABB
            MCore::AABB aabb;
            actorInstance->CalcMeshBasedAABB(actorInstance->GetLODLevel(), &aabb);

            // get the node OBB based AABB
            if (aabb.CheckIfIsValid() == false)
            {
                actorInstance->CalcNodeOBBBasedAABB(&aabb);
            }

            // get the node based AABB
            if (aabb.CheckIfIsValid() == false)
            {
                actorInstance->CalcNodeBasedAABB(&aabb);
            }

            // make sure the actor instance is covered in our global bounding box
            finalAABB.Encapsulate(aabb);
        }

        return finalAABB;
    }


    void RenderPlugin::OnAfterLoadProject()
    {
        ViewCloseup(false);
    }

    void RenderPlugin::OnAfterLoadActors()
    {
        ViewCloseup(false);
    }

    RenderPlugin::Layout* RenderPlugin::FindLayoutByName(const AZStd::string& layoutName)
    {
        // get the render layout templates and iterate through them
        const uint32 numLayouts = mLayouts.GetLength();
        for (uint32 i = 0; i < numLayouts; ++i)
        {
            // get the current layout
            Layout* layout = mLayouts[i];

            // check if the this is the layout
            if (AzFramework::StringFunc::Equal(layoutName.c_str(), layout->GetName(), false /* no case */))
            {
                return layout;
            }
        }

        // return the first layout in case it wasn't found
        if (mLayouts.GetIsEmpty() == false)
        {
            return mLayouts[0];
        }

        return nullptr;
    }


    void RenderPlugin::LayoutButtonPressed(const QString& text)
    {
        AZStd::string pressedButtonText = FromQtString(text);
        Layout* layout = FindLayoutByName(pressedButtonText);
        if (layout == nullptr)
        {
            return;
        }

        // save the current settings and disable rendering
        mRenderOptions.SetLastUsedLayout(layout->GetName());
        ClearViewWidgets();
        VisibilityChanged(false);

        mCurrentLayout = layout;

        QWidget* oldLayoutWidget = mRenderLayoutWidget;
        QWidget* newLayoutWidget = layout->Create(this, mInnerWidget);

        // delete the old render layout after we created the new one, so we can keep the old resources
        // this only removes it from the layout
        mBaseLayout->removeWidget(oldLayoutWidget);

        mRenderLayoutWidget = newLayoutWidget;

        // create thw new one and add it to the base layout
        mBaseLayout->addWidget(mRenderLayoutWidget);
        mRenderLayoutWidget->update();
        mBaseLayout->update();
        mRenderLayoutWidget->show();

        LoadRenderOptions();
        ViewCloseup(false, nullptr, 0.0f);

        // get rid of the layout widget
        if (oldLayoutWidget)
        {
            oldLayoutWidget->hide();
            oldLayoutWidget->deleteLater();
        }

        VisibilityChanged(true);
    }


    RenderViewWidget* RenderPlugin::CreateViewWidget(QWidget* parent)
    {
        // create a new OpenGL widget
        RenderViewWidget* viewWidget = new RenderViewWidget(this, parent);
        mViewWidgets.Add(viewWidget);
        return viewWidget;
    }


    // register keyboard shortcuts used for the render plugin
    void RenderPlugin::RegisterKeyboardShortcuts()
    {
        MysticQt::KeyboardShortcutManager* shortcutManger = GetMainWindow()->GetShortcutManager();

        shortcutManger->RegisterKeyboardShortcut("Show Selected", "Render Window", Qt::Key_S, false, false, true);
        shortcutManger->RegisterKeyboardShortcut("Show Entire Scene", "Render Window", Qt::Key_A, false, false, true);
        shortcutManger->RegisterKeyboardShortcut("Toggle Selection Box Rendering", "Render Window", Qt::Key_J, false, false, true);
    }


    // find the trajectory path for a given actor instance
    MCommon::RenderUtil::TrajectoryTracePath* RenderPlugin::FindTracePath(EMotionFX::ActorInstance* actorInstance)
    {
        // get the number of trace paths, iterate through them and find the one for the given actor instance
        const uint32 numPaths = mTrajectoryTracePaths.GetLength();
        for (uint32 i = 0; i < numPaths; ++i)
        {
            if (mTrajectoryTracePaths[i]->mActorInstance == actorInstance)
            {
                return mTrajectoryTracePaths[i];
            }
        }

        // we haven't created a path for the given actor instance yet, do so
        MCommon::RenderUtil::TrajectoryTracePath* tracePath = new MCommon::RenderUtil::TrajectoryTracePath();

        tracePath->mActorInstance = actorInstance;
        tracePath->mTraceParticles.Reserve(512);

        mTrajectoryTracePaths.Add(tracePath);
        return tracePath;
    }


    // reset the trajectory paths for all selected actor instances
    void RenderPlugin::ResetSelectedTrajectoryPaths()
    {
        // get the current selection
        CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();

        // iterate through the actor instances and reset their trajectory path
        for (uint32 i = 0; i < numSelectedActorInstances; ++i)
        {
            // get the actor instance and find the corresponding trajectory path
            EMotionFX::ActorInstance*                   actorInstance   = selectionList.GetActorInstance(i);
            MCommon::RenderUtil::TrajectoryTracePath*   trajectoryPath  = FindTracePath(actorInstance);

            // reset the trajectory path
            MCommon::RenderUtil::ResetTrajectoryPath(trajectoryPath);
        }
    }


    // update a visible character
    void RenderPlugin::UpdateActorInstance(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds)
    {
        // find the corresponding trajectory trace path for the given actor instance
        MCommon::RenderUtil::TrajectoryTracePath* trajectoryPath = FindTracePath(actorInstance);
        if (trajectoryPath)
        {
            const EMotionFX::Actor* actor = actorInstance->GetActor();
            const EMotionFX::Node* motionExtractionNode = actor->GetMotionExtractionNode();
            if (motionExtractionNode)
            {
                const MCore::Matrix worldTM = actorInstance->GetWorldSpaceTransform().ToMatrix();

                bool distanceTraveledEnough = false;
                if (trajectoryPath->mTraceParticles.GetIsEmpty())
                {
                    distanceTraveledEnough = true;
                }
                else
                {
                    const uint32 numParticles = trajectoryPath->mTraceParticles.GetLength();
                    const MCore::Matrix& oldWorldTM = trajectoryPath->mTraceParticles[numParticles - 1].mWorldTM;

                    const AZ::Vector3& oldPos = oldWorldTM.GetTranslation();
                    const MCore::Quaternion oldRot(oldWorldTM.Normalized());
                    const MCore::Quaternion rotation(worldTM.Normalized());

                    const AZ::Vector3 deltaPos = worldTM.GetTranslation() - oldPos;
                    const float deltaRot = MCore::Math::Abs(rotation.Dot(oldRot));
                    if (MCore::SafeLength(deltaPos) > 0.0001f || deltaRot < 0.99f)
                    {
                        distanceTraveledEnough = true;
                    }
                }

                // add the time delta to the time passed since the last add
                trajectoryPath->mTimePassed += timePassedInSeconds;

                const uint32 particleSampleRate = 30;
                if (trajectoryPath->mTimePassed >= (1.0f / particleSampleRate) && distanceTraveledEnough)
                {
                    // create the particle, fill its data and add it to the trajectory trace path
                    MCommon::RenderUtil::TrajectoryPathParticle trajectoryParticle;
                    trajectoryParticle.mWorldTM = worldTM;
                    trajectoryPath->mTraceParticles.Add(trajectoryParticle);

                    // reset the time passed as we just added a new particle
                    trajectoryPath->mTimePassed = 0.0f;
                }
            }

            // make sure we don't have too many items in our array
            if (trajectoryPath->mTraceParticles.GetLength() > 50)
            {
                trajectoryPath->mTraceParticles.RemoveFirst();
            }
        }
    }


    // update a visible character
    void RenderPlugin::RenderActorInstance(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds)
    {
        MCORE_UNUSED(timePassedInSeconds);
        RenderPlugin::EMStudioRenderActor* emstudioActor = FindEMStudioActor(actorInstance);
        if (emstudioActor == nullptr)
        {
            return;
        }

        // renderUtil options
        MCommon::RenderUtil* renderUtil = GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        // get the active widget & it's rendering options
        RenderViewWidget*   widget          = GetActiveViewWidget();
        RenderOptions*      renderOptions   = GetRenderOptions();

        const AZStd::unordered_set<AZ::u32>& visibleJointIndices = GetManager()->GetVisibleJointIndices();
        const AZStd::unordered_set<AZ::u32>& selectedJointIndices = GetManager()->GetSelectedJointIndices();

        // render the AABBs
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_AABB))
        {
            MCommon::RenderUtil::AABBRenderSettings settings;
            settings.mNodeBasedColor          = renderOptions->GetNodeAABBColor();
            settings.mStaticBasedColor        = renderOptions->GetStaticAABBColor();
            settings.mMeshBasedColor          = renderOptions->GetMeshAABBColor();
            settings.mCollisionMeshBasedColor = renderOptions->GetCollisionMeshAABBColor();

            renderUtil->RenderAABBs(actorInstance, settings);
        }

        if (widget->GetRenderFlag(RenderViewWidget::RENDER_OBB))
        {
            renderUtil->RenderOBBs(actorInstance, &visibleJointIndices, &selectedJointIndices, renderOptions->GetOBBsColor(), renderOptions->GetSelectedObjectColor());
        }
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_LINESKELETON))
        {
            const MCommon::Camera* camera = widget->GetRenderWidget()->GetCamera();
            const AZ::Vector3& cameraPos = camera->GetPosition();

            MCore::AABB aabb;
            actorInstance->CalcNodeBasedAABB(&aabb);
            const AZ::Vector3 aabbMid = aabb.CalcMiddle();
            const float aabbRadius = aabb.CalcRadius();
            const float camDistance = fabs((cameraPos - aabbMid).GetLength());

            // Avoid rendering too big joint spheres when zooming in onto a joint.
            float scaleMultiplier = 1.0f;
            if (camDistance < aabbRadius)
            {
                scaleMultiplier = camDistance / aabbRadius;
            }

            // Scale the joint spheres based on the character's extents, to avoid really large joint spheres
            // on small characters and too small spheres on large characters.
            static const float baseRadius = 0.005f;
            const float jointSphereRadius = aabb.CalcRadius() * scaleMultiplier * baseRadius;

            renderUtil->RenderSimpleSkeleton(actorInstance, &visibleJointIndices, &selectedJointIndices,
                renderOptions->GetLineSkeletonColor(), renderOptions->GetSelectedObjectColor(), jointSphereRadius);
        }

        bool cullingEnabled = renderUtil->GetCullingEnabled();
        bool lightingEnabled = renderUtil->GetLightingEnabled();
        renderUtil->EnableCulling(false); // disable culling
        renderUtil->EnableLighting(false); // disable lighting
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_SKELETON))
        {
            renderUtil->RenderSkeleton(actorInstance, emstudioActor->mBoneList, &visibleJointIndices, &selectedJointIndices, renderOptions->GetSkeletonColor(), renderOptions->GetSelectedObjectColor());
        }
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_NODEORIENTATION))
        {
            renderUtil->RenderNodeOrientations(actorInstance, emstudioActor->mBoneList, &visibleJointIndices, &selectedJointIndices, emstudioActor->mNormalsScaleMultiplier * renderOptions->GetNodeOrientationScale(), renderOptions->GetScaleBonesOnLength());
        }
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_ACTORBINDPOSE))
        {
            renderUtil->RenderBindPose(actorInstance);
        }

        // render motion extraction debug info
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_MOTIONEXTRACTION))
        {
            // render an arrow for the trajectory
            renderUtil->RenderTrajectoryPath(FindTracePath(actorInstance), renderOptions->GetTrajectoryArrowInnerColor(), emstudioActor->mCharacterHeight * 0.05f);
        }
        renderUtil->EnableCulling(cullingEnabled); // reset to the old state
        renderUtil->EnableLighting(lightingEnabled);

        const bool renderVertexNormals  = widget->GetRenderFlag(RenderViewWidget::RENDER_VERTEXNORMALS);
        const bool renderFaceNormals    = widget->GetRenderFlag(RenderViewWidget::RENDER_FACENORMALS);
        const bool renderTangents       = widget->GetRenderFlag(RenderViewWidget::RENDER_TANGENTS);
        const bool renderWireframe      = widget->GetRenderFlag(RenderViewWidget::RENDER_WIREFRAME);
        const bool renderCollisionMeshes= widget->GetRenderFlag(RenderViewWidget::RENDER_COLLISIONMESHES);

        const EMotionFX::Actor* actor = actorInstance->GetActor();

        if (renderVertexNormals || renderFaceNormals || renderTangents || renderWireframe || renderCollisionMeshes)
        {
            // iterate through all enabled nodes
            const EMotionFX::Pose* pose = actorInstance->GetTransformData()->GetCurrentPose();
            const MCore::Matrix actorInstanceWorldTM = actorInstance->GetWorldSpaceTransform().ToMatrix();

            const uint32 geomLODLevel   = actorInstance->GetLODLevel();
            const uint32 numEnabled     = actorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numEnabled; ++i)
            {
                EMotionFX::Node*    node      = emstudioActor->mActor->GetSkeleton()->GetNode(actorInstance->GetEnabledNode(i));
                const AZ::u32       nodeIndex = node->GetNodeIndex();
                EMotionFX::Mesh*    mesh      = emstudioActor->mActor->GetMesh(geomLODLevel, nodeIndex);

                renderUtil->ResetCurrentMesh();

                if (!mesh)
                {
                    continue;
                }

                const MCore::Matrix worldTM = pose->GetMeshNodeWorldSpaceTransform(geomLODLevel, nodeIndex).ToMatrix();

                if (!mesh->GetIsCollisionMesh())
                {
                    renderUtil->RenderNormals(mesh, worldTM, renderVertexNormals, renderFaceNormals, renderOptions->GetVertexNormalsScale() * emstudioActor->mNormalsScaleMultiplier, renderOptions->GetFaceNormalsScale() * emstudioActor->mNormalsScaleMultiplier, renderOptions->GetVertexNormalsColor(), renderOptions->GetFaceNormalsColor());
                    if (renderTangents)
                    {
                        renderUtil->RenderTangents(mesh, worldTM, renderOptions->GetTangentsScale() * emstudioActor->mNormalsScaleMultiplier, renderOptions->GetTangentsColor(), renderOptions->GetMirroredBitangentsColor(), renderOptions->GetBitangentsColor());
                    }
                    if (renderWireframe)
                    {
                        renderUtil->RenderWireframe(mesh, worldTM, renderOptions->GetWireframeColor(), false, emstudioActor->mNormalsScaleMultiplier);
                    }
                }
                else
                if (renderCollisionMeshes)
                {
                    renderUtil->RenderWireframe(mesh, worldTM, renderOptions->GetCollisionMeshColor(), false, emstudioActor->mNormalsScaleMultiplier);
                }
            }
        }

        // render the selection
        if (renderOptions->GetRenderSelectionBox() && EMotionFX::GetActorManager().GetNumActorInstances() != 1 && GetCurrentSelection()->CheckIfHasActorInstance(actorInstance))
        {
            MCore::AABB aabb = actorInstance->GetAABB();
            aabb.Widen(aabb.CalcRadius() * 0.005f);
            renderUtil->RenderSelection(aabb, renderOptions->GetSelectionColor());
        }

        // render node names
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_NODENAMES))
        {
            MCommon::Camera*    camera          = widget->GetRenderWidget()->GetCamera();
            const uint32        screenWidth     = widget->GetRenderWidget()->GetScreenWidth();
            const uint32        screenHeight    = widget->GetRenderWidget()->GetScreenHeight();

            renderUtil->RenderNodeNames(actorInstance, camera, screenWidth, screenHeight, renderOptions->GetNodeNameColor(), renderOptions->GetSelectedObjectColor(), visibleJointIndices, selectedJointIndices);
        }
    }


    void RenderPlugin::ResetCameras()
    {
        const uint32 numViews = GetNumViewWidgets();
        for (uint32 i = 0; i < numViews; ++i)
        {
            RenderViewWidget* viewWidget = GetViewWidget(i);
            viewWidget->OnResetCamera();
        }
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderPlugin.moc>
