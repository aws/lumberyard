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
#include "SequencerNode.h"
#include "SequencerTrack.h"
#include "MannequinNodes.h"
#include "MannequinDialog.h"

#include "SequencerSequence.h"
#include "SequencerDopeSheet.h"
#include "SequencerUndo.h"
#include "StringDlg.h"

#include "Clipboard.h"

#include "ISequencerSystem.h"

#include "Objects/EntityObject.h"
#include "ViewManager.h"
#include "RenderViewport.h"

#include "SequencerTrack.h"
#include "SequencerNode.h"

#include "MannequinDialog.h"
#include "FragmentTrack.h"

#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollBar>


#define EDIT_DISABLE_GRAY_COLOR QColor(180, 180, 180)


namespace
{
    static const int MUTE_TRACK_ICON_UNMUTED = 18;
    static const int MUTE_TRACK_ICON_MUTED = 19;
    static const int MUTE_TRACK_ICON_SOLO = 20;
    static const int MUTE_NODE_ICON_UNMUTED = 21;
    static const int MUTE_NODE_ICON_MUTED = 22;
    static const int MUTE_NODE_ICON_SOLO = 23;
}


const int kIconFromParamID[SEQUENCER_PARAM_TOTAL] = {0, 0, 14, 3, 10, 13};
const int kIconFromNodeType[SEQUENCER_NODE_TOTAL] = {1, 1, 1, 1, 1, 2};

