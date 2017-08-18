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

#include "stdafx.h"
#include "SelectionTreeErrorDialog.h"

#include "Util/AbstractSortModel.h"
#include "Util/ColumnGroupTreeView.h"
#include "Util/CryMemFile.h"					// CCryMemFile
#include "QtViewPaneManager.h"

#include <QtUtil.h>

#include <QAbstractTableModel>
#include <QBoxLayout>
#include <QHeaderView>
#include <QTreeView>

#include <QtWinExtras/QtWin>

#define BST_ERROR_DIAG_CLASSNAME "SelectionTree Error Report"

// CErrorReportDialog dialog
#define BITMAP_ERROR 0
#define BITMAP_WARNING 1
#define BITMAP_COMMENT 2

#define ID_REPORT_CONTROL 100

class SelectionTreeErrorModel : public AbstractSortModel
{
public:
	SelectionTreeErrorModel(QObject* parent)
        : AbstractSortModel(parent)
	{
		CImageList list;
		list.Create(IDB_ERROR_REPORT, 16, 1, RGB(255, 255, 255));
		IMAGEINFO image;
		list.GetImageInfo(0, &image);
		m_imageList = QtWin::fromHBITMAP(image.hbmImage, QtWin::HBitmapAlpha);
	}

	enum Column
	{
		ColumnSeverity,
		ColumnError,
		ColumnType,
		ColumnTree,
		ColumnFile,
		ColumnDescription
	};

	void setReport(CSelectionTreeErrorReport *report)
	{
		beginResetModel();
		// Store localarray of error records.
		m_errorRecords.clear();
		if (report != 0)
		{
			m_errorRecords.reserve(report->GetErrorCount());
			for (int i = 0; i < report->GetErrorCount(); i++)
			{
				const CSelectionTreeErrorRecord &err = report->GetError(i);
				m_errorRecords.push_back(err);
			}
		}
		endResetModel();
	}

	int rowCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return parent.isValid() ? 0 : m_errorRecords.size();
	}

	int columnCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return parent.isValid() ? 0 : 6;
	}

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
	{
		if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		{
			switch (section)
			{
			case ColumnSeverity:
				return tr("");
			case ColumnError:
				return tr("Error");
			case ColumnType:
				return tr("Type");
			case ColumnTree:
				return tr("Tree/Reference");
			case ColumnFile:
				return tr("File");
			case ColumnDescription:
				return tr("Description");
			default:
				Q_UNREACHABLE(break);
			}
		}
		return QAbstractTableModel::headerData(section, orientation, role);
	}

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
	{
		const CSelectionTreeErrorRecord& record = m_errorRecords[index.row()];
		return data(record, index.column(), role);
	}

	QVariant data(const CSelectionTreeErrorRecord& record, int column, int role = Qt::DisplayRole) const
	{
        switch (role)
        {
        case Qt::DisplayRole:
        {
            switch (column)
            {
            case ColumnError:
                return QtUtil::ToQString(record.error);
            case ColumnType:
                switch (record.module)
                {
                case CSelectionTreeErrorRecord::EMODULE_TREE:
                    return tr("SelectionTree");
                case CSelectionTreeErrorRecord::EMODULE_SIGNALS:
                    return tr("Signals");
                case CSelectionTreeErrorRecord::EMODULE_VARIABLES:
                    return tr("Variables");
                case CSelectionTreeErrorRecord::EMODULE_LEAFTRANSLATIONS:
                    return tr("LeafMapping");
                default:
                    return tr("Unknown");
                }
            case ColumnTree:
                return QtUtil::ToQString(record.group) + (record.type == CSelectionTreeErrorRecord::ETYPE_REFERENCE ? tr(" (Ref)") : QString());
            case ColumnFile:
                return QtUtil::ToQString(record.file);
            case ColumnDescription:
                return QtUtil::ToQString(record.description);
            }
        }
        case Qt::DecorationRole:
        {
            if (column == ColumnSeverity)
            {
                return m_imageList.copy(0, 16 * (record.severity - 1), 16, 16);
            }
        }
        case Qt::ForegroundRole:
        {
            if (record.severity == CSelectionTreeErrorRecord::ESEVERITY_ERROR)
                return QColor(Qt::red);
        }
        case Qt::UserRole:
            return record.severity;
        }
		return QVariant();
	}

    bool LessThan(const QModelIndex &lhs, const QModelIndex &rhs) const override
	{
        int column = lhs.column();
		if (column == ColumnSeverity)
            return lhs.data(Qt::UserRole).toInt() < rhs.data(Qt::UserRole).toInt();
        return lhs.data(Qt::DisplayRole).toString() < rhs.data(Qt::DisplayRole).toString();
	}

private:
	QPixmap m_imageList;
	std::vector<CSelectionTreeErrorRecord> m_errorRecords;
};


//////////////////////////////////////////////////////////////////////////
// CSelectionTreeErrorDialog
//////////////////////////////////////////////////////////////////////////
CSelectionTreeErrorDialog* CSelectionTreeErrorDialog::m_instance = 0;

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::RegisterViewClass()
{
	RegisterQtViewPane<CSelectionTreeErrorDialog>(GetIEditor(), BST_ERROR_DIAG_CLASSNAME, "Editor");
}


CSelectionTreeErrorDialog::CSelectionTreeErrorDialog()
	: QWinWidget()
	, m_wndReport(new ColumnGroupTreeView(this))
	, m_model(new SelectionTreeErrorModel(this))
{
	m_instance = this;
	m_pErrorReport = 0;

	setLayout(new QHBoxLayout);
	layout()->addWidget(m_wndReport);
	layout()->setContentsMargins(0, 0, 0, 0);
	m_wndReport->setModel(m_model);
	m_wndReport->AddGroup(SelectionTreeErrorModel::ColumnTree);

	m_wndReport->header()->resizeSection(SelectionTreeErrorModel::ColumnSeverity, QHeaderView::ResizeToContents);
	m_wndReport->header()->resizeSection(SelectionTreeErrorModel::ColumnError, 200);
	m_wndReport->header()->resizeSection(SelectionTreeErrorModel::ColumnType, 150);
	m_wndReport->header()->resizeSection(SelectionTreeErrorModel::ColumnTree, 100);
	m_wndReport->header()->resizeSection(SelectionTreeErrorModel::ColumnFile, 150);

	m_wndReport->header()->setStretchLastSection(true);
}

CSelectionTreeErrorDialog::~CSelectionTreeErrorDialog()
{
	m_instance = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::Open( CSelectionTreeErrorReport *pReport )
{
	bool reload = false;
	if(!m_instance)
	{
		GetIEditor()->OpenView( BST_ERROR_DIAG_CLASSNAME );
		reload = true;
	}

	if(!m_instance)
		return;

	reload |= m_instance->m_pErrorReport != pReport;
	if (reload)
	{
		m_instance->SetReport( pReport );
		m_instance->UpdateErrors();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::Close()
{
	if (m_instance)
	{
		m_instance->close();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::Clear()
{
	if (m_instance)
	{
		m_instance->SetReport( 0 );
		m_instance->UpdateErrors();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::Reload()
{
	if(!m_instance)
		return;

	m_instance->UpdateErrors();
}


void CSelectionTreeErrorDialog::UpdateErrors()
{
	ReloadErrors();
	if (m_pErrorReport)
		m_pErrorReport->SetReloaded();
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::ReloadErrors()
{
	m_model->setReport(m_pErrorReport);
}

#include <BSTEditor/Dialogs/SelectionTreeErrorDialog.moc>
