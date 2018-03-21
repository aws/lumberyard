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

#include "StdAfx.h"
#include "FlowGraphSearchCtrl.h"
#include "FlowGraphManager.h"
#include "FlowGraphHelpers.h"
#include "FlowGraphNode.h"
#include "HyperGraphDialog.h"
#include "CommentNode.h"
#include "Util/CryMemFile.h"                    // CCryMemFile
#include "QtUtilWin.h"
#include "QAbstractQVariantTreeDataModel.h"

#include <Objects/EntityObject.h>
#include <IAIAction.h>

#include <vector>
#include <QMenu>
#include <QTreeView>
#include <QKeyEvent>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <HyperGraph/ui_FlowGraphSearchCtrl.h>

#undef GetClassName

#define MAX_HISTORY_ENTRIES 20

CFlowGraphSearchOptions* CFlowGraphSearchOptions::GetSearchOptions()
{
    static CFlowGraphSearchOptions options;
    return &options;
}

CFlowGraphSearchOptions::CFlowGraphSearchOptions(const CFlowGraphSearchOptions& other)
{
    assert (0);
}

CFlowGraphSearchOptions::CFlowGraphSearchOptions()
{
    this->m_strFind   = "";
    this->m_bIncludePorts    = FALSE;
    this->m_bIncludeValues   = FALSE;
    this->m_bIncludeEntities = FALSE;
    this->m_bIncludeIDs      = FALSE;
    this->m_bExactMatch      = FALSE; // not serialized!
    this->m_LookinIndex = eFL_Current;
    this->m_findSpecial = eFLS_None;
}

void CFlowGraphSearchOptions::Serialize(bool bLoading)
{
    QSettings settings;
    const QString strSection("FlowGraphSearchHistory");
    settings.beginGroup(strSection);

    if (!bLoading)
    {
        settings.setValue("IncludePorts", m_bIncludePorts);
        settings.setValue("IncludeValues", m_bIncludeValues);
        settings.setValue("IncludeEntities", m_bIncludeEntities);
        settings.setValue("IncludeIDs", m_bIncludeIDs);
        settings.setValue("LookIn", m_LookinIndex);
        settings.setValue("Special", m_findSpecial);
        settings.setValue("LastFind", m_strFind);
        settings.setValue("Count", m_lstFindHistory.size());
        int i = 0;
        Q_FOREACH(QString value, m_lstFindHistory)
        {
            settings.setValue(QStringLiteral("Item%1").arg(i), value);
            ++i;
        }
    }
    else
    {
        m_bIncludePorts = settings.value("IncludePorts", false).toBool();
        m_bIncludeValues = settings.value("IncludeValues", false).toBool();
        m_bIncludeEntities = settings.value("IncludeEntities", false).toBool();
        m_bIncludeIDs = settings.value("IncludeIDs", false).toBool();
        m_strFind = settings.value("LastFind").toString();
        m_lstFindHistory.clear();
        m_LookinIndex = settings.value("LookIn", eFL_Current).toInt();
        m_findSpecial = settings.value("Special", eFLS_None).toInt();
        int count = settings.value("Count", 0).toInt();
        for (int i = 0; i < count; ++i)
        {
            const QString item = settings.value(QStringLiteral("Item%1").arg(i)).toString();
            if (item.isEmpty())
            {
                break;
            }
            m_lstFindHistory << item;
        }
    }
}

void AddComboHistory(QComboBox* cmb, QString strText, QStringList& lstHistory)
{
    if (strText.isEmpty())
    {
        return;
    }
    if (cmb->findText(strText) != -1)
    {
        return;
    }
    cmb->addItem(strText);

    lstHistory.clear();
    for (int i = 0; i < cmb->count(); i++)
    {
        lstHistory << cmb->itemText(i);
    }
}

void RestoryCombo(QComboBox* cmb, QStringList& lstHistory, QString lpszDefault = QString())
{
    cmb->clear();
    if (!lstHistory.empty())
    {
        cmb->addItems(lstHistory);
    }
    else if (!lpszDefault.isEmpty())
    {
        cmb->addItem(lpszDefault);
        lstHistory << lpszDefault;
    }
}