class MannNodesTreeModel
    : public QAbstractItemModel
{
public:
    MannNodesTreeModel(CMannNodesWidget* parent)
        : QAbstractItemModel(parent)
        , m_sequence(nullptr)
        , m_widget(parent)
    {
    }

    enum Role
    {
        NodeRole = Qt::UserRole,
        TrackRole
    };

    QStringList mimeTypes() const override
    {
        return{ EditorDragDropHelpers::GetAnimationNameClipboardFormat(), QStringLiteral("application/x-mannequin-track-index")};
    }

    QMimeData* mimeData(const QModelIndexList& indexes) const override
    {
        QMimeData* data = new QMimeData;
        QByteArray d;
        QDataStream stream(&d, QIODevice::WriteOnly);
        for (auto i : indexes)
        {
            if (i.column() == 0)
            {
                stream << i.row() << i.internalId();
            }
        }
        data->setData(QStringLiteral("application/x-mannequin-track-index"), d);
        return data;
    }

    Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override
    {
        if (!QAbstractItemModel::canDropMimeData(data, action, row, column, parent))
            return false;
        if (data->hasFormat(EditorDragDropHelpers::GetAnimationNameClipboardFormat()))
        {
            QModelIndex i = parent.child(row, column);
            return m_widget->IsPointValidForAnimationInContextDrop(i, data);
        }
        else if (data->hasFormat(QStringLiteral("application/x-mannequin-track-index")))
        {
            if (!parent.parent().isValid())
            {
                return row != -1;
            }
            return false;
        }
        return false;
    }

    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override
    {
        if (!canDropMimeData(data, action, row, column, parent))
        {
            return false;
        }
        if (data->hasFormat(EditorDragDropHelpers::GetAnimationNameClipboardFormat()))
        {
            return m_widget->CreatePointForAnimationInContextDrop(m_widget->IsPointValidForAnimationInContextDrop(parent.child(row, column), data), QPoint(), data);
        }
        else if (data->hasFormat(QStringLiteral("application/x-mannequin-track-index")))
        {
            QByteArray d = data->data(QStringLiteral("application/x-mannequin-track-index"));
            QDataStream stream(&d, QIODevice::ReadOnly);
            QModelIndexList indexes;
            while (!stream.atEnd())
            {
                int r;
                quintptr id;
                stream >> r >> id;
                indexes.push_back(createIndex(r, 0, id));
            }
            if (indexes.isEmpty())
            {
                return false;
            }
            QModelIndex target = parent;
            if (row >= 0 && row < rowCount(parent))
            {
                target = parent.child(row, 0);
            }
            m_widget->OnDragAndDrop(indexes, target);
            return true;
        }
        return false;
    }

    void setSequence(CSequencerSequence* seq)
    {
        beginResetModel();
        m_sequence = seq;
        endResetModel();
    }

    CSequencerSequence* sequence()
    {
        return m_sequence;
    }

    QModelIndex findNode(CSequencerNode* node) const
    {
        if (!m_sequence)
        {
            return QModelIndex();
        }

        for (int row = 0; row < m_sequence->GetNodeCount(); ++row)
        {
            if (node == m_sequence->GetNode(row))
            {
                return createIndex(row, 0, -1);
            }
        }
        return QModelIndex();
    }

    QModelIndex createTrack(CSequencerNode* node, ESequencerParamType nParamId)
    {
        const QModelIndex nodeIndex = findNode(node);
        const int count = rowCount(nodeIndex);
        beginInsertRows(nodeIndex, count, count);
        CSequencerTrack* sequenceTrack = node->CreateTrack(nParamId);
        if (sequenceTrack)
        {
            sequenceTrack->OnChange();
        }
        endInsertRows();
        return nodeIndex.child(count, 0);
    }

    void setTrackVisible(CSequencerNode* node, CSequencerTrack* track, bool visible)
    {
        // nothing do do?
        if (visible && (track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_HIDDEN) == 0)
        {
            return;
        }
        else if (!visible && (track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_HIDDEN))
        {
            return;
        }

        const QModelIndex nodeIndex = findNode(node);

        int row = 0;
        CSequencerNode::SParamInfo paramInfo;
        for (int i = 0; i < node->GetTrackCount(); ++i)
        {
            CSequencerTrack* t = node->GetTrackByIndex(i);
            const ESequencerParamType type = t->GetParameterType();

            if (!node->GetParamInfoFromId(type, paramInfo))
            {
                continue;
            }

            if (t == track)
            {
                break;
            }

            if (t->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_HIDDEN)
            {
                continue;
            }

            ++row;
        }

        // change hidden flag for this track.
        if (visible)
        {
            beginInsertRows(nodeIndex, row, row);
            track->SetFlags(track->GetFlags() & ~CSequencerTrack::SEQUENCER_TRACK_HIDDEN);
            endInsertRows();
        }
        else
        {
            beginRemoveRows(nodeIndex, row, row);
            track->SetFlags(track->GetFlags() | CSequencerTrack::SEQUENCER_TRACK_HIDDEN);
            endRemoveRows();
        }
    }

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
    {
        CSequencerNode* node = parent.data(NodeRole).value<CSequencerNode*>();
        CSequencerTrack* track = parent.child(row, 0).data(TrackRole).value<CSequencerTrack*>();
        if (!node || !track)
        {
            return false;
        }

        for (int r = row + count - 1; r >= row; --r)
        {
            const QModelIndex i = index(r, 0, parent);
            CSequencerTrack* track = i.data(TrackRole).value<CSequencerTrack*>();
            if (!track)
            {
                continue;
            }
            beginRemoveRows(parent, r, r);
            node->RemoveTrack(track);
            endRemoveRows();
        }

        return true;
    }

    int rowCount(const QModelIndex& parent) const override
    {
        if (!m_sequence)
        {
            return 0;
        }
        if (parent.isValid())
        {
            if (parent.internalId() == -3) // extra row
            {
                return 0;
            }
            else if (parent.internalId() == -2) // root index, get number of nodes
            {
                return m_sequence->GetNodeCount();
            }
            else if (parent.internalId() == -1) // node index, get number of tracks
            {
                int count = 0;
                CSequencerNode* node = m_sequence->GetNode(parent.row());
                CSequencerNode::SParamInfo paramInfo;
                for (int i = 0; i < node->GetTrackCount(); ++i)
                {
                    CSequencerTrack* track = node->GetTrackByIndex(i);
                    if (track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_HIDDEN)
                    {
                        continue;
                    }

                    const ESequencerParamType type = track->GetParameterType();

                    if (!node->GetParamInfoFromId(type, paramInfo))
                    {
                        continue;
                    }

                    ++count;
                }
                return count;
            }
            else
            {
                return 0;
            }
        }
        return 1;
    }

    int columnCount(const QModelIndex& parent) const override
    {
        Q_UNUSED(parent);
        return 2;
    }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override
    {
        if (row < 0 || row >= rowCount(parent) || column < 0 || column >= columnCount(parent))
        {
            return QModelIndex();
        }
        if (!parent.isValid())
        {
            return createIndex(row, column, row == 0 ? -2 : -3); // root index wanted
        }
        if (parent.internalId() == -1) // parent is a node index, create a track index
        {
            return createIndex(row, column, parent.row());
        }
        else  // create a node index
        {
            return createIndex(row, column, -1);
        }
    }

    QModelIndex parent(const QModelIndex& index) const override
    {
        if (!index.isValid())
        {
            return QModelIndex();
        }
        if (index.internalId() == -2 || index.internalId() == -3) // root index
        {
            return QModelIndex();
        }
        if (index.internalId() == -1) // node index -> gets root index as parent
        {
            return createIndex(0, 0, -2);
        }
        else // track index -> gets node index as parent
        {
            return createIndex(index.internalId(), 0, -1);
        }
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        QFont font;
        font.setBold(true);

        if (!index.parent().isValid())
        {
            if (index.row() == 0)
            {
                assert(m_sequence);
                switch (role)
                {
                case Qt::DisplayRole:
                    return index.column() == 0 ? m_sequence->GetName() : QString();
                case Qt::SizeHintRole:
                    return QSize(index.column() == 0 ? 150 : 20, 24);
                case Qt::FontRole:
                    return font;
                case Qt::TextAlignmentRole:
                    return QVariant::fromValue<int>(Qt::AlignTop | Qt::AlignLeft);
                default:
                    return QVariant();
                }
            }
            else if (index.row() == 1)
            {
                // Additional empty record like space for scrollbar in key control
                if (role == Qt::SizeHintRole)
                {
                    return QSize(index.column() == 0 ? 150 : 20, 18);
                }
                return QVariant();
            }
        }
        else if (index.internalId() == -1) // node index
        {
            CSequencerNode* node = m_sequence->GetNode(index.row());

            if (index.column() == 1)
            {
                if (role == Qt::DecorationRole)
                {
                    return GetIconIndexForMute(index);
                }
            }

            switch (role)
            {
            case Qt::DisplayRole:
                return index.column() == 0 ? QString::fromLatin1(node->GetName()) : QString();
            case Qt::SizeHintRole:
                return QSize(index.column() == 0 ? 150 : 20, 16 + 2);
            case Qt::FontRole:
                return font;
            case Qt::TextAlignmentRole:
                return QVariant::fromValue<int>(Qt::AlignTop | Qt::AlignLeft);
            case Qt::DecorationRole:
                return GetIconIndexForNode(node->GetType());
            case NodeRole:
                return QVariant::fromValue(node);
            default:
                return QVariant();
            }
        }
        else   // track index
        {
            CSequencerNode* node = m_sequence->GetNode(index.internalId());
            CSequencerNode::SParamInfo paramInfo;
            int row = index.row();
            for (int i = 0; i < node->GetTrackCount(); ++i)
            {
                CSequencerTrack* track = node->GetTrackByIndex(i);
                if (track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_HIDDEN)
                {
                    continue;
                }

                const ESequencerParamType type = track->GetParameterType();

                if (!node->GetParamInfoFromId(type, paramInfo))
                {
                    continue;
                }

                if (row-- > 0)
                {
                    continue;
                }

                if (track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_READONLY)
                {
                    node->SetReadOnly(true);
                }

                if (index.column() == 1)
                {
                    if (role == Qt::DecorationRole)
                    {
                        if ((type == SEQUENCER_PARAM_ANIMLAYER) || (type == SEQUENCER_PARAM_PROCLAYER))
                        {
                            return GetIconIndexForMute(index);
                        }
                        return QVariant();
                    }
                }

                switch (role)
                {
                case Qt::DisplayRole:
                    return index.column() == 0 ? QString::fromLatin1(paramInfo.name) : QString();
                case Qt::SizeHintRole:
                    return QSize(index.column() == 0 ? 150 : 20, gSettings.mannequinSettings.trackSize + 2);
                case Qt::ForegroundRole:
                    return QPalette().color(QPalette::Highlight);
                case Qt::TextAlignmentRole:
                    return QVariant::fromValue<int>(Qt::AlignTop | Qt::AlignLeft);
                case Qt::DecorationRole:
                    return GetIconIndexForParam(type);
                case NodeRole:
                    return QVariant::fromValue(node);
                case TrackRole:
                    return QVariant::fromValue(track);
                default:
                    return QVariant();
                }
            }
        }
        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        Qt::ItemFlags f = QAbstractItemModel::flags(index);
        if (!index.parent().isValid() && index.row() == 1)
        {
            f &= ~Qt::ItemIsSelectable;
        }
        CSequencerNode* node = nullptr;
        if (index.internalId() == -1 && (node = m_sequence->GetNode(index.row())))
        {
            if (node->CanAddTrackForParameter(SEQUENCER_PARAM_ANIMLAYER))
            {
                f |= Qt::ItemIsDropEnabled;
            }
        }
        if (index.isValid() && index.internalId() == -1) // only nodes can be moved
        {
            f |= Qt::ItemIsDragEnabled;
        }
        if (!index.parent().isValid())
        {
            f |= Qt::ItemIsDropEnabled;
        }
        return f;
    }

    void toggleMute(const QModelIndex& index)
    {
        if (CSequencerTrack* track = index.data(TrackRole).value<CSequencerTrack*>())
        {
            muteNodesRecursive(index, !track->IsMuted());
        }
        else if (CSequencerNode* node = index.data(NodeRole).value<CSequencerNode*>())
        {
            muteNodesRecursive(index, !node->IsMuted());
        }
    }

    void muteAllBut(const QModelIndex& index)
    {
        muteAllNodes();
        muteNodesRecursive(index, false);
    }

    void muteAllNodes()
    {
        muteNodesRecursive(index(0, 0), true);
    }

    void unmuteAllNodes()
    {
        muteNodesRecursive(index(0, 0), false);
    }

protected:
    void muteNodesRecursive(const QModelIndex& index, bool bMute)
    {
        if (CSequencerTrack* track = index.data(TrackRole).value<CSequencerTrack*>())
        {
            track->Mute(bMute);
            emit dataChanged(index.sibling(index.row(), 0), index.sibling(index.row(), 1));

            CSequencerNode* node = index.data(NodeRole).value<CSequencerNode*>();
            const QModelIndex parent = index.parent();
            CSequencerNode* parentNode = parent.data(NodeRole).value<CSequencerNode*>();
            CSequencerTrack* parentTrack = parent.data(TrackRole).value<CSequencerTrack*>();
            if (parentNode && !parentTrack && (parentNode == node))
            {
                uint32 mutedAnimLayerMask;
                uint32 mutedProcLayerMask;
                generateMuteMasks(parent, mutedAnimLayerMask, mutedProcLayerMask);
                parentNode->UpdateMutedLayerMasks(mutedAnimLayerMask, mutedProcLayerMask);
                emit dataChanged(parent.sibling(parent.row(), 0), parent.sibling(parent.row(), 1));
            }
        }
        else if (CSequencerNode* node = index.data(NodeRole).value<CSequencerNode*>())
        {
            node->Mute(bMute);
            emit dataChanged(index.sibling(index.row(), 0), index.sibling(index.row(), 1));
        }

        int numRecords = rowCount(index);
        for (int i = 0; i < numRecords; ++i)
        {
            muteNodesRecursive(index.child(i, 0), bMute);
        }
    }

    void generateMuteMasks(const QModelIndex& index, uint32& mutedAnimLayerMask, uint32& mutedProcLayerMask)
    {
        mutedAnimLayerMask = 0;
        mutedProcLayerMask = 0;

        uint32 animCount = 0;
        uint32 procCount = 0;
        bool bHasFragmentId = false;

        int numRecords = rowCount(index);
        for (uint32 i = 0; i < numRecords; ++i)
        {
            const QModelIndex child = index.child(i, 0);
            if (CSequencerTrack* track = child.data(TrackRole).value<CSequencerTrack*>())
            {
                switch (track->GetParameterType())
                {
                case SEQUENCER_PARAM_FRAGMENTID:
                {
                    bHasFragmentId = true;
                }
                break;
                case SEQUENCER_PARAM_ANIMLAYER:
                {
                    if (track->IsMuted())
                    {
                        mutedAnimLayerMask |= BIT(animCount);
                    }
                    ++animCount;
                }
                break;
                case SEQUENCER_PARAM_PROCLAYER:
                {
                    if (track->IsMuted())
                    {
                        mutedProcLayerMask |= BIT(procCount);
                    }
                    ++procCount;
                }
                break;
                default:
                    break;
                }
            }
        }

        if (bHasFragmentId && !animCount && !procCount)
        {
            // special case: if there is a fragmentId, but no anim or proc layers
            // assume everything is controlled by parent state.
            if (CSequencerNode* node = index.data(NodeRole).value<CSequencerNode*>())
            {
                if (node->IsMuted())
                {
                    mutedAnimLayerMask = 0xFFFFFFFF;
                    mutedProcLayerMask = 0xFFFFFFFF;
                }
            }
        }
    }

    QPixmap GetIconIndexForParam(ESequencerParamType type) const
    {
        return QPixmap(QStringLiteral(":/FragmentBrowser/Controls/sequencer_nodes_%1.png").arg(kIconFromParamID[type], 2, 10, QLatin1Char('0')));
    }

    QPixmap GetIconIndexForNode(ESequencerNodeType type) const
    {
        return QPixmap(QStringLiteral(":/FragmentBrowser/Controls/sequencer_nodes_%1.png").arg(kIconFromNodeType[type], 2, 10, QLatin1Char('0')));
    }

    QPixmap GetIconIndexForMute(const QModelIndex& index) const
    {
        int icon = MUTE_TRACK_ICON_UNMUTED;

        if (CSequencerTrack* track = index.data(TrackRole).value<CSequencerTrack*>())
        {
            icon = track->IsMuted() ? MUTE_TRACK_ICON_MUTED : MUTE_TRACK_ICON_UNMUTED;
        }
        else if (CSequencerNode* node = index.data(NodeRole).value<CSequencerNode*>())
        {
            icon = node->IsMuted() ? MUTE_NODE_ICON_MUTED : MUTE_NODE_ICON_UNMUTED;
        }

        return QPixmap(QStringLiteral(":/FragmentBrowser/Controls/sequencer_nodes_%1.png").arg(icon, 2, 10, QLatin1Char('0')));
    }

private:
    CSequencerSequence* m_sequence;
    CMannNodesWidget* m_widget;
};

