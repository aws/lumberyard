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

#include "DefaultViewWidget.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAction>
#include <QEvent>
#include <QMouseEvent>

#define STRETCH_VALUE 10

DefaultViewWidget::DefaultViewWidget(QWidget* parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_buttonLayout(nullptr)
    , m_bottomStretch(nullptr)
{
    m_image = new QLabel(this);
    m_image->setFixedSize(0, 0);
    m_label = new QLabel(this);
    m_topSpace = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_spaceBetweenLabelAndImage = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_spaceBetweenLabelAndButtons = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_bottomStretch = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_layout = new QVBoxLayout(this);
    m_layout->setSizeConstraint(QLayout::SetMinimumSize);
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch(STRETCH_VALUE); //helps keep buttons centered
    setLayout(m_layout);
    //if there is no image it's size is (0, 0)
    m_layout->addItem(m_topSpace);
    m_layout->addWidget(m_image);
    m_layout->addItem(m_spaceBetweenLabelAndImage);
    m_layout->addWidget(m_label);
    //add the button layout if any buttons are added
    m_layout->addItem(m_spaceBetweenLabelAndButtons);
    m_layout->addLayout(m_buttonLayout);
    for (int i = 0; i < m_buttons.count(); i++)
    {
        m_buttonLayout->addWidget(m_buttons[i]);
    }
    SetSpaceAtTop(STANDARD_TOP_SPACING);
    m_layout->addItem(m_bottomStretch);
}

DefaultViewWidget::~DefaultViewWidget()
{
    qDeleteAll(m_buttons);
    m_buttons.clear();
}

void DefaultViewWidget::SetLabel(QString text)
{
    m_label->setText(text);
    m_label->setAlignment(Qt::AlignHCenter);
}

void DefaultViewWidget::SetImage(QString path, QSize size)
{
    m_image->setPixmap(QPixmap(path));
    m_image->setFixedSize(size);
}

void DefaultViewWidget::AddButton(QString text, std::function<void()> onPressed)
{
    m_buttons.push_back(new QPushButton(text, this));
    connect(m_buttons.back(), &QPushButton::pressed, this, onPressed);
    m_buttons.back()->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_buttonLayout->addWidget(m_buttons.back(), Qt::AlignTop | Qt::AlignHCenter);
    m_buttonLayout->addStretch(STRETCH_VALUE); //add a stretch after each button to keep them evenly spaced and correctly sized
}

void DefaultViewWidget::AddButton(QString text, QAction* action)
{
    m_buttons.push_back(new QPushButton(text, this));
    m_buttons.back()->addAction(action);
    connect(m_buttons.back(), &QPushButton::pressed, action, &QAction::trigger);
    m_buttons.back()->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_buttonLayout->addWidget(m_buttons.back(), Qt::AlignTop | Qt::AlignHCenter);
    m_buttonLayout->addStretch(STRETCH_VALUE); //add a stretch after each button to keep them evenly spaced and correctly sized
}

void DefaultViewWidget::showEvent(QShowEvent* e)
{
    Invalidate();
    QWidget::showEvent(e);
}

void DefaultViewWidget::Invalidate()
{
    m_layout->update();
    m_layout->activate();
    m_buttonLayout->update();
    m_buttonLayout->activate();
}

void DefaultViewWidget::SetSpaceAtTop(int height)
{
    m_topSpace->changeSize(1, height, QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void DefaultViewWidget::SetSpaceBetweenImageAndLabel(int height)
{
    m_spaceBetweenLabelAndImage->changeSize(1, height, QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void DefaultViewWidget::SetSpaceBetweenLabelsAndButtons(int height)
{
    m_spaceBetweenLabelAndButtons->changeSize(1, height, QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void DefaultViewWidget::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
    if (mouseEvent->button() == Qt::LeftButton)
    {
        emit SignalLeftClicked(mouseEvent->globalPos());
    }
    else if (mouseEvent->button() == Qt::RightButton)
    {
        emit SignalRightClicked(mouseEvent->globalPos());
    }
    return QWidget::mouseReleaseEvent(mouseEvent);
}


#undef STRETCH_VALUE
#include <DefaultViewWidget.moc>
