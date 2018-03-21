#include "stdafx.h"


//QT
#include <QSettings>
#include <QVBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QGridLayout>
#include <QSizePolicy>
#include <QCheckBox>
#include <QScrollArea>
#include <QTreeWidget>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <QHeaderView>

//Editor
#include <IEditor.h>
#include <IEditorParticleUtils.h>
#include <Util/Variable.h>
#include <Particles/ParticleUIDefinition.h>
#include <Particles/ParticleItem.h>
#include <EditorDefs.h>
#include <IEditorParticleUtils.h>
#include <Include/IEditorParticleManager.h>

//EditorUI
#include <../EditorUI_QT/LibraryTreeViewItem.h>
#include <../EditorUI_QT/Utils.h>

#include "LODLevelWidget.h"

#define CHECKBOX_COLUMN 1
#define NAME_COLUMN 0


LODTreeWidget::LODTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
{
}

void LODTreeWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::RightButton)
    {
        QTreeWidget::mousePressEvent(e);
    }
    else
    {
        emit customContextMenuRequested(e->pos());
    }
}

int LODTreeWidget::GetContentHeight()
{
    if (invisibleRootItem()->childCount() == 0)
    {
        return qMax(height(), invisibleRootItem()->treeWidget()->height());
    }
    // The value is to saved to expand tree items
    int spaceToExpandItem = 10;
    int totalHeight = spaceToExpandItem;
    QtRecurseAll(invisibleRootItem(), [&](QTreeWidgetItem* item) {
        if (invisibleRootItem() == item)
        {
            return;
        }
        if (!item->isHidden())
        {
            totalHeight += rowHeight(indexFromItem(item));
        }
    });
    return totalHeight;
}


LODLevelWidget::LODLevelWidget(QWidget* parent)
    : QDockWidget(parent)
    , m_TitleLayout(nullptr)
    , m_TitleWidget(nullptr)
    , m_Layout(nullptr)
    , m_TitleActive(nullptr)
    , m_TitleDistanceBox(nullptr)
    , m_TitleCrossButton(nullptr)
    , m_TitleLabel(nullptr)
    , m_LodTree(nullptr)
    , m_Lod(nullptr)
    , m_SignalSelectedGuard(false)
{
}

LODLevelWidget::~LODLevelWidget()
{
    if (m_LodTree != nullptr)
    {
        auto item = m_LodTree->currentItem();
        if (item != nullptr)
        {
            CLibraryTreeViewItem* treeitem = static_cast<CLibraryTreeViewItem*>(item);
            emit SignalAttributeViewItemDeleted(treeitem->GetFullPath());
        }
    }

    SAFE_DELETE(m_Layout);
    SAFE_DELETE(m_TitleLayout);
    SAFE_DELETE(m_TitleActive);
    SAFE_DELETE(m_TitleDistanceBox);
    SAFE_DELETE(m_TitleLabel);
    SAFE_DELETE(m_TitleCrossButton);
    SAFE_DELETE(m_TitleWidget);
    SAFE_DELETE(m_LodTree);

    if (m_Lod != nullptr)
    {
        m_Lod->Release();
    }
}

void LODLevelWidget::Init(SLodInfo* lod)
{
    setFeatures(NoDockWidgetFeatures);
    m_Lod = lod;
    m_Lod->AddRef();
    //This widget

    //Title widgets
    m_TitleWidget = new QWidget(this);
    m_TitleWidget->setObjectName("LODLevelTitle");
    setTitleBarWidget(m_TitleWidget);

    m_TitleLayout = new QHBoxLayout(m_TitleWidget);

    m_TitleActive = new QCheckBox(m_TitleWidget);
    m_TitleActive->setChecked(m_Lod->GetActive());
    m_TitleLayout->addWidget(m_TitleActive, 0, Qt::AlignLeft);
    connect(m_TitleActive, &QCheckBox::stateChanged, this, &LODLevelWidget::LODStateChanged);

    m_TitleLabel = new QLabel("LOD Distance", m_TitleWidget);
    m_TitleLayout->addWidget(m_TitleLabel, 0, Qt::AlignLeft);

    m_TitleDistanceBox = new QDoubleSpinBox(m_TitleWidget);
    m_TitleDistanceBox->setMaximum(fHUGE);
    m_TitleDistanceBox->setValue(m_Lod->GetDistance());
    m_TitleLayout->addWidget(m_TitleDistanceBox, 0, Qt::AlignLeft);
    connect(m_TitleDistanceBox, SIGNAL(valueChanged(double)), this, SLOT(onDistanceChanged(double)));

    m_TitleLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

    m_TitleCrossButton = new QPushButton(m_TitleWidget);
    m_TitleCrossButton->setObjectName("LODWidgetCloseButton");
    m_TitleCrossButton->setIcon(QIcon(":/particleQT/buttons/close_btn.png"));
    m_TitleLayout->addWidget(m_TitleCrossButton, 0, Qt::AlignRight);
    connect(m_TitleCrossButton, &QPushButton::pressed, this, &LODLevelWidget::onRemoveLod);

    m_TitleWidget->setLayout(m_TitleLayout);
      
    BuildTreeGUI();

    onContentSizeChanged();
    setObjectName("LODLevelWidget");
}

