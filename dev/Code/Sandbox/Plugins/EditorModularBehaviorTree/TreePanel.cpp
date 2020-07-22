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
#include "TreePanel.h"

#include <QPropertyTree/QPropertyTree.h>
#include <QPropertyTree/QPropertyTreeStyle.h>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

#include <Util/PathUtil.h> // for getting game editing folder

TreePanel::TreePanel(QWidget* parent)
    : QDockWidget(parent)
    , propertiesAttachedToDocument(false)
{
    m_propertyTree = new QPropertyTree(this);
    QPropertyTreeStyle treeStyle;
    treeStyle.levelIndent = 1.0f;
    treeStyle.firstLevelIndent = 1.0f;

    m_propertyTree->setTreeStyle(treeStyle);

    setWidget(m_propertyTree);
    setFeatures(( QDockWidget::DockWidgetFeatures )(QDockWidget::AllDockWidgetFeatures & ~(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable)));
    connect(m_propertyTree, SIGNAL(signalChanged()), this, SLOT(OnPropertyTreeDataChanged()));
}

void TreePanel::Reset()
{
    m_propertyTree->detach();
    propertiesAttachedToDocument = false;
    m_behaviorTreeDocument.Reset();
}

void TreePanel::OnWindowEvent_NewFile()
{
    if (!CheckForUnsavedDataAndSave())
    {
        return;
    }

    QString gameFolder = QString::fromLocal8Bit(Path::GetEditingGameDataFolder().c_str());
    QDir behaviorFolder(QDir::fromNativeSeparators(gameFolder + "/libs/ai/behavior_trees/"));

    QString absoluteFilePath = QFileDialog::getSaveFileName(this, "Modular Behavior Tree", behaviorFolder.absolutePath(), "XML files (*.xml)");

    if (absoluteFilePath.isEmpty())
    {
        return;
    }

    Reset();

    QFileInfo fileInfo(absoluteFilePath);

    m_behaviorTreeDocument.NewFile(fileInfo.baseName().toStdString().c_str(), absoluteFilePath.toStdString().c_str());

    if (!propertiesAttachedToDocument)
    {
        m_propertyTree->attach(Serialization::SStruct(m_behaviorTreeDocument));
        propertiesAttachedToDocument = true;
    }
}

void TreePanel::OnWindowEvent_OpenFile()
{
    if (!CheckForUnsavedDataAndSave())
    {
        return;
    }

    QString gameFolder = QString::fromLocal8Bit(Path::GetEditingGameDataFolder().c_str());
    QDir behaviorFolder(QDir::fromNativeSeparators(gameFolder + "/libs/ai/behavior_trees/"));

    QString absoluteFilePath = QFileDialog::getOpenFileName(this, "Modular Behavior Tree", behaviorFolder.absolutePath(), "XML files (*.xml)");

    if (absoluteFilePath.isEmpty())
    {
        return;
    }

    Reset();

    QFileInfo fileInfo(absoluteFilePath);

    if (!m_behaviorTreeDocument.OpenFile(fileInfo.baseName().toStdString().c_str(), absoluteFilePath.toStdString().c_str()))
    {
        return;
    }

    if (!propertiesAttachedToDocument)
    {
        m_propertyTree->attach(Serialization::SStruct(m_behaviorTreeDocument));
        propertiesAttachedToDocument = true;
    }
}

void TreePanel::OnWindowEvent_Save()
{
    if (!CheckForBehaviorTreeErrors())
    {
        return;
    }

    if (m_behaviorTreeDocument.Loaded() && m_behaviorTreeDocument.Changed())
    {
        m_behaviorTreeDocument.Save();
    }
}

void TreePanel::OnWindowEvent_SaveToFile()
{
    if (!CheckForBehaviorTreeErrors())
    {
        return;
    }

    if (!m_behaviorTreeDocument.Loaded() && !m_behaviorTreeDocument.Changed())
    {
        return;
    }

    QString gameFolder = QString::fromStdString(Path::GetEditingGameDataFolder().c_str());
    QDir behaviorFolder(QDir::fromNativeSeparators(gameFolder + "/libs/ai/behavior_trees/"));

    QString absoluteFilePath = QFileDialog::getSaveFileName(this, "Modular Behavior Tree", behaviorFolder.absolutePath(), "XML files (*.xml)");

    if (absoluteFilePath.isEmpty())
    {
        return;
    }

    QFileInfo fileInfo(absoluteFilePath);

    m_behaviorTreeDocument.SaveToFile(fileInfo.baseName().toStdString().c_str(), absoluteFilePath.toStdString().c_str());

    m_propertyTree->revert();
}

void TreePanel::OnPropertyTreeDataChanged()
{
    m_behaviorTreeDocument.SetChanged();
}

bool TreePanel::HandleCloseEvent()
{
    return CheckForUnsavedDataAndSave();
}

bool TreePanel::CheckForUnsavedDataAndSave()
{
    if (!m_behaviorTreeDocument.Loaded() || !m_behaviorTreeDocument.Changed())
    {
        return true;
    }

    UnsavedChangesDialog saveDialog;
    saveDialog.exec();
    if (saveDialog.result == UnsavedChangesDialog::Yes)
    {
        if (!CheckForBehaviorTreeErrors())
        {
            return false;
        }

        m_behaviorTreeDocument.Save();
        return true;
    }
    else if (saveDialog.result == UnsavedChangesDialog::Cancel)
    {
        return false;
    }

    return true;
}

bool TreePanel::CheckForBehaviorTreeErrors()
{
    if (m_propertyTree->containsErrors())
    {
        m_propertyTree->focusFirstError();
        QMessageBox::critical(this, "Behavior Tree Editor", "This behavior tree contains an error and could not be saved.");
        return false;
    }

    return true;
}

#include <TreePanel.moc>
