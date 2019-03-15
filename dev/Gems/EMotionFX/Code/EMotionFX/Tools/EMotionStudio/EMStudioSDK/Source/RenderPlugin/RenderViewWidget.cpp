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

#include "RenderViewWidget.h"
#include "RenderPlugin.h"
#include "../EMStudioCore.h"
#include "../PreferencesWindow.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include <QMenuBar>


namespace EMStudio
{
    // constructor
    RenderViewWidget::RenderViewWidget(RenderPlugin* parentPlugin, QWidget* parentWidget)
        : QWidget(parentWidget)
    {
        for (uint32 i = 0; i < NUM_RENDER_OPTIONS; ++i)
        {
            mToolbarButtons[i] = nullptr;
            mActions[i] = nullptr;
        }

        mRenderOptionsWindow    = nullptr;
        mPlugin                 = parentPlugin;

        // create the vertical layout with the menu and the gl widget as entries
        QVBoxLayout* verticalLayout = new QVBoxLayout(this);
        verticalLayout->setSizeConstraint(QLayout::SetNoConstraint);
        verticalLayout->setSpacing(1);
        verticalLayout->setMargin(0);

        // create the top widget with the menu and the toolbar
        QWidget* topWidget = new QWidget();
        topWidget->setMinimumHeight(23); // menu fix (to get it working with the Ly style)
        QHBoxLayout* horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSizeConstraint(QLayout::SetMaximumSize);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setMargin(0);
        topWidget->setLayout(horizontalLayout);

        // create menu
        mMenu = new QMenuBar(this);
        mMenu->setStyleSheet("QMenuBar { min-height: 5px;}"); // menu fix (to get it working with the Ly style)
        mMenu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        mMenu->setNativeMenuBar(false);
        horizontalLayout->addWidget(mMenu);

        // create the toolbar
        QWidget* toolbarWidget = new QWidget();
        mToolbarLayout = new QHBoxLayout();
        mToolbarLayout->setSizeConstraint(QLayout::SetFixedSize);
        mToolbarLayout->setSpacing(0);
        mToolbarLayout->setMargin(0);
        toolbarWidget->setLayout(mToolbarLayout);

        horizontalLayout->addWidget(toolbarWidget);

        // add the top widget with the menu and toolbar and the gl widget to the vertical layout
        verticalLayout->addWidget(topWidget);

        QWidget* renderWidget = nullptr;
        mPlugin->CreateRenderWidget(this, &mRenderWidget, &renderWidget);
        verticalLayout->addWidget(renderWidget);

        QMenu* viewMenu = mMenu->addMenu(tr("&View"));

        CreateEntry(viewMenu, "Solid",                  "Solid.png",            RENDER_SOLID);
        CreateEntry(viewMenu, "Wireframe",              "Wireframe.png",        RENDER_WIREFRAME);
        viewMenu->addSeparator();
        CreateEntry(viewMenu, "Texturing",              "Texturing.png",        RENDER_TEXTURING);
        CreateEntry(viewMenu, "Lighting",               "Lighting.png",         RENDER_LIGHTING);
        CreateEntry(viewMenu, "Shadows",                "Shadows.png",          RENDER_SHADOWS, false);
        CreateEntry(viewMenu, "Backface Culling",       "BackfaceCulling.png",  RENDER_BACKFACECULLING);
        viewMenu->addSeparator();
        CreateEntry(viewMenu, "Vertex Normals",         "VertexNormals.png",    RENDER_VERTEXNORMALS);
        CreateEntry(viewMenu, "Face Normals",           "FaceNormals.png",      RENDER_FACENORMALS);
        CreateEntry(viewMenu, "Tangents",               "Tangents.png",         RENDER_TANGENTS);
        viewMenu->addSeparator();
        CreateEntry(viewMenu, "Actor Bounding Boxes",   "BoundingBoxes.png",    RENDER_AABB);
        CreateEntry(viewMenu, "Node OBBs",              "OBBs.png",             RENDER_OBB);
        CreateEntry(viewMenu, "Collision Meshes",       "CollisionMeshes.png",  RENDER_COLLISIONMESHES);
        CreateEntry(viewMenu, "Ragdoll Colliders",      QIcon(":/EMotionFX/RagdollCollider_Orange.png"), true,      RENDER_RAGDOLL_COLLIDERS);
        CreateEntry(viewMenu, "Ragdoll Joint Limits",   QIcon(":/EMotionFX/RagdollJointLimit_Orange.png"), true,    RENDER_RAGDOLL_JOINTLIMITS);
        CreateEntry(viewMenu, "Hit Detection Colliders",QIcon(":/EMotionFX/HitDetection_Blue.png"), true,           RENDER_HITDETECTION_COLLIDERS);
        // Note: Cloth collider editor is disabled as it is in preview
        CreateEntry(viewMenu, "Cloth Colliders",        QIcon(":/EMotionFX/ClothCollider_Purple.png"), true,        RENDER_CLOTH_COLLIDERS, /*isVisible*/false);
        viewMenu->addSeparator();
        CreateEntry(viewMenu, "Skeleton",               "Skeleton.png",         RENDER_SKELETON);
        CreateEntry(viewMenu, "Line Skeleton",          "SkeletonLines.png",    RENDER_LINESKELETON);
        CreateEntry(viewMenu, "Node Orientations",      "NodeOrientations.png", RENDER_NODEORIENTATION);
        CreateEntry(viewMenu, "Node Names",             "NodeNames.png",        RENDER_NODENAMES);
        CreateEntry(viewMenu, "Actor Bind Pose",        "",                     RENDER_ACTORBINDPOSE);  // don't add to the toolbar
        CreateEntry(viewMenu, "Motion Extraction",      "",                     RENDER_MOTIONEXTRACTION);// don't add to the toolbar
        viewMenu->addSeparator();
        CreateEntry(viewMenu, "Grid",                   "Grid.png",             RENDER_GRID);
        CreateEntry(viewMenu, "Gradient Background",    "",                     RENDER_USE_GRADIENTBACKGROUND);// don't add to the toolbar

        viewMenu->addSeparator();
        viewMenu->addAction("Reset", this, &RenderViewWidget::OnReset);

        viewMenu->addSeparator();
        viewMenu->addAction("Render Options", this, &RenderViewWidget::OnOptions);

        // the cameras menu
        QMenu* cameraMenu = mMenu->addMenu(tr("&Camera"));
        mCameraMenu = cameraMenu;

        cameraMenu->addAction("Perspective",       this, &RenderViewWidget::OnOrbitCamera);
        cameraMenu->addAction("Front",             this, &RenderViewWidget::OnOrthoFrontCamera);
        cameraMenu->addAction("Back",              this, &RenderViewWidget::OnOrthoBackCamera);
        cameraMenu->addAction("Left",              this, &RenderViewWidget::OnOrthoLeftCamera);
        cameraMenu->addAction("Right",             this, &RenderViewWidget::OnOrthoRightCamera);
        cameraMenu->addAction("Top",               this, &RenderViewWidget::OnOrthoTopCamera);
        cameraMenu->addAction("Bottom",            this, &RenderViewWidget::OnOrthoBottomCamera);
        cameraMenu->addSeparator();
        cameraMenu->addAction("Reset Camera",      [this]() { this->OnResetCamera(); });
        cameraMenu->addAction("Show Selected",     this, &RenderViewWidget::OnShowSelected);
        cameraMenu->addAction("Show Entire Scene", this, &RenderViewWidget::OnShowEntireScene);
        cameraMenu->addSeparator();

        mFollowCharacterAction = cameraMenu->addAction(tr("Follow Character"));
        mFollowCharacterAction->setCheckable(true);
        mFollowCharacterAction->setChecked(true);
        //mFollowCharacterAction->setChecked(IsCharacterFollowModeActive());
        connect(mFollowCharacterAction, &QAction::triggered, this, &RenderViewWidget::OnFollowCharacter);

        Reset();
    }


