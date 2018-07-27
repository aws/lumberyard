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
#include "TransitionBrowser.h"
#include "PreviewerPage.h"

#include "../FragmentEditor.h"
#include "../MannequinBase.h"

#include "../MannTagEditorDialog.h"
#include "../MannequinDialog.h"
#include "../SequencerDopeSheetBase.h"
#include "../MannTransitionPicker.h"
#include "../MannTransitionSettings.h"
#include "StringUtils.h"

#include "StringDlg.h"

#include <IGameFramework.h>
#include <ICryMannequinEditor.h>

#include <Mannequin/Controls/ui_TransitionBrowser.h>
#include "QtUtilWin.h"

#include <QMessageBox>
#include <QResizeEvent>

static const QString TRANSITION_DEFAULT_FRAGMENT_ID_FILTER_TEXT("<FragmentID Filter>");
static const QString TRANSITION_DEFAULT_TAG_FILTER_TEXT("<Tag Filter>");
static const QString TRANSITION_DEFAULT_ANIM_CLIP_FILTER_TEXT("<Anim Filter>");

CTransitionBrowser::CTransitionBrowser(CTransitionEditorPage& transitionEditorPage, QWidget* pParent)
    : QWidget(pParent)
    , m_animDB(NULL)
    , m_transitionEditorPage(transitionEditorPage)
    , m_context(NULL)
    , m_scopeContextDataIdx(0)
    , m_filterAnimClipFrom(TRANSITION_DEFAULT_ANIM_CLIP_FILTER_TEXT)
    , m_filterAnimClipTo(TRANSITION_DEFAULT_ANIM_CLIP_FILTER_TEXT)
{
    OnInitDialog();
}

CTransitionBrowser::~CTransitionBrowser()
{
}

BOOL CTransitionBrowser::OnInitDialog()
{
    ui.reset(new Ui::CTransitionBrowser);

    ui->setupUi(this);

    ui->treeWidget->SetBrowser(this);
    connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &CTransitionBrowser::OnSelectionChanged);
    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, this, &CTransitionBrowser::OnItemDoubleClicked);

    ui->FILTER_TAGS_FROM->setText(TRANSITION_DEFAULT_TAG_FILTER_TEXT);
    ui->FILTER_TAGS_FROM->selectAll();
    ui->FILTER_FRAGMENTS_FROM->setText(TRANSITION_DEFAULT_FRAGMENT_ID_FILTER_TEXT);
    ui->FILTER_FRAGMENTS_FROM->selectAll();
    ui->FILTER_ANIM_CLIP_FROM->setText(TRANSITION_DEFAULT_ANIM_CLIP_FILTER_TEXT);
    ui->FILTER_ANIM_CLIP_FROM->selectAll();

    ui->FILTER_TAGS_TO->setText(TRANSITION_DEFAULT_TAG_FILTER_TEXT);
    ui->FILTER_TAGS_TO->selectAll();
    ui->FILTER_FRAGMENTS_TO->setText(TRANSITION_DEFAULT_FRAGMENT_ID_FILTER_TEXT);
    ui->FILTER_FRAGMENTS_TO->selectAll();
    ui->FILTER_ANIM_CLIP_TO->setText(TRANSITION_DEFAULT_ANIM_CLIP_FILTER_TEXT);
    ui->FILTER_ANIM_CLIP_TO->selectAll();

    connect(ui->FILTER_TAGS_FROM, &QLineEdit::textChanged, this, &CTransitionBrowser::OnChangeFilterTagsFrom);
    connect(ui->FILTER_FRAGMENTS_FROM, &QLineEdit::textChanged, this, &CTransitionBrowser::OnChangeFilterFragmentsFrom);
    connect(ui->FILTER_ANIM_CLIP_FROM, &QLineEdit::textChanged, this, &CTransitionBrowser::OnChangeFilterAnimClipFrom);
    connect(ui->FILTER_TAGS_TO, &QLineEdit::textChanged, this, &CTransitionBrowser::OnChangeFilterTagsTo);
    connect(ui->FILTER_FRAGMENTS_TO, &QLineEdit::textChanged, this, &CTransitionBrowser::OnChangeFilterFragmentsTo);
    connect(ui->FILTER_ANIM_CLIP_TO, &QLineEdit::textChanged, this, &CTransitionBrowser::OnChangeFilterAnimClipTo);

    connect(ui->BLEND_OPEN, &QToolButton::clicked, this, &CTransitionBrowser::OnSwapFragmentFilters);
    connect(ui->SWAP_TAG_FILTER, &QToolButton::clicked, this, &CTransitionBrowser::OnSwapTagFilters);
    connect(ui->SWAP_ANIM_CLIP_FILTER, &QToolButton::clicked, this, &CTransitionBrowser::OnSwapAnimClipFilters);

    connect(ui->NEW, &QPushButton::clicked, this, &CTransitionBrowser::OnNewBtn);
    connect(ui->EDIT, &QPushButton::clicked, this, &CTransitionBrowser::OnEditBtn);
    connect(ui->COPY, &QPushButton::clicked, this, &CTransitionBrowser::OnCopyBtn);
    connect(ui->BLEND_OPEN, &QPushButton::clicked, this, &CTransitionBrowser::OnOpenBtn);
    connect(ui->DELETEBTN, &QPushButton::clicked, this, &CTransitionBrowser::OnDeleteBtn);

    Refresh();

    return true;
}

void CTransitionBrowser::slotVisibilityChanged(bool visible)
{
    if (!visible)
    {
        return;
    }

    if (auto pMannequinDialog = CMannequinDialog::GetCurrentInstance())
    {
        if (!pMannequinDialog->IsPaneSelected<CPreviewerPage*>())
        {
            pMannequinDialog->ShowPane<CTransitionEditorPage*>();
        }
    }
}

