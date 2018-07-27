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
//#include "api.h"

//Editor
#include <Util/Variable.h>

//EditorUI_QT
#include <../EditorUI_QT/FloatableDockPanel.h>

#include <AzCore/std/functional.h>

class DockWidgetTitleBar;
class QMenu;
class CAttributeView;
class CParticleUIDefinition;
class FluidTabBar;
class CParticleItem;
class ContextMenu;
class DefaultViewWidget;
class CBaseLibraryManager;
struct IVariableContainer;
class CParticleEffect;
struct SLodInfo;

struct ParticleTabData
{
    bool m_isNewTab;
    QString m_libraryName;
    QString m_libraryItemName;
};

Q_DECLARE_METATYPE(ParticleTabData)

class /*EDITOR_QT_UI_API*/ DockableAttributePanel
    : public FloatableDockPanel
{
    Q_OBJECT
public:
    DockableAttributePanel(QWidget* parent);
    ~DockableAttributePanel();

    void Init(const QString& panelName, CBaseLibraryManager* mgr);

    void LoadSessionState();
    void SaveSessionState();

    void ResetToDefaultLayout();

    void LoadAttributeLayout(QByteArray data);
    void SaveAttributeLayout(QByteArray& out);

    enum class ParamShortcuts
    {
        InsertComment = 0,
    };

    QShortcut* GetShortcut(ParamShortcuts item);

    enum class TabActions
    {
        CLOSE = 0,
        CLOSE_ALL,
        CLOSE_ALL_BUT_THIS,
    };

    QAction* GetMenuAction(TabActions action, const QString& contextItemName, QString displayAlias, QWidget* owner = nullptr);

    void AddLayoutMenuItemsTo(QMenu* menu); //Pass through to the CAttributeView would be better setup with GetMenuAction like requests. (see above)

    void OpenInTab(CBaseLibraryItem* item); //this will add a new tab for the item.

    ///////////////////////////////////////////////
    //This is exposed so that we can access the Emitter.Enabled parameter due to the desire to toggle this though other UI not just the attribute view.
    bool GetEnabledParameterValue(CParticleItem* item) const;
    void ToggleEnabledParameter(CParticleItem* item);
    void SetEnabledParameter(CParticleItem* item, bool enable);
    ///////////////////////////////////////////////

    //Exposed so we can reload the ui after resetting an emitter
    void RefreshParameterUI(CParticleItem* item, SLodInfo* lod = nullptr);

    void UpdateColors(const QMap<QString, QColor>& colorMap);
    void ShowDefaultView();
    void ShowAttributeView();

    void RefreshCurrentTab();
    QString GetCurrentTabText();

    void CloseTab(const QString& name);
    void CloseAllTabs();

    SLodInfo* GetCurrentLod() { return m_currentLod; };

signals:
    void SignalPopulateTitleBarMenu(QMenu* toPopulate);
    void SignalTabSelectionChanged(const QString& libraryName, const QString& itemName, const bool forceSelection = false);
    void SignalPopulateTabBarContextMenu(const QString& libraryName, const QString& itemName, ContextMenu* toPopulate);

    //POSSIBLE LEGACY ONLY SYNC USAGE HERE
    void SignalParameterChanged(const QString& libraryName, const QString& itemName); //emitted when a parameter has changed
    //POSSIBLE LEGACY ONLY SYNC USAGE HERE

    ///////////////////////////////////////////////
    //This is exposed so that we can access the Emitter.Enabled parameter due to the desire to toggle this though other UI not just the attribute view.
    void SignalEnabledParameterChanged(const QString& libraryName, const QString& itemName);
    ///////////////////////////////////////////////

    void SignalBuildCustomAttributeList(QMenu* menu);
    void SignalAddPreset(QString panelName, QString paneldata);
    void SignalResetPresetList();
    void SignalItemUndoPoint(const QString& itemName, bool selected, SLodInfo* lod);

    void SignalLODParticleHighLight(bool isHighLight, SLodInfo* lod, QString itemFullName);

public slots:
    void ItemSelectionChanged(CBaseLibraryItem* item);
    void LodItemSelectionChanged(CBaseLibraryItem* item, SLodInfo* lod);
    void ItemNameChanged(CBaseLibraryItem* item, const QString& oldName, const QString& currentName, const QString newlib = "");
    void UpdateItemName(const QString& fullOldName, const QString& fullNewName);
    void ImportPanelFromQString(QString filedata);
    void PassThroughSignalBuildPanelList(QMenu* menu);
    void PassThroughSignalAddPreset(QString panelName, QString data);
    void OnItemUndoPoint(const QString& itemName);
    void RefreshCurrentParameterUI();

private slots:
    //Tabbar
    void TabSelectionChange(int index);
    void TabClosing(int index);
    void TabMoved(int from, int to);
    void TabShowContextMenu(const QPoint& pos);
    void OnInsertCommentTriggered();
    void OnEnabledToggleTriggered();

private: //Functions
    QMenu* GetTitleBarMenu();

    DockWidgetTitleBar* CreateTabBar();
    void RefreshOpenTabColors();

    //Tabs
    int FindItemInTabs(QString& itemName) const;
    void SelectTab(unsigned int tabIndex);
    void AddToDynamicTab(QString& tabTitle, ParticleTabData& data);

    QColor GetTabTextColorFor(CParticleItem* item) const;

    void SetOnVariableChangeCallbackRecurse(IVariableContainer* vars);
    void OnVariableChange(IVariable* var);


    void RefreshGroupNodeSettings();

private: //Variables
    SLodInfo*               m_currentLod;
    CBaseLibraryManager*    m_libraryManager;

    CVarBlockPtr m_vars;

    QColor m_enabledTabTextColor;
    QColor m_disabledTabTextColor;

    DockWidgetTitleBar* m_titleBar;
    CAttributeView* m_attributeView;
    FluidTabBar* m_openParticleTabBar;
    CParticleUIDefinition* m_pParticleUI;
    QMenu* m_titleBarMenu;

    DefaultViewWidget* m_defaultView;

    int m_TabBarDynamicTabIndex;

    bool m_IgnoreAttributeRefresh;
};