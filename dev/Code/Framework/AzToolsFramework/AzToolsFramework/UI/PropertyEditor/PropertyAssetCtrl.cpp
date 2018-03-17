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

#include "StdAfx.h"

#include "PropertyAssetCtrl.hxx"
#include "PropertyQTConstants.h"

#include <QtWidgets/QLineEdit>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QClipboard>
#include <QtGui/QPixmapCache>
#include <QtCore/QMimeData>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorAssetMimeDataContainer.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/UI/Logging/GenericLogPanel.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>
#include <AzToolsFramework/ToolsComponents/ComponentAssetMimeDataContainer.h>

namespace AzToolsFramework
{
    PropertyAssetCtrl::PropertyAssetCtrl(QWidget* pParent, QString optionalValidDragDropExtensions)
        : QWidget(pParent)
        , m_optionalValidDragDropExtensions(optionalValidDragDropExtensions)
        , m_editNotifyTarget(nullptr)
        , m_editNotifyCallback(nullptr)
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        QHBoxLayout* pLayout = aznew QHBoxLayout();
        m_label = aznew QLabel(this);

        pLayout->setContentsMargins(0, 0, 0, 0);

        setFixedHeight(PropertyQTConstant_DefaultHeight);

        m_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_label->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_label->setFixedHeight(PropertyQTConstant_DefaultHeight);

        m_label->setFrameShape(QFrame::Panel);
        m_label->setFrameShadow(QFrame::Sunken);
        m_label->setFocusPolicy(Qt::StrongFocus);

        setAcceptDrops(true);

        m_editButton = aznew QPushButton(this);
        m_editButton->setFlat(true);
        m_editButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_editButton->setFixedSize(QSize(24, 16));
        m_editButton->setMouseTracking(true);
        m_editButton->setContentsMargins(0, 0, 0, 0);
        m_editButton->setIcon(QIcon(":/PropertyEditor/Resources/edit-asset.png"));
        m_editButton->setToolTip("Edit asset");
        m_editButton->setVisible(false);

        m_clearButton = aznew QPushButton(this);
        m_clearButton->setFlat(true);
        m_clearButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_clearButton->setFixedSize(QSize(16, 16));
        m_clearButton->setMouseTracking(true);
        m_clearButton->setIcon(QIcon(":/PropertyEditor/Resources/cross-small.png"));
        m_clearButton->setContentsMargins(0, 0, 0, 0);
        m_clearButton->setToolTip("Clear Asset");

        m_browseButton = aznew QPushButton(this); //changed for preview to have right click to open new asset browser popup
        m_browseButton->setFlat(true);
        m_browseButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_browseButton->setFixedSize(QSize(16, 16));
        m_browseButton->setText("...");
        m_browseButton->setMouseTracking(true);
        m_browseButton->setContentsMargins(0, 0, 0, 0);
        m_browseButton->setToolTip("Browse...");

        connect(m_editButton, &QPushButton::clicked, this, &PropertyAssetCtrl::OnEditButtonClicked);
        connect(m_browseButton, &QPushButton::clicked, this, &PropertyAssetCtrl::PopupAssetBrowser);
        connect(m_clearButton, &QPushButton::clicked, this, &PropertyAssetCtrl::ClearAsset);

        pLayout->addWidget(m_label);
        pLayout->addWidget(m_browseButton);
        pLayout->addWidget(m_editButton);
        pLayout->addWidget(m_clearButton);

        setLayout(pLayout);
        setFocusProxy(m_label);
        setFocusPolicy(m_label->focusPolicy());

