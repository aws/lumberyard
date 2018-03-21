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

//QT
#include <QSettings>

//Editor

#include <EditorDefs.h>
#include <BaseLibraryManager.h>
#include <IEditor.h>
#include <IEditorParticleUtils.h>
#include <Util/Variable.h>
#include <Particles/ParticleUIDefinition.h>
#include <Particles/ParticleItem.h>
#include <BaseLibraryItem.h>
#include <Include/IEditorParticleManager.h>

//EditorUI_QT
#include <../EditorUI_QT/DockWidgetTitleBar.h>
#include <../EditorUI_QT/AttributeView.h>
#include <../EditorUI_QT/FluidTabBar.h>
#include <../EditorUI_QT/ContextMenu.h>
#include <../EditorUI_QT/Utils.h>
#include <EditorUI_QTDLLBus.h>

// AZ
#include <AzFramework/API/ApplicationAPI.h>

//Local
#include "QT/DockableAttributePanel.h"


DockableAttributePanel::DockableAttributePanel(QWidget* parent)
    : FloatableDockPanel("", parent)
    , m_titleBar(nullptr)
    , m_attributeView(nullptr)
    , m_openParticleTabBar(nullptr)
    , m_pParticleUI(nullptr)
    , m_titleBarMenu(nullptr)
    , m_TabBarDynamicTabIndex(-1)
    , m_tabBarContextMenu(nullptr)
    , m_IgnoreAttributeRefresh(false)
{
    m_vars.reset();

    //delete previous saved panel layout since it doesn't include customized attribute configration 
    QSettings settings("Amazon", "Lumberyard");
    if (settings.value(PARTICLE_EDITOR_LAYOUT_SETTINGS_ATTRIBUTES_OLD).isNull() == false)
    {
        settings.remove(PARTICLE_EDITOR_LAYOUT_SETTINGS_ATTRIBUTES_OLD);
        settings.sync();
    }
}

DockableAttributePanel::~DockableAttributePanel()
{
    m_vars.reset();
    SAFE_DELETE(m_titleBar);
    SAFE_DELETE(m_openParticleTabBar);
    SAFE_DELETE(m_pParticleUI);
    SAFE_DELETE(m_titleBarMenu);
    SAFE_DELETE(m_tabBarContextMenu);
    SAFE_DELETE(m_attributeView);
}

