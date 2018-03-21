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
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QGridLayout>
#include <QSizePolicy>
#include <QCheckBox>
#include <QScrollArea>

//Editor
#include <IEditor.h>
#include <IEditorParticleUtils.h>
#include <Util/Variable.h>
#include <Particles/ParticleUIDefinition.h>
#include <Particles/ParticleItem.h>
#include <BaseLibraryItem.h>

//EditorUI_QT
#include <../EditorUI_QT/DockWidgetTitleBar.h>
#include "../EditorUI_QT/ContextMenu.h"
#include <../EditorUI_QT/LibraryTreeViewItem.h>
#include <../EditorUI_QT/DockableLibraryPanel.h>
#include <../EditorUI_QT/LibraryTreeView.h>
#include <../EditorUI_QT/LibraryTreeViewItem.h>
#include <../EditorUI_QT/DefaultViewWidget.h>

//Local
#include "DockableLODPanel.h"
#include "LODLevelWidget.h"
#include "ParticleEditorPlugin.h"

#include <Include/IEditorParticleManager.h>


LodWidget::LodWidget(QWidget* parent)
    : QWidget(parent)
    , m_BlendInLabel(nullptr)
    , m_BlendOutLabel(nullptr)
    , m_OverlapLabel(nullptr)
    , m_BlendInBox(nullptr)
    , m_BlendOutBox(nullptr)
    , m_OverlapBox(nullptr)
    , m_AddLevelOfDetailButton(nullptr)
    , m_TopWidget(nullptr)
    , m_TopLayout(nullptr)
    , m_LodLevelLayout(nullptr)
    , m_LodLevelPanel(nullptr)
    , m_LodLevelScroll(nullptr)
    , m_Item(nullptr)
    , m_SelectedLodLevel(-1)
{
}

LodWidget::~LodWidget()
{
    SAFE_DELETE(m_BlendInLabel);
    SAFE_DELETE(m_BlendOutLabel);
    SAFE_DELETE(m_OverlapLabel);
    SAFE_DELETE(m_BlendInBox);
    SAFE_DELETE(m_BlendOutBox);
    SAFE_DELETE(m_OverlapBox);
    SAFE_DELETE(m_AddLevelOfDetailButton);

    if (m_Item != nullptr)
    {
        m_Item->Release();
    }
}

void LodWidget::Init()
{
    m_Layout = new QVBoxLayout(this);
    m_Layout->setContentsMargins(0, 0, 0, 0);

    setObjectName("LODPanel");
}

void LodWidget::AddLodLevel()
{
    emit SignalAddLod(m_Item);
}
void LodWidget::AddLodLevelSlot()
{
    AddLodLevel();
}

void LodWidget::ClearLODSelection()
{
    for (int i = 0; i < m_LodLevels.size(); i++)
    {
        m_LodLevels[i]->ClearSelectionColor();
    }
}

void LodWidget::ClearGUI()
{
    if (m_TopWidget == nullptr)
    {
        return;
    }

    //Remove top widget
    m_TopLayout->removeWidget(m_BlendInLabel);
    m_TopLayout->removeWidget(m_BlendOutLabel);
    m_TopLayout->removeWidget(m_OverlapLabel);

    m_TopLayout->removeWidget(m_BlendInBox);
    m_TopLayout->removeWidget(m_BlendOutBox);
    m_TopLayout->removeWidget(m_OverlapBox);

    m_TopLayout->removeWidget(m_AddLevelOfDetailButton);

    SAFE_DELETE(m_BlendInLabel);
    SAFE_DELETE(m_BlendOutLabel);
    SAFE_DELETE(m_OverlapLabel);

    SAFE_DELETE(m_BlendInBox);
    SAFE_DELETE(m_BlendOutBox);
    SAFE_DELETE(m_OverlapBox);

    SAFE_DELETE(m_AddLevelOfDetailButton);

    SAFE_DELETE(m_TopLayout);

    m_Layout->removeWidget(m_TopWidget);
    SAFE_DELETE(m_TopWidget);

    //Remove lod scroll area and lod levels
    if (m_LodLevelScroll == nullptr)
    {
        return;
    }

    while (m_LodLevels.count() > 0)
    {
        delete m_LodLevels.takeLast();
    }

    m_LodLevelLayout->removeWidget(m_LodLevelPanel);
    SAFE_DELETE(m_LodLevelLayout);
    SAFE_DELETE(m_LodLevelPanel);


    m_Layout->removeWidget(m_LodLevelScroll);
    SAFE_DELETE(m_LodLevelScroll)
}

