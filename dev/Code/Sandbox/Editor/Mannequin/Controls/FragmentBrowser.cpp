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

#include "stdafx.h"
#include "FragmentBrowser.h"

#include "../FragmentEditor.h"
#include "../MannequinBase.h"

#include "../MannTagEditorDialog.h"
#include "../MannAdvancedPasteDialog.h"
#include "../MannequinDialog.h"
#include "../SequencerDopeSheetBase.h"
#include <IGameFramework.h>
#include <ICryMannequinEditor.h>

#include "QtUtilWin.h"

#include <QClipboard>
#include <QMessageBox>
#include <QMenu>
#include <QDragMoveEvent>
#include <QMimeData>
#include <QDebug>
#include <QTreeWidget>
#include <QApplication>
#include <QInputDialog>
#include <QFile>
#include <QTemporaryFile>
#include <QDir>

#define FILTER_DELAY_SECONDS (0.5f)


//////////////////////////////////////////////////////////////////////////

QDataStream& operator<<(QDataStream& out, const STreeFragmentDataPtr& obj);
QDataStream& operator>>(QDataStream& in, STreeFragmentDataPtr& obj);


class CFragmentBrowserTreeWidget
    : public QTreeWidget
{
public:
    CFragmentBrowserTreeWidget(QWidget* parent)
        : QTreeWidget(parent)
        , m_controller(nullptr) { }

    void setController(CFragmentBrowser* p)
    {
        m_controller = p;
    }

protected:
    void dragMoveEvent(QDragMoveEvent* event)
    {
        QTreeWidget::dragMoveEvent(event);
        if (!event->isAccepted())
        {
            return;
        }

        QTreeWidgetItem* item = itemAt(event->pos());

        // internal drag and drop
        if (item && event->mouseButtons() == Qt::RightButton)
        {
            STreeFragmentDataPtr dn = CFragmentBrowser::draggedFragment(event->mimeData());
            if (dn && dn->item && dn->item->parent() != item && dn->item->parent() != item->parent())
            {
                event->accept();
                return;
            }
        }

        // animations dragged from Geppetto
        std::vector<QString> animations = CFragmentBrowser::draggedAnimations(event->mimeData());
        if (!animations.empty())
        {
            if (item || topLevelItemCount() == 0)
            {
                // accept the drop if there's at least one valid animation that can
                // be dropped on the item under the cursor, or if there's at least
                // one valid animation and the tree is empty (in which case the user
                // will be asked if a new Fragment ID should be created)

                auto it = std::find_if(
                            animations.begin(),
                            animations.end(),
                            [this, item](const QString& animation)
                            {
                                const string& animationString(animation.toStdString().c_str());
                                if (!m_controller->IsValidAnimation(animationString))
                                {
                                    return false;
                                }

                                if (item)
                                {
                                    SFragTagState tagState;
                                    FragmentID fragID = m_controller->GetValidFragIDForAnim(item, tagState, animationString);
                                    if (fragID == FRAGMENT_ID_INVALID)
                                    {
                                        return false;
                                    }
                                }

                                return true;
                            });

                if (it != animations.end())
                {
                    event->accept();
                    return;
                }
            }
        }

        event->ignore();
    }

    void dropEvent(QDropEvent* event)
    {
        QTreeWidgetItem* item = itemAt(event->pos());

        // internal drag and drop
        if (item && event->mouseButtons() == Qt::RightButton)
        {
            STreeFragmentDataPtr dn = CFragmentBrowser::draggedFragment(event->mimeData());
            if (dn)
            {
                m_controller->OnRDrop(dn, item);
                event->accept();
            }
        }

        // animations dragged from Geppetto
        std::vector<QString> animations = CFragmentBrowser::draggedAnimations(event->mimeData());
        if (!animations.empty())
        {
            if (!item)
            {
                if (topLevelItemCount() == 0)
                {
                    auto result = QMessageBox::question(this,
                                        tr("Create new Fragment ID?"),
                                        tr("Before animations may be added a Fragment ID is required. Would you like to create one?"),
                                        QMessageBox::Ok | QMessageBox::Cancel);

                    if (result == QMessageBox::Ok)
                    {
                        if (m_controller->AddNewDefinition())
                        {
                            auto selected = selectedItems();

                            if (!selected.isEmpty())
                            {
                                item = selected.first();
                            }
                        }
                    }
                }
            }

            if (item)
            {
                for (auto& animation : animations)
                {
                    const string animationString(animation.toStdString().c_str());
                    if (!m_controller->IsValidAnimation(animationString))
                    {
                        continue;
                    }

                    // Get the fragment ID
                    SFragTagState tagState;
                    FragmentID fragID = m_controller->GetValidFragIDForAnim(item, tagState, animationString);
                    if (fragID != FRAGMENT_ID_INVALID)
                    {
                        // Add the animation to the animation database through the browser, and rebuild the ID entry
                        m_controller->AddAnimToFragment(fragID, tagState, animationString);
                    }
                }

                event->accept();
            }
        }
    }

    void keyPressEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            m_controller->OnTVEditSel();
            event->accept();
            return;
        default:
            break;
        }
        QTreeWidget::keyPressEvent(event);
    }

    QStringList mimeTypes() const override
    {
        QStringList types = QTreeWidget::mimeTypes();
        types << QStringLiteral("application/x-lumberyard-animation");
        return types;
    }

private:

    CFragmentBrowser*   m_controller;
};

QDataStream& operator<<(QDataStream& out, const STreeFragmentDataPtr& obj)
{
    out.writeRawData((const char*)&obj, sizeof(obj));
    return out;
}

QDataStream& operator>>(QDataStream& in, STreeFragmentDataPtr& obj)
{
    in.readRawData((char*)&obj, sizeof(obj));
    return in;
}


#include <Mannequin/Controls/ui_FragmentBrowser.h>

const int IMAGE_FOLDER = 1;
const int IMAGE_ANIM = 6;
const QString DEFAULT_FRAGMENT_ID_FILTER_TEXT("<FragmentID Filter>");
const QString DEFAULT_TAG_FILTER_TEXT("<Tag Filter>");
const QString DEFAULT_ANIM_CLIP_FILTER_TEXT("<Anim Filter>");

CFragmentBrowser::CFragmentBrowser(CMannFragmentEditor& fragEditor, QWidget* pParent)
    : QWidget(pParent)
    , m_animDB(NULL)
    , m_fragEditor(fragEditor)
    , m_editedItem(0)
    , m_rightDrag(false)
    , m_dragItem(0)
    , m_context(NULL)
    , m_scopeContext(-1)
    , m_fragmentID(FRAGMENT_ID_INVALID)
    , m_option(0)
    , m_showSubFolders(true)
    , m_showEmptyFolders(true)
    , m_filterDelay(0.0f)
{
    OnInitDialog();

    qRegisterMetaType<STreeFragmentDataPtr>("STreeFragmentDataPtr");
    qRegisterMetaTypeStreamOperators<STreeFragmentDataPtr>("STreeFragmentDataPtr");
}


CFragmentBrowser::~CFragmentBrowser()
{
}

void CFragmentBrowser::Update(void)
{
    static CTimeValue timeLast = gEnv->pTimer->GetAsyncTime();

    CTimeValue timeNow = gEnv->pTimer->GetAsyncTime();
    CTimeValue deltaTime = timeNow - timeLast;
    timeLast = timeNow;

    if (m_filterDelay > 0.0f)
    {
        m_filterDelay -= deltaTime.GetSeconds();

        if (m_filterDelay <= 0.0f)
        {
            m_filterDelay = 0.0f;
            RebuildAll();
        }
    }
}

enum
{
    CMD_COPY_SELECTED_TEXT = 1,
    CMD_COPY,
    CMD_PASTE,
    CMD_PASTE_INTO,
    CMD_FIND_FRAGMENT_REFERENCES,
    CMD_FIND_TAG_REFERENCES,
    CMD_NEW_FRAGMENT,
    CMD_NEW_FRAGMENT_ID,
    CMD_DELETE_SELECTED,
    CMD_EDIT_FRAGMENT_ID,
    CMD_TAG_DEFINITION_EDITOR,
    CMD_LAUNCH_TAG_DEF_EDITOR,
};

