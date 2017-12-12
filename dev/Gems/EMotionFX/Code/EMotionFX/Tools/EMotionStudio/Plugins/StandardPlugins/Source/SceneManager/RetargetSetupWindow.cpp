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
#include "RetargetSetupWindow.h"
#include "SceneManagerPlugin.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <EMotionFX/Source/NodeMap.h>
#include <EMotionFX/Source/RetargetSetup.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <MCore/Source/LogManager.h>

#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include <MysticQt/Source/MysticQtManager.h>
#include <AzFramework/API/ApplicationAPI.h>


namespace EMStudio
{
    // constructor
    RetargetSetupWindow::RetargetSetupWindow(QWidget* parent, SceneManagerPlugin* plugin)
        : QDialog(parent)
    {
        mPlugin = plugin;

        // set the window title
        setWindowTitle("Retarget Sources Setup");

        // set the minimum size
        setMinimumWidth(900);
        setMinimumHeight(700);

        // create the main layout
        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setMargin(3);
        mainLayout->setSpacing(1);
        setLayout(mainLayout);

        mDialogStack = new MysticQt::DialogStack();
        mainLayout->addWidget(mDialogStack);

        QVBoxLayout* sourceLayout = new QVBoxLayout();
        sourceLayout->setMargin(0);
        sourceLayout->setSpacing(2);

        // create the toolbar
        QHBoxLayout* toolBarLayout = new QHBoxLayout();
        toolBarLayout->setSpacing(0);
        sourceLayout->addLayout(toolBarLayout);

        mButtonAdd = new QPushButton();
        EMStudioManager::MakeTransparentButton(mButtonAdd, "/Images/Icons/Plus.png",       "Add a new source mapping.");
        connect(mButtonAdd, SIGNAL(clicked()), this, SLOT(OnAddSourceActor()));
        mButtonRemove = new QPushButton();
        EMStudioManager::MakeTransparentButton(mButtonRemove,  "/Images/Icons/Minus.png",  "Remove the currently selected source.");
        connect(mButtonRemove, SIGNAL(clicked()), this, SLOT(OnRemoveSourceActor()));
        mButtonClear = new QPushButton();
        EMStudioManager::MakeTransparentButton(mButtonClear,   "/Images/Icons/Clear.png",  "Clear all sources.");
        connect(mButtonClear, SIGNAL(clicked()), this, SLOT(OnClearSourceActors()));

        toolBarLayout->addWidget(mButtonAdd,       0, Qt::AlignLeft);
        toolBarLayout->addWidget(mButtonRemove,    0, Qt::AlignLeft);
        toolBarLayout->addWidget(mButtonClear,     0, Qt::AlignLeft);
        toolBarLayout->addSpacerItem(new QSpacerItem(100, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));

        // create the table widget
        mTableWidget = new QTableWidget();
        mTableWidget->setAlternatingRowColors(true);
        mTableWidget->setGridStyle(Qt::SolidLine);
        mTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        mTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        //mCurrentList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        mTableWidget->setCornerButtonEnabled(false);
        mTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mTableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);
        mTableWidget->setColumnCount(2);
        mTableWidget->setMinimumHeight(150);
        mTableWidget->setColumnWidth(0, 150);
        mTableWidget->setColumnWidth(1, 300);
        mTableWidget->setSortingEnabled(false);
        QHeaderView* verticalHeader = mTableWidget->verticalHeader();
        verticalHeader->setVisible(false);
        QTableWidgetItem* headerItem = new QTableWidgetItem("Name");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("Source Actor");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(1, headerItem);
        mTableWidget->horizontalHeader()->setStretchLastSection(true);
        mTableWidget->horizontalHeader()->setSortIndicatorShown(false);
        mTableWidget->horizontalHeader()->setSectionsClickable(false);
        //connect(mTableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(OnCurrentListSelectionChanged()));
        //connect(mTableWidget, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(OnChangeMapping(QTableWidgetItem*)));
        sourceLayout->addWidget(mTableWidget);
        mDialogStack->Add(sourceLayout, "Retarget Sources", false, false, true);