void LodWidget::RefreshCurrentGUI()
{
    RefreshGUI(m_Item, m_SelectedLodLevel);
}

void LodWidget::RefreshGUI(CParticleItem* item, int selectedLevel)
{
    EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
    EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);

    if (m_Item != nullptr)
    {
        m_Item->Release();
    }

    CParticleItem* top = item;
    while (top && top->GetParent() != nullptr)
    {
        top = top->GetParent();
    }

    ClearGUI();
    m_Item = item;

    if (m_Item == nullptr)
    {
        return;
    }

    m_Item->AddRef();
    //Top Widget
    m_TopWidget = new QWidget(this);
    m_Layout->addWidget(m_TopWidget);
    m_TopLayout = new QGridLayout(m_TopWidget);

    m_BlendInLabel = new QLabel("Blend In", m_TopWidget);
    m_BlendOutLabel = new QLabel("Blend Out", m_TopWidget);
    m_OverlapLabel = new QLabel("Overlap", m_TopWidget);

    m_BlendInBox = new QDoubleSpinBox(m_TopWidget);
    m_BlendInBox->setMinimum(0.0);
    m_BlendInBox->setValue(top->GetEffect()->GetParticleParams().fBlendIn);
    m_BlendOutBox = new QDoubleSpinBox(m_TopWidget);
    m_BlendOutBox->setMinimum(0.0);
    m_BlendOutBox->setValue(top->GetEffect()->GetParticleParams().fBlendOut);
    m_OverlapBox = new QDoubleSpinBox(m_TopWidget);
    m_OverlapBox->setMinimum(0.0);
    m_OverlapBox->setValue(top->GetEffect()->GetParticleParams().fOverlap);

    connect(m_BlendInBox, SIGNAL(valueChanged(double)), this, SLOT(BlendInValueChanged(double)));
    connect(m_BlendOutBox, SIGNAL(valueChanged(double)), this, SLOT(BlendOutValueChanged(double)));
    connect(m_OverlapBox, SIGNAL(valueChanged(double)), this, SLOT(OverlapValueChanged(double)));

    m_AddLevelOfDetailButton = new QPushButton("+ Add Level of Detail", m_TopWidget);
    connect(m_AddLevelOfDetailButton, SIGNAL(released()), this, SLOT(AddLodLevelSlot()));


    m_TopLayout->addWidget(m_BlendInLabel, 0, 0);
    m_TopLayout->addWidget(m_BlendInBox, 0, 1);
    m_TopLayout->addWidget(m_BlendOutLabel, 1, 0);
    m_TopLayout->addWidget(m_BlendOutBox, 1, 1);
    m_TopLayout->addWidget(m_OverlapLabel, 2, 0);
    m_TopLayout->addWidget(m_OverlapBox, 2, 1);

    m_TopLayout->addWidget(m_AddLevelOfDetailButton, 3, 0, 1, 2);

    m_TopWidget->setLayout(m_TopLayout);
    m_TopWidget->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum));

    //LoD Levels
    m_LodLevelScroll = new QScrollArea(this);
    m_LodLevelScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_LodLevelScroll->setWidgetResizable(true);
    m_LodLevelScroll->setFrameStyle(QFrame::NoFrame);
    m_LodLevelScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_LodLevelScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);


    m_LodLevelPanel = new QWidget(m_LodLevelScroll);

    m_LodLevelPanel->setObjectName("LODLevelPanel");
    m_LodLevelLayout = new QVBoxLayout(m_LodLevelPanel);

    int dummy = 0;
    int right = 0;
    m_LodLevelLayout->getContentsMargins(&dummy, &dummy, &right, &dummy);
    m_LodLevelLayout->setContentsMargins(0, 0, right, 0);

    m_LodLevelScroll->setWidget(m_LodLevelPanel);
    m_LodLevelScroll->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    m_LodLevelPanel->setLayout(m_LodLevelLayout);
    m_LodLevelPanel->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
    m_Layout->addWidget(m_LodLevelScroll);

    for (int i = 0; i < m_Item->GetEffect()->GetLevelOfDetailCount(); i++)
    {
        AddLODLevelWidget(m_Item->GetEffect()->GetLevelOfDetail(i));
    }

    SelectLevel(selectedLevel, "");

    this->setLayout(m_Layout);
    this->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void LodWidget::AddLODLevelWidget(SLodInfo* lod)
{
    LODLevelWidget* newLevel = new LODLevelWidget(m_LodLevelPanel);
    newLevel->Init(lod);
    m_LodLevelLayout->addWidget(newLevel);
    m_LodLevels.push_back(newLevel);

    connect(newLevel, SIGNAL(RemoveLod(LODLevelWidget*,SLodInfo*)), this, SLOT(RemoveLodLevel(LODLevelWidget*,SLodInfo*)));
    connect(newLevel, &LODLevelWidget::SignalLodParticleItemSelected, this, &LodWidget::onLodParticleItemSelected);
    connect(newLevel, &LODLevelWidget::SignalRefreshGUI, this, &LodWidget::RefreshCurrentGUI);
    connect(newLevel, &LODLevelWidget::SignalDistanceChanged, this, &LodWidget::OnLevelDistanceChanged);
    connect(newLevel, &LODLevelWidget::SignalUpdateLODIcon, this, &LodWidget::UpdateLODIcon);
}