void DockableAttributePanel::Init(const QString& panelName, CBaseLibraryManager* libraryManager)
{
    //Setup titlebar
    m_titleBar = new DockWidgetTitleBar(this);
    m_titleBar->SetupLabel(panelName);
    m_titleBar->SetShowMenuContextMenuCallback([&] {return GetTitleBarMenu();
        });
    setTitleBarWidget(m_titleBar);
    m_titleBarMenu = new QMenu;

    m_attributeView = new CAttributeView(this);
    m_attributeView->setObjectName("dwAttributeViewWidget");
    m_attributeView->setTitleBarWidget(CreateTabBar());
    m_attributeView->setFeatures(QDockWidget::NoDockWidgetFeatures);
    m_attributeView->setRefreshCallback([this](){ RefreshCurrentTab(); });

    //Setup Dock window
    setObjectName("dwAttributeView");
    setAllowedAreas(Qt::AllDockWidgetAreas);
    setWidget(m_attributeView);

    m_pParticleUI = new CParticleUIDefinition();
    m_vars = m_pParticleUI->CreateVars();

    m_attributeView->CreateDefaultConfigFile(m_vars);

    AZStd::string defaultAttributeViewLayoutPath(DEFAULT_ATTRIBUTE_VIEW_LAYOUT_PATH);
    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ResolveEnginePath, defaultAttributeViewLayoutPath);

    CAttributeViewConfig* attributeViewConfig = m_attributeView->CreateConfigFromFile(QString(defaultAttributeViewLayoutPath.c_str()));


    CRY_ASSERT(attributeViewConfig);
    m_attributeView->SetConfiguration(*attributeViewConfig, m_vars);
    SetOnVariableChangeCallbackRecurse(m_vars.get());

    //display helper text
    setWindowTitle("Attributes");
    m_attributeView->ShowDefaultView();
    connect(m_attributeView, &CAttributeView::SignalRefreshAttributeView, this, &DockableAttributePanel::RefreshGroupNodeSettings);
    connect(m_attributeView, &CAttributeView::SignalBuildCustomAttributeList, this, &DockableAttributePanel::PassThroughSignalBuildPanelList);
    connect(m_attributeView, &CAttributeView::SignalAddAttributePreset, this, &DockableAttributePanel::PassThroughSignalAddPreset);
    connect(m_attributeView, &CAttributeView::SignalResetPresetList, this, [=](){emit SignalResetPresetList(); });
    connect(m_attributeView, &CAttributeView::SignalRefreshAttributes, this, &DockableAttributePanel::RefreshCurrentParameterUI);
    connect(m_attributeView, &CAttributeView::SignalGetCurrentTabName, this, [=](QString& name){ name = GetCurrentTabText(); });
    connect(m_attributeView, &CAttributeView::SignalVarialbeUpdateHighlight, this, [=](bool ishighlight)
        {
            emit SignalLODParticleHighLight(ishighlight, m_currentLod, m_openParticleTabBar->tabText(m_openParticleTabBar->currentIndex()));
        });
    connect(m_attributeView, &CAttributeView::SignalIgnoreAttributeRefresh, this, [=](bool ignored) { m_IgnoreAttributeRefresh = ignored; });

    connect(m_attributeView, &CAttributeView::SignalItemUndoPoint, this, &DockableAttributePanel::OnItemUndoPoint);


    m_libraryManager = libraryManager;

    //pre-build attribute view's contents
    m_attributeView->FillFromConfiguration();
    //save default panel before load saved layout so it can be used for reset
    m_attributeView->getPanelWidget()->finalizePanels();
}

void DockableAttributePanel::LoadAttributeLayout(QByteArray data)
{
    CRY_ASSERT(m_attributeView);
    m_attributeView->LoadAttributeConfig(data);
}

void DockableAttributePanel::SaveAttributeLayout(QByteArray& out)
{
    CRY_ASSERT(m_attributeView);
    m_attributeView->SaveAttributeConfig(out);
}

void DockableAttributePanel::LoadSessionState()
{
    //load previous saved layout
    QSettings settings("Amazon", "Lumberyard");
    if (settings.value(PARTICLE_EDITOR_LAYOUT_SETTINGS_ATTRIBUTES).isNull() == false && m_attributeView)
    {
        LoadAttributeLayout(settings.value(PARTICLE_EDITOR_LAYOUT_SETTINGS_ATTRIBUTES).toByteArray());
    }
}

void DockableAttributePanel::SaveSessionState()
{
    CRY_ASSERT(m_attributeView);

    QByteArray saveData;
    SaveAttributeLayout(saveData);

    QSettings settings("Amazon", "Lumberyard");
    settings.setValue(PARTICLE_EDITOR_LAYOUT_SETTINGS_ATTRIBUTES, saveData);
    settings.sync();
}

void DockableAttributePanel::ResetToDefaultLayout()
{
    CRY_ASSERT(m_attributeView);

    //delete saved layout
    QSettings settings("Amazon", "Lumberyard");
    settings.remove(PARTICLE_EDITOR_LAYOUT_SETTINGS_ATTRIBUTES);
    settings.sync();

    //reset
    m_attributeView->ResetToDefaultLayout();
}

QShortcut* DockableAttributePanel::GetShortcut(ParamShortcuts item)
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());
    CRY_ASSERT(m_attributeView);
    QShortcut* shortcut = nullptr;
    switch (item)
    {
    case DockableAttributePanel::ParamShortcuts::InsertComment:
    {
        shortcut = new QShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Insert Comment"), this);
        connect(shortcut, &QShortcut::activated, this, &DockableAttributePanel::OnInsertCommentTriggered);
    }
    break;
    default:
        break;
    }

    return shortcut;
}

