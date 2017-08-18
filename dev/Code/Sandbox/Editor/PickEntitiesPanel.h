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

#ifndef CRYINCLUDE_EDITOR_PICKENTITIESPANEL_H
#define CRYINCLUDE_EDITOR_PICKENTITIESPANEL_H
#pragma once

#include <QWidget>
#include <QScopedPointer>

class QListWidget;
class QListWidgetItem;
class CBaseObject;
class QPickObjectButton;
struct IPickEntitesOwner;

namespace Ui {
    class CPickEntitiesPanel;
}


// CShapePanel dialog
class CPickEntitiesPanel
    : public QWidget
    , public IPickObjectCallback
{
public:
    CPickEntitiesPanel(QWidget* pParent = NULL);   // standard constructor
    virtual ~CPickEntitiesPanel();

    void SetOwner(IPickEntitesOwner* pOwner);
    IPickEntitesOwner* GetOwner() const { return m_pOwner; }

protected:
    void OnBnClickedSelect();
    void OnBnClickedRemove();
    void OnLbnDblclkEntities(QListWidgetItem* item);

    // Ovverriden from IPickObjectCallback
    void OnPick(CBaseObject* picked) override;
    bool OnPickFilter(CBaseObject* picked) override;
    void OnCancelPick() override;

    void ReloadEntities();

    IPickEntitesOwner* m_pOwner;
    QListWidget* m_entities;

    QScopedPointer<Ui::CPickEntitiesPanel> ui;
};

#endif // CRYINCLUDE_EDITOR_PICKENTITIESPANEL_H