    void RenderViewWidget::Reset()
    {
        SetRenderFlag(RENDER_SOLID, true);
        SetRenderFlag(RENDER_WIREFRAME, false);

        SetRenderFlag(RENDER_LIGHTING, true);
        SetRenderFlag(RENDER_TEXTURING, true);
        SetRenderFlag(RENDER_SHADOWS, true);

        SetRenderFlag(RENDER_VERTEXNORMALS, false);
        SetRenderFlag(RENDER_FACENORMALS, false);
        SetRenderFlag(RENDER_TANGENTS, false);

        SetRenderFlag(RENDER_AABB, false);
        SetRenderFlag(RENDER_OBB, false);
        SetRenderFlag(RENDER_COLLISIONMESHES, false);
        SetRenderFlag(RENDER_RAGDOLL_COLLIDERS, true);
        SetRenderFlag(RENDER_RAGDOLL_JOINTLIMITS, true);
        SetRenderFlag(RENDER_HITDETECTION_COLLIDERS, true);
        SetRenderFlag(RENDER_CLOTH_COLLIDERS, true);

        SetRenderFlag(RENDER_SKELETON, false);
        SetRenderFlag(RENDER_LINESKELETON, false);
        SetRenderFlag(RENDER_NODEORIENTATION, false);
        SetRenderFlag(RENDER_NODENAMES, false);
        SetRenderFlag(RENDER_ACTORBINDPOSE, false);
        SetRenderFlag(RENDER_MOTIONEXTRACTION, false);

        SetRenderFlag(RENDER_GRID, true);
        SetRenderFlag(RENDER_USE_GRADIENTBACKGROUND, true);
        SetRenderFlag(RENDER_BACKFACECULLING, false);
    }


