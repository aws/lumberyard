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

#ifndef TreePanel_h
#define TreePanel_h

#pragma once

#include <QDockWidget>

#include "BehaviorTreeDocument.h"

#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QDialog>

class QPropertyTree;

class TreePanel
    : public QDockWidget
{
    Q_OBJECT

public:
    explicit TreePanel(QWidget* parent = nullptr);

    void Reset();

    void OnWindowEvent_NewFile();
    void OnWindowEvent_OpenFile();
    void OnWindowEvent_Save();
    void OnWindowEvent_SaveToFile();

    bool HandleCloseEvent();

public slots:
    void OnPropertyTreeDataChanged();

private:
    bool CheckForUnsavedDataAndSave();
    bool CheckForBehaviorTreeErrors();

    QPropertyTree* m_propertyTree;

    BehaviorTreeDocument m_behaviorTreeDocument;
    bool propertiesAttachedToDocument;
};

struct UnsavedChangesDialog
    : public QDialog
{
    enum Result
    {
        Yes,
        No,
        Cancel
    };

    Result result;

    UnsavedChangesDialog()
        : result(Cancel)
    {
        setWindowTitle("Behavior Tree Editor");

        QLabel* label = new QLabel("There are unsaved changes. Do you want to save them?");

        QPushButton* yesButton = new QPushButton("Yes");
        QPushButton* noButton = new QPushButton("No");
        QPushButton* cancelButton = new QPushButton("Cancel");

        QGridLayout* gridLayout = new QGridLayout();
        gridLayout->addWidget(label, 0, 0);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->addWidget(yesButton);
        buttonLayout->addWidget(noButton);
        buttonLayout->addWidget(cancelButton);

        gridLayout->addLayout(buttonLayout, 1, 0, Qt::AlignCenter);

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->addLayout(gridLayout);
        setLayout(mainLayout);

        connect(yesButton, SIGNAL(clicked()), this, SLOT(clickedYes()));
        connect(noButton, SIGNAL(clicked()), this, SLOT(clickedNo()));
        connect(cancelButton, SIGNAL(clicked()), this, SLOT(clickedCancel()));
    }

    Q_OBJECT

public Q_SLOTS:
    void clickedYes()
    {
        result = Yes;
        accept();
    }

    void clickedNo()
    {
        result = No;
        reject();
    }

    void clickedCancel()
    {
        result = Cancel;
        close();
    }
};

#endif // TreePanel_h