void CTransitionBrowser::SetContext(SMannequinContexts& context)
{
    m_context = &context;
    m_scopeContextDataIdx = -1;

    ui->CONTEXT->clear();
    const uint32 numContexts = context.m_contextData.size();
    for (uint32 i = 0; i < numContexts; i++)
    {
        ui->CONTEXT->addItem(context.m_contextData[i].name);
    }
    ui->CONTEXT->setCurrentIndex(0);

    if (context.m_controllerDef)
    {
        SetScopeContextDataIdx(0);
    }
}

void CTransitionBrowser::SetDatabase(IAnimationDatabase* animDB)
{
    if (m_animDB != animDB)
    {
        m_animDB = animDB;

        Refresh();
    }
}

void CTransitionBrowser::OnSelectionChanged()
{
    UpdateButtonStatus();
}

void CTransitionBrowser::OnItemDoubleClicked()
{
    OpenSelected();
}

void CTransitionBrowser::Refresh()
{
    CTransitionBrowserTreeCtrl::SPresentationState treePresentationState;
    ui->treeWidget->SavePresentationState(OUT treePresentationState);

    if (m_animDB)
    {
        ui->treeWidget->clear();
        ui->treeWidget->setColumnCount(2);
        typedef std::vector<std::pair<CTransitionBrowserRecord*, CTransitionBrowserRecord*> > TTreeEntries;
        TTreeEntries treeEntries;

        SEditorFragmentBlendID::TEditorFragmentBlendIDArray blendIDs;
        MannUtils::GetMannequinEditorManager().GetFragmentBlends(m_animDB, blendIDs);

        for (SEditorFragmentBlendID::TEditorFragmentBlendIDArray::const_iterator iterBlend = blendIDs.begin(), end = blendIDs.end(); iterBlend != end; ++iterBlend)
        {
            bool bPushBranch = false;
            TTreeEntries branchEntries;
            SEditorFragmentBlendVariant::TEditorFragmentBlendVariantArray variants;
            MannUtils::GetMannequinEditorManager().GetFragmentBlendVariants(m_animDB, (*iterBlend).fragFrom, (*iterBlend).fragTo, variants);

            // get the fragment names
            const CTagDefinition& rFragDef = m_animDB->GetFragmentDefs();
            const QString cFragFrom = (TAG_ID_INVALID == (*iterBlend).fragFrom) ? "<Any Fragment>" : rFragDef.GetTagName((*iterBlend).fragFrom);
            const QString cFragTo = (TAG_ID_INVALID == (*iterBlend).fragTo) ? "<Any Fragment>" : rFragDef.GetTagName((*iterBlend).fragTo);

            // if the filter is valid and appears in the fragment name then let it in, otherwise skip it
            const QString cFragFromLower = cFragFrom.toLower();
            if (m_filterFragmentIDTextFrom[0] != 0 && !cFragFromLower.contains(m_filterFragmentIDTextFrom))
            {
                continue;
            }

            const QString cFragToLower = cFragTo.toLower();
            if (m_filterFragmentIDTextTo[0] != 0 && !cFragToLower.contains(m_filterFragmentIDTextTo))
            {
                continue;
            }

            // add to the tree
            CTransitionBrowserRecord* pRecord = new CTransitionBrowserRecord(cFragFrom, cFragTo);
            CTransitionBrowserRecord* pParentRecord = nullptr;
            branchEntries.push_back(std::make_pair(pRecord, pParentRecord));
            pParentRecord = pRecord;

            for (SEditorFragmentBlendVariant::TEditorFragmentBlendVariantArray::const_iterator iterVariant = variants.begin(), end = variants.end(); iterVariant != end; ++iterVariant)
            {
                const uint32 numBlends = m_animDB->GetNumBlends(
                        (*iterBlend).fragFrom,
                        (*iterBlend).fragTo,
                        (*iterVariant).tagsFrom,
                        (*iterVariant).tagsTo);

                // get the tag names
                QString fromBuffer;
                QString toBuffer;

                MannUtils::FlagsToTagList(fromBuffer, (*iterVariant).tagsFrom, (*iterBlend).fragFrom, (*m_context->m_controllerDef), "<ANY Tag>");
                MannUtils::FlagsToTagList(toBuffer, (*iterVariant).tagsTo, (*iterBlend).fragTo, (*m_context->m_controllerDef), "<ANY Tag>");

                // filter out by the tags now
                bool include = true;
                const size_t numFromFilters = m_filtersFrom.size();
                if (numFromFilters > 0)
                {
                    QString sTagState(fromBuffer.toLower());
                    for (uint32 f = 0; f < numFromFilters; f++)
                    {
                        if (!sTagState.contains(m_filtersFrom[f]))
                        {
                            include = false;
                            break;
                        }
                    }
                }
                const size_t numToFilters = m_filtersTo.size();
                if (numToFilters > 0 && include)
                {
                    QString sTagState(toBuffer.toLower());
                    for (uint32 f = 0; f < numToFilters; f++)
                    {
                        if (!sTagState.contains(m_filtersTo[f]))
                        {
                            include = false;
                            break;
                        }
                    }
                }
                if (!include)
                {
                    continue;
                }

                // Filter by anim clip
                bool bCheckTo = m_filterAnimClipTo.length() > 0 && m_filterAnimClipTo[0] != TRANSITION_DEFAULT_ANIM_CLIP_FILTER_TEXT[0];
                bool bCheckFrom = m_filterAnimClipFrom.length() > 0 && m_filterAnimClipFrom[0] != TRANSITION_DEFAULT_ANIM_CLIP_FILTER_TEXT[0];

                bool bMatchTo = bCheckTo && FilterByAnimClip((*iterBlend).fragTo, (*iterVariant).tagsTo, m_filterAnimClipTo);
                bool bMatchFrom = bCheckFrom && FilterByAnimClip((*iterBlend).fragFrom, (*iterVariant).tagsFrom, m_filterAnimClipFrom);

                bool bCheckToFailed = bCheckTo && !bMatchTo;
                bool bCheckFromFailed = bCheckFrom && !bMatchFrom;

                if (bCheckToFailed || bCheckFromFailed)
                {
                    // Anim clip filter was specified, but didn't match anything in this fragment,
                    // so don't add fragment to tree hierarchy.
                    continue;
                }

                // add to the tree
                CTransitionBrowserRecord* pTSRecord = new CTransitionBrowserRecord(fromBuffer, toBuffer);
                CTransitionBrowserRecord* pParentTSRecord = nullptr;
                branchEntries.push_back(std::make_pair(pTSRecord, pParentRecord));
                pParentTSRecord = pTSRecord;

                for (int blendIdx = 0; blendIdx < numBlends; ++blendIdx)
                {
                    const SFragmentBlend* pBlend = m_animDB->GetBlend(
                            (*iterBlend).fragFrom,
                            (*iterBlend).fragTo,
                            (*iterVariant).tagsFrom,
                            (*iterVariant).tagsTo,
                            blendIdx);

                    TTransitionID transitionID(
                        (*iterBlend).fragFrom,
                        (*iterBlend).fragTo,
                        (*iterVariant).tagsFrom,
                        (*iterVariant).tagsTo,
                        pBlend->uid);

                    CTransitionBrowserRecord* pBlendRecord = new CTransitionBrowserRecord(transitionID, pBlend->selectTime);

                    branchEntries.push_back(std::make_pair(pBlendRecord, pParentTSRecord));
                }

                if (numBlends > 0)
                {
                    bPushBranch = true;
                }
            }
            if (bPushBranch)
            {
                bPushBranch = true;
                for (TTreeEntries::iterator iterTE = branchEntries.begin(), end = branchEntries.end(); iterTE != end; ++iterTE)
                {
                    treeEntries.push_back((*iterTE));
                }
            }
            branchEntries.clear();
        }

        for (TTreeEntries::iterator iterTE = treeEntries.begin(), end = treeEntries.end(); iterTE != end; ++iterTE)
        {
            if ((*iterTE).second)
            {
                (*iterTE).second->addChild((*iterTE).first);
            }
            else
            {
                ui->treeWidget->addTopLevelItem((*iterTE).first);
            }
        }
    }

    ui->treeWidget->RestorePresentationState(treePresentationState);

    // Make sure opened record is visible and bold
    // do NOT unselect in transition editor
    if (HasOpenedRecord())
    {
        CTransitionBrowserRecord* pRecord = ui->treeWidget->FindRecordForTransition(m_openedRecordDataCollection[0].m_transitionID);
        if (pRecord)
        {
            ui->treeWidget->scrollToItem(pRecord);
        }

        ui->treeWidget->SetBold(m_openedRecordDataCollection, TRUE);
    }

    UpdateButtonStatus();
}

