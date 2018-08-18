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

// Description : TrackView's tree control.


#include "StdAfx.h"

#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Maestro/Bus/EditorSequenceComponentBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>

#include "TrackViewNodes.h"
#include "TrackViewDopeSheetBase.h"
#include "TrackViewUndo.h"
#include "StringDlg.h"
#include "TVEventsDialog.h"
#include "TrackViewDialog.h"

#include "Clipboard.h"

#include "IMovieSystem.h"

#include "Objects/EntityObject.h"
#include "ViewManager.h"
#include "RenderViewport.h"
#include "Export/ExportManager.h"

#include "Util/AutoDirectoryRestoreFileDialog.h"
#include "Util/BoostPythonHelpers.h"
#include "Objects/ObjectLayerManager.h"
#include "TrackViewFBXImportPreviewDialog.h"
#include "FBXExporterDialog.h"
#include "QtUtilWin.h"
#include "QtUtil.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimParamType.h"
#include "Maestro/Types/SequenceType.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QDragMoveEvent>
#include <QMimeData>
#include <QDebug>
#include <QTreeWidget>
#include <QCompleter>
#include <QColorDialog>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>


CTrackViewNodesCtrl::CRecord::CRecord(CTrackViewNode* pNode /*= nullptr*/)
    : m_pNode(pNode)
    , m_bVisible(false)
{
    if (pNode)
    {
        QVariant v;
        v.setValue<CTrackViewNodePtr>(pNode);
        setData(0, Qt::UserRole, v);
    }
}

class CTrackViewNodesCtrlDelegate
    : public QStyledItemDelegate
{
public:
    CTrackViewNodesCtrlDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        bool enabled = index.data(CTrackViewNodesCtrl::CRecord::EnableRole).toBool();
        QStyleOptionViewItem opt = option;
        if (!enabled)
        {
            opt.state &= ~QStyle::State_Enabled;
        }
        QStyledItemDelegate::paint(painter, opt, index);
    }
};


class CTrackViewNodesTreeWidget
    : public QTreeWidget
{
public:
    CTrackViewNodesTreeWidget(QWidget* parent)
        : QTreeWidget(parent)
        , m_controller(nullptr)
    {
        setItemDelegate(new CTrackViewNodesCtrlDelegate(this));
    }

    void setController(CTrackViewNodesCtrl* p)
    {
        m_controller = p;
    }

protected:

    // Allow both CopyActions and MoveActions to be valid drag and drop operations.
    Qt::DropActions supportedDropActions() const override
    {
        return Qt::CopyAction | Qt::MoveAction;
    }

    void dragMoveEvent(QDragMoveEvent* event)
    {
        CTrackViewNodesCtrl::CRecord* pRecord = (CTrackViewNodesCtrl::CRecord*) itemAt(event->pos());
        if (!pRecord)
        {
            return;
        }
        CTrackViewNode* pTargetNode = pRecord->GetNode();

        QTreeWidget::dragMoveEvent(event);
        if (!event->isAccepted())
        {
            return;
        }

        if (pTargetNode && pTargetNode->IsGroupNode() /*&& !m_draggedNodes.DoesContain(pTargetNode)*/)
        {
            CTrackViewAnimNode* pDragTarget = static_cast<CTrackViewAnimNode*>(pTargetNode);
            bool bAllValidReparenting = true;
            QList<CTrackViewAnimNode*> nodes = draggedNodes(event);
            Q_FOREACH(CTrackViewAnimNode * pDraggedNode, nodes)
            {
                if (!pDraggedNode->IsValidReparentingTo(pDragTarget))
                {
                    bAllValidReparenting = false;
                    break;
                }
            }

            if (!(bAllValidReparenting && !nodes.isEmpty()))
            {
                event->ignore();
            }

            return;
        }
    }

    void dropEvent(QDropEvent* event)
    {
        CTrackViewNodesCtrl::CRecord* record = (CTrackViewNodesCtrl::CRecord*) itemAt(event->pos());
        if (!record)
        {
            return;
        }
        CTrackViewNode* targetNode = record->GetNode();

        if (targetNode && targetNode->IsGroupNode())
        {
            CTrackViewAnimNode* dragTarget = static_cast<CTrackViewAnimNode*>(targetNode);
            bool allValidReparenting = true;
            QList<CTrackViewAnimNode*> nodes = draggedNodes(event);
            Q_FOREACH(CTrackViewAnimNode * draggedNode, nodes)
            {
                if (!draggedNode->IsValidReparentingTo(dragTarget))
                {
                    allValidReparenting = false;
                    break;
                }
            }

            if (allValidReparenting && !nodes.isEmpty())
            {
                // By default here the drop action is a CopyAction. That is what we want in case
                // some other random control accepts this drop (and then does nothing with the data). 
                // If that happens we will not receive any notifications. If the Action default was MoveAction,
                // the dragged items in the tree would be deleted out from under us causing a crash.
                // Since we are here, we know this drop is on the same control so we can
                // switch it to a MoveAction right now. The node parents will be fixed up below.
                event->setDropAction(Qt::MoveAction);

                QTreeWidget::dropEvent(event);
                if (!event->isAccepted())
                {
                    return;
                }

                if (nodes.size() > 0)
                {
                    // All nodes are from the same sequence
                    CTrackViewSequence* sequence = nodes[0]->GetSequence();
                    AZ_Assert(nullptr != sequence, "GetSequence() should never be null");

                    if (sequence->GetSequenceType() == SequenceType::Legacy)
                    {
                        CUndo undo("Drag and Drop TrackView Nodes");
                        Q_FOREACH(CTrackViewAnimNode * draggedNode, nodes)
                        {
                            draggedNode->SetNewParent(dragTarget);
                        }
                    }
                    else
                    {
                        AzToolsFramework::ScopedUndoBatch undoBatch("Drag and Drop TrackView Nodes");
                        Q_FOREACH(CTrackViewAnimNode * draggedNode, nodes)
                        {
                            draggedNode->SetNewParent(dragTarget);
                            undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
                        }
                    }
                }
            }
        }
    }

    void keyPressEvent(QKeyEvent* event)
    {
        // HAVE TO INCLUDE CASES FOR THESE IN THE ShortcutOverride handler in ::event() below
        switch (event->key())
        {
        case Qt::Key_Tab:
            if (m_controller)
            {
                m_controller->ShowNextResult();
                event->accept();
            }
            return;
        default:
            break;
        }
        QTreeWidget::keyPressEvent(event);
    }

    bool event(QEvent* e) override
    {
        if (e->type() == QEvent::ShortcutOverride)
        {
            // since we respond to the following things, let Qt know so that shortcuts don't override us
            bool respondsToEvent = false;

            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
            if (keyEvent->key() == Qt::Key_Tab)
            {
                respondsToEvent = true;
            }

            if (respondsToEvent)
            {
                e->accept();
                return true;
            }
        }

        return QTreeWidget::event(e);
    }


    bool focusNextPrevChild(bool next)
    {
        return false;   // so we get the tab key
    }

private:
    QList<CTrackViewAnimNode*> draggedNodes(QDropEvent* event)
    {
        QByteArray encoded = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        QList<CTrackViewAnimNode*> nodes;
        while (!stream.atEnd())
        {
            int row, col;
            QMap<int, QVariant> roleDataMap;
            stream >> row >> col >> roleDataMap;

            QVariant v = roleDataMap[Qt::UserRole];
            if (v.isValid())
            {
                CTrackViewNode* pNode = v.value<CTrackViewNodePtr>();
                if (pNode && pNode->GetNodeType() == eTVNT_AnimNode)
                {
                    nodes << (CTrackViewAnimNode*)pNode;
                }
            }
        }
        return nodes;
    }

    CTrackViewNodesCtrl*    m_controller;
};

QDataStream& operator<<(QDataStream& out, const CTrackViewNodePtr& obj)
{
    out.writeRawData((const char*) &obj, sizeof(obj));
    return out;
}

QDataStream& operator>>(QDataStream& in, CTrackViewNodePtr& obj)
{
    in.readRawData((char*) &obj, sizeof(obj));
    return in;
}



enum EMenuItem
{
    eMI_SelectInViewport = 603,
    eMI_CopyNodes = 605,
    eMI_CopySelectedNodes = 602,
    eMI_PasteNodes = 604,
    eMI_RemoveSelected = 10,
    eMI_CopyKeys = 599,
    eMI_CopySelectedKeys = 600,
    eMI_PasteKeys = 601,
    eMI_AddTrackBase = 1000,
    eMI_RemoveTrack = 299,
    eMI_ExpandAll = 650,
    eMI_CollapseAll = 659,
    eMI_ExpandFolders = 660,
    eMI_CollapseFolders = 661,
    eMI_ExpandEntities = 651,
    eMI_CollapseEntities = 652,
    eMI_ExpandCameras = 653,
    eMI_CollapseCameras = 654,
    eMI_ExpandMaterials = 655,
    eMI_CollapseMaterials = 656,
    eMI_ExpandEvents = 657,
    eMI_CollapseEvents = 658,
    eMI_Rename = 11,
    eMI_CreateFolder = 610,
    eMI_AddSelectedEntities = 500,
    eMI_AddDirectorNode = 501,
    eMI_AddConsoleVariable = 502,
    eMI_AddScriptVariable = 503,
    eMI_AddMaterial = 504,
    eMI_AddEvent = 505,
    eMI_AddCurrentLayer = 506,
    eMI_AddCommentNode = 507,
    eMI_AddRadialBlur = 508,
    eMI_AddColorCorrection = 509,
    eMI_AddDOF = 510,
    eMI_AddScreenfader = 511,
    eMI_AddShadowSetup = 513,
    eMI_AddEnvironment = 514,
    eMI_EditEvents = 550,
    eMI_SaveToFBX = 12,
    eMI_ImportFromFBX = 14,
    eMI_SetAsViewCamera = 13,
    eMI_SetAsActiveDirector = 15,
    eMI_Disable = 17,
    eMI_Mute = 18,
    eMI_CustomizeTrackColor = 19,
    eMI_ClearCustomTrackColor = 20,
    eMI_ShowHideBase = 100,
    eMI_SelectSubmaterialBase = 2000,
    eMI_SetAnimationLayerBase = 3000,
};

// The 'MI' represents a Menu Item.

#include <TrackView/ui_TrackViewNodes.h>


//////////////////////////////////////////////////////////////////////////
CTrackViewNodesCtrl::CTrackViewNodesCtrl(QWidget* hParentWnd, CTrackViewDialog* parent /* = 0 */)
    : QWidget(hParentWnd)
    , m_bIgnoreNotifications(false)
    , m_bNeedReload(false)
    , m_bSelectionChanging(false)
    , ui(new Ui::CTrackViewNodesCtrl)
    , m_pTrackViewDialog(parent)
{
    ui->setupUi(this);
    m_pDopeSheet = 0;
    m_currentMatchIndex = 0;
    m_matchCount = 0;

    qRegisterMetaType<CTrackViewNodePtr>("CTrackViewNodePtr");
    qRegisterMetaTypeStreamOperators<CTrackViewNodePtr>("CTrackViewNodePtr");

    ui->treeWidget->hide();
    ui->searchField->hide();
    ui->searchCount->hide();
    ui->searchField->installEventFilter(this);

    ui->treeWidget->setController(this);
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested, this, &CTrackViewNodesCtrl::OnNMRclick);
    connect(ui->treeWidget, &QTreeWidget::itemExpanded, this, &CTrackViewNodesCtrl::OnItemExpanded);
    connect(ui->treeWidget, &QTreeWidget::itemCollapsed, this, &CTrackViewNodesCtrl::OnItemExpanded);
    connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &CTrackViewNodesCtrl::OnSelectionChanged);
    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, this, &CTrackViewNodesCtrl::OnItemDblClick);

    connect(ui->searchField, &QLineEdit::textChanged, this, &CTrackViewNodesCtrl::OnFilterChange);

    // legacy node icons are enumerated and stored as png files on disk
    for (int i = 0; i <= 29; i++)
    {
        QIcon icon(QString(":/nodes/tvnodes-%1.png").arg(i, 2, 10, QLatin1Char('0')));
        if (!icon.isNull())
        {
            m_imageList[i] = icon;
        }
    }

    m_bEditLock = false;

    m_arrowCursor = Qt::ArrowCursor;
    m_noIcon = Qt::ForbiddenCursor;

    ///////////////////////////////////////////////////////////////
    // Populate m_componentTypeToIconMap with all component icons
    AZ::SerializeContext* serializeContext = nullptr;
    EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(serializeContext, "Failed to acquire serialize context.");

    serializeContext->EnumerateDerived<AZ::Component>([this](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&) -> bool
    {
        AZStd::string iconPath;
        EBUS_EVENT_RESULT(iconPath, AzToolsFramework::EditorRequests::Bus, GetComponentEditorIcon, classData->m_typeId, nullptr);
        if (!iconPath.empty())
        {
            m_componentTypeToIconMap[classData->m_typeId] = QIcon(iconPath.c_str());
        }

        return true; // continue enumerating
    });
    ///////////////////////////////////////////////////////////////


    GetIEditor()->GetUndoManager()->AddListener(this);
};