    void RenderViewWidget::SetRenderFlag(ERenderFlag option, bool isEnabled)
    {
        const uint32 optionIndex = (uint32)option;

        if (mToolbarButtons[optionIndex])
        {
            mToolbarButtons[optionIndex]->setChecked(isEnabled);
        }
        if (mActions[optionIndex])
        {
            mActions[optionIndex]->setChecked(isEnabled);
        }
    }


    void RenderViewWidget::CreateEntry(QMenu* menu, const char* menuEntryName, const char* toolbarIconFileName, int32 actionIndex, bool visible)
    {
        AZStd::string iconFileName = "Images/Rendering/";
        iconFileName += toolbarIconFileName;
        const QIcon& icon = MysticQt::GetMysticQt()->FindIcon(iconFileName.c_str());

        const bool addToToolbar = strcmp(toolbarIconFileName, "") != 0;
        CreateEntry(menu, menuEntryName, icon, addToToolbar, actionIndex, visible);
    }


    void RenderViewWidget::CreateEntry(QMenu* menu, const char* menuEntryName, const QIcon& icon, bool addToToolbar, int32 actionIndex, bool visible)
    {
        // menu entry
        mActions[actionIndex] = menu->addAction(menuEntryName);
        mActions[actionIndex]->setCheckable(true);
        if (visible == false)
        {
            mActions[actionIndex]->setVisible(visible);
        }

        if (addToToolbar)
        {
            // toolbar button
            QPushButton* toolbarButton = new QPushButton();
            mToolbarButtons[actionIndex] = toolbarButton;
            toolbarButton->setIcon(icon);
            toolbarButton->setCheckable(true);
            toolbarButton->setToolTip(menuEntryName);

            if (visible == false)
            {
                toolbarButton->setVisible(visible);
            }

            QSize buttonSize = QSize(20, 20);
            toolbarButton->setMinimumSize(buttonSize);
            toolbarButton->setMaximumSize(buttonSize);
            toolbarButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

            mToolbarLayout->addWidget(toolbarButton);
            toolbarButton->setIconSize(buttonSize - QSize(2, 2));

            // connect the menu entry with the toolbar button
            connect(toolbarButton, &QPushButton::clicked, mActions[actionIndex], &QAction::trigger);
            connect(mActions[actionIndex], &QAction::toggled, this, &RenderViewWidget::UpdateToolBarButton);
        }
    }


    // destructor
    RenderViewWidget::~RenderViewWidget()
    {
        mPlugin->RemoveViewWidget(this);
    }


