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

#ifndef CRYINCLUDE_EDITOR_SMARTOBJECTS_SMARTOBJECTSEDITORDIALOG_H
#define CRYINCLUDE_EDITOR_SMARTOBJECTS_SMARTOBJECTSEDITORDIALOG_H
#pragma once

#include <list>

//#include <uxtheme.h>

#include <IAISystem.h>
#include <Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h>

#include <QLabel>
#include <QMainWindow>

class CSmartObjectEntry;
class CSmartObjectHelperObject;
class CEntityObject;
class CSOParamBase;
class SmartObjectsPathFilterModel;
class SmartObjectsEditorModel;
class SmartObjectsEditorDialogLabel;
class ClickableLabel;
class QTextEdit;
class QTreeView;

typedef std::set< string > SetStrings;
typedef std::list< SmartObjectCondition > SmartObjectConditions;


class CSOLibrary
    : public IEditorNotifyListener
{
private:
    friend class CSmartObjectsEditorDialog;

    static bool SaveToFile(const char* sFileName);
    static bool LoadFromFile(const char* sFileName);

protected:
    ~CSOLibrary();

public:
    static void Reload();
    static void InvalidateSOEntities();

    static void String2Classes(const string& sClass, SetStrings& classes);

    struct CHelperData
    {
        QString className;
        QuatT qt;
        QString name;
        QString description;
    };
    // VectorHelperData contains all smart object helpers sorted by name of their smart object class
    typedef std::vector< CHelperData > VectorHelperData;


    struct CEventData
    {
        QString name;
        QString description;
    };
    // VectorEventData contains all smart object events sorted by name
    typedef std::vector< CEventData > VectorEventData;


    struct CStateData
    {
        QString name;
        QString description;
        QString location;
    };
    // VectorStateData contains all smart object states sorted by name
    typedef std::vector< CStateData > VectorStateData;


    struct CClassTemplateData
    {
        string name;

        struct CTemplateHelper
        {
            QString name;
            QuatT qt;
            float radius;
            bool project;
        };
        typedef std::vector< CTemplateHelper > TTemplateHelpers;

        string model;
        TTemplateHelpers helpers;
    };
    // VectorClassTemplateData contains all class templates sorted by name
    typedef std::vector< CClassTemplateData > VectorClassTemplateData;


    struct CClassData
    {
        QString name;
        QString description;
        QString location;
        QString templateName;
        CClassTemplateData const* pClassTemplateData;
    };
    // VectorClassData contains all smart object classes sorted by name
    typedef std::vector< CClassData > VectorClassData;


protected:
    // Called by the editor to notify the listener about the specified event.
    void OnEditorNotifyEvent(EEditorNotifyEvent event);

    static CClassTemplateData* AddClassTemplate(const char* name);
    static void LoadClassTemplates();

    // data
    static SmartObjectConditions    m_Conditions;
    static VectorHelperData         m_vHelpers;
    static VectorEventData          m_vEvents;
    static VectorStateData          m_vStates;
    static VectorClassTemplateData  m_vClassTemplates;
    static VectorClassData          m_vClasses;

    static int m_iNumEditors;
    static CSOLibrary* m_pInstance;

    static bool m_bSaveNeeded;
    static bool m_bLoadNeeded;

public:
    static bool StartEditing();

    static SmartObjectConditions&   GetConditions()
    {
        if (m_bLoadNeeded)
        {
            Load();
        }
        return m_Conditions;
    }
    static VectorHelperData&        GetHelpers()
    {
        if (m_bLoadNeeded)
        {
            Load();
        }
        return m_vHelpers;
    }
    static VectorEventData&         GetEvents()
    {
        if (m_bLoadNeeded)
        {
            Load();
        }
        return m_vEvents;
    }
    static VectorStateData&         GetStates()
    {
        if (m_bLoadNeeded)
        {
            Load();
        }
        return m_vStates;
    }
    static VectorClassTemplateData& GetClassTemplates()
    {
        if (m_bLoadNeeded)
        {
            Load();
        }
        return m_vClassTemplates;
    }
    static VectorClassData&         GetClasses()
    {
        if (m_bLoadNeeded)
        {
            Load();
        }
        return m_vClasses;
    }

    static bool Load();
    static bool Save();

    static VectorHelperData::iterator FindHelper(const QString& className, const QString& helperName);
    static VectorHelperData::iterator HelpersUpperBound(const QString& className);
    static VectorHelperData::iterator HelpersLowerBound(const QString& className);

    static void AddEvent(const char* name, const char* description);
    static VectorEventData::iterator FindEvent(const char* name);

    static void AddState(const char* name, const char* description, const char* location);
    static void AddAllStates(const string& sStates);
    static VectorStateData::iterator FindState(const char* name);

    static CClassTemplateData const* FindClassTemplate(const char* name);

    static void AddClass(const char* name, const char* description, const char* location, const char* templateName);
    static VectorClassData::iterator FindClass(const char* name);
};

Q_DECLARE_METATYPE(CSOLibrary::CClassData);

