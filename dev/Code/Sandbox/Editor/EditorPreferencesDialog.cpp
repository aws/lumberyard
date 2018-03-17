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
#include "EditorPreferencesDialog.h"
#include "EditorPreferencesTreeWidgetItem.h"
#include <ui_EditorPreferencesDialog.h>
#include "SettingsManagerDialog.h"
#include "PreferencesStdPages.h"
#include "DisplaySettings.h"
#include "CryEditDoc.h"
#include "MainWindow.h"
#include "Controls/ConsoleSCB.h"
#include "EditorPreferencesPageGeneral.h"
#include "EditorPreferencesPageFiles.h"
#include "EditorPreferencesPageViewportGeneral.h"
#include "EditorPreferencesPageViewportGizmo.h"
#include "EditorPreferencesPageViewportMovement.h"
#include "EditorPreferencesPageViewportDebug.h"
#include "EditorPreferencesPageMannequinGeneral.h"
#include "EditorPreferencesPageExperimentalLighting.h"
#include "EditorPreferencesPageFlowGraphGeneral.h"
#include "EditorPreferencesPageFlowGraphColors.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#if defined(Q_OS_WIN)
#include <QtWinExtras/QtWin>
#endif
#include <QObject>

using namespace AzQtComponents;

EditorPreferencesDialog::EditorPreferencesDialog(QWidget* pParent)
    : QDialog(new WindowDecorationWrapper(WindowDecorationWrapper::OptionAutoAttach | WindowDecorationWrapper::OptionAutoTitleBarButtons, pParent))
    , ui(new Ui::EditorPreferencesDialog)
    , m_currentPageItem(nullptr)
{
    ui->setupUi(this);

    AZ::SerializeContext* serializeContext = nullptr;
    EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(serializeContext, "Serialization context not available");

    static bool bAlreadyRegistered = false;
    if (!bAlreadyRegistered)
    {
        bAlreadyRegistered = true;
        GetIEditor()->GetClassFactory()->RegisterClass(new CStdPreferencesClassDesc);

        if (serializeContext)
        {
            CEditorPreferencesPage_General::Reflect(*serializeContext);
            CEditorPreferencesPage_Files::Reflect(*serializeContext);
            CEditorPreferencesPage_ViewportGeneral::Reflect(*serializeContext);
            CEditorPreferencesPage_ViewportGizmo::Reflect(*serializeContext);
            CEditorPreferencesPage_ViewportMovement::Reflect(*serializeContext);
            CEditorPreferencesPage_ViewportDebug::Reflect(*serializeContext);
#ifdef ENABLE_LEGACY_ANIMATION
            CEditorPreferencesPage_MannequinGeneral::Reflect(*serializeContext);
#endif //ENABLE_LEGACY_ANIMATION
            CEditorPreferencesPage_ExperimentalLighting::Reflect(*serializeContext);
            CEditorPreferencesPage_FlowGraphGeneral::Reflect(*serializeContext);
            CEditorPreferencesPage_FlowGraphColors::Reflect(*serializeContext);
        }
    }

    ui->propertyEditor->SetAutoResizeLabels(true);
    ui->propertyEditor->Setup(serializeContext, this, true, 250);

    ui->pageTree->setColumnCount(1);
    connect(ui->pageTree, &QTreeWidget::currentItemChanged, this, &EditorPreferencesDialog::OnTreeCurrentItemChanged);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &EditorPreferencesDialog::OnAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &EditorPreferencesDialog::OnReject);
    connect(ui->MANAGE_BTN, &QPushButton::clicked, this, &EditorPreferencesDialog::OnManage);
}


EditorPreferencesDialog::~EditorPreferencesDialog()
{
}


void EditorPreferencesDialog::showEvent(QShowEvent* event)
{
    origAutoBackup.bEnabled = gSettings.autoBackupEnabled;
    origAutoBackup.nTime = gSettings.autoBackupTime;
    origAutoBackup.nRemindTime = gSettings.autoRemindTime;

    CreateImages();
    CreatePages();
    ui->pageTree->setCurrentItem(ui->pageTree->topLevelItem(0));
    QDialog::showEvent(event);
}


void EditorPreferencesDialog::OnTreeCurrentItemChanged()
{
    QTreeWidgetItem* currentItem = ui->pageTree->currentItem();

    if (currentItem->type() == EditorPreferencesTreeWidgetItem::EditorPreferencesPage)
    {
        EditorPreferencesTreeWidgetItem* currentPageItem = static_cast<EditorPreferencesTreeWidgetItem*>(currentItem);
        if (currentPageItem != m_currentPageItem)
        {
            SetActivePage(currentPageItem);
        }
    }
    else
    {
        if (m_currentPageItem == nullptr || m_currentPageItem->parent() != currentItem)
        {
            EditorPreferencesTreeWidgetItem* child = (EditorPreferencesTreeWidgetItem*)currentItem->child(0);
            SetActivePage(child);
        }
    }
}


