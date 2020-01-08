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

#include <QGraphicsView>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QVariant>
#include <QVBoxLayout>


#include "GraphTabBar.h"

#include <ScriptCanvas/Bus/RequestBus.h>
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

            setContextMenuPolicy(Qt::CustomContextMenu);
            connect(this, &QTabBar::customContextMenuRequested, this, &GraphTabBar::OnContextMenu);
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

        AZ::Data::AssetId GraphTabBar::FindAssetId(int tabIndex)
        {
            QVariant metadataVariant = tabData(tabIndex);
            if (metadataVariant.isValid())
            {
                auto graphMetadata = metadataVariant.value<GraphTabMetadata>();
                return graphMetadata.m_assetId;
            }

            return AZ::Data::AssetId();
        }

        void GraphTabBar::AddGraphTab(const AZ::Data::AssetId& assetId, AZStd::string_view tabName)
        {
            InsertGraphTab(count(),  assetId, tabName);
        }

        void GraphTabBar::InsertGraphTab(int tabIndex, const AZ::Data::AssetId& assetId, AZStd::string_view tabName)
        {
            if (!SelectTab(assetId))
            {
                int newTabIndex = insertTab(tabIndex, "");
                DocumentContextNotificationBus::MultiHandler::BusConnect(assetId);
                ScriptCanvasFileState fileState;
                DocumentContextRequestBus::BroadcastResult(fileState, &DocumentContextRequests::GetScriptCanvasAssetModificationState, assetId);

                CanvasWidget* canvasWidget = aznew CanvasWidget(this);

                GraphTabMetadata tabMetadata;
                tabMetadata.m_assetId = assetId;
                tabMetadata.m_hostWidget = canvasWidget;
                tabMetadata.m_tabName = QString(tabName.data());
                tabMetadata.m_fileState = fileState;
                setTabData(newTabIndex, QVariant::fromValue(tabMetadata));
                SetTabText(newTabIndex, tabName.data(), fileState);
            }
        }

        bool GraphTabBar::SetGraphTabData(int index, GraphTabMetadata tabMetadata)
        {
            if (index >= 0 && index < count())
            {
                QVariant data = tabData(index);
                if (data.isValid())
                {
                    auto curMetadata = data.value<GraphTabMetadata>();
                    DocumentContextNotificationBus::MultiHandler::BusDisconnect(curMetadata.m_assetId);

                    // The canvas widget is owned by the tab and cannot be replaced
                    if (tabMetadata.m_hostWidget != curMetadata.m_hostWidget)
                    {
                        delete tabMetadata.m_hostWidget;
                        tabMetadata.m_hostWidget = aznew CanvasWidget(this);
                    }
                    setTabData(index, QVariant::fromValue(tabMetadata));
                    SetTabText(index, tabMetadata.m_tabName, tabMetadata.m_fileState);
                    DocumentContextNotificationBus::MultiHandler::BusConnect(tabMetadata.m_assetId);

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

        void GraphTabBar::OnContextMenu(const QPoint& point)
        {
            QPoint screenPoint = mapToGlobal(point);

            int tabIndex = tabAt(point);
            bool hasValidTab = (tabIndex >= 0);
            bool isModified = false;

            QVariant data = tabData(tabIndex);
            if (data.isValid())
            {
                auto graphMetadata = data.value<GraphTabMetadata>();

                isModified = graphMetadata.m_fileState == ScriptCanvasFileState::NEW
                          || graphMetadata.m_fileState == ScriptCanvasFileState::MODIFIED;
            }

            QMenu menu;

            QAction* saveAction = menu.addAction("Save");
            saveAction->setEnabled(hasValidTab && isModified);

            QAction* closeAction = menu.addAction("Close");
            closeAction->setEnabled(hasValidTab);

            QAction* closeAllAction = menu.addAction("Close All");

            QAction* closeAllButThis = menu.addAction("Close All But This");
            closeAllButThis->setEnabled(hasValidTab);

            menu.addSeparator();
            QAction* fullPathAction = menu.addAction("Copy Source Path To Clipboard");
            fullPathAction->setEnabled(hasValidTab);

            QAction* action = menu.exec(screenPoint);

            if (action)
            {
                if (action == saveAction)
                {
                    Q_EMIT SaveTab(tabIndex);
                }
                else if (action == closeAction)
                {
                    tabCloseRequested(tabIndex);
                }
                else if (action == closeAllAction)
                {
                    Q_EMIT CloseAllTabs();
                }
                else if (action == closeAllButThis)
                {
                    Q_EMIT CloseAllTabsBut(tabIndex);
                }
                else if (action == fullPathAction)
                {
                    Q_EMIT CopyPathToClipboard(tabIndex);
                }
            }
        }

        void GraphTabBar::mouseReleaseEvent(QMouseEvent* event)
        {
            if (event->button() == Qt::MidButton)
            {
                int tabIndex = tabAt(event->localPos().toPoint());

                if (tabIndex >= 0)
                {
                    tabCloseRequested(tabIndex);
                    return;
                }
            }

            QTabBar::mouseReleaseEvent(event);
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