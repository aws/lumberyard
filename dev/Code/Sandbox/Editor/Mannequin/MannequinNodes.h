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

// Description : Sequencer's tree control.


#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_MANNEQUINNODES_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_MANNEQUINNODES_H
#pragma once

#include "ISequencerSystem.h"

#include <QTreeView>
// forward declarations.
class CSequencerNode;
class CSequencerTrack;
class CSequencerSequence;
class CSequencerDopeSheetBase;

class MannNodesTreeModel;

//////////////////////////////////////////////////////////////////////////
class CMannNodesWidget
    : public QTreeView
{
public:
    typedef std::vector<CSequencerNode*> AnimNodes;

    enum EColumn
    {
        eCOLUMN_NODE = 0,
        eCOLUMN_MUTE
    };

    void SetSequence(CSequencerSequence* seq);
    void SetKeyListCtrl(CSequencerDopeSheetBase* keysCtrl);
    QModelIndexList expandedIndexes(const QModelIndex& parent = QModelIndex());
    void SyncKeyCtrl();
    void ExpandNode(CSequencerNode* node);
    bool IsNodeExpanded(CSequencerNode* node);
    void SelectNode(const char* sName);

    bool GetSelectedNodes(AnimNodes& nodes);

    void SetEditLock(bool bLock) { m_bEditLock = bLock; }

    QModelIndex SaveVerticalScrollPos() const;
    void RestoreVerticalScrollPos(const QModelIndex& index);

public:
    CMannNodesWidget(QWidget* parent = nullptr);
    virtual ~CMannNodesWidget();

    //////////////////////////////////////////////////////////////////////////
    // Callbacks.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnItemExpanded();
    virtual void OnSelectionChanged();
    virtual void OnVerticalScroll();
    virtual void OnDragAndDrop(const QModelIndexList& pRow, const QModelIndex& target);
    //////////////////////////////////////////////////////////////////////////

protected:
    void OnNMLclick(const QModelIndex& index);
    void OnNMRclick(const QPoint& point);
    void resizeEvent(QResizeEvent* event) override;

    void ShowHideTrack(CSequencerNode* node, int trackIndex);
    void RemoveTrack(const QModelIndex& index);
    void AddTrack(int paramIndex, CSequencerNode* node);
    bool CanPasteTrack(CSequencerNode* node);
    void PasteTrack(CSequencerNode* node);

    void RefreshTracks();

    int ShowPopupMenu(const QPoint& point, const QModelIndex& index);
    int ShowPopupMenuNode(const QPoint& point, const QModelIndex& index);
    int ShowPopupMenuMute(const QPoint& point, const QModelIndex& index);

    void SetPopupMenuLock(QMenu* menu);

    void RecordSequenceUndo();
    void InvalidateNodes();

    int GetIconIndexForParam(ESequencerParamType nParamId) const;
    int GetIconIndexForNode(ESequencerNodeType type) const;

    bool HasNode(const char* name) const;

    //! Set layer node animator for animation control with layer data in the editor.
    void SetLayerNodeAnimators();

protected:

    friend class MannNodesTreeModel;
    CSequencerNode* IsPointValidForAnimationInContextDrop(const QModelIndex& index, const QMimeData* pDataObject) const;
    bool CreatePointForAnimationInContextDrop(CSequencerNode* pItemInfo, const QPoint& point, const QMimeData* pDataObject);

    MannNodesTreeModel * m_model;
    CSequencerDopeSheetBase* m_keysCtrl;

    // Must not be vector, vector may invalidate pointers on add/remove.
    //typedef std::vector<SItemInfo*> ItemInfos;
    //ItemInfos m_itemInfos;

    bool m_bEditLock;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_MANNEQUINNODES_H