void LodWidget::RemoveLODLevelWidget(SLodInfo* lod)
{
    for (int i = 0; i < m_LodLevels.size(); i++)
    {
        if (m_LodLevels[i]->GetLod() == lod)
        {
            m_LodLevelLayout->removeWidget(m_LodLevels[i]);
            delete m_LodLevels[i];
            m_LodLevels.remove(i);
            return;
        }
    }
}

void LodWidget::UpdateLODIcon(CLibraryTreeViewItem* item)
{
    emit SignalUpdateLODIcon();
}

void LodWidget::updateTreeItemHighlight(bool ishighlight, SLodInfo* lod, QString fullname)
{
    for (LODLevelWidget* treewidget : m_LodLevels)
    {
        treewidget->updateTreeItemStyle(ishighlight, lod, fullname);
    }
}

void LodWidget::RemoveLodLevel(LODLevelWidget* widget, SLodInfo* lod)
{
    m_Item->GetEffect()->RemoveLevelOfDetail(lod);

    QString fullname = m_Item->GetFullName();

    if (!m_Item->GetEffect()->HasLevelOfDetail())
    {
        emit SignalLodIconRemoved(fullname);
    }

    RefreshLodPanels();
    emit SignalLodUndoPoint();
}

void LodWidget::BlendInValueChanged(double value)
{
    CParticleItem* top = m_Item;
    while (top->GetParent() != nullptr)
    {
        top = m_Item->GetParent();
    }

    ParticleParams params = top->GetEffect()->GetParticleParams();
    params.fBlendIn = value;
    top->GetEffect()->SetParticleParams(params);
    emit SignalLodUndoPoint();
}

void LodWidget::BlendOutValueChanged(double value)
{
    CParticleItem* top = m_Item;
    while (top->GetParent() != nullptr)
    {
        top = m_Item->GetParent();
    }

    ParticleParams params = top->GetEffect()->GetParticleParams();
    params.fBlendOut = value;
    top->GetEffect()->SetParticleParams(params);
    emit SignalLodUndoPoint();
}

void LodWidget::OverlapValueChanged(double value)
{
    CParticleItem* top = m_Item;
    while (top->GetParent() != nullptr)
    {
        top = m_Item->GetParent();
    }

    ParticleParams params = top->GetEffect()->GetParticleParams();
    params.fOverlap = value;
    top->GetEffect()->SetParticleParams(params);
    emit SignalLodUndoPoint();
}