QAction* DockableAttributePanel::GetMenuAction(TabActions action, const QString& contextItemName, QString displayAlias, QWidget* owner /* = nullptr*/)
{
    CRY_ASSERT(m_openParticleTabBar);

    QAction* act = nullptr;
    if (owner)
    {
        act = new QAction(displayAlias, owner);
    }
    else
    {
        act = new QAction(displayAlias, this);
    }
    switch (action)
    {
    case DockableAttributePanel::TabActions::CLOSE:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                m_openParticleTabBar->CloseTab(contextItemName);
            });
    }
    break;
    case DockableAttributePanel::TabActions::CLOSE_ALL:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                m_IgnoreAttributeRefresh = true;     //Ignore normal behavior as the refresh is not needed as we close each tab.
                m_openParticleTabBar->CloseAll();
                m_IgnoreAttributeRefresh = false;     //Allow refresh again.
            });
    }
    break;
    case DockableAttributePanel::TabActions::CLOSE_ALL_BUT_THIS:
    {
        connect(act, &QAction::triggered, this, [=]()
            {
                QString cntxtItemName = contextItemName.toUtf8();
                int tabIndex = FindItemInTabs(cntxtItemName);
                CRY_ASSERT(tabIndex != -1);
                m_IgnoreAttributeRefresh = true;     //Ignore normal behavior as the refresh is not needed as we close each tab.
                m_openParticleTabBar->CloseAllBut(tabIndex);
                m_IgnoreAttributeRefresh = false;     //Allow refresh again.
                tabIndex = FindItemInTabs(cntxtItemName);
                CRY_ASSERT(tabIndex != -1);
                SelectTab(tabIndex);     //This is being done to refresh the data and focus on the desired tab since it will be the only one remaining once this completes
            });
    }
    break;
    default:
    {
        act->setParent(nullptr);
        SAFE_DELETE(act);
        act = nullptr;
        break;
    }
    }
    return act;
}

void DockableAttributePanel::AddLayoutMenuItemsTo(QMenu* menu) //Pass through to the CAttributeView would be better setup with GetMenuAction like requests.
{
    CRY_ASSERT(m_attributeView);
    m_attributeView->AddLayoutMenu(menu);
}

void DockableAttributePanel::OpenInTab(CBaseLibraryItem* item)
{
    CRY_ASSERT(item);
    if (!item->IsParticleItem) // Return if the item is a folder
    {
        return;
    }
    QString particleLongName = item->GetFullName();

    int tabIndex = FindItemInTabs(particleLongName);
    if (tabIndex != -1) //Item is already in a tab
    {
        //If the tabIndex is the "dynamic" tab then we need to make it a "static" tab by resetting the dynamic tab index
        if (tabIndex == m_TabBarDynamicTabIndex)
        {
            m_TabBarDynamicTabIndex = -1;
        }
    }
    else //Item is not in a tab yet so add it as a "static" tab
    {
        QString library = item->GetLibrary()->GetName();
        QString tabTitle = item->GetFullName();

        ParticleTabData data;
        data.m_libraryItemName = particleLongName;
        data.m_libraryName = library;

        tabIndex = m_openParticleTabBar->addTab(tabTitle);
        data.m_isNewTab = true;//Mark the tab to be newly created, otherwise the m_openParticleTabBar::currentChanged event will cause an extra reload
        QVariant d;
        d.setValue(data);
        m_openParticleTabBar->setTabData(tabIndex, d);
        m_openParticleTabBar->setTabTextColor(tabIndex, GetTabTextColorFor(static_cast<CParticleItem*>(item)));
        if (m_openParticleTabBar->count() == 1)
        {
            ItemSelectionChanged(item);
            emit SignalTabSelectionChanged(QString(item->GetLibrary()->GetName()), QString(item->GetName()), true);
        }
    }
}

bool DockableAttributePanel::GetEnabledParameterValue(CParticleItem* item) const
{
    CRY_ASSERT(item);
    IParticleEffect* effect = item->GetEffect();
    if (effect)
    {
        return effect->IsEnabled();
    }
    return false;
}