void EditorPreferencesDialog::OnAccept()
{
    // Call on OK for all pages.
    QTreeWidgetItemIterator it(ui->pageTree);
    while (*it)
    {
        QTreeWidgetItem* item = *it;
        if (item->type() == EditorPreferencesTreeWidgetItem::EditorPreferencesPage)
        {
            EditorPreferencesTreeWidgetItem* pageItem = (EditorPreferencesTreeWidgetItem*)*it;
            pageItem->GetPreferencesPage()->OnApply();
        }
        ++it;
    }

    // Save settings.
    gSettings.Save();
    GetIEditor()->GetDisplaySettings()->SaveRegistry();

    if (GetIEditor()->GetDocument()->IsDocumentReady() && (
            origAutoBackup.bEnabled != gSettings.autoBackupEnabled ||
            origAutoBackup.nTime != gSettings.autoBackupTime ||
            origAutoBackup.nRemindTime != gSettings.autoRemindTime))
    {
        MainWindow::instance()->ResetAutoSaveTimers(true);
    }

    auto consoleWindow = CConsoleSCB::GetCreatedInstance();
    if (consoleWindow != nullptr)
    {
        consoleWindow->OnStyleSettingsChanged();
    }

    accept();
}

void EditorPreferencesDialog::OnReject()
{
    // QueryCancel for all pages.
    QTreeWidgetItemIterator it(ui->pageTree);
    while (*it)
    {
        QTreeWidgetItem* item = *it;
        if (item->type() == EditorPreferencesTreeWidgetItem::EditorPreferencesPage)
        {
            EditorPreferencesTreeWidgetItem* pageItem = (EditorPreferencesTreeWidgetItem*)*it;
            if (!pageItem->GetPreferencesPage()->OnQueryCancel())
            {
                return;
            }
        }
        ++it;
    }

    QTreeWidgetItemIterator cancelIt(ui->pageTree);
    while (*cancelIt)
    {
        QTreeWidgetItem* item = *cancelIt;
        if (item->type() == EditorPreferencesTreeWidgetItem::EditorPreferencesPage)
        {
            EditorPreferencesTreeWidgetItem* pageItem = (EditorPreferencesTreeWidgetItem*)*cancelIt;
            pageItem->GetPreferencesPage()->OnCancel();
        }
        ++cancelIt;
    }


    reject();
}

void EditorPreferencesDialog::OnManage()
{
    GetIEditor()->OpenView(LyViewPane::EditorSettingsManager);
    OnAccept();
}


void EditorPreferencesDialog::SetActivePage(EditorPreferencesTreeWidgetItem* pageItem)
{
    if (m_currentPageItem)
    {
        m_currentPageItem->SetActivePage(false);
    }

    m_currentPageItem = pageItem;
    m_currentPageItem->SetActivePage(true);

    ui->propertyEditor->ClearInstances();
    IPreferencesPage* instance = m_currentPageItem->GetPreferencesPage();
    const AZ::Uuid& classId = AZ::SerializeTypeInfo<IPreferencesPage>::GetUuid(instance);
    ui->propertyEditor->AddInstance(instance, classId);
    ui->propertyEditor->InvalidateAll();
    ui->propertyEditor->show();
    ui->propertyEditor->ExpandAll();
}


void EditorPreferencesDialog::CreateImages()
{
    m_selectedPixmap = QPixmap(":/res/Preferences_00.png");
    m_unSelectedPixmap = QPixmap(":/res/Preferences_01.png");
}

void EditorPreferencesDialog::CreatePages()
{
    std::vector<IClassDesc*> classes;
    GetIEditor()->GetClassFactory()->GetClassesBySystemID(ESYSTEM_CLASS_PREFERENCE_PAGE, classes);
    for (int i = 0; i < classes.size(); i++)
    {
        auto pUnknown = classes[i];

        IPreferencesPageCreator* pPageCreator = 0;
        if (FAILED(pUnknown->QueryInterface(&pPageCreator)))
        {
            continue;
        }

        int numPages = pPageCreator->GetPagesCount();
        for (int pindex = 0; pindex < numPages; pindex++)
        {
            IPreferencesPage* pPage = pPageCreator->CreateEditorPreferencesPage(pindex);
            if (!pPage)
            {
                continue;
            }

            QTreeWidgetItem* category = nullptr;
            QList<QTreeWidgetItem*> items = ui->pageTree->findItems(pPage->GetCategory(), Qt::MatchExactly);
            if (items.size() > 0)
            {
                category = items[0];
            }
            else
            {
                category = new QTreeWidgetItem(QStringList(pPage->GetCategory()));
                ui->pageTree->addTopLevelItem(category);
            }

            category->addChild(new EditorPreferencesTreeWidgetItem(pPage, m_selectedPixmap, m_unSelectedPixmap));
            category->setExpanded(true);
        }
    }
}

#include <EditorPreferencesDialog.moc>