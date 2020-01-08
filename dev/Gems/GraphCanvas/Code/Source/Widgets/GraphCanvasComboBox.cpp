/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "precompiled.h"

#include <QEvent>
#include <QFocusEvent>
#include <QHeaderView>
#include <QScopedValueRollback>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#include <Widgets/GraphCanvasComboBox.h>

namespace GraphCanvas
{   
    ////////////////////////////////////////
    // GraphCanvasComboBoxFilterProxyModel
    ////////////////////////////////////////    

    GraphCanvasComboBoxFilterProxyModel::GraphCanvasComboBoxFilterProxyModel(QObject* parent)
    {
    }

    bool GraphCanvasComboBoxFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        if (m_filter.isEmpty())
        {
            return true;
        }

        QAbstractItemModel* model = sourceModel();

        if (!model)
        {
            return false;
        }

        QModelIndex index = model->index(sourceRow, filterKeyColumn(), sourceParent);
        QString test = model->data(index, filterRole()).toString();

        return (test.lastIndexOf(m_testRegex) >= 0);
    }

    void GraphCanvasComboBoxFilterProxyModel::SetFilter(const QString& filter)
    {
        m_filter = filter;
        m_testRegex = QRegExp(m_filter, Qt::CaseInsensitive);
        invalidateFilter();

        if (m_filter.isEmpty())
        {
            sort(-1);
        }
        else
        {
            sort(filterKeyColumn());
        }
    }

    ////////////////////////////
    // GraphCanvasComboBoxMenu
    ////////////////////////////

    GraphCanvasComboBoxMenu::GraphCanvasComboBoxMenu(ComboBoxModelInterface* model, QWidget* parent)
        : QDialog(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
        , m_disableHiding(false)
        , m_ignoreNextFocusIn(false)
        , m_modelInterface(model)
    {
        setProperty("HasNoWindowDecorations", true);

        setAttribute(Qt::WA_ShowWithoutActivating);

        m_filterProxyModel.setSourceModel(m_modelInterface->GetDropDownItemModel());
        m_filterProxyModel.sort(m_modelInterface->GetSortColumn());
        m_filterProxyModel.setFilterKeyColumn(m_modelInterface->GetCompleterColumn());

        m_tableView.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_tableView.setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView.setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);        

        m_tableView.setModel(GetProxyModel());
        m_tableView.verticalHeader()->hide();
        m_tableView.verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);

        m_tableView.horizontalHeader()->hide();
        m_tableView.horizontalHeader()->setStretchLastSection(true);

        m_tableView.installEventFilter(this);
        m_tableView.setFocusPolicy(Qt::FocusPolicy::ClickFocus);

        m_tableView.setMinimumWidth(0);        

        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget(&m_tableView);
        setLayout(layout);

        m_disableHidingStateSetter.AddStateController(GetDisableHidingStateController());

        QObject::connect(&m_tableView, &QTableView::clicked, this, &GraphCanvasComboBoxMenu::OnTableClicked);

        m_closeTimer.setInterval(0);

        QAction* escapeAction = new QAction(this);
        escapeAction->setShortcut(Qt::Key_Escape);

        addAction(escapeAction);

        QObject::connect(escapeAction, &QAction::triggered, this, &GraphCanvasComboBoxMenu::CancelMenu);
    }

    GraphCanvasComboBoxMenu::~GraphCanvasComboBoxMenu()
    {
    }

    ComboBoxModelInterface* GraphCanvasComboBoxMenu::GetInterface()
    {
        return m_modelInterface;
    }

    const ComboBoxModelInterface* GraphCanvasComboBoxMenu::GetInterface() const
    {
        return m_modelInterface;
    }

    GraphCanvasComboBoxFilterProxyModel* GraphCanvasComboBoxMenu::GetProxyModel()
    {
        return &m_filterProxyModel;
    }

    const GraphCanvasComboBoxFilterProxyModel* GraphCanvasComboBoxMenu::GetProxyModel() const
    {
        return &m_filterProxyModel;
    }

    void GraphCanvasComboBoxMenu::ShowMenu()
    {
        clearFocus();
        m_tableView.clearFocus();

        show();

        m_disableHidingStateSetter.ReleaseState();

        int rowHeight = m_tableView.rowHeight(0);

        if (rowHeight > 0)
        {
            // Generic padding of like 20 pixels.
            setMinimumHeight(rowHeight * 4.5 + 20);
            setMaximumHeight(rowHeight * 4.5 + 20);
        }
    }

    void GraphCanvasComboBoxMenu::HideMenu()
    {
        m_disableHidingStateSetter.ReleaseState();

        m_tableView.clearFocus();
        clearFocus();
        reject();
    }

    void GraphCanvasComboBoxMenu::reject()
    {
        if (!m_disableHiding.GetState())
        {
            QDialog::reject();
        }
    }

    bool GraphCanvasComboBoxMenu::eventFilter(QObject* object, QEvent* event)
    {
        if (object == &m_tableView)
        {
            if (event->type() == QEvent::FocusOut)
            {
                HandleFocusOut();
            }
            else if (event->type() == QEvent::FocusIn)
            {
                HandleFocusIn();
            }
        }

        return false;
    }

    void GraphCanvasComboBoxMenu::focusInEvent(QFocusEvent* focusEvent)
    {
        QDialog::focusInEvent(focusEvent);

        if (focusEvent->isAccepted())
        {
            if (!m_ignoreNextFocusIn)
            {
                HandleFocusIn();
            }
            else
            {
                m_ignoreNextFocusIn = false;
            }
        }
    }

    void GraphCanvasComboBoxMenu::focusOutEvent(QFocusEvent* focusEvent)
    {
        QDialog::focusOutEvent(focusEvent);
        HandleFocusOut();
    }

    void GraphCanvasComboBoxMenu::showEvent(QShowEvent* showEvent)
    {
        QDialog::showEvent(showEvent);

        // So, despite me telling it to not activate, the window still gets a focus in event.
        // But, it doesn't get a focus out event, since it doesn't actually accept the focus in event?
        m_ignoreNextFocusIn = true;
        m_tableView.selectionModel()->clearSelection();        

        Q_EMIT VisibilityChanged(true);
    }

    void GraphCanvasComboBoxMenu::hideEvent(QHideEvent* hideEvent)
    {
        QDialog::hideEvent(hideEvent);

        clearFocus();

        Q_EMIT VisibilityChanged(false);
        m_tableView.selectionModel()->clearSelection();
    }

    GraphCanvas::StateController<bool>* GraphCanvasComboBoxMenu::GetDisableHidingStateController()
    {
        return &m_disableHiding;
    }

    void GraphCanvasComboBoxMenu::SetSelectedIndex(QModelIndex index)
    {
        m_tableView.selectionModel()->clear();

        if (index.isValid()
            && index.row() >= 0
            && index.row() < m_filterProxyModel.rowCount())
        {
            QItemSelection rowSelection(m_filterProxyModel.index(index.row(), 0, index.parent()), m_filterProxyModel.index(index.row(), m_filterProxyModel.columnCount() - 1, index.parent()));
            m_tableView.selectionModel()->select(rowSelection, QItemSelectionModel::Select);

            m_tableView.scrollTo(m_filterProxyModel.index(index.row(), 0, index.parent()));
        }
    }

    QModelIndex GraphCanvasComboBoxMenu::GetSelectedIndex() const
    {
        if (m_tableView.selectionModel()->hasSelection())
        {
            QModelIndexList selectedIndexes = m_tableView.selectionModel()->selectedIndexes();

            if (!selectedIndexes.empty())
            {
                return selectedIndexes.front();
            }
        }

        return QModelIndex();
    }

    void GraphCanvasComboBoxMenu::OnTableClicked(const QModelIndex& modelIndex)
    {
        if (modelIndex.isValid())
        {
            QModelIndex sourceIndex = m_filterProxyModel.mapToSource(modelIndex);

            if (sourceIndex.isValid())
            {
                Q_EMIT OnIndexSelected(sourceIndex);

                QObject::disconnect(m_closeConnection);
                m_closeConnection = QObject::connect(&m_closeTimer, &QTimer::timeout, this, &QDialog::accept);

                m_closeTimer.start();
            }
        }
    }

    void GraphCanvasComboBoxMenu::HandleFocusIn()
    {
        m_disableHidingStateSetter.SetState(true);

        emit OnFocusIn();
    }

    void GraphCanvasComboBoxMenu::HandleFocusOut()
    {
        m_disableHidingStateSetter.ReleaseState();

        QObject::disconnect(m_closeConnection);
        m_closeConnection = QObject::connect(&m_closeTimer, &QTimer::timeout, this, &QDialog::reject);

        m_closeTimer.start();

        emit OnFocusOut();
    }

    ////////////////////////
    // GraphCanvasComboBox
    ////////////////////////
    
    GraphCanvasComboBox::GraphCanvasComboBox(ComboBoxModelInterface* modelInterface, QWidget* parent)
        : QLineEdit(parent)
        , m_lineEditInFocus(false)
        , m_popUpMenuInFocus(false)
        , m_hasFocus(false)
        , m_ignoreNextComplete(false)
        , m_recursionBlocker(false)
        , m_comboBoxMenu(modelInterface)
        , m_modelInterface(modelInterface)
    {
        setProperty("HasNoWindowDecorations", true);
        setProperty("DisableFocusWindowFix", true);

        QAction* action = addAction(QIcon(":/GraphCanvasResources/Resources/triangle.png"), QLineEdit::ActionPosition::TrailingPosition);
        QObject::connect(action, &QAction::triggered, this, &GraphCanvasComboBox::OnOptionsClicked);        
        
        QAction* escapeAction = new QAction(this);
        escapeAction->setShortcut(Qt::Key_Escape);

        addAction(escapeAction);

        QObject::connect(escapeAction, &QAction::triggered, this, &GraphCanvasComboBox::ResetComboBox);                
        
        m_completer.setModel(m_modelInterface->GetCompleterItemModel());
        m_completer.setCompletionColumn(m_modelInterface->GetCompleterColumn());
        m_completer.setCompletionMode(QCompleter::CompletionMode::InlineCompletion);
        m_completer.setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);       
        
        QObject::connect(this, &QLineEdit::textEdited, this, &GraphCanvasComboBox::OnTextChanged);
        QObject::connect(this, &QLineEdit::returnPressed, this, &GraphCanvasComboBox::OnReturnPressed);
        QObject::connect(this, &QLineEdit::editingFinished, this, &GraphCanvasComboBox::OnEditComplete);
        
        m_filterTimer.setInterval(500);
        QObject::connect(&m_filterTimer, &QTimer::timeout, this, &GraphCanvasComboBox::UpdateFilter);

        m_closeTimer.setInterval(0);
        QObject::connect(&m_closeTimer, &QTimer::timeout, this, &GraphCanvasComboBox::CloseMenu);

        m_focusTimer.setInterval(0);
        QObject::connect(&m_focusTimer, &QTimer::timeout, this, &GraphCanvasComboBox::HandleFocusState);

        QObject::connect(&m_comboBoxMenu, &GraphCanvasComboBoxMenu::OnIndexSelected, this, &GraphCanvasComboBox::SetSelectedIndex);
        QObject::connect(&m_comboBoxMenu, &GraphCanvasComboBoxMenu::OnFocusIn, this, &GraphCanvasComboBox::OnMenuFocusIn);
        QObject::connect(&m_comboBoxMenu, &GraphCanvasComboBoxMenu::OnFocusOut, this, &GraphCanvasComboBox::OnMenuFocusOut);
        QObject::connect(&m_comboBoxMenu, &GraphCanvasComboBoxMenu::CancelMenu, this, &GraphCanvasComboBox::ResetComboBox);

        m_comboBoxMenu.accept();

        m_disableHidingStateSetter.AddStateController(m_comboBoxMenu.GetDisableHidingStateController());
    }
    
    GraphCanvasComboBox::~GraphCanvasComboBox()
    {
    }

    void GraphCanvasComboBox::RegisterViewId(const ViewId& viewId)
    {
        m_viewId = viewId;
    }

    void GraphCanvasComboBox::SetAnchorPoint(QPoint globalPoint)
    {
        m_anchorPoint = globalPoint;
        UpdateMenuPosition();
    }

    void GraphCanvasComboBox::SetMenuWidth(qreal width)
    {
        m_displayWidth = width;
        UpdateMenuPosition();
    }
    
    void GraphCanvasComboBox::SetSelectedIndex(const QModelIndex& selectedIndex)
    {
        if (m_selectedIndex != selectedIndex)
        {
            if (DisplayIndex(selectedIndex))
            {
                Q_EMIT SelectedIndexChanged(m_selectedIndex);
            }
        }
    }

    QModelIndex GraphCanvasComboBox::GetSelectedIndex() const
    {
        return m_selectedIndex;
    }

    void GraphCanvasComboBox::ResetComboBox()
    {        
        HideMenu();
        DisplayIndex(m_selectedIndex);
    }
    
    void GraphCanvasComboBox::CancelInput()
    {
        DisplayIndex(m_selectedIndex);
        HideMenu();
    }
    
    void GraphCanvasComboBox::HideMenu()
    {
        m_disableHidingStateSetter.ReleaseState();

        m_comboBoxMenu.HideMenu();

        ViewNotificationBus::Handler::BusDisconnect(m_viewId);
    }

    bool GraphCanvasComboBox::IsMenuVisible() const
    {
        return !m_comboBoxMenu.isHidden();
    }

    void GraphCanvasComboBox::focusInEvent(QFocusEvent* focusEvent)
    {
        QLineEdit::focusInEvent(focusEvent);

        m_lineEditInFocus = true;
        m_focusTimer.start();

        grabKeyboard();
    }

    void GraphCanvasComboBox::focusOutEvent(QFocusEvent* focusEvent)
    {
        QLineEdit::focusOutEvent(focusEvent);

        m_lineEditInFocus = false;
        m_focusTimer.start();

        releaseKeyboard();
    }

    void GraphCanvasComboBox::keyPressEvent(QKeyEvent* keyEvent)
    {
        QLineEdit::keyPressEvent(keyEvent);

        if (keyEvent->key() == Qt::Key_Down)
        {
            if (m_comboBoxMenu.isHidden())
            {
                m_comboBoxMenu.GetProxyModel()->SetFilter("");
                DisplayMenu();
            }

            QModelIndex selectedIndex = m_comboBoxMenu.GetSelectedIndex();

            if (!selectedIndex.isValid())
            {
                selectedIndex = m_selectedIndex;
            }

            selectedIndex = m_modelInterface->GetNextIndex(selectedIndex);

            m_comboBoxMenu.SetSelectedIndex(selectedIndex);

            QString typeName = m_comboBoxMenu.GetInterface()->GetNameForIndex(selectedIndex);

            if (!typeName.isEmpty())
            {
                setText(typeName);
                setSelection(0, static_cast<int>(typeName.size()));

                m_completer.setCompletionPrefix(typeName);
            }

            keyEvent->accept();
        }
        else if (keyEvent->key() == Qt::Key_Up)
        {
            if (m_comboBoxMenu.isHidden())
            {
                m_comboBoxMenu.GetProxyModel()->SetFilter("");
                DisplayMenu();
            }

            QModelIndex selectedIndex = m_comboBoxMenu.GetSelectedIndex();

            if (!selectedIndex.isValid())
            {
                selectedIndex = m_selectedIndex;
            }

            selectedIndex = m_modelInterface->GetPreviousIndex(selectedIndex);

            m_comboBoxMenu.SetSelectedIndex(selectedIndex);

            QString typeName = m_comboBoxMenu.GetInterface()->GetNameForIndex(selectedIndex);

            if (!typeName.isEmpty())
            {
                setText(typeName);
                setSelection(0, static_cast<int>(typeName.size()));

                m_completer.setCompletionPrefix(typeName);
            }

            keyEvent->accept();
        }
        else if (keyEvent->key() == Qt::Key_Escape)
        {
            ResetComboBox();

            keyEvent->accept();
        }
    }

    void GraphCanvasComboBox::OnViewScrolled()
    {
        ResetComboBox();
    }

    void GraphCanvasComboBox::OnViewCenteredOnArea()
    {
        ResetComboBox();
    }

    void GraphCanvasComboBox::OnZoomChanged(qreal zoomLevel)
    {

    }
    
    void GraphCanvasComboBox::OnTextChanged()
    {
        DisplayMenu();
        UpdateFilter();
    }
    
    void GraphCanvasComboBox::OnOptionsClicked()
    {
        if (m_comboBoxMenu.isHidden())
        {
            m_comboBoxMenu.GetProxyModel()->SetFilter("");
            DisplayMenu();
        }
        else
        {
            m_comboBoxMenu.accept();
        }
    }
    
    void GraphCanvasComboBox::OnReturnPressed()
    {
        const bool allowReset = false;
        if (SubmitData(allowReset))
        {
            m_comboBoxMenu.accept();
        }
        else
        {
            setText("");
            UpdateFilter();
        }

        // When we press enter, we will also get an editing complete signal. We want to ignore that since we handled it here.
        m_ignoreNextComplete = true;
    }
    
    void GraphCanvasComboBox::OnEditComplete()
    {
        if (m_ignoreNextComplete)
        {
            m_ignoreNextComplete = false;
            return;
        }

        SubmitData();

        m_closeState = CloseMenuState::Reject;
        m_closeTimer.start();
    }
    
    void GraphCanvasComboBox::UpdateFilter()
    {
        m_comboBoxMenu.GetProxyModel()->SetFilter(GetUserInputText());
    }

    void GraphCanvasComboBox::CloseMenu()
    {
        switch (m_closeState)
        {
        case CloseMenuState::Accept:
            m_comboBoxMenu.accept();
        default:
            m_comboBoxMenu.reject();
        }

        m_closeState = CloseMenuState::Reject;
    }

    void GraphCanvasComboBox::OnMenuFocusIn()
    {
        m_popUpMenuInFocus = true;
        m_focusTimer.start();
    }

    void GraphCanvasComboBox::OnMenuFocusOut()
    {
        m_popUpMenuInFocus = false;
        m_focusTimer.start();
    }

    void GraphCanvasComboBox::HandleFocusState()
    {
        bool focusState = m_lineEditInFocus || m_popUpMenuInFocus;

        if (focusState != m_hasFocus)
        {
            m_hasFocus = focusState;

            if (m_hasFocus)
            {
                Q_EMIT OnFocusIn();
            }
            else
            {
                Q_EMIT OnFocusOut();
            }
        }
    }

    bool GraphCanvasComboBox::DisplayIndex(const QModelIndex& index)
    {
        QSignalBlocker signalBlocker(this);

        QString name = m_comboBoxMenu.GetInterface()->GetNameForIndex(index);

        if (!name.isEmpty())
        {
            m_completer.setCompletionPrefix(name);
            setText(name);
            m_selectedIndex = index;
        }
        else
        {
            QModelIndex defaultIndex = m_modelInterface->GetDefaultIndex();

            if (index == defaultIndex)
            {
                m_selectedIndex = QModelIndex();
            }
            else
            {
                DisplayIndex(defaultIndex);
            }
            
        }

        return m_selectedIndex.isValid();
    }
    
    bool GraphCanvasComboBox::SubmitData(bool allowReset)
    {
        QString inputName = text();

        QModelIndex inputIndex = m_comboBoxMenu.GetInterface()->FindIndexForName(inputName);

        // We didn't input a valid type. So default to our last previously known value.
        if (!inputIndex.isValid())
        {
            if (allowReset)
            {
                DisplayIndex(m_selectedIndex);
                inputIndex = m_selectedIndex;
            }
        }
        else
        {
            SetSelectedIndex(inputIndex);
        }

        return inputIndex.isValid();
    }
    
    void GraphCanvasComboBox::DisplayMenu()
    {
        if (!m_recursionBlocker)
        {
            QScopedValueRollback<bool> valueRollback(m_recursionBlocker, true);
            if (m_comboBoxMenu.isHidden())
            {
                Q_EMIT OnMenuAboutToDisplay();
                ViewNotificationBus::Handler::BusConnect(m_viewId);

                qreal zoomLevel = 1.0;
                ViewRequestBus::EventResult(zoomLevel, m_viewId, &ViewRequests::GetZoomLevel);

                if (zoomLevel < 1.0)
                {
                    zoomLevel = 1.0;
                }

                m_comboBoxMenu.GetInterface()->SetFontScale(zoomLevel);
                m_comboBoxMenu.ShowMenu();
                m_comboBoxMenu.SetSelectedIndex(m_selectedIndex);                
                
                UpdateMenuPosition();
            }
        }

        if (!m_disableHidingStateSetter.HasState())
        {
            m_disableHidingStateSetter.SetState(true);
        }
    }
    
    QString GraphCanvasComboBox::GetUserInputText()
    {
        QString lineEditText = text();

        // The QCompleter doesn't seem to update the completion prefix when you delete anything, only when things are added.
        // To get it to update correctly when the user deletes something, I'm using the combination of things:
        //
        // 1) If we have a completion, that text will be auto filled into the quick filter because of the completion model.
        // So, we will compare those two values, and if they match, we know we want to search using the completion prefix.
        //
        // 2) If they don't match, it means that user deleted something, and the Completer didn't update it's internal state, so we'll just
        // use whatever is in the text box.
        //
        // 3) When the text field is set to empty, the current completion gets invalidated, but the prefix doesn't, so that gets special cased out.
        //
        // Extra fun: If you type in something, "Like" then delete a middle character, "Lie", and then put the k back in. It will auto complete the E
        // visually but the completion prefix will be the entire word.
        if (completer()
            && completer()->currentCompletion().compare(lineEditText, Qt::CaseInsensitive) == 0
            && !lineEditText.isEmpty())
        {
            lineEditText = completer()->completionPrefix();
        }

        return lineEditText;
    }

    void GraphCanvasComboBox::UpdateMenuPosition()
    {
        if (!m_comboBoxMenu.isHidden())
        {
            QRect dialogGeometry = m_comboBoxMenu.geometry();

            dialogGeometry.moveTopLeft(m_anchorPoint);
            dialogGeometry.setWidth(m_displayWidth);

            m_comboBoxMenu.setGeometry(dialogGeometry);
        }
    }
}

#include <Source/Widgets/GraphCanvasComboBox.moc>