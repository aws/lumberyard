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

#include "pch.h"
#include "Serialization/ClassFactory.h"
#include "Serialization/Pointers.h"
#include "Serialization/IArchive.h"
#include "JointNameSelector.h"
#include <IEditor.h>
#include <ICryPak.h>
#include <QDialogButtonBox>
#include <QBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QIcon>
#include <QMenu>
#include <QEvent>
#include <QKeyEvent>
#include <QCoreApplication>
#include <ICryAnimation.h>
#include "IResourceSelectorHost.h"

#include "../EditorCommon/DeepFilterProxyModel.h"

// ---------------------------------------------------------------------------

JointSelectionDialog::JointSelectionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Choose Joint...");
    setWindowIcon(QIcon("Editor/Icons/animation/bone.png"));
    setWindowModality(Qt::ApplicationModal);

    QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom);
    setLayout(layout);

    QBoxLayout* filterBox = new QBoxLayout(QBoxLayout::LeftToRight);
    layout->addLayout(filterBox);
    {
        filterBox->addWidget(new QLabel("Filter:", this), 0);
        filterBox->addWidget(m_filterEdit = new QLineEdit(this), 1);
        connect(m_filterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(onFilterChanged(const QString&)));
        m_filterEdit->installEventFilter(this);
    }

    QBoxLayout* infoBox = new QBoxLayout(QBoxLayout::LeftToRight);
    layout->addLayout(infoBox);
    {
        infoBox->addWidget(new QLabel("Skeleton:"));
        infoBox->addWidget(m_skeletonLabel = new QLabel(""), 1);
        //QFont font = m_skeletonLabel->font();
        //font.setBold(true);
        //m_skeletonLabel->setFont(font);
    }

    m_model = new QStandardItemModel();
#if 0
    m_model->setColumnCount(3);
#else
    m_model->setColumnCount(2);
#endif
    m_model->setHeaderData(0, Qt::Horizontal, "Joint Name", Qt::DisplayRole);
    m_model->setHeaderData(1, Qt::Horizontal, "Id", Qt::DisplayRole);
#if 0
    m_model->setHeaderData(2, Qt::Horizontal, "CRC32", Qt::DisplayRole);
#endif

    m_filterModel = new DeepFilterProxyModel(this);
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setDynamicSortFilter(true);

    m_tree = new QTreeView(this);
    //m_tree->setColumnCount(3);
    m_tree->setModel(m_filterModel);

    m_tree->header()->setStretchLastSection(false);
#if QT_VERSION >= 0x50000
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Interactive);
#if 0
    m_tree->header()->setSectionResizeMode(2, QHeaderView::Interactive);
#endif
#else
    m_tree->header()->setResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setResizeMode(1, QHeaderView::Interactive);
#if 0
    m_tree->header()->setResizeMode(2, QHeaderView::Interactive);
#endif
#endif
    m_tree->header()->resizeSection(1, 40);
    m_tree->header()->resizeSection(2, 80);
    connect(m_tree, SIGNAL(activated(const QModelIndex&)), this, SLOT(onActivated(const QModelIndex&)));

    layout->addWidget(m_tree, 1);

    QDialogButtonBox* buttons = new QDialogButtonBox(this);
    buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttons, 0);
}

bool JointSelectionDialog::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_filterEdit && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = (QKeyEvent*)event;
        if (keyEvent->key() == Qt::Key_Down ||
            keyEvent->key() == Qt::Key_Up ||
            keyEvent->key() == Qt::Key_PageDown ||
            keyEvent->key() == Qt::Key_PageUp)
        {
            QCoreApplication::sendEvent(m_tree, event);
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void JointSelectionDialog::onFilterChanged(const QString& str)
{
    m_filterModel->setFilterString(str);
    m_filterModel->invalidate();
    m_tree->expandAll();

    QModelIndex currentSource = m_filterModel->mapToSource(m_tree->selectionModel()->currentIndex());
    if (!currentSource.isValid() || !m_filterModel->matchFilter(currentSource.row(), currentSource.parent()))
    {
        QModelIndex firstMatchingIndex = m_filterModel->findFirstMatchingIndex(QModelIndex());
        if (firstMatchingIndex.isValid())
        {
            m_tree->selectionModel()->setCurrentIndex(firstMatchingIndex, QItemSelectionModel::SelectCurrent);
        }
    }
}

void JointSelectionDialog::onActivated(const QModelIndex& index)
{
    m_tree->setCurrentIndex(index);
    accept();
}

QSize JointSelectionDialog::sizeHint() const
{
    return QSize(600, 900);
}

bool JointSelectionDialog::chooseJoint(QString &name, IDefaultSkeleton* skeleton)
{
    m_skeletonLabel->setText(skeleton ? skeleton->GetModelFilePath() : "No character loaded");
    if (skeleton)
    {
        QString selectedName = name;

        typedef std::vector<QList<QStandardItem*> > JointIDToItem;
        JointIDToItem jointToItem;
        QStandardItem* selectedItem = 0;

        int jointCount = skeleton->GetJointCount();
        jointToItem.resize(jointCount);
        for (int i = 0; i < jointCount; ++i)
        {
            QString jointName = QString::fromLocal8Bit(skeleton->GetJointNameByID(i));
            QList<QStandardItem*>& items = jointToItem[i];
            QStandardItem* item;
            item = new QStandardItem(jointName);
            item->setEditable(false);
            item->setData(jointName);
            items.append(item);

            QString idStr;
            idStr.sprintf("%i", i);
            item = new QStandardItem(idStr);
            item->setEditable(false);
            item->setData(jointName);
            items.append(item);

#if 0
            QString crcStr;
            crcStr.sprintf("%08x", skeleton->GetJointCRC32ByID(i));
            item = new QStandardItem(crcStr);
            item->setEditable(false);
            item->setData(jointName);
            items.append(item);
#endif

            if (jointName == selectedName)
            {
                selectedItem = items[0];
            }
        }

        for (int i = 0; i < jointCount; ++i)
        {
            int parent = skeleton->GetJointParentIDByID(i);
            if (size_t(parent) >= jointToItem.size())
            {
                m_model->appendRow(jointToItem[i]);
            }
            else
            {
                jointToItem[parent][0]->appendRow(jointToItem[i]);
            }
        }

        if (selectedItem)
        {
            m_tree->selectionModel()->setCurrentIndex(m_filterModel->mapFromSource(m_model->indexFromItem(selectedItem)), QItemSelectionModel::SelectCurrent);
        }

        m_tree->expandAll();
    }


    if (exec() == QDialog::Accepted && m_tree->selectionModel()->currentIndex().isValid())
    {
        QModelIndex currentIndex = m_tree->selectionModel()->currentIndex();
        QModelIndex sourceCurrentIndex = m_filterModel->mapToSource(currentIndex);
        QStandardItem* item = m_model->itemFromIndex(sourceCurrentIndex);
        if (item)
        {
            name = item->data().toString();
            return true;
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
QString JointNameSelector(const SResourceSelectorContext& x, const QString& previousValue, IDefaultSkeleton* defaultSkeleton)
{
    QWidget parent(x.parentWidget);
    parent.setWindowModality(Qt::ApplicationModal);
    JointSelectionDialog dialog(&parent);

    QString jointName = previousValue;
    dialog.chooseJoint(jointName, defaultSkeleton);
    return jointName;
}
REGISTER_RESOURCE_SELECTOR("Joint", JointNameSelector, "Editor/Icons/animation/bone.png")

#include <CharacterTool/JointNameSelector.moc>
