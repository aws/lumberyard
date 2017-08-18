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

#ifndef CRYINCLUDE_EDITOR_CHARACTEREDITOR_ANIMATIONBROWSER_H
#define CRYINCLUDE_EDITOR_CHARACTEREDITOR_ANIMATIONBROWSER_H
#pragma once

#include "ICryAnimation.h"

#define COLOR_GREYED_OUT 0x999999
#define COLOR_BLACK 0x000000
#define COLOR_RED 0x4444FF

#include <QMainWindow>

class AnimationBrowserModel;
class AnimationBrowserTreeModel;

class QComboBox;
class QSortFilterProxyModel;
class QTreeView;

class CAnimationBrowser
    : public QMainWindow
    , public IEditorNotifyListener
{
    Q_OBJECT
public:
    typedef QStringList     TDSelectedAnimations;
    // ids of attachments in the attachment browser and the combobox
    struct SAttachmentIDs
    {
        int32 iIndexAttachBrowser;
        int32 iIndexComboBox;
    };


public:
    CAnimationBrowser(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    virtual ~CAnimationBrowser();

    void SetModel_SKEL(ICharacterInstance* pSkelInstance);
    ICharacterInstance* GetSelected_SKEL() const { return m_pInstance_SKEL; }
    void SetModel_SKIN(IAttachmentSkin* pAttachmentSkin);
    IAttachmentSkin* GetSelected_SKIN() const { return m_pInstance_SKIN; }

    void ReloadAnimations();

    bool GetSelectedAnimations(TDSelectedAnimations& anims);

    // Update comboBox
    void UpdateCharacterComboBox(const std::vector<const char*>& attachmentNames, const std::vector<int>& posInAttachBrowser);
    void UpdateCharacterComboBoxSelection(const int ind);

    std::vector<SAttachmentIDs>& GetAttachmentIDs(){return m_attachmentIDs; }

    QToolBar* GetToolBar(){return m_wndToolBar; }

    typedef Functor0 OnDblClickCallback;
    void AddOnDblClickCallback(OnDblClickCallback callback)
    { stl::push_back_unique(m_onDblClickCallbacks, callback); }
    void RemoveOnDblClickCallback(OnDblClickCallback callback)
    { stl::find_and_erase(m_onDblClickCallbacks, callback); }

    void CloseThis()
    { close(); }

protected:
    virtual void OnInitDialog();
    void PlaySelectedAnimations(QStringList& anims);
    void ExportCAF2HTR(const string name);
    void ExportVGrid(const string name);
    void RegenerateAimGrid(const string name);

    void OnReportSelChange();
	// Messages
	//////////////////////////////////////////////////////////////////////////
    void OnReportItemDblClick();
    void OnFilterText();
    void OnSelectCharacters();
    void OnSelectMotionFromHistory();
    void OnReportItemRClick(const QPoint& pos);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IEditorNotifyListener
    //////////////////////////////////////////////////////////////////////////
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
    //////////////////////////////////////////////////////////////////////////

    QTreeView* m_wndReport;
    AnimationBrowserModel* m_animationModel;
    QSortFilterProxyModel* m_animationFilterModel;
    AnimationBrowserTreeModel* m_animationTreeModel;
    QToolBar* m_wndToolBar;

    QLineEdit* m_editFilterText;
    QComboBox* m_comboSelectCharacter;
    QComboBox* m_comboHistory;

    _smart_ptr< ICharacterInstance > m_pInstance_SKEL;
    ICharacterInstance* m_pCharacterAnimLastUpdated;

    _smart_ptr< IAttachmentSkin > m_pInstance_SKIN;
    IAttachmentSkin* m_pAttachmentSkinMorphLastUpdated;

    QString m_filterText;
    QString m_filterTextPrev;

    typedef std::vector<OnDblClickCallback> OnDblClickCallbackList;
    OnDblClickCallbackList m_onDblClickCallbacks;

	// this implements the AnimationStreamingListener functionality
private:
    std::vector<SAttachmentIDs> m_attachmentIDs;
    bool bSelMotionFromHistory;

    int m_builtRecordsCount;

    void UpdateAnimationHistory(const TDSelectedAnimations& anims);
};


#endif // CRYINCLUDE_EDITOR_CHARACTEREDITOR_ANIMATIONBROWSER_H