        QWidget* mappingWidget = new QWidget();
        mDialogStack->Add(mappingWidget, "Mapping Setup", false, true, true);
    }


    // destructor
    RetargetSetupWindow::~RetargetSetupWindow()
    {
    }


    // reinit the interface
    void RetargetSetupWindow::Reinit()
    {
        // if we haven't selected an actor, return
        const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* targetActor = selection.GetSingleActor();
        if (targetActor == nullptr)
        {
            mTableWidget->setRowCount(0);
            return;
        }

        MCore::String actorFile;

        // display all retarget infos
        EMotionFX::RetargetSetup* retargetSetup = targetActor->GetRetargetSetup();
        const uint32 numRetargetInfos = retargetSetup->GetNumRetargetInfos();
        mTableWidget->setRowCount(numRetargetInfos);
        for (uint32 i = 0; i < numRetargetInfos; ++i)
        {
            const EMotionFX::RetargetSetup::RetargetInfo& retargetInfo = retargetSetup->GetRetargetInfo(i);

            mTableWidget->setRowHeight(i, 21);

            QLineEdit* idEditWidget = new QLineEdit();
            idEditWidget->setPlaceholderText("<enter unique id>");

            MysticQt::LinkWidget* actorLink = new MysticQt::LinkWidget();
            actorLink->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
            actorLink->setMaximumHeight(22);
            actorLink->setProperty("index", i);
            connect(actorLink, SIGNAL(clicked()), this, SLOT(OnLoadSourceActor()));
            /*
                    MysticQt::LinkWidget* mappingLink = new MysticQt::LinkWidget();
                    mappingLink->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
                    mappingLink->setMaximumHeight(22);
                    mappingLink->setText("Configure");
                    connect( mappingLink, SIGNAL(clicked()), this, SLOT(OnSetupMapping()) );
            */
            QWidget* sourceCellWidget = new QWidget();
            QHBoxLayout* cellLayout = new QHBoxLayout();
            cellLayout->setSpacing(0);
            cellLayout->setContentsMargins(3, 1, 1, 1);
            cellLayout->addWidget(actorLink, Qt::AlignVCenter | Qt::AlignLeft);
            sourceCellWidget->setLayout(cellLayout);
            QWidget* spacerWidget = new QWidget();
            spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            cellLayout->addWidget(spacerWidget);
            /*
                    QWidget* mappingCellWidget = new QWidget();
                    QHBoxLayout* mappingLayout = new QHBoxLayout();
                    mappingLayout->setSpacing(0);
                    mappingLayout->setContentsMargins(3, 1, 1, 1);
                    mappingLayout->addWidget( mappingLink, Qt::AlignVCenter | Qt::AlignLeft );
                    mappingCellWidget->setLayout( mappingLayout );
                    QWidget* mappingSpacerWidget = new QWidget();
                    mappingSpacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
                    mappingLayout->addWidget(mappingSpacerWidget);
            */
            // if we are dealing with an empty info
            if (retargetInfo.mSourceActorInstance == nullptr)
            {
                actorLink->setText("Load actor");
                mTableWidget->setCellWidget(i, 0, idEditWidget);
                mTableWidget->setCellWidget(i, 1, sourceCellWidget);
                //          mTableWidget->setCellWidget(i, 1, mappingCellWidget);
                continue;
            }

            actorFile = retargetInfo.mSourceActorInstance->GetActor()->GetFileNameString();
            actorLink->setText(actorFile.AsChar());
        }
    }


    //
    void RetargetSetupWindow::OnAddSourceActor()
    {
        // if we haven't selected an actor, return
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* targetActor = selection.GetSingleActor();
        if (targetActor == nullptr)
        {
            return;
        }

        // register a new retarget entry to the setup
        EMotionFX::RetargetSetup* retargetSetup = targetActor->GetRetargetSetup();
        retargetSetup->RegisterEmptyRetargetInfo();

        // reinit the window
        Reinit();
    }


    // remove the currently selected retarget info
    void RetargetSetupWindow::OnRemoveSourceActor()
    {
        // if we haven't selected an actor, return
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* targetActor = selection.GetSingleActor();
        if (targetActor == nullptr)
        {
            return;
        }

        //
        //EMotionFX::RetargetSetup* retargetSetup = targetActor->GetRetargetSetup();
        // TODO: remove the currently selected one
    }


    //
    void RetargetSetupWindow::OnClearSourceActors()
    {
        // if we haven't selected an actor, return
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* targetActor = selection.GetSingleActor();
        if (targetActor == nullptr)
        {
            return;
        }

        // remove all retarget infos
        EMotionFX::RetargetSetup* retargetSetup = targetActor->GetRetargetSetup();
        retargetSetup->RemoveAllRetargetInfos();

        // reinit the window
        Reinit();
    }


    // load a source actor
    void RetargetSetupWindow::OnLoadSourceActor()
    {
        // if we haven't selected an actor, return
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* targetActor = selection.GetSingleActor();
        if (targetActor == nullptr)
        {
            return;
        }

        // show the actor selection dialog
        AZStd::string filename = GetMainWindow()->GetFileManager()->LoadActorFileDialog(this);
        if (filename.empty())
        {
            return;
        }

        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        // get the index into the table (and retarget infos)
        MysticQt::LinkWidget* linkWidget = qobject_cast<MysticQt::LinkWidget*>(sender());
        uint32 index = linkWidget->property("index").toUInt();

        // load the source actor
        EMotionFX::Importer::ActorSettings actorSettings;
        actorSettings.mLoadMeshes           = false;
        actorSettings.mLoadGeometryLODs     = false;
        actorSettings.mLoadMorphTargets     = false;
        EMotionFX::Actor* sourceActor = EMotionFX::GetImporter().LoadActor(filename.c_str(), &actorSettings);
        if (sourceActor == nullptr)
        {
            QMessageBox::critical(this, "Error", "Failed to load the actor file.");
            return;
        }

        //MCore::LogInfo("index = %d    filename=%s", index, filename.AsChar());
        EMotionFX::RetargetSetup* retargetSetup = targetActor->GetRetargetSetup();
        retargetSetup->InitRetargetInfo(index, sourceActor, nullptr);

        Reinit();
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/SceneManager/RetargetSetupWindow.moc>