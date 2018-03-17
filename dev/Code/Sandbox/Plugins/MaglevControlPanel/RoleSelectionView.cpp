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
#include <stdafx.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <QAWSCredentialsEditor.h>
#include <QApplication>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QUrl>
#include <QErrorMessage>
#include <QRadioButton>
#include <QScrollArea>
#include <RoleSelectionView.moc>
#include <EditorCoreAPI.h>
#include <IAWSResourceManager.h>
#include <MaglevControlPanelPlugin.h>
#include <algorithm>

using namespace AZStd;

static const int scrollMaxHeight = 700;

ScrollSelectionDialog::ScrollSelectionDialog(QWidget* parent)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::Dialog | Qt::WindowCloseButtonHint)
{
}

ScrollSelectionDialog::~ScrollSelectionDialog()
{
}

void ScrollSelectionDialog::Display()
{
    exec();
}

void ScrollSelectionDialog::InitializeWindow()
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(GetWindowName());

    setLayout(&m_mainContainer);

    AddScrollHeadings(&m_mainContainer);

    m_scrollLayout = new QVBoxLayout;

    m_scrollWidget = new QWidget;
    m_scrollWidget->setLayout(m_scrollLayout);

    m_scrollWidget->resize(defaultScrollWidth, defaultScrollHeight);
    m_scrollWidget->setFixedWidth(defaultScrollWidth);
    m_scrollWidget->setMinimumHeight(defaultScrollHeight);

    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidget(m_scrollWidget);
    m_scrollArea->setFixedWidth(defaultScrollWidth + scrollBuffer);
    m_scrollArea->setMinimumHeight(defaultScrollHeight);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_mainContainer.addWidget(m_scrollArea);

    AddButtonRow(&m_mainContainer);

    setFixedWidth(GetDefaultWidth());

    m_scrollArea->setMaximumHeight(scrollMaxHeight);

    connect(&m_buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(OnRadioButtonClicked(int)));

    SetupConnections();

    OnModelReset();
    MoveCenter();
}

void ScrollSelectionDialog::AddNoDataLabel(QVBoxLayout* scrollLayout)
{
    if (GetNoDataText() != nullptr)
    {
        SetNoDataLabel(new QLabel(GetNoDataText()));
        SetupLabel(GetNoDataLabel(), scrollLayout);
        SetupNoDataConnection();
    }
}

void ScrollSelectionDialog::SetupLabel(QLabel* label, QVBoxLayout* scrollLayout)
{
    if (label != nullptr)
    {
        label->setWordWrap(true);
        label->setTextFormat(Qt::AutoText);
        label->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        label->setFixedWidth(ScrollSelectionDialog::defaultScrollWidth);
        scrollLayout->addWidget(label);
        m_extraScrollWidgets.push_back(label);
    }
}

void ScrollSelectionDialog::AddHorizontalSeparator(QVBoxLayout* mainLayout)
{
    QFrame* frameSeparator = new QFrame;
    frameSeparator->setFrameShape(QFrame::HLine);
    frameSeparator->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(frameSeparator, 1);
}

void ScrollSelectionDialog::AddButtonRow(QVBoxLayout* mainLayout)
{
    QHBoxLayout* cancelOkLayout = new QHBoxLayout;
    cancelOkLayout->setAlignment(Qt::AlignRight);

    AddOkCancel(cancelOkLayout);

    mainLayout->addLayout(cancelOkLayout);
}

void ScrollSelectionDialog::AddOkCancel(QHBoxLayout* cancelOkLayout)
{
    // This is a fairly small window and the cancel and ok default size seems a bit large
    QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(pButtonBox, &QDialogButtonBox::rejected, this, &ScrollSelectionDialog::OnCancel);
    connect(pButtonBox, &QDialogButtonBox::accepted, this, &ScrollSelectionDialog::OnOK);

    cancelOkLayout->addWidget(pButtonBox);
}

void ScrollSelectionDialog::OnRadioButtonClicked(int)
{
}

void ScrollSelectionDialog::OnModelReset()
{
    BindData();
}

void ScrollSelectionDialog::OnCancel()
{
    CloseWindow();
}