void LodWidget::SelectLevel(int index, const QString& itemName)
{
    if (m_LodLevels.size() == 0)
    {
        return;
    }
    
    if (index >= m_LodLevels.size())
    {
        index = m_LodLevels.size() - 1;
    }
    
    //if a level is selected
    if (index >= 0)
    {
        m_LodLevels[index]->SetSelected(itemName);
    }

    m_SelectedLodLevel = index;
}

void LodWidget::RemoveSelectedLevel()
{
    if (m_SelectedLodLevel < 0 || m_SelectedLodLevel >= m_LodLevels.size())
    {
        return;
    }

    m_Item->GetEffect()->RemoveLevelOfDetail(m_LodLevels[m_SelectedLodLevel]->GetLod());

    RefreshLodPanels();

    emit SignalUpdateLODIcon();
    emit SignalLodUndoPoint();
}

void LodWidget::RemoveAllLevels()
{
    if (m_Item == nullptr)
    {
        return;
    }

    m_Item->GetEffect()->RemoveAllLevelOfDetails();
    RefreshGUI(m_Item, m_SelectedLodLevel);

    emit SignalUpdateLODIcon();
    emit SignalLodUndoPoint();
}

void LodWidget::MoveSelectedLevelUp()
{
    if (m_SelectedLodLevel < 0 || m_SelectedLodLevel >= m_LodLevels.size())
    {
        return;
    }

    auto selectedLod = m_LodLevels[m_SelectedLodLevel]->GetLod();

    SLodInfo* aboveLod = nullptr;
    for (int i = 0; i < m_Item->GetEffect()->GetLevelOfDetailCount(); i++)
    {
        if (m_Item->GetEffect()->GetLevelOfDetail(i)->GetDistance() < selectedLod->GetDistance())
        {
            aboveLod = m_Item->GetEffect()->GetLevelOfDetail(i);
        }
    }

    if (aboveLod == nullptr)
    {
        return;
    }

    selectedLod->SetDistance(aboveLod->GetDistance() - 1); //Move it up one level

    m_LodLevels[m_SelectedLodLevel]->RefreshDistance();
}

void LodWidget::RefreshLodPanels()
{
    EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
    EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);

    IParticleEffect* effect = m_Item->GetEffect();

    for (int i = 0; i < m_LodLevels.size(); i++)
    {
        bool found = false;

        for (int l = 0; l < effect->GetLevelOfDetailCount(); l++)
        {
            if (effect->GetLevelOfDetail(l) == m_LodLevels[i]->GetLod())
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            RemoveLODLevelWidget(m_LodLevels[i]->GetLod());
        }
    }

    for (int i = 0; i < effect->GetLevelOfDetailCount(); i++)
    {
        bool found = false;

        for (int l = 0; l < m_LodLevels.size(); l++)
        {
            if (effect->GetLevelOfDetail(i) == m_LodLevels[l]->GetLod())
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            AddLODLevelWidget(effect->GetLevelOfDetail(i));
        }
    }

    OnLevelDistanceChanged();
}



void LodWidget::MoveSelectedLevelDown()
{
    if (m_SelectedLodLevel < 0 || m_SelectedLodLevel >= m_LodLevels.size())
    {
        return;
    }
    
    auto selectedLod = m_LodLevels[m_SelectedLodLevel]->GetLod();

    SLodInfo* belowLod = nullptr;
    for (int i = 0; i < m_Item->GetEffect()->GetLevelOfDetailCount(); i++)
    {
        if (m_Item->GetEffect()->GetLevelOfDetail(i)->GetDistance() > selectedLod->GetDistance())
        {
            belowLod = m_Item->GetEffect()->GetLevelOfDetail(i);
            break;
        }
    }

    if (belowLod == nullptr)
    {
        return;
    }

    selectedLod->SetDistance(belowLod->GetDistance() + 1); //Move it up one level

    m_LodLevels[m_SelectedLodLevel]->RefreshDistance();
}