//////////////////////////////////////////////////////////////////////////
CFlowGraphSearchCtrl::CFlowGraphSearchCtrl(CHyperGraphDialog* pDialog)
    : AzQtComponents::StyledDockWidget(pDialog)
    , ui(new Ui::FlowGraphSearchCtrl())
{
    setWindowTitle("Search");
    m_pResultsCtrl = 0;
    m_pDialog = pDialog;

    QWidget* w = new QWidget(this);
    ui->setupUi(w);
    setWidget(w);

    CFlowGraphSearchOptions::GetSearchOptions()->Serialize(true);
    RestoryCombo(ui->findWhat, CFlowGraphSearchOptions::GetSearchOptions()->m_lstFindHistory);
    ui->includePorts->setChecked(CFlowGraphSearchOptions::GetSearchOptions()->m_bIncludePorts);
    ui->includeValues->setChecked(CFlowGraphSearchOptions::GetSearchOptions()->m_bIncludeValues);
    ui->includeEntities->setChecked(CFlowGraphSearchOptions::GetSearchOptions()->m_bIncludeEntities);
    ui->includeIDs->setChecked(CFlowGraphSearchOptions::GetSearchOptions()->m_bIncludeIDs);
    ui->lookIn->setCurrentIndex(CFlowGraphSearchOptions::GetSearchOptions()->m_LookinIndex);
    ui->special->setCurrentIndex(CFlowGraphSearchOptions::GetSearchOptions()->m_findSpecial);

    setFocusProxy(ui->findWhat);

    connect(ui->includePorts, &QCheckBox::toggled, this, &CFlowGraphSearchCtrl::OnIncludePortsClicked);
    connect(ui->includeValues, &QCheckBox::toggled, this, &CFlowGraphSearchCtrl::OnIncludeValuesClicked);
    connect(ui->includeEntities, &QCheckBox::toggled, this, &CFlowGraphSearchCtrl::OnIncludeEntitiesClicked);
    connect(ui->includeIDs, &QCheckBox::toggled, this, &CFlowGraphSearchCtrl::OnIncludeIDsClicked);
    connect(ui->findWhat, SIGNAL(currentIndexChanged(int)), this, SLOT(OnOptionChanged()));
    connect(ui->findWhat->lineEdit(), SIGNAL(editingFinished()), this, SLOT(OnButtonFindAll()));
    connect(ui->lookIn, SIGNAL(currentIndexChanged(int)), this, SLOT(OnOptionChanged()));
    connect(ui->special, SIGNAL(currentIndexChanged(int)), this, SLOT(OnOptionChanged()));
    connect(ui->findAll, &QPushButton::clicked, this, &CFlowGraphSearchCtrl::OnButtonFindAll);
}

//////////////////////////////////////////////////////////////////////////
CFlowGraphSearchCtrl::~CFlowGraphSearchCtrl()
{
    CFlowGraphSearchOptions::GetSearchOptions()->Serialize(false);
}


uint32 Option2Cat(CFlowGraphSearchOptions::EFindSpecial o)
{
    switch (o)
    {
    case CFlowGraphSearchOptions::eFLS_Approved:
        return EFLN_APPROVED;
    case CFlowGraphSearchOptions::eFLS_Advanced:
        return EFLN_ADVANCED;
    case CFlowGraphSearchOptions::eFLS_Debug:
        return EFLN_DEBUG;
    //case CFlowGraphSearchOptions::eFLS_Legacy:
    //  return EFLN_LEGACY;
    //case CFlowGraphSearchOptions::eFLS_WIP:
    //  return EFLN_WIP;
    case CFlowGraphSearchOptions::eFLS_Obsolete:
        return EFLN_OBSOLETE;
    default:
        return EFLN_DEBUG;
    }
}


//////////////////////////////////////////////////////////////////////////
class CFinder
{
    struct Matcher
    {
        enum Method
        {
            SLOPPY = 0,
            EXACT_MATCH,
        };

