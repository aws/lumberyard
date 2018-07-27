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
#include "./ContextMenu.h"
#include <ContextMenu.moc>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QMenuBar>
#include <QDebug>

ContextMenu::ContextMenu(QWidget* p)
    : QMenu(nullptr)
{
    Initialize();
    // do not set the parent, otherwise we will get the broken stylesheet
    // but delete with it
    // this behaviour can be changed again, once ParticleEditor is restyled
    connect(p, &QObject::destroyed, this, &QObject::deleteLater);
}

ContextMenu::ContextMenu(const QString& title, QWidget* parent)
    : QMenu(title, nullptr)
{
    Initialize();
    // do not set the parent, otherwise we will get the broken stylesheet
    // but delete with it
    // this behaviour can be changed again, once ParticleEditor is restyled
    connect(parent, &QObject::destroyed, this, &QObject::deleteLater);
}

void ContextMenu::Initialize()
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setBlurRadius(5);
    setGraphicsEffect(effect);
}
