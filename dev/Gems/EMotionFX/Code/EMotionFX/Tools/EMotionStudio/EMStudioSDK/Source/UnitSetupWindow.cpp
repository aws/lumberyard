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
#include "UnitSetupWindow.h"
#include "MainWindow.h"
#include "EMStudioManager.h"
#include "RenderPlugin/RenderPlugin.h"
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>


namespace EMStudio
{
    // constructor
    UnitSetupWindow::UnitSetupWindow(QWidget* parent)
        : QWidget(parent)
    {
        QVBoxLayout* rootLayout = new QVBoxLayout();
        rootLayout->setMargin(0);
        rootLayout->setSpacing(0);

        QWidget* holderWidget = new QWidget();
        holderWidget->setObjectName("StyledWidgetDark");

        QVBoxLayout* mainLayout = new QVBoxLayout();

        // create the node groups table
        QTableWidget* tableWidget = new QTableWidget();

        // create the table widget
        tableWidget->setSortingEnabled(false);
        tableWidget->setAlternatingRowColors(true);
        tableWidget->setCornerButtonEnabled(false);
        tableWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        tableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);

        // set the table to row single selection
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

        // set header items for the table
        tableWidget->setColumnCount(2);
        tableWidget->setRowCount(4);

        QStringList headers;
        headers.append("Setting");
        headers.append("Value");
        tableWidget->setHorizontalHeaderLabels(headers);

        QHeaderView* horizontalHeader = tableWidget->horizontalHeader();
        horizontalHeader->setDefaultAlignment(Qt::AlignLeft);
        horizontalHeader->setStretchLastSection(true);

        //  tableWidget->horizontalHeader()->hide();

        mainLayout->addWidget(tableWidget);

        mainLayout->setSizeConstraint(QLayout::SizeConstraint::SetMinimumSize);
        mainLayout->setMargin(0);