BOOL CFragmentBrowser::OnInitDialog()
{
    setWindowTitle("Fragment Browser");

    for (int i = 0; i < 10; i++)
    {
        m_imageList << QIcon(QString(":/FragmentBrowser/Controls/animtree_%1.png").arg(i, 2, 10, QChar('0')));
    }

    ui.reset(new Ui::CFragmentBrowser);

    ui->setupUi(this);

    m_TreeCtrl = ui->TREE_FB;
    ((CFragmentBrowserTreeWidget*)m_TreeCtrl)->setController(this);
    m_cbContext = ui->CONTEXT;
    m_CurrFile = ui->EDIT_ADBFILE;
    m_editFilterTags = ui->FILTER_TAGS;
    m_editFilterFragmentIDs = ui->FILTER_FRAGMENT;
    m_editAnimClipFilter = ui->FILTER_ANIM_CLIP;
    m_chkShowSubFolders = ui->FRAGMENT_SUBFOLDERS_CHK;
    m_chkShowSubFolders->setChecked(m_showSubFolders);
    m_chkShowEmptyFolders = ui->FRAGMENT_SHOWEMPTY_CHK;
    m_chkShowEmptyFolders->setChecked(m_showEmptyFolders);
    m_contextMenuButton = ui->BTN_OPEN_CONTEXT_MENU;

    m_editFilterTags->setText(DEFAULT_TAG_FILTER_TEXT);
    m_editFilterTags->selectAll();
    m_editFilterFragmentIDs->setText(DEFAULT_FRAGMENT_ID_FILTER_TEXT);
    m_editFilterFragmentIDs->selectAll();
    m_editAnimClipFilter->setText(DEFAULT_ANIM_CLIP_FILTER_TEXT);
    m_editAnimClipFilter->selectAll();

    connect(m_chkShowSubFolders, &QCheckBox::toggled, this, &CFragmentBrowser::OnToggleSubfolders);
    connect(m_chkShowEmptyFolders, &QCheckBox::toggled, this, &CFragmentBrowser::OnToggleShowEmpty);
    connect(m_cbContext, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CFragmentBrowser::SetScopeContext);
    connect(m_editFilterTags, &QLineEdit::textChanged, this, &CFragmentBrowser::OnChangeFilterTags);
    connect(m_editFilterFragmentIDs, &QLineEdit::textChanged, this, &CFragmentBrowser::OnChangeFilterFragments);
    connect(m_editAnimClipFilter, &QLineEdit::textChanged, this, &CFragmentBrowser::OnChangeFilterAnimClip);
    connect(m_contextMenuButton, &QPushButton::clicked, this, [this]()
    {
        this->OnTVRightClick(m_contextMenuButton->pos());
    });

    m_TreeCtrl->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_TreeCtrl->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_TreeCtrl, &QTreeWidget::itemDoubleClicked, this, &CFragmentBrowser::OnTVEditSel);
    connect(m_TreeCtrl, &QTreeWidget::customContextMenuRequested, this, &CFragmentBrowser::OnTVRightClick);
    connect(m_TreeCtrl, &QTreeWidget::itemSelectionChanged, this, &CFragmentBrowser::OnSelectionChanged);

    QAction *deleteAction = new QAction(m_TreeCtrl);
    deleteAction->setShortcut(Qt::Key_Delete);
    connect(deleteAction, &QAction::triggered, this, &CFragmentBrowser::OnDelete);

    m_newFragmentAction = new QAction(tr("New Fragment"), m_TreeCtrl);
    m_newFragmentAction->setData(CMD_NEW_FRAGMENT);
    connect(m_newFragmentAction, &QAction::triggered, this, &CFragmentBrowser::OnNewFragment);

    m_newFragmentIdAction = new QAction(tr("New Fragment ID"), m_TreeCtrl);
    m_newFragmentIdAction->setData(CMD_NEW_FRAGMENT_ID);
    connect(m_newFragmentIdAction, &QAction::triggered, this, &CFragmentBrowser::OnNewDefinition);

    m_editFragmentIdAction = new QAction(tr("Edit Fragment ID"), m_TreeCtrl);
    m_editFragmentIdAction->setData(CMD_EDIT_FRAGMENT_ID);
    connect(m_editFragmentIdAction, &QAction::triggered, this, &CFragmentBrowser::OnRenameDefinition);

    m_deleteAction = new QAction(tr("Delete Selected"), m_TreeCtrl);
    m_deleteAction->setData(CMD_DELETE_SELECTED);
    connect(m_deleteAction, &QAction::triggered, this, &CFragmentBrowser::OnDelete);

    m_copyAction = new QAction(tr("Copy"), m_TreeCtrl);
    m_copyAction->setData(CMD_COPY);
    m_copyAction->setShortcut(QKeySequence::Copy);
    connect(m_copyAction, &QAction::triggered, this, &CFragmentBrowser::OnMenuCopy);
    m_pasteAction = new QAction(tr("Paste"), m_TreeCtrl);
    m_pasteAction->setData(CMD_PASTE);
    m_pasteAction->setShortcut(QKeySequence::Paste);
    connect(m_pasteAction, &QAction::triggered, this, [this]()
        {
            this->OnMenuPaste(false);
        });
    m_pasteIntoAction = new QAction(tr("Paste Into..."), m_TreeCtrl);
    m_pasteIntoAction->setData(CMD_PASTE_INTO);
    m_pasteIntoAction->setShortcut(QKeySequence("Ctrl+Shift+V"));
    connect(m_pasteIntoAction, &QAction::triggered, this, [this]()
        {
            this->OnMenuPaste(true);
        });

    m_launchTagDefinitionEditorAction = new QAction(tr("Tag Definition Editor"), m_TreeCtrl);
    m_launchTagDefinitionEditorAction->setData(CMD_LAUNCH_TAG_DEF_EDITOR);
    connect(m_launchTagDefinitionEditorAction, &QAction::triggered, this, &CFragmentBrowser::OnTagDefEditorBtn);

    UpdateControlEnabledStatus();

    return TRUE;
}

QTreeWidgetItem* CFragmentBrowser::GetSelectedItem()
{
    QList<QTreeWidgetItem*> selItems = m_TreeCtrl->selectedItems();
    QTreeWidgetItem* item = selItems.empty() ? nullptr : selItems.back();
    return item;
}

const QTreeWidgetItem* CFragmentBrowser::GetSelectedItem() const
{
    QList<QTreeWidgetItem*> selItems = m_TreeCtrl->selectedItems();
    const QTreeWidgetItem* item = selItems.empty() ? nullptr : selItems.back();
    return item;
}


void CFragmentBrowser::UpdateControlEnabledStatus()
{
}

void CFragmentBrowser::SetContext(SMannequinContexts& context)
{
    m_context = &context;

    m_cbContext->clear();
    const uint32 numContexts = context.m_contextData.size();
    for (uint32 i = 0; i < numContexts; i++)
    {
        m_cbContext->addItem(context.m_contextData[i].name);
    }
    m_cbContext->setCurrentIndex(0);

    if (context.m_controllerDef)
    {
        SetScopeContext(0);
    }

    UpdateControlEnabledStatus();
}

bool CFragmentBrowser::FragmentHasTag(const STreeFragmentData* fragData, const CTagDefinition* tagDef, TagID tagID) const
{
    auto &tagState = fragData->tagState;

    const CTagDefinition& globalTagDefs = m_context->m_controllerDef->m_tags;
    if (tagDef == &globalTagDefs)
    {
        return tagDef->IsSet(tagState.globalTags, tagID);
    }

    const CTagDefinition* subTagDef = m_animDB->GetFragmentDefs().GetSubTagDefinition(fragData->fragID);
    if (tagDef == subTagDef)
    {
        return tagDef->IsSet(tagState.fragmentTags, tagID);
    }

    return false;
}

STreeFragmentDataPtr CFragmentBrowser::draggedFragment(const QMimeData* mimeData)
{
    QByteArray encoded = mimeData->data("application/x-qabstractitemmodeldatalist");
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    while (!stream.atEnd())
    {
        int row, col;
        QMap<int, QVariant> roleDataMap;
        stream >> row >> col >> roleDataMap;

        QVariant v = roleDataMap[Qt::UserRole];
        if (v.isValid())
        {
            STreeFragmentDataPtr pNode = v.value<STreeFragmentDataPtr>();
            if (pNode)
            {
                return pNode;
            }
        }
    }
    return nullptr;
}

std::vector<QString> CFragmentBrowser::draggedAnimations(const QMimeData* mimeData)
{
    std::vector<QString> animations;

    const QString format = QStringLiteral("application/x-lumberyard-animation");

    if (mimeData->hasFormat(format))
    {
        QByteArray data = mimeData->data(format);
        QDataStream stream(&data, QIODevice::ReadOnly);

        while (!stream.atEnd())
        {
            QString name;
            stream >> name;
            animations.emplace_back(name.toUtf8().data());
        }
    }

    return animations;
}

void CFragmentBrowser::OnTagRenamed(const CTagDefinition* tagDef, TagID tagID, const QString& updatedName)
{
    const CTagDefinition& fragDefs = m_animDB->GetFragmentDefs();

    if (tagDef == &fragDefs)
    {
        // FragmentID name changed, update top-level tree item

        QTreeWidgetItem* item = m_fragmentItems[tagID];
        item->setText(0, updatedName);
    }
    else
    {
        // update items that contain the updated fragment

        for (auto fragData : m_fragmentData)
        {
            if (fragData->tagSet && FragmentHasTag(fragData, tagDef, tagID))
            {
                QTreeWidgetItem* item = fragData->item;

                // for a fragment has tags Tag0+Tag1+Tag2 set, the tree looks like
                //
                // FragmentID
                // |
                // +-- Tag0 ( tagState: Tag0 )
                //     |
                //     +-- Tag1 ( tagState: Tag0 + Tag1 )
                //         |
                //         +-- Tag2 ( tagState: Tag0 + Tag1 + Tag2 )
                //             |
                //             +-- fragment ( tagState: Tag0 + Tag1 + Tag2 )
                //
                // we only update the text for this item if the tag is not already set on the parent item

                QTreeWidgetItem* parent = item->parent();

                auto parentFragData = parent->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();
                if (!parentFragData || !FragmentHasTag(parentFragData, tagDef, tagID))
                {
                    item->setText(0, updatedName);
                }
            }
        }
    }

    // refresh the fragment editor if the currently edited fragment was affected
    if (m_editedItem)
    {
        auto fragData = m_editedItem->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();

        if ((tagDef == &fragDefs && fragData->fragID == tagID) ||
            (FragmentHasTag(fragData, tagDef, tagID)))
        {
            EditItem(m_editedItem);
        }
    }
}

void CFragmentBrowser::OnTagRemoved(const CTagDefinition* tagDef, TagID tagID)
{
    const CTagDefinition& fragDefs = m_animDB->GetFragmentDefs();

    if (tagDef == &fragDefs)
    {
        // remove FragmentID

        CleanFragmentID(tagID);
    }
    else
    {
        // remove fragments containing the tag and fix remaining tag sets

        std::vector<QTreeWidgetItem*> itemsToRemove;

        for (auto fragData : m_fragmentData)
        {
            if (FragmentHasTag(fragData, tagDef, tagID))
            {
                // add to the list of fragments to be removed

                if (!fragData->tagSet)
                {
                    auto item = fragData->item;
                    itemsToRemove.push_back(fragData->item);
                }
            }
            else
            {
                // adjust tag state if necessary

                auto &tagState = fragData->tagState;

                TagState* origTags = nullptr;

                const CTagDefinition& globalTagDefs = m_context->m_controllerDef->m_tags;
                if (tagDef == &globalTagDefs)
                {
                    origTags = &tagState.globalTags;
                }
                else
                {
                    const CTagDefinition* subTagDef = m_animDB->GetFragmentDefs().GetSubTagDefinition(fragData->fragID);
                    if (tagDef == subTagDef)
                    {
                        origTags = &tagState.fragmentTags;
                    }
                }

                if (origTags)
                {
                    TagState tags;
                    tags.Clear();

                    for (uint32 i = 0; i < tagDef->GetNum(); ++i)
                    {
                        if (i != tagID)
                        {
                            if (tagDef->IsSet(*origTags, i))
                            {
                                tagDef->Set(tags, i < tagID ? i : i - 1, true);
                            }
                        }
                    }

                    *origTags = tags;
                }
            }
        }

        // remove items containing the tag and parent items if there are no other siblings

        for (auto item : itemsToRemove)
        {
            while (item->parent() && item->childCount() == 0)
            {
                if (item == m_editedItem)
                {
                    m_editedItem = nullptr;
                }

                auto fragData = item->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();

                stl::find_and_erase(m_fragmentData, fragData);

                auto parent = item->parent();

                delete fragData;
                delete item;

                item = parent;
            }
        }
    }

    // reset the fragment editor if the currently edited fragment was removed
    if (m_editedItem == nullptr)
    {
        EditItem(nullptr);
    }
}

