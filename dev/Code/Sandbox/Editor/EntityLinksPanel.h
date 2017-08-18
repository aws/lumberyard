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

#ifndef CRYINCLUDE_EDITOR_ENTITYLINKSPANEL_H
#define CRYINCLUDE_EDITOR_ENTITYLINKSPANEL_H

#pragma once
// EntityLinksPanel.h : header file
//

#include "PickObjectTool.h"

#include <QWidget>
#include <QScopedPointer>

/////////////////////////////////////////////////////////////////////////////
// CEntityLinksPanel dialog
class QTableView;
class QModelIndex;
class QPickObjectButton;

namespace Ui {
    class CEntityLinksPanel;
}

class CEntityLinksPanel
    : public QWidget
    , public IPickObjectCallback
{
    // Construction
public:
    CEntityLinksPanel(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CEntityLinksPanel();

    void SetEntity(class CEntityObject* entity);

    // Implementation
protected:
    void    ReloadLinks();

    void OnRclickLinks(QPoint localPoint);
    void OnBnClickedRemove(int nCurrentIndex);
    void OnDblClickLinks(const QModelIndex& index);

    // Ovverriden from IPickObjectCallback
    void OnPick(CBaseObject* picked);
    void OnCancelPick();

    CEntityObject* m_entity;
    QPickObjectButton* m_pickButton;

    int m_currentLinkIndex;
    QTableView* m_links;
    QScopedPointer<Ui::CEntityLinksPanel> ui;
};

#endif // CRYINCLUDE_EDITOR_ENTITYLINKSPANEL_H
