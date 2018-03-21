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

#ifndef CRYINCLUDE_EDITOR_ENTITYPANEL_H
#define CRYINCLUDE_EDITOR_ENTITYPANEL_H

#pragma once
// EntityPanel.h : header file
//

#include "PickObjectTool.h"
#include "Controls/PickObjectButton.h"

#include <QSharedPointer>
#include <QScopedPointer>
#include <QWidget>
#include <QHash>

/////////////////////////////////////////////////////////////////////////////
// CEntityPanel dialog
class QPushButton;
class QGroupBox;
class QTreeWidget;
class QTreeWidgetItem;
class QListWidget;
class QListWidgetItem;

namespace Ui {
    class CEntityPanel;
    class CEntityEventsPanel;
}

class CEntityPanel
    : public QWidget
{
    Q_OBJECT
    // Construction
public:
    CEntityPanel(QWidget* pParent = nullptr);   // standard constructor

    // Dialog Data
    void SetEntity(class CEntityObject* entity);
    bool IsSetTo(CEntityObject* entity) const { return m_entity == entity; }

    // Implementation
protected slots:
    // Generated message map functions
    void OnEditScript();
    void OnReloadScript();
    void OnFileCommands();
    void OnPrototype();
    void OnBnClickedGetphysics();
    void OnBnClickedResetphysics();
    void OnBnClickedOpenFlowGraph();
    void OnBnClickedRemoveFlowGraph();
    void OnBnClickedListFlowGraphs();
    void OnBnClickedTrackViewSequence();

protected:
    QPushButton*    m_editScriptButton;
    QPushButton*    m_reloadScriptButton;
    QPushButton*    m_prototypeButton;
    QPushButton*    m_flowGraphOpenBtn;
    QPushButton*    m_flowGraphRemoveBtn;
    QPushButton*    m_flowGraphListBtn;
    QPushButton*    m_physicsBtn[2];
    QPushButton*    m_trackViewSequenceButton;
    CEntityObject*  m_entity;
    QGroupBox*      m_frame;

private:
    QSharedPointer<Ui::CEntityPanel> ui;
};

/////////////////////////////////////////////////////////////////////////////
// CEntityPanel dialog
class QPickObjectButton;

class CEntityEventsPanel
    : public QWidget
    , public IPickObjectCallback
{
    Q_OBJECT
    // Construction
public:
    CEntityEventsPanel(QWidget* pParent = nullptr);   // standard constructor
    ~CEntityEventsPanel();

    // Dialog Data
    enum
    {
        IDD = IDD_PANEL_ENTITY_EVENTS
    };
    QPushButton*        m_sendEvent;
    QPushButton*        m_runButton;
    QPushButton*        m_gotoMethodBtn;
    QPushButton*        m_addMethodBtn;
    QPushButton*        m_addMissionBtn;
    QPushButton*        m_removeButton;
    QTreeWidget*        m_eventTree;
    QPickObjectButton*  m_pickButton;   // KDAB_PORT functionality has been disabled, button never does anything
    QListWidget*        m_methods;
    QString             m_selectedMethod;

    void SetEntity(class CEntityObject* entity);

    // Implementation
protected:
    void    ReloadMethods();
    void    ReloadEvents();

    // Ovverriden from IPickObjectCallback
    void OnPick(CBaseObject* picked) override;
    void OnCancelPick() override;

    void GotoMethod(const QString& method);

protected slots:
    void OnDblclkMethods(QListWidgetItem* item);
    void OnSelChangedMethods();
    void OnGotoMethod();
    void OnAddMethod();
    void OnEventAdd();
    void OnEventRemove();
    void OnSelChangedEventTree();
    void OnRclickEventTree();
    void OnDblClickEventTree();
    void OnRunMethod();
    void OnEventSend();
    void OnBnAddMission();

protected:
    CEntityObject* m_entity;
    class CPickObjectTool* m_pickTool;

    QString m_currentSourceEvent;
    int m_currentTrgEventId;

    QScopedPointer<Ui::CEntityEventsPanel> ui;
};

#endif // CRYINCLUDE_EDITOR_ENTITYPANEL_H
