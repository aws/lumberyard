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
#include "./FluidTabBar.h"
#include <FluidTabBar.moc>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QPushButton>

FluidTabBar::FluidTabBar(QWidget* p)
    : QTabBar(p)
{
    connect(this, &QTabBar::currentChanged, this, &FluidTabBar::AddCloseButtonToCurrentOnly);
}

QSize FluidTabBar::sizeHint() const
{
    return QSize(-1, QTabBar::sizeHint().height());
}
QSize FluidTabBar::minimumSizeHint() const
{
    return QSize(-1, QTabBar::minimumSizeHint().height());
}

void FluidTabBar::paintEvent(QPaintEvent* e)
{
    QTabBar::paintEvent(e);
}

void FluidTabBar::CloseAll()
{
    int c = count();
    for (int i = c - 1; i >= 0; i--)
    {
        if (i == currentIndex())
        {
            continue;
        }
        tabCloseRequested(i);
    }
    int i = currentIndex();
    tabCloseRequested(i); //Close current as last to avoid generating many onChanged events
}
void FluidTabBar::CloseAllBut(int keepOpenIdx)
{
    QString tabUID = tabText(keepOpenIdx); //This should be a unique id
    int c = count();
    for (int i = c - 1; i >= 0; i--)
    {
        if (tabText(i).compare(tabUID) == 0 || i == currentIndex())
        {
            continue;
        }
        tabCloseRequested(i);
    }
    //Close current as last to avoid generating many onChanged events
    c = count();
    for (int i = c - 1; i >= 0; i--)
    {
        if (tabText(i).compare(tabUID) == 0)
        {
            continue;
        }
        tabCloseRequested(i);
    }
}

void FluidTabBar::AddCloseButtonToCurrentOnly()
{
    for (int i = 0; i < count(); i++)
    {
        if (i == currentIndex())
        {
            AddCloseButton(i);
        }
        else
        {
            setTabButton(i, QTabBar::RightSide, nullptr);
        }
    }
}

void FluidTabBar::AddCloseButton(int tabIndex)
{
    /*if(tabText(tabIndex).compare("Blank")==0)
    {
        RemoveCloseButton(tabIndex);
        return;
    }*/
    QPushButton* closeButton = new QPushButton();
    closeButton->setObjectName("FluidTabCloseButton");
    setTabButton(tabIndex, QTabBar::RightSide, closeButton);
    connect(closeButton, &QPushButton::clicked, this, [this]()
        {
            tabCloseRequested(currentIndex());
        });
}

void FluidTabBar::RemoveCloseButton(int tabIndex)
{
    setTabButton(tabIndex, QTabBar::RightSide, nullptr);
}

void FluidTabBar::CloseTab(QString name)
{
    for (unsigned int i = 0; i < count(); i++)
    {
        if (QString::compare(tabText(i), name, Qt::CaseInsensitive) == 0)
        {
            tabCloseRequested(i);
        }
    }
}