//////////////////////////////////////////////////////////////////////////
//
// Main Dialog, the Smart Objects Editor.
//
//////////////////////////////////////////////////////////////////////////
class CSmartObjectsEditorDialog
    : public QMainWindow
    , public IEditorNotifyListener
{
    Q_OBJECT

    friend class CDragReportCtrl;

public:
    static void RegisterViewClass();
    static const GUID& GetClassID();

    explicit CSmartObjectsEditorDialog(QWidget* parent = nullptr);
    ~CSmartObjectsEditorDialog();

    //void SetView(CString name);
    QWidget* GetTaskPanel(){ return m_taskPanel; }

    // vehicle logic
    //  void SetVehiclePrototype( CVehiclePrototype* pProt );
    //  bool ApplyToVehicle();

    void OnObjectEvent(CBaseObject* object, int event);
    void RecalcLayout(BOOL bNotify = TRUE);

    QString GetFolderPath(const QModelIndex& index) const;
    QString GetCurrentFolderPath() const;
    QModelIndex SetCurrentFolder(const QString& folder);
    void ModifyRuleOrder(int from, int to);

    void SetTemplateDefaults(SmartObjectCondition& condition, const CSOParamBase* param, QString* message = nullptr) const;

protected:
    void DeleteHelper(const QString& className, const QString& helperName);

protected:
    void OnInitDialog();
    void OnDestroy();
    void closeEvent(QCloseEvent* event) override;

public:
    void OnAddEntry();
    void OnEditEntry();
    void OnRemoveEntry();
    void OnDuplicateEntry();

    void ReloadEntries(bool bFromFile);

protected:
    void EnableIfOneSelected(QWidget* target);
    void EnableIfSelected(QWidget* target);

    void OnHelpersEdit();
    void OnHelpersNew();
    void OnHelpersDelete();
    void OnHelpersDone();

    void OnReportColumnRClick(const QPoint& point);
    void OnContextMenu(const QPoint& point);
    void OnReportSelChanged();

    void OnTreeSelChanged();

    void OnDescriptionEdit();

    void CreatePanes();

    void SinkSelection();
    void UpdatePropertyTables();

    // Called by the editor to notify the listener about the specified event.
    void OnEditorNotifyEvent(EEditorNotifyEvent event);

    // Called by the property editor control
    void OnUpdateProperties(IVariable* pVar);
    void OnTreeViewDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    bool ChangeTemplate(SmartObjectCondition* pRule, const CSOParamBase* pParam) const;

    void ParseClassesFromProperties(CBaseObject* pObject, SetStrings& classes);

public:
    // Manager.
    //  CSmartObjectsManager*   m_pManager;

private:
    bool                    m_bSinkNeeded;

protected:
    QString                 m_sNewObjectClass;
    QString                 m_sFilterClasses;
    QString                 m_sFirstFilterClass;
    QRect                   m_rcCloseBtn;
    bool                    m_bFilterCanceled;

    ReflectedPropertyControl*          m_Properties;
    CVarBlockPtr            m_vars;

    // dialog stuff
    QTreeView*              m_View;
    SmartObjectsEditorDialogLabel* m_topLabel;
    QWidget*            m_taskPanel;

    ClickableLabel*     m_pDuplicate;
    ClickableLabel*     m_pDelete;
    ClickableLabel*     m_pItemHelpersEdit;
    ClickableLabel*     m_pItemHelpersNew;
    ClickableLabel*     m_pItemHelpersDelete;
    ClickableLabel*     m_pItemHelpersDone;

    // Smart Object Helpers
    bool                    m_bIgnoreNotifications;
    QString                 m_sEditedClass;
    typedef std::multimap< CEntityObject*, CSmartObjectHelperObject* > MapHelperObjects;
    MapHelperObjects        m_mapHelperObjects;

public:
    // tree view
    QTreeView* m_Tree;
    SmartObjectsPathFilterModel* m_pathModel;
    SmartObjectsEditorModel* m_model;

private:
    // Description
    QTextEdit* m_Description;

    SmartObjectConditions::iterator FindRuleByPtr(const SmartObjectCondition* pCondition);
    bool CheckOutLibrary();
    bool SaveSOLibrary(bool updatePropertiesFirst = true);
};

class SmartObjectsEditorDialogLabel : public QLabel
{
    Q_OBJECT
public:
    SmartObjectsEditorDialogLabel(QWidget* parent = nullptr)
        : QLabel(parent)
    {
        setMouseTracking(true);
    }

    QSize sizeHint() const override
    {
        return QLabel::sizeHint().expandedTo(QSize(0, 23));
    }

signals:
    void closeRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void leaveEvent(QEvent* event) override { update(); }
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QRect m_rcCloseBtn;
    bool m_closing = false;
};

Q_DECLARE_METATYPE(SmartObjectCondition*)

#endif // CRYINCLUDE_EDITOR_SMARTOBJECTS_SMARTOBJECTSEDITORDIALOG_H
