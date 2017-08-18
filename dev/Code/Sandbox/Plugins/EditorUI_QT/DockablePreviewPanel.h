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
#include "api.h"
#include "FloatableDockPanel.h"

class DockWidgetTitleBar;
class CPreviewWindowView;
class QMenu;
struct SLodInfo;
class QShortcut;

class EDITOR_QT_UI_API DockablePreviewPanel
    : public FloatableDockPanel
{
    Q_OBJECT
public:
    DockablePreviewPanel(QWidget* parent);
    ~DockablePreviewPanel();

    void Init(const QString& panelName);

    void ResetToDefaultLayout();

    void LoadSessionState(QString path);
    void LoadSessionState(QByteArray data = QByteArray());
    void SaveSessionState(QString path = QString());
    void SaveSessionState(QByteArray& out);
    void ForceParticleEmitterRestart();

    enum class Shortcuts
    {
        FocusCameraOnEmitter = 0,
    };

    QShortcut* GetShortcut(Shortcuts item);

signals:
    void SignalPopulateTitleBarMenu(QMenu* toPopulate);

public slots:
    void ItemSelectionChanged(CBaseLibraryItem* item);
    void LodSelectionChanged(CBaseLibraryItem* item, SLodInfo* lod);

private: //Functions
    QMenu* GetTitleBarMenu();

private: //Variables
    DockWidgetTitleBar* m_titleBar;
    CPreviewWindowView* m_previewWindowView;
};