void CTransitionBrowserTreeCtrl::SetBoldRecursive(QTreeWidgetItem* pRecords, const TTransitionBrowserRecordDataCollection& recordDataCollection, const BOOL bBold)
{
    if (!pRecords)
    {
        return;
    }

    for (int recordIndex = 0; recordIndex < pRecords->childCount(); recordIndex++) // GetCount changes whenever a row gets expanded!
    {
        CTransitionBrowserRecord* pRecord = (CTransitionBrowserRecord*) pRecords->child(recordIndex);

        bool match = false;
        if ( // Incredibly slow! Use map for example to optimize this
            std::find(recordDataCollection.begin(), recordDataCollection.end(), pRecord->GetRecordData()) != recordDataCollection.end())
        {
            for (int itemIndex = 0; itemIndex < pRecord->columnCount(); ++itemIndex)
            {
                QFont f = pRecord->font(itemIndex);
                f.setBold(bBold);
                pRecord->setFont(itemIndex, f);
            }
            match = true;
        }

        if (match)
        {
            SetBoldRecursive(pRecord, recordDataCollection, bBold);
        }
    }
}

void CTransitionBrowserTreeCtrl::SetBold(const TTransitionBrowserRecordDataCollection& recordDataCollection, const BOOL bBold)
{
    SetBoldRecursive(invisibleRootItem(), recordDataCollection, bBold);
}

void GetRecordDataCollectionRecursively(CTransitionBrowserRecord* pRecord, OUT TTransitionBrowserRecordDataCollection& recordDataCollection)
{
    recordDataCollection.clear();
    while (pRecord)
    {
        recordDataCollection.push_back(pRecord->GetRecordData());
        pRecord = (CTransitionBrowserRecord*)pRecord->parent();
    }
}

void SetBoldIncludingParents(QTreeWidgetItem* pRecord, BOOL bBold = TRUE)
{
    while (pRecord)
    {
        for (int i = 0; i < pRecord->childCount(); ++i)
        {
            QTreeWidgetItem* pItem = pRecord->child(i);
            if (pItem)
            {
                for (int colIndex = 0; colIndex < pItem->columnCount(); ++colIndex)
                {
                    QFont f = pItem->font(colIndex);
                    f.setBold(bBold);
                    pItem->setFont(colIndex, f);
                }
            }
        }
        pRecord = pRecord->parent();
    }
}

void CTransitionBrowser::SetOpenedRecord(CTransitionBrowserRecord* pRecordToOpen, const TTransitionID& pickedTransitionID)
{
    if (HasOpenedRecord())
    {
        // Unselect currently opened record
        ui->treeWidget->SetBold(m_openedRecordDataCollection, FALSE);
    }

    if (NULL != pRecordToOpen)
    {
        GetRecordDataCollectionRecursively(pRecordToOpen, m_openedRecordDataCollection);
        ui->treeWidget->SetBold(m_openedRecordDataCollection, TRUE);
        ui->treeWidget->scrollToItem(pRecordToOpen);
    }
    else
    {
        m_openedRecordDataCollection.clear();
    }
    ui->treeWidget->update();

    // Should have fetched enough data to begin displaying/building the sequence now.
    if (HasOpenedRecord())
    {
        m_openedTransitionID = pickedTransitionID;
    }
    else
    {
        m_openedTransitionID = TTransitionID();
    }

    m_transitionEditorPage.LoadSequence(m_scopeContextDataIdx, m_openedTransitionID.fromID, m_openedTransitionID.toID, m_openedTransitionID.fromFTS, m_openedTransitionID.toFTS, m_openedTransitionID.uid);
    CMannequinDialog::GetCurrentInstance()->ShowPane<CTransitionEditorPage*>();
}

