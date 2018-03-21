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
#include "MannContextEditorDialog.h"
#include "MannequinDialog.h"
#include "MannNewContextDialog.h"
#include <Mannequin/ui_MannContextEditorDialog.h>
#include "Util/AbstractSortModel.h"
#include "QtUtilWin.h"
#include "QtUtil.h"

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

#include <StringUtils.h>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <QInputDialog>

namespace
{
    inline QString BoolToString(bool b)
    {
        return b ? "True" : "False";
    }

    inline QString VecToString(const Vec3& v)
    {
        char szBuf[0x80];
        sprintf (szBuf, "%g, %g, %g", v.x, v.y, v.z);
        return szBuf;
    }

    inline QString AngToString(const Ang3& a)
    {
        char szBuf[0x80];
        sprintf (szBuf, "%g, %g, %g", a.x, a.y, a.z);
        return szBuf;
    }
}


//////////////////////////////////////////////////////////////////////////

enum EContextReportColumn
{
    CONTEXTCOLUMN_INDEX,
    CONTEXTCOLUMN_NAME,
    CONTEXTCOLUMN_ENABLED,
    CONTEXTCOLUMN_DATABASE,
    CONTEXTCOLUMN_CONTEXTID,
    CONTEXTCOLUMN_TAGS,
    CONTEXTCOLUMN_FRAGTAGS,
    CONTEXTCOLUMN_MODEL,
    CONTEXTCOLUMN_ATTACHMENT,
    CONTEXTCOLUMN_ATTACHMENTCONTEXT,
    CONTEXTCOLUMN_ATTACHMENTHELPER,
    CONTEXTCOLUMN_STARTPOSITION,
    CONTEXTCOLUMN_STARTROTATION,
    CONTEXTCOLUMN_SLAVE_CONTROLLER_DEF,
    CONTEXTCOLUMN_SLAVE_DATABASE,
};

class CXTPMannContextRecord
{
public:
    CXTPMannContextRecord(SScopeContextData* contextData)
        : m_contextData(contextData)
    {
        const SControllerDef* controllerDef = CMannequinDialog::GetCurrentInstance()->Contexts()->m_controllerDef;

        m_data.insert(CONTEXTCOLUMN_INDEX, m_contextData->dataID);
        m_data.insert(CONTEXTCOLUMN_NAME, m_contextData->name);
        m_data.insert(CONTEXTCOLUMN_ENABLED, BoolToString(m_contextData->startActive));
        m_data.insert(CONTEXTCOLUMN_DATABASE, QtUtil::ToQString(m_contextData->database ? m_contextData->database->GetFilename() : ""));
        m_data.insert(CONTEXTCOLUMN_CONTEXTID, QtUtil::ToQString(m_contextData->contextID == CONTEXT_DATA_NONE ? "" : controllerDef->m_scopeContexts.GetTagName(m_contextData->contextID)));

        CryStackStringT<char, 512> sTags;
        controllerDef->m_tags.FlagsToTagList(m_contextData->tags, sTags);
        m_data.insert(CONTEXTCOLUMN_TAGS, QtUtil::ToQString(sTags.c_str()));

        m_data.insert(CONTEXTCOLUMN_FRAGTAGS, QtUtil::ToQString(m_contextData->fragTags));

        QString modelName;
        if (m_contextData->viewData[0].charInst)
        {
            modelName = m_contextData->viewData[0].charInst->GetFilePath();
        }
        else if (m_contextData->viewData[0].pStatObj)
        {
            modelName = m_contextData->viewData[0].pStatObj->GetFilePath();
        }
        m_data.insert(CONTEXTCOLUMN_MODEL, modelName);

        m_data.insert(CONTEXTCOLUMN_ATTACHMENT, QtUtil::ToQString(m_contextData->attachment));
        m_data.insert(CONTEXTCOLUMN_ATTACHMENTCONTEXT, QtUtil::ToQString(m_contextData->attachmentContext));
        m_data.insert(CONTEXTCOLUMN_ATTACHMENTHELPER, QtUtil::ToQString(m_contextData->attachmentHelper));
        m_data.insert(CONTEXTCOLUMN_STARTPOSITION, VecToString(m_contextData->startLocation.t));
        m_data.insert(CONTEXTCOLUMN_STARTROTATION, AngToString(m_contextData->startRotationEuler));

        m_data.insert(CONTEXTCOLUMN_SLAVE_CONTROLLER_DEF, QtUtil::ToQString(m_contextData->pControllerDef ? m_contextData->pControllerDef->m_filename.c_str() : ""));
        m_data.insert(CONTEXTCOLUMN_SLAVE_DATABASE, QtUtil::ToQString(m_contextData->pSlaveDatabase ? m_contextData->pSlaveDatabase->GetFilename() : ""));
    }

