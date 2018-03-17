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

#pragma once

// include MCore
#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/Color.h>
#include <MCore/Source/CommandGroup.h>
#include <MCore/Source/CommandManagerCallback.h>

// include the command system
#include <EMotionFX/CommandSystem/Source/CommandManager.h>

// include the gizmos
#include <EMotionFX/Rendering/Common/TransformationManipulator.h>

// include the EMStudio Config
#include "EMStudioConfig.h"
#include <MysticQt/Source/MysticQtManager.h>
#include "PluginManager.h"
#include "LayoutManager.h"
#include "OutlinerManager.h"
#include "NotificationWindowManager.h"
#include "Workspace.h"
#include "MainWindow.h"

// include Qt
#include <QString>
#include <QPainter>
#include <QPointer>
#include <QWidget>

// forward declarations
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLabel)


namespace EMStudio
{
    // forward declarations
    class MotionEventPresetManager;

    /**
     *
     *
     *
     */
    class EMSTUDIO_API EMStudioManager
    {
        MCORE_MEMORYOBJECTCATEGORY(EMStudioManager, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        EMStudioManager(QApplication* app, int& argc, char* argv[]);
        ~EMStudioManager();

        const char* GetCompileDate() const                                      { return mCompileDate.AsChar(); }

        MCORE_INLINE QApplication* GetApp()                                     { return mApp; }
        MCORE_INLINE bool HasMainWindow() const                                 { return !mMainWindow.isNull(); }
        MainWindow* GetMainWindow();
        MCORE_INLINE PluginManager* GetPluginManager()                          { return mPluginManager; }
        MCORE_INLINE LayoutManager* GetLayoutManager()                          { return mLayoutManager; }
        MCORE_INLINE OutlinerManager* GetOutlinerManager()                      { return mOutlinerManager; }
        MCORE_INLINE NotificationWindowManager* GetNotificationWindowManager()  { return mNotificationWindowManager; }
        MCORE_INLINE CommandSystem::CommandManager* GetCommandManager()         { return mCommandManager; }
        MCore::String GetAppDataFolder() const;
        MCore::String GetRecoverFolder() const;
        MCore::String GetAutosavesFolder() const;

        // text rendering helper function
        static void RenderText(QPainter& painter, const QString& text, const QColor& textColor, const QFont& font, const QFontMetrics& fontMetrics, Qt::Alignment textAlignment, const QRect& rect);

        // motion event presets
        MotionEventPresetManager* GetEventPresetManger() const                  { return mEventPresetManager; }

        void SetAutoLoadLastWorkspace(bool autoLoad)                            { mAutoLoadLastWorkspace = autoLoad; }
        bool GetAutoLoadLastWorkspace() const                                   { return mAutoLoadLastWorkspace; }

        const char* ConstructHTMLLink(const char* text, const MCore::RGBAColor& color = MCore::RGBAColor(0.95315f, 0.609375f, 0.109375f));
        void SetWidgetAsInvalidInput(QWidget* widget);

        static void MakeTransparentButton(QPushButton* button, const char* iconFileName, const char* toolTipText, uint32 width = 20, uint32 height = 20);
        static QLabel* MakeSeperatorLabel(uint32 width, uint32 height);

        int ExecuteApp();
        void LogInfo();

        // in case the array is empty, all nodes are shown
        void SetVisibleNodeIndices(const MCore::Array<uint32>& visibleNodeIndices);
        MCore::Array<uint32>& GetVisibleNodeIndices();

        void SetSelectedNodeIndices(const MCore::Array<uint32>& selectedNodeIndices)            { mSelectedNodeIndices = selectedNodeIndices; }
        MCore::Array<uint32>& GetSelectedNodeIndices()                                          { return mSelectedNodeIndices; }

        Workspace* GetWorkspace()                                                               { return &mWorkspace; }

        // functions for adding/removing gizmos
        MCommon::TransformationManipulator* AddTransformationManipulator(MCommon::TransformationManipulator* manipulator);
        void RemoveTransformationManipulator(MCommon::TransformationManipulator* manipulator);
        MCore::Array<MCommon::TransformationManipulator*>* GetTransformationManipulators();

        void ClearScene();  // remove animgraphs, animgraph instances and actors

        MCORE_INLINE bool GetAvoidRendering() const                                             { return mAvoidRendering; }
        MCORE_INLINE void SetAvoidRendering(bool avoidRendering)                                { mAvoidRendering = avoidRendering; }

    private:
        MCore::Array<MCommon::TransformationManipulator*> mTransformationManipulators;
        QPointer<MainWindow>                mMainWindow;
        QApplication*                       mApp;
        PluginManager*                      mPluginManager;
        LayoutManager*                      mLayoutManager;
        OutlinerManager*                    mOutlinerManager;
        NotificationWindowManager*          mNotificationWindowManager;
        CommandSystem::CommandManager*      mCommandManager;
        MCore::String                       mCompileDate;
        MCore::Array<uint32>                mVisibleNodeIndices;        // node name rendering filter
        MCore::Array<uint32>                mSelectedNodeIndices;       // node name rendering filter
        Workspace                           mWorkspace;
        bool                                mAutoLoadLastWorkspace;
        MCore::String                       mHTMLLinkString;
        bool                                mAvoidRendering;
        MotionEventPresetManager*           mEventPresetManager;

        class EMSTUDIO_API EventProcessingCallback
            : public MCore::CommandManagerCallback
        {
            MCORE_MEMORYOBJECTCATEGORY(EventProcessingCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK);

        public:
            void OnPreExecuteCommand(MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine) override;
            void OnPostExecuteCommand(MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine, bool wasSuccess, const MCore::String& outResult) override;
            void OnPreExecuteCommandGroup(MCore::CommandGroup* group, bool undo) override                                                                                                       { MCORE_UNUSED(group); MCORE_UNUSED(undo); }
            void OnPostExecuteCommandGroup(MCore::CommandGroup* group, bool wasSuccess) override                                                                                                { MCORE_UNUSED(group); MCORE_UNUSED(wasSuccess); }
            void OnAddCommandToHistory(uint32 historyIndex, MCore::CommandGroup* group, MCore::Command* command, const MCore::CommandLine& commandLine) override                                { MCORE_UNUSED(historyIndex); MCORE_UNUSED(group); MCORE_UNUSED(command); MCORE_UNUSED(commandLine); }
            void OnRemoveCommand(uint32 historyIndex) override                                                                                                                                  { MCORE_UNUSED(historyIndex); }
            void OnSetCurrentCommand(uint32 index) override                                                                                                                                     { MCORE_UNUSED(index); }
        };
        EventProcessingCallback*    mEventProcessingCallback;
    };