        m_label->installEventFilter(this);
        m_currentAssetType = AZ::Data::s_invalidAssetType;

        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(ShowContextMenu(const QPoint&)));
    }

    void PropertyAssetCtrl::ShowContextMenu(const QPoint& pos)
    {
        QClipboard* clipboard = QApplication::clipboard();
        if (!clipboard)
        {
            // Can't do anything without a clipboard, so just return
            return;
        }

        QPoint globalPos = mapToGlobal(pos);

        QMenu myMenu;

        QAction* copyAction = myMenu.addAction(tr("Copy asset reference"));
        QAction* pasteAction = myMenu.addAction(tr("Paste asset reference"));

        copyAction->setEnabled(GetCurrentAssetID().IsValid());

        bool canPasteFromClipboard = false;

        // Do we have stuff on the clipboard?
        const QMimeData* mimeData = clipboard->mimeData();
        AZ::Data::AssetId readId;

        if (mimeData && mimeData->hasFormat(EditorAssetMimeDataContainer::GetMimeType()))
        {
            AZ::Data::AssetType readType;
            // This verifies that the mime data matches any restrictions for this particular asset property
            if (IsCorrectMimeData(mimeData, &readId, &readType))
            {
                if (readId.IsValid())
                {
                    canPasteFromClipboard = true;
                }
            }
        }

        pasteAction->setEnabled(canPasteFromClipboard);

        QAction* selectedItem = myMenu.exec(globalPos);
        if (selectedItem == copyAction)
        {
            QMimeData* newMimeData = aznew QMimeData();

            AzToolsFramework::EditorAssetMimeDataContainer mimeDataContainer;
            mimeDataContainer.AddEditorAsset(m_currentAssetID, m_currentAssetType);
            mimeDataContainer.AddToMimeData(newMimeData);

            clipboard->setMimeData(newMimeData);
        }
        else if (selectedItem == pasteAction)
        {
            if (canPasteFromClipboard)
            {
                SetCurrentAssetID(readId);
            }
        }
    }

    bool PropertyAssetCtrl::IsCorrectMimeData(const QMimeData* pData, AZ::Data::AssetId* pAssetId, AZ::Data::AssetType* pAssetType) const
    {
        if (pAssetId)
        {
            pAssetId->SetInvalid();
        }

        if (pAssetType)
        {
            (*pAssetType) = 0;
        }

        if (!pData)
        {
            return false;
        }

        if (pData->hasFormat(EditorAssetMimeDataContainer::GetMimeType()))
        {
            AzToolsFramework::EditorAssetMimeDataContainer cont;
            if (!cont.FromMimeData(pData))
            {
                return false;
            }

            if (cont.m_assets.empty())
            {
                return false;
            }

            AZ::Data::AssetId assetId = cont.m_assets[0].m_assetId;
            AZ::Data::AssetType assetType = cont.m_assets[0].m_assetType;

            if (assetType.IsNull())
            {
                return false;
            }

            if (!assetId.IsValid())
            {
                return false;
            }

            if (assetType == GetCurrentAssetType())
            {
                if (pAssetId)
                {
                    (*pAssetId) = assetId;
                }

                if (pAssetType)
                {
                    (*pAssetType) = assetType;
                }

                return true;
            }
        }
        if (pData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()) || pData->hasFormat(EditorAssetMimeDataContainer::GetMimeType()))
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetType assetType;

            if (pData->hasFormat(AssetBrowser::AssetBrowserEntry::GetMimeType()))
            {
                AZStd::vector<AssetBrowser::AssetBrowserEntry*> entries;
                if (!AssetBrowser::AssetBrowserEntry::FromMimeData(pData, entries))
                {
                    return false;
                }

                const AssetBrowser::ProductAssetBrowserEntry* product = nullptr;
                for (auto entry : entries)
                {
                    product = azrtti_cast<const AssetBrowser::ProductAssetBrowserEntry*>(entry);
                    if (!product)
                    {
                        AZStd::vector<const ProductAssetBrowserEntry*> products;
                        entry->GetChildren<ProductAssetBrowserEntry>(products);
                        if (products.size() > 0)
                        {
                            product = products[0];
                            if (product)
                            {
                                break;
                            }
                        }
                    }
                }

                if (!product)
                {
                    return false;
                }

                assetId = product->GetAssetId();
                assetType = product->GetAssetType();
            }
            else
            {
                AzToolsFramework::EditorAssetMimeDataContainer cont;
                if (!cont.FromMimeData(pData))
                {
                    return false;
                }

                if (cont.m_assets.empty())
                {
                    return false;
                }

                assetId = cont.m_assets[0].m_assetId;
                assetType = cont.m_assets[0].m_assetType;
            }

            if (assetType.IsNull())
            {
                return false;
            }

            if (!assetId.IsValid())
            {
                return false;
            }

            if (assetType == GetCurrentAssetType())
            {
                if (pAssetId)
                {
                    (*pAssetId) = assetId;
                }

                if (pAssetType)
                {
                    (*pAssetType) = assetType;
                }

                return true;
            }
        }
        else
        {
            // Support for generic URI sources (e.g. windows explorer).
            const QList<QUrl> urls = pData->urls();
            if (!urls.empty() && urls[0].isLocalFile())
            {
                AZStd::string filename = urls[0].toLocalFile().toUtf8().constData();
                EBUS_EVENT(AzFramework::ApplicationRequests::Bus, MakePathAssetRootRelative, filename);
                AZ::Data::AssetId assetId;
                EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, filename.c_str(), AZ::Data::s_invalidAssetType, false);

                // Verify extension.
                if (!m_optionalValidDragDropExtensions.isEmpty())
                {
                    QString fname(filename.c_str());

                    int dotIndex = fname.lastIndexOf('.');
                    if (dotIndex < 0)
                    {
                        return false;
                    }

                    QString extension = fname.mid(dotIndex);
                    if (m_optionalValidDragDropExtensions.indexOf(extension) < 0)
                    {
                        return false;
                    }
                }

                if (assetId.IsValid())
                {
                    if (pAssetId)
                    {
                        (*pAssetId) = assetId;
                    }

                    // For now we have to assume it's the same asset type. The catalog will need to validate this for us.
                    return true;
                }
            }
        }

        return false;
    }

    void PropertyAssetCtrl::ClearErrorButton()
    {
        if (m_errorButton)
        {
            layout()->removeWidget(m_errorButton);
            delete m_errorButton;
            m_errorButton = nullptr;
        }
    }

    void PropertyAssetCtrl::UpdateErrorButton(const AZStd::string& errorLog)
    {
        if (m_errorButton)
        {
            // If the button is already active, disconnect its pressed handler so we don't get multiple popups
            disconnect(m_errorButton, &QPushButton::pressed, this, 0);
        }
        else
        {
            // If error button already set, don't do it again
            m_errorButton = aznew QPushButton(this);
            m_errorButton->setFlat(true);
            m_errorButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            m_errorButton->setFixedSize(QSize(16, 16));
            m_errorButton->setMouseTracking(true);
            m_errorButton->setIcon(QIcon("Editor/Icons/PropertyEditor/error_icon.png"));
            m_errorButton->setToolTip("Show Errors");

            // Insert the error button after the asset label
            qobject_cast<QHBoxLayout*>(layout())->insertWidget(1, m_errorButton);
        }

        // Connect pressed to opening the error dialog
        // Must capture this for call to QObject::connect
        connect(m_errorButton, &QPushButton::pressed, this, [errorLog]() {
            // Create the dialog for the log panel, and set the layout
            QDialog* logDialog = new QDialog();
            logDialog->setMinimumSize(1024, 400);
            logDialog->setObjectName("Asset Errors");
            QHBoxLayout* pLayout = new QHBoxLayout(logDialog);
            logDialog->setLayout(pLayout);

            // Create the Generic Log Panel and place it into the QDialog
            AzToolsFramework::LogPanel::GenericLogPanel* logPanel = new AzToolsFramework::LogPanel::GenericLogPanel(logDialog);
            logDialog->layout()->addWidget(logPanel);

            // Give the log panel data to display
            logPanel->ParseData(errorLog.c_str(), errorLog.size());

            // The user can click the "reset button" to reset the tabs to default
            auto tabsResetFunction = [logPanel]() -> void
            {
                logPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("All output", "", ""));
                logPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Warnings/Errors Only", "", "", false, true, true, false));
            };

            // We call the above function to set the initial state to be the reset state, otherwise it would start blank.
            tabsResetFunction();

            // Connect the above function to when the user clicks the reset tabs button
            QObject::connect(logPanel, &AzToolsFramework::LogPanel::BaseLogPanel::TabsReset, logPanel, tabsResetFunction);
            // Connect to finished slot to delete the allocated dialog
            QObject::connect(logDialog, &QDialog::finished, [logDialog](int) { logDialog->deleteLater(); });

            // Show the dialog
            logDialog->adjustSize();
            logDialog->show();
        });
    }

    void PropertyAssetCtrl::SourceFileChanged(AZStd::string /*relativePath*/, AZStd::string /*scanFolder*/, AZ::Uuid sourceUUID)
    {
        if (GetCurrentAssetID().m_guid == sourceUUID)
        {
            ClearErrorButton();
        }
    }

    void PropertyAssetCtrl::SourceFileFailed(AZStd::string /*relativePath*/, AZStd::string /*scanFolder*/, AZ::Uuid sourceUUID)
    {
        if (GetCurrentAssetID().m_guid == sourceUUID)
        {
            UpdateAssetDisplay();
        }
    }

    void PropertyAssetCtrl::dragEnterEvent(QDragEnterEvent* event)
    {
        if (m_label->isEnabled() && IsCorrectMimeData(event->mimeData()))
        {
            m_label->setProperty("DropHighlight", true);
            m_label->style()->unpolish(m_label);
            m_label->style()->polish(m_label);
            event->acceptProposedAction();
        }
    }

    void PropertyAssetCtrl::dragLeaveEvent(QDragLeaveEvent* event)
    {
        (void)event;

        //do nothing if the label is disabled
        if (!m_label->isEnabled())
        {
            return;
        }

        m_label->setProperty("DropHighlight", QVariant());
        m_label->style()->unpolish(m_label);
        m_label->style()->polish(m_label);
    }

    void PropertyAssetCtrl::dropEvent(QDropEvent* event)
    {
        //do nothing if the label is disabled
        if (!m_label->isEnabled())
        {
            return;
        }

        AZ::Data::AssetId readId;
        AZ::Data::AssetType readType;
        if (IsCorrectMimeData(event->mimeData(), &readId, &readType))
        {
            if (readId.IsValid())
            {
                SetCurrentAssetID(readId);
            }
            event->acceptProposedAction();
        }

        m_label->setProperty("DropHighlight", QVariant());
        m_label->style()->unpolish(m_label);
        m_label->style()->polish(m_label);
    }

    void PropertyAssetCtrl::UpdateTabOrder()
    {
        setTabOrder(m_label, m_browseButton);
        setTabOrder(m_browseButton, m_clearButton);
    }

    bool PropertyAssetCtrl::eventFilter(QObject* obj, QEvent* event)
    {
        (void)obj;

        if (isEnabled())
        {
            if (event->type() == QEvent::MouseButtonDblClick)
            {
                // if its filled in with an asset, open the asset.
                if (m_currentAssetID.IsValid())
                {
                    bool someoneHandledIt = false;
                    AssetBrowser::AssetBrowserInteractionNotificationBus::Broadcast(&AssetBrowser::AssetBrowserInteractionNotifications::OpenAssetInAssociatedEditor, m_currentAssetID, someoneHandledIt);
                }
            }
        }

        return false;
    }

    PropertyAssetCtrl::~PropertyAssetCtrl()
    {
    }

    void PropertyAssetCtrl::OnEditButtonClicked()
    {
        if (m_editNotifyCallback)
        {
            AZ_Error("Asset Property", m_editNotifyTarget, "No notification target set for edit callback.");
            m_editNotifyCallback->Invoke(m_editNotifyTarget, GetCurrentAssetID(), GetCurrentAssetType());
            return;
        }
        else
        {
            // Show default asset editor (property grid dialog) if this asset type has edit reflection.
            AZ::SerializeContext* serializeContext = nullptr;
            EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
            if (serializeContext)
            {
                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(GetCurrentAssetType());
                if (classData && classData->m_editData)
                {
                    // if there is no file selected then open the asset editor and let them create one
                    AZ::Data::Asset<AZ::Data::AssetData> asset = GetCurrentAssetID().IsValid()? AZ::Data::AssetManager::Instance().GetAsset(GetCurrentAssetID(), GetCurrentAssetType(), true) : AZ::Data::Asset<AZ::Data::AssetData>();
                    QObject* rootOfAll = this;
                    while (QObject* nextParent = rootOfAll->parent())
                    {
                        rootOfAll = nextParent;
                    }

                    AssetEditor::AssetEditorRequestsBus::Broadcast(&AssetEditor::AssetEditorRequests::OpenAssetEditor, asset);
                    return;
                }
            }
        }

        QMessageBox::warning(GetActiveWindow(), tr("Unable to Edit Asset"),
            tr("No callback is provided and no associated editor could be found."), QMessageBox::Ok, QMessageBox::Ok);
    }

    void PropertyAssetCtrl::PopupAssetBrowser()
    {
        AZ_Assert(m_currentAssetType != AZ::Data::s_invalidAssetType, "No asset type was assigned.");

        // Request the AssetBrowser Dialog and set a type filter
        AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection(GetCurrentAssetType());
        selection.SetSelectedAssetId(m_currentAssetID);
        EditorRequests::Bus::Broadcast(&EditorRequests::BrowseForAssets, selection);
        if (selection.IsValid())
        {
            auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
            AZ_Assert(product, "Incorrect entry type selected. Expected product.");
            SetCurrentAssetID(product->GetAssetId());
        }
    }

    void PropertyAssetCtrl::ClearAsset()
    {
        SetCurrentAssetHint(AZStd::string());
        SetCurrentAssetID(AZ::Data::AssetId());
        EBUS_EVENT(ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, Refresh_EntireTree);
    }


    void PropertyAssetCtrl::SetCurrentAssetID(const AZ::Data::AssetId& newID)
    {
        m_currentAssetID = newID;

        // If the id is valid, connect to the asset system bus
        if (newID.IsValid())
        {
            AssetSystemBus::Handler::BusConnect();
        }
        // Otherwise, don't listen for events.
        else
        {
            AssetSystemBus::Handler::BusDisconnect();
        }

        UpdateAssetDisplay();
        emit OnAssetIDChanged(newID);
    }

    void PropertyAssetCtrl::SetCurrentAssetType(const AZ::Data::AssetType& newType)
    {
        m_currentAssetType = newType;
        UpdateAssetDisplay();
    }

    void PropertyAssetCtrl::SetCurrentAssetHint(const AZStd::string& hint)
    {
        m_currentAssetHint = hint;
    }

    void PropertyAssetCtrl::UpdateAssetDisplay()
    {
        if (m_currentAssetType == AZ::Data::s_invalidAssetType)
        {
            return;
        }

        AZ::Outcome<AssetSystem::JobInfoContainer> jobOutcome = AZ::Failure();
        AssetSystemJobRequestBus::BroadcastResult(jobOutcome, &AssetSystemJobRequestBus::Events::GetAssetJobsInfoByAssetID, GetCurrentAssetID(), false);

        if (jobOutcome.IsSuccess())
        {
            AZStd::string assetPath;
            AssetSystem::JobInfoContainer& jobs = jobOutcome.GetValue();

            // Get the asset relative path
            AssetSystem::JobStatus assetStatus = AssetSystem::JobStatus::Completed;

            if (!jobs.empty())
            {
                assetPath = jobs[0].m_sourceFile;

                AZStd::string errorLog;

                for (const auto& jobInfo : jobs)
                {
                    // If the job has failed, mark the asset as failed, and collect the log.
                    switch (jobInfo.m_status)
                    {
                    case AssetSystem::JobStatus::Failed:
                    case AssetSystem::JobStatus::Failed_InvalidSourceNameExceedsMaxLimit:
                        {
                            assetStatus = AssetSystem::JobStatus::Failed;

                            AZ::Outcome<AZStd::string> logOutcome = AZ::Failure();
                            AssetSystemJobRequestBus::BroadcastResult(logOutcome, &AssetSystemJobRequestBus::Events::GetJobLog, jobInfo.m_jobRunKey);
                            if (logOutcome.IsSuccess())
                            {
                                errorLog += logOutcome.TakeValue();
                                errorLog += '\n';
                            }
                        }
                        break;

                    // If the job is in progress, mark the asset as in progress
                    case AssetSystem::JobStatus::InProgress:
                        {
                            // Only mark asset in progress if it isn't already in progress, or marked as an error
                            if (assetStatus == AssetSystem::JobStatus::Completed)
                            {
                                assetStatus = AssetSystem::JobStatus::InProgress;
                            }
                        }
                        break;
                    }
                }

                switch (assetStatus)
                {
                // In case of failure, render failure icon
                case AssetSystem::JobStatus::Failed:
                    {
                        UpdateErrorButton(errorLog);
                    }
                    break;

                // In case of success, remove error elements
                case AssetSystem::JobStatus::Completed:
                    {
                        ClearErrorButton();
                    }
                    break;
                }
            }

            // Only change the asset name if the asset not found or there's no last known good name for it
            if (!assetPath.empty() && (assetStatus != AssetSystem::JobStatus::Completed || m_currentAssetHint.empty()))
            {
                m_currentAssetHint = assetPath;
            }
        }

        // Get the asset file name
        AZStd::string assetName = m_currentAssetHint;
        if (!m_currentAssetHint.empty())
        {
            AzFramework::StringFunc::Path::GetFileName(m_currentAssetHint.c_str(), assetName);
        }

        setToolTip(m_currentAssetHint.c_str());
        m_label->setText(assetName.c_str());
    }

    void PropertyAssetCtrl::SetEditButtonEnabled(bool enabled)
    {
        m_editButton->setEnabled(enabled);
    }

    void PropertyAssetCtrl::SetEditButtonVisible(bool visible)
    {
        m_editButton->setVisible(visible);
    }

    void PropertyAssetCtrl::SetEditButtonIcon(const QIcon& icon)
    {
        m_editButton->setIcon(icon);
    }

    void PropertyAssetCtrl::SetEditNotifyTarget(void* editNotifyTarget)
    {
        m_editNotifyTarget = editNotifyTarget;
    }

    void PropertyAssetCtrl::SetEditNotifyCallback(EditCallbackType* editNotifyCallback)
    {
        m_editNotifyCallback = editNotifyCallback;
    }

    void PropertyAssetCtrl::SetEditButtonTooltip(QString tooltip)
    {
        m_editButton->setToolTip(tooltip);
    }

    const AZ::Uuid& AssetPropertyHandlerDefault::GetHandledType() const
    {
        return AZ::GetAssetClassId();
    }

    QWidget* AssetPropertyHandlerDefault::CreateGUI(QWidget* pParent)
    {
        PropertyAssetCtrl* newCtrl = aznew PropertyAssetCtrl(pParent);
        connect(newCtrl, &PropertyAssetCtrl::OnAssetIDChanged, this, [ newCtrl ](AZ::Data::AssetId newAssetID)
            {
                (void)newAssetID;
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        return newCtrl;
    }

    void AssetPropertyHandlerDefault::ConsumeAttribute(PropertyAssetCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        (void)debugName;

        if (attrib == AZ_CRC("EditCallback", 0xb74f2ee1))
        {
            PropertyAssetCtrl::EditCallbackType* func = azdynamic_cast<PropertyAssetCtrl::EditCallbackType*>(attrValue->GetAttribute());
            if (func)
            {
                GUI->SetEditButtonVisible(true);
                GUI->SetEditNotifyCallback(func);
            }
            else
            {
                GUI->SetEditNotifyCallback(nullptr);
            }
        }
        else if (attrib == AZ_CRC("EditButton", 0x898c35dc))
        {
            GUI->SetEditButtonVisible(true);

            AZStd::string iconPath;
            attrValue->Read<AZStd::string>(iconPath);

            if (!iconPath.empty())
            {
                GUI->SetEditButtonIcon(QIcon(iconPath.c_str()));
            }
        }
        else if (attrib == AZ_CRC("EditDescription", 0x9b52634a))
        {
            AZStd::string buttonTooltip;
            if (attrValue->Read<AZStd::string>(buttonTooltip))
            {
                GUI->SetEditButtonTooltip(tr(buttonTooltip.c_str()));
            }
        }
    }

    void AssetPropertyHandlerDefault::WriteGUIValuesIntoProperty(size_t index, PropertyAssetCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (void)index;
        (void)node;

        if (!GUI->GetCurrentAssetID().IsValid())
        {
            instance = nullptr;
        }
        else
        {
            instance = property_t(GUI->GetCurrentAssetID(), GUI->GetCurrentAssetType(), GUI->GetCurrentAssetHint());
        }
    }

    bool AssetPropertyHandlerDefault::ReadValuesIntoGUI(size_t index, PropertyAssetCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (void)index;
        (void)node;

        GUI->blockSignals(true);

        AZ_Assert(node->GetElementMetadata()->m_genericClassInfo, "Property does not have element data.");
        AZ_Assert(node->GetElementMetadata()->m_genericClassInfo->GetNumTemplatedArguments() == 1, "Asset<> should have only 1 template parameter.");

        const AZ::Uuid& assetTypeId = node->GetElementMetadata()->m_genericClassInfo->GetTemplatedTypeId(0);

        GUI->SetCurrentAssetHint(instance.GetHint());
        GUI->SetCurrentAssetType(assetTypeId);
        GUI->SetCurrentAssetID(instance.GetId());
        GUI->SetEditNotifyTarget(node->GetParent()->GetInstance(0));

        GUI->blockSignals(false);
        return false;
    }

    QWidget* SimpleAssetPropertyHandlerDefault::CreateGUI(QWidget* pParent)
    {
        PropertyAssetCtrl* newCtrl = aznew PropertyAssetCtrl(pParent);

        connect(newCtrl, &PropertyAssetCtrl::OnAssetIDChanged, this, [newCtrl](AZ::Data::AssetId newAssetID)
            {
                (void)newAssetID;
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        return newCtrl;
    }

    void SimpleAssetPropertyHandlerDefault::ConsumeAttribute(PropertyAssetCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        Q_UNUSED(GUI);
        Q_UNUSED(attrib);
        Q_UNUSED(attrValue);
        Q_UNUSED(debugName);
    }

    void SimpleAssetPropertyHandlerDefault::WriteGUIValuesIntoProperty(size_t index, PropertyAssetCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (void)index;
        (void)node;

        AZStd::string assetPath;
        EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, GUI->GetCurrentAssetID());

        instance.SetAssetPath(assetPath.c_str());
    }

    bool SimpleAssetPropertyHandlerDefault::ReadValuesIntoGUI(size_t index, PropertyAssetCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (void)index;
        (void)node;

        GUI->blockSignals(true);

        GUI->SetCurrentAssetType(instance.GetAssetType());

        AZ::Data::AssetId assetId;
        if (!instance.GetAssetPath().empty())
        {
            EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, instance.GetAssetPath().c_str(), instance.GetAssetType(), true);
        }

        // Set the hint in case the asset is not able to be found by assetId
        GUI->SetCurrentAssetHint(instance.GetAssetPath());
        GUI->SetCurrentAssetID(assetId);

        GUI->blockSignals(false);
        return false;
    }

    void RegisterAssetPropertyHandler()
    {
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew AssetPropertyHandlerDefault());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew SimpleAssetPropertyHandlerDefault());
    }
}

#include <UI/PropertyEditor/PropertyAssetCtrl.moc>