void CTransitionBrowser::OnChangeContext()
{
    SetScopeContextDataIdx(ui->CONTEXT->currentIndex());
}

void CTransitionBrowser::SetScopeContextDataIdx(int scopeContextDataIdx)
{
    const bool rangeValid = (scopeContextDataIdx >= 0) && (scopeContextDataIdx < m_context->m_contextData.size());
    assert(rangeValid);

    if ((scopeContextDataIdx != m_scopeContextDataIdx) && rangeValid)
    {
        m_scopeContextDataIdx = scopeContextDataIdx;

        SetDatabase(m_context->m_contextData[scopeContextDataIdx].database);

        ui->CONTEXT->setCurrentIndex(scopeContextDataIdx);
    }
}

void CTransitionBrowser::OnChangeFilterTagsFrom(const QString& filter)
{
    if (!filter.isEmpty() && filter[0] != TRANSITION_DEFAULT_TAG_FILTER_TEXT[0] && filter != m_filterTextFrom)
    {
        m_filterTextFrom = filter;
        UpdateTags(m_filtersFrom, m_filterTextFrom);

        Refresh();

        ui->FILTER_TAGS_FROM->setFocus();
    }

    if (filter.isEmpty())
    {
        ui->FILTER_TAGS_FROM->setText(TRANSITION_DEFAULT_TAG_FILTER_TEXT);
        ui->FILTER_TAGS_FROM->selectAll();
    }
}

void CTransitionBrowser::OnChangeFilterFragmentsFrom(const QString& filter)
{
    if (!filter.isEmpty() && filter[0] != TRANSITION_DEFAULT_FRAGMENT_ID_FILTER_TEXT[0] && filter != m_filterFragmentIDTextFrom)
    {
        m_filterFragmentIDTextFrom = filter.toLower();

        Refresh();

        ui->FILTER_FRAGMENTS_FROM->setFocus();
    }

    if (filter.isEmpty())
    {
        ui->FILTER_FRAGMENTS_FROM->setText(TRANSITION_DEFAULT_FRAGMENT_ID_FILTER_TEXT);
        ui->FILTER_FRAGMENTS_FROM->selectAll();
    }
}

void CTransitionBrowser::OnChangeFilterAnimClipFrom(const QString& filter)
{
    if (!filter.isEmpty() && filter[0] != TRANSITION_DEFAULT_ANIM_CLIP_FILTER_TEXT[0] && filter != m_filterAnimClipFrom)
    {
        m_filterAnimClipFrom = filter.toLower();

        Refresh();
        ui->FILTER_ANIM_CLIP_FROM->setFocus();
    }

    if (filter.isEmpty())
    {
        ui->FILTER_ANIM_CLIP_FROM->setText(TRANSITION_DEFAULT_ANIM_CLIP_FILTER_TEXT);
        ui->FILTER_ANIM_CLIP_FROM->selectAll();
    }
}

void CTransitionBrowser::OnChangeFilterTagsTo(const QString& filter)
{
    if (!filter.isEmpty() && filter[0] != TRANSITION_DEFAULT_TAG_FILTER_TEXT[0] && filter != m_filterTextTo)
    {
        m_filterTextTo = filter;
        UpdateTags(m_filtersTo, m_filterTextTo);

        Refresh();

        ui->FILTER_TAGS_TO->setFocus();
    }

    if (filter.isEmpty())
    {
        ui->FILTER_TAGS_TO->setText(TRANSITION_DEFAULT_TAG_FILTER_TEXT);
        ui->FILTER_TAGS_TO->selectAll();
    }
}

void CTransitionBrowser::OnChangeFilterFragmentsTo(const QString& filter)
{
    if (!filter.isEmpty() && filter[0] != TRANSITION_DEFAULT_FRAGMENT_ID_FILTER_TEXT[0] && filter != m_filterFragmentIDTextTo)
    {
        m_filterFragmentIDTextTo = filter.toLower();

        Refresh();

        ui->FILTER_FRAGMENTS_TO->setFocus();
    }

    if (filter.isEmpty())
    {
        ui->FILTER_FRAGMENTS_TO->setText(TRANSITION_DEFAULT_FRAGMENT_ID_FILTER_TEXT);
        ui->FILTER_FRAGMENTS_TO->selectAll();
    }
}

void CTransitionBrowser::OnChangeFilterAnimClipTo(const QString& filter)
{
    if (!filter.isEmpty() && filter[0] != TRANSITION_DEFAULT_ANIM_CLIP_FILTER_TEXT[0] && filter != m_filterAnimClipTo)
    {
        m_filterAnimClipTo = filter.toLower();

        Refresh();
        ui->FILTER_ANIM_CLIP_TO->setFocus();
    }

    if (filter.isEmpty())
    {
        ui->FILTER_ANIM_CLIP_TO->setText(TRANSITION_DEFAULT_ANIM_CLIP_FILTER_TEXT);
        ui->FILTER_ANIM_CLIP_TO->selectAll();
    }
}

bool CTransitionBrowser::DeleteTransition(const TTransitionID& transitionToDelete)
{
    bool hadDeletedTransitionOpen = false;
    if (HasOpenedRecord())
    {
        CTransitionBrowserRecord::SRecordData& openedRecordData = m_openedRecordDataCollection[0]; // first recorddata is the opened transition (the others are the folders)
        assert(!openedRecordData.m_bFolder);

        if (transitionToDelete == openedRecordData.m_transitionID)
        {
            // the currently opened transitionID has been deleted, close it
            hadDeletedTransitionOpen = true;
            SetOpenedRecord(NULL, TTransitionID());
        }
    }

    MannUtils::GetMannequinEditorManager().DeleteBlend(m_animDB, transitionToDelete.fromID, transitionToDelete.toID, transitionToDelete.fromFTS, transitionToDelete.toFTS, transitionToDelete.uid);

    return hadDeletedTransitionOpen;
}

