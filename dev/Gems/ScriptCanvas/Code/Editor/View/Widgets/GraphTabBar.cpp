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

#include "GraphTabBar.h"

#include <QVariant>

#include <ScriptCanvas/Bus/RequestBus.h>

#include <QVBoxLayout>
#include <QGraphicsView>
#include <QLabel>

#include <Editor/View/Widgets/CanvasWidget.h>

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        GraphTabBar::GraphTabBar(QWidget* parent /*= nullptr*/)
            : QTabBar(parent)
        {
            setExpanding(false);
            setTabsClosable(true);
            setMinimumWidth(200);
            setMinimumHeight(32);
            setDocumentMode(true);
            setMovable(true);
            setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

            connect(this, &QTabBar::currentChanged, this, &GraphTabBar::currentChangedTab);
        }

        void GraphTabBar::RemoveAllBars()
        {
            for (int i = count() - 1; i >= 0; --i)
            {
                Q_EMIT TabCloseNoButton(i);
            }
        }

        void GraphTabBar::SetTabText(int tabIndex, const QString& path, ScriptCanvasFileState fileState)
        {
            if (tabIndex >= 0 && tabIndex < count())
            {
                QVariant data = tabData(tabIndex);
                if (data.isValid())
                {
                    auto graphMetadata = data.value<GraphTabMetadata>();
                    graphMetadata.m_fileState = fileState;
                    graphMetadata.m_tabName = path;
                    setTabData(tabIndex, QVariant::fromValue(graphMetadata));
                }

                const char* fileStateTag = "";
                switch (fileState)
                {
                case ScriptCanvasFileState::NEW:
                    fileStateTag = "^";
                    break;
                case ScriptCanvasFileState::MODIFIED:
                    fileStateTag = "*";
                    break;
                default:
                    break;
                }

                setTabText(tabIndex, QString("%1%2").arg(path).arg(fileStateTag));
            }
        }

        void GraphTabBar::tabInserted(int index)
        {
            QTabBar::tabInserted(index);

            Q_EMIT TabInserted(index);
        }

        void GraphTabBar::tabRemoved(int index)
        {
            QTabBar::tabRemoved(index);

            Q_EMIT TabRemoved(index);
        }

        bool GraphTabBar::SelectTab(const AZ::Data::AssetId& assetId)
        {
            int tabIndex = FindTab(assetId);
            if (-1 != tabIndex)
            {
                setCurrentIndex(tabIndex);
            }
            return false;
        }

        int GraphTabBar::FindTab(const AZ::Data::AssetId& assetId) const
        {
            for (int tabIndex = 0; tabIndex < count(); ++tabIndex)
            {
                QVariant metadataVariant = tabData(tabIndex);
                if (metadataVariant.isValid())
                {
                    auto graphMetadata = metadataVariant.value<GraphTabMetadata>();
                    if (graphMetadata.m_assetId == assetId)
                    {
                        return tabIndex;
                    }
                }
            }
            return -1;
        }

        void GraphTabBar::AddGraphTab(const AZ::EntityId& sceneId, const AZ::Data::AssetId& assetId, const AZ::EntityId& graphId, const AZStd::string& tabName)
        {
            InsertGraphTab(count(), sceneId, assetId, graphId, tabName);
        }

        void GraphTabBar::InsertGraphTab(int tabIndex, const AZ::EntityId& sceneId, const AZ::Data::AssetId& assetId, const AZ::EntityId& graphId, const AZStd::string& tabName)
        {
            if (!SelectTab(assetId))
            {
                int newTabIndex = insertTab(tabIndex, "");
                DocumentContextNotificationBus::MultiHandler::BusConnect(assetId);
                ScriptCanvasFileState fileState;
                DocumentContextRequestBus::BroadcastResult(fileState, &DocumentContextRequests::GetScriptCanvasAssetModificationState, assetId);

                CanvasWidget* canvasWidget = aznew CanvasWidget(assetId, graphId, this);
                canvasWidget->ShowScene(sceneId);

                GraphTabMetadata tabMetadata;
                tabMetadata.m_assetId = assetId;
                tabMetadata.m_hostWidget = canvasWidget;
                tabMetadata.m_tabName = QString(tabName.data());
                tabMetadata.m_fileState = fileState;
                setTabData(newTabIndex, QVariant::fromValue(tabMetadata));
                SetTabText(newTabIndex, tabName.data(), fileState);

                setCurrentIndex(newTabIndex);
                currentChangedTab(newTabIndex);
            }
        }

        void GraphTabBar::InsertBlankGraphTab(int tabIndex, const AZ::Data::AssetId& assetId)
        {
            if (!SelectTab(assetId))
            {
                int newTabIndex = insertTab(tabIndex, "");
                DocumentContextNotificationBus::MultiHandler::BusConnect(assetId);
                ScriptCanvasFileState fileState;
                DocumentContextRequestBus::BroadcastResult(fileState, &DocumentContextRequests::GetScriptCanvasAssetModificationState, assetId);

                CanvasWidget* canvasWidget = aznew CanvasWidget(assetId, AZ::EntityId(), this);

                GraphTabMetadata tabMetadata;
                tabMetadata.m_assetId = assetId;
                tabMetadata.m_hostWidget = canvasWidget;
                tabMetadata.m_fileState = fileState;

                setTabData(newTabIndex, QVariant::fromValue(tabMetadata));
                setCurrentIndex(newTabIndex);
                currentChangedTab(newTabIndex);
            }
        }

        bool GraphTabBar::SetGraphTabData(const AZ::Data::AssetId& assetId, const AZ::EntityId& sceneId, const AZ::EntityId& graphId, const AZStd::string& tabName)
        {
            int index = FindTab(assetId);
            if (index >= 0 && index < count())
            {
                QVariant data = tabData(index);
                if (data.isValid())
                {
                    auto graphMetadata = data.value<GraphTabMetadata>();

                    graphMetadata.m_tabName = QString(tabName.data());
                    if (graphMetadata.m_hostWidget)
                    {
                        if (auto canvasWidget = qobject_cast<Widget::CanvasWidget*>(graphMetadata.m_hostWidget))
                        {
                            canvasWidget->SetGraphId(graphId);
                            canvasWidget->ShowScene(sceneId);
                        }
                    }

                    setTabData(index, QVariant::fromValue(graphMetadata));
                    SetTabText(index, graphMetadata.m_tabName, graphMetadata.m_fileState);
                    update();
                    show();
                    return true;
                }
            }

            return false;
        }

        void GraphTabBar::currentChangedTab(int index)
        {
            if (index < 0)
            {
                return;
            }

            QVariant data = tabData(index);
            if (!data.isValid())
            {
                return;
            }

            auto graphMetadata = data.value<GraphTabMetadata>();

            ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::OnChangeActiveGraphTab, graphMetadata);

        }

        void GraphTabBar::CloseTab(int index)
        {
            if (index >= 0 && index < count())
            {
                QVariant data = tabData(index);
                if (data.isValid())
                {
                    auto graphMetadata = data.value<GraphTabMetadata>();
                    delete graphMetadata.m_hostWidget;
                    DocumentContextNotificationBus::MultiHandler::BusDisconnect(graphMetadata.m_assetId);
                }
                removeTab(index);
            }
        }

        void GraphTabBar::OnAssetModificationStateChanged(ScriptCanvasFileState fileState)
        {
            const AZ::Data::AssetId* assetId = DocumentContextNotificationBus::GetCurrentBusId();
            if (!assetId)
            {
                return;
            }
            int index = FindTab(*assetId);

            if (index >= 0 && index < count())
            {
                QVariant data = tabData(index);
                if (data.isValid())
                {
                    auto graphMetadata = data.value<GraphTabMetadata>();
                    SetTabText(index, graphMetadata.m_tabName, fileState);
                }
            }
        }

        #include <Editor/View/Widgets/GraphTabBar.moc>
    }
}
