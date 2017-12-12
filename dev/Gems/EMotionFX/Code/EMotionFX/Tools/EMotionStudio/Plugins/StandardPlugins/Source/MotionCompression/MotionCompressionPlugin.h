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

#ifndef __EMSTUDIO_MOTIONCOMPRESSIONPLUGIN_H
#define __EMSTUDIO_MOTIONCOMPRESSIONPLUGIN_H

// include MCore
#include "../StandardPluginsConfig.h"
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <MysticQt/Source/DialogStack.h>
#include <MysticQt/Source/PropertyWidget.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include "KeyframeOptimizationWidget.h"
#include "WaveletCompressionWidget.h"

namespace EMStudio
{
    /**
     * plugin for handling motion compression.
     * provides interface for wavelet compression and keyframe optimization methods.
     */
    class MotionCompressionPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT
                           MCORE_MEMORYOBJECTCATEGORY(MotionCompressionPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000667
        };

        MotionCompressionPlugin();
        ~MotionCompressionPlugin();

        // overloaded
        const char* GetCompileDate() const override     { return MCORE_DATE; }
        const char* GetName() const override            { return "Motion Compression"; }
        uint32 GetClassID() const override              { return CLASS_ID; }
        const char* GetCreatorName() const override     { return "MysticGD"; }
        float GetVersion() const override               { return 1.0f;  }
        bool GetIsClosable() const override             { return true;  }
        bool GetIsFloatable() const override            { return true;  }
        bool GetIsVertical() const override             { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() override;
        void ReInit();

    public slots:
        void VisibilityChanged(bool isVisible);
        void ValidatePluginLinks();
        void MotionSelectionChanged();

    private:
        // define callbacks for motion deletion
        MCORE_DEFINECOMMANDCALLBACK(CommandWaveletCompressMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandKeyframeCompressMotionCallback);

        CommandWaveletCompressMotionCallback*       mWaveletCompressMotionCallback;
        CommandKeyframeCompressMotionCallback*      mKeyframeCompressMotionCallback;

        MysticQt::DialogStack*              mDialogStack;
        KeyframeOptimizationWidget*         mKeyframeOptimizationWidget;
        WaveletCompressionWidget*           mWaveletCompressionWidget;

        // pointers to the motion window plugin
        QTableWidget*       mMotionTable;
        MotionWindowPlugin* mMotionWindowPlugin;
        MotionListWindow*   mMotionListWindow;
    };
} // namespace EMStudio


#endif