void DockableAttributePanel::ToggleEnabledParameter(CParticleItem* item)
{
    CRY_ASSERT(item);
    SetEnabledParameter(item, !GetEnabledParameterValue(item));
}

void DockableAttributePanel::SetEnabledParameter(CParticleItem* item, bool enable)
{
    CRY_ASSERT(item);
    bool previousState = !enable;
    IParticleEffect* effect = item->GetEffect();
    if (effect)
    {
        previousState = effect->IsEnabled();
        effect->SetEnabled(enable);
    }
    if (previousState == enable)
    {
        return;
    }
    RefreshOpenTabColors();
}

void DockableAttributePanel::UpdateColors(const QMap<QString, QColor>& colorMap)
{
    m_enabledTabTextColor = Qt::red;
    m_disabledTabTextColor = Qt::red;

    if (colorMap.contains("CDockWidgetTitleBarColor"))
    {
        m_enabledTabTextColor = colorMap["CDockWidgetTitleBarColor"];
    }

    if (colorMap.contains("CTextEditDisabledText"))
    {
        m_disabledTabTextColor = colorMap["CTextEditDisabledText"];
    }
    RefreshOpenTabColors();
}

void DockableAttributePanel::ItemSelectionChanged(CBaseLibraryItem* item)
{
    EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
    EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);

    m_currentLod = nullptr;
    CRY_ASSERT(m_attributeView);

    if (item && item->IsParticleItem)
    {
        m_attributeView->OnStartReload();
        m_IgnoreAttributeRefresh = true; //ignore updates
        {
            QString particleLongName = item->GetFullName();
            int tabIndex = FindItemInTabs(particleLongName);
            if (tabIndex != -1) //Item is already in a tab so select that tab
            {
                SelectTab(tabIndex);
            }
            else //Item is not in a tab yet so add it to the "dynamic" tab
            {
                QString library = item->GetLibrary()->GetName();
                QString tabTitle = item->GetFullName();

                ParticleTabData data;
                data.m_libraryItemName = particleLongName;
                data.m_libraryName = library;

                AddToDynamicTab(tabTitle, data);
            }
        }
        m_IgnoreAttributeRefresh = false; //respect updates

        RefreshParameterUI(static_cast<CParticleItem*>(item));
        m_attributeView->OnEndReload();
    }
    else
    {
        //Clear selection.
        m_attributeView->setEnabled(false);
    }
}

void DockableAttributePanel::LodItemSelectionChanged(CBaseLibraryItem* item, SLodInfo* lod)
{
    CRY_ASSERT(m_attributeView);

    this->m_currentLod = lod;

    if (item && item->IsParticleItem)
    {
        m_attributeView->OnStartReload();
        m_IgnoreAttributeRefresh = true; //ignore updates
        {
            QString particleLongName = item->GetFullName();
            int tabIndex = FindItemInTabs(particleLongName);
            if (tabIndex != -1) //Item is already in a tab so select that tab
            {
                SelectTab(tabIndex);
            }
            else //Item is not in a tab yet so add it to the "dynamic" tab
            {
                QString library = item->GetLibrary()->GetName();
                QString tabTitle = item->GetFullName();

                ParticleTabData data;
                data.m_libraryItemName = particleLongName;
                data.m_libraryName = library;

                AddToDynamicTab(tabTitle, data);
            }
        }
        m_IgnoreAttributeRefresh = false; //respect updates

        CParticleItem* particleItem = static_cast<CParticleItem*>(item);
        RefreshParameterUI(particleItem, lod);
        m_attributeView->OnEndReload();
    }
    else
    {
        //Clear selection.
        m_attributeView->setEnabled(false);
    }
}


