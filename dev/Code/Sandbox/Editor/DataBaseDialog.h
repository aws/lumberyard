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

#ifndef CRYINCLUDE_EDITOR_DATABASEDIALOG_H
#define CRYINCLUDE_EDITOR_DATABASEDIALOG_H
#pragma once


#include "IDataBaseItem.h"
#include "Resource.h"
#include <QMainWindow>

#define DATABASE_VIEW_VER "1.00"

class CEntityProtLibDialog;
namespace AzQtComponents
{
    class TabWidget;
}

//////////////////////////////////////////////////////////////////////////
// Pages of the data base dialog.
//////////////////////////////////////////////////////////////////////////
class CDataBaseDialogPage
    : public QMainWindow
{
    Q_OBJECT
public:
    CDataBaseDialogPage(unsigned int nIDTemplate, QWidget* pParentWnd = 0)
        : QMainWindow(pParentWnd) {};
    CDataBaseDialogPage(QWidget* pParentWnd = 0)
        : QMainWindow(pParentWnd) {};

    //! This dialog is activated.
    virtual void SetActive(bool bActive) = 0;
    virtual void Update() = 0;
};

/** Main dialog window of DataBase window.
*/
class CDataBaseDialog
    : public QWidget
{
    Q_OBJECT

public:
    CDataBaseDialog(QWidget* pParent = 0);
    virtual ~CDataBaseDialog();

    static void RegisterViewClass();
    static const GUID& GetClassID();

    enum
    {
        IDD = IDD_DATABASE
    };

    //! Select Object/Terrain
    CDataBaseDialogPage* SelectDialog(EDataBaseItemType type, IDataBaseItem* pItem = NULL);

    CDataBaseDialogPage* GetPage(int num);
    int GetSelection() const;

    CDataBaseDialogPage* GetCurrent();

    //! Called every frame.
    void Update();

protected:
    virtual void OnOK() {};
    virtual void OnCancel() {};
    virtual void PostNcDestroy();

    BOOL OnInitDialog();

    //! Activates/Deactivates dialog window.
    void Activate(CDataBaseDialogPage* dlg, bool bActive);
    void Activate(int index);
    void Select(int num);

    CDataBaseDialogPage* GetPageFromTab(QWidget* widget);

    // prevent sending the metrics event when the 
    // Database View is just open
    bool m_isReady = false;

    AzQtComponents::TabWidget* m_tabCtrl;

    //////////////////////////////////////////////////////////////////////////
    // Database dialogs.
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Menu.
    //CXTMenuBar m_menubar;
};

#endif // CRYINCLUDE_EDITOR_DATABASEDIALOG_H