void CTransitionBrowser::DeleteSelected()
{
    TTransitionID transitionID;
    if (GetSelectedTransition(transitionID))
    {
        const int result = QMessageBox::question(this, "Delete Transition", "Are you sure you want to delete the selected transition?");
        if (QMessageBox::Yes != result)
        {
            return;
        }

        DeleteTransition(transitionID);

        Refresh();
    }
}

void CTransitionBrowser::OnEditBtn()
{
    EditSelected();
}

void CTransitionBrowser::OnCopyBtn()
{
    CopySelected();
}

void CTransitionBrowser::OnOpenBtn()
{
    OpenSelected();
}

void CTransitionBrowser::OnDeleteBtn()
{
    DeleteSelected();
}

void CTransitionBrowser::OnNewBtn()
{
    TTransitionID selectedTransitionID;
    SFragmentBlend blendToAdd;
    bool hasTransitionSelected = GetSelectedTransition(selectedTransitionID);

    TTransitionID transitionID(selectedTransitionID);
    if (ShowTransitionSettingsDialog(transitionID, "New Transition"))
    {
        if (hasTransitionSelected && transitionID.IsSameBlendVariant(selectedTransitionID))
        {
            // "New" clones an existing blend, but only if we didn't change the variant
            const SFragmentBlend* pCurBlend = m_animDB->GetBlend(transitionID.fromID, transitionID.toID, transitionID.fromFTS, transitionID.toFTS, transitionID.uid);
            const SFragmentBlendUid newUid = blendToAdd.uid;
            blendToAdd = *pCurBlend;
            blendToAdd.uid = newUid;
        }
        else
        {
            static CFragment fragmentNew;
            blendToAdd.pFragment = &fragmentNew; // we need a valid CFragment because AddBlend below copies the contents of pFragment
        }

        transitionID.uid = MannUtils::GetMannequinEditorManager().AddBlend(m_animDB, transitionID.fromID, transitionID.toID, transitionID.fromFTS, transitionID.toFTS, blendToAdd);
        Refresh();
        SelectAndOpenRecord(transitionID);
    }
}


void CTransitionBrowser::OnSwapFragmentFilters()
{
    QString fragTo = m_filterFragmentIDTextTo;
    QString fragFrom = m_filterFragmentIDTextFrom;

    m_filterFragmentIDTextFrom = fragTo;
    m_filterFragmentIDTextTo = fragFrom;

    UpdateFilters();
}

void CTransitionBrowser::OnSwapTagFilters()
{
    QString tagTo = m_filterTextTo;
    QString tagFrom = m_filterTextFrom;

    m_filterTextFrom = tagTo;
    m_filterTextTo = tagFrom;

    UpdateFilters();
}

void CTransitionBrowser::OnSwapAnimClipFilters()
{
    std::swap(m_filterAnimClipTo, m_filterAnimClipFrom);

    UpdateFilters();
}

void CTransitionBrowser::SelectAndOpenRecord(const TTransitionID& transitionID)
{
    ui->treeWidget->SelectItem(transitionID);
    OpenSelectedTransition(transitionID);
}

CTransitionBrowserRecord* CTransitionBrowser::GetSelectedRecord()
{
    CTransitionBrowserRecord* pRecord = nullptr;
    QList<QTreeWidgetItem*> selection = ui->treeWidget->selectedItems();
    if (selection.size())
    {
        pRecord = (CTransitionBrowserRecord*) selection.front();
    }
    return pRecord;
}

bool CTransitionBrowser::GetSelectedTransition(TTransitionID& outTransitionID)
{
    const CTransitionBrowserRecord* pRecord = GetSelectedRecord();

    if (pRecord && !pRecord->IsFolder() && ui->treeWidget->selectedItems().size() <= 1)
    {
        outTransitionID = pRecord->GetTransitionID();
        return true;
    }
    return false;
}

void CTransitionBrowser::OpenSelected()
{
    TTransitionID transitionID;
    if (GetSelectedTransition(transitionID))
    {
        OpenSelectedTransition(transitionID);
    }
}

void CTransitionBrowser::OpenSelectedTransition(const TTransitionID& transitionID)
{
    // if it has no FragmentIDs, or is missing one of them, then launch the FragmentIDPicker
    // or if it has no tags, or is missing one set of them, then it needs to launch the TagStatePicker
    FragmentID lclFromID = transitionID.fromID;
    FragmentID lclToID = transitionID.toID;
    SFragTagState fromTags = transitionID.fromFTS;
    SFragTagState toTags = transitionID.toFTS;

    // Only show a dialog when necessary to fill out incomplete information
    if (lclFromID == TAG_ID_INVALID || lclToID == TAG_ID_INVALID)
    {
        CMannTransitionPickerDlg transitionInfoDlg(lclFromID, lclToID, fromTags, toTags);
        // on return it *should* be ok to call LoadSequence with the modified parameters the user has picked.
        if (transitionInfoDlg.exec() == QDialog::Rejected)
        {
            // abort loading
            return;
        }
    }

    SetOpenedRecord(GetSelectedRecord(), TTransitionID(lclFromID, lclToID, fromTags, toTags, transitionID.uid));
}

bool CTransitionBrowser::ShowTransitionSettingsDialog(TTransitionID& inoutTransitionID, const QString& windowCaption)
{
    CMannTransitionSettingsDlg transitionInfoDlg(windowCaption, inoutTransitionID.fromID, inoutTransitionID.toID, inoutTransitionID.fromFTS, inoutTransitionID.toFTS);
    // on return it *should* be ok to call LoadSequence with the modified parameters the user has picked.
    if (CMannTransitionSettingsDlg::Accepted != transitionInfoDlg.exec())
    {
        // abort loading
        return false;
    }
    return true;
}

