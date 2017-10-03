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

#ifndef __EMSTUDIO_RENDERPLUGIN_H
#define __EMSTUDIO_RENDERPLUGIN_H

// include MCore
#include <MCore/Source/StandardHeaders.h>
#include "../EMStudioConfig.h"
#include "../DockWidgetPlugin.h"
#include "RenderOptions.h"
#include "RenderWidget.h"
#include "RenderUpdateCallback.h"
#include "RenderViewWidget.h"
#include "../PreferencesWindow.h"
#include <EMotionFX/Rendering/Common/Camera.h>
#include <EMotionFX/Rendering/Common/TranslateManipulator.h>
#include <EMotionFX/Rendering/Common/RotateManipulator.h>
#include <EMotionFX/Rendering/Common/ScaleManipulator.h>
#include <EMotionFX/Rendering/OpenGL2/Source/glactor.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/CommandSystem/Source/ImporterCommands.h>
#include <QWidget>
#include <QSignalMapper>


namespace EMStudio
{
#define DEFAULT_FLIGHT_TIME 1.0f

    class EMSTUDIO_API RenderPlugin
        : public DockWidgetPlugin
    {
        MCORE_MEMORYOBJECTCATEGORY(RenderPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE)
        Q_OBJECT

    public:
        enum
        {
            CLASS_ID = 0xa83f74a2
        };

        struct EMSTUDIO_API EMStudioRenderActor
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderPlugin::EMStudioRenderActor, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

            EMotionFX::Actor*                           mActor;
            MCore::Array<uint32>                        mBoneList;
            RenderGL::GLActor*                          mRenderActor;
            MCore::Array<EMotionFX::ActorInstance*>     mActorInstances;
            //MCore::AABB                               mMeshAABB;
            //MCore::AABB                               mCollisionMeshAABB;
            //MCore::AABB                               mNodeAABB;
            float                                       mNormalsScaleMultiplier;
            float                                       mCharacterHeight;
            float                                       mOffsetFromTrajectoryNode;
            MCore::Array<MCore::Matrix>                 mBindPoseGlobalMatrices;
            bool                                        mMustCalcNormalScale;

            EMStudioRenderActor(EMotionFX::Actor* actor, RenderGL::GLActor* renderActor);
            virtual ~EMStudioRenderActor();

            void CalculateNormalScaleMultiplier();
        };

        class Layout
        {
            MCORE_MEMORYOBJECTCATEGORY(RenderPlugin::Layout, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

        public:
            Layout() { mRenderPlugin = nullptr; }
            virtual ~Layout() { }
            virtual QWidget* Create(RenderPlugin* renderPlugin, QWidget* parent) = 0;
            virtual const char* GetName() = 0;
            virtual const char* GetImageFileName() = 0;

        private:
            RenderPlugin* mRenderPlugin;
        };

        enum EMode
        {
            MODE_SELECTION              = 0,
            MODE_TRANSLATIONMANIPULATOR = 1,
            MODE_ROTATIONMANIPULATOR    = 2,
            MODE_SCALEMANIPULATOR       = 3
        };

        RenderPlugin();
        virtual ~RenderPlugin();

        bool Init() override;
        void OnAfterLoadProject() override;

        // callback functions
        virtual void CreateRenderWidget(RenderViewWidget* renderViewWidget, RenderWidget** outRenderWidget, QWidget** outWidget) = 0;
        virtual bool CreateEMStudioActor(EMotionFX::Actor* actor) = 0;

        EMStudioPlugin::EPluginType GetPluginType() const override              { return EMStudioPlugin::PLUGINTYPE_RENDERING; }
        uint32 GetProcessFramePriority() const override                         { return 100; }

        void AddSettings(PreferencesWindow* preferencesWindow) override;