void LodWidget::MoveSelectedToTop()
{
    if (m_SelectedLodLevel < 0 || m_SelectedLodLevel >= m_LodLevels.size())
    {
        return;
    }

    auto selectedLod = m_LodLevels[m_SelectedLodLevel]->GetLod();

    auto currentTopLod = m_Item->GetEffect()->GetLevelOfDetail(0);

    if (selectedLod->GetDistance() == currentTopLod->GetDistance())
    {
        return;
    }

    selectedLod->SetDistance(currentTopLod->GetDistance() - 1);

    m_LodLevels[m_SelectedLodLevel]->RefreshDistance();
}

void LodWidget::MoveSelectedToBottom()
{
    if (m_SelectedLodLevel < 0 || m_SelectedLodLevel >= m_LodLevels.size())
    {
        return;
    }
    
    auto selectedLod = m_LodLevels[m_SelectedLodLevel]->GetLod();

    auto currentBottomLod = m_Item->GetEffect()->GetLevelOfDetail(m_LodLevels.size() - 1);

    if (selectedLod->GetDistance() == currentBottomLod->GetDistance())
    {
        return;
    }

    selectedLod->SetDistance(currentBottomLod->GetDistance() + 1);

    m_LodLevels[m_SelectedLodLevel]->RefreshDistance();
}

void LodWidget::SelectFirstLevel()
{
    if (m_LodLevels.size() == 0)
    {
        return;
    }

    m_LodLevels[0]->SelectTopItem();
}

void LodWidget::SelectLastLevel()
{
    if (m_LodLevels.size() == 0)
    {
        return;
    }

    int sel = max(0, m_Item->GetEffect()->GetLevelOfDetailCount() - 1);
    m_LodLevels[sel]->SelectTopItem();
}

void LodWidget::onLodParticleItemSelected(LODLevelWidget* lodLevel, IParticleEffect* baseParticle, SLodInfo* lod)
{
    for (unsigned int i = 0, _e = m_LodLevels.count(); i < _e; i++)
    {
        if (m_LodLevels[i] != lodLevel)
        {
            m_LodLevels[i]->ClearSelectionColor();
        }
        else
        {
            m_SelectedLodLevel = i;
        }
    }

    emit SignalLodItemSelected(baseParticle, lod);
}

void LodWidget::OnLevelDistanceChanged()
{
    //Sort level widgets
    int outer, inner;
    for (outer = m_LodLevelLayout->count() - 1; outer > 0; outer--)
    {
        for (inner = 0; inner < outer; inner++)
        {
            LODLevelWidget* first = static_cast<LODLevelWidget*>(m_LodLevelLayout->itemAt(inner)->widget());
            LODLevelWidget* second = static_cast<LODLevelWidget*>(m_LodLevelLayout->itemAt(inner + 1)->widget());

            if (first->GetLod()->GetDistance() > second->GetLod()->GetDistance())
            {
                m_LodLevelLayout->removeWidget(first);
                m_LodLevelLayout->insertWidget(inner + 1, first);
            }
        }
    }

    emit SignalLodUndoPoint();
}

DockableLODPanel::DockableLODPanel(QWidget* parent)
    : FloatableDockPanel("", parent)
    , m_titleBar(nullptr)
    , m_titleBarMenu(nullptr)
    , m_IgnoreAttributeRefresh(false)
    , m_LodWidget(nullptr)
    , m_selectedParticle(nullptr)
    , m_defaultView(nullptr)
    , m_centralWidget(nullptr)
    , m_currentSelectedItem(nullptr)
{
}

DockableLODPanel::~DockableLODPanel()
{
    SAFE_DELETE(m_titleBar);
    SAFE_DELETE(m_titleBarMenu);
    SAFE_DELETE(m_LodWidget);
    ClearParticle();
}

void DockableLODPanel::Init(const QString& panelName)
{
    //Setup titlebar
    m_titleBar = new DockWidgetTitleBar(this);
    m_centralWidget = new QWidget(this);
    m_LodWidget = new LodWidget(m_centralWidget);
    m_layout = new QVBoxLayout(m_centralWidget);
    m_defaultView = new DefaultViewWidget(m_centralWidget);
    m_titleBarMenu = new QMenu;

    DecorateDefaultView();
    m_titleBar->SetupLabel(panelName);
    m_titleBar->SetShowMenuContextMenuCallback([&] {return GetTitleBarMenu();
        });
    setTitleBarWidget(m_titleBar);
    setWidget(m_centralWidget);
    m_centralWidget->setLayout(m_layout);
    BuildGUI();
    //Build menu
    setWindowTitle("Level of detail");
    ShowDefaultView();
}

