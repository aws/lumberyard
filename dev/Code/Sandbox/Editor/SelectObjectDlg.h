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
#ifndef CRYINCLUDE_EDITOR_SELECTOBJECTDLG_H
#define CRYINCLUDE_EDITOR_SELECTOBJECTDLG_H
#include "HyperGraph/IHyperGraph.h"

#include <QDialog>
#include <QScopedPointer>
#include <QList>
#include <QItemSelection>
#include <QTimer>

namespace Ui {
    class SELECT_OBJECT;
}

class CBaseObject;
class ObjectSelectorTableView;
class ObjectSelectorModel;
class QCheckBox;
class QShowEvent;

class CSelectObjectDlg
    : public QWidget
    , public IEditorNotifyListener
    , public IHyperGraphManagerListener
{
    Q_OBJECT
public:
    explicit CSelectObjectDlg(QWidget* parent = nullptr);
    ~CSelectObjectDlg();

    //! Single instance of this dialog.
    static CSelectObjectDlg* m_instance;
    static void RegisterViewClass();
    static const GUID& GetClassID()
    {
        // {E9D27BFE-6FC3-425a-9846-E1E52CEBB693}
        static const GUID guid = {
            0xe9d27bfe, 0x6fc3, 0x425a, { 0x98, 0x46, 0xe1, 0xe5, 0x2c, 0xeb, 0xb6, 0x93 }
        };
        return guid;
    }
    static CSelectObjectDlg* GetInstance();

private Q_SLOTS:
    void UpdateDisplayMode();
    void UpdateObjectMask();
    void InvertSelection();
    void SelectAll();
    void SelectNone();
    void Select();
    void SelectAllTypes();
    void SelectNoneTypes();
    void InvertTypes();
    void HideSelected();
    void FreezeSelected();
    void DeleteSelected();
    void Refresh();
    void FilterByName();
    void FilterByValue();
    void SearchAllObjectsToggled();
    void AutoSelectToggled();
    void DisplayAsTreeToggled();
    void FastSelectFilterChanged();
    void UpdateCountLabel();
    void OnVisualSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void OnDoubleClick(const QModelIndex&);
    void InvalidateFilter();

protected:
    void OnEditorNotifyEvent(EEditorNotifyEvent ev) override;
    void OnHyperGraphManagerEvent(EHyperGraphEvent ev, IHyperGraph* pGraph, IHyperNode* pINode) override;
    void showEvent(QShowEvent* ev) override;
    void OnLayerUpdate(int event, CObjectLayer* pLayer);
    void OnObjectEvent(CBaseObject* pObject, int ev);

private:
    QList<QCheckBox*> ObjectTypeCheckBoxes() const;
    void setTrackViewModified(bool);
    void ApplyListSelectionToObjectManager();
    void AutoSelectItemObject(int iItem);
    bool isRowSelected(int row) const;
    void UpdateButtonsEnabledState();

    QScopedPointer<Ui::SELECT_OBJECT> ui;
    QScopedPointer<ObjectSelectorModel> m_model;
    bool m_bIgnoreCallbacks = false;
    bool m_bAutoselect = false;
    bool m_picking = false;
    bool m_bIsLinkTool = false;
    bool m_bLayerModified = false;
    bool m_initialized = false;
    bool m_disableInvalidateFilter = false;
    bool m_ignoreSelectionChanged = false;
    QTimer m_invalidateTimer;
};

#endif // CRYINCLUDE_EDITOR_SELECTOBJECTDLG_H