        // render actors
        EMStudioRenderActor* FindEMStudioActor(EMotionFX::ActorInstance* actorInstance, bool doubleCheckInstance = true);
        EMStudioRenderActor* FindEMStudioActor(EMotionFX::Actor* actor);
        uint32 FindEMStudioActorIndex(EMStudioRenderActor* EMStudioRenderActor);

        void AddEMStudioActor(EMStudioRenderActor* emstudioActor);
        bool DestroyEMStudioActor(EMotionFX::Actor* actor);

        // reinitializes the render actors
        void ReInit(bool resetViewCloseup = true);

        // view close up helpers
        void ViewCloseup(bool selectedInstancesOnly = true, RenderWidget* renderWidget = nullptr, float flightTime = DEFAULT_FLIGHT_TIME);
        void SetSkipFollowCalcs(bool skipFollowCalcs);

        // keyboard shortcuts
        void RegisterKeyboardShortcuts() override;

        // manipulators
        void ReInitTransformationManipulators();
        MCommon::TransformationManipulator* GetActiveManipulator(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY);
        MCORE_INLINE MCommon::TranslateManipulator* GetTranslateManipulator()       { return mTranslateManipulator; }
        MCORE_INLINE MCommon::RotateManipulator* GetRotateManipulator()             { return mRotateManipulator; }
        MCORE_INLINE MCommon::ScaleManipulator* GetScaleManipulator()               { return mScaleManipulator; }

        // other helpers
        MCORE_INLINE RenderOptions* GetRenderOptions()                              { return &mRenderOptions; }
        MCORE_INLINE EMode GetMode()                                                { return mMode; }

        // view widget helpers
        MCORE_INLINE RenderViewWidget* GetFocusViewWidget()                         { return mFocusViewWidget; }
        MCORE_INLINE void SetFocusViewWidget(RenderViewWidget* focusViewWidget)     { mFocusViewWidget = focusViewWidget; }

        MCORE_INLINE RenderViewWidget* GetViewWidget(uint32 index)                  { return mViewWidgets[index]; }
        MCORE_INLINE uint32 GetNumViewWidgets()                                     { return mViewWidgets.GetLength(); }
        RenderViewWidget* CreateViewWidget(QWidget* parent);
        void RemoveViewWidget(RenderViewWidget* viewWidget);
        void ClearViewWidgets();

        MCORE_INLINE RenderViewWidget* GetActiveViewWidget()                        { return mActiveViewWidget; }
        MCORE_INLINE void SetActiveViewWidget(RenderViewWidget* viewWidget)         { mActiveViewWidget = viewWidget; }

        MCORE_INLINE void AddLayout(Layout* layout)                                 { mLayouts.Add(layout); }
        Layout* FindLayoutByName(const MCore::String& layoutName);

        MCORE_INLINE QCursor& GetZoomInCursor()                                     { assert(mZoomInCursor); return *mZoomInCursor; }
        MCORE_INLINE QCursor& GetZoomOutCursor()                                    { assert(mZoomOutCursor); return *mZoomOutCursor; }

        MCORE_INLINE CommandSystem::SelectionList* GetCurrentSelection() const      { return mCurrentSelection; }
        MCORE_INLINE MCommon::RenderUtil* GetRenderUtil() const                     { return mRenderUtil; }

        MCore::AABB GetSceneAABB(bool selectedInstancesOnly);

        MCommon::RenderUtil::TrajectoryTracePath* FindTracePath(EMotionFX::ActorInstance* actorInstance);
        void ResetSelectedTrajectoryPaths();

        void ProcessFrame(float timePassedInSeconds) override;

        void UpdateActorInstances(float timePassedInSeconds);

        virtual void UpdateActorInstance(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds);
        virtual void RenderActorInstance(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds);
        virtual void ResetCameras();

        void SaveRenderOptions();
        void LoadRenderOptions();

    public slots:
        void SetSelectionMode();
        void SetTranslationMode();
        void SetRotationMode();
        void SetScaleMode();

        void CleanEMStudioActors();

