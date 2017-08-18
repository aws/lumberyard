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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_TRANSITIONBROWSER_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_TRANSITIONBROWSER_H
#pragma once


#include <ICryMannequin.h>

#include <QDockWidget>
#include <QTreeWidget>
#include <QScopedPointer>

namespace Ui {
    class CTransitionBrowser;
}


struct SMannequinContexts;
class PreviewerPage;
class CTransitionEditorPage;
class CTransitionBrowser;
class QComboBox;
class QPushButton;
class QLineEdit;
class QToolButton;

struct TTransitionID
{
    TTransitionID()
        : fromID(FRAGMENT_ID_INVALID)
        , toID(FRAGMENT_ID_INVALID)
    {
    }

    TTransitionID(const FragmentID fID, const FragmentID tID, const SFragTagState& fFragTag, const SFragTagState& tFragTag, const SFragmentBlendUid _uid)
        : fromID(fID)
        , toID(tID)
        , fromFTS(fFragTag)
        , toFTS(tFragTag)
        , uid(_uid)
    {
    }

    TTransitionID(const TTransitionID& transitionID)
        : fromID(transitionID.fromID)
        , toID(transitionID.toID)
        , fromFTS(transitionID.fromFTS)
        , toFTS(transitionID.toFTS)
        , uid(transitionID.uid)
    {
    }

    bool operator==(const TTransitionID& rhs) const
    {
        // If the uids match, the other fields should also match:
        CRY_ASSERT((uid != rhs.uid) || (fromID == rhs.fromID && toID == rhs.toID &&
                                        fromFTS == rhs.fromFTS && toFTS == rhs.toFTS));

        return uid == rhs.uid;
    }

    bool IsSameBlendVariant(const TTransitionID& rhs) const
    {
        return (fromID == rhs.fromID && toID == rhs.toID &&
                fromFTS == rhs.fromFTS && toFTS == rhs.toFTS);
    }

    FragmentID fromID, toID;
    SFragTagState fromFTS, toFTS;
    SFragmentBlendUid uid;
};

//////////////////////////////////////////////////////////////////////////
// Item class for browser.
//////////////////////////////////////////////////////////////////////////
class CTransitionBrowserRecord
    : public QTreeWidgetItem
{
public:
    struct SRecordData
    {
        SRecordData()
        {
        }
        SRecordData(const QString& from, const QString& to);
        SRecordData(const TTransitionID& transitionID, float selectTime);

        bool operator==(const SRecordData& rhs) const
        {
            if (m_bFolder != rhs.m_bFolder)
            {
                return false;
            }

            if (m_bFolder)
            {
                return (m_from == rhs.m_from) && (m_to == rhs.m_to);
            }
            else
            {
                return (m_transitionID == rhs.m_transitionID);
            }
        }

        bool operator!=(const SRecordData& rhs) const
        {
            return !operator==(rhs);
        }

        QString m_from;
        QString m_to;
        bool m_bFolder;
        TTransitionID m_transitionID; // only stored for the non-folders
    };

public:
    // create a folder
    CTransitionBrowserRecord(const QString& from, const QString& to);

    // create an entry that is not a folder
    CTransitionBrowserRecord(const TTransitionID& transitionID, float selectTime);

    inline CTransitionBrowserRecord* GetParent()
    {
        return (CTransitionBrowserRecord*)parent();
    }

    const TTransitionID& GetTransitionID() const;
    inline const SRecordData& GetRecordData() const
    {
        return m_data;
    }

    inline bool IsFolder() const
    {
        return m_data.m_bFolder;
    }

    bool IsEqualTo(const TTransitionID& transitionID) const;
    bool IsEqualTo(const SRecordData& data) const;

private:
    void CreateItems();

    SRecordData m_data;
};
typedef std::vector<CTransitionBrowserRecord::SRecordData> TTransitionBrowserRecordDataCollection;