    CXTPMannContextRecord()
        : m_contextData(nullptr)
    {}


    SScopeContextData* ContextData() const { return m_contextData; }

    QVariant DataForColumn(int column) const
    {
        return m_data.value(column);
    }

private:
    SScopeContextData* m_contextData;
    QHash<int, QVariant> m_data;
};
Q_DECLARE_METATYPE(CXTPMannContextRecord*);



class CMannContextTableModel
    : public AbstractSortModel
{
public:
    CMannContextTableModel(QObject* parent = nullptr)
        : AbstractSortModel(parent)
    {
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_contextRecords.size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 15;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid())
        {
            return QVariant();
        }
        assert(index.row() < m_contextRecords.size());
        const CXTPMannContextRecord& record = m_contextRecords[index.row()];
        return data(record, index.column(), role);
    }

    QVariant data(const CXTPMannContextRecord& record, int column, int role = Qt::DisplayRole) const
    {
        if (role == Qt::DisplayRole)
        {
            return record.DataForColumn(column);
        }
        else if (role == Qt::UserRole)
        {
            return QVariant::fromValue(const_cast<CXTPMannContextRecord*>(&record));
        }
        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation != Qt::Horizontal)
        {
            return QVariant();
        }
        else if (role == Qt::DisplayRole)
        {
            switch (section)
            {
            case CONTEXTCOLUMN_INDEX:
                return tr("Index");
            case CONTEXTCOLUMN_NAME:
                return tr("Name");
            case CONTEXTCOLUMN_ENABLED:
                return tr("Start On");
            case CONTEXTCOLUMN_DATABASE:
                return tr("Database");
            case CONTEXTCOLUMN_CONTEXTID:
                return tr("Context");
            case CONTEXTCOLUMN_TAGS:
                return tr("Tags");
            case CONTEXTCOLUMN_FRAGTAGS:
                return tr("FragTags");
            case CONTEXTCOLUMN_MODEL:
                return tr("Model");
            case CONTEXTCOLUMN_ATTACHMENT:
                return tr("Attachment");
            case CONTEXTCOLUMN_ATTACHMENTCONTEXT:
                return tr("Att. Context");
            case CONTEXTCOLUMN_ATTACHMENTHELPER:
                return tr("Att. Helper");
            case CONTEXTCOLUMN_STARTPOSITION:
                return tr("Start Position");
            case CONTEXTCOLUMN_STARTROTATION:
                return tr("Start Rotation");
            case CONTEXTCOLUMN_SLAVE_CONTROLLER_DEF:
                return tr("Slave Controller Def");
            case CONTEXTCOLUMN_SLAVE_DATABASE:
                return tr("Slave Database");
            default:
                return QString();
            }
        }
        return QVariant();
    }

    void sort(int column, Qt::SortOrder order) override
    {
        layoutAboutToBeChanged();
        std::sort(m_contextRecords.begin(), m_contextRecords.end(), [&](const CXTPMannContextRecord& lhs, const CXTPMannContextRecord& rhs)
            {
                return lessThan(lhs, rhs, column);
            });
        if (order == Qt::DescendingOrder)
        {
            std::reverse(m_contextRecords.begin(), m_contextRecords.end());
        }
        layoutChanged();
    }

    bool lessThan(const CXTPMannContextRecord& lhs, const CXTPMannContextRecord& rhs, int column) const
    {
        const QVariant l = data(lhs, column);
        const QVariant r = data(rhs, column);
        bool ok, ok2;
        const int lInt = l.toInt(&ok);
        const int rInt = r.toInt(&ok2);
        if (ok && ok2)
        {
            return lInt < rInt;
        }
        return l.toString() < r.toString();
    }

    void setContextRecords(const std::vector<CXTPMannContextRecord>& records)
    {
        beginResetModel();
        m_contextRecords = records;
        endResetModel();
    }

private:
    std::vector<CXTPMannContextRecord> m_contextRecords;
};