void CFragmentBrowser::OnTagAdded(const CTagDefinition* tagDef, const QString& name)
{
    const CTagDefinition& fragDefs = m_animDB->GetFragmentDefs();

    if (tagDef == &fragDefs)
    {
        AddFragmentIDItem(name);
    }
}

void CFragmentBrowser::RebuildAll()
{
    // Clear old data
    m_fragEditor.FlushChanges();

    // WARNING: clear m_fragmentItems before m_TreeCtrl.DeleteAllItems() because
    //   DeleteAllItems() might still send a SelectItem message, which will
    //   do a lookup in the m_fragmentItems and subsequently the context,
    //   which could be switched already
    m_fragmentItems.clear();

    m_TreeCtrl->clear();

    m_editedItem = nullptr;
    for (uint32 i = 0; i < m_fragmentData.size(); i++)
    {
        delete(m_fragmentData[i]);
    }
    m_fragmentData.resize(0);

    // Fill new data & try to reselect currently selected item
    if (m_animDB)
    {
        const CTagDefinition& fragDefs  = m_animDB->GetFragmentDefs();
        const uint32 numFrags = fragDefs.GetNum();

        for (uint32 i = 0; i < numFrags; i++)
        {
            QString fragTagName = fragDefs.GetTagName(i);
            QTreeWidgetItem* item = new QTreeWidgetItem();
            item->setText(0, fragTagName);
            item->setIcon(0, m_imageList[IMAGE_FOLDER]);
            m_TreeCtrl->addTopLevelItem(item);
            m_fragmentItems.push_back(item);
        }

        for (uint32 i = 0; i < numFrags; i++)
        {
            BuildFragment(i);
        }

        if (!m_showEmptyFolders)
        {
            for (uint32 i = 0; i < numFrags; i++)
            {
                if (!m_fragmentItems[i]->childCount())
                {
                    delete m_fragmentItems[i];
                    m_fragmentItems[i] = 0;
                }
            }
        }

        if (m_filterFragmentIDText.length() > 0)
        {
            for (uint32 i = 0; i < numFrags; i++)
            {
                if (m_fragmentItems[i] != 0)
                {
                    QString fragTagName = QtUtil::ToQString(fragDefs.GetTagName(i)).toLower();
                    if (!fragTagName.contains(m_filterFragmentIDText))
                    {
                        delete m_fragmentItems[i];
                        m_fragmentItems[i] = 0;
                    }
                }
            }
        }
    }
    m_TreeCtrl->sortItems(0, Qt::AscendingOrder);
    OnSelectionChanged();
}

void CFragmentBrowser::SetDatabase(IAnimationDatabase* animDB)
{
    if (m_animDB != animDB)
    {
        m_animDB = animDB;

        RebuildAll();

        if (m_animDB)
        {
            if (m_fragmentID != FRAGMENT_ID_INVALID)
            {
                m_editedItem = 0;
                SelectFragment(m_fragmentID, m_tagState, m_option);
            }
        }
        else
        {
            m_editedItem = 0;
            m_fragmentID = FRAGMENT_ID_INVALID;
            m_tagState = SFragTagState();
            m_option = 0;
        }
    }
}

QTreeWidgetItem* CFragmentBrowser::FindFragmentItem(FragmentID fragmentID, const SFragTagState& tagState, uint32 option) const
{
    for (uint32 i = 0; i < m_fragmentData.size(); i++)
    {
        STreeFragmentData& fragData = *m_fragmentData[i];
        if ((fragData.fragID == fragmentID) && (fragData.tagState == tagState) && (fragData.option == option) && (!fragData.tagSet))
        {
            return fragData.item;
        }
    }

    return NULL;
}

bool CFragmentBrowser::SelectFragment(FragmentID fragmentID, const SFragTagState& tagState, uint32 option)
{
    SFragTagState bestTagState;
    uint32 numOptions = m_animDB->FindBestMatchingTag(SFragmentQuery(fragmentID, tagState), &bestTagState);
    if (numOptions > 0)
    {
        option = option % numOptions;
        QTreeWidgetItem* selItem = FindFragmentItem(fragmentID, bestTagState, option);

        if (selItem)
        {
            //--- Found tag node
            // expand tree
            QTreeWidgetItem* parentItem = selItem->parent();

            while (parentItem != nullptr)
            {
                parentItem->setExpanded(true);
                parentItem = parentItem->parent();
            }

            m_TreeCtrl->selectionModel()->clear();
            selItem->setSelected(true);

            SetEditItem(selItem, fragmentID, bestTagState, option);
            m_onSelectedItemEditCallback(fragmentID, GetItemText(selItem));
        }

        m_fragEditor.SetFragment(fragmentID, bestTagState, option);

        CMannequinDialog::GetCurrentInstance()->ShowPane<CFragmentEditorPage*>();
        m_fragEditor.update();

        return true;
    }

    return false;
}

IAnimationDatabase* CFragmentBrowser::FindHostDatabase(uint32 contextID, const SFragTagState& tagState) const
{
    const SScopeContextData* contextData = m_context->GetContextDataForID(tagState.globalTags, contextID, eMEM_FragmentEditor);
    if (contextData && contextData->database)
    {
        return contextData->database;
    }

    return m_animDB;
}

bool CFragmentBrowser::SetTagState(const SFragTagState& newTags)
{
    if ((m_fragmentID != FRAGMENT_ID_INVALID) && (newTags != m_tagState))
    {
        //--- Ensure edited fragment is stored
        m_fragEditor.FlushChanges();

        SFragmentQuery query(m_fragmentID, m_tagState, m_option);
        const CFragment* fragment = m_animDB->GetBestEntry(query);

        uint32 newOption = m_option;
        if (fragment)
        {
            IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
            newOption = mannequinSys.GetMannequinEditorManager()->AddFragmentEntry(m_animDB, m_fragmentID, newTags, *fragment);
            mannequinSys.GetMannequinEditorManager()->DeleteFragmentEntry(m_animDB, m_fragmentID, m_tagState, m_option);
        }

        CleanFragment(m_fragmentID);
        BuildFragment(m_fragmentID);

        return SelectFragment(m_fragmentID, newTags, newOption);
    }
    return false;
}

void CFragmentBrowser::OnRDrop(STreeFragmentDataPtr fragmentDataFrom, QTreeWidgetItem* hitDest)
{
    if (hitDest != NULL)
    {
        if (fragmentDataFrom)
        {
            STreeFragmentData* fragmentDataTo = hitDest->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();
            FragmentID fragID;
            SFragTagState newTagState = fragmentDataFrom->tagState;
            if (fragmentDataTo)
            {
                fragID = fragmentDataTo->fragID;
                newTagState = fragmentDataTo->tagState;
            }
            else
            {
                QString szFragID = hitDest->text(0);
                fragID = m_animDB->GetFragmentID(szFragID.toUtf8().data());

                if (fragID == FRAGMENT_ID_INVALID)
                {
                    return;
                }
            }

            m_fragEditor.FlushChanges();

            SFragmentQuery query(fragmentDataFrom->fragID, fragmentDataFrom->tagState, fragmentDataFrom->option);
            const CFragment* cloneFrag = m_animDB->GetBestEntry(query);
            IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
            uint32 newOption = mannequinSys.GetMannequinEditorManager()->AddFragmentEntry(m_animDB, fragID, newTagState, *cloneFrag);

            CleanFragment(fragID);
            BuildFragment(fragID);

            SelectFragment(fragID, newTagState, newOption);
        }
    }
}

void CFragmentBrowser::OnTVRightClick(const QPoint& pos)
{
    auto selectedItems = m_TreeCtrl->selectedItems();

    QMenu menu;

    menu.addAction(m_newFragmentIdAction);
    menu.addAction(m_newFragmentAction);
    menu.addAction(m_editFragmentIdAction);
    menu.addAction(m_deleteAction);
    menu.addSeparator();
    auto copyNameAction = menu.addAction(tr("Copy Name"));
    copyNameAction->setData(CMD_COPY_SELECTED_TEXT);
    menu.addAction(m_copyAction);
    menu.addAction(m_pasteAction);
    menu.addAction(m_pasteIntoAction);
    menu.addSeparator();

    // enable "New Fragment" if at least one FragmentID or tag set item is selected
    const bool enableNewFragmentAction = std::find_if(selectedItems.begin(), selectedItems.end(),
                                    [](const QTreeWidgetItem* item)
                                    {
                                        auto fragmentData = item->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();
                                        return fragmentData == nullptr || fragmentData->tagSet == true;
                                    }) != selectedItems.end();
    m_newFragmentAction->setEnabled(enableNewFragmentAction);

    m_editFragmentIdAction->setDisabled(selectedItems.empty());
    m_deleteAction->setDisabled(selectedItems.empty());
    copyNameAction->setDisabled(selectedItems.empty());
    m_copyAction->setDisabled(selectedItems.empty());

    m_pasteAction->setEnabled(!m_copiedFragmentIDs.empty() || (!m_copiedItems.empty() && !selectedItems.empty()));
    m_pasteIntoAction->setDisabled(m_copiedItems.empty() || selectedItems.empty());

    menu.addAction(m_launchTagDefinitionEditorAction);

    auto findTagTransitionsAction = menu.addAction(tr("Find Tag Transitions"));
    findTagTransitionsAction->setData(CMD_FIND_TAG_REFERENCES);
    findTagTransitionsAction->setEnabled(!selectedItems.empty());

    auto findFragmentTransitionsAction = menu.addAction(tr("Find Fragment Transitions"));
    findFragmentTransitionsAction->setData(CMD_FIND_FRAGMENT_REFERENCES);
    findFragmentTransitionsAction->setEnabled(!selectedItems.empty());

    QAction* a = menu.exec(QCursor::pos());
    int res = a ? a->data().toInt() : -1;

    CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();

    switch (res)
    {
    case CMD_COPY_SELECTED_TEXT:
    {
        auto item = selectedItems.back();
        QString selectedNodeText = item->text(0);
        QApplication::clipboard()->setText(selectedNodeText);
        break;
    }
    case CMD_FIND_FRAGMENT_REFERENCES:
    {
        auto item = selectedItems.at(0);
        QString selectedNodeText = item->text(0);
        QTreeWidgetItem* root = item->parent();

        // Find highest root (the fragment)
        while (root != NULL)
        {
            item = root;
            root = item->parent();
        }

        QString fragmentName = item->text(0);

        if (pMannequinDialog != NULL)
        {
            pMannequinDialog->FindFragmentReferences(fragmentName);
        }
    } break;
    case CMD_FIND_TAG_REFERENCES:
    {
        auto item = selectedItems.at(0);
        // GetSelectedNodeText (used earlier) doesn't actually get the text on the node...
        QString tagName = item->text(0);

        if (pMannequinDialog != NULL)
        {
            pMannequinDialog->FindTagReferences(tagName);
        }
    } break;
    }
}