//////////////////////////////////////////////////////////////////////////
CTrackViewNodesCtrl::~CTrackViewNodesCtrl()
{
    GetIEditor()->GetUndoManager()->RemoveListener(this);
}

bool CTrackViewNodesCtrl::eventFilter(QObject* o, QEvent* e)
{
    if (o == ui->searchField && e->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
        if (keyEvent->key() == Qt::Key_Tab && keyEvent->modifiers() == Qt::NoModifier)
        {
            ShowNextResult();
            return true;
        }
    }
    return QWidget::eventFilter(o, e);
}


//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnSequenceChanged()
{
    assert(m_pTrackViewDialog);

    m_nodeToRecordMap.clear();
    ui->treeWidget->clear();

    FillAutoCompletionListForFilter();

    Reload();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::SetDopeSheet(CTrackViewDopeSheetBase* pDopeSheet)
{
    m_pDopeSheet = pDopeSheet;
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewNodesCtrl::GetIconIndexForTrack(const CTrackViewTrack* pTrack) const
{
    int nImage = 13; // Default

    if (!pTrack)
    {
        return nImage;
    }

    CAnimParamType paramType = pTrack->GetParameterType();
    AnimValueType valueType = pTrack->GetValueType();
    AnimNodeType nodeType = pTrack->GetAnimNode()->GetType();

    // If it's a track which belongs to the post-fx node,
    // just use a default icon.
    if (nodeType == AnimNodeType::RadialBlur
        || nodeType == AnimNodeType::ColorCorrection
        || nodeType == AnimNodeType::DepthOfField
        || nodeType == AnimNodeType::ShadowSetup)
    {
        return nImage;
    }

    AnimParamType type = paramType.GetType();

    if (type == AnimParamType::FOV)
    {
        nImage = 2;
    }
    else if (type == AnimParamType::Position)
    {
        nImage = 3;
    }
    else if (type == AnimParamType::Rotation)
    {
        nImage = 4;
    }
    else if (type == AnimParamType::Scale)
    {
        nImage = 5;
    }
    else if (type == AnimParamType::Event || type == AnimParamType::TrackEvent)
    {
        nImage = 6;
    }
    else if (type == AnimParamType::Visibility)
    {
        nImage = 7;
    }
    else if (type == AnimParamType::Camera)
    {
        nImage = 8;
    }
    else if (type == AnimParamType::Sound)
    {
        nImage = 9;
    }
    else if (type == AnimParamType::Animation || type == AnimParamType::TimeRanges || valueType == AnimValueType::CharacterAnim || valueType == AnimValueType::AssetBlend)
    {
        nImage = 10;
    }
    else if (type == AnimParamType::Sequence)
    {
        nImage = 11;
    }
    else if (type == AnimParamType::Float)
    {
        nImage = 13;
    }
    else if (type == AnimParamType::Capture)
    {
        nImage = 25;
    }
    else if (type == AnimParamType::Console)
    {
        nImage = 15;
    }
    else if (type == AnimParamType::LookAt)
    {
        nImage = 17;
    }
    else if (type == AnimParamType::TimeWarp)
    {
        nImage = 22;
    }
    else if (type == AnimParamType::CommentText)
    {
        nImage = 23;
    }
    else if (type == AnimParamType::ShakeMultiplier || type == AnimParamType::TransformNoise)
    {
        nImage = 28;
    }

    return nImage;
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewNodesCtrl::GetIconIndexForNode(AnimNodeType type) const
{
    int nImage = 0;
    if (type == AnimNodeType::Entity)
    {
        // TODO: Get specific icons for lights, particles etc.
        nImage = 21;
    }
    else if (type == AnimNodeType::AzEntity)
    {
        nImage = 29;
    }
    else if (type == AnimNodeType::Director)
    {
        nImage = 27;
    }
    else if (type == AnimNodeType::Camera)
    {
        nImage = 8;
    }
    else if (type == AnimNodeType::CVar)
    {
        nImage = 15;
    }
    else if (type == AnimNodeType::ScriptVar)
    {
        nImage = 14;
    }
    else if (type == AnimNodeType::Material)
    {
        nImage = 16;
    }
    else if (type == AnimNodeType::Event)
    {
        nImage = 6;
    }
    else if (type == AnimNodeType::Group)
    {
        nImage = 1;
    }
    else if (type == AnimNodeType::Layer)
    {
        nImage = 20;
    }
    else if (type == AnimNodeType::Comment)
    {
        nImage = 23;
    }
    else if (type == AnimNodeType::Light)
    {
        nImage = 18;
    }
    else if (type == AnimNodeType::ShadowSetup)
    {
        nImage = 24;
    }

    return nImage;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewNodesCtrl::CRecord* CTrackViewNodesCtrl::AddAnimNodeRecord(CRecord* pParentRecord, CTrackViewAnimNode* pAnimNode)
{
    CRecord* pNewRecord = new CRecord(pAnimNode);

    pNewRecord->setText(0, pAnimNode->GetName());
    UpdateAnimNodeRecord(pNewRecord, pAnimNode);
    pParentRecord->insertChild(GetInsertPosition(pParentRecord, pAnimNode), pNewRecord);
    FillNodesRec(pNewRecord, pAnimNode);

    return pNewRecord;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewNodesCtrl::CRecord* CTrackViewNodesCtrl::AddTrackRecord(CRecord* pParentRecord, CTrackViewTrack* pTrack)
{
    const CTrackViewAnimNode* pAnimNode = pTrack->GetAnimNode();

    CRecord* pNewTrackRecord = new CRecord(pTrack);
    pNewTrackRecord->setSizeHint(0, QSize(30, 18));
    pNewTrackRecord->setText(0, pTrack->GetName());
    UpdateTrackRecord(pNewTrackRecord, pTrack);
    pParentRecord->insertChild(GetInsertPosition(pParentRecord, pTrack), pNewTrackRecord);
    FillNodesRec(pNewTrackRecord, pTrack);

    return pNewTrackRecord;
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewNodesCtrl::GetInsertPosition(CRecord* pParentRecord, CTrackViewNode* pNode)
{
    // Search for insert position
    const int siblingCount = pParentRecord->childCount();
    for (int i = 0; i < siblingCount; ++i)
    {
        CRecord* pRecord = static_cast<CRecord*>(pParentRecord->child(i));
        CTrackViewNode* pSiblingNode = pRecord->GetNode();

        if (*pNode < *pSiblingNode)
        {
            return i;
        }
    }

    return siblingCount;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::AddNodeRecord(CRecord* pRecord, CTrackViewNode* pNode)
{
    assert(m_nodeToRecordMap.find(pNode) == m_nodeToRecordMap.end());
    if (m_nodeToRecordMap.find(pNode) != m_nodeToRecordMap.end())
    {
        // For safety. Shouldn't happen
        return;
    }

    CRecord* pNewRecord = nullptr;

    if (pNode->IsHidden())
    {
        return;
    }

    switch (pNode->GetNodeType())
    {
    case eTVNT_AnimNode:
        pNewRecord = AddAnimNodeRecord(pRecord, static_cast<CTrackViewAnimNode*>(pNode));
        break;
    case eTVNT_Track:
        pNewRecord = AddTrackRecord(pRecord, static_cast<CTrackViewTrack*>(pNode));
        break;
    }

    if (pNewRecord)
    {
        if (!pNode->IsGroupNode() && pNode->GetChildCount() == 0)       // groups and compound tracks are draggable
        {
            pNewRecord->setFlags(pNewRecord->flags() & ~Qt::ItemIsDragEnabled);
        }
        if (!pNode->IsGroupNode())                                      // only groups can be dropped into
        {
            pNewRecord->setFlags(pNewRecord->flags() & ~Qt::ItemIsDropEnabled);
        }
        if (pNode->GetExpanded())
        {
            pNewRecord->setExpanded(true);
        }

        if (pNode->IsSelected())
        {
            m_bIgnoreNotifications = true;
            SelectRow(pNode, false, false);
            m_bIgnoreNotifications = false;
        }

        m_nodeToRecordMap[pNode] = pNewRecord;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::FillNodesRec(CRecord* pRecord, CTrackViewNode* pCurrentNode)
{
    const unsigned int childCount = pCurrentNode->GetChildCount();

    for (unsigned int childIndex = 0; childIndex < childCount; ++childIndex)
    {
        CTrackViewNode* pNode = pCurrentNode->GetChild(childIndex);

        if (!pNode->IsHidden())
        {
            AddNodeRecord(pRecord, pNode);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::UpdateNodeRecord(CRecord* pRecord)
{
    CTrackViewNode* pNode = pRecord->GetNode();

    if (pNode)
    {
        switch (pNode->GetNodeType())
        {
        case eTVNT_AnimNode:
            UpdateAnimNodeRecord(pRecord, static_cast<CTrackViewAnimNode*>(pNode));
            break;
        case eTVNT_Track:
            UpdateTrackRecord(pRecord, static_cast<CTrackViewTrack*>(pNode));
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::UpdateTrackRecord(CRecord* pRecord, CTrackViewTrack* pTrack)
{
    int nImage = GetIconIndexForTrack(pTrack);
    assert(m_imageList.contains(nImage));
    pRecord->setIcon(0, m_imageList[nImage]);

    // Check if parameter is valid for non sub tracks
    CTrackViewAnimNode* pAnimNode = pTrack->GetAnimNode();
    const bool isParamValid = pTrack->IsSubTrack() || pAnimNode->IsParamValid(pTrack->GetParameterType());

    // Check if disabled or muted
    const bool bDisabledOrMuted = pTrack->IsDisabled() || pTrack->IsMuted();

    // If track is not valid and disabled/muted color node in grey
    pRecord->setData(0, CRecord::EnableRole, !bDisabledOrMuted && isParamValid);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::UpdateAnimNodeRecord(CRecord* pRecord, CTrackViewAnimNode* pAnimNode)
{
    const QColor TextColorForMissingEntity(226, 52, 43);        // LY palette for 'Error/Failure'
    const QColor TextColorForInvalidMaterial(226, 52, 43);      // LY palette for 'Error/Failure'
    const QColor BackColorForActiveDirector(243, 81, 29);       // LY palette for 'Primary'
    const QColor BackColorForInactiveDirector(22, 23, 27);      // LY palette for 'Background (In Focus)'
    const QColor BackColorForGroupNodes(42, 84, 244);           // LY palette for 'Secondary'

    QFont f = font();
    f.setBold(true);
    pRecord->setFont(0, f);

    AnimNodeType nodeType = pAnimNode->GetType();
    if (nodeType == AnimNodeType::Component)
    {
        // get the component icon from cached component icons
        bool searchCompleted = false;
        
        AZ::Entity* azEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(azEntity, &AZ::ComponentApplicationBus::Events::FindEntity, 
                                                        static_cast<CTrackViewAnimNode*>(pAnimNode->GetParentNode())->GetAzEntityId());
        if (azEntity)
        {
            const AZ::Component* component = azEntity->FindComponent(pAnimNode->GetComponentId());
            if (component)
            {
                auto findIter = m_componentTypeToIconMap.find(AzToolsFramework::GetUnderlyingComponentType(*component));
                if (findIter != m_componentTypeToIconMap.end())
                {
                    pRecord->setIcon(0, findIter->second);
                }
            }           
        }     
    }
    else
    {
        // legacy node icons
        int nNodeImage = GetIconIndexForNode(nodeType);
        assert(m_imageList.contains(nNodeImage));

        pRecord->setIcon(0, m_imageList[nNodeImage]);
    }
    

    const bool bDisabled = pAnimNode->IsDisabled();
    pRecord->setData(0, CRecord::EnableRole, !bDisabled);

    if (nodeType == AnimNodeType::Group)
    {
        pRecord->setBackgroundColor(0, BackColorForGroupNodes);
        pRecord->setSizeHint(0, QSize(30, 20));
    }
    else if (nodeType == AnimNodeType::Entity || nodeType == AnimNodeType::Camera || nodeType == AnimNodeType::GeomCache || nodeType == AnimNodeType::AzEntity)
    {
        if (!pAnimNode->GetNodeEntity(false))
        {
            // In case of a missing entity,color it red.
            pRecord->setForeground(0, TextColorForMissingEntity);
        }
    }
    else if (nodeType == AnimNodeType::Material)
    {
        // Check if a valid material can be found by the node name.
        _smart_ptr<IMaterial> pMaterial = nullptr;
        QString matName;
        int subMtlIndex = GetMatNameAndSubMtlIndexFromName(matName, pAnimNode->GetName());
        pMaterial = gEnv->p3DEngine->GetMaterialManager()->FindMaterial(matName.toUtf8().data());
        if (pMaterial)
        {
            bool bMultiMat = pMaterial->GetSubMtlCount() > 0;
            bool bMultiMatWithoutValidIndex = bMultiMat && (subMtlIndex < 0 || subMtlIndex >= pMaterial->GetSubMtlCount());
            bool bLeafMatWithIndex = !bMultiMat && subMtlIndex != -1;
            if (bMultiMatWithoutValidIndex || bLeafMatWithIndex)
            {
                pMaterial = nullptr;
            }
        }

        if (!pMaterial)
        {
            pRecord->setForeground(0, TextColorForInvalidMaterial);
        }
        else
        {
            // set to default color from palette
            // materials that originally pointed to material groups and are changed to sub-materials need this to reset their color
            pRecord->setForeground(0, palette().color(foregroundRole()));
        }
    }

    // Mark the active director and other directors properly.
    if (pAnimNode->IsActiveDirector())
    {
        pRecord->setBackgroundColor(0, BackColorForActiveDirector);
    }
    else if (nodeType == AnimNodeType::Director)
    {
        pRecord->setBackgroundColor(0, BackColorForInactiveDirector);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::Reload()
{
    ui->treeWidget->clear();
    OnFillItems();
}


void CTrackViewNodesCtrl::OnFillItems()
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (pSequence)
    {
        CTrackViewSequenceNotificationContext context(pSequence);

        m_nodeToRecordMap.clear();

        CRecord* pRootGroupRec = new CRecord(pSequence);
        pRootGroupRec->setText(0, pSequence->GetName());
        QFont f = font();
        f.setBold(true);
        pRootGroupRec->setData(0, Qt::FontRole, f);
        pRootGroupRec->setSizeHint(0, QSize(width(), 24));

        m_nodeToRecordMap[pSequence] = pRootGroupRec;
        ui->treeWidget->addTopLevelItem(pRootGroupRec);

        FillNodesRec(pRootGroupRec, pSequence);
        pRootGroupRec->setExpanded(pSequence->GetExpanded());

        // Additional empty record like space for scrollbar in key control
        CRecord* pGroupRec = new CRecord();
        pGroupRec->setSizeHint(0, QSize(width(), 18));
        ui->treeWidget->addTopLevelItem(pRootGroupRec);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnItemExpanded(QTreeWidgetItem* item)
{
    CRecord* pRecord = (CRecord*) item;

    if (pRecord && pRecord->GetNode())
    {
        bool currentlyExpanded = pRecord->GetNode()->GetExpanded();
        bool expanded = item->isExpanded();

        if (expanded != currentlyExpanded)
        {
            bool isDuringUndo = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

            // Don't record another undo event if this OnItemExpanded callback is fired because we are Undoing or Redoing.
            if (isDuringUndo)
            {
                pRecord->GetNode()->SetExpanded(expanded);
            }
            else
            {
                CTrackViewSequence* sequence = pRecord->GetNode()->GetSequence();
                AZ_Assert(nullptr != sequence, "Expected valid sequence");
                AzToolsFramework::ScopedUndoBatch undoBatch("Set Node Expanded");
                pRecord->GetNode()->SetExpanded(expanded);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
        }
    }

    UpdateDopeSheet();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnSelectionChanged()
{
    // Need to avoid the second call to this, because GetSelectedRows is broken
    // with multi selection
    if (m_bSelectionChanging)
    {
        return;
    }
    m_bSelectionChanging = true;

    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (pSequence)
    {
        CTrackViewSequenceNotificationContext context(pSequence);
        pSequence->ClearSelection();

        QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
        int nCount = items.count();
        for (int i = 0; i < nCount; i++)
        {
            CRecord* pRecord = (CRecord*)items.at(i);

            if (pRecord && pRecord->GetNode())
            {
                if (!pRecord->GetNode()->IsSelected())
                {
                    pRecord->GetNode()->SetSelected(true);
                    ui->treeWidget->setCurrentItem(pRecord);
                }
            }
        }
    }

    m_bSelectionChanging = false;
    UpdateDopeSheet();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnNMRclick(QPoint point)
{
    CRecord* pRecord = 0;
    bool isOnAzEntity = false;
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSequence)
    {
        return;
    }

    bool isLegacySequence = pSequence->GetSequenceType() == SequenceType::Legacy;

    CTrackViewSequenceNotificationContext context(pSequence);

    // Find node under mouse.
    // Select the item that is at the point myPoint.
    pRecord = static_cast<CRecord*>(ui->treeWidget->itemAt(point));

    CTrackViewAnimNode* pGroupNode = nullptr;
    CTrackViewNode* pNode = nullptr;
    CTrackViewAnimNode* pAnimNode = nullptr;
    CTrackViewTrack* pTrack = nullptr;

    if (pRecord && pRecord->GetNode())
    {
        pNode = pRecord->GetNode();

        if (pNode)
        {
            ETrackViewNodeType nodeType = pNode->GetNodeType();

            if (nodeType == eTVNT_AnimNode)
            {
                pAnimNode = static_cast<CTrackViewAnimNode*>(pNode);
                isOnAzEntity = (pAnimNode && pAnimNode->GetType() == AnimNodeType::AzEntity);

                if (pAnimNode->GetType() == AnimNodeType::Director || pAnimNode->GetType() == AnimNodeType::Group || isOnAzEntity)
                {
                    pGroupNode = pAnimNode;
                }
            }
            else if (nodeType == eTVNT_Sequence)
            {
                pGroupNode = pSequence;
            }
            else if (nodeType == eTVNT_Track)
            {
                pTrack = static_cast<CTrackViewTrack*>(pNode);
                pAnimNode = pTrack->GetAnimNode();
            }
        }
    }
    else
    {
        pNode = pSequence;
        pGroupNode = pSequence;
        pRecord = m_nodeToRecordMap[pSequence];
    }

    int cmd = ShowPopupMenu(point, pRecord);

    float scrollPos = SaveVerticalScrollPos();

    if (cmd == eMI_SaveToFBX)
    {
        CExportManager* pExportManager = static_cast<CExportManager*>(GetIEditor()->GetExportManager());

        if (pExportManager)
        {
            CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
            if (!pSequence)
            {
                return;
            }

            CTrackViewAnimNodeBundle selectedNodes = pSequence->GetSelectedAnimNodes();
            const unsigned int numSelectedNodes = selectedNodes.GetCount();
            if (numSelectedNodes == 0)
            {
                return;
            }

            QString file = QString(pSequence->GetName()) + QString(".fbx");
            QString selectedSequenceFBXStr = QString(pSequence->GetName()) + ".fbx";

            if (numSelectedNodes > 1)
            {
                file = selectedSequenceFBXStr;
            }
            else
            {
                file = QString(selectedNodes.GetNode(0)->GetName()) + QString(".fbx");
            }

            QString path = QFileDialog::getSaveFileName(this, tr("Export Selected Nodes To FBX File"), QString(), tr("FBX Files (*.fbx)"));

            if (!path.isEmpty())
            {
                pExportManager->SetBakedKeysSequenceExport(false);
                pExportManager->Export(path.toUtf8().data(), "", "", false, false, false, false, true);
            }
        }
    }
    else if (cmd == eMI_ImportFromFBX)
    {
        if (pAnimNode)
        {
            ImportFromFBX();
        }
    }
    else if (cmd == eMI_SetAsViewCamera)
    {
        if (pAnimNode && pAnimNode->GetType() == AnimNodeType::Camera)
        {
            pAnimNode->SetAsViewCamera();
        }
    }
    else if (cmd == eMI_RemoveSelected)
    {
        if (isLegacySequence)
        {
            CUndo undo("Delete selected TrackView Nodes/Tracks");
            pSequence->DeleteSelectedNodes();
        }
        else
        {
            // Let the AZ Undo system manage the nodes on the sequence entity
            AzToolsFramework::ScopedUndoBatch undoBatch("Delete selected TrackView Nodes/Tracks");
            auto id = pSequence->GetSequenceComponentEntityId();
            pSequence->DeleteSelectedNodes();
            undoBatch.MarkEntityDirty(id);
        }
    }

    if (pGroupNode)
    {
        // Group operations applicable to AZEntities and Group Nodes
        if (cmd == eMI_ExpandAll)
        {
            BeginUndoTransaction();
            pGroupNode->GetAllAnimNodes().ExpandAll();
            EndUndoTransaction();
        }
        else if (cmd == eMI_CollapseAll)
        {
            BeginUndoTransaction();
            pGroupNode->GetAllAnimNodes().CollapseAll();
            EndUndoTransaction();
        }

        if (!isOnAzEntity)
        {
            // Group operations not applicable to AZEntities
            if (cmd == eMI_AddSelectedEntities)
            {
                CTrackViewAnimNodeBundle addedNodes;
                if (isLegacySequence)
                {
                    CUndo undo("Add Entities to TrackView");
                    addedNodes = pGroupNode->AddSelectedEntities(m_pTrackViewDialog->GetDefaultTracksForEntityNode());
                }
                else
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Add Entities to TrackView");
                    addedNodes = pGroupNode->AddSelectedEntities(m_pTrackViewDialog->GetDefaultTracksForEntityNode());
                    undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                }

                // check to make sure all nodes were added and notify user if they weren't
                if (addedNodes.GetCount() != GetIEditor()->GetSelection()->GetCount())
                {
                    IMovieSystem* movieSystem = GetIEditor()->GetMovieSystem();

                    AZStd::string messages = movieSystem->GetUserNotificationMsgs();

                    // Create a list of all lines
                    AZStd::vector<AZStd::string> lines;
                    AzFramework::StringFunc::Tokenize(messages.c_str(), lines, "\n");

                    // Truncate very long messages. No information is lost because
                    // all of these errors will have been logged to the console already.
                    const int maxLines = 30;
                    AZStd::string shortMessages;
                    if (lines.size() > maxLines)
                    {
                        int numLines = 0;
                        for (AZStd::string line : lines)
                        {
                            shortMessages += line + "\n";
                            if (++numLines >= maxLines)
                            {
                                break;
                            }
                        }
                        shortMessages += "Message truncated, please see console for a full list of warnings.\n";
                    }
                    else
                    {
                        shortMessages = messages;
                    }

                    QMessageBox::information(this, tr("Track View Warning"), tr(shortMessages.c_str()));

                    // clear the notification log now that we've consumed and presented them.
                    movieSystem->ClearUserNotificationMsgs();
                }

                pGroupNode->BindToEditorObjects();
            }
            else if (cmd == eMI_AddCurrentLayer)
            {
                if (isLegacySequence)
                {
                    CUndo undo("Add Current Layer to TrackView");
                    pGroupNode->AddCurrentLayer();
                }
                else
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Add Current Layer to TrackView");
                    pGroupNode->AddCurrentLayer();
                    undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                }
            }
            else if (cmd == eMI_AddScreenfader)
            {
                if (isLegacySequence)
                {
                    CUndo undo("Add TrackView Screen Fader Node");
                    pGroupNode->CreateSubNode("ScreenFader", AnimNodeType::ScreenFader);
                }
                else
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Screen Fader Node");
                    pGroupNode->CreateSubNode("ScreenFader", AnimNodeType::ScreenFader);
                    undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                }
            }
            else if (cmd == eMI_AddCommentNode)
            {
                QString commentNodeName = pGroupNode->GetAvailableNodeNameStartingWith("Comment");
                if (isLegacySequence)
                {
                    CUndo undo("Add TrackView Comment Node");
                    pGroupNode->CreateSubNode(commentNodeName, AnimNodeType::Comment);
                }
                else
                {
                    CUndo undo("Add TrackView Comment Node");
                    pGroupNode->CreateSubNode(commentNodeName, AnimNodeType::Comment);
                }
            }
            else if (cmd == eMI_AddRadialBlur)
            {
                if (isLegacySequence)
                {
                    CUndo undo("Add TrackView Radial Blur Node");
                    pGroupNode->CreateSubNode("RadialBlur", AnimNodeType::RadialBlur);
                }
                else
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Radial Blur Node");
                    pGroupNode->CreateSubNode("RadialBlur", AnimNodeType::RadialBlur);
                    undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                }
            }
            else if (cmd == eMI_AddColorCorrection)
            {
                if (isLegacySequence)
                {
                    CUndo undo("Add TrackView Color Correction Node");
                    pGroupNode->CreateSubNode("ColorCorrection", AnimNodeType::ColorCorrection);
                }
                else
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Color Correction Node");
                    pGroupNode->CreateSubNode("ColorCorrection", AnimNodeType::ColorCorrection);
                    undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                }
            }
            else if (cmd == eMI_AddDOF)
            {
                if (isLegacySequence)
                {
                    CUndo undo("Add TrackView Depth of Field Node");
                    pGroupNode->CreateSubNode("DepthOfField", AnimNodeType::DepthOfField);
                }
                else
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Depth of Field Node");
                    pGroupNode->CreateSubNode("DepthOfField", AnimNodeType::DepthOfField);
                    undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                }
            }
            else if (cmd == eMI_AddShadowSetup)
            {
                if (isLegacySequence)
                {
                    CUndo undo("Add TrackView Shadow Setup Node");
                    pGroupNode->CreateSubNode("ShadowsSetup", AnimNodeType::ShadowSetup);
                }
                else
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Shadow Setup Node");
                    pGroupNode->CreateSubNode("ShadowsSetup", AnimNodeType::ShadowSetup);
                    undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                }
            }
            else if (cmd == eMI_AddEnvironment)
            {
                if (isLegacySequence)
                {
                    CUndo undo("Add TrackView Environment Node");
                    pGroupNode->CreateSubNode("Environment", AnimNodeType::Environment);
                }
                else
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Environment Node");
                    pGroupNode->CreateSubNode("Environment", AnimNodeType::Environment);
                    undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                }
            }
            else if (cmd == eMI_AddDirectorNode)
            {
                QString name = pGroupNode->GetAvailableNodeNameStartingWith("Director");
                if (isLegacySequence)
                {
                    CUndo undo("Add TrackView Director Node");
                    pGroupNode->CreateSubNode(name, AnimNodeType::Director);
                }
                else
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Director Node");
                    pGroupNode->CreateSubNode(name, AnimNodeType::Director);
                    undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                }
            }
            else if (cmd == eMI_AddConsoleVariable)
            {
                StringDlg dlg(tr("Console Variable Name"));
                if (dlg.exec() == QDialog::Accepted && !dlg.GetString().isEmpty())
                {
                    QString name = pGroupNode->GetAvailableNodeNameStartingWith(dlg.GetString());
                    if (isLegacySequence)
                    {
                        CUndo undo("Add TrackView Console (CVar) Node");
                        pGroupNode->CreateSubNode(name, AnimNodeType::CVar);
                    }
                    else
                    {
                        AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Console (CVar) Node");
                        pGroupNode->CreateSubNode(name, AnimNodeType::CVar);
                        undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                    }
                }
            }
            else if (cmd == eMI_AddScriptVariable)
            {
                StringDlg dlg(tr("Script Variable Name"));
                if (dlg.exec() == QDialog::Accepted && !dlg.GetString().isEmpty())
                {
                    QString name = pGroupNode->GetAvailableNodeNameStartingWith(dlg.GetString());
                    if (isLegacySequence)
                    {
                        CUndo undo("Add TrackView Script Variable Node");
                        pGroupNode->CreateSubNode(name, AnimNodeType::ScriptVar);
                    }
                    else
                    {
                        AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Script Variable Node");
                        pGroupNode->CreateSubNode(name, AnimNodeType::ScriptVar);
                        undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                    }
                }
            }
            else if (cmd == eMI_AddMaterial)
            {
                StringDlg dlg(tr("Material Name"));
                if (dlg.exec() == QDialog::Accepted && !dlg.GetString().isEmpty())
                {
                    if (pGroupNode->GetAnimNodesByName(dlg.GetString().toUtf8().data()).GetCount() == 0)
                    {
                        if (isLegacySequence)
                        {
                            CUndo undo("Add TrackView Material Node");
                            pGroupNode->CreateSubNode(dlg.GetString(), AnimNodeType::Material);
                        }
                        else
                        {
                            AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Material Node");
                            pGroupNode->CreateSubNode(dlg.GetString(), AnimNodeType::Material);
                            undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                        }
                    }
                }
            }
            else if (cmd == eMI_AddEvent)
            {
                StringDlg dlg(tr("Track Event Name"));
                if (dlg.exec() == QDialog::Accepted && !dlg.GetString().isEmpty())
                {
                    if (isLegacySequence)
                    {
                        CUndo undo("Add TrackView Event Node");
                        pGroupNode->CreateSubNode(dlg.GetString(), AnimNodeType::Event);
                    }
                    else
                    {
                        AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Event Node");
                        pGroupNode->CreateSubNode(dlg.GetString(), AnimNodeType::Event);
                        undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                    }
                }
            }
            else if (cmd == eMI_PasteNodes)
            {
                if (isLegacySequence)
                {
                    CUndo undo("Paste TrackView Nodes");
                    pGroupNode->PasteNodesFromClipboard(this);
                }
                else
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Paste TrackView Nodes");
                    pGroupNode->PasteNodesFromClipboard(this);
                    undoBatch.MarkEntityDirty(pGroupNode->GetSequence()->GetSequenceComponentEntityId());
                }
            }
            else if (cmd == eMI_CreateFolder)
            {
                CreateFolder(pGroupNode);
            }
            else if (cmd == eMI_ExpandFolders)
            {
                BeginUndoTransaction();
                pGroupNode->GetAnimNodesByType(AnimNodeType::Group).ExpandAll();
                pGroupNode->GetAnimNodesByType(AnimNodeType::Director).ExpandAll();
                EndUndoTransaction();
            }
            else if (cmd == eMI_CollapseFolders)
            {
                BeginUndoTransaction();
                pGroupNode->GetAnimNodesByType(AnimNodeType::Group).CollapseAll();
                pGroupNode->GetAnimNodesByType(AnimNodeType::Director).CollapseAll();
                EndUndoTransaction();
            }
            else if (cmd == eMI_ExpandEntities)
            {
                BeginUndoTransaction();
                pGroupNode->GetAnimNodesByType(AnimNodeType::Entity).ExpandAll();
                pGroupNode->GetAnimNodesByType(AnimNodeType::AzEntity).ExpandAll();
                EndUndoTransaction();
            }
            else if (cmd == eMI_CollapseEntities)
            {
                BeginUndoTransaction();
                pGroupNode->GetAnimNodesByType(AnimNodeType::Entity).CollapseAll();
                pGroupNode->GetAnimNodesByType(AnimNodeType::AzEntity).CollapseAll();
                EndUndoTransaction();
            }
            else if (cmd == eMI_ExpandCameras)
            {
                BeginUndoTransaction();
                pGroupNode->GetAnimNodesByType(AnimNodeType::Camera).ExpandAll();
                EndUndoTransaction();
            }
            else if (cmd == eMI_CollapseCameras)
            {
                BeginUndoTransaction();
                pGroupNode->GetAnimNodesByType(AnimNodeType::Camera).CollapseAll();
                EndUndoTransaction();
            }
            else if (cmd == eMI_ExpandMaterials)
            {
                BeginUndoTransaction();
                pGroupNode->GetAnimNodesByType(AnimNodeType::Material).ExpandAll();
                EndUndoTransaction();
            }
            else if (cmd == eMI_CollapseMaterials)
            {
                BeginUndoTransaction();
                pGroupNode->GetAnimNodesByType(AnimNodeType::Material).CollapseAll();
                EndUndoTransaction();
            }
            else if (cmd == eMI_ExpandEvents)
            {
                BeginUndoTransaction();
                pGroupNode->GetAnimNodesByType(AnimNodeType::Event).ExpandAll();
                EndUndoTransaction();
            }
            else if (cmd == eMI_CollapseEvents)
            {
                BeginUndoTransaction();
                pGroupNode->GetAnimNodesByType(AnimNodeType::Event).CollapseAll();
                EndUndoTransaction();
            }
        }  
    }

    if (cmd == eMI_EditEvents)
    {
        EditEvents();
    }
    else if (cmd == eMI_Rename)
    {
        if (pAnimNode || pGroupNode)
        {
            CTrackViewAnimNode* animNode = static_cast<CTrackViewAnimNode*>(pNode);
            QString oldName = animNode->GetName();

            StringDlg dlg(tr("Rename Node"));
            dlg.SetString(oldName);

            // add check for duplicate entity names if this is bound to an Object node
            if (animNode->IsBoundToEditorObjects())
            {
                dlg.SetCheckCallback([this](QString newName) -> bool
                {
                    bool nameExists = static_cast<CObjectManager*>(GetIEditor()->GetObjectManager())->IsDuplicateObjectName(newName);
                    if (nameExists)
                    {
                        QMessageBox::warning(this, tr("Entity already exists"), QString(tr("Entity named '%1' already exists.\n\nPlease choose another unique name.")).arg(newName));
                        return false;
                    }
                    // Max name length is 512 when creating a new sequence, match that here for rename.
                    // It would be nice to make this a restriction at input but I didnt see a way to do that with StringDlg and this is
                    // very unlikely to happen in normal usage.
                    const int maxLenth = 512;
                    if (newName.length() > maxLenth)
                    {
                        QMessageBox::warning(
                            this,
                            tr("New entity name is too long"), 
                            QString(tr("New entity name is over the maximum of %1.\n\nPlease reduce the length.")).arg(maxLenth)
                        );
                        return false;
                    }
                    return true;
                });
            }

            if (dlg.exec() == QDialog::Accepted)
            {
                const CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();
                QString name = dlg.GetString();
                sequenceManager->RenameNode(animNode, name.toUtf8().data());
                UpdateNodeRecord(pRecord);
            }
        }
    }
    else if (cmd == eMI_SetAsActiveDirector)
    {
        if (pNode && pNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pAnimNode = static_cast<CTrackViewAnimNode*>(pNode);
            pAnimNode->SetAsActiveDirector();
        }
    }
    else if (cmd >= eMI_AddTrackBase && cmd < eMI_AddTrackBase + 1000)
    {
        if (pAnimNode)
        {
            UINT_PTR menuId = cmd - eMI_AddTrackBase;
            
            if (pAnimNode->GetType() != AnimNodeType::AzEntity)
            {
                // add track
                auto findIter = m_menuParamTypeMap.find(menuId);
                if (findIter != m_menuParamTypeMap.end())
                {
                    if (isLegacySequence)
                    {
                        CUndo undo("Add TrackView Track");
                        pAnimNode->CreateTrack(findIter->second);
                    }
                    else
                    {
                        AzToolsFramework::ScopedUndoBatch undoBatch("Add TrackView Track");
                        pAnimNode->CreateTrack(findIter->second);
                        undoBatch.MarkEntityDirty(pAnimNode->GetSequence()->GetSequenceComponentEntityId());
                    }
                }
            }                   
        }
    }
    else if (cmd == eMI_RemoveTrack)
    {
        if (pTrack)
        {

            if (isLegacySequence)
            {
                CUndo undo("Remove TrackView Track");
                pTrack->GetAnimNode()->RemoveTrack(pTrack);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Remove TrackView Track");
                pTrack->GetAnimNode()->RemoveTrack(pTrack);
                undoBatch.MarkEntityDirty(pTrack->GetSequence()->GetSequenceComponentEntityId());
            }

        }
    }
    else if (cmd >= eMI_ShowHideBase && cmd < eMI_ShowHideBase + 100)
    {
        if (pAnimNode)
        {
            unsigned int childIndex = cmd - eMI_ShowHideBase;

            if (childIndex < pAnimNode->GetChildCount())
            {
                CTrackViewNode* pChild = pAnimNode->GetChild(childIndex);
                pChild->SetHidden(!pChild->IsHidden());
            }
        }
    }
    else if (cmd == eMI_CopyKeys)
    {
        pSequence->CopyKeysToClipboard(false, true);
    }
    else if (cmd == eMI_CopySelectedKeys)
    {
        pSequence->CopyKeysToClipboard(true, true);
    }
    else if (cmd == eMI_PasteKeys)
    {
        CUndo undo("Paste TrackView Keys");
        pSequence->PasteKeysFromClipboard(pAnimNode, pTrack);
    }
    else if (cmd == eMI_CopyNodes)
    {
        if (pAnimNode)
        {
            pAnimNode->CopyNodesToClipboard(false, this);
        }
        else
        {
            pSequence->CopyNodesToClipboard(false, this);
        }
    }
    else if (cmd == eMI_CopySelectedNodes)
    {
        pSequence->CopyNodesToClipboard(true, this);
    }
    else if (cmd == eMI_SelectInViewport)
    {
        CUndo undo("Select TrackView Nodes in Viewport");
        pSequence->SelectSelectedNodesInViewport();
    }
    else if (cmd >= eMI_SelectSubmaterialBase && cmd < eMI_SelectSubmaterialBase + 100)
    {
        if (pAnimNode)
        {
            QString matName;
            int subMtlIndex = GetMatNameAndSubMtlIndexFromName(matName, pAnimNode->GetName());
            QString newMatName;
            newMatName = tr("%1.[%2]").arg(matName).arg(cmd - eMI_SelectSubmaterialBase + 1);
            CUndo undo("Rename TrackView node");
            pAnimNode->SetName(newMatName.toUtf8().data());
            pAnimNode->SetSelected(true);
            UpdateNodeRecord(pRecord);
        }
    }
    else if (cmd >= eMI_SetAnimationLayerBase && cmd < eMI_SetAnimationLayerBase + 100)
    {
        if (pNode && pNode->GetNodeType() == eTVNT_Track)
        {
            CTrackViewTrack* pTrack = static_cast<CTrackViewTrack*>(pNode);
            pTrack->SetAnimationLayerIndex(cmd - eMI_SetAnimationLayerBase);
        }
    }
    else if (cmd == eMI_Disable)
    {
        if (pNode)
        {
            CTrackViewSequence* sequence = pNode->GetSequence();
            AZ_Assert(nullptr != sequence, "Expected valid sequence");
            AzToolsFramework::ScopedUndoBatch undoBatch("Node Set Disabled");
            pNode->SetDisabled(!pNode->IsDisabled());
            undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
        }
    }
    else if (cmd == eMI_Mute)
    {
        if (pTrack)
        {
            pTrack->SetMuted(!pTrack->IsMuted());
        }
    }
    else if (cmd == eMI_CustomizeTrackColor)
    {
        CustomizeTrackColor(pTrack);
    }
    else if (cmd == eMI_ClearCustomTrackColor)
    {
        if (pTrack)
        {
            pTrack->ClearCustomColor();
        }
    }

    if (cmd)
    {
        RestoreVerticalScrollPos(scrollPos);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnItemDblClick(QTreeWidgetItem* item, int)
{
    CRecord* pRecord = (CRecord*)item;
    if (pRecord && pRecord->GetNode())
    {
        CTrackViewNode* pNode = pRecord->GetNode();

        if (pNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pAnimNode = static_cast<CTrackViewAnimNode*>(pNode);
            CEntityObject* pEntity = pAnimNode->GetNodeEntity();

            if (pEntity)
            {
                CUndo undo("Select Object");
                GetIEditor()->ClearSelection();
                GetIEditor()->SelectObject(pEntity);
            }
        }
    }
}

CTrackViewTrack* CTrackViewNodesCtrl::GetTrackViewTrack(const Export::EntityAnimData* pAnimData, CTrackViewTrackBundle trackBundle, const QString& nodeName)
{
    for (unsigned int trackID = 0; trackID < trackBundle.GetCount(); ++trackID)
    {
        CTrackViewTrack* pTrack = trackBundle.GetTrack(trackID);
        const QString bundleTrackName = pTrack->GetAnimNode()->GetName();

        if (bundleTrackName.compare(nodeName, Qt::CaseInsensitive) != 0)
        {
            continue;
        }

        // Position, Rotation
        if (pTrack->IsCompoundTrack())
        {
            for (unsigned int childTrackID = 0; childTrackID < pTrack->GetChildCount(); ++childTrackID)
            {
                CTrackViewTrack* childTrack = static_cast<CTrackViewTrack*>(pTrack->GetChild(childTrackID));
                // Have to cast GetType to int since the enum it returns is not the same enum that pAnimData->dataType is
                if (static_cast<int>(childTrack->GetParameterType().GetType()) == pAnimData->dataType)
                {
                    return childTrack;
                }
            }
        }

        // FOV
        // Have to cast GetType to int since the enum it returns is not the same enum that pAnimData->dataType is
        if (static_cast<int>(pTrack->GetParameterType().GetType()) == pAnimData->dataType)
        {
            return pTrack;
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::ImportFromFBX()
{
    CExportManager* pExportManager = static_cast<CExportManager*>(GetIEditor()->GetExportManager());

    CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptOpen, QFileDialog::AnyFile, {}, {}, "FBX Files (*.fbx)", {}, {}, this);

    if (dlg.exec())
    {
        bool bImportResult = pExportManager->ImportFromFile(dlg.selectedFiles().first().toStdString().c_str());

        if (!bImportResult)
        {
            return;
        }
    }
    else
    {
        return;
    }

    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (pSequence)
    {
        CUndo undo("Replace Keys");
        CTrackViewTrackBundle tracks = pSequence->GetAllTracks();
        const unsigned int numTracks = tracks.GetCount();

        for (unsigned int trackID = 0; trackID < numTracks; ++trackID)
        {
            CTrackViewTrack* pTrack = tracks.GetTrack(trackID);
            CUndo::Record(new CUndoTrackObject(pTrack, false));
        }

        CTrackViewTrackBundle trackBundle = pSequence->GetAllTracks();

        const Export::CData pData = pExportManager->GetData();
        const int objectsCount = pData.GetObjectCount();

        CTrackViewFBXImportPreviewDialog importSelectionDialog;

        for (int objectID = 0; objectID < objectsCount; ++objectID)
        {
            importSelectionDialog.AddTreeItem(pData.GetObject(objectID)->name);
        }

        if (importSelectionDialog.exec() != QDialog::Accepted)
        {
            return;
        }

        // Remove all keys from the affected tracks
        for (int objectID = 0; objectID < objectsCount; ++objectID)
        {
            Export::Object* pObject = pData.GetObject(objectID);

            if (pObject)
            {
                // Clear only the selected tracks for which we have AnimNodes
                if (!importSelectionDialog.IsObjectSelected(pObject->name))
                {
                    continue;
                }
                const char* updatedNodeName = pObject->name;
                if (pSequence->GetAnimNodesByName(updatedNodeName).GetCount() == 0)
                {
                    continue;
                }

                int animatonDataCount = pObject->GetEntityAnimationDataCount();
                for (int animDataID = 0; animDataID < animatonDataCount; ++animDataID)
                {
                    const Export::EntityAnimData* pAnimData = pObject->GetEntityAnimationData(animDataID);

                    CTrackViewTrack* pTrack = GetTrackViewTrack(pAnimData, trackBundle, (QString)updatedNodeName);

                    if (pTrack)
                    {
                        CTrackViewKeyBundle keys = pTrack->GetAllKeys();

                        for (int deleteKeyID = (int)keys.GetKeyCount() - 1; deleteKeyID >= 0; --deleteKeyID)
                        {
                            CTrackViewKeyHandle key = keys.GetKey(deleteKeyID);
                            key.Delete();
                        }
                    }
                }
            }
        }
        // Add keys from FBX file
        for (int objectID = 0; objectID < objectsCount; ++objectID)
        {
            const Export::Object* pObject = pData.GetObject(objectID);

            if (pObject)
            {
                // only process selected nodes from file for which we have AnimNodes
                if (!importSelectionDialog.IsObjectSelected(pObject->name))
                {
                    continue;
                }
                const char* updatedNodeName = pObject->name;
                if (pSequence->GetAnimNodesByName(updatedNodeName).GetCount() == 0)
                {
                    continue;
                }

                const int animatonDataCount = pObject->GetEntityAnimationDataCount();

                // Add keys from the imported file to the selected tracks
                for (int animDataID = 0; animDataID < animatonDataCount; ++animDataID)
                {
                    const Export::EntityAnimData* pAnimData = pObject->GetEntityAnimationData(animDataID);
                    CTrackViewTrack* pTrack = GetTrackViewTrack(pAnimData, trackBundle, (QString)updatedNodeName);

                    if (pTrack)
                    {
                        CTrackViewKeyHandle key = pTrack->CreateKey(pAnimData->keyTime);
                        I2DBezierKey bezierKey;
                        key.GetKey(&bezierKey);
                        bezierKey.value = Vec2(pAnimData->keyTime, pAnimData->keyValue);
                        key.SetKey(&bezierKey);
                    }
                }

                // After all keys are added, we are able to add the left and right tangents to the imported keys
                for (int animDataID = 0; animDataID < animatonDataCount; ++animDataID)
                {
                    const Export::EntityAnimData* pAnimData = pObject->GetEntityAnimationData(animDataID);

                    CTrackViewTrack* pTrack = GetTrackViewTrack(pAnimData, trackBundle, (QString)updatedNodeName);

                    if (pTrack)
                    {
                        CTrackViewKeyHandle key = pTrack->GetKeyByTime(pAnimData->keyTime);
                        ISplineInterpolator* pSpline = pTrack->GetSpline();

                        if (pSpline)
                        {
                            const int keyIndex = key.GetIndex();

                            ISplineInterpolator::ValueType inTangent;
                            ISplineInterpolator::ValueType outTangent;
                            ZeroStruct(inTangent);
                            ZeroStruct(outTangent);

                            float currentKeyTime = 0.0f;
                            pSpline->SetKeyFlags(keyIndex, SPLINE_KEY_TANGENT_BROKEN);

                            if (keyIndex > 0)
                            {
                                currentKeyTime = key.GetTime() - key.GetPrevKey().GetTime();
                                inTangent[0] = pAnimData->leftTangentWeight * currentKeyTime;
                                inTangent[1] = inTangent[0] * pAnimData->leftTangent;
                                pSpline->SetKeyInTangent(keyIndex, inTangent);
                            }

                            if (keyIndex < (pTrack->GetKeyCount() - 1))
                            {
                                CTrackViewKeyHandle nextKey = key.GetNextKey();
                                currentKeyTime = nextKey.GetTime() - key.GetTime();
                                outTangent[0] = pAnimData->rightTangentWeight * currentKeyTime;
                                outTangent[1] = outTangent[0] * pAnimData->rightTangent;
                                pSpline->SetKeyOutTangent(keyIndex, outTangent);
                            }
                        }
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::EditEvents()
{
    CTVEventsDialog dlg;
    dlg.exec();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::CreateFolder(CTrackViewAnimNode* pGroupNode)
{
    // Change Group of the node.
    StringDlg dlg(tr("Enter Folder Name"));
    if (dlg.exec() == QDialog::Accepted)
    {
        QString name = dlg.GetString();
        if (name.isEmpty())
        {
            return;
        }

        CUndo undo("Create folder");
        if (!pGroupNode->CreateSubNode(name, AnimNodeType::Group))
        {
            QMessageBox::critical(this, QString(), tr("The name already exists. Use another."));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
struct STrackMenuTreeNode
{
    QMenu menu;
    CAnimParamType paramType;
    std::map<QString, std::unique_ptr<STrackMenuTreeNode> > children;
};

//////////////////////////////////////////////////////////////////////////
struct SContextMenu
{
    QMenu main;
    QMenu expandSub;
    QMenu collapseSub;
    QMenu setLayerSub;
    STrackMenuTreeNode addTrackSub;
    QMenu addComponentSub;
};

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::AddGroupNodeAddItems(SContextMenu& contextMenu, CTrackViewAnimNode* pAnimNode)
{
    contextMenu.main.addAction("Create Folder")->setData(eMI_CreateFolder);

    CSelectionGroup* selection = GetIEditor()->GetSelection();
    if (selection && selection->GetCount() > 0)
    {
        const char* msg = (selection->GetCount() == 1) ? "Add Selected Entity" : "Add Selected Entities";
        contextMenu.main.addAction(msg)->setData(eMI_AddSelectedEntities);
    }

    const bool bIsDirectorOrSequence = (pAnimNode->GetType() == AnimNodeType::Director || pAnimNode->GetNodeType() == eTVNT_Sequence);
    CTrackViewAnimNode* pDirector = bIsDirectorOrSequence ? pAnimNode : pAnimNode->GetDirector();

    CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
    CObjectLayer* pLayer = pLayerManager->GetCurrentLayer();
    const QString name = pLayer->GetName();
    if (pDirector->GetAnimNodesByName(name.toUtf8().data()).GetCount() > 0)
    {
        contextMenu.main.addAction("Add Current Layer")->setData(eMI_AddCurrentLayer);
    }

    if (pDirector->GetAnimNodesByType(AnimNodeType::RadialBlur).GetCount() == 0)
    {
        contextMenu.main.addAction("Add Radial Blur Node")->setData(eMI_AddRadialBlur);
    }

    if (pDirector->GetAnimNodesByType(AnimNodeType::ColorCorrection).GetCount() == 0)
    {
        contextMenu.main.addAction("Add Color Correction Node")->setData(eMI_AddColorCorrection);
    }

    if (pDirector->GetAnimNodesByType(AnimNodeType::DepthOfField).GetCount() == 0)
    {
        contextMenu.main.addAction("Add Depth of Field Node")->setData(eMI_AddDOF);
    }

    if (pDirector->GetAnimNodesByType(AnimNodeType::ScreenFader).GetCount() == 0)
    {
        contextMenu.main.addAction("Add Screen Fader")->setData(eMI_AddScreenfader);
    }

    if (pDirector->GetAnimNodesByType(AnimNodeType::ShadowSetup).GetCount() == 0)
    {
        contextMenu.main.addAction("Add Shadows Setup Node")->setData(eMI_AddShadowSetup);
    }

    if (pDirector->GetAnimNodesByType(AnimNodeType::Environment).GetCount() == 0)
    {
        contextMenu.main.addAction("Add Environment Node")->setData(eMI_AddEnvironment);
    }

    // A director node cannot have another director node as a child.
    if (pAnimNode->GetType() != AnimNodeType::Director)
    {
        contextMenu.main.addAction("Add Director(Scene) Node")->setData(eMI_AddDirectorNode);
    }

    contextMenu.main.addAction("Add Comment Node")->setData(eMI_AddCommentNode);
    contextMenu.main.addAction("Add Console Variable Node")->setData(eMI_AddConsoleVariable);
    contextMenu.main.addAction("Add Script Variable Node")->setData(eMI_AddScriptVariable);
    contextMenu.main.addAction("Add Material Node")->setData(eMI_AddMaterial);
    contextMenu.main.addAction("Add Event Node")->setData(eMI_AddEvent);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::AddMenuSeperatorConditional(QMenu& menu, bool& bAppended)
{
    if (bAppended)
    {
        menu.addSeparator();
    }

    bAppended = false;
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewNodesCtrl::ShowPopupMenuSingleSelection(SContextMenu& contextMenu, CTrackViewSequence* pSequence, CTrackViewNode* pNode)
{
    bool bAppended = false;
    bool isOnComponentNode = false;
    bool isOnAzEntityNode = false;

    const bool bOnSequence = pNode->GetNodeType() == eTVNT_Sequence;
    const bool bOnNode = pNode->GetNodeType() == eTVNT_AnimNode;
    const bool bOnTrack = pNode->GetNodeType() == eTVNT_Track;
    const bool bIsLightAnimationSet = pSequence->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet;

    // Get track & anim node pointers
    CTrackViewTrack* pTrack = bOnTrack ? static_cast<CTrackViewTrack*>(pNode) : nullptr;
    const bool bOnTrackNotSub = bOnTrack && !pTrack->IsSubTrack();

    if (bOnNode)
    {
        AnimNodeType nodeType = static_cast<CTrackViewAnimNode*>(pNode)->GetType();
        if (nodeType == AnimNodeType::Component)
        {
            isOnComponentNode = true;
        }
        else if (nodeType == AnimNodeType::AzEntity)
        {
            isOnAzEntityNode = true;
        }
    }

    CTrackViewAnimNode* pAnimNode = nullptr;
    if (bOnSequence || bOnNode)
    {
        pAnimNode = static_cast<CTrackViewAnimNode*>(pNode);
    }
    else if (bOnTrack)
    {
        pAnimNode = pTrack->GetAnimNode();
    }

    bool isOnDirector = (pAnimNode && pAnimNode->GetType() == AnimNodeType::Director);
    bool isOnAzEntity = (pAnimNode && pAnimNode->GetType() == AnimNodeType::AzEntity);

    // Sequence Nodes - add 'Select In Viewport' for Sequence Components only
    if (bOnSequence && static_cast<CTrackViewSequence*>(pNode)->GetSequenceType() == SequenceType::SequenceComponent)
    {
        contextMenu.main.addAction("Select In Viewport")->setData(eMI_SelectInViewport);
        contextMenu.main.addSeparator();
    }

    // Entity
    if (bOnNode && !bIsLightAnimationSet && (pAnimNode->GetEntity() != nullptr || pAnimNode->IsBoundToAzEntity()))
    {
        AddMenuSeperatorConditional(contextMenu.main, bAppended);

        contextMenu.main.addAction("Select In Viewport")->setData(eMI_SelectInViewport);

        if (pAnimNode->GetType() == AnimNodeType::Camera)
        {
            contextMenu.main.addAction("Set As View Camera")->setData(eMI_SetAsViewCamera);
        }

        bAppended = true;
    }

    {
        bool bCopyPasteRenameAppended = false;

        // Copy & paste nodes
        if ((bOnNode || bOnSequence) && !isOnComponentNode)
        {
            AddMenuSeperatorConditional(contextMenu.main, bAppended);
            contextMenu.main.addAction("Copy")->setData(eMI_CopyNodes);
            bCopyPasteRenameAppended = true;
        }

        if (pNode->IsGroupNode() && !isOnAzEntity)
        {
            contextMenu.main.addAction("Paste")->setData(eMI_PasteNodes);
            bCopyPasteRenameAppended = true;
        }

        if ((bOnNode || bOnSequence || bOnTrackNotSub) && !isOnComponentNode)
        {
            contextMenu.main.addAction("Delete")->setData(bOnTrackNotSub ? eMI_RemoveTrack : eMI_RemoveSelected);
            bCopyPasteRenameAppended = true;
        }

        // Renaming
        if (pNode->CanBeRenamed())
        {
            AddMenuSeperatorConditional(contextMenu.main, bAppended);
            contextMenu.main.addAction("Rename")->setData(eMI_Rename);
            bCopyPasteRenameAppended = true;
        }

        bAppended = bAppended || bCopyPasteRenameAppended;
    }

    if (bOnTrack)
    {
        // Copy & paste keys
        AddMenuSeperatorConditional(contextMenu.main, bAppended);
        contextMenu.main.addAction("Copy Keys")->setData(eMI_CopyKeys);
        contextMenu.main.addAction("Copy Selected Keys")->setData(eMI_CopySelectedKeys);
        contextMenu.main.addAction("Paste Keys")->setData(eMI_PasteKeys);
        bAppended = true;
    }

    // Flags
    {
        bool bFlagAppended = false;

        if (!bOnSequence)
        {
            AddMenuSeperatorConditional(contextMenu.main, bAppended);
            QAction* a = contextMenu.main.addAction("Disabled");
            a->setData(eMI_Disable);
            a->setCheckable(true);
            a->setChecked(pNode->IsDisabled());
            bFlagAppended = true;
        }

        if (bOnTrack)
        {
            if (pTrack->GetParameterType() == AnimParamType::Sound)
            {
                AddMenuSeperatorConditional(contextMenu.main, bAppended);
                bool bMuted = pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Muted;
                QAction* a = contextMenu.main.addAction("Muted");
                a->setData(eMI_Mute);
                a->setCheckable(true);
                a->setChecked(bMuted);
                bFlagAppended = true;
            }
        }

        // In case that it's a director node instead of a normal group node,
        if (bOnNode && pAnimNode->GetType() == AnimNodeType::Director)
        {
            AddMenuSeperatorConditional(contextMenu.main, bAppended);
            QAction* a = contextMenu.main.addAction("Active Director");
            a->setData(eMI_SetAsActiveDirector);
            a->setCheckable(true);
            a->setChecked(pAnimNode->IsActiveDirector());
            bFlagAppended = true;
        }

        bAppended = bAppended || bFlagAppended;
    }

    // Expand / collapse
    if (bOnSequence || pNode->IsGroupNode())
    {
        AddMenuSeperatorConditional(contextMenu.main, bAppended);

        contextMenu.expandSub.addAction("Expand all")->setData(eMI_ExpandAll);
        contextMenu.collapseSub.addAction("Collapse all")->setData(eMI_CollapseAll);
        if (!isOnAzEntity)
        {
            contextMenu.expandSub.addAction("Expand Folders")->setData(eMI_ExpandFolders);
            contextMenu.collapseSub.addAction("Collapse Folders")->setData(eMI_CollapseFolders);
            contextMenu.expandSub.addAction("Expand Entities")->setData(eMI_ExpandEntities);
            contextMenu.collapseSub.addAction("Collapse Entities")->setData(eMI_CollapseEntities);
            contextMenu.expandSub.addAction("Expand Cameras")->setData(eMI_ExpandCameras);
            contextMenu.collapseSub.addAction("Collapse Cameras")->setData(eMI_CollapseCameras);
            contextMenu.expandSub.addAction("Expand Materials")->setData(eMI_ExpandMaterials);
            contextMenu.collapseSub.addAction("Collapse Materials")->setData(eMI_CollapseMaterials);
            contextMenu.expandSub.addAction("Expand Events")->setData(eMI_ExpandEvents);
            contextMenu.collapseSub.addAction("Collapse Events")->setData(eMI_CollapseEvents);
        }
        contextMenu.expandSub.setTitle("Expand");
        contextMenu.main.addMenu(&contextMenu.expandSub);
        contextMenu.collapseSub.setTitle("Collapse");
        contextMenu.main.addMenu(&contextMenu.collapseSub);

        bAppended = true;
    }

    // Add/Remove
    {
        if (bOnSequence || (pNode->IsGroupNode() && !isOnAzEntity) )
        {
            AddMenuSeperatorConditional(contextMenu.main, bAppended);
            AddGroupNodeAddItems(contextMenu, pAnimNode);
            bAppended = true;
        }

        if (bOnNode)
        {
            AddMenuSeperatorConditional(contextMenu.main, bAppended);

            if (!isOnAzEntity)
            {
                // Create 'Add Tracks' submenu
                m_menuParamTypeMap.clear();

                if (FillAddTrackMenu(contextMenu.addTrackSub, pAnimNode))
                {
                    // add script table properties
                    unsigned int currentId = 0;
                    CreateAddTrackMenuRec(contextMenu.main, "Add Track", pAnimNode, contextMenu.addTrackSub, currentId);
                }
            }

            bAppended = true;
        }
    }

    if (bOnNode && !bIsLightAnimationSet && !isOnDirector && !isOnComponentNode && !isOnAzEntityNode)
    {
        AddMenuSeperatorConditional(contextMenu.main, bAppended);
        contextMenu.main.addAction("Import FBX File...")->setData(eMI_ImportFromFBX);
        contextMenu.main.addAction("Export FBX File...")->setData(eMI_SaveToFBX);
        bAppended = true;
    }

    // Events
    if (bOnSequence || pNode->IsGroupNode() && !bIsLightAnimationSet && !isOnAzEntity)
    {
        AddMenuSeperatorConditional(contextMenu.main, bAppended);
        contextMenu.main.addAction("Edit Events...")->setData(eMI_EditEvents);
        bAppended = true;
    }

    // Sub material menu
    if (bOnNode && pAnimNode->GetType() == AnimNodeType::Material)
    {
        QString matName;
        int subMtlIndex = GetMatNameAndSubMtlIndexFromName(matName, pAnimNode->GetName());
        _smart_ptr<IMaterial> pMtl = gEnv->p3DEngine->GetMaterialManager()->FindMaterial(matName.toUtf8().data());
        bool bMultMatNode = pMtl ? pMtl->GetSubMtlCount() > 0 : false;

        bool bMatAppended = false;

        if (bMultMatNode)
        {
            for (int k = 0; k < pMtl->GetSubMtlCount(); ++k)
            {
                _smart_ptr<IMaterial> pSubMaterial = pMtl->GetSubMtl(k);

                if (pSubMaterial)
                {
                    QString subMaterialName = pSubMaterial->GetName();

                    if (!subMaterialName.isEmpty())
                    {
                        AddMenuSeperatorConditional(contextMenu.main, bAppended);
                        QString subMatName = QString("[%1] %2").arg(k + 1).arg(subMaterialName);
                        QAction* a = contextMenu.main.addAction(subMatName);
                        a->setData(eMI_SelectSubmaterialBase + k);
                        a->setCheckable(true);
                        a->setChecked(k == subMtlIndex);
                        bMatAppended = true;
                    }
                }
            }
        }

        bAppended = bAppended || bMatAppended;
    }

    // Delete track menu
    if (bOnTrackNotSub)
    {
        if (pTrack->GetParameterType() == AnimParamType::Animation || pTrack->GetParameterType() == AnimParamType::LookAt || pTrack->GetValueType() == AnimValueType::CharacterAnim)
        {
            // Add the set-animation-layer pop-up menu.
            AddMenuSeperatorConditional(contextMenu.main, bAppended);
            CreateSetAnimationLayerPopupMenu(contextMenu.setLayerSub, pTrack);
            contextMenu.setLayerSub.setTitle("Set Animation Layer");
            contextMenu.main.addMenu(&contextMenu.setLayerSub);
            bAppended = true;
        }
    }

    // Track color
    if (bOnTrack)
    {
        AddMenuSeperatorConditional(contextMenu.main, bAppended);
        contextMenu.main.addAction("Customize Track Color...")->setData(eMI_CustomizeTrackColor);
        if (pTrack->HasCustomColor())
        {
            contextMenu.main.addAction("Clear Custom Track Color")->setData(eMI_ClearCustomTrackColor);
        }
        bAppended = true;
    }

    // Track hide/unhide flags
    if (bOnNode && !pNode->IsGroupNode())
    {
        AddMenuSeperatorConditional(contextMenu.main, bAppended);
        QString string = QString("%1 Tracks").arg(pAnimNode->GetName());
        contextMenu.main.addAction(string)->setEnabled(false);

        bool bAppendedTrackFlag = false;

        const unsigned int numChildren = pAnimNode->GetChildCount();
        for (unsigned int childIndex = 0; childIndex < numChildren; ++childIndex)
        {
            CTrackViewNode* pChild = pAnimNode->GetChild(childIndex);
            if (pChild->GetNodeType() == eTVNT_Track)
            {
                CTrackViewTrack* pTrack = static_cast<CTrackViewTrack*>(pChild);

                if (pTrack->IsSubTrack())
                {
                    continue;
                }

                QAction* a = contextMenu.main.addAction(QString("  %1").arg(pTrack->GetName()));
                a->setData(eMI_ShowHideBase + childIndex);
                a->setCheckable(true);
                a->setChecked(!pTrack->IsHidden());
                bAppendedTrackFlag = true;
            }
        }

        bAppended = bAppendedTrackFlag || bAppended;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewNodesCtrl::ShowPopupMenuMultiSelection(SContextMenu& contextMenu)
{
    QList<QTreeWidgetItem*> records = ui->treeWidget->selectedItems();

    bool bNodeSelected = false;
    for (int currentNode = 0; currentNode < records.size(); ++currentNode)
    {
        CRecord* pItemInfo = (CRecord*)records[currentNode];
        ETrackViewNodeType type = pItemInfo->GetNode()->GetNodeType();

        if (pItemInfo->GetNode()->GetNodeType() == eTVNT_AnimNode)
        {
            bNodeSelected = true;
        }
    }

    if (bNodeSelected)
    {
        contextMenu.main.addAction("Copy Selected Nodes")->setData(eMI_CopySelectedNodes);
    }

    contextMenu.main.addAction("Remove Selected Nodes/Tracks")->setData(eMI_RemoveSelected);

    if (bNodeSelected)
    {
        contextMenu.main.addSeparator();
        contextMenu.main.addAction("Select In Viewport")->setData(eMI_SelectInViewport);

        // Importing FBX is currently only supported on legacy entities. Legacy
        // sequences contain only legacy Cry entities and no AZ component entities.
        CAnimationContext* context = GetIEditor()->GetAnimation();
        AZ_Assert(context, "Expected valid GetIEditor()->GetAnimation()");
        if (context)
        {
            CTrackViewSequence* sequence = context->GetSequence();
            if (sequence && sequence->GetSequenceType() == SequenceType::Legacy)
            {
                contextMenu.main.addAction("Import From FBX File")->setData(eMI_ImportFromFBX);
                contextMenu.main.addAction("Save To FBX File")->setData(eMI_SaveToFBX);
            }
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewNodesCtrl::ShowPopupMenu(QPoint point, const CRecord* pRecord)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSequence)
    {
        return 0;
    }

    SContextMenu contextMenu;

    CTrackViewNode* pNode = pRecord ? pRecord->GetNode() : nullptr;
    if (!pNode)
    {
        return 0;
    }

    if (ui->treeWidget->selectedItems().size() > 1)
    {
        ShowPopupMenuMultiSelection(contextMenu);
    }
    else if (pNode)
    {
        ShowPopupMenuSingleSelection(contextMenu, pSequence, pNode);
    }

    if (m_bEditLock)
    {
        SetPopupMenuLock(&contextMenu.main);
    }

    QAction* action = contextMenu.main.exec(QCursor::pos());
    int ret = action ? action->data().toInt() : 0;

    return ret;
}

//////////////////////////////////////////////////////////////////////////
// Add tracks that can be added to the given animation node to the
// internal track menu tree data structure rooted at menuAddTrack
bool CTrackViewNodesCtrl::FillAddTrackMenu(STrackMenuTreeNode& menuAddTrack, const CTrackViewAnimNode* pAnimNode)
{
    bool bTracksToAdd = false;
    const AnimNodeType nodeType = pAnimNode->GetType();
    int paramCount = 0;
    IAnimNode::AnimParamInfos animatableProperties;
    CTrackViewNode* parentNode = pAnimNode->GetParentNode();
   
    // all AZ::Entity entities are animated through components. Component nodes always have a parent - the containing AZ::Entity
    if (nodeType == AnimNodeType::Component && parentNode)
    {
        // component node - query all the animatable tracks via an EBus request

        // all AnimNodeType::Component are parented to AnimNodeType::AzEntityNodes - get the parent to get it's AZ::EntityId to use for the EBus request
        if (parentNode->GetNodeType() == eTVNT_AnimNode)
        {
            // this cast is safe because we check that the type is eTVNT_AnimNode
            const AZ::EntityId azEntityId = static_cast<CTrackViewAnimNode*>(parentNode)->GetAzEntityId();

            // query the animatable component properties from the Sequence Component
            Maestro::EditorSequenceComponentRequestBus::Event(const_cast<CTrackViewAnimNode*>(pAnimNode)->GetSequence()->GetSequenceComponentEntityId(), 
                                                                    &Maestro::EditorSequenceComponentRequestBus::Events::GetAllAnimatablePropertiesForComponent, 
                                                                    animatableProperties, azEntityId, pAnimNode->GetComponentId());

            // also add any properties handled in CryMovie
            pAnimNode->AppendNonBehaviorAnimatableProperties(animatableProperties);

            paramCount = animatableProperties.size();
        }       
    }
    else
    {
        // legacy Entity
        paramCount = pAnimNode->GetParamCount(); 
    }
    
    for (int i = 0; i < paramCount; ++i)
    {
        CAnimParamType paramType;
        QString name;

        // get the animatable param name
        if (nodeType == AnimNodeType::Component)
        {
            paramType = animatableProperties[i].paramType;
        }
        else
        {
            // legacy node
            paramType = pAnimNode->GetParamType(i);
            if (paramType == AnimParamType::Invalid)
            {
                continue;
            }

            IAnimNode::ESupportedParamFlags paramFlags = pAnimNode->GetParamFlags(paramType);

            CTrackViewTrack* pTrack = pAnimNode->GetTrackForParameter(paramType);
            if (pTrack && !(paramFlags & IAnimNode::eSupportedParamFlags_MultipleTracks))
            {
                continue;
            }
        }
        name = pAnimNode->GetParamName(paramType);
        QStringList splittedName = name.split("/", QString::SkipEmptyParts);

        STrackMenuTreeNode* pCurrentNode = &menuAddTrack;
        for (unsigned int j = 0; j < splittedName.size() - 1; ++j)
        {
            const QString& segment = splittedName[j];
            auto findIter = pCurrentNode->children.find(segment);
            if (findIter != pCurrentNode->children.end())
            {
                pCurrentNode = findIter->second.get();
            }
            else
            {
                STrackMenuTreeNode* pNewNode = new STrackMenuTreeNode;
                pCurrentNode->children[segment] = std::unique_ptr<STrackMenuTreeNode>(pNewNode);
                pCurrentNode = pNewNode;
            }
        }

        // only add tracks to the that STrackMenuTreeNode tree that haven't already been added
        CTrackViewTrackBundle matchedTracks = pAnimNode->GetTracksByParam(paramType);
        if (matchedTracks.GetCount() == 0)
        {
            STrackMenuTreeNode* pParamNode = new STrackMenuTreeNode;
            pCurrentNode->children[splittedName.back()] = std::unique_ptr<STrackMenuTreeNode>(pParamNode);
            pParamNode->paramType = paramType;

            bTracksToAdd = true;
        }  
    }
    
    return bTracksToAdd;
}

//////////////////////////////////////////////////////////////////////////
//
// FillAddTrackMenu fills the data structure for tracks to add (a STrackMenuTreeNode tree)
// CreateAddTrackMenuRec actually creates the Qt submenu from this data structure
//
void CTrackViewNodesCtrl::CreateAddTrackMenuRec(QMenu& parent, const QString& name, CTrackViewAnimNode* animNode, struct STrackMenuTreeNode& node, unsigned int& currentId)
{
    if (node.paramType.GetType() == AnimParamType::Invalid)
    {
        node.menu.setTitle(name);
        parent.addMenu(&node.menu);

        for (auto iter = node.children.begin(); iter != node.children.end(); ++iter)
        {
            CreateAddTrackMenuRec(node.menu, iter->first, animNode, *iter->second.get(), currentId);
        }
    }
    else
    {
        m_menuParamTypeMap[currentId] = node.paramType;
        QVariant paramTypeMenuID;
        paramTypeMenuID.setValue(eMI_AddTrackBase + currentId);
        parent.addAction(name)->setData(paramTypeMenuID);
        ++currentId;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::SetPopupMenuLock(QMenu* menu)
{
    if (!m_bEditLock || !menu)
    {
        return;
    }

    UINT count = menu->actions().size();
    for (UINT i = 0; i < count; ++i)
    {
        QAction* a = menu->actions()[i];
        QString menuString = a->text();

        if (menuString != "Expand" && menuString != "Collapse")
        {
            a->setEnabled(false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
float CTrackViewNodesCtrl::SaveVerticalScrollPos() const
{
    int sbMin = ui->treeWidget->verticalScrollBar()->minimum();
    int sbMax = ui->treeWidget->verticalScrollBar()->maximum();
    return float(ui->treeWidget->verticalScrollBar()->value() - sbMin) / std::max(float(sbMax - sbMin), 1.0f);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::RestoreVerticalScrollPos(float fScrollPos)
{
    int sbMin = ui->treeWidget->verticalScrollBar()->minimum();
    int sbMax = ui->treeWidget->verticalScrollBar()->maximum();
    int newScrollPos = qRound(fScrollPos * (sbMax - sbMin)) + sbMin;
    ui->treeWidget->verticalScrollBar()->setValue(newScrollPos);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::FillAutoCompletionListForFilter()
{
    QStringList strings;
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (pSequence)
    {
        ui->noitems->hide();
        ui->treeWidget->show();
        ui->searchField->show();
        ui->searchCount->show();
        CTrackViewAnimNodeBundle animNodes = pSequence->GetAllAnimNodes();
        const unsigned int animNodeCount = animNodes.GetCount();

        for (unsigned int i = 0; i < animNodeCount; ++i)
        {
            strings << animNodes.GetNode(i)->GetName();
        }
    }
    else
    {
        ui->noitems->show();
        ui->treeWidget->hide();
        ui->searchField->hide();
        ui->searchCount->hide();
    }

    QCompleter* c = new QCompleter(strings, this);
    c->setCaseSensitivity(Qt::CaseInsensitive);
    c->setCompletionMode(QCompleter::InlineCompletion);
    ui->searchField->setCompleter(c);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnFilterChange(const QString& text)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

    if (pSequence)
    {
        m_currentMatchIndex = 0;    // Reset the match index
        m_matchCount = 0;           // and the count.
        if (!text.isEmpty())
        {
            QList<QTreeWidgetItem*> items = ui->treeWidget->findItems(text, Qt::MatchContains | Qt::MatchRecursive);

            CTrackViewAnimNodeBundle animNodes = pSequence->GetAllAnimNodes();

            m_matchCount = items.size();                    // and the count.

            if (!items.empty())
            {
                ui->treeWidget->selectionModel()->clear();
                items.front()->setSelected(true);
            }
        }

        QString matchCountText = QString("%1/%2").arg(m_matchCount == 0 ? 0 : 1).arg(m_matchCount); // One-based indexing
        ui->searchCount->setText(matchCountText);
    }
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewNodesCtrl::GetMatNameAndSubMtlIndexFromName(QString& matName, const char* nodeName)
{
    if (const char* pCh = strstr(nodeName, ".["))
    {
        char matPath[MAX_PATH];
        cry_strcpy(matPath, nodeName, (size_t)(pCh - nodeName));
        matName = matPath;
        pCh += 2;
        if ((*pCh) != 0)
        {
            const int index = atoi(pCh) - 1;
            return index;
        }
    }
    else
    {
        matName = nodeName;
    }

    return -1;
}

//////////////////////////////////////////////////////////////////////////

void CTrackViewNodesCtrl::ShowNextResult()
{
    if (m_matchCount > 1)
    {
        CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();

        if (pSequence && !ui->searchField->text().isEmpty())
        {
            QList<QTreeWidgetItem*> items = ui->treeWidget->findItems(ui->searchField->text(), Qt::MatchContains | Qt::MatchRecursive);

            CTrackViewAnimNodeBundle animNodes = pSequence->GetAllAnimNodes();

            m_matchCount = items.size();                    // and the count.

            if (!items.empty())
            {
                m_currentMatchIndex = ++m_currentMatchIndex % m_matchCount;
                ui->treeWidget->selectionModel()->clear();
                items[m_currentMatchIndex]->setSelected(true);
            }

            QString matchCountText = QString("%1/%2").arg(m_currentMatchIndex + 1).arg(m_matchCount); // One-based indexing
            ui->searchCount->setText(matchCountText);
        }
    }
}

void CTrackViewNodesCtrl::keyPressEvent(QKeyEvent* event)
{
    // HAVE TO INCLUDE CASES FOR THESE IN THE ShortcutOverride handler in ::event() below
    switch (event->key())
    {
    case Qt::Key_Z:
        if (event->modifiers() == Qt::ControlModifier)
        {
            GetIEditor()->Undo();
            event->accept();
        }
        break;
    default:
        break;
    }
}

bool CTrackViewNodesCtrl::event(QEvent* e)
{
    if (e->type() == QEvent::ShortcutOverride)
    {
        // since we respond to the following things, let Qt know so that shortcuts don't override us
        bool respondsToEvent = false;

        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
        if (keyEvent->key() == Qt::Key_Z && keyEvent->modifiers() == Qt::ControlModifier)
        {
            respondsToEvent = true;
        }

        if (respondsToEvent)
        {
            e->accept();
            return true;
        }
    }

    return QWidget::event(e);
}

void CTrackViewNodesCtrl::CreateSetAnimationLayerPopupMenu(QMenu& menuSetLayer, CTrackViewTrack* pTrack) const
{
    // First collect layers already in use.
    std::vector<int> layersInUse;

    CTrackViewTrackBundle lookAtTracks = pTrack->GetAnimNode()->GetTracksByParam(AnimParamType::LookAt);
    assert(lookAtTracks.GetCount() <= 1);

    if (lookAtTracks.GetCount() > 0)
    {
        const int kDefaultLookIKLayer = 15;
        int lookIKLayerIndex = lookAtTracks.GetTrack(0)->GetAnimationLayerIndex();

        if (lookIKLayerIndex < 0) // Not set before, use the default instead.
        {
            lookIKLayerIndex = kDefaultLookIKLayer;
        }

        layersInUse.push_back(lookIKLayerIndex);
    }

    CTrackViewTrackBundle animationTracks = pTrack->GetAnimNode()->GetTracksByParam(AnimParamType::Animation);

    const unsigned int numAnimationTracks = animationTracks.GetCount();
    for (int i = 0; i < numAnimationTracks; ++i)
    {
        CTrackViewTrack* pAnimationTrack = animationTracks.GetTrack(i);
        if (pAnimationTrack)
        {
            const int kAdditiveLayerOffset = 6;
            int layerIndex = pAnimationTrack->GetAnimationLayerIndex();

            if (layerIndex < 0) // Not set before, use the default instead.
            {
                layerIndex = i == 0 ? 0 : kAdditiveLayerOffset + i;
            }

            layersInUse.push_back(layerIndex);
        }
    }

    // Add layer items.
    for (int i = 0; i < 16; ++i)
    {
        QString layerText = QString("Layer #%1").arg(i);

        QAction* a = menuSetLayer.addAction(layerText);
        a->setData(eMI_SetAnimationLayerBase + i);
        a->setCheckable(true);
        a->setChecked(pTrack->GetAnimationLayerIndex() == i);
        a->setEnabled(!stl::find(layersInUse, i));
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::CustomizeTrackColor(CTrackViewTrack* pTrack)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSequence)
    {
        return;
    }

    QColor defaultColor;
    if (pTrack->HasCustomColor())
    {
        ColorB customColor = pTrack->GetCustomColor();
        defaultColor = QColor(customColor.r, customColor.g, customColor.b);
    }
    QColor res = QColorDialog::getColor(defaultColor, this);
    if (res.isValid())
    {
        CUndo undo("Customize Track Color");
        CUndo::Record(new CUndoTrackObject(pTrack, pSequence));

        pTrack->SetCustomColor(ColorB(res.red(), res.green(), res.blue()));

        UpdateDopeSheet();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::ClearCustomTrackColor(CTrackViewTrack* pTrack)
{
    CTrackViewSequence* pSequence = GetIEditor()->GetAnimation()->GetSequence();
    if (!pSequence)
    {
        return;
    }

    CUndo undo("Clear Custom Track Color");
    CUndo::Record(new CUndoTrackObject(pTrack, pSequence));

    pTrack->ClearCustomColor();
    UpdateDopeSheet();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    UpdateDopeSheet();
}

//////////////////////////////////////////////////////////////////////////
CTrackViewNodesCtrl::CRecord* CTrackViewNodesCtrl::GetNodeRecord(const CTrackViewNode* pNode) const
{
    auto findIter = m_nodeToRecordMap.find(pNode);
    if (findIter == m_nodeToRecordMap.end())
    {
        return nullptr;
    }

    CRecord* pRecord = findIter->second;
    assert (pRecord->GetNode() == pNode);
    return findIter->second;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::UpdateDopeSheet()
{
    UpdateRecordVisibility();

    if (m_pDopeSheet)
    {
        m_pDopeSheet->update();
    }
}

//////////////////////////////////////////////////////////////////////////
// Workaround: CXTPReportRecord::IsVisible is
// unreliable after the last visible element
//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::UpdateRecordVisibility()
{
    // Mark all records invisible
    for (auto iter = m_nodeToRecordMap.begin(); iter != m_nodeToRecordMap.end(); ++iter)
    {
        iter->second->m_bVisible = ui->treeWidget->visualItemRect(iter->second).isValid();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnNodeChanged(CTrackViewNode* pNode, ITrackViewSequenceListener::ENodeChangeType type)
{
    if (pNode->GetSequence() != GetIEditor()->GetAnimation()->GetSequence())
    {
        return;
    }

    if (!m_bIgnoreNotifications)
    {
        CTrackViewNode* pParentNode = pNode->GetParentNode();

        CRecord* pNodeRecord = GetNodeRecord(pNode);
        CRecord* pParentNodeRecord = pParentNode ? GetNodeRecord(pParentNode) : nullptr;

        float storedScrollPosition = SaveVerticalScrollPos();

        switch (type)
        {
        case ITrackViewSequenceListener::eNodeChangeType_Added:
        case ITrackViewSequenceListener::eNodeChangeType_Unhidden:
            if (pParentNodeRecord)
            {
                AddNodeRecord(pParentNodeRecord, pNode);
            }
            break;
        case ITrackViewSequenceListener::eNodeChangeType_Removed:
        case ITrackViewSequenceListener::eNodeChangeType_Hidden:
            if (pNodeRecord)
            {
                EraseNodeRecordRec(pNode);
                delete pNodeRecord;
            }
            break;
        case ITrackViewSequenceListener::eNodeChangeType_Expanded:
            if (pNodeRecord)
            {
                pNodeRecord->setExpanded(true);
            }
            break;
        case ITrackViewSequenceListener::eNodeChangeType_Collapsed:
            if (pNodeRecord)
            {
                pNodeRecord->setExpanded(false);
            }
            break;
        case ITrackViewSequenceListener::eNodeChangeType_Disabled:
        case ITrackViewSequenceListener::eNodeChangeType_Enabled:
        case ITrackViewSequenceListener::eNodeChangeType_Muted:
        case ITrackViewSequenceListener::eNodeChangeType_Unmuted:
        case ITrackViewSequenceListener::eNodeChangeType_NodeOwnerChanged:
            if (pNodeRecord)
            {
                UpdateNodeRecord(pNodeRecord);
            }
        }

        switch (type)
        {
        case ITrackViewSequenceListener::eNodeChangeType_Added:
        case ITrackViewSequenceListener::eNodeChangeType_Unhidden:
        case ITrackViewSequenceListener::eNodeChangeType_Removed:
        case ITrackViewSequenceListener::eNodeChangeType_Hidden:
        case ITrackViewSequenceListener::eNodeChangeType_Expanded:
        case ITrackViewSequenceListener::eNodeChangeType_Collapsed:
            update();
            RestoreVerticalScrollPos(storedScrollPosition);
            break;
        case eNodeChangeType_SetAsActiveDirector:
            update();
            break;
        }
    }
    else
    {
        m_bNeedReload = true;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnNodeRenamed(CTrackViewNode* pNode, const char* pOldName)
{
    if (!m_bIgnoreNotifications)
    {
        CRecord* pNodeRecord = GetNodeRecord(pNode);
        pNodeRecord->setText(0, pNode->GetName());

        update();
    }
    else
    {
        m_bNeedReload = true;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::BeginUndoTransaction()
{
    m_bNeedReload = false;
    m_bIgnoreNotifications = true;
    m_storedScrollPosition = SaveVerticalScrollPos();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::EndUndoTransaction()
{
    m_bIgnoreNotifications = false;

    if (m_bNeedReload)
    {
        Reload();
        RestoreVerticalScrollPos(m_storedScrollPosition);
        m_bNeedReload = false;
    }

    UpdateDopeSheet();
}

//////////////////////////////////////////////////////////////////////////
QIcon CTrackViewNodesCtrl::GetIconForTrack(const CTrackViewTrack* pTrack)
{
    int r = GetIconIndexForTrack(pTrack);
    return m_imageList.contains(r) ? m_imageList[r] : QIcon();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnKeysChanged(CTrackViewSequence* pSequence)
{
    if (!m_bIgnoreNotifications && pSequence && pSequence == GetIEditor()->GetAnimation()->GetSequence())
    {
        UpdateDopeSheet();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnKeySelectionChanged(CTrackViewSequence* pSequence)
{
    OnKeysChanged(pSequence);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::OnNodeSelectionChanged(CTrackViewSequence* pSequence)
{
    if (m_bSelectionChanging)
    {
        return;
    }

    if (!m_bIgnoreNotifications && pSequence && pSequence == GetIEditor()->GetAnimation()->GetSequence())
    {
        UpdateDopeSheet();

        CTrackViewAnimNodeBundle animNodes = pSequence->GetAllAnimNodes();
        const uint numNodes = animNodes.GetCount();
        for (uint i = 0; i < numNodes; ++i)
        {
            CTrackViewAnimNode* pNode = animNodes.GetNode(i);
            if (pNode->IsSelected())
            {
                SelectRow(pNode, false, false);
            }
            else
            {
                DeselectRow(pNode);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::SelectRow(CTrackViewNode* pNode, const bool bEnsureVisible, const bool bDeselectOtherRows)
{
    std::unordered_map<const CTrackViewNode*, CRecord*>::const_iterator it = m_nodeToRecordMap.find(pNode);
    if (it != m_nodeToRecordMap.end())
    {
        if (bDeselectOtherRows)
        {
            ui->treeWidget->selectionModel()->clear();
        }
        (*it).second->setSelected(true);
        if (bEnsureVisible)
        {
            ui->treeWidget->scrollToItem((*it).second);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::DeselectRow(CTrackViewNode* pNode)
{
    std::unordered_map<const CTrackViewNode*, CRecord*>::const_iterator it = m_nodeToRecordMap.find(pNode);
    if (it != m_nodeToRecordMap.end())
    {
        (*it).second->setSelected(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewNodesCtrl::EraseNodeRecordRec(CTrackViewNode* pNode)
{
    m_nodeToRecordMap.erase(pNode);

    const unsigned int numChildren = pNode->GetChildCount();
    for (unsigned int i = 0; i < numChildren; ++i)
    {
        EraseNodeRecordRec(pNode->GetChild(i));
    }
}

#include <TrackView/TrackViewNodes.moc>
