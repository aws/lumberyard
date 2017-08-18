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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_FRAGMENTBROWSER_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_FRAGMENTBROWSER_H
#pragma once


#include <ICryMannequin.h>
#include <QWidget>
#include <QScopedPointer>
#include <QComboBox>

class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QCheckBox;
namespace Ui {
    class CFragmentBrowser;
}

class CMannFragmentEditor;
struct SMannequinContexts;
class CFragmentBrowserTreeWidget;

class CFragmentBrowser
    : public QWidget
{
    Q_OBJECT
public:
    struct STreeFragmentData
    {
        FragmentID fragID;
        SFragTagState tagState;
        uint32 option;
        QTreeWidgetItem* item;
        bool tagSet;
    };


    CFragmentBrowser(CMannFragmentEditor& fragEditor, QWidget* pParent);
    virtual ~CFragmentBrowser();
    void Update(void);

    void SetContext(SMannequinContexts& context);

    void SetScopeContext(int scopeContextID);
    bool SelectFragment(FragmentID fragmentID, const SFragTagState& tagState, uint32 option = 0);
    bool SetTagState(const SFragTagState& newTags);

    typedef Functor2< FragmentID, const QString& > OnSelectedItemEditCallback;
    void SetOnSelectedItemEditCallback(OnSelectedItemEditCallback onSelectedItemEditCallback);

    typedef Functor0 OnScopeContextChangedCallback;
    void SetOnScopeContextChangedCallback(OnScopeContextChangedCallback onScopeContextChangedCallback);

    FragmentID GetSelectedFragmentId() const;
    QString GetSelectedNodeText() const;

    void SetADBFileNameTextToCurrent();

    int GetScopeContextId() const
    {
        return m_cbContext->currentIndex();
    }

    void RebuildAll();

    static CFragmentBrowser::STreeFragmentData* draggedFragment(const QMimeData* mimeData);
    static std::vector<QString> draggedAnimations(const QMimeData* mimeData);

public slots:
    void OnTagRenamed(const CTagDefinition* tagDef, TagID tagID, const QString& name);
    void OnTagRemoved(const CTagDefinition* tagDef, TagID tagID);
    void OnTagAdded(const CTagDefinition* tagDef, const QString& name);

protected:
    friend class CFragmentBrowserTreeWidget;
    BOOL OnInitDialog();

    void EditSelectedItem();
    void EditItem(QTreeWidgetItem* item);
    void SetEditItem(QTreeWidgetItem* editedItem, FragmentID fragmentID, const SFragTagState& tagState, uint32 option);
    void SetDatabase(IAnimationDatabase* animDB);

    QTreeWidgetItem* FindFragmentItem(FragmentID fragmentID, const SFragTagState& tagState, uint32 option) const;

    void CleanFragmentID(FragmentID fragID);
    void CleanFragment(FragmentID fragID);
    void BuildFragment(FragmentID fragID);
    QTreeWidgetItem* FindInChildren (const QString& szTagStr, QTreeWidgetItem* parent);


    // Drag / drop helpers
    bool IsValidAnimation(const string& animationName) const;
    FragmentID GetValidFragIDForAnim(const QTreeWidgetItem* item, SFragTagState& tagState, const string& animationName);
    bool AddAnimToFragment(FragmentID fragID, SFragTagState& tagState, const string& animationName);

    IAnimationDatabase* FindHostDatabase(uint32 contextID, const SFragTagState& tagState) const;

    QString GetItemText(const QTreeWidgetItem* item) const;
    FragmentID GetItemFragmentId(const QTreeWidgetItem* item) const;

protected slots:
    void OnTVRightClick(const QPoint& pos);
    void OnTVEditSel();
    void OnChangeFilterTags(const QString& filter);
    void OnChangeFilterFragments(const QString& filter);
    void OnChangeFilterAnimClip(const QString& filter);
    void OnToggleSubfolders();
    void OnToggleShowEmpty();
    void OnNewFragment();
    void OnEditBtn();
    void OnDelete();
    void OnTagDefEditorBtn();
    void OnRDrop(STreeFragmentData* pDragData, QTreeWidgetItem* hitDest);
    void OnNewDefinition();
    void OnRenameDefinition();

    void SelectFirstKey();

    uint32 AddNewFragment(FragmentID fragID, const SFragTagState& newTagState, const string& sAnimName);
private:
    struct SCopyItem
    {
        SCopyItem(const CFragment& _fragment, const SFragTagState _tagState)
            : fragment(_fragment)
            , tagState(_tagState){}
        CFragment fragment;
        SFragTagState tagState;
    };

    struct SCopyItemFragmentID
    {
        QString name;
        SFragmentDef fragmentDef;
        string subTagDefinitionFilename;
        std::vector<SCopyItem> copiedItems;
    };

    void UpdateControlEnabledStatus();
    void OnSelectionChanged();
    void OnMenuCopy();
    void OnMenuPaste(bool advancedPaste);
    bool AddNewDefinition();
    QTreeWidgetItem* AddFragmentIDItem(const QString& fragmentName);
    void CopyItem(const QTreeWidgetItem* item);
    void DeleteFragments(const std::vector<STreeFragmentData*> fragments);
    void DeleteDefinition(FragmentID fragmentID);
    void PasteDefinition(const SCopyItemFragmentID& copiedFragmentID);
    void PasteFragments(const QTreeWidgetItem* targetItem, const std::vector<SCopyItem>& copiedItems, bool advancedPaste);
    void CopyFragments(std::vector<SCopyItem>& copiedItems, FragmentID fragmentID, const SFragTagState* tagState, const SFragTagState* parentTagState);
    QTreeWidgetItem* GetSelectedItem();
    const QTreeWidgetItem* GetSelectedItem() const;
    bool FragmentHasTag(const STreeFragmentData* fragData, const CTagDefinition* tagDef, TagID tagID) const;
    bool TagStateMatches(FragmentID fragmentID, const SFragTagState& parentTags, const SFragTagState& childTags) const;
    QList<QTreeWidgetItem*> UniqueSelectedItems() const;

private:
    UINT m_nFragmentClipboardFormat;

    QTreeWidget* m_TreeCtrl;
    QComboBox* m_cbContext;
    QLineEdit* m_editFilterTags;
    QLineEdit* m_editFilterFragmentIDs;
    QLineEdit* m_editAnimClipFilter;
    QCheckBox* m_chkShowSubFolders;

    QCheckBox* m_chkShowEmptyFolders;
    QPushButton* m_contextMenuButton;

    bool m_showSubFolders;
    bool m_showEmptyFolders;

    QString m_filterText;
    QStringList m_filters;

    QString m_filterFragmentIDText;

    QString m_filterAnimClipText;

    IAnimationDatabase* m_animDB;
    CMannFragmentEditor& m_fragEditor;

    QTreeWidgetItem* m_editedItem;

    bool m_rightDrag;

    QTreeWidgetItem* m_dragItem;

    typedef std::vector<SCopyItem> TCopiedItems;
    TCopiedItems m_copiedItems;

    std::vector<SCopyItemFragmentID> m_copiedFragmentIDs;

    QVector<QIcon> m_imageList;
    QLineEdit*              m_CurrFile;

    SMannequinContexts* m_context;
    int m_scopeContext;

    FragmentID m_fragmentID;
    SFragTagState m_tagState;
    uint32 m_option;

    typedef std::vector<STreeFragmentData*> TFragmentData;
    TFragmentData m_fragmentData;

    std::vector<QTreeWidgetItem*> m_fragmentItems;

    float m_filterDelay;

    OnSelectedItemEditCallback m_onSelectedItemEditCallback;

    OnScopeContextChangedCallback m_onScopeContextChangedCallback;

    QScopedPointer<Ui::CFragmentBrowser> ui;
    QAction* m_newFragmentAction;
    QAction* m_newFragmentIdAction;
    QAction* m_deleteAction;
    QAction* m_editFragmentIdAction;
    QAction* m_tagDefinitionEditorAction;
    QAction* m_copyAction;
    QAction* m_pasteAction;
    QAction* m_pasteIntoAction;
    QAction* m_launchTagDefinitionEditorAction;
};

using STreeFragmentDataPtr = CFragmentBrowser::STreeFragmentData *;
Q_DECLARE_METATYPE(STreeFragmentDataPtr);

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_FRAGMENTBROWSER_H