void CTransitionBrowser::EditSelected()
{
    TTransitionID transitionID;
    if (GetSelectedTransition(transitionID))
    {
        const TTransitionID oldTransitionID(transitionID);
        if (ShowTransitionSettingsDialog(transitionID, "Edit Transition"))
        {
            // If they've selected the same variant, then drop out
            if (oldTransitionID.IsSameBlendVariant(transitionID))
            {
                return;
            }

            // Get the old blend
            SFragmentBlend blendToMove;
            MannUtils::GetMannequinEditorManager().GetFragmentBlend(m_animDB, oldTransitionID.fromID, oldTransitionID.toID, oldTransitionID.fromFTS, oldTransitionID.toFTS, oldTransitionID.uid, blendToMove);

            // And add it as a new one
            transitionID.uid = MannUtils::GetMannequinEditorManager().AddBlend(m_animDB, transitionID.fromID, transitionID.toID, transitionID.fromFTS, transitionID.toFTS, blendToMove);

            // remove the old blend
            bool editedTransitionWasOpened = DeleteTransition(oldTransitionID);

            Refresh();
            ui->treeWidget->SelectItem(transitionID);

            if (editedTransitionWasOpened)
            {
                OpenSelectedTransition(transitionID);
            }
        }
    }
}

void CTransitionBrowser::CopySelected()
{
    TTransitionID transitionID;
    if (GetSelectedTransition(transitionID))
    {
        const TTransitionID oldTransitionID(transitionID);
        if (ShowTransitionSettingsDialog(transitionID, "Copy Transition"))
        {
            // Get the old blend
            SFragmentBlend blendToMove;
            MannUtils::GetMannequinEditorManager().GetFragmentBlend(m_animDB, oldTransitionID.fromID, oldTransitionID.toID, oldTransitionID.fromFTS, oldTransitionID.toFTS, oldTransitionID.uid, blendToMove);

            // And add it as a new one
            blendToMove.uid = SFragmentBlendUid(SFragmentBlendUid::NEW);
            transitionID.uid = MannUtils::GetMannequinEditorManager().AddBlend(m_animDB, transitionID.fromID, transitionID.toID, transitionID.fromFTS, transitionID.toFTS, blendToMove);

            Refresh();
            OpenSelectedTransition(transitionID);
        }
    }
}

void CTransitionBrowser::FindFragmentReferences(QString fragSearchStr)
{
    fragSearchStr = fragSearchStr.toLower();

    m_filterFragmentIDTextFrom = fragSearchStr;
    m_filterFragmentIDTextTo = "";
    m_filterTextFrom = "";
    m_filterTextTo = "";
    m_filterAnimClipFrom = "";
    m_filterAnimClipTo = "";

    UpdateFilters();
}

void CTransitionBrowser::FindTagReferences(QString tagSearchStr)
{
    tagSearchStr = tagSearchStr.toLower();

    m_filterFragmentIDTextFrom = "";
    m_filterFragmentIDTextTo = "";
    m_filterTextFrom = tagSearchStr;
    m_filterTextTo = "";
    m_filterAnimClipFrom = "";
    m_filterAnimClipTo = "";

    UpdateFilters();
}

void CTransitionBrowser::FindClipReferences(QString animSearchStr)
{
    animSearchStr = animSearchStr.toLower();

    m_filterFragmentIDTextFrom = "";
    m_filterFragmentIDTextTo = "";
    m_filterTextFrom = "";
    m_filterTextTo = "";
    m_filterAnimClipFrom = animSearchStr;
    m_filterAnimClipTo = "";

    UpdateFilters();
}

void CTransitionBrowser::UpdateTags(QStringList& tags, QString tagString)
{
    tags = tagString.toLower().split(QStringLiteral(" "));
}

void CTransitionBrowser::UpdateFilters()
{
    ui->FILTER_TAGS_FROM->setText(m_filterTextFrom);
    ui->FILTER_FRAGMENTS_FROM->setText(m_filterFragmentIDTextFrom);
    ui->FILTER_ANIM_CLIP_FROM->setText(m_filterAnimClipFrom);
    ui->FILTER_TAGS_TO->setText(m_filterTextTo);
    ui->FILTER_FRAGMENTS_TO->setText(m_filterFragmentIDTextTo);
    ui->FILTER_ANIM_CLIP_TO->setText(m_filterAnimClipTo);

    UpdateTags(m_filtersFrom, m_filterTextFrom);
    UpdateTags(m_filtersTo, m_filterTextTo);
    Refresh();
}

void CTransitionBrowser::UpdateButtonStatus()
{
    const CTransitionBrowserRecord* pRec = GetSelectedRecord();
    if (pRec && !pRec->IsFolder())
    {
        ui->NEW->setEnabled(TRUE);
        ui->EDIT->setEnabled(TRUE);
        ui->COPY->setEnabled(TRUE);
        ui->DELETEBTN->setEnabled(TRUE);
        ui->BLEND_OPEN->setEnabled(TRUE);
    }
    else
    {
        ui->NEW->setEnabled(FALSE);
        ui->EDIT->setEnabled(FALSE);
        ui->COPY->setEnabled(FALSE);
        ui->DELETEBTN->setEnabled(FALSE);
        ui->BLEND_OPEN->setEnabled(FALSE);
    }

    ui->NEW->setEnabled(ui->CONTEXT->count() > 0);
}

