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

#include "stdafx.h"
#include "DockablePreviewPanel.h"
#include "DockWidgetTitleBar.h"
#include "PreviewWindowView.h"

//QT
#include <QShortcut>

//Editor
#include <IEditor.h>
#include <IEditorParticleUtils.h>
#include <BaseLibraryItem.h>
#include <Particles/ParticleItem.h>

DockablePreviewPanel::DockablePreviewPanel(QWidget* parent)
    : FloatableDockPanel("", parent)
    , m_titleBar(nullptr)
    , m_previewWindowView(nullptr)
{
}

DockablePreviewPanel::~DockablePreviewPanel()
{
    if (m_titleBar)
    {
        delete m_titleBar;
        m_titleBar = nullptr;
    }

    if (m_previewWindowView)
    {
        delete m_previewWindowView;
        m_previewWindowView = nullptr;
    }
}

void DockablePreviewPanel::Init(const QString& panelName)
{
    //Setup titlebar
    m_titleBar = new DockWidgetTitleBar(this);
    m_titleBar->SetupLabel(panelName);
    m_titleBar->SetShowMenuContextMenuCallback([&] {return GetTitleBarMenu();
        });

    //Setup preview
    m_previewWindowView = new CPreviewWindowView(this);
    m_previewWindowView->setBaseSize(QSize(256, 256));
    m_previewWindowView->setObjectName("dwPreviewCtlWidget");

    //setup the Dock Widget
    setTitleBarWidget(m_titleBar);
    setWindowTitle(panelName);
    connect(this, &QDockWidget::windowTitleChanged, m_titleBar, &DockWidgetTitleBar::SetTitle);
    setAllowedAreas(Qt::AllDockWidgetAreas);
    setMinimumWidth(256);
    setSizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Preferred);
    m_previewWindowView->setFocusPolicy(Qt::StrongFocus);
    setWidget(m_previewWindowView);
    SetHotkeyHandler(m_previewWindowView, [&](QKeyEvent* e){m_previewWindowView->HandleHotkeys(e); });
    SetShortcutHandler(m_previewWindowView, [&](QShortcutEvent* e){return m_previewWindowView->HandleShortcuts(e); });
}

void DockablePreviewPanel::ResetToDefaultLayout()
{
    m_previewWindowView->ResetToDefaultLayout();
}

void DockablePreviewPanel::LoadSessionState(QByteArray data)
{
    if (data.isEmpty())
    {
        return;
    }
    CRY_ASSERT(m_previewWindowView);
    m_previewWindowView->LoadSessionState(data);
}

void DockablePreviewPanel::LoadSessionState(QString path)
{
    CRY_ASSERT(m_previewWindowView);
    m_previewWindowView->LoadSessionState(path);
}

void DockablePreviewPanel::SaveSessionState(QString path)
{
    CRY_ASSERT(m_previewWindowView);
    m_previewWindowView->SaveSessionState(path);
}

void DockablePreviewPanel::SaveSessionState(QByteArray& out)
{
    CRY_ASSERT(m_previewWindowView);
    m_previewWindowView->SaveSessionState(out);
}

void DockablePreviewPanel::ForceParticleEmitterRestart()
{
    CRY_ASSERT(m_previewWindowView);
    m_previewWindowView->ForceParticleEmitterRestart();
}

QShortcut* DockablePreviewPanel::GetShortcut(Shortcuts item)
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());
    CRY_ASSERT(m_previewWindowView);
    QShortcut* created = nullptr;
    switch (item)
    {
    case Shortcuts::FocusCameraOnEmitter:
    {
        created = new QShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Previewer.Focus"), this);
        connect(created, &QShortcut::activated, this, [&]()
            {
                if (!GetIEditor()->GetParticleUtils()->HotKey_IsEnabled())
                {
                    return;
                }
                m_previewWindowView->FocusCameraOnEmitter();
            });
    }
    break;
    default:
        break;
    }

    return created;
}

void DockablePreviewPanel::ItemSelectionChanged(CBaseLibraryItem* item)
{
    CRY_ASSERT(m_previewWindowView);
    if (!item)
    {
        m_previewWindowView->SetActiveEffect(nullptr, nullptr);
    }
    else if (item->GetType() == EDataBaseItemType::EDB_TYPE_PARTICLE)
    {
        m_previewWindowView->SetActiveEffect(static_cast<CParticleItem*>(item), nullptr);
    }
    else
    {
        CRY_ASSERT(0 && "Unsupported item type being passed to DockablePreviewPanel::ItemSelectionChanged");
    }
}

void DockablePreviewPanel::LodSelectionChanged(CBaseLibraryItem* item, SLodInfo* lod)
{
    CRY_ASSERT(m_previewWindowView);
    if (!item)
    {
        m_previewWindowView->SetActiveEffect(nullptr, nullptr);
    }
    else if (item->GetType() == EDataBaseItemType::EDB_TYPE_PARTICLE)
    {
        m_previewWindowView->SetActiveEffect(static_cast<CParticleItem*>(item), lod);
    }
    else
    {
        CRY_ASSERT(0 && "Unsupported item type being passed to DockablePreviewPanel::ItemSelectionChanged");
    }
}

QMenu* DockablePreviewPanel::GetTitleBarMenu()
{
    CRY_ASSERT(m_previewWindowView);
    QMenu* menu = m_previewWindowView->ExecPreviewMenu();
    emit SignalPopulateTitleBarMenu(menu);
    return menu;
}

#include <DockablePreviewPanel.moc>