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
#include "MotionCompressionPlugin.h"
#include <EMotionFX/Source/MotionManager.h>
#include <MCore/Source/LogManager.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTableWidget>


namespace EMStudio
{
    // constructor
    MotionCompressionPlugin::MotionCompressionPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        mDialogStack                        = nullptr;
        mMotionWindowPlugin                 = nullptr;
        mMotionListWindow                   = nullptr;
        mMotionTable                        = nullptr;
        mWaveletCompressMotionCallback      = nullptr;
        mKeyframeCompressMotionCallback     = nullptr;
    }


    // destructor
    MotionCompressionPlugin::~MotionCompressionPlugin()
    {
        GetCommandManager()->RemoveCommandCallback(mWaveletCompressMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mKeyframeCompressMotionCallback, false);
        delete mWaveletCompressMotionCallback;
        delete mKeyframeCompressMotionCallback;
    }


    // clone the log window
    EMStudioPlugin* MotionCompressionPlugin::Clone()
    {
        return new MotionCompressionPlugin();
    }


    // init after the parent dock window has been created
    bool MotionCompressionPlugin::Init()
    {
        MCore::LogInfo("Initializing motion compression window.");

        // create callbacks
        mKeyframeCompressMotionCallback     = new CommandKeyframeCompressMotionCallback(false, true);
        mWaveletCompressMotionCallback      = new CommandWaveletCompressMotionCallback(false, true);
        GetCommandManager()->RegisterCommandCallback("KeyframeCompressMotion", mKeyframeCompressMotionCallback);
        GetCommandManager()->RegisterCommandCallback("WaveletCompressMotion", mWaveletCompressMotionCallback);

        // create the dialog stack
        assert(mDialogStack == nullptr);
        mDialogStack = new MysticQt::DialogStack();

        // create control widgets
        mKeyframeOptimizationWidget = new KeyframeOptimizationWidget(mDialogStack);
        mWaveletCompressionWidget   = new WaveletCompressionWidget(mDialogStack);

        // prepare the dock window
        mDock->SetContents(mDialogStack);
        mDock->setMinimumWidth(100);
        mDock->setMinimumHeight(100);

        mDialogStack->Add(mKeyframeOptimizationWidget, "Keyframe Optimization");
        mDialogStack->Add(mWaveletCompressionWidget, "Wavelet Compression");

        // add functionality to the controls
        connect(mDock, SIGNAL(visibilityChanged(bool)), this, SLOT(VisibilityChanged(bool)));

        // connect to the motions table
        ValidatePluginLinks();

        // reinit the dialog
        ReInit();

        return true;
    }


    // reinitialize the window
    void MotionCompressionPlugin::ReInit()
    {
        MotionSelectionChanged();
    }


    // reinit the window when it gets activated
    void MotionCompressionPlugin::VisibilityChanged(bool isVisible)
    {
        if (isVisible)
        {
            ReInit();
        }
    }


    // validate the connection between compression widget and the motion list window
    void MotionCompressionPlugin::ValidatePluginLinks()
    {
        if (mMotionWindowPlugin == nullptr)
        {
            EMStudioPlugin* motionBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
            if (motionBasePlugin)
            {
                mMotionWindowPlugin = (MotionWindowPlugin*)motionBasePlugin;
                mMotionListWindow   = mMotionWindowPlugin->GetMotionListWindow();
                mMotionTable        = mMotionListWindow->GetMotionTable();

                // connect motion table with wavelet motion compression plugin
                connect(mMotionTable, SIGNAL(itemSelectionChanged()), this, SLOT(MotionSelectionChanged()));
            }
        }
    }


    // updates motion pointers if the selection within the motion list window changes
    void MotionCompressionPlugin::MotionSelectionChanged()
    {
        // get the currently selected motion
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();

        // set the motion of the motion compression widgets
        mWaveletCompressionWidget->SetMotion(motion);
        mKeyframeOptimizationWidget->SetMotion(motion);
    }


    //-----------------------------------------------------------------------------------------
    // command callbacks
    //-----------------------------------------------------------------------------------------

    bool ReInitMotionCompressionPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionCompressionPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MotionCompressionPlugin* motionCompressionWindow = (MotionCompressionPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        if (motionCompressionWindow->GetDockWidget()->visibleRegion().isEmpty() == false)
        {
            motionCompressionWindow->ReInit();
        }

        return true;
    }

    bool MotionCompressionPlugin::CommandKeyframeCompressMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMotionCompressionPlugin(); }
    bool MotionCompressionPlugin::CommandKeyframeCompressMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMotionCompressionPlugin(); }
    bool MotionCompressionPlugin::CommandWaveletCompressMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMotionCompressionPlugin(); }
    bool MotionCompressionPlugin::CommandWaveletCompressMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMotionCompressionPlugin(); }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionCompression/MotionCompressionPlugin.moc>