void DockableAttributePanel::ItemNameChanged(CBaseLibraryItem* item, const QString& oldName, const QString& currentName, const QString newLib)
{
    CRY_ASSERT(item);

    QString oldFullName = item->GetLibrary()->GetName(); // Grab old library
    oldFullName.append(".");
    oldFullName.append(oldName);
    QString prevName = oldFullName;

    QString nextTabTitle = newLib.isEmpty() ? QString(item->GetLibrary()->GetName()) + "." + currentName : newLib + "." + currentName;

    int tabIndex = FindItemInTabs(prevName);
    if (tabIndex != -1)
    {
        ParticleTabData tabData = m_openParticleTabBar->tabData(tabIndex).value<ParticleTabData>();
        tabData.m_libraryItemName = nextTabTitle;
        tabData.m_libraryName = newLib.isEmpty() ? QString(item->GetLibrary()->GetName()) : newLib;
        QVariant d;
        d.setValue(tabData);
        m_openParticleTabBar->setTabData(tabIndex, d); //Update the tab data
        m_openParticleTabBar->setTabText(tabIndex, nextTabTitle); //Update the tab title name
    }
}

void DockableAttributePanel::UpdateItemName(const QString& fullOldName, const QString& fullNewName)
{
    QStringList newNameList = fullNewName.split(".");
    QString newLibName = newNameList.first();
    QString prevName = fullOldName;
    int tabIndex = FindItemInTabs(prevName);

    if (tabIndex != -1)
    {
        ParticleTabData tabData = m_openParticleTabBar->tabData(tabIndex).value<ParticleTabData>();
        tabData.m_libraryItemName = fullNewName;
        tabData.m_libraryName = newLibName;
        QVariant d;
        d.setValue(tabData);
        m_openParticleTabBar->setTabData(tabIndex, d); //Update the tab data
        m_openParticleTabBar->setTabText(tabIndex, fullNewName); //Update the tab title name
    }
}

void DockableAttributePanel::TabSelectionChange(int index)
{
    if (m_IgnoreAttributeRefresh)
    {
        return;
    }

    if (index == -1)
    {
        //There is no tab to change to
        return;
    }

    if (m_openParticleTabBar->tabData(index).canConvert<ParticleTabData>())
    {
        ParticleTabData tabData = m_openParticleTabBar->tabData(index).value<ParticleTabData>();
        QString tabTitle = m_openParticleTabBar->tabText(index);
        qDebug() << "Particle Editor: changed to particle" << tabTitle << "via tab" << index;

        CParticleItem* libItem = static_cast<CParticleItem*>(m_libraryManager->FindItemByName(tabData.m_libraryItemName));
        m_attributeView->SetCurrentItem(libItem);
        if (libItem)
        {
            //set the attributeview's selected emitter by name
            emit SignalTabSelectionChanged(tabData.m_libraryName, QString(libItem->GetName()));
        }
    }
}

void DockableAttributePanel::ImportPanelFromQString(QString filedata)
{
    m_attributeView->ImportPanel(filedata, false);
}

void DockableAttributePanel::TabClosing(int index)
{
    CRY_ASSERT(m_openParticleTabBar);
    CRY_ASSERT(m_attributeView);

    if (m_TabBarDynamicTabIndex != -1)
    {
        if (m_TabBarDynamicTabIndex == index)
        {
            m_TabBarDynamicTabIndex = -1; //reset the dynamictabindex to needing a new tab.
        }
        else if (m_TabBarDynamicTabIndex > index)
        {
            m_TabBarDynamicTabIndex--; //we are about to remove an index that is under this stored index so that means it will be reduced by 1
        }
    }

    m_openParticleTabBar->removeTab(index);
    if (m_openParticleTabBar->count() == 0)
    {
        m_openParticleTabBar->repaint(); //Needed to ensure proper redraw when last tab is removed
        m_attributeView->setEnabled(false);
        m_attributeView->ShowDefaultView();
        SignalTabSelectionChanged("", "");
    }
}

void DockableAttributePanel::TabMoved(int from, int to)
{
    // the logic here is based on the tab that has moved not the tab that is grabbed is moving from (int) to(int)
    if (m_TabBarDynamicTabIndex == to)
    {
        m_TabBarDynamicTabIndex = from;
    }
}

