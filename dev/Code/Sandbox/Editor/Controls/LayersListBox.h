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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include "../Include/ISourceControl.h"

#include <QTreeView>
#include <QTimer>
#include <QRunnable>
#include <QMutex>

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

class LayerModel;

// CLayersListBox dialog
/*!
 *  CLayerListBox   is a special owner draw version of list box to display layers.
 */

class CLayersListBox
    : public QTreeView
    , private AzToolsFramework::SourceControlNotificationBus::Handler
{
    Q_OBJECT
public:
    typedef Functor0 UpdateCallback;

    struct SLayerInfo
    {
        QString name;
        bool visible = false;
        bool usable = false;
        bool expanded = false;
        int childCount = 0;
        bool lastchild = false;
        bool isModified = false;
        TSmartPtr<CObjectLayer> pLayer = nullptr;
        int indent = false;

        SLayerInfo() = default;
    };

    //This structure stores fileAttributes cache of a layer
    struct SCacheLayerAttributes
    {
        QString name;
        uint32 fileAttributes = ESccFileAttributes::SCC_FILE_ATTRIBUTE_INVALID;
        bool isLayerNew = true;
        bool queuedRefresh = false;
        AZStd::sys_time_t time;
        TSmartPtr<CObjectLayer> pLayer = nullptr;

        SCacheLayerAttributes();
    };

    struct LayerAttribRequest
    {
        QString layerName;
        AZStd::string layerPath;

        LayerAttribRequest() = default;
        LayerAttribRequest(QString name)
            : layerName(name)
        {}
    };

    struct LayerAttribResult
    {
        QString layerName;
        uint32 fileAttributes = ESccFileAttributes::SCC_FILE_ATTRIBUTE_INVALID;

        LayerAttribResult() = default;
        LayerAttribResult(QString name, uint32 fileAttrib)
            : layerName(name)
            , fileAttributes(fileAttrib)
        {}
    };

    typedef std::vector<SLayerInfo> Layers;

    //Map of layer name to layer file attributes
    typedef std::map<QString, SCacheLayerAttributes> LayersAttributeMap;

    CLayersListBox(QWidget* parent = 0);   // standard constructor
    virtual ~CLayersListBox();

    void ReloadLayers();
    void SelectLayer(const QString& layerName);
    void SelectLayer(const QModelIndex& index);
    CObjectLayer* GetCurrentLayer();
    void ExpandAll(bool isExpand);

    void SetUpdateCallback(UpdateCallback& cb);

    int GetCount() const;

signals:
    void contextMenuRequested(int item);
    void itemSelected();

private slots:
    void InvalidateListBoxFileAttributes();

protected:
    void OnLButtonDown(QMouseEvent* event);
    void OnLButtonUp(QMouseEvent* event);
    void OnLButtonDblClk(QMouseEvent* event);
    void OnRButtonDown(QMouseEvent* event);
    void OnRButtonUp(QMouseEvent* event);

    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

    enum LayerToggleFlag
    {
        LayerToggleFlagNone = 0,
        LayerToggleFlagVisibility = BIT(0),
        LayerToggleFlagUsability = BIT(1),
    };

    //void UpdateLayers();
    void AddLayerRecursively(CObjectLayer* pLayer, int level);
    void ReloadListItems();
    void OnModifyLayer(int index);
    void PopupMenu(int item);

    //This function starts the async job to get layer attribute
    void StartFileAttributeUpdateJob(QString layerName, SCacheLayerAttributes& attribute);

    //This function gets called on a timer
    void OnTimerCompleted();
    //This function updates the cache layer attributes
    void UpdateLayerAttribute(QString layerName, uint32 fileAttributes);

    //This function helps to Sync CacheData
    void SyncAttributeMap();

    //Used to toggle either the visibility and usability for a given layer and all its children.
    void ToggleLayerStates(int layerIndex, LayerToggleFlag flags);

    void ThreadWorker();

    std::vector<SLayerInfo> m_layersInfo;

    QPixmap m_dragCursorBitmap;

    UpdateCallback m_updateCalback;
    bool m_noReload = false;

    int m_draggingItem = -1;
    int m_rclickedItem = -1;
    int m_lclickedItem = -1;

    LayersAttributeMap m_layersAttributeMap;
    QTimer m_timer;
    QMutex m_mutex;
    uint m_callbackHandle; // callback handle

    AZStd::queue<LayerAttribRequest> m_layerRequestQueue;
    AZStd::mutex m_layerRequestMutex;

    AZStd::queue<LayerAttribResult> m_layerResultQueue;
    AZStd::mutex m_layerResultMutex;

    AZStd::atomic_bool m_invalidateLayerAttribs = { false };
    AZStd::atomic_bool m_refreshAllLayers = { false };
    AZStd::atomic_bool m_connectedSC = { false };
    AZStd::atomic_bool m_shutdownThread = { false };

    AZStd::thread m_workerThread;
    AZStd::semaphore m_workerSemaphore;

    AzToolsFramework::SourceControlState m_SCState = AzToolsFramework::SourceControlState::Disabled;

private:
    // AzToolsFramework::SourceControlNotificationBus::Handler:
    void ConnectivityStateChanged(const AzToolsFramework::SourceControlState state) override;

    bool isPointInExpandButton(const QPoint& point, const QModelIndex& index) const;
    void UpdateLayersFromConnection();
    void QueueLayersForUpdate();

    AZStd::atomic_bool m_threadWorkerBusy = { false };

    LayerModel* m_model;
};

Q_DECLARE_METATYPE(CLayersListBox::SCacheLayerAttributes*)