//////////////////////////////////////////////////////////////////////////
CMannContextEditorDialog::CMannContextEditorDialog(QWidget* pParent)
    : QDialog(pParent)
    , ui(new Ui::MannContextEditorDialog)
{
    ui->setupUi(this);

    m_contextModel = new CMannContextTableModel(this);
    ui->m_wndReport->setModel(m_contextModel);
    ui->m_wndReport->AddGroup(CONTEXTCOLUMN_CONTEXTID);
    ui->m_wndReport->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->m_wndReport->setSelectionBehavior(QAbstractItemView::SelectRows);

    // flush changes so nothing is lost.
    CMannequinDialog::GetCurrentInstance()->FragmentEditor()->TrackPanel()->FlushChanges();

    connect(ui->m_wndReport, &QTreeView::activated, this, &CMannContextEditorDialog::OnReportItemDblClick);
    connect(ui->m_wndReport->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMannContextEditorDialog::OnReportSelChanged);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CMannContextEditorDialog::accept);
    connect(ui->MANN_NEW_CONTEXT, &QAbstractButton::clicked, this, &CMannContextEditorDialog::OnNewContext);
    connect(ui->MANN_EDIT_CONTEXT, &QAbstractButton::clicked, this, &CMannContextEditorDialog::OnEditContext);
    connect(ui->MANN_CLONE_CONTEXT, &QAbstractButton::clicked, this, &CMannContextEditorDialog::OnCloneAndEditContext);
    connect(ui->MANN_DELETE_CONTEXT, &QAbstractButton::clicked, this, &CMannContextEditorDialog::OnDeleteContext);
    connect(ui->MANN_MOVE_UP, &QAbstractButton::clicked, this, &CMannContextEditorDialog::OnMoveUp);
    connect(ui->MANN_MOVE_DOWN, &QAbstractButton::clicked, this, &CMannContextEditorDialog::OnMoveDown);
    connect(ui->MANN_IMPORT_BACKGROUND, &QAbstractButton::clicked, this, &CMannContextEditorDialog::OnImportBackground);

    OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
CMannContextEditorDialog::~CMannContextEditorDialog()
{
    CMannequinDialog::GetCurrentInstance()->LoadNewPreviewFile(NULL);
    CMannequinDialog::GetCurrentInstance()->StopEditingFragment();
}

