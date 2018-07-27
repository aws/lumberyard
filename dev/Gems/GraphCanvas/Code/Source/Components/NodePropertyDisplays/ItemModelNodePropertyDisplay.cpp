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
#include "precompiled.h"

#include <QGraphicsProxyWidget>
#include <QToolButton>

#include <Components/NodePropertyDisplays/ItemModelNodePropertyDisplay.h>
#include <Source/Components/NodePropertyDisplays/ui_ItemModelInspectionDialog.h>

#include <GraphCanvas/Widgets/NodePropertyBus.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    ///////////////////////////////////
    // ItemModelInspectionFilterModel
    ///////////////////////////////////

    bool ItemModelInspectionFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        QAbstractItemModel* model = sourceModel();
        QModelIndex index = model->index(sourceRow, 0, sourceParent);
        QString displayString = model->data(index, Qt::DisplayRole).toString();
        
        return displayString.contains(m_filterRegex);
    }

    void ItemModelInspectionFilterModel::SetFilter(const QString& filter)
    {
        m_filter = filter;
        m_filterRegex = QRegExp(m_filter, Qt::CaseInsensitive);

        invalidateFilter();
    }

    void ItemModelInspectionFilterModel::ClearFilter()
    {
        m_filter.clear();
        m_filterRegex = QRegExp(m_filter, Qt::CaseInsensitive);

        invalidateFilter();
    }

    //////////////////////////////
    // ItemModelInspectionDialog
    //////////////////////////////

    ItemModelInspectionDialog::ItemModelInspectionDialog(QWidget* parent, ItemModelDataInterface* itemDataInterface)
        : QDialog(parent)        
        , m_dataInterface(itemDataInterface)
        , m_ui(new Ui::ItemModelInspectionDialog)
    {
        setWindowFlags((windowFlags() | Qt::WindowCloseButtonHint) & ~(Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint));
        setAttribute(Qt::WA_DeleteOnClose);

        m_ui->setupUi(this);

        setWindowTitle(m_dataInterface->GetModelName());

        ItemModelInspectionFilterModel* inspectionModel = aznew ItemModelInspectionFilterModel();
        inspectionModel->setSourceModel(m_dataInterface->GetItemModel());

        m_ui->tableView->setModel(inspectionModel);
        m_ui->tableView->horizontalHeader()->setStretchLastSection(true);

        m_delayTimer.setInterval(250);
        m_delayTimer.setSingleShot(true);

        QObject::connect(m_ui->filter, &QLineEdit::textChanged, this, &ItemModelInspectionDialog::OnFilterChanged);
        QObject::connect(m_ui->filter, &QLineEdit::returnPressed, this, &ItemModelInspectionDialog::ForceUpdateFilter);

        QObject::connect(m_ui->tableView, &QTableView::doubleClicked, this, &ItemModelInspectionDialog::OnSubmitSelection);        

        QObject::connect(&m_delayTimer, &QTimer::timeout, this, &ItemModelInspectionDialog::UpdateFilter);
    }

    ItemModelInspectionDialog::~ItemModelInspectionDialog()
    {
        m_ui->tableView->setModel(nullptr);

        delete m_ui;
    }

    void ItemModelInspectionDialog::OnSubmitSelection(const QModelIndex& index)
    {
        QModelIndex sourceIndex = static_cast<QSortFilterProxyModel*>(m_ui->tableView->model())->mapToSource(index);
        Q_EMIT OnIndexSelected(sourceIndex);
        close();
    }

    void ItemModelInspectionDialog::OnFilterChanged(const QString& filter)
    {
        m_delayTimer.stop();
        m_delayTimer.start();
    }

    void ItemModelInspectionDialog::ForceUpdateFilter()
    {
        m_delayTimer.stop();
        UpdateFilter();
    }

    void ItemModelInspectionDialog::UpdateFilter()
    {
        ItemModelInspectionFilterModel* filterModel = static_cast<ItemModelInspectionFilterModel*>(m_ui->tableView->model());

        filterModel->SetFilter(m_ui->filter->text());
    }

    /////////////////////////////
    // ItemModelSelectionWidget
    /////////////////////////////

    ItemModelSelectionWidget::ItemModelSelectionWidget(ItemModelDataInterface* dataInterface)
        : QWidget(nullptr)
        , m_lineEdit(aznew Internal::FocusableLineEdit())
        , m_dataInterface(dataInterface)
    {
        m_completer = new QCompleter(dataInterface->GetItemModel(), this);
        m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        m_completer->setCompletionMode(QCompleter::InlineCompletion);

        m_lineEdit->setCompleter(m_completer);
        m_lineEdit->setPlaceholderText(dataInterface->GetPlaceholderString().c_str());

        m_inspectModelButton = new QToolButton();
        m_inspectModelButton->setText("...");        

        m_layout = new QHBoxLayout(this);
        m_layout->addWidget(m_lineEdit);
        m_layout->addWidget(m_inspectModelButton);
        
        m_layout->addWidget(m_inspectModelButton);

        setContentsMargins(0, 0, 0, 0);
        m_layout->setContentsMargins(0, 0, 0, 0);

        setLayout(m_layout);

        QObject::connect(m_lineEdit, &Internal::FocusableLineEdit::OnFocusIn, this, &ItemModelSelectionWidget::HandleFocusIn);
        QObject::connect(m_lineEdit, &Internal::FocusableLineEdit::OnFocusOut, this, &ItemModelSelectionWidget::HandleFocusOut);
        QObject::connect(m_lineEdit, &Internal::FocusableLineEdit::returnPressed, this, &ItemModelSelectionWidget::SubmitSelection);

        QObject::connect(m_inspectModelButton, &QToolButton::clicked, this, &ItemModelSelectionWidget::InspectModel);        
    }

    ItemModelSelectionWidget::~ItemModelSelectionWidget()
    {
    }
    
    void ItemModelSelectionWidget::UpdateDisplay()
    {
        QSignalBlocker signalBlocker(m_lineEdit);
        m_lineEdit->setText(m_dataInterface->GetDisplayString().c_str());
    }

    void ItemModelSelectionWidget::OnEscape()
    {
        SetCurrentIndex(m_currentIndex);
        UpdateDisplay();
        m_lineEdit->selectAll();
    }

    void ItemModelSelectionWidget::HandleFocusIn()
    {        
        Q_EMIT OnFocusIn();

        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void ItemModelSelectionWidget::HandleFocusOut()
    {
        Q_EMIT OnFocusOut();
        SetCurrentIndex(m_currentIndex);
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void ItemModelSelectionWidget::SubmitSelection()
    {
        QString itemName = m_lineEdit->text();

        QModelIndex itemIndex;

        if (!itemName.isEmpty())
        {
            // Current index has some quirks. If you manually fill in the correct element
            // it will not update the current index. So I'm going to re-test the specified current
            // index to see if it's correct. If it's not, then I'll scrape the entire model to find the correct
            // index.
            QAbstractItemModel* itemModel = m_completer->model();
            itemIndex = m_completer->currentIndex();

            if (itemIndex.isValid())
            {
                QVariant elementInformation = itemModel->data(itemIndex, Qt::EditRole);

                if (elementInformation.toString().compare(itemName, Qt::CaseInsensitive) != 0)
                {
                    itemIndex = QModelIndex();
                }
            }

            if (!itemIndex.isValid())
            {
                for (int i = 0; i < itemModel->rowCount(); ++i)
                {
                    QModelIndex testIndex = itemModel->index(i, 0);

                    QVariant elementInformation = itemModel->data(testIndex, Qt::EditRole);

                    if (elementInformation.toString().compare(itemName, Qt::CaseInsensitive) == 0)
                    {
                        itemIndex = testIndex;
                        break;
                    }
                }
            }
        }

        HandleIndexSubmission(itemIndex);
    }

    void ItemModelSelectionWidget::InspectModel()
    {
        ItemModelInspectionDialog* itemInspectionDialog = aznew ItemModelInspectionDialog(nullptr, m_dataInterface);        
        QObject::connect(itemInspectionDialog, &ItemModelInspectionDialog::OnIndexSelected, this, &ItemModelSelectionWidget::OnDialogSelection);
        itemInspectionDialog->show();
    }

    void ItemModelSelectionWidget::OnDialogSelection(const QModelIndex& modelIndex)
    {
        HandleIndexSubmission(modelIndex);
    }

    void ItemModelSelectionWidget::HandleIndexSubmission(const QModelIndex& modelIndex)
    {
        if (modelIndex.isValid())
        {
            SetCurrentIndex(modelIndex);
            m_lineEdit->selectAll();
        }
        else
        {
            QSignalBlocker block(m_lineEdit);
            m_lineEdit->setText("");
        }

        Q_EMIT OnIndexSelected(modelIndex);
    }

    void ItemModelSelectionWidget::SetCurrentIndex(const QModelIndex& currentIndex)
    {
        m_currentIndex = currentIndex;
    }

    /////////////////////////////////
    // ItemModelNodePropertyDisplay
    /////////////////////////////////
	
    ItemModelNodePropertyDisplay::ItemModelNodePropertyDisplay(ItemModelDataInterface* dataInterface)
        : m_itemModelDataInterface(dataInterface)
        , m_proxyWidget(nullptr)
        , m_itemModelSelectionWidget(nullptr)
    {
        m_itemModelDataInterface->RegisterDisplay(this);

        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();
    }
    
    ItemModelNodePropertyDisplay::~ItemModelNodePropertyDisplay()
    {        
        delete m_itemModelDataInterface;
        delete m_disabledLabel;
        delete m_displayLabel;
        CleanupProxyWidget();
    }
    
    void ItemModelNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("variable").c_str());
        m_displayLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisplayLabelStyle("variable").c_str());

        if (m_itemModelSelectionWidget)
        {
            m_itemModelSelectionWidget->setMinimumSize(m_displayLabel->GetStyleHelper().GetMinimumSize().toSize());
        }
    }
    
    void ItemModelNodePropertyDisplay::UpdateDisplay()
    {
        AZStd::string displayString = m_itemModelDataInterface->GetDisplayString();
        
        if (!m_itemModelDataInterface->IsItemValid())
        {
            displayString = m_itemModelDataInterface->GetPlaceholderString();
        }
        
        m_displayLabel->SetLabel(displayString.c_str());

        if (m_itemModelSelectionWidget)
        {
            m_itemModelSelectionWidget->UpdateDisplay();
        }
    }
    
    QGraphicsLayoutItem* ItemModelNodePropertyDisplay::GetDisabledGraphicsLayoutItem()
{
        CleanupProxyWidget();
        return m_disabledLabel;
    }

    QGraphicsLayoutItem* ItemModelNodePropertyDisplay::GetDisplayGraphicsLayoutItem()
{
        CleanupProxyWidget();
        return m_displayLabel;
    }

    QGraphicsLayoutItem* ItemModelNodePropertyDisplay::GetEditableGraphicsLayoutItem()
{
        SetupProxyWidget();
        return m_proxyWidget;
    }

    void ItemModelNodePropertyDisplay::EditStart()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::LockEditState, this);
        TryAndSelectNode();
    }

    void ItemModelNodePropertyDisplay::AssignIndex(const QModelIndex& modelIndex)
    {
        m_itemModelDataInterface->AssignIndex(modelIndex);
        UpdateDisplay();
    }

    void ItemModelNodePropertyDisplay::EditFinished()
    {
        UpdateDisplay();
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);
    }

    void ItemModelNodePropertyDisplay::SetupProxyWidget()
    {
        if (!m_itemModelSelectionWidget)
        {
            m_proxyWidget = new QGraphicsProxyWidget();

            m_itemModelSelectionWidget = aznew ItemModelSelectionWidget(m_itemModelDataInterface);
            m_itemModelSelectionWidget->setProperty("HasNoWindowDecorations", true);
            m_itemModelSelectionWidget->setProperty("DisableFocusWindowFix", true);

            m_proxyWidget->setWidget(m_itemModelSelectionWidget);

            QObject::connect(m_itemModelSelectionWidget, &ItemModelSelectionWidget::OnFocusIn, [this]() { EditStart(); });
            QObject::connect(m_itemModelSelectionWidget, &ItemModelSelectionWidget::OnFocusOut, [this]() { EditFinished(); });
            QObject::connect(m_itemModelSelectionWidget, &ItemModelSelectionWidget::OnIndexSelected, [this](const QModelIndex& selectedIndex) { AssignIndex(selectedIndex); });

            RegisterShortcutDispatcher(m_itemModelSelectionWidget);
            UpdateDisplay();
            RefreshStyle();
        }
        
    }

    void ItemModelNodePropertyDisplay::CleanupProxyWidget()
    {
        if (m_itemModelSelectionWidget) 
        {
            UnregisterShortcutDispatcher(m_itemModelSelectionWidget);
            delete m_itemModelSelectionWidget; // NB: this implicitly deletes m_proxyWidget
            m_itemModelSelectionWidget = nullptr;
            m_proxyWidget = nullptr;
        }
    }

#include <Source/Components/NodePropertyDisplays/ItemModelNodePropertyDisplay.moc>
}