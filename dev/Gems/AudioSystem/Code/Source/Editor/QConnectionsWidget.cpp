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

#include <QConnectionsWidget.h>

#include <ACEEnums.h>
#include <AudioControl.h>
#include <AudioControlsEditorPlugin.h>
#include <IAudioSystemControl.h>
#include <IAudioSystemEditor.h>
#include <IEditor.h>
#include <ImplementationManager.h>

#include <QDropEvent>
#include <QEvent>
#include <QMimeData>
#include <QMessageBox>
#include <QMenu>
#include <Serialization/IArchive.h>
#include <Serialization/STL.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    QConnectionsWidget::QConnectionsWidget(QWidget* pParent, const AZStd::string& sGroup)
        : QWidget(pParent)
        , m_sGroup(sGroup)
        , m_notFoundColor(QColor(0xf3, 0x81, 0x1d))
        , m_localisedColor(QColor(0x42, 0x85, 0xf4))
        , m_pControl(nullptr)
    {
        setupUi(this);

        m_pConnectionProperties->setSizeToContent(true);
        m_pConnectionPropertiesFrame->setHidden(true);
        connect(m_pConnectionProperties, SIGNAL(signalChanged()), this, SLOT(CurrentConnectionModified()));

        m_pConnectionList->viewport()->installEventFilter(this);
        m_pConnectionList->installEventFilter(this);

        connect(m_pConnectionList, SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(ShowConnectionContextMenu(const QPoint&)));
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::Init(const AZStd::string& sGroup)
    {
        m_sGroup = sGroup;
        connect(m_pConnectionList, SIGNAL(itemSelectionChanged()), this, SLOT(SelectedConnectionChanged()));
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::CurrentConnectionModified()
    {
        if (m_pControl)
        {
            m_pControl->SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    bool QConnectionsWidget::eventFilter(QObject* pObject, QEvent* pEvent)
    {
        IAudioSystemEditor* pAudioSystemEditorImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (pAudioSystemEditorImpl && m_pControl && pEvent->type() == QEvent::Drop)
        {
            QDropEvent* pDropEvent = static_cast<QDropEvent*>(pEvent);
            const QMimeData* pData = pDropEvent->mimeData();
            QString format = "application/x-qabstractitemmodeldatalist";
            if (pData->hasFormat(format))
            {
                QByteArray encoded = pData->data(format);
                QDataStream stream(&encoded, QIODevice::ReadOnly);
                while (!stream.atEnd())
                {
                    int row, col;
                    QMap<int,  QVariant> roleDataMap;
                    stream >> row >> col >> roleDataMap;
                    if (!roleDataMap.isEmpty())
                    {
                        if (IAudioSystemControl* pMiddlewareControl = pAudioSystemEditorImpl->GetControl(roleDataMap[eMDR_ID].toUInt()))
                        {
                            if (pAudioSystemEditorImpl->GetCompatibleTypes(m_pControl->GetType()) & pMiddlewareControl->GetType())
                            {
                                MakeConnectionTo(pMiddlewareControl);
                            }
                        }
                    }
                }
            }
            return true;
        }
        else if (pEvent->type() == QEvent::KeyPress)
        {
            QKeyEvent* pDropEvent = static_cast<QKeyEvent*>(pEvent);
            if (pDropEvent && pDropEvent->key() == Qt::Key_Delete && pObject == m_pConnectionList)
            {
                RemoveSelectedConnection();
                return true;
            }
        }
        return QWidget::eventFilter(pObject, pEvent);
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::ShowConnectionContextMenu(const QPoint& pos)
    {
        QMenu contextMenu(tr("Context menu"), this);
        contextMenu.addAction(tr("Remove Connection"), this, SLOT(RemoveSelectedConnection()));
        contextMenu.exec(m_pConnectionList->mapToGlobal(pos));
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::SelectedConnectionChanged()
    {
        TConnectionPtr pConnection;
        EACEControlType eType = AudioControls::eACET_NUM_TYPES;
        if (m_pControl)
        {
            QList<QListWidgetItem*> selected = m_pConnectionList->selectedItems();
            if (selected.length() == 1)
            {
                QListWidgetItem* pCurrent = selected[0];
                if (pCurrent)
                {
                    const CID nExternalId = pCurrent->data(eMDR_ID).toInt();
                    pConnection = m_pControl->GetConnection(nExternalId);
                    eType = m_pControl->GetType();
                }
            }
        }

        if (pConnection && pConnection->HasProperties())
        {
            m_pConnectionProperties->attach(Serialization::SStruct(*pConnection.get()));
            m_pConnectionPropertiesFrame->setHidden(false);
        }
        else
        {
            m_pConnectionProperties->detach();
            m_pConnectionPropertiesFrame->setHidden(true);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::MakeConnectionTo(IAudioSystemControl* pAudioMiddlewareControl)
    {
        IAudioSystemEditor* pAudioSystemEditorImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (m_pControl && pAudioMiddlewareControl && pAudioSystemEditorImpl)
        {
            CUndo undo("Connected Audio Control to Audio System");

            TConnectionPtr pConnection = m_pControl->GetConnection(pAudioMiddlewareControl, m_sGroup);
            if (pConnection)
            {
                // connection already exists, select it
                const int size = m_pConnectionList->count();
                for (int i = 0; i < size; ++i)
                {
                    QListWidgetItem* pItem = m_pConnectionList->item(i);
                    if (pItem && pItem->data(eMDR_ID).toInt() == pAudioMiddlewareControl->GetId())
                    {
                        m_pConnectionList->clearSelection();
                        pItem->setSelected(true);
                        m_pConnectionList->setCurrentItem(pItem);
                        m_pConnectionList->scrollToItem(pItem);
                        break;
                    }
                }
            }
            else
            {
                pConnection = pAudioSystemEditorImpl->CreateConnectionToControl(m_pControl->GetType(), pAudioMiddlewareControl);
                if (pConnection)
                {
                    pConnection->SetGroup(m_sGroup);
                    m_pControl->AddConnection(pConnection);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::RemoveSelectedConnection()
    {
        CUndo undo("Disconnected Audio Control from Audio System");
        if (m_pControl)
        {
            QMessageBox messageBox(this);
            messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            messageBox.setDefaultButton(QMessageBox::Yes);
            messageBox.setWindowTitle("Audio Controls Editor");
            QList<QListWidgetItem*> selected = m_pConnectionList->selectedItems();
            const int size = selected.length();
            if (size > 0)
            {
                if (size == 1)
                {
                    messageBox.setText("Are you sure you want to delete the connection between \"" + QString(m_pControl->GetName().c_str()) + "\" and \"" + selected[0]->text() + "\"?");
                }
                else
                {
                    messageBox.setText("Are you sure you want to delete the " + QString::number(size) + " selected connections?");
                }
                if (messageBox.exec() == QMessageBox::Yes)
                {
                    if (IAudioSystemEditor* pAudioSystemEditorImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl())
                    {
                        AZStd::vector<IAudioSystemControl*> connectedMiddlewareControls;
                        connectedMiddlewareControls.reserve(selected.size());
                        for (int i = 0; i < size; ++i)
                        {
                            CID nAudioSystemControlID = selected[i]->data(eMDR_ID).toInt();
                            connectedMiddlewareControls.push_back(pAudioSystemEditorImpl->GetControl(nAudioSystemControlID));
                        }

                        for (int i = 0; i < size; ++i)
                        {
                            if (IAudioSystemControl* pAudioSystemControl = connectedMiddlewareControls[i])
                            {
                                pAudioSystemEditorImpl->ConnectionRemoved(pAudioSystemControl);
                                m_pControl->RemoveConnection(pAudioSystemControl);
                            }
                        }
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::UpdateConnections()
    {
        m_pConnectionList->clear();
        IAudioSystemEditor* pAudioSystemEditorImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (pAudioSystemEditorImpl && m_pControl)
        {
            m_pConnectionList->setEnabled(true);
            const size_t size = m_pControl->ConnectionCount();
            for (size_t i = 0; i < size; ++i)
            {
                TConnectionPtr pConnection = m_pControl->GetConnectionAt(i);
                if (pConnection && pConnection->GetGroup() == m_sGroup)
                {
                    if (IAudioSystemControl* pAudioSystemControl = pAudioSystemEditorImpl->GetControl(pConnection->GetID()))
                    {
                        CreateItemFromConnection(pAudioSystemControl);
                    }
                }
            }
        }
        else
        {
            m_pConnectionList->setEnabled(false);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::CreateItemFromConnection(IAudioSystemControl* pAudioSystemControl)
    {
        if (IAudioSystemEditor* pAudioSystemEditorImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl())
        {
            const TImplControlType nType = pAudioSystemControl->GetType();

            QListWidgetItem* pListItem = new QListWidgetItem(QIcon(QString(pAudioSystemEditorImpl->GetTypeIcon(nType).data())), QString(pAudioSystemControl->GetName().c_str()));
            pListItem->setData(eMDR_ID, pAudioSystemControl->GetId());
            pListItem->setData(eMDR_LOCALISED, pAudioSystemControl->IsLocalised());
            if (pAudioSystemControl->IsPlaceholder())
            {
                pListItem->setToolTip(tr("Control not found in currently loaded audio system project"));
                pListItem->setForeground(m_notFoundColor);
            }
            else if (pAudioSystemControl->IsLocalised())
            {
                pListItem->setForeground(m_localisedColor);
            }
            m_pConnectionList->insertItem(0, pListItem);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void QConnectionsWidget::SetControl(CATLControl* pControl)
    {
        if (m_pControl != pControl)
        {
            m_pControl = pControl;
            m_pConnectionList->clear();
            UpdateConnections();
        }
        else if (m_pControl->ConnectionCount() != m_pConnectionList->count())
        {
            UpdateConnections();
        }
    }

} // namespace AudioControls

#include <Source/Editor/QConnectionsWidget.moc>