void CFragmentBrowser::OnTVEditSel()
{
    EditSelectedItem();
    m_TreeCtrl->setFocus();
}

void CFragmentBrowser::OnMenuCopy()
{
    m_fragEditor.FlushChanges();

    m_copiedItems.clear();
    m_copiedFragmentIDs.clear();

    for (const auto item : UniqueSelectedItems())
    {
        CopyItem(item);
    }
}

QList<QTreeWidgetItem*> CFragmentBrowser::UniqueSelectedItems() const
{
    auto selectedItems = m_TreeCtrl->selectedItems();

    auto it = selectedItems.begin();

    for (const auto item : selectedItems)
    {
        // check if one of the predecessors of item is already in the list

        bool foundPredecessor = false;

        auto parent = item->parent();

        while (parent)
        {
            if (std::find(selectedItems.begin(), selectedItems.end(), parent) != selectedItems.end())
            {
                foundPredecessor = true;
                break;
            }

            parent = parent->parent();
        }

        // remove if predecessor found

        if (foundPredecessor)
        {
            it = selectedItems.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return selectedItems;
}

void CFragmentBrowser::CopyItem(const QTreeWidgetItem* copySourceItem)
{
    const STreeFragmentDataPtr pFragmentDataCopy = copySourceItem->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();

    if (!pFragmentDataCopy)
    {
        // Copy fragmentID data

        const QString name = copySourceItem->text(0);

        const FragmentID fragmentID = m_animDB->GetFragmentID(name.toUtf8().data());
        if (fragmentID != FRAGMENT_ID_INVALID)
        {
            SCopyItemFragmentID copyItemFragmentID;

            copyItemFragmentID.name = name;

            copyItemFragmentID.fragmentDef = m_context->m_controllerDef->GetFragmentDef(fragmentID);

            const CTagDefinition* const pSubTagDefinition = m_animDB->GetFragmentDefs().GetSubTagDefinition(fragmentID);
            copyItemFragmentID.subTagDefinitionFilename = pSubTagDefinition ? pSubTagDefinition->GetFilename() : "";

            CopyFragments(copyItemFragmentID.copiedItems, fragmentID, nullptr, nullptr);

            m_copiedFragmentIDs.push_back(copyItemFragmentID);
        }
    }
    else
    {
        if (pFragmentDataCopy->tagSet)
        {
            // Copy all fragments matching the given tag set

            const SFragTagState* parentTagState = nullptr;

            const QTreeWidgetItem* parentItem = copySourceItem->parent();
            if (parentItem)
            {
                const STreeFragmentDataPtr parentViewData = parentItem->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();
                if (parentViewData)
                {
                    parentTagState = &parentViewData->tagState;
                }
            }

            CopyFragments(m_copiedItems, pFragmentDataCopy->fragID, &pFragmentDataCopy->tagState, parentTagState);
        }
        else
        {
            // Copy selected fragment

            SFragmentQuery query(pFragmentDataCopy->fragID, pFragmentDataCopy->tagState, pFragmentDataCopy->option);
            const CFragment* cloneFrag = m_animDB->GetBestEntry(query);
            if (cloneFrag != NULL)
            {
                m_copiedItems.push_back(SCopyItem(*cloneFrag, SFragTagState()));
            }
        }
    }
}

void CFragmentBrowser::CopyFragments(std::vector<SCopyItem>& copiedItems, FragmentID fragmentID, const SFragTagState* tagState, const SFragTagState* parentTagState)
{
    const CTagDefinition& globalTagDef = m_animDB->GetTagDefs();
    const CTagDefinition* pFragTagDef = m_animDB->GetFragmentDefs().GetSubTagDefinition(fragmentID);
    const uint32 numTagSets = m_animDB->GetTotalTagSets(fragmentID);
    for (uint32 itTagSets = 0; itTagSets < numTagSets; ++itTagSets)
    {
        SFragTagState entryTagState;
        const uint32 numOptions = m_animDB->GetTagSetInfo(fragmentID, itTagSets, entryTagState);

        if (tagState)
        {
            const bool tagsMatch = m_chkShowSubFolders ? TagStateMatches(fragmentID, *tagState, entryTagState) :
                                                         *tagState == entryTagState;
            if (!tagsMatch)
            {
                continue;
            }
        }

        // Strip parent tags out
        if (parentTagState)
        {
            entryTagState.globalTags = globalTagDef.GetDifference(entryTagState.globalTags, parentTagState->globalTags);
            entryTagState.fragmentTags = pFragTagDef ? pFragTagDef->GetDifference(entryTagState.fragmentTags, parentTagState->fragmentTags) : entryTagState.fragmentTags;
        }

        // Copy fragments
        for (uint32 itOptions = 0; itOptions < numOptions; ++itOptions)
        {
            const CFragment* const pFragment = m_animDB->GetEntry(fragmentID, itTagSets, itOptions);
            CRY_ASSERT(pFragment);
            copiedItems.push_back(SCopyItem(*pFragment, entryTagState));
        }
    }
}

void CFragmentBrowser::OnMenuPaste(bool advancedPaste)
{
    if (m_copiedItems.empty() && m_copiedFragmentIDs.empty())
    {
        return;
    }

    m_fragEditor.FlushChanges();

    const auto targetItem = GetSelectedItem();

    if (!targetItem)
    {
        // No targets selected, create new FragmentIDs

        if (!m_copiedFragmentIDs.empty())
        {
            if (!advancedPaste)
            {
                for (auto& copiedFragmentID : m_copiedFragmentIDs)
                {
                    PasteDefinition(copiedFragmentID);
                }
            }
        }
    }
    else
    {
        // Paste fragments

        if (!m_copiedItems.empty())
        {
            PasteFragments(targetItem, m_copiedItems, advancedPaste);
        }

        for (auto& copiedFragmentID : m_copiedFragmentIDs)
        {
            PasteFragments(targetItem, copiedFragmentID.copiedItems, advancedPaste);
        }
    }
}

void CFragmentBrowser::PasteDefinition(const SCopyItemFragmentID& copiedFragmentID)
{
    // create a fragment with the same name as the original fragment and a numeric suffix (e.g. Idle1)

    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    IMannequinEditorManager* pManEditMan = mannequinSys.GetMannequinEditorManager();

    // extract name and suffix

    QRegExp re("^(.*\\D)(\\d+)$");

    QString baseName;
    int suffix;

    if (re.indexIn(copiedFragmentID.name) != -1)
    {
        baseName = re.cap(1);
        suffix = re.cap(2).toInt() + 1;
    }
    else
    {
        baseName = copiedFragmentID.name;
        suffix = 1;
    }

    // keep trying until we have a valid name

    EModifyFragmentIdResult result;
    QString fragmentName;

    while (true)
    {
        fragmentName = QStringLiteral("%1%2").arg(baseName).arg(suffix);
        result = pManEditMan->CreateFragmentID(m_animDB->GetFragmentDefs(), fragmentName.toUtf8().data());

        if (result != eMFIR_DuplicateName)
        {
            break;
        }

        ++suffix;
    }

    if (result == eMFIR_Success)
    {
        // fill in fragmentDef information from original FragmentID

        const FragmentID fragmentID = m_animDB->GetFragmentID(fragmentName.toUtf8().data());

        pManEditMan->SetFragmentDef(*m_context->m_controllerDef, fragmentID, copiedFragmentID.fragmentDef);

        const CTagDefinition* pTagDef;
        if (!copiedFragmentID.subTagDefinitionFilename.empty())
        {
            pTagDef = mannequinSys.GetAnimationDatabaseManager().LoadTagDefs(copiedFragmentID.subTagDefinitionFilename.c_str(), true);
        }
        else
        {
            pTagDef = nullptr;
        }

        pManEditMan->SetFragmentTagDef(m_context->m_controllerDef->m_fragmentIDs, fragmentID, pTagDef);

        QTreeWidgetItem* item = AddFragmentIDItem(fragmentName);
        item->setExpanded(true);

        // paste fragments

        PasteFragments(item, copiedFragmentID.copiedItems, false);
    }
}

void CFragmentBrowser::PasteFragments(const QTreeWidgetItem* targetItem, const std::vector<SCopyItem>& copiedItems, bool advancedPaste)
{
    const STreeFragmentDataPtr pFragmentDataTo = targetItem->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();

    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    const FragmentID destFragmentID = pFragmentDataTo ? pFragmentDataTo->fragID : m_animDB->GetFragmentID(targetItem->text(0).toUtf8().data());
    SFragTagState copyBaseTagState = pFragmentDataTo ? pFragmentDataTo->tagState : SFragTagState();

    if (advancedPaste)
    {
        // Advanced pasting: let the user change the destination base tag state
        CMannAdvancedPasteDialog advancedPasteDialog(*m_animDB, destFragmentID, copyBaseTagState, this);
        if (advancedPasteDialog.exec() == QDialog::Accepted)
        {
            copyBaseTagState = advancedPasteDialog.GetTagState();
        }
        else
        {
            return;
        }
    }

    for (TCopiedItems::const_iterator itCopiedItems = copiedItems.begin(); itCopiedItems != copiedItems.end(); ++itCopiedItems)
    {
        if (destFragmentID != FRAGMENT_ID_INVALID)
        {
            // Convert tag state
            // For fragment tags, we need to do a CRC look-up since the tag definitions might not be the exact same
            SFragTagState insertionTagState = copyBaseTagState;
            const CTagDefinition& globalTagDef = m_animDB->GetTagDefs();
            const CTagDefinition* pFragTagDefDest = m_animDB->GetFragmentDefs().GetSubTagDefinition(destFragmentID);
            const CTagDefinition* pFragTagDefSource = m_animDB->GetFragmentDefs().GetSubTagDefinition(destFragmentID);

            const auto transferTags = [](const CTagDefinition& sourceDef, const CTagDefinition& destDef, const TagState& sourceTagState, TagState& destTagState)
            {
                const TagID numSourceTags = sourceDef.GetNum();
                for (TagID itSourceTags = 0; itSourceTags < numSourceTags; ++itSourceTags)
                {
                    if (sourceDef.IsSet(sourceTagState, itSourceTags))
                    {
                        // Tag set in the source state, set it in the destination only if there's no tag set within the same group
                        const TagID destTagID = destDef.Find(sourceDef.GetTagCRC(itSourceTags));
                        const TagGroupID groupId = destTagID != TAG_ID_INVALID ? destDef.GetGroupID(destTagID) : GROUP_ID_NONE;
                        const bool nonExclusive = groupId == GROUP_ID_NONE || !destDef.IsGroupSet(destTagState, groupId);
                        if (destTagID != TAG_ID_INVALID && nonExclusive)
                        {
                            destDef.Set(destTagState, destTagID, true);
                        }
                    }
                }
            };
            transferTags(globalTagDef, globalTagDef, itCopiedItems->tagState.globalTags, insertionTagState.globalTags);
            if (pFragTagDefSource && pFragTagDefDest)
            {
                transferTags(*pFragTagDefSource, *pFragTagDefDest, itCopiedItems->tagState.fragmentTags, insertionTagState.fragmentTags);
            }

            const uint32 newOption = mannequinSys.GetMannequinEditorManager()->AddFragmentEntry(m_animDB, destFragmentID, insertionTagState, itCopiedItems->fragment);

            const bool isLast = itCopiedItems + 1 == copiedItems.end();
            if (isLast)
            {
                CleanFragment(destFragmentID);
                BuildFragment(destFragmentID);
                SelectFragment(destFragmentID, insertionTagState, newOption);
            }
        }
    }
}

void CFragmentBrowser::SetEditItem(QTreeWidgetItem* editedItem, FragmentID fragmentID, const SFragTagState& tagState, uint32 option)
{
    if (m_editedItem)
    {
        QFont f = m_editedItem->font(0);
        f.setBold(false);
        m_editedItem->setFont(0, f);
    }

    m_editedItem = editedItem;
    if (editedItem)
    {
        QFont f = m_editedItem->font(0);
        f.setBold(true);
        m_editedItem->setFont(0, f);
    }
    m_fragmentID = fragmentID;
    m_tagState = tagState;
    m_option = option;
}

void CFragmentBrowser::SetADBFileNameTextToCurrent()
{
    QTreeWidgetItem* rootItem = GetSelectedItem();
    if (rootItem)
    {
        while (rootItem->parent() != NULL)
        {
            rootItem = rootItem->parent();
        }

        QString IDName = rootItem->text(0);
        const char* filename = m_animDB == NULL ? "" : m_animDB->FindSubADBFilenameForID(m_animDB->GetFragmentID(IDName.toUtf8().data()));

        QString adbName = Path::GetFile(filename);
        m_CurrFile->setText(adbName);
    }
}

void CFragmentBrowser::OnSelectionChanged()
{
    UpdateControlEnabledStatus();
    SetADBFileNameTextToCurrent();
}

void CFragmentBrowser::OnChangeFilterTags(const QString& filter)
{
    if (filter.isEmpty() || (filter[0] != DEFAULT_TAG_FILTER_TEXT[0] && filter != m_filterText))
    {
        m_filterText = filter;
        m_filterDelay = FILTER_DELAY_SECONDS;

        int Position = 0;
        QString token;

        m_filters = filter.split(" ");
        for (int i = 0; i < m_filters.size(); ++i)
        {
            m_filters[i] = m_filters[i].toLower();
        }

        // The RebuildAll() is now called in Update() when the filter delay reaches 0
    }

    if (filter.isEmpty())
    {
        m_editFilterTags->setText(DEFAULT_TAG_FILTER_TEXT);
        m_editFilterTags->selectAll();
    }
}

void CFragmentBrowser::OnChangeFilterFragments(const QString& filter)
{
    if (filter.isEmpty() || (filter[0] != DEFAULT_FRAGMENT_ID_FILTER_TEXT[0] && filter != m_filterFragmentIDText))
    {
        m_filterFragmentIDText = filter.toLower();
        m_filterDelay = FILTER_DELAY_SECONDS;

        // The RebuildAll() is now called in Update() when the filter delay reaches 0
    }

    if (filter.isEmpty())
    {
        m_editFilterFragmentIDs->setText(DEFAULT_FRAGMENT_ID_FILTER_TEXT);
        m_editFilterFragmentIDs->selectAll();
    }
}

void CFragmentBrowser::OnChangeFilterAnimClip(const QString& filter)
{
    if (filter.isEmpty() || (filter[0] != DEFAULT_ANIM_CLIP_FILTER_TEXT[0] && filter != m_filterAnimClipText))
    {
        m_filterAnimClipText = filter.toLower();
        m_filterDelay = FILTER_DELAY_SECONDS;

        // The RebuildAll() is now called in Update() when the filter delay reaches 0
    }

    if (filter.isEmpty())
    {
        m_editAnimClipFilter->setText(DEFAULT_ANIM_CLIP_FILTER_TEXT);
        m_editAnimClipFilter->selectAll();
    }
}

void CFragmentBrowser::OnToggleSubfolders()
{
    bool newCheck = m_chkShowSubFolders->isChecked();

    if (newCheck != m_showSubFolders)
    {
        m_showSubFolders = newCheck;
        RebuildAll();
    }
}

void CFragmentBrowser::OnToggleShowEmpty()
{
    bool newCheck = m_chkShowEmptyFolders->isChecked();

    if (newCheck != m_showEmptyFolders)
    {
        m_showEmptyFolders = newCheck;
        RebuildAll();
    }
}

void CFragmentBrowser::OnTagDefEditorBtn()
{
    CMannequinDialog::GetCurrentInstance()->LaunchTagDefinitionEditor();
}

FragmentID CFragmentBrowser::GetSelectedFragmentId() const
{
    const QTreeWidgetItem* item = GetSelectedItem();
    if (!item)
    {
        return FRAGMENT_ID_INVALID;
    }

    return GetItemFragmentId(item);
}

FragmentID CFragmentBrowser::GetItemFragmentId(const QTreeWidgetItem* item) const
{
    auto fragmentData = item->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();

    if (fragmentData == nullptr)
    {
        auto it = std::find(m_fragmentItems.begin(), m_fragmentItems.end(), item);

        if (it != m_fragmentItems.end())
        {
            return static_cast<FragmentID>(std::distance(m_fragmentItems.begin(), it));
        }
    }
    else
    {
        return fragmentData->fragID;
    }

    return FRAGMENT_ID_INVALID;
}

QString CFragmentBrowser::GetSelectedNodeText() const
{
    if (const QTreeWidgetItem* item = GetSelectedItem())
    {
        return item->text(0);
    }
    else
    {
        return "";
    }
}

void CFragmentBrowser::SetScopeContext(int scopeContextID)
{
    const bool rangeValid = (0 <= scopeContextID) && (scopeContextID < m_context->m_contextData.size());
    const int modifiedScopeContextID = rangeValid ? scopeContextID : -1;

    if (modifiedScopeContextID != m_scopeContext)
    {
        m_scopeContext = modifiedScopeContextID;

        IAnimationDatabase* const pAnimationDatabase = rangeValid ? m_context->m_contextData[modifiedScopeContextID].database : NULL;

        SetDatabase(pAnimationDatabase);

        m_cbContext->setCurrentIndex(modifiedScopeContextID);

        if (m_onScopeContextChangedCallback)
        {
            m_onScopeContextChangedCallback();
        }
    }
}

bool CFragmentBrowser::TagStateMatches(FragmentID fragmentID, const SFragTagState& parentTags, const SFragTagState& childTags) const
{
    // check if global tags match

    const CTagDefinition& globalTagDef = m_animDB->GetTagDefs();

    if (parentTags.globalTags.IsEmpty())
    {
        if (!childTags.globalTags.IsEmpty())
        {
            return false;
        }
    }
    else
    {
        if (!globalTagDef.Contains(childTags.globalTags, parentTags.globalTags))
        {
            return false;
        }
    }

    // check if fragment tags match

    const CTagDefinition* fragmentTagDef = m_animDB->GetFragmentDefs().GetSubTagDefinition(fragmentID);
    if (fragmentTagDef)
    {
        if (parentTags.fragmentTags.IsEmpty())
        {
            if (!childTags.fragmentTags.IsEmpty())
            {
                return false;
            }
        }
        else
        {
            if (!fragmentTagDef->Contains(childTags.fragmentTags, parentTags.fragmentTags))
            {
                return false;
            }
        }
    }

    return true;
}

void CFragmentBrowser::OnDelete()
{
    if (!m_animDB)
    {
        return;
    }

    auto selectedItems = UniqueSelectedItems();

    // sort items by Fragment ID name, so that the list of fragments to be deleted will appear sorted

    const CTagDefinition& fragDefs = m_animDB->GetFragmentDefs();

    std::stable_sort(selectedItems.begin(), selectedItems.end(),
            [&](const QTreeWidgetItem* lhs, const QTreeWidgetItem* rhs)
            {
                return strcmp(fragDefs.GetTagName(GetItemFragmentId(lhs)), fragDefs.GetTagName(GetItemFragmentId(rhs))) < 0;
            });

    std::vector<FragmentID> fragmentIDsToDelete;
    std::vector<STreeFragmentDataPtr> fragmentsToDelete;
    std::vector<QString> pathsToDelete;

    for (QTreeWidgetItem* item : selectedItems)
    {
        const auto ItemPath = [&](const QTreeWidgetItem* item)
        {
            QString path = item->text(0);
            item = item->parent();

            while (item != nullptr)
            {
                path = QStringLiteral("%1/%2").arg(item->text(0)).arg(path);
                item = item->parent();
            }

            return path;
        };

        STreeFragmentDataPtr fragmentData = item->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();
        if (fragmentData)
        {
            if (!fragmentData->tagSet)
            {
                // delete a fragment

                fragmentsToDelete.push_back(fragmentData);
                pathsToDelete.push_back(ItemPath(item));
            }
            else
            {
                // delete a tag set, find child fragments that contain these tags

                const auto& tagState = fragmentData->tagState;

                for (auto childFragmentData : m_fragmentData)
                {
                    if (!childFragmentData->tagSet && childFragmentData->fragID == fragmentData->fragID)
                    {
                        if (TagStateMatches(fragmentData->fragID, tagState, childFragmentData->tagState))
                        {
                            fragmentsToDelete.push_back(childFragmentData);
                            pathsToDelete.push_back(ItemPath(childFragmentData->item));
                        }
                    }
                }
            }
        }
        else
        {
            // delete a Fragment ID

            auto it = std::find(m_fragmentItems.begin(), m_fragmentItems.end(), item);
            if (it != m_fragmentItems.end())
            {
                FragmentID fragmentID = static_cast<FragmentID>(std::distance(m_fragmentItems.begin(), it));
                fragmentIDsToDelete.push_back(fragmentID);

                bool fragmentsFound = false;

                for (auto fragmentData : m_fragmentData)
                {
                    if (!fragmentData->tagSet && fragmentData->fragID == fragmentID)
                    {
                        pathsToDelete.push_back(ItemPath(fragmentData->item));
                        fragmentsFound = true;
                    }
                }

                if (fragmentsFound == false)
                {
                    pathsToDelete.push_back(fragDefs.GetTagName(fragmentID));
                }
            }
        }
    }

    if (fragmentIDsToDelete.empty() && fragmentsToDelete.empty())
    {
        return;
    }

    QString message = tr("The following fragments will be deleted. Are you sure you want to continue?\r\n\r\n");

    for (const auto& path : pathsToDelete)
    {
        message += QStringLiteral("%1\r\n").arg(path);
    }

    if (QMessageBox::question(this, "Delete fragments", message) != QMessageBox::Yes)
    {
        return;
    }

    m_fragEditor.FlushChanges();

    DeleteFragments(fragmentsToDelete);

    for (auto fragmentID : fragmentIDsToDelete)
    {
        DeleteDefinition(fragmentID);
    }

    SelectFragment(m_fragmentID, m_tagState, m_option);
    EditSelectedItem();
}

void CFragmentBrowser::DeleteFragments(const std::vector<STreeFragmentData*> fragments)
{
    // group fragments by fragmentID and tagState

    struct TagStateOptions
    {
        SFragTagState tagState;
        std::vector<uint32> indices;
    };

    std::map<FragmentID, std::vector<TagStateOptions>> fragmentMap;

    for (auto fragment : fragments)
    {
        auto& options = fragmentMap[fragment->fragID];

        auto it = std::find_if(
                    options.begin(),
                    options.end(),
                    [&](const TagStateOptions& entry)
                    {
                        return entry.tagState == fragment->tagState;
                    });

        if (it != options.end())
        {
            it->indices.push_back(fragment->option);
        }
        else
        {
            options.push_back({ fragment->tagState, { fragment->option } });
        }
    }

    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

    for (auto& fragmentOptions : fragmentMap)
    {
        FragmentID fragmentID = fragmentOptions.first;

        for (auto& options : fragmentOptions.second)
        {
            mannequinSys.GetMannequinEditorManager()->DeleteFragmentEntries(m_animDB, fragmentID, options.tagState, options.indices);
        }

        CleanFragment(fragmentID);
        BuildFragment(fragmentID);
    }
}

void CFragmentBrowser::EditSelectedItem()
{
    EditItem(GetSelectedItem());
}

void CFragmentBrowser::EditItem(QTreeWidgetItem* item)
{
    if (item)
    {
        STreeFragmentDataPtr pFragData = item->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();

        if (pFragData)
        {
            SetEditItem(item, pFragData->fragID, pFragData->tagState, pFragData->option);
            m_onSelectedItemEditCallback(pFragData->fragID, GetItemText(item));
            CMannequinDialog::GetCurrentInstance()->ShowPane<CFragmentEditorPage*>();
            m_fragEditor.SetFragment(pFragData->fragID, pFragData->tagState, pFragData->option);
            SelectFirstKey();
            m_fragEditor.update();
        }
    }
    else
    {
        m_editedItem = nullptr;
        m_fragmentID = FRAGMENT_ID_INVALID;
        m_tagState = SFragTagState();
        m_option = 0;

        m_onSelectedItemEditCallback(FRAGMENT_ID_INVALID, QString());
    }
}

QString CFragmentBrowser::GetItemText(const QTreeWidgetItem* item) const
{
    if (m_showSubFolders)
    {
        // We don't want the "Option 1" bit...
        item = item->parent();
    }

    // Get the tag name
    QString tags = item->text(0);
    item = item->parent();

    // Build a string of "+" separated tag names (omitting the root parent)
    while (item && item->parent() != NULL)
    {
        tags = item->text(0) + "+" + tags;
        item = item->parent();
    }

    return tags;
}

void CFragmentBrowser::OnEditBtn()
{
    EditSelectedItem();
}

void CFragmentBrowser::CleanFragmentID(FragmentID fragID)
{
    // First remove the child fragment nodes
    CleanFragment(fragID);

    // Now the parent FragmentID
    QTreeWidgetItem* fragmentDefItem = m_fragmentItems[fragID];
    if (m_editedItem == fragmentDefItem)
    {
        m_editedItem = nullptr;
    }
    delete fragmentDefItem;
    stl::find_and_erase(m_fragmentItems, fragmentDefItem);
    if (m_fragmentID == fragID)
    {
        m_fragmentID = FRAGMENT_ID_INVALID;
    }

    for (size_t i = 0; i < m_fragmentData.size(); i++)
    {
        STreeFragmentDataPtr treeFragData = m_fragmentData[i];
        if (treeFragData->fragID > fragID)
        {
            treeFragData->fragID--;
        }
    }

    OnSelectionChanged(); // deleting the last item in the treeview doesn't call OnTVSelectChange
}

void CFragmentBrowser::CleanFragment(FragmentID fragID)
{
    //--- Remove old data
    const int numFragDatas = m_fragmentData.size();
    for (int i = numFragDatas - 1; i >= 0; i--)
    {
        STreeFragmentDataPtr treeFragData = m_fragmentData[i];
        if (treeFragData->fragID == fragID)
        {
            if (m_editedItem == treeFragData->item)
            {
                m_editedItem = nullptr;
            }
            delete treeFragData->item;

            delete treeFragData;
            m_fragmentData.erase(m_fragmentData.begin() + i);
        }
    }
}

QTreeWidgetItem* CFragmentBrowser::FindInChildren(const QString& szTagStr, QTreeWidgetItem* hCurrItem)
{
    for (int i = 0; i < hCurrItem->childCount(); ++i)
    {
        QTreeWidgetItem* hChildItem = hCurrItem->child(i);
        QString name = hChildItem->text(0);
        if (name == szTagStr)
        {
            return hChildItem;
        }
    }
    return NULL;
}


// helper function
struct sCompareTagPrio
{
    typedef std::pair<int, QString> TTagPair;
    const bool operator() (const TTagPair& p1, const TTagPair& p2) const { return p1.first < p2.first; }
};


void CFragmentBrowser::BuildFragment(FragmentID fragID)
{
    QTreeWidgetItem* fragItem = m_fragmentItems[fragID];

    const uint32 numFilters = m_filters.size();

    //--- Insert new data
    QString szTagStateBuff;
    const uint32 numTagSets = m_animDB->GetTotalTagSets(fragID);
    for (uint32 t = 0; t < numTagSets; t++)
    {
        SFragTagState tagState;
        uint32 numOptions = m_animDB->GetTagSetInfo(fragID, t, tagState);
        MannUtils::FlagsToTagList(szTagStateBuff, tagState, fragID, *m_context->m_controllerDef);
        QString sTagState (szTagStateBuff);

        bool include = true;
        if (numFilters > 0)
        {
            QString sTagStateLC = sTagState.toLower();

            for (uint32 f = 0; f < numFilters; f++)
            {
                if (!sTagStateLC.contains(m_filters[f]))
                {
                    include = false;
                    break;
                }
            }
        }

        if (m_filterAnimClipText.length() > 0)
        {
            bool bMatch = false;
            for (uint32 o = 0; o < numOptions && !bMatch; o++)
            {
                const CFragment* fragment = m_animDB->GetEntry(fragID, t, o);

                uint32 numALayers = fragment->m_animLayers.size();
                uint32 numPLayers = fragment->m_procLayers.size();
                for (uint32 ali = 0; ali < numALayers; ali++)
                {
                    const TAnimClipSequence& animLayer = fragment->m_animLayers[ali];
                    const uint32 numClips = animLayer.size();
                    for (uint32 c = 0; c < numClips; c++)
                    {
                        const SAnimClip& animClip = animLayer[c];
                        if (QString(animClip.animation.animRef.c_str()).contains(m_filterAnimClipText, Qt::CaseInsensitive))
                        {
                            bMatch = true;
                            break;
                        }
                    }
                }
                // -- allow searching of proc clips by clip name and by extra debug info
                for (uint32 pli = 0; pli < numPLayers; pli++)
                {
                    const TProcClipSequence& procLayer = fragment->m_procLayers[pli];
                    const uint32 numClips = procLayer.size();
                    for (uint32 c = 0; c < numClips; c++)
                    {
                        const SProceduralEntry& procClip = procLayer[c];
                        string name;
                        if (!procClip.typeNameHash.IsEmpty())
                        {
                            IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
                            name = mannequinSys.GetProceduralClipFactory().FindTypeName(procClip.typeNameHash);
                            IProceduralParams::StringWrapper debugInfo;
                            if (procClip.pProceduralParams)
                            {
                                procClip.pProceduralParams->GetExtraDebugInfo(debugInfo);
                                name += debugInfo.c_str();
                            }
                            name.MakeLower();
                        }
                        if (!name.empty() && QString(name.c_str()).contains(m_filterAnimClipText, Qt::CaseInsensitive))
                        {
                            bMatch = true;
                            break;
                        }
                    }
                }
            }
            if (!bMatch)
            {
                // Anim clip filter was specified, but didn't match anything in this fragment,
                // so don't add fragment to tree hierarchy.
                include = false;
            }
        }

        if (!include)
        {
            continue;
        }

        QTreeWidgetItem* parent = fragItem;

        typedef std::vector < sCompareTagPrio::TTagPair > TTags;
        TTags vTags;

        const bool noFolderForOptions = !m_showSubFolders && (numOptions < 2);

        if (m_showSubFolders)
        {
            bool bDone = sTagState.isEmpty();
            while (!bDone)
            {
                // Separate the tag strings by '+'
                int nPlusIdx = sTagState.indexOf('+');

                QString sSingleTagString;
                // None found - just copy the string into the buffer
                if (-1 == nPlusIdx)
                {
                    sSingleTagString = sTagState;
                    bDone = true;
                }
                // Found: split the string, copy the first name into the buffer
                else
                {
                    sSingleTagString = sTagState.left(nPlusIdx);
                    sTagState = sTagState.right(sTagState.length() - nPlusIdx - 1);
                }

                const CTagDefinition& tagDefs = m_animDB->GetTagDefs();
                TagID id = tagDefs.Find(sSingleTagString.toUtf8().data());
                if (TAG_ID_INVALID == id)
                {
                    const CTagDefinition* fragTagDefs = m_context->m_controllerDef->GetFragmentTagDef(fragID);
                    if (fragTagDefs)
                    {
                        id = fragTagDefs->Find(sSingleTagString.toUtf8().data());
                        if (id != TAG_ID_INVALID)
                        {
                            vTags.push_back(std::make_pair(fragTagDefs->GetPriority(id), sSingleTagString));
                        }
                    }
                }
                else
                {
                    vTags.push_back(std::make_pair(tagDefs.GetPriority(id), sSingleTagString));
                }
            }
        }
        else if (!sTagState.isEmpty() && !noFolderForOptions)
        {
            vTags.push_back(std::make_pair(0, sTagState));
        }

        if (!vTags.empty())
        {
            std::sort (vTags.rbegin(), vTags.rend(), sCompareTagPrio());

            for (TTags::const_iterator it = vTags.begin(); it != vTags.end(); ++it)
            {
                // Search for the tag string under the current node.
                // If found, use that node as a parent. If not, create a new one for that tag
                QTreeWidgetItem* newParent = FindInChildren (it->second, parent);
                if (newParent)
                {
                    parent = newParent;
                }
                else
                {
                    QTreeWidgetItem* oldParent = parent;
                    parent = new QTreeWidgetItem(oldParent);
                    parent->setText(0, it->second);
                    parent->setIcon(0, m_imageList[IMAGE_FOLDER]);

                    const STreeFragmentDataPtr pOldParentData = oldParent->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();
                    SFragTagState itemTagState = pOldParentData ? pOldParentData->tagState : SFragTagState();

                    const QString& tagName = it->second;
                    const CTagDefinition& globalTagDef = m_animDB->GetTagDefs();
                    const CTagDefinition* pFragTagDef = m_animDB->GetFragmentDefs().GetSubTagDefinition(fragID);
                    const TagID globalTagId = globalTagDef.Find(tagName.toUtf8().data());
                    const TagID fragmentTagID = pFragTagDef ? pFragTagDef->Find(tagName.toUtf8().data()) : TAG_ID_INVALID;
                    if (fragmentTagID != TAG_ID_INVALID)
                    {
                        pFragTagDef->Set(itemTagState.fragmentTags, fragmentTagID, true);
                    }
                    else
                    {
                        globalTagDef.Set(itemTagState.globalTags, globalTagId, true);
                    }

                    STreeFragmentDataPtr fragmentData = new STreeFragmentData();
                    fragmentData->fragID                  = fragID;
                    fragmentData->tagState            = itemTagState;
                    fragmentData->item                    = parent;
                    fragmentData->tagSet                    = true;
                    m_fragmentData.push_back(fragmentData);
                    QVariant v;
                    v.setValue<STreeFragmentDataPtr>(fragmentData);
                    parent->setData(0, Qt::UserRole, v);
                }
            }
        }
        else if (!noFolderForOptions)
        {
            parent = new QTreeWidgetItem(parent);
            parent->setText(0, MannUtils::GetEmptyTagString());
            parent->setIcon(0, m_imageList[IMAGE_FOLDER]);
            STreeFragmentDataPtr fragmentData = new STreeFragmentData();
            fragmentData->fragID                  = fragID;
            fragmentData->tagState            = tagState;
            fragmentData->item                    = parent;
            fragmentData->tagSet                    = true;
            m_fragmentData.push_back(fragmentData);
            QVariant v;
            v.setValue<STreeFragmentDataPtr>(fragmentData);
            parent->setData(0, Qt::UserRole, v);
        }

        for (uint32 o = 0; o < numOptions; o++)
        {
            const CFragment* fragment = m_animDB->GetEntry(fragID, t, o);

            QString nodeName;

            if (noFolderForOptions)
            {
                nodeName = szTagStateBuff;
            }
            else
            {
                nodeName = tr("Option %1").arg(o + 1);
            }

            QTreeWidgetItem* entry = new QTreeWidgetItem(parent);
            entry->setText(0, nodeName);
            entry->setIcon(0, m_imageList[IMAGE_ANIM]);
            entry->setFlags(entry->flags() & ~Qt::ItemIsDropEnabled);

            STreeFragmentDataPtr fragmentData = new STreeFragmentData();
            fragmentData->fragID                  = fragID;
            fragmentData->tagState            = tagState;
            fragmentData->option                    = o;
            fragmentData->item                    = entry;
            fragmentData->tagSet                    = false;
            m_fragmentData.push_back(fragmentData);
            QVariant v;
            v.setValue<STreeFragmentDataPtr>(fragmentData);
            entry->setData(0, Qt::UserRole, v);
        }
    }
}


uint32 CFragmentBrowser::AddNewFragment(FragmentID fragID, const SFragTagState& newTagState, const string& sAnimName)
{
    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

    ActionScopes scopeMask = m_context->m_controllerDef->GetScopeMask(fragID, newTagState);
    uint32 numScopes = m_context->m_controllerDef->m_scopeDef.size();
    uint32 newOption = OPTION_IDX_INVALID;

    // Add fragment on root scope (only)
    for (uint32 itScopes = 0; itScopes < numScopes; ++itScopes)
    {
        if (scopeMask & (1 << itScopes))
        {
            const SScopeDef& scopeDef = m_context->m_controllerDef->m_scopeDef[itScopes];
            SFragTagState tagState = newTagState;
            tagState.globalTags = m_context->m_controllerDef->m_tags.GetUnion(tagState.globalTags, scopeDef.additionalTags);

            IAnimationDatabase* pDatabase = FindHostDatabase(scopeDef.context, tagState);
            if (!sAnimName.empty())
            {
                CFragment newFrag;
                newFrag.m_animLayers.push_back(TAnimClipSequence());
                TAnimClipSequence& animSeq = newFrag.m_animLayers.back();

                animSeq.push_back(SAnimClip());
                SAnimClip& animClp = animSeq.back();
                animClp.animation.animRef.SetByString(sAnimName);

                newOption = mannequinSys.GetMannequinEditorManager()->AddFragmentEntry(pDatabase, fragID, tagState, newFrag);
            }
            else
            {
                newOption = mannequinSys.GetMannequinEditorManager()->AddFragmentEntry(pDatabase, fragID, tagState, CFragment());
            }

            break;
        }
    }

    CleanFragment(fragID);
    BuildFragment(fragID);

    SelectFragment(fragID, newTagState, newOption);
    return newOption;
}


void CFragmentBrowser::OnNewFragment()
{
    auto selectedItems = m_TreeCtrl->selectedItems();

    for (QTreeWidgetItem* item : selectedItems)
    {
        FragmentID fragID;
        SFragTagState newTagState;

        STreeFragmentDataPtr pFragData = item->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();
        if (pFragData)
        {
            if (!pFragData->tagSet)
            {
                continue;
            }

            fragID = pFragData->fragID;
            newTagState = pFragData->tagState;
        }
        else
        {
            auto it = std::find(m_fragmentItems.begin(), m_fragmentItems.end(), item);
            if (it == m_fragmentItems.end())
            {
                continue;
            }

            fragID = static_cast<FragmentID>(std::distance(m_fragmentItems.begin(), it));
        }

        IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
        uint32 newOption = mannequinSys.GetMannequinEditorManager()->AddFragmentEntry(m_animDB, fragID, newTagState, CFragment());

        CleanFragment(fragID);
        BuildFragment(fragID);

        SelectFragment(fragID, newTagState, newOption);
    }

    EditSelectedItem();
}

void CFragmentBrowser::OnNewDefinition()
{
    AddNewDefinition();
}

bool CFragmentBrowser::AddNewDefinition()
{
    if (!m_animDB)
    {
        return false;
    }

    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    IMannequinEditorManager* pManEditMan = mannequinSys.GetMannequinEditorManager();
    assert(pManEditMan);

    QString fragmentName;
    EModifyFragmentIdResult result = eMFIR_Success;

    // Keep retrying until we get a valid name or the user gives up
    do
    {
        QInputDialog fragmentNameDialog;
        fragmentNameDialog.setWindowTitle(tr("New ID"));
        fragmentNameDialog.setLabelText(tr("New FragmentID Name"));
        fragmentNameDialog.setWindowFlags(fragmentNameDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

        if (fragmentNameDialog.exec() != QDialog::Accepted)
        {
            return false;
        }

        fragmentName = fragmentNameDialog.textValue();

        result = pManEditMan->CreateFragmentID(m_animDB->GetFragmentDefs(), fragmentName.toUtf8().data());
        if (result != eMFIR_Success)
        {
            const QString commonErrorMessage = tr("Failed to create FragmentID with name '%1'.\n  Reason:\n\n  %2").arg(fragmentName);
            switch (result)
            {
            case eMFIR_DuplicateName:
            {
                QMessageBox::critical(this, QString(), commonErrorMessage.arg(tr("There is already a FragmentID with this name.")));
                break;
            }
            case eMFIR_InvalidNameIdentifier:
            {
                QMessageBox box(this);
                box.setText(commonErrorMessage.arg(tr("Invalid name identifier for a FragmentID.\n  A valid name can only use a-Z, A-Z, 0-9 and _ characters and cannot start with a digit.")));
                box.exec();
                break;
            }
            case eMFIR_UnknownInputTagDefinition:
            {
                QMessageBox box(this);
                box.setText(commonErrorMessage.arg(tr("Unknown input tag definition. Your changes cannot be saved.")));
                box.exec();
                break;
            }
            default:
                assert(false);
                return false;
            }
        }
    } while (result != eMFIR_Success);

    const FragmentID fragID = m_animDB->GetFragmentID(fragmentName.toUtf8().data());

    CMannTagEditorDialog tagEditorDialog(m_animDB, fragID, this);

    if (tagEditorDialog.exec() != QDialog::Accepted)
    {
        pManEditMan->DeleteFragmentID(m_animDB->GetFragmentDefs(), fragID);
        return false;
    }

    fragmentName = tagEditorDialog.FragmentName();

    QTreeWidgetItem* item = AddFragmentIDItem(fragmentName);

    CleanFragment(fragID);
    BuildFragment(fragID);

    //--- Autoselect the new node
    item->setExpanded(true);
    m_TreeCtrl->selectionModel()->clear();
    item->setSelected(true);

    return true;
}

QTreeWidgetItem* CFragmentBrowser::AddFragmentIDItem(const QString& fragmentName)
{
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, fragmentName);
    item->setIcon(0, m_imageList[1]);

    m_fragmentItems.push_back(item);

    m_TreeCtrl->addTopLevelItem(item);
    m_TreeCtrl->sortItems(0, Qt::AscendingOrder);

    return item;
}

static void CollectFilenamesOfSubADBsUsingFragmentIDInRule(const SMiniSubADB::TSubADBArray& subADBs, const FragmentID fragmentIDToFind, DynArray<string>& outDatabases)
{
    assert(fragmentIDToFind != FRAGMENT_ID_INVALID);

    for (const auto& subADB : subADBs)
    {
        for (const auto& fragID : subADB.vFragIDs)
        {
            if (fragID == fragmentIDToFind)
            {
                outDatabases.push_back(subADB.filename);
                break;
            }
        }

        CollectFilenamesOfSubADBsUsingFragmentIDInRule(subADB.vSubADBs, fragmentIDToFind, outDatabases);
    }
}

void CFragmentBrowser::DeleteDefinition(FragmentID fragmentID)
{
    IMannequin& mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
    IMannequinEditorManager* pManEditMan = mannequinSys.GetMannequinEditorManager();
    assert(pManEditMan);

    QString fragmentName = m_animDB->GetFragmentDefs().GetTagName(fragmentID);

    // Warn and bail out if a sub ADB rule uses this fragmentID
    {
        DynArray<string> foundFragmentIDRules;
        DynArray<const IAnimationDatabase*> databases;
        pManEditMan->GetLoadedDatabases(databases);

        const char* fragmentDefFilename = m_animDB->GetFragmentDefs().GetFilename();
        const uint32 fragmentDefFilenameCrc = CCrc32::ComputeLowercase(fragmentDefFilename);

        for (const IAnimationDatabase* pDatabase : databases)
        {
            if (!pDatabase)
            {
                continue;
            }

            const char* databaseFragmentDefFilename = pDatabase->GetFragmentDefs().GetFilename();
            const uint32 databaseFragmentDefFilenameCrc = CCrc32::ComputeLowercase(databaseFragmentDefFilename);
            const bool usingSameFragmentDef = (databaseFragmentDefFilenameCrc == fragmentDefFilenameCrc);
            if (usingSameFragmentDef)
            {
                SMiniSubADB::TSubADBArray subADBs;
                pDatabase->GetSubADBFragmentFilters(subADBs);

                CollectFilenamesOfSubADBsUsingFragmentIDInRule(subADBs, fragmentID, foundFragmentIDRules);
            }
        }

        if (!foundFragmentIDRules.empty())
        {
            QString message;
            message = QString("Unable to delete FragmentID '%1'.\n\nIt is still used in a sub ADB rule for\n").arg(fragmentName);
            for (const string& s : foundFragmentIDRules)
            {
                message += s;
                message += "\n";
            }

            QMessageBox::information(this, "Delete Failed", message);
            return;
        }
    }

    // Save the current previewer sequence to a temp file
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    tempFile.setFileTemplate(QDir::tempPath() + "/MNQXXXXXX.tmp");
    tempFile.open();
    QString tempFileName = tempFile.fileName();
    tempFile.remove();

    CMannequinDialog::GetCurrentInstance()->Previewer()->SaveSequence(tempFileName);

    // Delete the FragmentID
    EModifyFragmentIdResult result = pManEditMan->DeleteFragmentID(m_animDB->GetFragmentDefs(), fragmentID);

    if (result != eMFIR_Success)
    {
        QString commonErrorMessage = tr("Failed to delete FragmentID with name '%1'.\n  Reason:\n\n  %2").arg(fragmentName);
        switch (result)
        {
        case eMFIR_UnknownInputTagDefinition:
            QMessageBox::warning(this, "Unknown Tag Definition", commonErrorMessage.arg(tr("Unknown input tag definition. Your changes cannot be saved.")));
            break;

        case eMFIR_InvalidFragmentId:
            QMessageBox::warning(this, "Invalid Fragment", commonErrorMessage.arg(tr("Invalid FragmentID. Your changes cannot be saved.")));
            break;

        default:
            assert(false);
            break;
        }

        // Clean up the temp file
        QFile::remove(tempFileName);
        return;
    }

    CleanFragmentID(fragmentID);

    // Reload the previewer sequence from the temp file and clean up
    CMannequinDialog::GetCurrentInstance()->Previewer()->LoadSequence(tempFileName);
    QFile::remove(tempFileName);
    CMannequinDialog::GetCurrentInstance()->Previewer()->KeyProperties()->ForceUpdate();
}


void CFragmentBrowser::OnRenameDefinition()
{
    if (!m_animDB)
    {
        return;
    }

    const FragmentID selectedFragmentId = GetSelectedFragmentId();
    if (selectedFragmentId == FRAGMENT_ID_INVALID)
    {
        return;
    }

    CMannTagEditorDialog tagEditorDialog(m_animDB, selectedFragmentId, this);
    if (tagEditorDialog.exec() != QDialog::Accepted)
    {
        return;
    }

    QString fragmentName = tagEditorDialog.FragmentName();

    const QList<QTreeWidgetItem*> selectedItems = m_TreeCtrl->selectedItems();
    if (!selectedItems.isEmpty())
    {
        selectedItems.front()->setText(0, fragmentName);
    }

    CleanFragment(selectedFragmentId);
    BuildFragment(selectedFragmentId);

    OnSelectionChanged();
}

void CFragmentBrowser::SetOnSelectedItemEditCallback(OnSelectedItemEditCallback onSelectedItemEditCallback)
{
    m_onSelectedItemEditCallback = onSelectedItemEditCallback;
}

void CFragmentBrowser::SetOnScopeContextChangedCallback(OnScopeContextChangedCallback onScopeContextChangedCallback)
{
    m_onScopeContextChangedCallback = onScopeContextChangedCallback;
}

void CFragmentBrowser::SelectFirstKey()
{
    if (!m_fragEditor.SelectFirstKey(SEQUENCER_PARAM_ANIMLAYER))
    {
        if (!m_fragEditor.SelectFirstKey(SEQUENCER_PARAM_PROCLAYER))
        {
            m_fragEditor.SelectFirstKey();
        }
    }
}

bool CFragmentBrowser::IsValidAnimation(const string& animationName) const
{
    if (!m_context)
    {
        return false;
    }

    auto& contextData = m_context->m_contextData;

    auto it = std::find_if(
                contextData.begin(),
                contextData.end(),
                [&](SScopeContextData& contextData)
                {
                    return (contextData.animSet->GetAnimIDByName(animationName) != -1);
                });

    return (it != contextData.end());
}

FragmentID CFragmentBrowser::GetValidFragIDForAnim(const QTreeWidgetItem* item, SFragTagState& tagState, const string& animationName)
{
    if (!IsValidAnimation(animationName))
    {
        return FRAGMENT_ID_INVALID;
    }

    FragmentID fragID;

    STreeFragmentData* fragmentData = item->data(0, Qt::UserRole).value<STreeFragmentDataPtr>();
    if (fragmentData)
    {
        fragID = fragmentData->fragID;
        tagState = fragmentData->tagState;
    }
    else
    {
        QString IDName = item->text(0);
        fragID = m_animDB->GetFragmentID(IDName.toUtf8().data());
        tagState = SFragTagState();
    }

    return fragID;
}

//
bool CFragmentBrowser::AddAnimToFragment(FragmentID fragID, SFragTagState& tagState, const string& animationName)
{
    MannUtils::AppendTagsFromAnimName(animationName, tagState);
    AddNewFragment(fragID, tagState, animationName);

    return true;
}

#include <Mannequin/Controls/FragmentBrowser.moc>