void DockableAttributePanel::TabShowContextMenu(const QPoint& pos)
{
    CRY_ASSERT(m_openParticleTabBar);
    CRY_ASSERT(m_tabBarContextMenu);
    m_tabBarContextMenu->clear();

    int index = m_openParticleTabBar->tabAt(pos);
    if (index != -1)
    {
        ParticleTabData tabData = m_openParticleTabBar->tabData(index).value<ParticleTabData>();
        emit SignalPopulateTabBarContextMenu(tabData.m_libraryName, tabData.m_libraryItemName, m_tabBarContextMenu);
        m_tabBarContextMenu->exec(m_openParticleTabBar->mapToGlobal(pos));
    }
}

QMenu* DockableAttributePanel::GetTitleBarMenu()
{
    CRY_ASSERT(m_titleBarMenu);
    m_titleBarMenu->clear();
    emit SignalPopulateTitleBarMenu(m_titleBarMenu);
    return m_titleBarMenu;
}

DockWidgetTitleBar* DockableAttributePanel::CreateTabBar()
{
    DockWidgetTitleBar* propertiesTabBar = new DockWidgetTitleBar(this);
    propertiesTabBar->setObjectName("AttributeTabTitleBar");
    m_openParticleTabBar = propertiesTabBar->SetupFluidTabBar();
    m_openParticleTabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    m_openParticleTabBar->setTabsClosable(false); //FluidTabBar handles close buttons manually
    m_openParticleTabBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

    //Callbacks
    connect(m_openParticleTabBar, &QTabBar::currentChanged, this, &DockableAttributePanel::TabSelectionChange);
    connect(m_openParticleTabBar, &QTabBar::tabCloseRequested, this, &DockableAttributePanel::TabClosing);
    connect(m_openParticleTabBar, &QTabBar::tabMoved, this, &DockableAttributePanel::TabMoved);
    connect(m_openParticleTabBar, &FluidTabBar::customContextMenuRequested, this, &DockableAttributePanel::TabShowContextMenu);

    m_tabBarContextMenu = new ContextMenu(this);

    return propertiesTabBar;
}

void DockableAttributePanel::RefreshOpenTabColors()
{
    CRY_ASSERT(m_openParticleTabBar);
    for (int tabIdx = 0; tabIdx < m_openParticleTabBar->count(); tabIdx++)
    {
        if (m_openParticleTabBar->tabData(tabIdx).canConvert<ParticleTabData>())
        {
            ParticleTabData tabData = m_openParticleTabBar->tabData(tabIdx).value<ParticleTabData>();
            CParticleItem* libItem = static_cast<CParticleItem*>(m_libraryManager->FindItemByName(tabData.m_libraryItemName));
            CRY_ASSERT(libItem);
            CRY_ASSERT(libItem->GetType() == EDB_TYPE_PARTICLE);
            m_openParticleTabBar->setTabTextColor(tabIdx, GetTabTextColorFor(static_cast<CParticleItem*>(libItem)));
        }
    }
}

int DockableAttributePanel::FindItemInTabs(QString& itemName) const
{
    CRY_ASSERT(m_openParticleTabBar);
    int tabIndex = -1;
    for (int i = 0; i < m_openParticleTabBar->count(); i++)
    {
        if (m_openParticleTabBar->tabData(i).canConvert<ParticleTabData>())
        {
            ParticleTabData tabData = m_openParticleTabBar->tabData(i).value<ParticleTabData>();
            if (tabData.m_libraryItemName == itemName)
            {
                tabIndex = i;
                break;
            }
        }
    }
    return tabIndex;
}

void DockableAttributePanel::SelectTab(unsigned int tabIndex)
{
    CRY_ASSERT(m_openParticleTabBar);
    CRY_ASSERT(tabIndex < m_openParticleTabBar->count());

    m_openParticleTabBar->setCurrentIndex(tabIndex);
}