void LODLevelWidget::RefreshDistance()
{
    m_TitleDistanceBox->setValue(m_Lod->GetDistance());
}


void LODLevelWidget::onActiveChanged(bool active)
{
    m_Lod->SetActive(active);
}

int LODLevelWidget::GetWidgetHeight()
{
    CRY_ASSERT(m_Lod);
    CRY_ASSERT(m_TitleWidget);
    CRY_ASSERT(m_LodTree);
    int totalSize = 0;
    totalSize += m_TitleWidget->height();
    totalSize += m_LodTree->GetContentHeight();
    return totalSize;
}

void LODLevelWidget::onDistanceChanged(double distance)
{
    m_Lod->SetDistance(distance);
    emit SignalDistanceChanged();
}

void LODLevelWidget::onContentSizeChanged()
{
    setFixedHeight(GetWidgetHeight());
    update();
    emit OnContentChanged();
}

void LODLevelWidget::onRemoveLod()
{
    auto item = m_LodTree->currentItem();
    if (item != nullptr)
    {
        CLibraryTreeViewItem* treeitem = static_cast<CLibraryTreeViewItem*>(item);
        emit SignalAttributeViewItemDeleted(treeitem->GetFullPath());
    }
    onContentSizeChanged();
    emit RemoveLod(this, m_Lod);
}

void LODLevelWidget::BuildTreeGUI()
{
    if (m_LodTree != nullptr)
    {
        m_Layout->removeWidget(m_LodTree);
        SAFE_DELETE(m_LodTree);
    }

    m_LodTree = new LODTreeWidget(this);
    m_LodTree->setColumnCount(2);
    m_LodTree->headerItem()->setHidden(true);
    m_LodTree->setSelectionMode(QAbstractItemView::SingleSelection);
    //m_LodTree->hideColumn(0);
    m_LodTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_LodTree, &QTreeWidget::customContextMenuRequested, this, &LODLevelWidget::OnItemContextMenu);

    BuildTreeItem(nullptr, m_Lod->GetTopParticle());

    connect(m_LodTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onItemClicked(QTreeWidgetItem*,int)));
    connect(m_LodTree, &QTreeWidget::currentItemChanged, this, &LODLevelWidget::onCurrentItemChanged);
    connect(m_LodTree, &QTreeWidget::itemExpanded, this, &LODLevelWidget::onContentSizeChanged);
    connect(m_LodTree, &QTreeWidget::itemCollapsed, this, &LODLevelWidget::onContentSizeChanged);


    m_LodTree->header()->moveSection(CHECKBOX_COLUMN, NAME_COLUMN);
    m_LodTree->header()->setStretchLastSection(true);
    m_LodTree->header()->resizeSection(CHECKBOX_COLUMN, 24);

    setWidget(m_LodTree);
}

void LODLevelWidget::onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    if (current != nullptr)
    {
        IDataBaseItem* databaseItem = static_cast<CLibraryTreeViewItem*>(current)->GetItem();
        CRY_ASSERT(databaseItem);
        CParticleItem* particleItem = static_cast<CParticleItem*>(databaseItem);
        CRY_ASSERT(particleItem);
        CRY_ASSERT(particleItem->GetEffect());
        SLodInfo* lod = particleItem->GetEffect()->GetLevelOfDetailByDistance(m_Lod->GetDistance());

        if (lod)
        {
            emit SignalLodParticleItemSelected(this, particleItem->GetEffect(), lod);
            UpdateItemModified((CLibraryTreeViewItem*)current, particleItem);
        }
    }
}