        bool operator() (const char* a, const char* b) const
        {
            switch (m)
            {
            default:
            case SLOPPY:
                return strstri(a, b) != 0;
                break;
            case EXACT_MATCH:
                return strcmp(a, b) == 0;
                break;
            }
        }
        bool operator() (const QString& a, const QString& b) const
        {
            switch (m)
            {
            default:
            case SLOPPY:
                return a.contains(b, Qt::CaseInsensitive);
                break;
            case EXACT_MATCH:
                return a == b;
                break;
            }
        }
        Matcher(Method m)
            : m(m) {}
        Method m;
    };


public:
    CFinder (CFlowGraphSearchOptions* pOptions, CFlowGraphSearchCtrl::ResultVec& resultVec)
        : m_pOptions (pOptions)
        , m_resultVec(resultVec)
        , m_matcher(pOptions->m_bExactMatch ? Matcher::EXACT_MATCH : Matcher::SLOPPY)
    {
        m_cat = Option2Cat((CFlowGraphSearchOptions::EFindSpecial) m_pOptions->m_findSpecial);
    }

    bool Accept (CFlowGraph* pFG, CHyperNode* pNode, HyperNodeID id)
    {
        switch (m_pOptions->m_LookinIndex)
        {
        case CFlowGraphSearchOptions::eFL_AIActions:
            if (pFG->GetAIAction() == 0)
            {
                return false;
            }
            break;
        case CFlowGraphSearchOptions::eFL_CustomActions:
            if (pFG->GetCustomAction() == 0)
            {
                return false;
            }
            break;
        case CFlowGraphSearchOptions::eFL_Entities:
            if (pFG->GetEntity() == 0)
            {
                return false;
            }
            break;
        }

        const Matcher& matches = m_matcher; // only for nice wording "matches"

        QString context;
        const QString& nodeName = pNode->GetTitle();

        // check if it's an special editor node
        if (pNode->IsEditorSpecialNode())
        {
            if (m_pOptions->m_findSpecial == CFlowGraphSearchOptions::eFLS_None)
            {
                if (matches(nodeName, m_pOptions->m_strFind) != 0)
                {
                    context = QString("%1: %2 (ID=%3)").arg(pNode->GetUIClassName()).arg(nodeName).arg(id);
                    m_resultVec.push_back(CFlowGraphSearchCtrl::Result(pFG, id, context));
                    return true;
                }
            }
            return false;
        }

        // we can be sure it is a normal node.
        CFlowNode* pFlowNode = static_cast<CFlowNode*> (pNode);

        if (m_pOptions->m_findSpecial == CFlowGraphSearchOptions::eFLS_None)
        {
            if (matches(nodeName, m_pOptions->m_strFind) != 0)
            {
                context = QObject::tr("Node: %1 (ID=%2)").arg(nodeName).arg(id);
                m_resultVec.push_back(CFlowGraphSearchCtrl::Result(pFG, id, context));
            }

            if (m_pOptions->m_bIncludeIDs)
            {
                char ids[32];
                _snprintf(ids, sizeof(ids) - 1, "(ID=%d)", id);
                ids[sizeof(ids) - 1] = 0;
                if (matches(ids, m_pOptions->m_strFind.toUtf8().data()) != 0)
                {
                    context = QObject::tr("Node: %1 (ID=%2)").arg(nodeName).arg(id);
                    m_resultVec.push_back(CFlowGraphSearchCtrl::Result(pFG, id, context));
                }
            }

            if (m_pOptions->m_bIncludeEntities)
            {
                CEntityObject* pEntity = pFlowNode->GetEntity();
                if (pEntity != 0 && matches(pEntity->GetName(), m_pOptions->m_strFind.toUtf8().data()) != 0)
                {
                    context = QObject::tr("Entity: %1").arg(pEntity->GetName());
                    m_resultVec.push_back(CFlowGraphSearchCtrl::Result(pFG, id, context));
                }
            }

            if (m_pOptions->m_bIncludePorts)
            {
                const CHyperNode::Ports& inputs = *pNode->GetInputs();
                for (int i = 0; i < inputs.size(); ++i)
                {
                    if (inputs[i].pVar != 0 && (matches(inputs[i].pVar->GetHumanName(), m_pOptions->m_strFind.toUtf8().data()) != 0))
                    {
                        context = QObject::tr("InPort: %1").arg(inputs[i].pVar->GetHumanName());
                        m_resultVec.push_back(CFlowGraphSearchCtrl::Result(pFG, id, context));
                    }
                }
                const CHyperNode::Ports& outputs = *pNode->GetOutputs();
                for (int i = 0; i < outputs.size(); ++i)
                {
                    if (outputs[i].pVar != 0 && (matches(outputs[i].pVar->GetHumanName(), m_pOptions->m_strFind.toUtf8().data()) != 0))
                    {
                        context = QObject::tr("OutPort: %1").arg(outputs[i].pVar->GetHumanName());
                        m_resultVec.push_back(CFlowGraphSearchCtrl::Result(pFG, id, context));
                    }
                }
            }
            if (m_pOptions->m_bIncludeValues)
            {
                // check port values
                QString val;
                CVarBlock* pVarBlock = pNode->GetInputsVarBlock();
                if (pVarBlock != 0)
                {
                    for (int i = 0; i < pVarBlock->GetNumVariables(); ++i)
                    {
                        IVariable* pVar = pVarBlock->GetVariable(i);
                        pVar->Get(val);
                        if (matches(val, m_pOptions->m_strFind) != 0)
                        {
                            context = QObject::tr("Value: %1=%2").arg(pVar->GetHumanName()).arg(val);
                            m_resultVec.push_back(CFlowGraphSearchCtrl::Result(pFG, id, context));
                        }
                    }
                    delete pVarBlock;
                }
            }
        }
        else
        {
            // special find (category or NoEntity)
            if (m_pOptions->m_findSpecial == CFlowGraphSearchOptions::eFLS_NoEntity)
            {
                if (pNode->CheckFlag(EHYPER_NODE_ENTITY))
                {
                    bool bEntityPortConnected = false;
                    if (pNode->GetInputs() && (*pNode->GetInputs())[0].nConnected != 0)
                    {
                        bEntityPortConnected = true;
                    }

                    bool valid = (pNode->CheckFlag(EHYPER_NODE_ENTITY_VALID) ||
                                  pNode->CheckFlag(EHYPER_NODE_GRAPH_ENTITY) ||
                                  pNode->CheckFlag(EHYPER_NODE_GRAPH_ENTITY2) ||
                                  bEntityPortConnected);
                    if (!valid)
                    {
                        context = QObject::tr("No TargetEntity");
                        m_resultVec.push_back(CFlowGraphSearchCtrl::Result(pFG, id, context));
                    }
                }
            }
            // special find (NoLinks), not that fast
            else if (m_pOptions->m_findSpecial == CFlowGraphSearchOptions::eFLS_NoLinks)
            {
                bool bConnected = false;
                for (int pass = 0; pass < 2; ++pass)
                {
                    CHyperNode::Ports* pPorts = pass == 0 ? pNode->GetInputs() : pNode->GetOutputs();
                    for (int i = 0; i < pPorts->size(); ++i)
                    {
                        if ((*pPorts)[i].nConnected != 0)
                        {
                            bConnected = true;
                            break;
                        }
                    }
                    if (bConnected)
                    {
                        break;
                    }
                }
                if (bConnected == false)
                {
                    context = QObject::tr("No Links");
                    m_resultVec.push_back(CFlowGraphSearchCtrl::Result(pFG, id, context));
                }
            }
            else
            {
                // category
                if (pFlowNode->GetCategory() == m_cat)
                {
                    context = QObject::tr("Category: %1").arg(pFlowNode->GetCategoryName());
                    m_resultVec.push_back(CFlowGraphSearchCtrl::Result(pFG, id, context));
                }
            }
        }
        return false;
    }

protected:
    CFlowGraphSearchOptions* m_pOptions;
    CFlowGraphSearchCtrl::ResultVec& m_resultVec;
    Matcher m_matcher;
    uint32 m_cat;
};