//////////////////////////////////////////////////////////////////////////
// Tree class for browser.
//////////////////////////////////////////////////////////////////////////
class CTransitionBrowserTreeCtrl
    : public QTreeWidget
{
public:
    typedef CTransitionBrowserRecord CRecord;

public:
    struct SPresentationState
    {
    private:
        TTransitionBrowserRecordDataCollection expandedRecordDataCollection;
        TTransitionBrowserRecordDataCollection selectedRecordDataCollection;
        std::shared_ptr<CTransitionBrowserRecord::SRecordData> pFocusedRecordData;

        friend class CTransitionBrowserTreeCtrl;
    };

public:
    CTransitionBrowserTreeCtrl(QWidget* parent);
    void SetBrowser(CTransitionBrowser* pTransitionBrowser) { m_pTransitionBrowser = pTransitionBrowser; }
    void SelectItem(const TTransitionID& transitionID);

    CRecord* FindRecordForTransition(const TTransitionID& transitionID);

    void SavePresentationState(OUT SPresentationState& outState) const;
    void RestorePresentationState(const SPresentationState& state);

    void SetBold(const TTransitionBrowserRecordDataCollection& recordDataCollection, const BOOL bBold = TRUE);

    //void PrintRows(const char* szWhen); // Was useful for debugging

protected:
    void SelectAndFocusRecordDataCollection(const CTransitionBrowserRecord::SRecordData* pFocusedRecordData, const TTransitionBrowserRecordDataCollection* pSelectedRecordDataCollection);
    void ExpandRowsForRecordDataCollection(const TTransitionBrowserRecordDataCollection& expandRecordDataCollection);
    CRecord* SearchRecordForTransition(CRecord* pRecord, const TTransitionID& transitionID);
    void SelectItem(CTransitionBrowserRecord* pFocusedRecord);

    void SetBoldRecursive(QTreeWidgetItem* pRecords, const TTransitionBrowserRecordDataCollection& recordDataCollection, const BOOL bBold);

    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

protected:
    CTransitionBrowser* m_pTransitionBrowser;
};

class CTransitionBrowser
    : public QWidget
{
    Q_OBJECT
public:
    CTransitionBrowser(CTransitionEditorPage& transitionEditorPage, QWidget* pParent = nullptr);
    virtual ~CTransitionBrowser();

    void Refresh();
    void SetContext(SMannequinContexts& context);
    void SelectAndOpenRecord(const TTransitionID& transitionID);

    void FindFragmentReferences(QString fragSearchStr);
    void FindTagReferences(QString tagSearchStr);
    void FindClipReferences(QString icafSearchStr);

private slots:
    void slotVisibilityChanged(bool visible);

private:
    BOOL OnInitDialog();

    void OnSelectionChanged();
    void OnItemDoubleClicked();


    void UpdateButtonStatus();

    void SetDatabase(IAnimationDatabase* animDB);
    void SetScopeContextDataIdx(int scopeContextDataIdx);

    void SetOpenedRecord(CTransitionBrowserRecord* pRecordToOpen, const TTransitionID& pickedTransitionID);

    CTransitionBrowserRecord* GetSelectedRecord();
    bool GetSelectedTransition(TTransitionID& outTransitionID);

    void OpenSelected();
    void EditSelected();
    void CopySelected();
    void DeleteSelected();

    void OpenSelectedTransition(const TTransitionID& transitionID);
    bool ShowTransitionSettingsDialog(TTransitionID& inoutTransitionID, const QString& windowCaption);
    bool DeleteTransition(const TTransitionID& transitionToDelete); // returns true when the deleted transition was currently opened

    inline bool HasOpenedRecord() const
    {
        return !m_openedRecordDataCollection.empty();
    }

protected slots:
    void OnNewBtn();
    void OnEditBtn();
    void OnCopyBtn();
    void OnDeleteBtn();
    void OnOpenBtn();
    void OnChangeContext();
    void OnSwapFragmentFilters();
    void OnSwapTagFilters();
    void OnSwapAnimClipFilters();

    void OnChangeFilterTagsFrom(const QString& filter);
    void OnChangeFilterFragmentsFrom(const QString& filter);
    void OnChangeFilterAnimClipFrom(const QString& filter);
    void OnChangeFilterTagsTo(const QString& filter);
    void OnChangeFilterFragmentsTo(const QString& filter);
    void OnChangeFilterAnimClipTo(const QString& filter);

    void UpdateTags(QStringList& tags, QString tagString);
    void UpdateFilters();

    bool FilterByAnimClip(const FragmentID fragId, const SFragTagState& tagState, const QString& filter);

private:
    IAnimationDatabase* m_animDB;
    CTransitionEditorPage& m_transitionEditorPage;

    QString m_filterTextFrom;
    QStringList m_filtersFrom;
    QString m_filterFragmentIDTextFrom;
    QString m_filterAnimClipFrom;

    QString m_filterTextTo;
    QStringList m_filtersTo;
    QString m_filterFragmentIDTextTo;
    QString m_filterAnimClipTo;

    SMannequinContexts* m_context;
    int m_scopeContextDataIdx;

    TTransitionBrowserRecordDataCollection m_openedRecordDataCollection; // the record currently opened in the transition editor (followed by the parent records)
    TTransitionID m_openedTransitionID; // This is NOT necessarily equal to the transitionID inside m_pOpenedRecord.
    // m_openedTransitionID is the blend actually opened, the blend the user
    // picked in the transition picker.

    QScopedPointer<Ui::CTransitionBrowser> ui;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_CONTROLS_TRANSITIONBROWSER_H