        void VisibilityChanged(bool visible);

        void OnValueChanged(MysticQt::PropertyWidget::Property* property);

        void LayoutButtonPressed(const QString& text);

    protected:
        QPushButton* AddLayoutButton(const char* imageFileName);
        //void SetAspiredRenderingFPS(int32 fps);

        // motion extraction paths
        MCore::Array< MCommon::RenderUtil::TrajectoryTracePath* >   mTrajectoryTracePaths;

        // the transformation manipulators
        MCommon::TranslateManipulator*      mTranslateManipulator;
        MCommon::RotateManipulator*         mRotateManipulator;
        MCommon::ScaleManipulator*          mScaleManipulator;

        MCommon::RenderUtil*                mRenderUtil;
        RenderUpdateCallback*               mUpdateCallback;

        EMode                               mMode;
        RenderOptions                       mRenderOptions;
        MCore::Array<EMStudioRenderActor*>  mActors;

        // view widgets
        MCore::Array<RenderViewWidget*>     mViewWidgets;
        RenderViewWidget*                   mActiveViewWidget;
        RenderViewWidget*                   mFocusViewWidget;

        // render view layouts
        MCore::Array<Layout*>               mLayouts;
        Layout*                             mCurrentLayout;

        // cursor image files
        QCursor*                            mZoomInCursor;
        QCursor*                            mZoomOutCursor;
        QCursor*                            mTranslateCursor;
        QCursor*                            mRotateCursor;
        QCursor*                            mNotAllowedCursor;

        // transformation manipulator buttons
        QPushButton*                        mSelectionModeButton;
        QPushButton*                        mTranslationModeButton;
        QPushButton*                        mRotationModeButton;
        QPushButton*                        mScaleModeButton;

        // window visibility
        bool                                mIsVisible;

        // base layout and interface functionality
        QHBoxLayout*                        mBaseLayout;
        QWidget*                            mToolbarWidget;
        QVBoxLayout*                        mToolbarLayout;
        QWidget*                            mRenderLayoutWidget;
        QSignalMapper*                      mSignalMapper;
        QWidget*                            mInnerWidget;
        CommandSystem::SelectionList*           mCurrentSelection;
        bool                                mFirstFrameAfterReInit;

        MysticQt::PropertyWidget::Property*     mGridUnitSizeProperty;
        MysticQt::PropertyWidget::Property*     mVertexNormalScaleProperty;
        MysticQt::PropertyWidget::Property*     mFaceNormalProperty;
        MysticQt::PropertyWidget::Property*     mTangentScaleProperty;
        MysticQt::PropertyWidget::Property*     mNodeOrientScaleProperty;
        MysticQt::PropertyWidget::Property*     mScaleBonesOnLengthProperty;
        MysticQt::PropertyWidget::Property*     mRenderBonesOnlyProperty;
        MysticQt::PropertyWidget::Property*     mNearClipPlaneDistProperty;
        MysticQt::PropertyWidget::Property*     mFarClipPlaneDistProperty;
        MysticQt::PropertyWidget::Property*     mFOVProperty;
        MysticQt::PropertyWidget::Property*     mTexturePathProperty;
        MysticQt::PropertyWidget::Property*     mAutoMipMapProperty;
        MysticQt::PropertyWidget::Property*     mSkipLoadTexturesProperty;
        MysticQt::PropertyWidget::Property*     mMainLightIntensityProperty;
        MysticQt::PropertyWidget::Property*     mMainLightAngleAProperty;
        MysticQt::PropertyWidget::Property*     mMainLightAngleBProperty;
        MysticQt::PropertyWidget::Property*     mSpecularIntensityProperty;
        MysticQt::PropertyWidget::Property*     mRimIntensityProperty;
        MysticQt::PropertyWidget::Property*     mRimWidthProperty;
        MysticQt::PropertyWidget::Property*     mRimAngleProperty;
        MysticQt::PropertyWidget::Property*     mShowFPSProperty;