void DockableLODPanel::OnAttributeViewItemDeleted(const QString& name)
{
    if (m_currentSelectedItem)
    {
        QString fullname = m_currentSelectedItem->GetLibrary()->GetName() + name;

        emit SignalAttributeViewItemDeleted(fullname);
    }
}

void DockableLODPanel::CollectItemRecursive(CBaseLibraryItem* rootItem, AZStd::list<AZStd::string>& itemIds)
{
    itemIds.push_back(rootItem->GetFullName().toUtf8().data());
    for (int i = 0; i < rootItem->GetChildCount(); i++)
    {
        CollectItemRecursive(rootItem->GetChild(i), itemIds);
    }
}

void DockableLODPanel::OnLodUndoPoint()
{
    AddLodUndoPoint(m_selectedParticle);
}

void DockableLODPanel::AddLodUndoPoint(CParticleItem* item)
{
    bool isSuspend = false;
    EBUS_EVENT_RESULT(isSuspend, EditorLibraryUndoRequestsBus, IsSuspend);

    if (isSuspend)
    {
        return;
    }

    CParticleItem* parent = item;    
    if (!parent)
    {
        return;
    }

    while (parent->GetParent())
    {
        parent = parent->GetParent();
    }

    AZStd::list<AZStd::string> itemIds;
    CollectItemRecursive(parent, itemIds);
    EBUS_EVENT(EditorLibraryUndoRequestsBus, AddItemGroupUndo, itemIds);
}

void DockableLODPanel::LoadSessionState()
{
    QSettings settings("Amazon", "Lumberyard");
    if (settings.value(PARTICLE_EDITOR_LAYOUT_SETTINGS_LOD).isNull() == false)
    {
        restoreGeometry(settings.value(PARTICLE_EDITOR_LAYOUT_SETTINGS_LOD).toByteArray());
    }
}

void DockableLODPanel::SaveSessionState()
{
    QSettings settings("Amazon", "Lumberyard");
    settings.setValue(PARTICLE_EDITOR_LAYOUT_SETTINGS_LOD, saveGeometry());
    settings.sync();
}

void DockableLODPanel::UpdateColors(const QMap<QString, QColor>& colorMap)
{
    m_enabledTextColor = Qt::red;
    m_disabledTextColor = Qt::red;

    if (colorMap.contains("CDockWidgetTitleBarColor"))
    {
        m_enabledTextColor = colorMap["CDockWidgetTitleBarColor"];
    }

    if (colorMap.contains("CTextEditDisabledText"))
    {
        m_disabledTextColor = colorMap["CTextEditDisabledText"];
    }
}

QMenu* DockableLODPanel::GetTitleBarMenu()
{
    CRY_ASSERT(m_titleBarMenu);
    m_titleBarMenu->clear();
    emit SignalPopulateTitleBarMenu(m_titleBarMenu);
    return m_titleBarMenu;
}

void DockableLODPanel::OnAddLod(CLibraryTreeViewItem* item)
{
    //Add Level of detail for particle
    CRY_ASSERT(item);
    CParticleItem* pParticle = static_cast<CParticleItem*>(item->GetItem());
    CRY_ASSERT(pParticle);
    CRY_ASSERT(pParticle->GetEffect());
    
    OnAddLod(pParticle);

    EmitIconChangeRecursive(item);
}


void DockableLODPanel::EmitIconChangeRecursive(CLibraryTreeViewItem* item)
{
    emit SignalChangeLODIcon(item->GetLibraryName(), item->GetFullPath());

    for (int i = 0; i < item->childCount(); i++)
    {
        EmitIconChangeRecursive(static_cast<CLibraryTreeViewItem*>(item->child(i)));
    }
}

