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

#ifndef CRYINCLUDE_EDITOR_PREFABPANEL_H
#define CRYINCLUDE_EDITOR_PREFABPANEL_H
#pragma once



#include "Controls/PickObjectButton.h"
#include <QWidget>
#include <QScopedPointer>

class CBaseObject;
class CPrefabObject;
// CPrefabPanel dialog

namespace Ui  {
    class CPrefabPanel;
}
class QTableWidget;
class QTableWidgetItem;
class QAbstractButton;
class QPushButton;
class QToolButton;
class QLineEdit;


class CPrefabPanel
    : public QWidget
    , public IPickObjectCallback
{
public:
    CPrefabPanel(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CPrefabPanel();

    void SetObject(CPrefabObject* object);
    void OnObjectEvent(CBaseObject* pObject, int nEvent);

protected:
    //////////////////////////////////////////////////////////////////////////
    // Override IPickObjectCallback
    void OnPick(CBaseObject* picked) override;
    bool OnPickFilter(CBaseObject* picked) override;
    void OnCancelPick() override;
    //////////////////////////////////////////////////////////////////////////

    void OnContextMenu();
    void OnDblclkObjects(QTableWidgetItem* item);
    void OnItemchangedObjects();

    void OnBnClickedPick() {};
    void OnBnClickedGotoPrefab();
    void OnBnClickedDelete();
    void OnBnClickedCloneSelected();
    void OnBnClickedCloneAll();
    void OnBnClickedExtractSelected();
    void OnBnClickedExtractAll();
    void OnBnClickedOpen();
    void OnBnClickedClose();
    void OnBnClickedChangePivot();
    void OnBnClickedOpenAll();
    void OnBnClickedCloseAll();

    void ReloadObjects();
    void ReloadListCtrl();

    //////////////////////////////////////////////////////////////////////////
protected:
    //////////////////////////////////////////////////////////////////////////
    struct ChildRecord
    {
        _smart_ptr<CBaseObject> pObject;
        int level;
        bool bSelected;
    };
    void RecursivelyGetAllPrefabChilds(CBaseObject* obj, std::vector<ChildRecord>& childs, int level);

    void GetAllPrefabObjects(std::vector<CPrefabObject*>& outPrefabObjects);
    void SetPrefabPivotModeOnCurrentEditTool(bool pivotModeOn);
    int GetSelectedObjects(std::vector<CBaseObject*>& selectedObjects);

protected:
    CPrefabObject* m_pPrefabObject;

    std::vector<QAbstractButton*> m_prefabButtons;

    bool m_changePivotMode;
    bool m_updatingObjectList;
    int m_type;

    std::vector<ChildRecord> m_childObjects;
    QScopedPointer<Ui::CPrefabPanel> ui;
};

#endif // CRYINCLUDE_EDITOR_PREFABPANEL_H