        mUnitTypeComboBox = new QComboBox();
        mUnitTypeComboBox->setMaximumWidth(100);
        mUnitTypeComboBox->addItem("Millimeter");
        mUnitTypeComboBox->addItem("Centimeter");
        mUnitTypeComboBox->addItem("Decimeter");
        mUnitTypeComboBox->addItem("Meter");
        mUnitTypeComboBox->addItem("Kilometer");
        mUnitTypeComboBox->addItem("Inch");
        mUnitTypeComboBox->addItem("Foot");
        mUnitTypeComboBox->addItem("Yard");
        mUnitTypeComboBox->addItem("Mile");
        connect(mUnitTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnUnitTypeComboIndexChanged(int)));

        mUnitTypeComboBox->blockSignals(true);
        MCore::Distance::EUnitType unitType = EMotionFX::GetEMotionFX().GetUnitType();
        switch (unitType)
        {
        case MCore::Distance::UNITTYPE_MILLIMETERS:
            mUnitTypeComboBox->setCurrentIndex(0);
            break;
        case MCore::Distance::UNITTYPE_CENTIMETERS:
            mUnitTypeComboBox->setCurrentIndex(1);
            break;
        case MCore::Distance::UNITTYPE_DECIMETERS:
            mUnitTypeComboBox->setCurrentIndex(2);
            break;
        case MCore::Distance::UNITTYPE_METERS:
            mUnitTypeComboBox->setCurrentIndex(3);
            break;
        case MCore::Distance::UNITTYPE_KILOMETERS:
            mUnitTypeComboBox->setCurrentIndex(4);
            break;
        case MCore::Distance::UNITTYPE_INCHES:
            mUnitTypeComboBox->setCurrentIndex(5);
            break;
        case MCore::Distance::UNITTYPE_FEET:
            mUnitTypeComboBox->setCurrentIndex(6);
            break;
        case MCore::Distance::UNITTYPE_YARDS:
            mUnitTypeComboBox->setCurrentIndex(7);
            break;
        case MCore::Distance::UNITTYPE_MILES:
            mUnitTypeComboBox->setCurrentIndex(8);
            break;
        default:
            mUnitTypeComboBox->setCurrentIndex(3);
            MCORE_ASSERT(false);
        }
        mUnitTypeComboBox->blockSignals(false);

        tableWidget->setItem(0, 0, new QTableWidgetItem("One Unit Is One:"));
        tableWidget->setCellWidget(0, 1, mUnitTypeComboBox);
        tableWidget->setRowHeight(0, 21);

        // find the current near clip plane
        double curNearClip      = 0.1;
        double curFarClip       = 100.0;
        double curGridSpacing   = 0.2;
        const uint32 numPlugins = GetPluginManager()->GetNumActivePlugins();
        for (uint32 i = 0; i < numPlugins; ++i)
        {
            EMStudioPlugin* plugin = GetPluginManager()->GetActivePlugin(i);
            if (plugin->GetPluginType() != EMStudioPlugin::PLUGINTYPE_RENDERING)
            {
                continue;
            }

            RenderPlugin* renderPlugin = static_cast<RenderPlugin*>(plugin);
            curNearClip     = renderPlugin->GetRenderOptions()->mNearClipPlaneDistance;
            curFarClip      = renderPlugin->GetRenderOptions()->mFarClipPlaneDistance;
            curGridSpacing  = renderPlugin->GetRenderOptions()->mGridUnitSize;
        }

        mNearClipSpinner = new MysticQt::DoubleSpinBox();
        mNearClipSpinner->setMaximumWidth(100);
        mNearClipSpinner->setSingleStep(0.1f);
        mNearClipSpinner->setDecimals(7);
        mNearClipSpinner->setRange(0.000001f, 10000.0f);
        mNearClipSpinner->setValue(curNearClip);
        connect(mNearClipSpinner,  SIGNAL(valueChanged(double)),   this, SLOT(ApplyWidgetValues()));

        tableWidget->setItem(1, 0, new QTableWidgetItem("Near Clip Plane Distance:"));
        tableWidget->setCellWidget(1, 1, mNearClipSpinner);
        tableWidget->setRowHeight(1, 21);

        mFarClipSpinner = new MysticQt::DoubleSpinBox();
        mFarClipSpinner->setMaximumWidth(100);
        mFarClipSpinner->setSingleStep(1.0f);
        mFarClipSpinner->setDecimals(7);
        mFarClipSpinner->setRange(0.000001f, 1000000.0f);
        mFarClipSpinner->setValue(curFarClip);
        connect(mFarClipSpinner,   SIGNAL(valueChanged(double)),   this, SLOT(ApplyWidgetValues()));

        tableWidget->setItem(2, 0, new QTableWidgetItem("Far Clip Plane Distance:"));
        tableWidget->setCellWidget(2, 1, mFarClipSpinner);
        tableWidget->setRowHeight(2, 21);

        mGridSpinner = new MysticQt::DoubleSpinBox();
        mGridSpinner->setMaximumWidth(100);
        mGridSpinner->setSingleStep(0.5f);
        mGridSpinner->setDecimals(7);
        mGridSpinner->setRange(0.000001f, 1000000.0f);
        mGridSpinner->setValue(curGridSpacing);
        connect(mGridSpinner,  SIGNAL(valueChanged(double)),   this, SLOT(ApplyWidgetValues()));

        tableWidget->setItem(3, 0, new QTableWidgetItem("Grid Substep Spacing:"));
        tableWidget->setCellWidget(3, 1, mGridSpinner);
        tableWidget->setRowHeight(3, 21);

        tableWidget->setColumnWidth(0, 175);

        holderWidget->setLayout(mainLayout);

        rootLayout->addWidget(holderWidget);
        setLayout(rootLayout);
    }


    // destructor
    UnitSetupWindow::~UnitSetupWindow()
    {
    }


    // when the combo box
    void UnitSetupWindow::OnUnitTypeComboIndexChanged(int index)
    {
        MCore::Distance::EUnitType unitType;
        switch (index)
        {
        case 0:
            unitType = MCore::Distance::UNITTYPE_MILLIMETERS;
            mNearClipSpinner->setValue(10.0f);
            mFarClipSpinner->setValue(200000.0f);
            mGridSpinner->setValue(200.0f);
            break;
        case 1:
            unitType = MCore::Distance::UNITTYPE_CENTIMETERS;
            mNearClipSpinner->setValue(1.0f);
            mFarClipSpinner->setValue(20000.0f);
            mGridSpinner->setValue(20.0f);
            break;
        case 2:
            unitType = MCore::Distance::UNITTYPE_DECIMETERS;
            mNearClipSpinner->setValue(0.1f);
            mFarClipSpinner->setValue(2000.0f);
            mGridSpinner->setValue(2.0f);
            break;
        case 3:
            unitType = MCore::Distance::UNITTYPE_METERS;
            mNearClipSpinner->setValue(0.1f);
            mFarClipSpinner->setValue(200.0f);
            mGridSpinner->setValue(0.2f);
            break;
        case 4:
            unitType = MCore::Distance::UNITTYPE_KILOMETERS;
            mNearClipSpinner->setValue(0.0001f);
            mFarClipSpinner->setValue(0.2f);
            mGridSpinner->setValue(0.0002f);
            break;
        case 5:
            unitType = MCore::Distance::UNITTYPE_INCHES;
            mNearClipSpinner->setValue(3.937f);
            mFarClipSpinner->setValue(7874.0f);
            mGridSpinner->setValue(7.87402f);
            break;
        case 6:
            unitType = MCore::Distance::UNITTYPE_FEET;
            mNearClipSpinner->setValue(0.328f);
            mFarClipSpinner->setValue(656.0f);
            mGridSpinner->setValue(0.656168f);
            break;
        case 7:
            unitType = MCore::Distance::UNITTYPE_YARDS;
            mNearClipSpinner->setValue(0.109f);
            mFarClipSpinner->setValue(218.0f);
            mGridSpinner->setValue(0.218723f);
            break;
        case 8:
            unitType = MCore::Distance::UNITTYPE_MILES;
            mNearClipSpinner->setValue(0.000062f);
            mFarClipSpinner->setValue(0.1242f);
            mGridSpinner->setValue(0.000124274f);
            break;
        default:
            unitType = MCore::Distance::UNITTYPE_METERS;
            mNearClipSpinner->setValue(0.1f);
            mFarClipSpinner->setValue(200.0f);
            mGridSpinner->setValue(0.2f);
            MCORE_ASSERT(false);
        }

        // apply the widget values to the settings
        ApplyWidgetValues();

        // update the saved preferences
        GetMainWindow()->SavePreferences();

        // get the current unit type and check if we want to change it
        MCore::Distance::EUnitType currentUnitType = EMotionFX::GetEMotionFX().GetUnitType();
        if (currentUnitType == unitType)
        {
            return;
        }

        // update the EMotion FX unit type
        EMotionFX::GetEMotionFX().SetUnitType(unitType);

        // convert all actors, motions and animgraphs into the new scale
        ScaleData();
    }


    // apply the current widget values to the render plugins
    void UnitSetupWindow::ApplyWidgetValues()
    {
        // find the render plugins and update the values
        const uint32 numPlugins = GetPluginManager()->GetNumActivePlugins();
        for (uint32 i = 0; i < numPlugins; ++i)
        {
            EMStudioPlugin* plugin = GetPluginManager()->GetActivePlugin(i);
            if (plugin->GetPluginType() != EMStudioPlugin::PLUGINTYPE_RENDERING)
            {
                continue;
            }

            RenderPlugin* renderPlugin = static_cast<RenderPlugin*>(plugin);
            renderPlugin->GetRenderOptions()->mNearClipPlaneDistance    = mNearClipSpinner->value();
            renderPlugin->GetRenderOptions()->mFarClipPlaneDistance     = mFarClipSpinner->value();
            renderPlugin->GetRenderOptions()->mGridUnitSize             = mGridSpinner->value();
        }
    }


    // scale all data
    void UnitSetupWindow::ScaleData()
    {
        // rescale actors
        const uint32 numActors = EMotionFX::GetActorManager().GetNumActors();
        if (numActors > 0)
        {
            MCore::CommandGroup commandGroup("Change actor scales");
            MCore::String tempString;

            for (uint32 i = 0; i < numActors; ++i)
            {
                EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);
                
                if (actor->GetIsOwnedByRuntime())
                {
                    continue;
                }

                tempString.Format("ScaleActorData -id %d -unitType \"%s\"", actor->GetID(), MCore::Distance::UnitTypeToString(EMotionFX::GetEMotionFX().GetUnitType()));
                commandGroup.AddCommandString(tempString.AsChar());
            }

            // execute the command group
            MCore::String outResult;
            if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult, false) == false)
            {
                MCore::LogError(outResult.AsChar());
            }
        }

        // rescale motions
        const uint32 numMotions = EMotionFX::GetMotionManager().GetNumMotions();
        if (numMotions > 0)
        {
            MCore::CommandGroup commandGroup("Change motion scales");
            MCore::String tempString;

            for (uint32 i = 0; i < numMotions; ++i)
            {
                EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);

                if (motion->GetIsOwnedByRuntime())
                {
                    continue;
                }

                tempString.Format("ScaleMotionData -id %d -unitType \"%s\"", motion->GetID(), MCore::Distance::UnitTypeToString(EMotionFX::GetEMotionFX().GetUnitType()));
                commandGroup.AddCommandString(tempString.AsChar());
            }

            // execute the command group
            MCore::String outResult;
            if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult, false) == false)
            {
                MCore::LogError(outResult.AsChar());
            }
        }

        // rescale animGraphs
        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        if (numAnimGraphs > 0)
        {
            MCore::CommandGroup commandGroup("Change animgraph scales");
            MCore::String tempString;

            for (uint32 i = 0; i < numAnimGraphs; ++i)
            {
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

                if (animGraph->GetIsOwnedByRuntime())
                {
                    continue;
                }

                tempString.Format("ScaleAnimGraphData -id %d -unitType \"%s\"", animGraph->GetID(), MCore::Distance::UnitTypeToString(EMotionFX::GetEMotionFX().GetUnitType()));
                commandGroup.AddCommandString(tempString.AsChar());
            }

            // execute the command group
            MCore::String outResult;
            if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult, false) == false)
            {
                MCore::LogError(outResult.AsChar());
            }
        }

        // find the render plugins and reset camera min and max distances
        const uint32 numPlugins = GetPluginManager()->GetNumActivePlugins();
        for (uint32 i = 0; i < numPlugins; ++i)
        {
            EMStudioPlugin* plugin = GetPluginManager()->GetActivePlugin(i);
            if (plugin->GetPluginType() != EMStudioPlugin::PLUGINTYPE_RENDERING)
            {
                continue;
            }

            RenderPlugin* renderPlugin = static_cast<RenderPlugin*>(plugin);
            renderPlugin->ResetCameras();
        }
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/UnitSetupWindow.moc>