//////////////////////////////////////////////////////////////////////////
CMannNodesWidget::CMannNodesWidget(QWidget* parent)
    : QTreeView(parent)
    , m_model(new MannNodesTreeModel(this))
{
    m_keysCtrl = 0;

    setAcceptDrops(true);
    setDragEnabled(true);
    setModel(m_model);

    connect(this, &QTreeView::clicked, this, &CMannNodesWidget::OnNMLclick);
    connect(this, &QWidget::customContextMenuRequested, this, &CMannNodesWidget::OnNMRclick);
    connect(this, &QTreeView::collapsed, this, &CMannNodesWidget::OnItemExpanded);
    connect(this, &QTreeView::expanded, this, &CMannNodesWidget::OnItemExpanded);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMannNodesWidget::OnSelectionChanged);

    setHeaderHidden(true);
    header()->setSectionResizeMode(QHeaderView::Fixed);
    header()->resizeSection(0, 150);
    header()->resizeSection(1, 20);

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setContextMenuPolicy(Qt::CustomContextMenu);

    if (verticalScrollBar())
    {
        connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &CMannNodesWidget::OnVerticalScroll);
    }

    m_bEditLock = false;
};


//////////////////////////////////////////////////////////////////////////
CMannNodesWidget::~CMannNodesWidget()
{

};