void DockableLODPanel::EmitIconChangeRecursive(CParticleItem* item)
{
    emit SignalChangeLODIcon(item->GetLibrary()->GetName(), item->GetFullName());

    for (int i = 0; i < item->GetChildCount(); i++)
    {
        EmitIconChangeRecursive(static_cast<CParticleItem*>(item->GetChild(i)));
    }
}

void DockableLODPanel::UpdateLODIcon()
{
    if (m_selectedParticle)
    {
        CParticleItem* top = m_selectedParticle;

        if (m_selectedParticle->GetParent())
        {
            while (top && top->GetParent())
            {
                top = top->GetParent();
            }
        }

        EmitIconChangeRecursive(top);
    }
}

void DockableLODPanel::EmitIconChangeRecursive(QString library, IParticleEffect* item)
{
    QString fullname = item->GetName();
    IParticleEffect* parent = item->GetParent();

    while (parent != nullptr)
    {
        fullname = parent->GetName() + ('.' + fullname);
        parent = parent->GetParent();
    }

    emit SignalChangeLODIcon(library, fullname);

    for (int i = 0; i < item->GetChildCount(); i++)
    {
        EmitIconChangeRecursive(library, item->GetChild(i));
    }
}

void DockableLODPanel::OnAddLod(CParticleItem* item)
{
    IParticleEffect* effect = item->GetEffect();

    if (effect)
    {
        effect->AddLevelOfDetail();
        item->GetLibrary()->SetModified();
    }

    //Add level of detail on UI
    if (item != m_selectedParticle)
    {
        RefreshGUI(item);
    }
    else
    {
        m_LodWidget->RefreshLodPanels();
    }

    show();

    IParticleEffect* top = item->GetEffect();
    while (top->GetParent() != nullptr)
    {
        top = top->GetParent();
    }
    EmitIconChangeRecursive(QString(item->GetLibrary()->GetName()), top);

    AddLodUndoPoint(item);
}


void DockableLODPanel::ItemSelectionChanged(CBaseLibraryItem* item)
{
    m_currentSelectedItem = item;
    if (item && item->IsParticleItem)
    {
        RefreshGUI(static_cast<CParticleItem*>(item));
    }
    else
    {
        m_LodWidget->RefreshGUI(nullptr);
        m_selectedParticle = nullptr;
    }
}

void DockableLODPanel::LibraryItemReselected(CBaseLibraryItem* item)
{
    m_LodWidget->ClearLODSelection();
}

void DockableLODPanel::SelectLod(const SLodInfo* lod)
{
    CParticleItem* parItem = static_cast<CParticleItem*>(m_currentSelectedItem);
    if (parItem)
    {
        int idx = parItem->GetEffect()->GetLevelOfDetailIndex(lod);
        if (idx >= 0)
        {
            m_LodWidget->SelectLevel(idx, parItem->GetFullName().toUtf8().data());
        }
    }
}

void DockableLODPanel::PerformTitleMenuAction(TitleMenuActions action)
{
    switch (action)
    {
    case TitleMenuActions::AddLevel:
        if (m_selectedParticle != nullptr)
        {
            OnAddLod(m_selectedParticle);
        }
        break;
    case TitleMenuActions::MoveUp:
        m_LodWidget->MoveSelectedLevelUp();
        break;
    case TitleMenuActions::MoveDown:
        m_LodWidget->MoveSelectedLevelDown();
        break;
    case TitleMenuActions::MoveToTop:
        m_LodWidget->MoveSelectedToTop();
        break;
    case TitleMenuActions::MoveToBottom:
        m_LodWidget->MoveSelectedToBottom();
        break;
    case TitleMenuActions::JumpToFirst:
        m_LodWidget->SelectFirstLevel();
        break;
    case TitleMenuActions::JumpToLast:
        m_LodWidget->SelectLastLevel();
        break;
    case TitleMenuActions::Remove:
        m_LodWidget->RemoveSelectedLevel();
        break;
    case TitleMenuActions::RemoveAll:
        m_LodWidget->RemoveAllLevels();
        break;
    }
}


void DockableLODPanel::SetParticle(CParticleItem* item)
{
    ClearParticle();
    m_selectedParticle = item;
    m_selectedParticle->AddRef();
}