void LODLevelWidget::onItemClicked(QTreeWidgetItem* item, int column)
{
    CRY_ASSERT(item);
    IDataBaseItem* databaseItem = static_cast<CLibraryTreeViewItem*>(item)->GetItem();
    CRY_ASSERT(databaseItem);
    CParticleItem* particleItem = static_cast<CParticleItem*>(databaseItem);
    CRY_ASSERT(particleItem);
    CRY_ASSERT(particleItem->GetEffect());
    ParticleParams params = particleItem->GetEffect()->GetLodParticle(m_Lod)->GetParticleParams();

    switch (item->checkState(CHECKBOX_COLUMN))
    {
    case Qt::Checked:
    case Qt::PartiallyChecked:
        params.bEnabled = true;
        break;
    case Qt::Unchecked:
        params.bEnabled = false;
        break;
    }

    particleItem->GetEffect()->GetLodParticle(m_Lod)->SetParticleParams(params);
    m_Lod->Refresh();
}

void LODLevelWidget::ClearSelectionColor()
{
    m_LodTree->clearSelection();
    m_LodTree->setCurrentItem(m_LodTree->invisibleRootItem());
}



void LODLevelWidget::BuildTreeItem(CLibraryTreeViewItem* parent, IParticleEffect* particle)
{
    if (particle->GetLodParticle(m_Lod) == nullptr)
    {
        return;
    }

    CLibraryTreeViewItem* item = nullptr;
    QString name = "";
    if (parent)
    {
        name = parent->GetItem()->GetFullName();
        name.append('.');
    }
    name.append(particle->GetName());

    item = new CLibraryTreeViewItem(parent, GetIEditor()->GetParticleManager(), name.toUtf8().data(), NAME_COLUMN, CHECKBOX_COLUMN);

    item->setExpanded(true);

    QStringList namelist = name.split(".");

    item->setText(0, namelist[namelist.size() - 1]);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

    if (particle->GetLodParticle(m_Lod)->GetParticleParams().bEnabled)
    {
        item->setCheckState(CHECKBOX_COLUMN, Qt::Checked);
    }
    else
    {
        item->setCheckState(CHECKBOX_COLUMN, Qt::Unchecked);
    }

    for (int i = 0; i < particle->GetChildCount(); i++)
    {
        BuildTreeItem(item, particle->GetChild(i));
    }

    if (parent == nullptr)
    {
        m_LodTree->addTopLevelItem(item);
    }

    UpdateItemModified(item, (CParticleItem*)item->GetItem());
}

void LODLevelWidget::SelectTopItem()
{
    QTreeWidgetItem* item = m_LodTree->topLevelItem(0);
    CRY_ASSERT(item);
    m_LodTree->setCurrentItem(item);
}

CLibraryTreeViewItem* LODLevelWidget::FindTreeItem(CLibraryTreeViewItem* root, const QString& name)
{
    for (int i = 0; i < root->childCount(); i++)
    {
        auto item = root->child(i);
        CLibraryTreeViewItem* treeItem = static_cast<CLibraryTreeViewItem*>(item);
        if (name == treeItem->ToString())
        {
            return treeItem;
        }
        CLibraryTreeViewItem* reItem = FindTreeItem(treeItem, name);
        if (reItem)
        {
            return reItem;
        }
    }
    return nullptr;
}

void LODLevelWidget::SetSelected(const QString& curItemName)
{
    QTreeWidgetItem* itemToSel = nullptr;
    if (!curItemName.isEmpty())
    {
        QString itemPath;
        for (int i = 0; i < m_LodTree->topLevelItemCount() && (!itemToSel); i++)
        {
            auto item = m_LodTree->topLevelItem(i);
            CLibraryTreeViewItem* treeItem = static_cast<CLibraryTreeViewItem*>(item);
            if (curItemName == treeItem->ToString())
            {
                itemToSel = treeItem;
                break;
            }
            itemToSel = FindTreeItem(treeItem, curItemName);
        }        
    }

    if (itemToSel)
    {
        m_LodTree->expandItem(itemToSel);
    }

    m_LodTree->setCurrentItem(itemToSel);

}