void DockableAttributePanel::AddToDynamicTab(QString& tabTitle, ParticleTabData& data)
{
    CRY_ASSERT(m_openParticleTabBar);
    int tabIndex = -1;
    //Item is not in a tab yet so add it to the "dynamic" tab
    if (m_TabBarDynamicTabIndex != -1) //"dynamic" tab is valid
    {
        tabIndex = m_TabBarDynamicTabIndex;
        m_openParticleTabBar->setTabText(tabIndex, tabTitle); //update the name so we dont show previous name
    }
    else //Create a new "dynamic" tab
    {
        tabIndex = m_openParticleTabBar->addTab(tabTitle);
        data.m_isNewTab = true;//Mark the tab to be newly created, otherwise the m_openParticleTabBar::currentChanged event will cause an extra reload
        m_TabBarDynamicTabIndex = tabIndex;
    }
    QVariant d;
    d.setValue(data);
    m_openParticleTabBar->setTabData(tabIndex, d);
    CParticleItem* libItem = static_cast<CParticleItem*>(m_libraryManager->FindItemByName(data.m_libraryItemName));
    m_openParticleTabBar->setTabTextColor(tabIndex, GetTabTextColorFor(static_cast<CParticleItem*>(libItem)));
    SelectTab(tabIndex);//Select the tab ...
}

QColor DockableAttributePanel::GetTabTextColorFor(CParticleItem* item) const
{
    CRY_ASSERT(item);
    CRY_ASSERT(item->GetType() == EDB_TYPE_PARTICLE);
    QColor retval = Qt::red;
    if (GetEnabledParameterValue(item))
    {
        retval = m_enabledTabTextColor;
    }
    else
    {
        retval = m_disabledTabTextColor;
    }
    return retval;
}

void DockableAttributePanel::SetOnVariableChangeCallbackRecurse(IVariableContainer* vars)
{
    CRY_ASSERT(vars);
    for (int i = 0; i < vars->GetNumVariables(); i++)
    {
        IVariable* var = vars->GetVariable(i);
        CRY_ASSERT(var);
        var->AddOnSetCallback(functor(*this, &DockableAttributePanel::OnVariableChange));
        SetOnVariableChangeCallbackRecurse(var);
    }
}

void DockableAttributePanel::OnVariableChange(IVariable* var)
{
    if (m_IgnoreAttributeRefresh)
    {
        return;
    }

    CRY_ASSERT(m_openParticleTabBar);
    CRY_ASSERT(m_pParticleUI);
    int curTab = m_openParticleTabBar->currentIndex();
    if (curTab != -1)
    {
        CRY_ASSERT(m_openParticleTabBar->tabData(curTab).canConvert<ParticleTabData>());
        ParticleTabData tabData = m_openParticleTabBar->tabData(curTab).value<ParticleTabData>();

        CParticleItem* particleItem = static_cast<CParticleItem*>(m_libraryManager->FindItemByName(tabData.m_libraryItemName));
        CRY_ASSERT(particleItem);

        //if any of these are true particleitem is invalid, return to prevent crashing
        if (!particleItem)
        {
            return;
        }
        if (particleItem->GetFullName().isEmpty() || !particleItem->GetEffect() || !particleItem->GetLibrary())
        {
            return;
        }

        particleItem->SetModified();
        GetIEditor()->SetModifiedFlag();

        m_pParticleUI->SetToParticles(particleItem, m_currentLod);

        // If enabled state is changed, refresh UI active icons ... special case for handling of visualization changes in other widgets.
        if (QString::compare(var->GetName(), "Enabled") == 0)
        {
            emit SignalEnabledParameterChanged(tabData.m_libraryName, tabData.m_libraryItemName);
            m_openParticleTabBar->setTabTextColor(curTab, GetTabTextColorFor(particleItem));//NOTE: the above SetToParticles should sync the value as the "changed" version.
        }
        RefreshGroupNodeSettings();
        //POSSIBLE LEGACY ONLY SYNC USAGE HERE
        emit SignalParameterChanged(tabData.m_libraryName, tabData.m_libraryItemName);
        //POSSIBLE LEGACY ONLY SYNC USAGE HERE
    }
}


void DockableAttributePanel::RefreshCurrentParameterUI()
{
    EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
    EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);

    m_pParticleUI->SetFromParticles(m_attributeView->GetCurrentItem(), m_currentLod);
}

