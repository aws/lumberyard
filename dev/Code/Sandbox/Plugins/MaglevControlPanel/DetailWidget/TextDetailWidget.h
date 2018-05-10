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

#include "DetailWidget.h"

#include <IAWSResourceManager.h>
#include <ResourceManagementView.h>

#include "StackEventsSplitter.h"

#include <QVBoxLayout>
#include <QShortcut>
#include <QAction>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QTextEdit>
#include <QResizeEvent>
#include <QPushButton>

class DetailTextEditWidget
    : public QTextEdit
{
    Q_OBJECT

public:

    void resizeEvent(QResizeEvent* thisEvent) override
    {
        QTextEdit::resizeEvent(thisEvent);
        OnResized(thisEvent->size());
    }

    bool IsModified() const
    {
        return document() && document()->isModified();
    }

    void keyPressEvent(QKeyEvent* thisEvent) override
    {
        // We don't want to process this when we're focused on another control in the same space
        // (Our search control does this)
        if (!hasFocus())
        {
            return;
        }

        QTextEdit::keyPressEvent(thisEvent);
        OnKeyPressed(thisEvent);
    }
signals:

    void OnResized(const QSize& newSize);
    void OnKeyPressed(QKeyEvent* event);
};


class TextDetailWidget
    : public DetailWidget
{
    Q_OBJECT

public:

    TextDetailWidget(ResourceManagementView* view, QSharedPointer<IStackStatusModel> stackStatusModel, QWidget* parent)
        : DetailWidget{view, parent}
    {
        QWidget* root;
        if (stackStatusModel && stackStatusModel->GetStackEventsModel())
        {
            auto splitter = new StackEventsSplitter {
                stackStatusModel->GetStackEventsModel(), this
            };
            root = new QFrame {splitter};
            splitter->SetTopWidget(root);

            auto thisLayout = new QVBoxLayout {};
            thisLayout->setMargin(0);
            thisLayout->setSpacing(0);
            thisLayout->addWidget(splitter);
            setLayout(thisLayout);
        }
        else
        {
            root = this;
        }

        auto layout = new QVBoxLayout(root);
        layout->setMargin(0);
        layout->setSpacing(0);

        m_helpLabel = new QLabel {
            this
        };
        m_helpLabel->setObjectName("TextContentHelp");
        m_helpLabel->setProperty("class", "Message");
        m_helpLabel->setHidden(true);
        layout->addWidget(m_helpLabel);

        m_textEdit = new DetailTextEditWidget;
        m_textEdit->setObjectName("TextContentEdit");
        m_textEdit->setWordWrapMode(QTextOption::WrapMode::NoWrap);
        m_textEdit->setAcceptRichText(false);
        m_textEdit->setFontFamily("Courier New");
        layout->addWidget(m_textEdit);

        auto footer = new QFrame {};
        footer->setObjectName("TextContentFooter");
        layout->addWidget(footer);

        m_footerLayout = new QHBoxLayout {};
        m_footerLayout->setSpacing(0);
        m_footerLayout->setMargin(0);
        footer->setLayout(m_footerLayout);

        m_lineLabel = new QLabel {};
        m_lineLabel->setObjectName("Line");
        m_footerLayout->addWidget(m_lineLabel);

        m_columnLabel = new QLabel {};
        m_columnLabel->setObjectName("Column");
        m_footerLayout->addWidget(m_columnLabel);

        m_footerLayout->addStretch();
    }


    void show() override
    {
        DetailWidget::show();

        connectUntilHidden(m_textEdit, &QTextEdit::copyAvailable, this, &TextDetailWidget::OnTextCopyAvailableChanged);
        connectUntilHidden(m_textEdit, &QTextEdit::undoAvailable, this, &TextDetailWidget::OnTextUndoAvailableChanged);
        connectUntilHidden(m_textEdit, &QTextEdit::redoAvailable, this, &TextDetailWidget::OnTextRedoAvailableChanged);

        connectUntilHidden(m_textEdit, &QTextEdit::cursorPositionChanged, this, &TextDetailWidget::OnTextEditCursorMoved);

        connectUntilHidden(m_view->m_findNextShortcut, &QAction::triggered, this, &TextDetailWidget::OnFindNextShortcut);
        connectUntilHidden(m_view->m_findPrevShortcut, &QAction::triggered, this, &TextDetailWidget::OnFindPreviousShortcut);
        connectUntilHidden(m_view->m_hideSearchShortcut, &QAction::triggered, this, &TextDetailWidget::OnTextEditHideSearch);
        connectUntilHidden(m_view->m_editSearchBox, &QLineEdit::returnPressed, this, &TextDetailWidget::OnFindNextShortcut);

        connectUntilHidden(m_view->m_findNext, &QPushButton::clicked, this, &TextDetailWidget::OnFindNextShortcut);
        connectUntilHidden(m_view->m_findPrevious, &QPushButton::clicked, this, &TextDetailWidget::OnFindPreviousShortcut);

        connectUntilHidden(m_view->m_clearSearchBox, &QPushButton::clicked, this, &TextDetailWidget::OnTextEditSearchClear);
        connectUntilHidden(m_view->m_hideSearchBox, &QPushButton::clicked, this, &TextDetailWidget::OnTextEditHideSearch);

        connectUntilHidden(m_view->m_editSearchBox, &QLineEdit::textChanged, this, &TextDetailWidget::OnTextEditSearchChanged);

        connectUntilHidden(m_view->m_menuSearch, &QAction::triggered, this, &TextDetailWidget::OnSearchShortcut);

        connectUntilHidden(m_view->m_menuCut, &QAction::triggered, this, &TextDetailWidget::OnMenuCut);
        connectUntilHidden(m_view->m_menuCopy, &QAction::triggered, this, &TextDetailWidget::OnMenuCopy);
        connectUntilHidden(m_view->m_menuPaste, &QAction::triggered, this, &TextDetailWidget::OnMenuPaste);

        connectUntilHidden(m_view->m_menuUndo, &QAction::triggered, this, &TextDetailWidget::OnMenuUndo);
        connectUntilHidden(m_view->m_menuRedo, &QAction::triggered, this, &TextDetailWidget::OnMenuRedo);

        OnTextEditCursorMoved();
        OnTextCopyAvailableChanged(m_copyAvailable);
        OnTextUndoAvailableChanged(m_undoAvailable);
        OnTextRedoAvailableChanged(m_redoAvailable);
        OnTextEditSearchChanged();

        m_view->m_menuPaste->setDisabled(m_textEdit->isReadOnly());

        m_view->m_editSearchBox->setEnabled(true);
        m_view->m_menuSearch->setEnabled(true);
        m_view->m_findNextShortcut->setEnabled(true);
        m_view->m_hideSearchShortcut->setEnabled(true);
        m_view->m_findPrevShortcut->setEnabled(true);
        m_view->m_clearSearchBox->setEnabled(true);
        m_view->m_searchLabel->setEnabled(true);
        m_view->m_editSearchBox->setVisible(true);
        m_view->m_searchLabel->setVisible(true);

        m_view->SetParentFrame(m_textEdit);

        connectUntilHidden(m_textEdit, &DetailTextEditWidget::OnResized, m_view, &ResourceManagementView::OnTextEditResized);
    }

protected:

    DetailTextEditWidget* m_textEdit {
        nullptr
    };
    QLabel* m_helpLabel {
        nullptr
    };
    QLabel* m_lineLabel {
        nullptr
    };
    QLabel* m_columnLabel {
        nullptr
    };
    QHBoxLayout* m_footerLayout {
        nullptr
    };

    void AddButton(QPushButton* button)
    {
        m_footerLayout->addWidget(button);
    }

private:

    // Doesn't seem to be a way to query the QTextEdit for this
    // state, so we save it so it can be restored when this
    // widget is shown.
    bool m_copyAvailable {
        false
    };
    bool m_undoAvailable {
        false
    };
    bool m_redoAvailable {
        false
    };

    QString SearchText()
    {
        return m_view->m_editSearchBox->text();
    }

    void OnSearchShortcut()
    {
        m_view->ShowSearchFrame();
        m_view->m_editSearchBox->setFocus();
        m_view->m_editSearchBox->selectAll();
    }

    void OnFindNextShortcut()
    {
        if (!m_textEdit->find(SearchText()))
        {
            m_textEdit->moveCursor(QTextCursor::Start);
            m_textEdit->find(SearchText());
        }
    }

    void OnFindPreviousShortcut()
    {
        if (!m_textEdit->find(SearchText(), QTextDocument::FindBackward))
        {
            m_textEdit->moveCursor(QTextCursor::End);
            m_textEdit->find(SearchText(), QTextDocument::FindBackward);
        }
    }

    void SetNextPreviousEnabledState(bool enabledState)
    {
        m_view->m_findNext->setEnabled(enabledState);
        m_view->m_findPrevious->setEnabled(enabledState);
    }

    void OnTextEditSearchChanged()
    {
        if (!isVisible())
        {
            return;
        }

        if (!SearchText().length())
        {
            m_textEdit->setExtraSelections({});
        }

        QList<QTextEdit::ExtraSelection> extraSelections;

        QTextCursor previousCursor = m_textEdit->textCursor();

        m_textEdit->moveCursor(QTextCursor::Start);
        QColor color = QColor(Qt::gray).lighter(ResourceManagementView::SEARCH_LIGHTER_VALUE);

        while (m_textEdit->find(SearchText()))
        {
            QTextEdit::ExtraSelection extra;
            extra.format.setBackground(color);

            extra.cursor = m_textEdit->textCursor();
            extraSelections.append(extra);
        }

        m_view->m_clearSearchBox->setVisible(SearchText().length() > 0);
        SetNextPreviousEnabledState(extraSelections.length() > 1);
        m_textEdit->setExtraSelections(extraSelections);

        // Try to restore our search from our previous location if possible
        if (!previousCursor.isNull())
        {
            previousCursor.setPosition(previousCursor.selectionStart());
            m_textEdit->setTextCursor(previousCursor);

            if (extraSelections.length())
            {
                if (!m_textEdit->find(SearchText()))
                {
                    // Otherwise search to the first one we found from the top
                    m_textEdit->setTextCursor(extraSelections.front().cursor);
                }
            }
        }
    }

    void OnTextEditSearchClear()
    {
        m_view->m_editSearchBox->clear();
    }

    void OnTextEditHideSearch()
    {
        OnTextEditSearchClear();
        m_view->HideSearchFrame();
    }

    void OnTextEditCursorMoved()
    {
        QTextCursor textCursor = m_textEdit->textCursor();
        if (textCursor.isNull())
        {
            m_lineLabel->setHidden(true);
            m_columnLabel->setHidden(true);
        }
        else
        {
            m_lineLabel->setText(tr("Line %1").arg(textCursor.blockNumber()));
            m_lineLabel->setHidden(false);

            m_columnLabel->setText(tr("Col %1").arg(textCursor.columnNumber()));
            m_columnLabel->setHidden(false);
        }
    }

    void OnMenuCut()
    {
        m_textEdit->cut();
    }

    void OnMenuCopy()
    {
        m_textEdit->copy();
    }

    void OnMenuPaste()
    {
        m_textEdit->paste();
    }

    void OnMenuUndo()
    {
        m_textEdit->undo();
    }

    void OnMenuRedo()
    {
        m_textEdit->redo();
    }

    void OnTextCopyAvailableChanged(bool isAvailable)
    {
        m_view->m_menuCopy->setDisabled(isAvailable == false);
        m_view->m_menuCut->setDisabled(isAvailable == false || m_textEdit->isReadOnly());
        m_copyAvailable = isAvailable;
    }

    void OnTextRedoAvailableChanged(bool isAvailable)
    {
        m_view->m_menuRedo->setDisabled(isAvailable == false);
        m_redoAvailable = isAvailable;
    }

    void OnTextUndoAvailableChanged(bool isAvailable)
    {
        m_view->m_menuUndo->setDisabled(isAvailable == false);
        m_undoAvailable = isAvailable;
    }
};