void LODLevelWidget::LODStateChanged(int state)
{
    if (state == Qt::Unchecked)
    {
        m_Lod->SetActive(false);
    }
    else
    {
        m_Lod->SetActive(true);        
    }
}

void LODLevelWidget::updateTreeItemStyle(bool ishighlight, SLodInfo* lod, QString fullname)
{
    if (lod != m_Lod)
    {
        return;
    }
    QStringList splitName = fullname.split(".");
    splitName.takeFirst();
    QString itemName = splitName.join(".");

    QList<QTreeWidgetItem*> clist = m_LodTree->findItems(splitName.takeLast(), Qt::MatchContains | Qt::MatchRecursive, 0);
    QTreeWidgetItem* updateitem = nullptr;

    for (QTreeWidgetItem* item : clist)
    {
        CLibraryTreeViewItem* treeitem = static_cast<CLibraryTreeViewItem*>(item);
        if (treeitem == nullptr)
        {
            continue;
        }

        if (treeitem->GetPath() == itemName)
        {
            updateitem = treeitem;
            break;
        }
    }


    if (updateitem)
    {
        CLibraryTreeViewItem* treeitem = static_cast<CLibraryTreeViewItem*>(updateitem);
        CParticleItem* particleItem = static_cast<CParticleItem*>(treeitem->GetItem());
        if (!ishighlight)
        {
            treeitem->setTextColor(NAME_COLUMN, QColor(255, 153, 0));
        }
    }
}

void LODLevelWidget::OnItemContextMenu(const QPoint& pos)
{
    CLibraryTreeViewItem* treeitem = static_cast<CLibraryTreeViewItem*>(m_LodTree->itemAt(pos));
    if (treeitem == nullptr)
    {
        return;
    }
    QMenu* menu = new QMenu(this);
    //ignore selection
    QAction* removeaction = menu->addAction("Remove");

    bool* removed = new bool(false);
    connect(removeaction, &QAction::triggered, this, [=]()
        {
            //TODO:
            //ADD BACK UP SUPPORT FOR REMOVE ITEM
            CParticleItem* particleItem = static_cast<CParticleItem*>(treeitem->GetItem());
            CRY_ASSERT(particleItem);
            CRY_ASSERT(particleItem->GetEffect());
            particleItem->GetEffect()->RemoveLevelOfDetail(m_Lod);

            if (treeitem->parent())
            {
                m_LodTree->setCurrentItem(treeitem->parent());
            }

            emit SignalUpdateLODIcon(treeitem);
            emit SignalAttributeViewItemDeleted(treeitem->GetFullPath());

            *removed = true;
        });

    menu->exec(mapToGlobal(pos));
    if (*removed)
    {
        RefreshGUIDetour();
    }
    delete removed;
}

void LODLevelWidget::RefreshGUIDetour()
{
    emit SignalRefreshGUI();
}

void LODLevelWidget::UpdateItemModified(CLibraryTreeViewItem* parent, CParticleItem* particle)
{
    CParticleUIDefinition* m_pParticleUI  = new CParticleUIDefinition();
    CVarBlockPtr m_vars = m_pParticleUI->CreateVars();
    m_pParticleUI->SetFromParticles((CParticleItem*)particle, m_Lod);


    bool hasDefault = true;
    for (int i = 0; i < m_vars->GetNumVariables(); i++)
    {
        auto * var = m_vars->GetVariable(i);
        if (var && !var->HasDefaultValue())
        {
            hasDefault = false;
        }
    }

    if (m_Lod == nullptr)
    {
        return;
    }

    if (!hasDefault)
    {
        parent->setTextColor(CHECKBOX_COLUMN, QColor(255, 153, 0));
    }
    else
    {
        parent->setTextColor(NAME_COLUMN, QColor("white"));
    }
}

SLodInfo* LODLevelWidget::GetLod()
{
    return m_Lod;
}

#include <QT/LODLevelWidget.moc>