    /**
     *
     *
     *
     */
    class EMSTUDIO_API Initializer
    {
    public:
        static bool MCORE_CDECL Init(QApplication* app, int& argc, char* argv[]);
        static void MCORE_CDECL Shutdown();
    };


    // the global manager
    extern EMSTUDIO_API EMStudioManager* gEMStudioMgr;

    // shortcuts
    MCORE_INLINE QApplication*                  GetApp()                        { return gEMStudioMgr->GetApp(); }
    MCORE_INLINE EMStudioManager*               GetManager()                    { return gEMStudioMgr; }
    MCORE_INLINE bool                           HasMainWindow()                 { return gEMStudioMgr->HasMainWindow(); }
    MCORE_INLINE MainWindow*                    GetMainWindow()                 { return gEMStudioMgr->GetMainWindow(); }
    MCORE_INLINE PluginManager*                 GetPluginManager()              { return gEMStudioMgr->GetPluginManager(); }
    MCORE_INLINE LayoutManager*                 GetLayoutManager()              { return gEMStudioMgr->GetLayoutManager(); }
    MCORE_INLINE OutlinerManager*               GetOutlinerManager()            { return gEMStudioMgr->GetOutlinerManager(); }
    MCORE_INLINE NotificationWindowManager*     GetNotificationWindowManager()  { return gEMStudioMgr->GetNotificationWindowManager(); }
    MCORE_INLINE MotionEventPresetManager*      GetEventPresetManager()         { return gEMStudioMgr->GetEventPresetManger(); }
    MCORE_INLINE CommandSystem::CommandManager* GetCommandManager()             { return gEMStudioMgr->GetCommandManager(); }
}   // namespace EMStudio