void ScrollSelectionDialog::CloseWindow()
{
    // We deal with both options currently because this window is currently transitioning
    // between the "old" view pane system and just being a QDialog
    close();
    GetIEditor()->CloseView(GetWindowName());
}

void ScrollSelectionDialog::MoveCenter()
{
    QRect scr = QApplication::desktop()->screenGeometry(0);
    move(scr.center() - rect().center());
}

void ScrollSelectionDialog::OnOK()
{
    QAbstractButton* checkedButton = m_buttonGroup.checkedButton();
    if (checkedButton)
    {
        QString text = checkedButton->text();
        SetCurrentSelection(text);
    }
    CloseWindow();
}

QString ScrollSelectionDialog::GetSelectedButtonText() const
{
    QAbstractButton* checkedButton = m_buttonGroup.checkedButton();
    if (checkedButton)
    {
        return checkedButton->text();
    }
    return {};
}

void ScrollSelectionDialog::BindData()
{
    auto buttons = m_buttonGroup.buttons();

    for (int i = 0; i < buttons.count(); ++i)
    {
        QAbstractButton* item = buttons.at(i);
        m_buttonGroup.removeButton(item);
        m_scrollLayout->removeWidget(item);
        delete item;
    }

    for (auto thisWidget : m_extraScrollWidgets)
    {
        m_scrollLayout->removeWidget(thisWidget);
        delete thisWidget;
    }
    m_extraScrollWidgets.clear();
    int numHeadingRows = 0;
    int additionalHeadingSize = 0;
    if (!GetRowCount())
    {
        AddNoDataLabel(m_scrollLayout);

        additionalHeadingSize += GetNoDataLabel()->heightForWidth(defaultScrollWidth);
        additionalHeadingSize += verticalHeadingMarginSize;
    }
    else
    {
        numHeadingRows += AddScrollColumnHeadings(m_scrollLayout);

        if (GetHasDataText() != nullptr)
        {
            QLabel* label = new QLabel {
                GetHasDataText()
            };
            SetupLabel(label, m_scrollLayout);

            additionalHeadingSize += label->heightForWidth(defaultScrollWidth);
            additionalHeadingSize += verticalHeadingMarginSize;
        }

        for (int row = 0; row < GetRowCount(); ++row)
        {
            AddRow(row, m_scrollLayout);
        }
    }

    numHeadingRows += AddLowerScrollControls(m_scrollLayout);
    m_scrollWidget->resize(defaultScrollWidth, additionalHeadingSize + (GetRowCount() + numHeadingRows) * scrollRowHeight);

    DoDynamicResize();
    OnBindData();

    QAbstractButton* selectedButton = m_buttonGroup.checkedButton();
    if (selectedButton)
    {
        m_scrollArea->ensureWidgetVisible(selectedButton);
    }
}

void ScrollSelectionDialog::DoDynamicResize()
{
    const int scrollBarBuffer = 10; // Extra height given to prevent the scroll bar from appearing by default
    QSize widgetSize = m_scrollWidget->size();
    widgetSize.setHeight(widgetSize.height() + scrollBarBuffer);
    m_scrollArea->resize(widgetSize);

    QSize scrollSize = m_scrollArea->size();
    if (GetRowCount())
    {
        int scrollHeight = std::min(m_scrollWidget->height(), scrollMaxHeight);
        scrollSize.setHeight(scrollHeight + defaultHeight - defaultScrollHeight + windowBuffer);
    }
    else
    {
        scrollSize.setHeight(defaultHeight);       
    }

    setMinimumHeight(scrollSize.height());
    resize(scrollSize);
}

void ScrollSelectionDialog::AddRow(int rowNum, QVBoxLayout* scrollLayout)
{
    QRadioButton* radioButton = new QRadioButton;

    radioButton->setText(GetDataForRow(rowNum));

    if (IsSelected(rowNum))
    {
        radioButton->setChecked(true);
    }
    else
    {
        radioButton->setChecked(false);
    }

    m_buttonGroup.addButton(radioButton);
    scrollLayout->addWidget(radioButton);
}

void ScrollSelectionDialog::AddExtraScrollWidget(QWidget* addWidget)
{
    m_extraScrollWidgets.push_back(addWidget);
}

void ScrollSelectionDialog::UpdateView()
{
}