void DockableLODPanel::ClearParticle()
{
    if (m_selectedParticle != nullptr)
    {
        m_selectedParticle->Release();
        m_selectedParticle = nullptr;
    }
}

void DockableLODPanel::BuildGUI()
{
    m_LodWidget->Init();
    connect(m_LodWidget, SIGNAL(SignalLodItemSelected(IParticleEffect*,SLodInfo*)), this, SLOT(onLodItemSelectionChanged(IParticleEffect*,SLodInfo*)));
    m_LodWidget->show();

    connect(m_LodWidget, SIGNAL(SignalAddLod(CParticleItem*)), this, SLOT(OnAddLod(CParticleItem*)));
    connect(m_LodWidget, &LodWidget::SignalLodIconRemoved, this, &DockableLODPanel::RemoveLodIcon);
    connect(m_LodWidget, &LodWidget::SignalUpdateLODIcon, this, &DockableLODPanel::UpdateLODIcon);
    connect(m_LodWidget, &LodWidget::SignalAttributeViewItemDeleted, this, &DockableLODPanel::OnAttributeViewItemDeleted);
    connect(m_LodWidget, &LodWidget::SignalLodUndoPoint, this, &DockableLODPanel::OnLodUndoPoint);

    m_layout->addWidget(m_defaultView, 0, Qt::AlignHCenter | Qt::AlignTop);
    m_layout->addWidget(m_LodWidget);
    m_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));
}

void DockableLODPanel::RefreshGUI(CParticleItem* item)
{
    EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
    EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);
    HideDefaultView();
    SetParticle(item);
    m_LodWidget->RefreshGUI(item);
}

void DockableLODPanel::RefreshGUI()
{
    if (!m_selectedParticle)
    {
        ShowDefaultView();
        return;
    }
    RefreshGUI(m_selectedParticle);
}

void DockableLODPanel::onLodItemSelectionChanged(IParticleEffect* baseParticleEffect, SLodInfo* lod)
{
    IEditorParticleManager* pParticleMgr = GetIEditor()->GetParticleManager();

    //Due to inconsistency with the IParticleEffect interface we can not use IParticleEffect::GetFullName().
    //GetFullName returns a crystring, the documentation says this is not allowed in abstract interfaces.
    //When the crystring is destroyed when it goes out of scope the program will cause an error as the program
    //tries to free data it is not allowed to.
    string fullname = baseParticleEffect->GetName();
    IParticleEffect* parent = baseParticleEffect->GetParent();

    while (parent != nullptr)
    {
        fullname = parent->GetName() + ('.' + fullname);
        parent = parent->GetParent();
    }


    IDataBaseItem* baseItem = pParticleMgr->FindItemByName(fullname.c_str());

    if (baseItem != nullptr)
    {
        CBaseLibraryItem* item = static_cast<CBaseLibraryItem*>(baseItem);
        if (item != nullptr)
        {
            emit SignalLodItemSelectionChanged(item, lod);
        }
    }
}

void DockableLODPanel::updateTreeItemHighLight(bool ishighlight, SLodInfo* lod, QString fullname)
{
    m_LodWidget->updateTreeItemHighlight(ishighlight, lod, fullname);
}

void DockableLODPanel::RemoveLodIcon(QString fullname)
{
    QStringList namelist = fullname.split(".");
    QString libname = namelist.takeAt(0);
    QString itemname = namelist.join(".");

    emit SignalChangeLODIcon(libname, fullname);
}


void DockableLODPanel::DecorateDefaultView()
{
    CRY_ASSERT(m_defaultView);
    m_defaultView->SetLabel(tr("Add an LOD to an item to continue"));
}

void DockableLODPanel::ShowDefaultView()
{
    CRY_ASSERT(m_defaultView);
    CRY_ASSERT(m_LodWidget);
    m_defaultView->show();
    m_LodWidget->hide();
}

void DockableLODPanel::HideDefaultView()
{
    CRY_ASSERT(m_defaultView);
    CRY_ASSERT(m_LodWidget);
    m_defaultView->hide();
    m_LodWidget->show();
}

#include <QT/DockableLODPanel.moc>