    // show the global rendering options dialog
    void RenderViewWidget::OnOptions()
    {
        if (mRenderOptionsWindow == nullptr)
        {
            mRenderOptionsWindow = new PreferencesWindow(this);
            mRenderOptionsWindow->Init();

            AzToolsFramework::ReflectedPropertyEditor* generalPropertyWidget = mRenderOptionsWindow->FindPropertyWidgetByName("General");
            if (!generalPropertyWidget)
            {
                generalPropertyWidget = mRenderOptionsWindow->AddCategory("General", "Images/Preferences/General.png", false);
                generalPropertyWidget->ClearInstances();
                generalPropertyWidget->InvalidateAll();
            }

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
                return;
            }

            PluginOptions* pluginOptions = mPlugin->GetOptions();
            AZ_Assert(pluginOptions, "Expected options in render plugin");
            generalPropertyWidget->AddInstance(pluginOptions, azrtti_typeid(pluginOptions));

            // Now add EMStudio settings
            generalPropertyWidget->SetAutoResizeLabels(true);
            generalPropertyWidget->Setup(serializeContext, nullptr, true);
            generalPropertyWidget->show();
            generalPropertyWidget->ExpandAll();
            generalPropertyWidget->InvalidateAll();
        }

        mRenderOptionsWindow->show();
    }


    void RenderViewWidget::OnShowSelected()
    {
        mRenderWidget->ViewCloseup(true, DEFAULT_FLIGHT_TIME);
    }


    void RenderViewWidget::OnShowEntireScene()
    {
        mRenderWidget->ViewCloseup(false, DEFAULT_FLIGHT_TIME);
    }


    void RenderViewWidget::SetCharacterFollowModeActive(bool active)
    {
        mFollowCharacterAction->setChecked(active);
    }


    void RenderViewWidget::OnFollowCharacter()
    {
        CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance* followInstance = selectionList.GetFirstActorInstance();

        if (followInstance && GetIsCharacterFollowModeActive() && mRenderWidget)
        {
            mRenderWidget->ViewCloseup(true, DEFAULT_FLIGHT_TIME, 1);
        }
    }


    void RenderViewWidget::SaveOptions(QSettings* settings)
    {
        const uint32 numRenderOptions = NUM_RENDER_OPTIONS;
        for (uint32 i = 0; i < numRenderOptions; ++i)
        {
            QString name = QString(i);
            settings->setValue(name, mActions[i]->isChecked());
        }

        settings->setValue("CameraMode", (int32)mRenderWidget->GetCameraMode());
        settings->setValue("CharacterFollowMode", GetIsCharacterFollowModeActive());
    }


    void RenderViewWidget::LoadOptions(QSettings* settings)
    {
        const uint32 numRenderOptions = NUM_RENDER_OPTIONS;
        for (uint32 i = 0; i < numRenderOptions; ++i)
        {
            QString name = QString(i);
            const bool isEnabled = settings->value(name, mActions[i]->isChecked()).toBool();
            SetRenderFlag((ERenderFlag)i, isEnabled);
        }

        RenderWidget::CameraMode cameraMode = (RenderWidget::CameraMode)settings->value("CameraMode", (int32)mRenderWidget->GetCameraMode()).toInt();
        mRenderWidget->SwitchCamera(cameraMode);

        const bool followMode = settings->value("CharacterFollowMode", GetIsCharacterFollowModeActive()).toBool();
        SetCharacterFollowModeActive(followMode);
    }


    uint32 RenderViewWidget::FindActionIndex(QAction* action)
    {
        const uint32 numRenderOptions = NUM_RENDER_OPTIONS;
        for (uint32 i = 0; i < numRenderOptions; ++i)
        {
            if (mActions[i] == action)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    void RenderViewWidget::UpdateToolBarButton(bool checked)
    {
        assert(sender()->inherits("QAction"));
        QAction* action = qobject_cast<QAction*>(sender());

        const uint32 actionIndex = FindActionIndex(action);
        if (actionIndex != MCORE_INVALIDINDEX32)
        {
            mToolbarButtons[actionIndex]->setChecked(checked);
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderViewWidget.moc>