void DockableAttributePanel::RefreshParameterUI(CParticleItem* item, SLodInfo* lod)
{
    CRY_ASSERT(item);
    CRY_ASSERT(item->GetLibrary());
    CRY_ASSERT(m_attributeView);
    CRY_ASSERT(m_pParticleUI);
    CRY_ASSERT(m_attributeView->getPanelWidget());
    CRY_ASSERT(GetIEditor());

    EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
    EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);
    
    m_IgnoreAttributeRefresh = true;
    {
        //Building the UI
        if (m_attributeView->getPanelWidget()->isEmpty())
        {
            m_attributeView->Clear();
            QString particlePath = QString().sprintf("%s.%s", item->GetLibrary()->GetName(), item->GetName());
            m_attributeView->InitConfiguration(particlePath);
            LoadSessionState(); //configure the UI the way the user wants it to be.
        }
        else
        {
            SaveSessionState();
        }

        m_pParticleUI->SetFromParticles(item, lod);
    }
    m_IgnoreAttributeRefresh = false;

    m_attributeView->HideDefaultView();
    m_attributeView->setEnabled(true);
    m_attributeView->SetCurrentItem(item);
    RefreshGroupNodeSettings();
}

void DockableAttributePanel::RefreshCurrentTab()
{
    //closes current tab then reopens tab and inserts it to current index
    int index = m_openParticleTabBar->currentIndex();
    if (m_openParticleTabBar->tabData(index).canConvert<ParticleTabData>())
    {
        ParticleTabData tabData = m_openParticleTabBar->tabData(index).value<ParticleTabData>();
        QString tabTitle = m_openParticleTabBar->tabText(index);
        CParticleItem* libItem = static_cast<CParticleItem*>(m_libraryManager->FindItemByName(tabData.m_libraryItemName));
        if (libItem != nullptr)
        {
            QString itemName = libItem->GetName();
            emit SignalTabSelectionChanged(tabData.m_libraryName, itemName);
        }
        else
        {
            CloseTab(tabTitle);
        }
    }
}

QString DockableAttributePanel::GetCurrentTabText()
{
    int index = m_openParticleTabBar->currentIndex();
    if (index == -1)
    {
        return "";
    }
    return m_openParticleTabBar->tabText(index);
}

void DockableAttributePanel::CloseTab(const QString& name)
{
    m_openParticleTabBar->CloseTab(name);
}

void DockableAttributePanel::OnEnabledToggleTriggered()
{
    if (!GetIEditor()->GetParticleUtils()->HotKey_IsEnabled())
    {
        return;
    }
    IVariable* var = m_attributeView->GetVarFromPath("Emitter.Enabled");
    if (var != nullptr)
    {
        bool state = false;
        var->Get(state);
        var->Set(!state);
    }
}

void DockableAttributePanel::OnInsertCommentTriggered()
{
    if (!GetIEditor()->GetParticleUtils()->HotKey_IsEnabled())
    {
        return;
    }
    m_attributeView->FocusVar("Emitter.Comment");
}

void DockableAttributePanel::CloseAllTabs()
{
    m_openParticleTabBar->CloseAll();
}

void DockableAttributePanel::RefreshGroupNodeSettings()
{
    IVariable* groupVar = m_attributeView->GetVarFromPath("GroupNode.Group");
    if (!groupVar)
    {
        return;
    }
    bool isGroup = false;
    groupVar->Get(isGroup);
    if (isGroup)
    {
        m_attributeView->HideAllButGroup("Group&&Comment");
    }
    else
    {
        m_attributeView->ShowAllButGroup("Group");
    }
}

void DockableAttributePanel::PassThroughSignalBuildPanelList(QMenu* menu)
{
    emit SignalBuildCustomAttributeList(menu);
}

void DockableAttributePanel::PassThroughSignalAddPreset(QString panelName, QString data)
{
    emit SignalAddPreset(panelName, data);
}

void DockableAttributePanel::OnItemUndoPoint(const QString& itemName)
{
    emit SignalItemUndoPoint(itemName, true, m_currentLod);
}

#include <QT/DockableAttributePanel.moc>