        MysticQt::PropertyWidget::Property*     mGroundLightColorProperty;
        MysticQt::PropertyWidget::Property*     mSkyLightColorProperty;
        MysticQt::PropertyWidget::Property*     mRimLightColorProperty;
        MysticQt::PropertyWidget::Property*     mBGColorProperty;
        MysticQt::PropertyWidget::Property*     mGradientBGTopColorProperty;
        MysticQt::PropertyWidget::Property*     mGradientBGBottomColorProperty;
        MysticQt::PropertyWidget::Property*     mWireframeColorProperty;
        MysticQt::PropertyWidget::Property*     mColMeshColorProperty;
        MysticQt::PropertyWidget::Property*     mVertexNormalColorProperty;
        MysticQt::PropertyWidget::Property*     mFaceNormalColorProperty;
        MysticQt::PropertyWidget::Property*     mTangentColorProperty;
        MysticQt::PropertyWidget::Property*     mMirrorBinormalColorProperty;
        MysticQt::PropertyWidget::Property*     mBinormalColorProperty;
        MysticQt::PropertyWidget::Property*     mNodeAABBColorProperty;
        MysticQt::PropertyWidget::Property*     mStaticAABBColorProperty;
        MysticQt::PropertyWidget::Property*     mMeshAABBColorProperty;
        MysticQt::PropertyWidget::Property*     mColMeshAABBColorProperty;
        MysticQt::PropertyWidget::Property*     mNodeOOBColorProperty;
        MysticQt::PropertyWidget::Property*     mLineSkeletonColorProperty;
        MysticQt::PropertyWidget::Property*     mSkeletonColorProperty;
        MysticQt::PropertyWidget::Property*     mGizmoColorProperty;
        MysticQt::PropertyWidget::Property*     mNodeNameColorProperty;
        MysticQt::PropertyWidget::Property*     mGridColorProperty;
        MysticQt::PropertyWidget::Property*     mGridMainAxisColorProperty;
        MysticQt::PropertyWidget::Property*     mGridSubstepColorProperty;
        MysticQt::PropertyWidget::Property*     mTrajectoryPathColorProperty;

        MysticQt::PropertyWidget::Property*     mEnableAdvRenderingProperty;
        MysticQt::PropertyWidget::Property*     mBloomingEnabledProperty;
        MysticQt::PropertyWidget::Property*     mBloomThresholdProperty;
        MysticQt::PropertyWidget::Property*     mBloomIntensity;
        MysticQt::PropertyWidget::Property*     mBloomRadius;

        // command callbacks
        MCORE_DEFINECOMMANDCALLBACK(UpdateRenderActorsCallback);
        MCORE_DEFINECOMMANDCALLBACK(ReInitRenderActorsCallback);
        MCORE_DEFINECOMMANDCALLBACK(CreateActorInstanceCallback);
        MCORE_DEFINECOMMANDCALLBACK(RemoveActorInstanceCallback);
        MCORE_DEFINECOMMANDCALLBACK(SelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(UnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(ClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandResetToBindPoseCallback);
        MCORE_DEFINECOMMANDCALLBACK(AdjustActorInstanceCallback);
        UpdateRenderActorsCallback*         mUpdateRenderActorsCallback;
        ReInitRenderActorsCallback*         mReInitRenderActorsCallback;
        CreateActorInstanceCallback*        mCreateActorInstanceCallback;
        RemoveActorInstanceCallback*        mRemoveActorInstanceCallback;
        SelectCallback*                     mSelectCallback;
        UnselectCallback*                   mUnselectCallback;
        ClearSelectionCallback*             mClearSelectionCallback;
        CommandResetToBindPoseCallback*     mResetToBindPoseCallback;
        AdjustActorInstanceCallback*        mAdjustActorInstanceCallback;
    };
} // namespace EMStudio

#endif