bool CTransitionBrowser::FilterByAnimClip(const FragmentID fragId, const SFragTagState& tagState, const QString& filter)
{
    const uint32 numTagSets = m_animDB->GetTotalTagSets(fragId);

    for (uint32 tagId = 0; tagId < numTagSets; ++tagId)
    {
        SFragTagState tagSetState;
        uint32 numOptions = m_animDB->GetTagSetInfo(fragId, tagId, tagSetState);

        if (tagSetState != tagState)
        {
            continue;
        }

        for (uint32 optionId = 0; optionId < numOptions; ++optionId)
        {
            const CFragment* sourceFragment = m_animDB->GetEntry(fragId, tagId, optionId);

            if (sourceFragment)
            {
                uint32 numAnimLayers = sourceFragment->m_animLayers.size();
                for (uint32 layerId = 0; layerId < numAnimLayers; ++layerId)
                {
                    const TAnimClipSequence& animLayer = sourceFragment->m_animLayers[layerId];
                    const uint32 numClips = animLayer.size();
                    for (uint32 clipId = 0; clipId < numClips; ++clipId)
                    {
                        const SAnimClip& animClip = animLayer[clipId];
                        if (QtUtil::ToQString(animClip.animation.animRef.c_str()).contains(filter))
                        {
                            return TRUE;
                        }
                    }
                }
            }
        }
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////
// CTransitionBrowserTreeCtrl

enum ETransitionReportColumn
{
    TRANSITIONCOLUMN_FROM = 0,
    TRANSITIONCOLUMN_TO
};

//////////////////////////////////////////////////////////////////////////
CTransitionBrowserTreeCtrl::CTransitionBrowserTreeCtrl(QWidget* parent)
    : QTreeWidget(parent)
    , m_pTransitionBrowser(0)
{
    header()->resizeSections(QHeaderView::Fixed);
}

void CTransitionBrowserTreeCtrl::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return && m_pTransitionBrowser)
    {
        CTransitionBrowserRecord* pRecord = (CTransitionBrowserRecord*) currentItem();
        if (pRecord)
        {
            if (pRecord->IsFolder())
            {
                pRecord->setExpanded(!pRecord->isExpanded());
            }
            else
            {
                m_pTransitionBrowser->SelectAndOpenRecord(pRecord->GetTransitionID());
            }
            event->accept();
        }
        return;
    }

    QTreeWidget::keyPressEvent(event);
}

void CTransitionBrowserTreeCtrl::resizeEvent(QResizeEvent* event)
{
    QTreeWidget::resizeEvent(event);
    for (int i = 0; i < columnCount(); i++)
    {
        header()->resizeSection(i, event->size().width() / columnCount());
    }
}

/*
// Was useful for debugging
//////////////////////////////////////////////////////////////////////////
void CTransitionBrowserTreeCtrl::PrintRows(const char* szWhen)
{
    CryWarning(
        VALIDATOR_MODULE_ANIMATION,
        VALIDATOR_WARNING,
        "\n--%s---\n", szWhen);

    CXTPReportRows *pRows = GetRows();
    for (int i = 0,nNumRows = pRows->GetCount(); i < nNumRows; i++)
    {
        CXTPReportRow *pRow = pRows->GetAt(i);
        CTransitionBrowserRecord* pCurrentRecord = RowToRecord(pRow);

        CryWarning(
            VALIDATOR_MODULE_ANIMATION,
            VALIDATOR_WARNING,
            "Row %d, from=%s  to=%s  expanded=%d\n",
            i,
            pCurrentRecord ? pCurrentRecord->GetRecordData().m_from : "<NULL>",
            pCurrentRecord ? pCurrentRecord->GetRecordData().m_to : "<NULL>",
            pRow->IsExpanded() ? 1 : 0
            );
    }

    CryWarning(
        VALIDATOR_MODULE_ANIMATION,
        VALIDATOR_WARNING,
        "-----\n");
}*/