//////////////////////////////////////////////////////////////////////////
void CMannContextEditorDialog::OnInitDialog()
{
    PopulateReport();

    EnableControls();

    int colCount = ui->m_wndReport->header()->count();
    for (int col = 0; col < colCount; col++)
    {
        ui->m_wndReport->header()->resizeSection(col, ui->m_wndReport->header()->sectionSizeHint(col) + 30);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannContextEditorDialog::PopulateReport()
{
    CXTPMannContextRecord* selectedRecord = GetFocusedRecord();
    const uint selectedID = selectedRecord ? selectedRecord->DataForColumn(CONTEXTCOLUMN_INDEX).toUInt() : 0;


    std::vector<CXTPMannContextRecord> items;
    SMannequinContexts* pContexts = CMannequinDialog::GetCurrentInstance()->Contexts();
    for (size_t i = 0; i < pContexts->m_contextData.size(); i++)
    {
        items.push_back(CXTPMannContextRecord(&pContexts->m_contextData[i]));
    }
    m_contextModel->setContextRecords(items);

    SetFocusedRecord(selectedID);
}

//////////////////////////////////////////////////////////////////////////
void CMannContextEditorDialog::EnableControls()
{
    const QModelIndex focusedIndex = GetFocusedRecordIndex();
    CXTPMannContextRecord* pRow = GetFocusedRecord();
    const bool isValidRowSelected = pRow;

    const int index = isValidRowSelected ? focusedIndex.row() : 0;

    ui->MANN_NEW_CONTEXT->setEnabled(true);
    ui->MANN_EDIT_CONTEXT->setEnabled(isValidRowSelected);
    ui->MANN_CLONE_CONTEXT->setEnabled(isValidRowSelected);
    ui->MANN_DELETE_CONTEXT->setEnabled(isValidRowSelected);
    ui->MANN_MOVE_UP->setEnabled(isValidRowSelected && index > 0);
    ui->MANN_MOVE_DOWN->setEnabled(isValidRowSelected && index < focusedIndex.model()->rowCount(focusedIndex.parent()) - 1);
}

//////////////////////////////////////////////////////////////////////////
CXTPMannContextRecord* CMannContextEditorDialog::GetFocusedRecord()
{
    return GetFocusedRecordIndex().data(Qt::UserRole).value<CXTPMannContextRecord*>();
}


QModelIndex CMannContextEditorDialog::GetFocusedRecordIndex() const
{
    const QModelIndexList& list = ui->m_wndReport->selectionModel()->selectedRows();
    const QModelIndex index = list.isEmpty() ? QModelIndex() : list.first();
    return index.parent().isValid() ? index : QModelIndex();
}


//////////////////////////////////////////////////////////////////////////
void CMannContextEditorDialog::OnReportSelChanged()
{
    EnableControls();
}

//////////////////////////////////////////////////////////////////////////
void CMannContextEditorDialog::OnReportItemDblClick(const QModelIndex& index)
{
    //ignore if group column
    if (index.parent().isValid())
    {
        OnEditContext();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannContextEditorDialog::OnNewContext()
{
    CMannNewContextDialog dialog(nullptr, CMannNewContextDialog::eContextDialog_New, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        PopulateReport();
    }
}


//////////////////////////////////////////////////////////////////////////
void CMannContextEditorDialog::OnEditContext()
{
    CXTPMannContextRecord* pRecord = GetFocusedRecord();
    if (pRecord)
    {
        CMannNewContextDialog dialog(pRecord->ContextData(), CMannNewContextDialog::eContextDialog_Edit, this);
        if (dialog.exec() == QDialog::Accepted)
        {
            PopulateReport();
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CMannContextEditorDialog::OnCloneAndEditContext()
{
    CXTPMannContextRecord* pRecord = GetFocusedRecord();
    if (pRecord)
    {
        CMannNewContextDialog dialog(pRecord->ContextData(), CMannNewContextDialog::eContextDialog_CloneAndEdit, this);
        if (dialog.exec() == QDialog::Accepted)
        {
            PopulateReport();
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CMannContextEditorDialog::OnDeleteContext()
{
    if (CXTPMannContextRecord* pRecord = GetFocusedRecord())
    {
        QString message = tr("Are you sure you want to delete \"%1\"?\nYou will not be able to undo this change.").arg(pRecord->ContextData()->name);

        if (QMessageBox::question(this, tr("Delete Context"), message) == QMessageBox::Yes)
        {
            SMannequinContexts* pContexts = CMannequinDialog::GetCurrentInstance()->Contexts();
            std::vector<SScopeContextData>::const_iterator iter = pContexts->m_contextData.begin();
            while (iter != pContexts->m_contextData.end())
            {
                if ((*iter).dataID == pRecord->ContextData()->dataID)
                {
                    pContexts->m_contextData.erase(iter);
                    break;
                }
                ++iter;
            }
            PopulateReport();
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CMannContextEditorDialog::OnMoveUp()
{
    const QModelIndex focusedIndex = GetFocusedRecordIndex();
    if (focusedIndex.isValid())
    {
        CXTPMannContextRecord* pRec = focusedIndex.data(Qt::UserRole).value<CXTPMannContextRecord*>();
        const QModelIndex indexAbove = ui->m_wndReport->indexAbove(focusedIndex);
        CXTPMannContextRecord* pAbove = indexAbove.data(Qt::UserRole).value<CXTPMannContextRecord*>();
        Q_ASSERT(pAbove);

        SMannequinContexts& contexts = (*CMannequinDialog::GetCurrentInstance()->Contexts());
        std::vector<SScopeContextData>::iterator recIt = std::find(contexts.m_contextData.begin(), contexts.m_contextData.end(), *pRec->ContextData());
        std::vector<SScopeContextData>::iterator abvIt = std::find(contexts.m_contextData.begin(), contexts.m_contextData.end(), *pAbove->ContextData());
        if (recIt != contexts.m_contextData.end() && abvIt != contexts.m_contextData.end())
        {
            std::swap(*recIt, *abvIt);
            std::swap((*recIt).dataID, (*abvIt).dataID);
        }

        const uint32 tempDataID = (*abvIt).dataID;
        PopulateReport();
        SetFocusedRecord((*abvIt).dataID);
        ui->m_wndReport->setFocus();

        EnableControls();
    }
}


//////////////////////////////////////////////////////////////////////////
void CMannContextEditorDialog::OnMoveDown()
{
    const QModelIndex focusedIndex = GetFocusedRecordIndex();
    if (focusedIndex.isValid())
    {
        CXTPMannContextRecord* pRec = focusedIndex.data(Qt::UserRole).value<CXTPMannContextRecord*>();
        const QModelIndex indexBelow = ui->m_wndReport->indexBelow(focusedIndex);
        CXTPMannContextRecord* pBelow = indexBelow.data(Qt::UserRole).value<CXTPMannContextRecord*>();
        Q_ASSERT(pBelow);

        SMannequinContexts& contexts = (*CMannequinDialog::GetCurrentInstance()->Contexts());
        std::vector<SScopeContextData>::iterator recIt = std::find(contexts.m_contextData.begin(), contexts.m_contextData.end(), *pRec->ContextData());
        std::vector<SScopeContextData>::iterator blwIt = std::find(contexts.m_contextData.begin(), contexts.m_contextData.end(), *pBelow->ContextData());
        if (recIt != contexts.m_contextData.end() && blwIt != contexts.m_contextData.end())
        {
            std::swap(*recIt, *blwIt);
            std::swap((*recIt).dataID, (*blwIt).dataID);
        }

        PopulateReport();
        SetFocusedRecord((*blwIt).dataID);
        ui->m_wndReport->setFocus();

        EnableControls();
    }
}


//////////////////////////////////////////////////////////////////////////
void CMannContextEditorDialog::SetFocusedRecord(const uint32 dataID)
{
    QAbstractItemModel* model = ui->m_wndReport->model();
    for (int groupRow = 0; groupRow < model->rowCount(); ++groupRow)
    {
        const QModelIndex parent = model->index(groupRow, 0);
        for (int row = 0; row < model->rowCount(parent); ++row)
        {
            const QModelIndex index = model->index(row, CONTEXTCOLUMN_INDEX, parent);
            if (model->data(index).toUInt() == dataID)
            {
                ui->m_wndReport->setCurrentIndex(index);
                return;
            }
        }
    }
}

const char* OBJECT_WHITELIST[] =
{
    "Brush",
    "Entity",
    "PrefabInstance",
    "Solid",
    "TagPoint"
};
const uint32 NUM_WHITELISTED_OBJECTS = ARRAYSIZE(OBJECT_WHITELIST);

bool IsObjectWhiteListed(const char* objectType)
{
    for (uint32 i = 0; i < NUM_WHITELISTED_OBJECTS; i++)
    {
        if (strcmp(OBJECT_WHITELIST[i], objectType) == 0)
        {
            return true;
        }
    }

    return false;
}

const char* ENTITY_WHITELIST[] =
{
    "RigidBodyEx",
    "Light",
    "Switch"
};
const uint32 NUM_WHITELISTED_ENTITIES = ARRAYSIZE(ENTITY_WHITELIST);

bool IsEntityWhiteListed(const char* entityClass)
{
    for (uint32 i = 0; i < NUM_WHITELISTED_ENTITIES; i++)
    {
        if (strcmp(ENTITY_WHITELIST[i], entityClass) == 0)
        {
            return true;
        }
    }

    return false;
}

//--- Remove any objects that aren't relevant/won't work in the Mannequin editor
void StripBackgroundXML(XmlNodeRef xmlRoot)
{
    if (xmlRoot)
    {
        uint32 numChildren = xmlRoot->getChildCount();
        for (int32 i = numChildren - 1; i >= 0; i--)
        {
            XmlNodeRef xmlObject = xmlRoot->getChild(i);
            const char* type = xmlObject->getAttr("Type");
            if (!IsObjectWhiteListed(type))
            {
                xmlRoot->removeChild(xmlObject);
            }
            else if (strcmp(type, "Entity") == 0)
            {
                const char* entClass = xmlObject->getAttr("EntityClass");
                if (!IsEntityWhiteListed(entClass))
                {
                    xmlRoot->removeChild(xmlObject);
                }
            }
        }
    }
}

void CMannContextEditorDialog::OnImportBackground()
{
    AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection("Group");
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (!selection.IsValid())
    {
        return;
    }

    XmlNodeRef xmlData = GetISystem()->LoadXmlFromFile(selection.GetResult()->GetFullPath().c_str());
    if (xmlData)
    {
        StripBackgroundXML(xmlData);

        const uint32 numObjects = xmlData->getChildCount();
        QStringList objectNames;
        objectNames << tr("None");
        for (uint32 i = 0; i < numObjects; i++)
        {
            XmlNodeRef child = xmlData->getChild(i);
            objectNames.append(child->getAttr("Name"));
        }

        bool ok = false;
        const QString result = QInputDialog::getItem(this, tr("Import Background"), tr("Root Object"), objectNames, 0, false, &ok);

        if (ok)
        {
            int curRoot = objectNames.indexOf(result) - 1;
            QuatT rootTran(IDENTITY);

            if (curRoot >= 0)
            {
                XmlNodeRef child = xmlData->getChild(curRoot);
                child->getAttr("Pos", rootTran.t);
                child->getAttr("Rotate", rootTran.q);
            }

            CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();
            SMannequinContexts* pContexts = pMannequinDialog->Contexts();
            pContexts->backGroundObjects = xmlData;
            pContexts->backgroundRootTran = rootTran;
        }
    }
}