//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchCtrl::DoTheFind(ResultVec& vec)
{
    CFlowGraphSearchOptions* pOptions = CFlowGraphSearchOptions::GetSearchOptions();
    CFinder finder (pOptions, vec);

    if (pOptions->m_LookinIndex == CFlowGraphSearchOptions::eFL_Current)
    {
        CFlowGraph* pFG = (CFlowGraph*) m_pDialog->GetGraph();
        if (!pFG)
        {
            return;
        }

        IHyperGraphEnumerator* pEnum = pFG->GetNodesEnumerator();
        for (IHyperNode* pINode = pEnum->GetFirst(); pINode != 0; pINode = pEnum->GetNext())
        {
            finder.Accept(pFG, (CHyperNode*) pINode, pINode->GetId());
        }
        pEnum->Release();
    }
    else
    {
        CFlowGraphManager* pFGMgr = GetIEditor()->GetFlowGraphManager();
        int numFG = pFGMgr->GetFlowGraphCount();
        for (int i = 0; i < numFG; ++i)
        {
            CFlowGraph* pFG = pFGMgr->GetFlowGraph(i);
            IHyperGraphEnumerator* pEnum = pFG->GetNodesEnumerator();
            for (IHyperNode* pINode = pEnum->GetFirst(); pINode != 0; pINode = pEnum->GetNext())
            {
                finder.Accept(pFG, (CHyperNode*) pINode, pINode->GetId());
            }
            pEnum->Release();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchCtrl::OnButtonFindAll()
{
    FindAll(false);
}
//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchCtrl::OnIncludePortsClicked()
{
    CFlowGraphSearchOptions::GetSearchOptions()->m_bIncludePorts = ui->includePorts->isChecked();
    CFlowGraphSearchOptions::GetSearchOptions()->Serialize(false);
}
//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchCtrl::OnIncludeValuesClicked()
{
    CFlowGraphSearchOptions::GetSearchOptions()->m_bIncludeValues = ui->includeValues->isChecked();
    CFlowGraphSearchOptions::GetSearchOptions()->Serialize(false);
}
//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchCtrl::OnIncludeEntitiesClicked()
{
    CFlowGraphSearchOptions::GetSearchOptions()->m_bIncludeEntities = ui->includeEntities->isChecked();
    CFlowGraphSearchOptions::GetSearchOptions()->Serialize(false);
}
//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchCtrl::OnIncludeIDsClicked()
{
    CFlowGraphSearchOptions::GetSearchOptions()->m_bIncludeIDs = ui->includeIDs->isChecked();
    CFlowGraphSearchOptions::GetSearchOptions()->Serialize(false);
}
//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchCtrl::FindAll(bool bSelectFirst)
{
    assert (m_pResultsCtrl != 0);

    // update search options
    OnOptionChanged();
    CFlowGraphSearchOptions* pOptions = CFlowGraphSearchOptions::GetSearchOptions();

    // add search string to history
    AddComboHistory(ui->findWhat, pOptions->m_strFind, pOptions->m_lstFindHistory);
    pOptions->Serialize(false);

    // perform search
    ResultVec resultVec;
    DoTheFind(resultVec);
    // update result control
    m_pResultsCtrl->Populate(resultVec);

    // force showing of results pane if we found something
    m_pDialog->ShowResultsPane(true, resultVec.size() != 0);

    if (bSelectFirst)
    {
        m_pResultsCtrl->SelectFirstResult();
        m_pResultsCtrl->ShowSelectedResult();
    }

    CFlowGraphSearchOptions::GetSearchOptions()->Serialize(false);

    // keep focus if no results
    // if (resultVec.size () == 0)
    //  GotoDlgCtrl(&m_cmbFind);
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchCtrl::UpdateOptions()
{
    bool enable = CFlowGraphSearchOptions::GetSearchOptions()->m_findSpecial == CFlowGraphSearchOptions::eFLS_None;
    ui->findAll->setEnabled(enable);
    ui->includePorts->setEnabled(enable);
    ui->includeValues->setEnabled(enable);
    ui->includeEntities->setEnabled(enable);
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchCtrl::OnOptionChanged()
{
    UpdateOptions();
    CFlowGraphSearchOptions::GetSearchOptions()->m_strFind = ui->findWhat->currentText();
    CFlowGraphSearchOptions::GetSearchOptions()->m_LookinIndex = ui->lookIn->currentIndex();
    CFlowGraphSearchOptions::GetSearchOptions()->m_findSpecial = ui->special->currentIndex();
    CFlowGraphSearchOptions::GetSearchOptions()->Serialize(false);
}

// called from CHyperGraphDialog...
void CFlowGraphSearchCtrl::SetResultsCtrl(CFlowGraphSearchResultsCtrl* pCtrl)
{
    m_pResultsCtrl = pCtrl;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchCtrl::Find(const QString& searchString, bool bSetTextOnly, bool bSelectFirst, bool bTempExactMatch)
{
    CFlowGraphSearchOptions::GetSearchOptions()->m_strFind = searchString;
    CFlowGraphSearchOptions::GetSearchOptions()->m_bExactMatch = bTempExactMatch ? TRUE : FALSE;
    UpdateOptions();
    if (bSetTextOnly == false)
    {
        FindAll(bSelectFirst);
    }
    CFlowGraphSearchOptions::GetSearchOptions()->m_bExactMatch = FALSE;
}




//////////////////////////////////////////////////////////////////////////
class CFlowGraphManagerPrototypeFilteredModel;

Q_DECLARE_METATYPE(CHyperNode*)


class CFlowGraphSearchResultsModel
    : public QAbstractQVariantTreeDataModel
{
public:
    CFlowGraphSearchResultsModel(QObject* parent)
        : QAbstractQVariantTreeDataModel(parent) { GroupByField(-1); }

    void GroupByField(int field);
    bool IsGrouping() const { return m_groupField != -1; }
    void Populate(const CFlowGraphSearchCtrl::ResultVec& v);

    CHyperNode* node(const QModelIndex& index) const;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    enum
    {
        GraphRole = Qt::UserRole,
        NodeIDRole,
        NodeRole,
        NodePtrRole,
        ContextRole
    };
    void rebuild();

    int m_groupField;
    CFlowGraphSearchCtrl::ResultVec m_results;
    std::vector<int> m_columns;
};

void CFlowGraphSearchResultsModel::Populate(const CFlowGraphSearchCtrl::ResultVec& results)
{
    m_results = results;
    rebuild();
}

void CFlowGraphSearchResultsModel::GroupByField(int field)
{
    m_groupField = field;
    switch (m_groupField)
    {
    case 0:
        m_columns.clear();
        m_columns.push_back(NodeRole);
        m_columns.push_back(ContextRole);
        break;
    case 1:
        m_columns.clear();
        m_columns.push_back(GraphRole);
        m_columns.push_back(ContextRole);
        break;
    case 2:
        m_columns.clear();
        m_columns.push_back(GraphRole);
        m_columns.push_back(NodeRole);
        break;
    default:
        m_columns.clear();
        m_columns.push_back(GraphRole);
        m_columns.push_back(NodeRole);
        m_columns.push_back(ContextRole);
        break;
    }
    rebuild();
}

CHyperNode* CFlowGraphSearchResultsModel::node(const QModelIndex& index) const
{
    Item* item = itemFromIndex(index);
    if (0 == item)
    {
        return 0;
    }

    if (!item->m_data.contains(NodePtrRole))
    {
        return 0;
    }

    QVariant v = item->m_data[NodePtrRole];
    return v.value<CHyperNode*>();
}

void CFlowGraphSearchResultsModel::rebuild()
{
    beginResetModel();
    m_root.reset(new Folder("root"));

    typedef std::map< QString, std::shared_ptr<Folder> > GraphFolders;
    GraphFolders gf;

    for (auto& v : m_results)
    {
        CHyperNode* pNode = (CHyperNode*)v.m_pGraph->FindNode(v.m_nodeId);
        assert(pNode != 0);

        std::shared_ptr<Folder> folder = m_root;
        QString key;
        switch (m_groupField)
        {
        case 0: // group by graph
            key = v.m_pGraph->GetName();
            break;
        case 1: // group by node
            key = QString("%1 (ID=%2)").arg(pNode->GetTitle()).arg(v.m_nodeId);
            break;
        case 2: // group by context
            key = v.m_context;
            break;
        }

        if (!key.isEmpty())
        {
            auto it = gf.find(key);
            if (it == gf.end())
            {
                folder = std::make_shared<Folder>(key);
                m_root->addChild(folder);
                gf[key] = folder;
            }
            else
            {
                folder = (*it).second;
            }
        }

        std::shared_ptr<Item> item(new Item());
        item->m_data[GraphRole] = v.m_pGraph->GetName();
        item->m_data[ContextRole] = v.m_context;
        item->m_data[NodeIDRole] = v.m_nodeId;
        item->m_data[NodeRole] = QString("%1 (ID=%2)").arg(pNode->GetTitle()).arg(v.m_nodeId);

        QVariant var;
        var.setValue<CHyperNode*>(pNode);
        item->m_data[NodePtrRole] = var;

        folder->addChild(item);
    }
    endResetModel();
}

int CFlowGraphSearchResultsModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
    return m_columns.size();
}

QVariant CFlowGraphSearchResultsModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
    Item* item = itemFromIndex(index);
    if (0 == item)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        if (item->asFolder())
        {
            if (index.column() != 0)
            {
                return QVariant();
            }
        }
        else
        {
            role = m_columns[index.column()];
        }
    }

    if (item->m_data.contains(role))
    {
        return item->m_data[role];
    }
    return QVariant();
}

QVariant CFlowGraphSearchResultsModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
    if (role == Qt::DisplayRole)
    {
        switch (m_columns[section])
        {
        case GraphRole:
            return "Graph";
        case NodeRole:
            return "Node";
        case ContextRole:
            return "Context";
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}


class CFlowGraphSearchResultsProxyModel
    : public QSortFilterProxyModel
{
public:
    CFlowGraphSearchResultsProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent) { }
};



//////////////////////////////////////////////////////////////////////////
class CFlowGraphSearchResultsView
    : public QTreeView
{
public:
    CFlowGraphSearchResultsView(CFlowGraphSearchResultsCtrl* parent)
        : QTreeView(parent)
        , m_parent(parent) { }

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    CFlowGraphSearchResultsCtrl*    m_parent;
};

void CFlowGraphSearchResultsView::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        m_parent->ShowSelectedResult();
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
CFlowGraphSearchResultsCtrl::CFlowGraphSearchResultsCtrl(CHyperGraphDialog* pDialog)
    : AzQtComponents::StyledDockWidget(pDialog)
    , m_model(0)
{
    setWindowTitle("Search Results");
    m_pDialog = pDialog;
    m_dataView = new CFlowGraphSearchResultsView(this);
    setWidget(m_dataView);

    m_dataView->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    m_dataView->setSortingEnabled(true);
    connect(m_dataView->header(), &QHeaderView::customContextMenuRequested, this, &CFlowGraphSearchResultsCtrl::OnReportColumnRClick);
    connect(m_dataView, &QAbstractItemView::doubleClicked, this, &CFlowGraphSearchResultsCtrl::ShowSelectedResult);
}

void CFlowGraphSearchResultsCtrl::Populate(const CFlowGraphSearchCtrl::ResultVec& results)
{
    if (0 == m_model)
    {
        m_model = new CFlowGraphSearchResultsModel(this);
        m_proxyModel = new CFlowGraphSearchResultsProxyModel(this);
        m_model->Populate(results);
        m_proxyModel->setSourceModel(m_model);
        m_dataView->setModel(m_proxyModel);
    }
    else
    {
        m_model->Populate(results);
    }
}

//////////////////////////////////////////////////////////////////////////
// KDAB_PORT_UNUSED not ported, does nothing
// void CFlowGraphSearchResultsCtrl::OnReportItemHyperlink( NMHDR* pNotifyStruct, LRESULT* result )
// {
//  XTP_NM_REPORTRECORDITEM* pItemNotify = (XTP_NM_REPORTRECORDITEM*) pNotifyStruct;
// }

#define ID_GROUP_BY_THIS  1
#define ID_COLUMN_BEST_FIT  2
#define ID_SORT_ASC  3
#define ID_SORT_DESC  4
#define ID_SHOW_GROUPBOX 5
#define ID_UNGROUP 6
#define ID_ENABLE_PREVIEW 7

#define ID_COLUMN_SHOW 100

//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchResultsCtrl::OnReportColumnRClick(const QPoint& pos)
{
    QMenu menu;

    QMenu* menuColumns = menu.addMenu("Columns");
    int nColumnCount = m_model->columnCount();
    for (int nColumn = 0; nColumn < nColumnCount; ++nColumn)
    {
        QAction* a = menuColumns->addAction(m_model->headerData(nColumn, Qt::Horizontal, Qt::DisplayRole).toString());
        a->setData(ID_COLUMN_SHOW + nColumn);
        a->setCheckable(true);
        a->setChecked(!m_dataView->isColumnHidden(nColumn));
        if (nColumn == 0 && m_model->IsGrouping())
        {
            a->setEnabled(false);   // can't hide first column as it breaks ability to open/close the folder
        }
    }
    menu.addAction("Sort ascending")->setData(ID_SORT_ASC);
    menu.addAction("Sort descending")->setData(ID_SORT_DESC);
    menu.addSeparator();
    QAction* groupByAction = menu.addAction("Group by this field");
    groupByAction->setData(ID_GROUP_BY_THIS);
    groupByAction->setEnabled(!m_model->IsGrouping());
    menu.addAction("Ungroup")->setData(ID_UNGROUP);
    // KDAB_PORT_UNUSED This functionality is not supported by QTreeView
    // menu.addSeparator();
    // menu.addAction("Enable Preview")->setData(ID_ENABLE_PREVIEW);

    QAction* res = menu.exec(QCursor::pos());
    if (!res)
    {
        return;
    }
    int nMenuResult = res->data().toInt();

    if (nMenuResult >= ID_COLUMN_SHOW)
    {
        int nColumn = nMenuResult - ID_COLUMN_SHOW;
        m_dataView->setColumnHidden(nColumn, !m_dataView->isColumnHidden(nColumn));
    }

    // other general items
    switch (nMenuResult)
    {
    case ID_SORT_ASC:
    case ID_SORT_DESC:
        if (m_model)
        {
            int column = m_dataView->columnAt(pos.x());
            m_dataView->sortByColumn(column, nMenuResult == ID_SORT_DESC ? Qt::DescendingOrder : Qt::AscendingOrder);
        }
        break;
    case ID_UNGROUP:
        GroupBy(-1);
        break;
    case ID_GROUP_BY_THIS:
        GroupBy(m_dataView->columnAt(pos.x()));
        break;
        // KDAB_PORT_UNUSED: not implemented
        //  case ID_SHOW_GROUPBOX:
        //      ShowGroupBy(!IsGroupByVisible());
        //      break;
        //  case ID_COLUMN_BEST_FIT:
        //      GetColumns()->GetReportHeader()->BestFit(pColumn);
        //      break;
        //  case ID_ENABLE_PREVIEW:
        //      EnablePreviewMode(!IsPreviewMode());
        //      Populate();
        //      break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchResultsCtrl::GroupBy(int column)
{
    // show all the columns otherwise the view gets confused when the columns change in the model
    int nColumnCount = m_model->columnCount();
    for (int nColumn = 0; nColumn < nColumnCount; ++nColumn)
    {
        m_dataView->setColumnHidden(nColumn, false);
    }
    m_model->GroupByField(column);
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchResultsCtrl::SelectFirstResult()
{
    m_dataView->setCurrentIndex(m_proxyModel->index(0, 0));
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphSearchResultsCtrl::ShowSelectedResult()
{
    CHyperNode* node = m_model->node(m_proxyModel->mapToSource(m_dataView->currentIndex()));
    if (node)
    {
        m_pDialog->SetGraph((CFlowGraph*) node->GetGraph());
        m_pDialog->ShowAndSelectNode(node, true);
    }
}


#include <HyperGraph/FlowGraphSearchCtrl.moc>