//////////////////////////////////////////////////////////////////////////
void CTransitionBrowserTreeCtrl::ExpandRowsForRecordDataCollection(const TTransitionBrowserRecordDataCollection& expandedRecordDataCollection)
{
    QList<QTreeWidgetItem*> workList;
    for (int i = 0; i < topLevelItemCount(); ++i)
    {
        workList << topLevelItem(i);
    }
    while (!workList.empty())
    {
        CTransitionBrowserRecord* pCurrentRecord = (CTransitionBrowserRecord*) workList.front();
        workList.pop_front();
        for (int i = 0; i < pCurrentRecord->childCount(); i++)
        {
            workList << pCurrentRecord->child(i);
        }

        if ( // Incredibly slow! Use map for example to optimize this
            std::find(expandedRecordDataCollection.begin(), expandedRecordDataCollection.end(), pCurrentRecord->GetRecordData()) != expandedRecordDataCollection.end())
        {
            pCurrentRecord->setExpanded(TRUE);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionBrowserTreeCtrl::SelectAndFocusRecordDataCollection(const CTransitionBrowserRecord::SRecordData* pFocusedRecordData, const TTransitionBrowserRecordDataCollection* pMarkedRecordDataCollection)
{
    selectionModel()->clear();
    CTransitionBrowserRecord* pFocusedRecord = NULL;
    if (pFocusedRecordData || pMarkedRecordDataCollection)
    {
        QList<QTreeWidgetItem*> workList;
        for (int i = 0; i < topLevelItemCount(); ++i)
        {
            workList << topLevelItem(i);
        }
        while (!workList.empty())
        {
            CTransitionBrowserRecord* pCurrentRecord = (CTransitionBrowserRecord*) workList.front();
            workList.pop_front();
            for (int i = 0; i < pCurrentRecord->childCount(); i++)
            {
                workList << pCurrentRecord->child(i);
            }

            if (pFocusedRecordData && pCurrentRecord && pCurrentRecord->IsEqualTo(*pFocusedRecordData))
            {
                selectionModel()->clear();
                pCurrentRecord->setSelected(true);
                setCurrentItem(pCurrentRecord);
            }
            else if (pMarkedRecordDataCollection && // Incredibly slow! Use map for example to optimize this
                     std::find(pMarkedRecordDataCollection->begin(), pMarkedRecordDataCollection->end(), pCurrentRecord->GetRecordData()) != pMarkedRecordDataCollection->end())
            {
                selectionModel()->clear();
                pCurrentRecord->setSelected(true);
            }
        }
    }

    if (pFocusedRecord)
    {
        scrollToItem(pFocusedRecord);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionBrowserTreeCtrl::SelectItem(CTransitionBrowserRecord* pRecord)
{
    selectionModel()->clear();
    if (pRecord)
    {
        pRecord->setSelected(true);
        scrollToItem(pRecord);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTransitionBrowserTreeCtrl::SelectItem(const TTransitionID& transitionID)
{
    selectionModel()->clear();
    CRecord* pRecord = FindRecordForTransition(transitionID);
    if (pRecord)
    {
        scrollToItem(pRecord);
        SelectItem(pRecord);
    }
}

CTransitionBrowserTreeCtrl::CRecord* CTransitionBrowserTreeCtrl::FindRecordForTransition(const TTransitionID& transitionID)
{
    for (int i = 0; i < topLevelItemCount(); i++)
    {
        CRecord* pRecord = SearchRecordForTransition((CRecord*)topLevelItem(i), transitionID);
        if (pRecord)
        {
            return pRecord;
        }
    }
    return NULL;
}

CTransitionBrowserTreeCtrl::CRecord* CTransitionBrowserTreeCtrl::SearchRecordForTransition(CRecord* pRecord, const TTransitionID& transitionID)
{
    CRecord* pFound = NULL;

    if (pRecord->IsEqualTo(transitionID))
    {
        pFound = pRecord;
    }
    else
    {
        if (pRecord->childCount())
        {
            for (int i = 0, numChildren = pRecord->childCount(); (i < numChildren) && (pFound == NULL); ++i)
            {
                CRecord* pReportRecord = (CRecord*) pRecord->child(i);
                pFound = SearchRecordForTransition((CRecord*)pReportRecord, transitionID);
                if (pFound)
                {
                    return pFound;
                }
            }
        }
    }

    return pFound;
}

void CTransitionBrowserTreeCtrl::SavePresentationState(CTransitionBrowserTreeCtrl::SPresentationState& outState) const
{
    outState = CTransitionBrowserTreeCtrl::SPresentationState();

    QList<QTreeWidgetItem*> workList;
    for (int i = 0; i < topLevelItemCount(); ++i)
    {
        workList << topLevelItem(i);
    }
    while (!workList.empty())
    {
        CTransitionBrowserRecord* pCurrentRecord = (CTransitionBrowserRecord*) workList.front();
        workList.pop_front();
        for (int i = 0; i < pCurrentRecord->childCount(); i++)
        {
            workList << pCurrentRecord->child(i);
        }

        outState.expandedRecordDataCollection.push_back(pCurrentRecord->GetRecordData());
    }

    QList<QTreeWidgetItem*> selection = selectedItems();
    for (int i = 0, nNumRows = selection.size(); i < nNumRows; i++)
    {
        QTreeWidgetItem* pRecord = selection[i];
        if (pRecord->isExpanded())
        {
            outState.expandedRecordDataCollection.push_back(((CTransitionBrowserRecord*) pRecord)->GetRecordData());
        }
    }

    QTreeWidgetItem* pFocusedRecord = currentItem();
    if (pFocusedRecord)
    {
        outState.pFocusedRecordData = std::make_shared<CTransitionBrowserRecord::SRecordData>(((CTransitionBrowserRecord*) pFocusedRecord)->GetRecordData());
    }
}

void CTransitionBrowserTreeCtrl::RestorePresentationState(const CTransitionBrowserTreeCtrl::SPresentationState& state)
{
    ExpandRowsForRecordDataCollection(state.expandedRecordDataCollection);
    SelectAndFocusRecordDataCollection(state.pFocusedRecordData.get(), &state.selectedRecordDataCollection);
}

//////////////////////////////////////////////////////////////////////////
//
CTransitionBrowserRecord::SRecordData::SRecordData(const QString& from, const QString& to)
    : m_from(from)
    , m_to(to)
    , m_bFolder(true)
{
}

CTransitionBrowserRecord::SRecordData::SRecordData(const TTransitionID& transitionID, float selectTime)
    : m_to("")
    , m_bFolder(false)
    , m_transitionID(transitionID)
{
    m_from = CTransitionBrowser::tr("Select @ %1").arg(selectTime, 5, 'f', 2);
}

//////////////////////////////////////////////////////////////////////////
// CTransitionBrowserRecord
CTransitionBrowserRecord::CTransitionBrowserRecord(const QString& from, const QString& to)
    : QTreeWidgetItem()
    , m_data(from, to)
{
    CreateItems();
}

CTransitionBrowserRecord::CTransitionBrowserRecord(const TTransitionID& transitionID, float selectTime)
    : QTreeWidgetItem()
    , m_data(transitionID, selectTime)
{
    CreateItems();
}

const TTransitionID& CTransitionBrowserRecord::GetTransitionID() const
{
    assert(!m_data.m_bFolder);
    return m_data.m_transitionID;
}

bool CTransitionBrowserRecord::IsEqualTo(const TTransitionID& transitionID) const
{
    return m_data.m_bFolder ? false : (m_data.m_transitionID == transitionID);
}

bool CTransitionBrowserRecord::IsEqualTo(const SRecordData& recordData) const
{
    return (m_data == recordData);
}

//////////////////////////////////////////////////////////////////////////
void CTransitionBrowserRecord::CreateItems()
{
    const int IMAGE_FOLDER = 0;
    const int IMAGE_ITEM = 1;

    setText(0, m_data.m_from);
    setText(1, m_data.m_to);

    setIcon(0, m_data.m_bFolder ? QIcon(":/FragmentBrowser/Controls/mann_folder.png") : QIcon(":/FragmentBrowser/Controls/mann_tag.png"));
}

#include <Mannequin/Controls/TransitionBrowser.moc>