QModelIndexList allIndexes(const QAbstractItemModel* model, const QModelIndex& parent = QModelIndex())
{
    QModelIndexList result;
    for (int i = 0; i < model->rowCount(parent); ++i)
    {
        const QModelIndex index = model->index(i, 0, parent);
        result.push_back(index);
        result += allIndexes(model, index);
    }
    return result;
};

//////////////////////////////////////////////////////////////////////////
void CMannNodesWidget::SetSequence(CSequencerSequence* seq)
{
    m_model->setSequence(seq);
    foreach (const QModelIndex &index, allIndexes(m_model))
    {
        CSequencerNode* node = index.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>();
        if (node == nullptr || node->GetStartExpanded())
        {
            expand(index);
        }
    }

    SyncKeyCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CMannNodesWidget::SetKeyListCtrl(CSequencerDopeSheetBase* keysCtrl)
{
    m_keysCtrl = keysCtrl;
    //SyncKeyCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CMannNodesWidget::OnItemExpanded()
{
    SyncKeyCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CMannNodesWidget::OnSelectionChanged()
{
    m_keysCtrl->ClearSelection();

    if (!m_model->sequence())
    {
        return;
    }

    // Clear track selections.
    for (int i = 0; i < m_model->sequence()->GetNodeCount(); i++)
    {
        CSequencerNode* node = m_model->sequence()->GetNode(i);
        for (int t = 0; t < node->GetTrackCount(); t++)
        {
            node->GetTrackByIndex(t)->SetSelected(false);
        }
    }


    const QModelIndexList selection = selectionModel()->selectedRows();
    for (const QModelIndex& index : selection)
    {
        CSequencerTrack* track = index.data(MannNodesTreeModel::TrackRole).value<CSequencerTrack*>();
        if (!track)
        {
            continue;
        }

        for (int i = 0; i < m_keysCtrl->GetCount(); i++)
        {
            if (track)
            {
                track->SetSelected(true);
            }
            if (m_keysCtrl->GetTrack(i) == track)
            {
                m_keysCtrl->SelectItem(i);
                break;
            }
        }
    }

    GetIEditor()->Notify(eNotify_OnUpdateSequencerKeySelection);
}


QModelIndexList CMannNodesWidget::expandedIndexes(const QModelIndex& parent)
{
    QModelIndexList result;
    QModelIndex curIndex = parent;
    do
    {
        result.push_back(curIndex);
        curIndex = indexBelow(curIndex);
    } while (curIndex.isValid() && visualRect(curIndex).isValid());
    return result;
};

//////////////////////////////////////////////////////////////////////////
void CMannNodesWidget::SyncKeyCtrl()
{
    if (!m_keysCtrl)
    {
        return;
    }

    m_keysCtrl->ResetContent();

    if (!m_model->sequence())
    {
        return;
    }

    int nStartRow = 0;
    QModelIndex topRow = indexAt(rect().topLeft());

    foreach (const QModelIndex&index, expandedIndexes(topRow))
    {
        const int nItemHeight = index.data(Qt::SizeHintRole).toSize().height();

        CSequencerDopeSheet::Item item;
        CSequencerNode* node = index.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>();
        CSequencerTrack* track = index.data(MannNodesTreeModel::TrackRole).value<CSequencerTrack*>();

        if (track != nullptr && node != nullptr)
        {
            item = CSequencerDopeSheet::Item(nItemHeight, node, track->GetParameterType(), track);
        }
        else
        {
            item = CSequencerDopeSheet::Item(nItemHeight, node);
        }
        item.nHeight = nItemHeight;
        item.bSelected = false;
        if (track)
        {
            item.bSelected = (track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_SELECTED) != 0;
        }
        m_keysCtrl->AddItem(item);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannNodesWidget::ExpandNode(CSequencerNode* node)
{
    expand(m_model->findNode(node));
    SyncKeyCtrl();
}

//////////////////////////////////////////////////////////////////////////
bool CMannNodesWidget::IsNodeExpanded(CSequencerNode* node)
{
    return isExpanded(m_model->findNode(node));
}

//////////////////////////////////////////////////////////////////////////
void CMannNodesWidget::SelectNode(const char* sName)
{
    selectionModel()->clear();
    const QModelIndex root = m_model->index(0, 0);
    const int count = m_model->rowCount(root);
    for (int i = 0; i < count; ++i)
    {
        const QModelIndex index = root.child(i, 0);
        CSequencerNode* node = index.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>();
        if (node != nullptr && _stricmp(node->GetName(), sName) == 0)
        {
            setCurrentIndex(index);
            break;
        }
    }

    SyncKeyCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CMannNodesWidget::OnVerticalScroll()
{
    SyncKeyCtrl();
}

//////////////////////////////////////////////////////////////////////////
void CMannNodesWidget::OnNMLclick(const QModelIndex& index)
{
    if (index.column() == eCOLUMN_MUTE)
    {
        m_model->toggleMute(index);
        RefreshTracks();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannNodesWidget::OnNMRclick(const QPoint& point)
{
    if (!m_model->sequence())
    {
        return;
    }

    // Select the item that is at the point myPoint.

    const QModelIndex index = indexAt(point);
    CSequencerNode* node = index.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>();

    if (!node)
    {
        return;
    }

    int cmd = ShowPopupMenu(point, index);

    QModelIndex scrollPos = SaveVerticalScrollPos();

    if (cmd >= eMenuItem_AddLayer_First && cmd < eMenuItem_AddLayer_Last)
    {
        if (node)
        {
            int paramIndex = cmd - eMenuItem_AddLayer_First;
            AddTrack(paramIndex, node);
        }
    }
    else if (cmd == eMenuItem_CreateForCurrentTags)
    {
        node->OnMenuOption(eMenuItem_CreateForCurrentTags);
        RefreshTracks();
    }

    else if (cmd == eMenuItem_CopyLayer)
    {
        m_keysCtrl->CopyTrack();
    }
    else if (cmd == eMenuItem_PasteLayer)
    {
        if (node)
        {
            PasteTrack(node);
        }
    }
    else if (cmd == eMenuItem_RemoveLayer)
    {
        if (node)
        {
            RemoveTrack(index);
        }
    }
    else if (cmd >= eMenuItem_ShowHide_First && cmd < eMenuItem_ShowHide_Last)
    {
        if (node)
        {
            ShowHideTrack(node, cmd - eMenuItem_ShowHide_First);
        }
    }
    else if (cmd == eMenuItem_CopySelectedKeys)
    {
        m_keysCtrl->CopyKeys();
    }
    else if (cmd == eMenuItem_CopyKeys)
    {
        m_keysCtrl->CopyKeys(true, true, true);
    }
    else if (cmd == eMenuItem_PasteKeys)
    {
        CSequencerNode* node = index.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>();
        CSequencerTrack* track = index.data(MannNodesTreeModel::TrackRole).value<CSequencerTrack*>();
        m_keysCtrl->PasteKeys(node, track, 0);
    }
    else if (cmd == eMenuItem_MuteNode)
    {
        m_model->toggleMute(index);
        RefreshTracks();
    }
    else if (cmd == eMenuItem_SoloNode)
    {
        m_model->muteAllBut(index);
        RefreshTracks();
    }
    else if (cmd == eMenuItem_MuteAll)
    {
        m_model->muteAllNodes();
        RefreshTracks();
    }
    else if (cmd == eMenuItem_UnmuteAll)
    {
        m_model->unmuteAllNodes();
        RefreshTracks();
    }
    else
    {
        if (node)
        {
            node->OnMenuOption(cmd);
        }
    }

    if (cmd)
    {
        RestoreVerticalScrollPos(scrollPos);
        SyncKeyCtrl();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannNodesWidget::OnDragAndDrop(const QModelIndexList& pRows, const QModelIndex& target)
{
    if (pRows.isEmpty())
    {
        return;
    }


    bool bContainTarget = pRows.contains(target);

    if (target.isValid())
    {
        QModelIndex pFirstRecordSrc = pRows.first();
        QModelIndex pRecordTrg = target;
        if (pFirstRecordSrc.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>() && pRecordTrg.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>() && !bContainTarget)
        {
            const char* srcFirstNodeName = pFirstRecordSrc.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>()->GetName();

            CAnimSequenceUndo undo(m_model->sequence(), "Reorder Node");
            CSequencerNode* pTargetNode = pRecordTrg.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>();

            for (size_t i = 0; i < pRows.count(); ++i)
            {
                QModelIndex pRecord = pRows.at(pRows.count() - i - 1);
                bool next = false;
                m_model->sequence()->ReorderNode(pRecord.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>(), pTargetNode, next);
            }

            InvalidateNodes();
            SelectNode(srcFirstNodeName);
        }
        else
        {
            if (pRecordTrg == m_model->index(0, 0))
            {
                CAnimSequenceUndo undo(m_model->sequence(), "Detach Anim Node from Group");
                for (size_t i = 0; i < pRows.count(); ++i)
                {
                    QModelIndex pRecord = pRows.at(pRows.count() - i - 1);
                    m_model->sequence()->ReorderNode(pRecord.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>(), NULL, false);
                }
                InvalidateNodes();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMannNodesWidget::InvalidateNodes()
{
    const QSignalBlocker sb(m_model); // do _not_ reload the model, ignore it being reset
    /*
    Notify belows informs other listeners about changes in the sequence.
    We already know about these changes. The reset is a noop in that case and will only make trouble with selection state and such.
    */
    GetIEditor()->Notify(eNotify_OnUpdateSequencer);
}

//////////////////////////////////////////////////////////////////////////
bool CMannNodesWidget::GetSelectedNodes(AnimNodes& nodes)
{
    const QModelIndexList selection = selectionModel()->selectedRows();
    for (const QModelIndex& index : selection)
    {
        CSequencerNode* node = index.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>();
        if (node)
        {
            stl::push_back_unique(nodes, node);
        }
    }

    return !nodes.empty();
}

//////////////////////////////////////////////////////////////////////////
bool CMannNodesWidget::HasNode(const char* name) const
{
    if (name == NULL)
    {
        return false;
    }

    CSequencerNode* pNode = m_model->sequence()->FindNodeByName(name);
    if (pNode)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void CMannNodesWidget::AddTrack(int paramIndex, CSequencerNode* node)
{
    CSequencerNode::SParamInfo paramInfo;
    if (!node->GetParamInfo(paramIndex, paramInfo))
    {
        return;
    }

    CAnimSequenceUndo undo(m_model->sequence(), "Add Layer");

    m_model->createTrack(node, paramInfo.paramId);
    InvalidateNodes();
    ExpandNode(node);
}

bool CMannNodesWidget::CanPasteTrack(CSequencerNode* node)
{
    CClipboard clip(this);
    if (clip.IsEmpty())
    {
        return false;
    }

    XmlNodeRef copyNode = clip.Get();
    if (copyNode == NULL || strcmp(copyNode->getTag(), "TrackCopy"))
    {
        return false;
    }

    if (copyNode->getChildCount() < 1)
    {
        return false;
    }

    XmlNodeRef trackNode = copyNode->getChild(0);

    int intParamId = 0;
    trackNode->getAttr("paramId", intParamId);
    if (!intParamId)
    {
        return false;
    }

    if (!node->CanAddTrackForParameter(static_cast<ESequencerParamType>(intParamId)))
    {
        return false;
    }

    return true;
}

void CMannNodesWidget::PasteTrack(CSequencerNode* node)
{
    if (!CanPasteTrack(node))
    {
        return;
    }

    CClipboard clip(this);
    XmlNodeRef copyNode = clip.Get();
    XmlNodeRef trackNode = copyNode->getChild(0);
    int intParamId = 0;
    trackNode->getAttr("paramId", intParamId);

    CAnimSequenceUndo undo(m_model->sequence(), "Paste Layer");

    const QModelIndex trackIndex = m_model->createTrack(node, static_cast<ESequencerParamType>(intParamId));
    CSequencerTrack* sequenceTrack = trackIndex.data(MannNodesTreeModel::TrackRole).value<CSequencerTrack*>();
    if (sequenceTrack)
    {
        sequenceTrack->SerializeSelection(trackNode, true, false, 0.0f);
        sequenceTrack->OnChange();
    }

    InvalidateNodes();
    ExpandNode(node);
}

void CMannNodesWidget::RemoveTrack(const QModelIndex& index)
{
    if (QMessageBox::question(this, QString(), tr("Are you sure you want to delete this track ? Undo will not be available !")) == QMessageBox::Yes)
    {
        CAnimSequenceUndo undo(m_model->sequence(), "Remove Track");

        m_model->removeRow(index.row(), index.parent());
        InvalidateNodes();
    }
}

void CMannNodesWidget::ShowHideTrack(CSequencerNode* node, int trackIndex)
{
    CAnimSequenceUndo undo(m_model->sequence(), "Modify Track");

    CSequencerTrack* track = node->GetTrackByIndex(trackIndex);

    m_model->setTrackVisible(node, track, track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_HIDDEN);

    InvalidateNodes();
}

void CMannNodesWidget::RefreshTracks()
{
    if (m_keysCtrl)
    {
        float fTime = m_keysCtrl->GetCurrTime();
        if (CMannequinDialog::GetCurrentInstance()->IsPaneSelected<CFragmentEditorPage*>())
        {
            CMannequinDialog::GetCurrentInstance()->FragmentEditor()->SetTime(fTime);
        }
        else if (CMannequinDialog::GetCurrentInstance()->IsPaneSelected<CPreviewerPage*>())
        {
            CMannequinDialog::GetCurrentInstance()->Previewer()->SetTime(fTime);
        }
        else if (CMannequinDialog::GetCurrentInstance()->IsPaneSelected<CTransitionEditorPage*>())
        {
            CMannequinDialog::GetCurrentInstance()->TransitionEditor()->SetTime(fTime);
        }
    }

    InvalidateNodes();
}

int CMannNodesWidget::ShowPopupMenu(const QPoint& point, const QModelIndex& index)
{
    // Create pop up menu.

    switch (index.column())
    {
    default:
    case eCOLUMN_NODE:
    {
        return ShowPopupMenuNode(point, index);
    }
    break;
    case eCOLUMN_MUTE:
    {
        return ShowPopupMenuMute(point, index);
    }
    break;
    }

    return 0;
}

int CMannNodesWidget::ShowPopupMenuNode(const QPoint& point, const QModelIndex& index)
{
    QMenu menu;
    QMenu menuAddTrack;

    bool onNode = false;

    CSequencerNode* node = index.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>();
    CSequencerTrack* track = index.data(MannNodesTreeModel::TrackRole).value<CSequencerTrack*>();

    if (selectionModel()->selectedRows().count() == 1)
    {
        bool notOnValidItem = !node;
        bool onValidItem = !notOnValidItem;
        onNode = onValidItem && track == NULL;
        bool onTrack = onValidItem && track != NULL;
        bool bReadOnly = false;
        if ((onValidItem && node->IsReadOnly()) ||
            (onTrack && (track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_READONLY) != 0))
        {
            bReadOnly = true;
        }
        if (onValidItem)
        {
            if (onNode)
            {
                menu.addSeparator();
                menu.addAction(tr("Copy Selected Keys"))->setData(eMenuItem_CopySelectedKeys);
            }
            else    // On a track
            {
                menu.addAction(tr("Copy Keys"))->setData(eMenuItem_CopyKeys);
            }

            if (!bReadOnly)
            {
                menu.addAction(tr("Paste Keys"))->setData(eMenuItem_PasteKeys);

                if (onNode)
                {
                    menu.addSeparator();

                    node->InsertMenuOptions(&menu);
                }
            }
        }

        // add layers submenu
        bool bTracksToAdd = false;

        if (onValidItem && !bReadOnly)
        {
            menu.addSeparator();
            // List`s which tracks can be added to animation node.
            const int validParamCount = node->GetParamCount();
            for (int i = 0; i < validParamCount; ++i)
            {
                CSequencerNode::SParamInfo paramInfo;
                if (!node->GetParamInfo(i, paramInfo))
                {
                    continue;
                }

                if (!node->CanAddTrackForParameter(paramInfo.paramId))
                {
                    continue;
                }

                menuAddTrack.addAction(paramInfo.name)->setData(eMenuItem_AddLayer_First + i);
                bTracksToAdd = true;
            }
        }

        if (!bReadOnly)
        {
            if (bTracksToAdd)
            {
                menuAddTrack.setTitle(tr("Add Layer"));
                menu.addMenu(&menuAddTrack);
            }
        }
        else
        {
            menu.addAction(tr("Create Instance for Current Tags"))->setData(eMenuItem_CreateForCurrentTags);
        }

        if (onTrack)
        {
            menu.addAction(tr("Copy Layer"))->setData(eMenuItem_CopyLayer);
        }

        if (!bReadOnly)
        {
            if (bTracksToAdd)
            {
                bool canPaste = node && CanPasteTrack(node);
                QAction* action = menu.addAction(tr("Paste Layer"));
                action->setData(eMenuItem_PasteLayer);
                action->setEnabled(canPaste);
            }
            if (onTrack)
            {
                // The FragmentID and TransitionProperties Layers should not be removable
                if (track->GetParameterType() != SEQUENCER_PARAM_FRAGMENTID &&
                    track->GetParameterType() != SEQUENCER_PARAM_TRANSITIONPROPS)
                {
                    menu.addAction(tr("Remove Layer"))->setData(eMenuItem_RemoveLayer);
                }
            }
        }

        if (bTracksToAdd || onTrack)
        {
            menu.addSeparator();
        }

        if (onValidItem)
        {
            menu.addAction(tr("%1 Tracks").arg(node->GetName()))->setEnabled(false);

            // Show tracks in anim node.
            {
                CSequencerNode::SParamInfo paramInfo;
                for (int i = 0; i < node->GetTrackCount(); i++)
                {
                    CSequencerTrack* track = node->GetTrackByIndex(i);

                    if (!node->GetParamInfoFromId(track->GetParameterType(), paramInfo))
                    {
                        continue;
                    }

                    // change hidden flag for this track.
                    bool checked = true;
                    if (track->GetFlags() & CSequencerTrack::SEQUENCER_TRACK_HIDDEN)
                    {
                        checked = false;
                    }
                    QAction* action = menu.addAction(QString::fromLatin1("  %1").arg(paramInfo.name));
                    action->setCheckable(true);
                    action->setChecked(checked);
                    action->setData(eMenuItem_ShowHide_First + i);
                }
            }
        }
    }

    // track menu

    if (m_bEditLock)
    {
        SetPopupMenuLock(&menu);
    }

    QAction* action = menu.exec(mapToGlobal(point));
    int ret = action ? action->data().toInt() : 0;

    if (onNode)
    {
        node->ClearMenuOptions(&menu);
    }

    return ret;
}

int CMannNodesWidget::ShowPopupMenuMute(const QPoint& point, const QModelIndex& index)
{
    QMenu menu;

    bool onNode = false;

    if (selectionModel()->selectedRows().count() == 1)
    {
        CSequencerNode* node = index.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>();
        CSequencerTrack* track = index.data(MannNodesTreeModel::TrackRole).value<CSequencerTrack*>();
        bool notOnValidItem = !node;
        bool onValidItem = !notOnValidItem;
        if (onValidItem)
        {
            if (track)
            {
                // solo not active, so mute options are available
                menu.addAction(tr("Mute All But This"))->setData(eMenuItem_SoloNode);
                menu.addSeparator();
                menu.addAction(track->IsMuted() ? tr("Unmute") : tr("Mute"))->setData(eMenuItem_MuteNode);
                menu.addSeparator();
                menu.addAction(tr("Mute All"))->setData(eMenuItem_MuteAll);
                menu.addAction(tr("Unmute All"))->setData(eMenuItem_UnmuteAll);
            }
            else if (node)
            {
                // solo not active, so mute options are available
                menu.addAction(tr("Mute All But This"))->setData(eMenuItem_SoloNode);
                menu.addSeparator();
                menu.addAction(node->IsMuted() ? tr("Unmute") : tr("Mute"))->setData(eMenuItem_MuteNode);
                menu.addSeparator();
                menu.addAction(tr("Mute All"))->setData(eMenuItem_MuteAll);
                menu.addAction(tr("Unmute All"))->setData(eMenuItem_UnmuteAll);
            }

            if (m_bEditLock)
            {
                SetPopupMenuLock(&menu);
            }

            QAction* action = menu.exec(mapToGlobal(point));
            int ret = action ? action->data().toInt() : 0;

            return ret;
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------
void CMannNodesWidget::SetPopupMenuLock(QMenu* menu)
{
    if (!m_bEditLock || !menu)
    {
        return;
    }

    for (QAction* action : menu->actions())
    {
        const QString menuString = action->text();

        if (menuString != tr("Expand") && menuString != tr("Collapse"))
        {
            action->setEnabled(false);
        }
    }
}

void CMannNodesWidget::resizeEvent(QResizeEvent* event)
{
    QTreeView::resizeEvent(event);

    header()->resizeSection(0, width() - 2 * frameWidth() - 32);

    SyncKeyCtrl();
}

QModelIndex CMannNodesWidget::SaveVerticalScrollPos() const
{
    return indexAt(QPoint(0, 0));
}

void CMannNodesWidget::RestoreVerticalScrollPos(const QModelIndex& index)
{
    scrollTo(index, QAbstractItemView::PositionAtTop);
}

//////////////////////////////////////////////////////////////////////////
CSequencerNode* CMannNodesWidget::IsPointValidForAnimationInContextDrop(const QModelIndex& index, const QMimeData* pDataObject) const
{
    if (!CMannequinDialog::GetCurrentInstance()->IsPaneSelected<CFragmentEditorPage*>())
    {
        return nullptr;
    }

    QString clipFormat = EditorDragDropHelpers::GetAnimationNameClipboardFormat();
    auto hData = pDataObject->data(clipFormat);

    if (hData.isNull())
    {
        return nullptr;
    }

    string sAnimName = hData.data();

    if (sAnimName.empty())
    {
        return nullptr;
    }

    if (!m_model->sequence())
    {
        return nullptr;
    }

    if (m_bEditLock)
    {
        return nullptr;
    }

    auto node = index.data(MannNodesTreeModel::NodeRole).value<CSequencerNode*>();

    if (!node)
    {
        return nullptr;
    }

    if (!node->CanAddTrackForParameter(SEQUENCER_PARAM_ANIMLAYER))
    {
        return nullptr;
    }
    return node;
}

//////////////////////////////////////////////////////////////////////////
// Assume IsPointValidForAnimationInContextDrop returned true
bool CMannNodesWidget::CreatePointForAnimationInContextDrop(CSequencerNode* node, const QPoint& point, const QMimeData* pDataObject)
{
    // List`s which tracks can be added to animation node.
    const unsigned int validParamCount = (unsigned int) node->GetParamCount();

    QString clipFormat = EditorDragDropHelpers::GetAnimationNameClipboardFormat();
    auto hData = pDataObject->data(clipFormat);

    if (hData.isNull())
    {
        return false;
    }

    string sAnimName = hData.data();

    unsigned int nAnimLyrIdx = 0;
    for (; nAnimLyrIdx < validParamCount; ++nAnimLyrIdx)
    {
        CSequencerNode::SParamInfo paramInfo;
        if (!node->GetParamInfo(nAnimLyrIdx, paramInfo))
        {
            continue;
        }

        if (SEQUENCER_PARAM_ANIMLAYER == paramInfo.paramId)
        {
            break;
        }
    }

    if (nAnimLyrIdx == validParamCount)
    {
        return false;
    }

    _smart_ptr<CSequencerNode> pNode = node;
    AddTrack(nAnimLyrIdx, node);
    CSequencerTrack* pTrack = pNode->GetTrackByIndex(pNode->GetTrackCount() - 1);

    int keyID = pTrack->CreateKey(0.0f);

    CClipKey newKey;
    newKey.time = 0.0f;
    newKey.animRef.SetByString (sAnimName);
    pTrack->SetKey(keyID, &newKey);
    pTrack->SelectKey(keyID, true);
    return true;
}
