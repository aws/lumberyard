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
#include "precompiled.h"

#include <QCoreApplication>

#include <ISystem.h>
#include <IConsole.h>

#include <Editor/View/Windows/Tools/BatchConverter/BatchConverter.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

#include <Editor/View/Widgets/GraphTabBar.h>
#include <Editor/View/Windows/MainWindow.h>

namespace ScriptCanvasEditor
{
    ///////////////////////////////
    // ScriptCanvasBatchConverter
    ///////////////////////////////
    
    ScriptCanvasBatchConverter::ScriptCanvasBatchConverter(MainWindow* mainWindow, QStringList directories)
        : BatchOperatorTool(mainWindow, directories, "Running Batch Converter...")        
        , m_processing(false)        
    {
        GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);
    }
    
    void ScriptCanvasBatchConverter::PostOnActiveGraphChanged()
    {
        if (m_assetId.IsValid() && m_processing)
        {
            m_processing = false;

            GetMainWindow()->SaveAsset(m_assetId, [this](bool isSuccessful)
            {
                AZ::Data::AssetId assetId = m_assetId;
                m_assetId.SetInvalid();

                GetMainWindow()->CloseScriptCanvasAsset(assetId);
                SignalOperationComplete();
            });
        }
    }
    
    BatchOperatorTool::OperationStatus ScriptCanvasBatchConverter::OperateOnFile(const QString& fileName)
    {
        OperationStatus status = OperationStatus::Incomplete;
        
        QByteArray utf8FileName = fileName.toUtf8();
        const bool loadBlocking = true;
        AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset;
        DocumentContextRequestBus::BroadcastResult(scriptCanvasAsset, &DocumentContextRequests::LoadScriptCanvasAsset, utf8FileName.data(), loadBlocking);
        if (scriptCanvasAsset.IsReady())
        {
            if (GetMainWindow()->m_tabBar->FindTab(scriptCanvasAsset.GetId()) < 0)
            {
                m_assetId = scriptCanvasAsset.GetId();
                m_processing = true;

                GetProgressDialog()->setLabelText(QString("Converting %1...\n").arg(fileName));
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

                GetMainWindow()->OpenScriptCanvasAsset(scriptCanvasAsset);
            }
            else
            {
                status = OperationStatus::Complete;
            }
        }

        return status;
    }
}