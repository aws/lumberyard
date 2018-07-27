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

// Description : implementation file


#include "StdAfx.h"
#include "AnimationBrowser.h"
#include <I3DEngine.h>
#include <ICryAnimation.h>
#include <IVertexAnimation.h>
#include "ModelViewport.h"
#include "CryCharMorphParams.h"
#include "Material/MaterialManager.h"
#include "Clipboard.h"
#include "Util/UIEnumerations.h"
#include "Util/EditorUtils.h"
#include <IResourceSelectorHost.h>

#include "Util/AbstractGroupProxyModel.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QSortFilterProxyModel>
#include <QToolBar>
#include <QTreeView>
#include <QPainter>

static int MAXIMUM_HISTORY_SIZE = 50;

namespace CharacterEditor
{
    const uint32 kAnimationListIconCount = 6;
}

class AnimationBrowserModel
    : public QAbstractTableModel
    , public IAnimationStreamingListener
{
public:
    AnimationBrowserModel(QObject* parent = nullptr)
        : QAbstractTableModel(parent)
    {
        m_pInstance_SKEL = NULL;
        m_pInstance_SKIN = NULL;

        CreateBitmapToolbarDynamic(m_imageList);
    }

    // this implements the AnimationStreamingListener functionality
    void NotifyAnimLoaded(const int32 globalAnimId) override;
    void NotifyAnimUnloaded(const int32 globalAnimId) override;
    void NotifyAnimReloaded(const int32 globalAnimId) override;

    void SetModel_SKEL(ICharacterInstance* pSkelInstance)
    {
        beginResetModel();
        m_pInstance_SKEL = pSkelInstance;
        m_pInstance_SKIN = nullptr;
        endResetModel();
    }

    void SetModel_SKIN(IAttachmentSkin* pSkelInstance)
    {
        beginResetModel();
        m_pInstance_SKIN = pSkelInstance;
        m_pInstance_SKEL = nullptr;
        endResetModel();
    }

    QStringList mimeTypes() const override
    {
        return QStringList(EditorDragDropHelpers::GetAnimationNameClipboardFormat());
    }

    QMimeData* mimeData(const QModelIndexList& indexes) const override
    {
        QMimeData* mimeData = new QMimeData;
        if (!indexes.isEmpty())
        {
            QModelIndex index = indexes.first().sibling(indexes.first().row(), 0);
            mimeData->setData(EditorDragDropHelpers::GetAnimationNameClipboardFormat(), index.data().toString().toUtf8());
        }
        return mimeData;
    }

    Qt::DropActions supportedDragActions() const override
    {
        return Qt::MoveAction | Qt::LinkAction;
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        Qt::ItemFlags f = QAbstractTableModel::flags(index);
        if (index.isValid())
        {
            f |= Qt::ItemIsDragEnabled;
        }
        return f;
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        if (!parent.isValid())
        {
            if (m_pInstance_SKEL)
            {
                IAnimationSet* pAnimationSet = m_pInstance_SKEL->GetIAnimationSet();
                if (pAnimationSet == 0)
                {
                    return 0;
                }
                return pAnimationSet->GetAnimationCount();
            }
            else if (m_pInstance_SKIN)
            {
                ISkin* pISkin = m_pInstance_SKIN->GetISkin();
                const IVertexFrames* pVertextFrames = pISkin ? pISkin->GetVertexFrames() : nullptr;
                return pVertextFrames ? pVertextFrames->GetCount() : 0;
            }
        }
        return 0;
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 5;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (section >= columnCount() || orientation != Qt::Horizontal || role != Qt::DisplayRole)
        {
            return QAbstractItemModel::headerData(section, orientation, role);
        }

        switch (section)
        {
        case 0:
            return tr("Animation");
        case 1:
            return tr("Frame");
        case 2:
            return tr("Size");
        case 3:
            return tr("PosCtrls");
        case 4:
            return tr("RotCtrl");
        default:
            return QVariant();
        }
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() >= rowCount(index.parent()) || index.column() >= columnCount(index.parent()))
        {
            return QVariant();
        }

        if (m_pInstance_SKEL)
        {
            auto animationSet = m_pInstance_SKEL->GetIAnimationSet();
            assert(animationSet);

            if (role == Qt::DisplayRole)
            {
                switch (index.column())
                {
                case 0:
                    return QString::fromLatin1(animationSet->GetNameByAnimID(index.row()));
                case 1:
                    return 1 + static_cast<int>(animationSet->GetDuration_sec(index.row()) * 30 /*animationSamplingFrequencyHz*/ + 0.5f);
                case 2:
                    return animationSet->GetAnimationSize(index.row());
                case 3:
                    return animationSet->GetTotalPosKeys(index.row());
                case 4:
                    return animationSet->GetTotalRotKeys(index.row());
                default:
                    return QVariant();
                }
            }
            else if (role == Qt::ForegroundRole)
            {
                switch (index.column())
                {
                case 0:
                    return animationSet->IsAnimLoaded(index.row()) ? QVariant::fromValue<QBrush>(Qt::red) : QVariant();
                default:
                    return QVariant();
                }
            }
            else if (role == Qt::ToolTipRole)
            {
                return GenerateToolTips(index.row());
            }
            else if (role == Qt::TextAlignmentRole)
            {
                return index.column() == 0 ? Qt::AlignLeft : Qt::AlignCenter;
            }
            else if (role == Qt::DecorationRole)
            {
                return index.column() == 0 ? GetAnimIcon(index.row()) : QVariant();
            }
            else
            {
                return QVariant();
            }
        }
        else if (m_pInstance_SKIN)
        {
            auto pSkin = m_pInstance_SKIN->GetISkin();
            assert(pSkin);
            const IVertexFrames* pVertexFrames = pSkin->GetVertexFrames();
            assert(pVertexFrames);

            if (role == Qt::DisplayRole)
            {
                switch (index.column())
                {
                case 0:
                    return QString::fromLatin1(pVertexFrames->GetNameByIndex(index.row()));
                case 1:
                    return 1 + static_cast<int>(1.5f); // original: (1.0 / 30 * 30 + 0.5)
                case 2:
                    return 0;
                case 3:
                    return 0;
                case 4:
                    return 0;
                default:
                    return QVariant();
                }
            }
            else if (role == Qt::DisplayRole)
            {
                return index.column() < 2 ? QVariant() : QVariant::fromValue<QBrush>(Qt::gray);
            }
            else if (role == Qt::TextAlignmentRole)
            {
                return index.column() == 0 ? Qt::AlignLeft : Qt::AlignCenter;
            }
        }
        return {};
    }

    QString GenerateToolTips(int row) const;
    QPixmap GetAnimIcon(int row) const;

    bool LoadAndCombineTrueColorImages(QVector<QPixmap>& imageList, const QStringList& nIDResources, int nIconWidth, int nIconHeight);
    bool CreateBitmapToolbarDynamic(QVector<QPixmap>& imageList);

private:
    QVector<QPixmap> m_imageList;
    _smart_ptr< ICharacterInstance > m_pInstance_SKEL;
    _smart_ptr< IAttachmentSkin > m_pInstance_SKIN;
};

class AnimationBrowserTreeModel
    : public AbstractGroupProxyModel
{
public:
    AnimationBrowserTreeModel(QObject* parent = nullptr)
        : AbstractGroupProxyModel(parent)
    {
    }

protected:
    QStringList GroupForSourceIndex(const QModelIndex& sourceIndex) const override
    {
        const QStringList animationName = sourceIndex.data().toString().split(QLatin1String("_"));
        return animationName.size() == 1 ? QStringList() : QStringList(animationName.first());
    }
};

CAnimationBrowser::CAnimationBrowser(QWidget* parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
    bSelMotionFromHistory = false;

    m_pInstance_SKEL = NULL;
    m_pInstance_SKIN = NULL;
    m_builtRecordsCount = 0;

    m_pCharacterAnimLastUpdated = 0;
    m_pAttachmentSkinMorphLastUpdated = 0;

    GetIEditor()->RegisterNotifyListener(this);

    OnInitDialog();

    auto comboBoxActivated = static_cast<void(QComboBox::*)(int)>(&QComboBox::activated);

    connect(m_wndReport, &QTreeView::customContextMenuRequested, this, &CAnimationBrowser::OnReportItemRClick);
    connect(m_wndReport->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CAnimationBrowser::OnReportSelChange);
    connect(m_wndReport, &QTreeView::doubleClicked, this, &CAnimationBrowser::OnReportItemDblClick);
    connect(m_editFilterText, &QLineEdit::textChanged, this, &CAnimationBrowser::OnFilterText);
    //
    connect(m_comboSelectCharacter, comboBoxActivated, this, &CAnimationBrowser::OnSelectCharacters);
    connect(m_comboHistory, comboBoxActivated, this, &CAnimationBrowser::OnSelectMotionFromHistory);
}

CAnimationBrowser::~CAnimationBrowser()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

void CAnimationBrowser::OnSelectMotionFromHistory()
{
    bSelMotionFromHistory = true;
    QStringList anims;

    if (GetSelectedAnimations(anims))
    {
        PlaySelectedAnimations(anims);
    }
}



enum
{
    AB_Icon_Type_Lmg,
    AB_Icon_Type_Aim,
    AB_Icon_Type_Normal,

    AB_IconCount_Type
};

enum
{
    AB_Icon_Loop_Looping,
    AB_Icon_Loop_NonLooping,

    AB_IconCount_Loop
};

enum
{
    AB_Icon_Feature_Additive,
    AB_Icon_Feature_Override,

    AB_IconCount_Feature
};

enum
{
    AB_Icon_Demand_OnDemand,
    AB_Icon_Demand_Default,

    AB_IconCount_OnDemand
};

enum
{
    AB_Icon_Location_InPak,
    AB_Icon_Location_OnDisk,

    AB_IconCount_Location
};

enum
{
    AB_Icon_Event_Default,
    AB_Icon_Event_Sound,

    AB_IconCount_AnimEvent
};

bool AnimationBrowserModel::CreateBitmapToolbarDynamic(QVector<QPixmap>& imageList)
{
    imageList.clear();

    QMap<int, QString> iType;
    iType[AB_Icon_Type_Lmg] = QLatin1String("animatio.png");
    iType[AB_Icon_Type_Aim] = QLatin1String("bmp00024.png");
    iType[AB_Icon_Type_Normal] = QLatin1String("bmp00003.png");

    QMap<int, QString> iLoop;
    iLoop[AB_Icon_Loop_Looping] = QLatin1String("bmp00026.png");
    iLoop[AB_Icon_Loop_NonLooping] = QLatin1String("bmp00028.png");

    QMap<int, QString> iFeature;
    iFeature[AB_Icon_Feature_Additive] = QLatin1String("bmp00029.png");
    iFeature[AB_Icon_Feature_Override] = QLatin1String("bmp00030.png");

    QMap<int, QString> iOnDemand;
    iOnDemand[AB_Icon_Demand_OnDemand] = QLatin1String("bmp00031.png");
    iOnDemand[AB_Icon_Demand_Default] = QLatin1String("bmp00002.png");

    QMap<int, QString> iLocation;
    iLocation[AB_Icon_Location_InPak] = QLatin1String("animflag_inside_pak.png");
    iLocation[AB_Icon_Location_OnDisk] = QLatin1String("animflag_on_disk.png");

    QMap<int, QString> iAnimEvents;
    iAnimEvents[AB_Icon_Event_Default] = QLatin1String("bmp00002.png");
    iAnimEvents[AB_Icon_Event_Sound] = QLatin1String("animations_tree_soundevent.png");

    QStringList iconIds;

    // add empty icon as the first icon, for when feature icon is not set
    for (uint32 i = 0; i < CharacterEditor::kAnimationListIconCount; ++i)
    {
        iconIds.push_back(QLatin1String("bitmap5.png"));
    }

    // create the linear vector with magic indices, combinatorial
    for (int32 i = 0; i < AB_IconCount_Type; ++i)
    {
        for (int32 j = 0; j < AB_IconCount_Loop; ++j)
        {
            for (int32 m = 0; m < AB_IconCount_Feature; ++m)
            {
                for (int32 n = 0; n < AB_IconCount_OnDemand; ++n)
                {
                    for (int32 o = 0; o < AB_IconCount_Location; ++o)
                    {
                        for (int32 p = 0; p < AB_IconCount_AnimEvent; ++p)
                        {
                            iconIds.push_back(iType[i]);
                            iconIds.push_back(iLoop[j]);
                            iconIds.push_back(iFeature[m]);
                            iconIds.push_back(iOnDemand[n]);
                            iconIds.push_back(iLocation[o]);
                            iconIds.push_back(iAnimEvents[p]);
                        }
                    }
                }
            }
        }
    }

    LoadAndCombineTrueColorImages(imageList, iconIds, 16, 16);
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::OnInitDialog()
{
    // Create filter Toolbar.
    {
        m_wndToolBar = addToolBar(QString());
        m_wndToolBar->setFloatable(false);

        m_wndToolBar->addAction(QPixmap(QLatin1String(":/ce_animations_toolbar_00.png")), QString(), this, &CAnimationBrowser::ReloadAnimations);
        m_wndToolBar->addSeparator();

        // Filter label
        m_wndToolBar->addWidget(new QLabel(tr("Filter")));

        // Filter control
        m_editFilterText = new QLineEdit;
        m_wndToolBar->addWidget(m_editFilterText);
        m_editFilterText->setFixedWidth(80);

        m_wndToolBar->addSeparator();

        //Add "choose character" ComboBox
        m_comboSelectCharacter = new QComboBox;
        m_wndToolBar->addWidget(m_comboSelectCharacter);
        m_comboSelectCharacter->setFixedWidth(100);

        // "motion history" label
        m_wndToolBar->addWidget(new QLabel(tr("History")));

        //Add "motion history" ComboBox
        m_comboHistory = new QComboBox;
        m_wndToolBar->addWidget(m_comboHistory);
        m_comboHistory->setFixedWidth(230);
    }

    m_animationModel = new AnimationBrowserModel(this);
    m_animationFilterModel = new QSortFilterProxyModel(this);
    m_animationTreeModel = new AnimationBrowserTreeModel(this);

    m_animationFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_animationFilterModel->setSourceModel(m_animationModel);
    m_animationTreeModel->setSourceModel(m_animationFilterModel);

    m_wndReport = new QTreeView;
    m_wndReport->setModel(m_animationTreeModel);
    m_wndReport->setContextMenuPolicy(Qt::CustomContextMenu);
    m_wndReport->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_wndReport->setDragEnabled(true);
    m_wndReport->setSortingEnabled(true);
    m_wndReport->sortByColumn(0, Qt::AscendingOrder);
    m_wndReport->header()->resizeSection(0, 300);
    m_wndReport->header()->resizeSection(1, 50);
    m_wndReport->header()->resizeSection(2, 50);
    m_wndReport->header()->resizeSection(3, 50);
    m_wndReport->header()->resizeSection(4, 50);

    setCentralWidget(m_wndReport);
}

//------------------------------------------------------------------------------
// Combine several bitmap to make a bigger bitmap
// Assume all the icons has the same sizes
bool AnimationBrowserModel::LoadAndCombineTrueColorImages(QVector<QPixmap>& imageList, const QStringList& nIDResources, int nIconWidth, int nIconHeight)
{
    int32 count = (UINT)nIDResources.size();
    int32 finalWidth = nIconWidth * CharacterEditor::kAnimationListIconCount;
    int32 finalHeight = nIconHeight;

    imageList.clear();

    //assert(count % CharacterEditor::kAnimationListIconCount == 0);

    for (int32 i = 0; i < count; i += CharacterEditor::kAnimationListIconCount)
    {
        QPixmap finalBitmap(finalWidth, finalHeight);
        finalBitmap.fill(Qt::transparent);

        for (int32 j = 0; j < CharacterEditor::kAnimationListIconCount; ++j)
        {
            QPixmap bitmap(QStringLiteral(":/%1").arg(nIDResources[i + j]));
            assert(bitmap.width() == nIconWidth && bitmap.height() == nIconHeight);

            // Add current image data to the final one
            QPainter painter(&finalBitmap);
            const QRect r(j * nIconWidth, 0, nIconWidth, nIconHeight);
            painter.drawPixmap(r, bitmap);
        }

        imageList.push_back(finalBitmap);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
QPixmap AnimationBrowserModel::GetAnimIcon(int nAnimId) const
{
    if (!m_pInstance_SKEL)
    {
        return QPixmap();
    }

    auto pAnimations = m_pInstance_SKEL->GetIAnimationSet();
    if (!pAnimations)
    {
        return QPixmap();
    }


    int32 iconCount[CharacterEditor::kAnimationListIconCount] = { AB_IconCount_Type, AB_IconCount_Loop, AB_IconCount_Feature, AB_IconCount_OnDemand, AB_IconCount_Location, AB_IconCount_AnimEvent };
    int32 typeSize[CharacterEditor::kAnimationListIconCount];

    typeSize[ CharacterEditor::kAnimationListIconCount - 1 ] = iconCount[ CharacterEditor::kAnimationListIconCount - 1 ];
    for (int32 i = CharacterEditor::kAnimationListIconCount - 2; 0 <= i; --i)
    {
        typeSize[ i ] = typeSize[ i + 1 ] * iconCount[ i ];
    }

    int32 typeId[CharacterEditor::kAnimationListIconCount] = {-1, -1, -1, -1, -1, -1};


    const char* name = pAnimations->GetNameByAnimID(nAnimId);
    if (name[0] == '#')
    {
        return QPixmap();
    }

    uint32 flags = pAnimations->GetAnimationFlags(nAnimId);

    uint32 loaded  = flags & CA_ASSET_LOADED;
    uint32 created = flags & CA_ASSET_CREATED;

    uint32 xlmg      = flags & CA_ASSET_LMG;
    uint32 xlmgvalid = flags & CA_ASSET_LMG_VALID;
    if (xlmg && xlmgvalid == 0)
    {
        created = xlmgvalid;
    }

    if (created == 0)
    {
        return QPixmap(); // motion missing
    }
    uint32 aimPose = flags & CA_AIMPOSE;

    if (xlmg && created)
    {
        typeId[0] = AB_Icon_Type_Lmg;
    }
    else if (aimPose)
    {
        typeId[0] = AB_Icon_Type_Aim;
    }
    else
    {
        typeId[0] = AB_Icon_Type_Normal;
    }

    if (flags & CA_ASSET_CYCLE)
    {
        typeId[1] = AB_Icon_Loop_Looping;
    }
    else
    {
        typeId[1] = AB_Icon_Loop_NonLooping;
    }

    if (flags & CA_ASSET_ADDITIVE)
    {
        typeId[2] = AB_Icon_Feature_Additive;
    }
    else
    {
        typeId[2] = AB_Icon_Feature_Override;
    }

    if (flags & CA_ASSET_ONDEMAND)
    {
        typeId[3] = AB_Icon_Demand_OnDemand;
    }
    else
    {
        typeId[3] = AB_Icon_Demand_Default;
    }

    bool bWasLoadedFromPak = false;

    const char* pPath = pAnimations->GetFilePathByID(nAnimId);
    bool bIsInsidePak = gEnv->pCryPak->IsFileExist(pPath, ICryPak::eFileLocation_InPak);
    bool bIsOnDisk = gEnv->pCryPak->IsFileExist(pPath, ICryPak::eFileLocation_OnDisk);

    const char* pFilePathDBA = pAnimations->GetDBAFilePath(nAnimId);
    if (pFilePathDBA)
    {
        //if an asset is in a DBA, then we treat it like in a PAK, because we cant hot load it anyway.
        bIsInsidePak = 1;
        bIsOnDisk = 0;
    }

    bool bLoadingFromPakFirst = (gEnv->pCryPak->GetPakPriority() == 1);
    bool bLoadingFromPakOnly = (gEnv->pCryPak->GetPakPriority() == 2);

    if (bLoadingFromPakFirst)
    {
        bWasLoadedFromPak = bIsInsidePak ? true : !bIsOnDisk;
    }
    else
    if (bLoadingFromPakOnly)
    {
        bWasLoadedFromPak = bIsInsidePak;
    }
    else
    {
        bWasLoadedFromPak = bIsOnDisk ? false : bIsInsidePak;
    }

    typeId[4] = (bWasLoadedFromPak ? AB_Icon_Location_InPak : AB_Icon_Location_OnDisk);


    bool hasSoundAnimEvents = false;
    assert(gEnv->pCharacterManager);
    IAnimEvents* pAnimEventsManager = gEnv->pCharacterManager->GetIAnimEvents();
    assert(pAnimEventsManager);
    const char* animFilePath = pAnimations->GetFilePathByID(nAnimId);
    IAnimEventList* pAnimEventList = pAnimEventsManager->GetAnimEventList(animFilePath);
    if (pAnimEventList)
    {
        const uint32 animEventCount = pAnimEventList->GetCount();
        for (uint32 i = 0; i < animEventCount && !hasSoundAnimEvents; ++i)
        {
            const CAnimEventData& animEventData = pAnimEventList->GetByIndex(i);
            const char* eventName = animEventData.GetName();
            if (azstricmp(eventName, "sound") == 0 ||
                azstricmp(eventName, "footstep") == 0 ||
                azstricmp(eventName, "foley") == 0 ||
                strstr(eventName, "asfx_") != NULL)
            {
                hasSoundAnimEvents = true;
            }
        }
    }



    typeId[5] = hasSoundAnimEvents ? AB_Icon_Event_Sound : AB_Icon_Event_Default;


    int32 iconId = typeId[0] * typeSize[1]
        + typeId[1] * typeSize[2]
        + typeId[2] * typeSize[3]
        + typeId[3] * typeSize[4]
        + typeId[4] * typeSize[5]
        + typeId[5];


    return m_imageList.at(iconId + 1);     // skip the first 64x16 "missing icon"
}

//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::ReloadAnimations()
{
}

void CAnimationBrowser::UpdateCharacterComboBoxSelection(const int ind)
{
    //------------------------------------------------------------------------------
    // Set combobox content
    m_comboSelectCharacter->setCurrentIndex(ind);
}

void CAnimationBrowser::UpdateCharacterComboBox(const std::vector<const char*>& attachmentNames, const std::vector<int>& posInAttachBrowser)
{
    if (attachmentNames.size() != posInAttachBrowser.size())
    {
        return;
    }

    //------------------------------------------------------------------------------
    // Set combobox content
    m_comboSelectCharacter->clear();
    m_comboSelectCharacter->addItem(tr("Base Character"));

    for (int i = 0; i < (int)attachmentNames.size(); ++i)
    {
        m_comboSelectCharacter->addItem(attachmentNames[i]);
    }

    //------------------------------------------------------------------------------
    // Set name id maps

    SAttachmentIDs id;
    int indCombo = 1;
    for (int i = 0; i < (int)attachmentNames.size(); ++i)
    {
        id.iIndexAttachBrowser = posInAttachBrowser[i];
        id.iIndexComboBox = indCombo;
        ++indCombo;
        m_attachmentIDs.push_back(id);
    }

    m_comboSelectCharacter->setCurrentIndex(0);
}

//////////////////////////////////////////////////////////////////////////
bool CAnimationBrowser::GetSelectedAnimations(TDSelectedAnimations& anims)
{
    anims.clear();

    if (!bSelMotionFromHistory)
    {
        for (const QModelIndex& index : m_wndReport->selectionModel()->selectedRows())
        {
            const QString name = index.data().toString();
            if (m_wndReport->model()->rowCount(index) == 0 && !name.isEmpty())
            {
                anims.push_back(name);
            }
        }
    }
    else
    {
        //------------------------------------------------------------------------------
        //  Unselect all the motions in motion browser
        m_wndReport->selectionModel()->clearSelection();

        //------------------------------------------------------------------------------
        //
        const QString animName = m_comboHistory->currentText();
        if (!animName.isEmpty())
        {
            anims.push_back(animName);
        }

        m_comboHistory->setToolTip(animName);
    }

    return !anims.empty();
}


void CAnimationBrowser::OnReportSelChange()
{
    bSelMotionFromHistory = false;

    TDSelectedAnimations anims;

    GetSelectedAnimations(anims);

    if (anims.empty())
    {
        return;
    }

    QString combinedString = anims.join(QLatin1String(","));
    GetIEditor()->GetResourceSelectorHost()->SetGlobalSelection("animation", combinedString.toUtf8().data());

    UpdateAnimationHistory(anims);
}

//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::OnReportItemDblClick()
{
    QStringList anims;
    GetSelectedAnimations(anims);
    PlaySelectedAnimations(anims);
    for (size_t i = 0; i < m_onDblClickCallbacks.size(); ++i)
    {
        m_onDblClickCallbacks[i]();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::PlaySelectedAnimations(QStringList& anims)
{
}

//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::OnFilterText()
{
    m_filterText = m_editFilterText->text();
}

//////////////////////////////////////////////////////////////////////////
void CAnimationBrowser::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (event == eNotify_OnIdleUpdate)
    {
        if (m_filterTextPrev != m_filterText)
        {
            m_filterTextPrev = m_filterText;
            m_animationFilterModel->setFilterFixedString(m_filterText);
        }
    }
}

void CAnimationBrowser::OnSelectCharacters()
{
}


QString AnimationBrowserModel::GenerateToolTips(int row) const
{
    if (!m_pInstance_SKEL)
    {
        return QString();
    }

    auto pAnimations = m_pInstance_SKEL->GetIAnimationSet();
    QString text;

    const char* name = pAnimations->GetNameByAnimID(row);
    const char* pFilePath = pAnimations->GetFilePathByName(name);
    if (pFilePath)
    {
        if (name[0] == '#')
        {
            text = tr("%1\n MorphTarget").arg(name);
        }
        else
        {
            uint32 flags = pAnimations->GetAnimationFlags(row);
            bool aimpose = (flags & CA_AIMPOSE) != 0;
            bool lmg = (flags & CA_ASSET_LMG) != 0;
            bool lmgvalid = (flags & CA_ASSET_LMG_VALID) != 0;
            bool created = (flags & CA_ASSET_CREATED) != 0;

            if (lmg)
            {
                if (!created)
                {
                    const char* pStatusTXT = pAnimations->GetAnimationStatus(row);
                    text = tr("Parametric Group not created because of XML-Syntax Error\n %1").arg(pStatusTXT);
                }
                else if (!lmgvalid)
                {
                    const char* pStatusTXT = pAnimations->GetAnimationStatus(row);
                    text = pStatusTXT;
                }
                else
                {
                    text = tr("This is a valid Parametric Group");
                }
            }
            else
            {
                if (!created)
                {
                    text = tr("File does not exist");
                }
                else if (aimpose)
                {
                    text = tr("This is an Aim-Pose");
                }
                else
                {
                    text = tr("This is an CAF-File");
                }

                text += tr("\n FilePath: %1").arg(pFilePath);

                const char* pFilePathDBA = pAnimations->GetDBAFilePath(row);
                if (pFilePathDBA)
                {
                    text += tr("\n FilePath: %1").arg(pFilePathDBA);
                }
            }
        }
    }

    return text;
}

void CAnimationBrowser::SetModel_SKEL(ICharacterInstance* pSkelInstance)
{
    m_pInstance_SKEL = pSkelInstance;
    if (m_pCharacterAnimLastUpdated == pSkelInstance)
    {
        return;
    }
    m_pCharacterAnimLastUpdated = pSkelInstance;

    if (m_pInstance_SKIN != nullptr)
    {
        if (IVertexAnimation* pVertexAnimation = m_pInstance_SKIN->GetIVertexAnimation())
        {
            pVertexAnimation->ReleaseAllFrameWeightsManual();
        }
    }

    m_pInstance_SKIN = 0;
    m_pAttachmentSkinMorphLastUpdated = 0;
    if (pSkelInstance == 0)
    {
        return;
    }

    m_animationModel->SetModel_SKEL(m_pInstance_SKEL);

    if (m_comboSelectCharacter->currentIndex() <= 0)
    {
        m_comboSelectCharacter->setCurrentIndex(0);
    }
}

void CAnimationBrowser::SetModel_SKIN(IAttachmentSkin* pAttachmentSkin)
{
    m_pInstance_SKIN = pAttachmentSkin;
    if (m_pAttachmentSkinMorphLastUpdated == pAttachmentSkin)
    {
        return;
    }
    m_pAttachmentSkinMorphLastUpdated = pAttachmentSkin;
    m_pInstance_SKEL = 0;
    m_pCharacterAnimLastUpdated = 0;
    if (pAttachmentSkin == 0)
    {
        return;
    }

    m_animationModel->SetModel_SKIN(m_pInstance_SKIN);

    if (m_comboSelectCharacter->currentIndex() <= 0)
    {
        m_comboSelectCharacter->setCurrentIndex(0);
    }
}

void CAnimationBrowser::ExportCAF2HTR(const string name)
{
    const QString path = QFileDialog::getSaveFileName(this, QString(), QString(), tr("HTR (*.htr)"));

    if (path.isEmpty())
    {
        return;
    }

    string dirName = PathUtil::GetParentDirectory(path.toUtf8().data()) + string("\\");

    ISkeletonAnim* pISkeletonAnim = m_pInstance_SKEL->GetISkeletonAnim();
    pISkeletonAnim->ExportHTRAndICAF(name.c_str(), dirName.c_str());
}

void CAnimationBrowser::ExportVGrid(const string name)
{
    ISkeletonAnim* pISkeletonAnim = m_pInstance_SKEL->GetISkeletonAnim();
    pISkeletonAnim->ExportVGrid(name.c_str());
}

void CAnimationBrowser::RegenerateAimGrid(const string name)
{
    assert(m_pInstance_SKEL);

    IAnimationSet* pAnimations = m_pInstance_SKEL->GetIAnimationSet();
    assert(pAnimations);

    ISkeletonAnim* pSkeletonAnim = m_pInstance_SKEL->GetISkeletonAnim();
    assert(pSkeletonAnim);

    std::vector<string> currentAnimations;
    for (uint32 i = 0; i < ISkeletonAnim::LayerCount; ++i)
    {
        string animationName = "";
        const uint32 animsInLayerCount = pSkeletonAnim->GetNumAnimsInFIFO(i);
        if (0 < animsInLayerCount)
        {
            const CAnimation& animation = pSkeletonAnim->GetAnimFromFIFO(i, animsInLayerCount - 1);
            const int16 animationId = animation.GetAnimationId();
            animationName = pAnimations->GetNameByAnimID(animationId);
        }
        currentAnimations.push_back(animationName);
    }

    m_pInstance_SKEL->ReloadCHRPARAMS();
    pAnimations->RebuildAimHeader(name.c_str(), &m_pInstance_SKEL->GetIDefaultSkeleton());

    assert(currentAnimations.size() == ISkeletonAnim::LayerCount);
    for (uint32 i = 0; i < ISkeletonAnim::LayerCount; ++i)
    {
        const string& animationName = currentAnimations[i];
        const bool hasAnimationInLayer = !animationName.empty();
        if (hasAnimationInLayer)
        {
            CryCharAnimationParams params;
            params.m_nFlags |= CA_LOOP_ANIMATION;
            params.m_nLayerID = i;
            params.m_fTransTime = 0.f;
            pSkeletonAnim->StartAnimation(animationName.c_str(), params);
        }
    }
}

void CAnimationBrowser::OnReportItemRClick(const QPoint& pos)
{
    QMenu menu;

    const QModelIndexList rows = m_wndReport->selectionModel()->selectedRows();

    if (rows.isEmpty()) // No rows selected
    {
        return;
    }

    string name = m_wndReport->currentIndex().data().toString().toUtf8().data();

    // create main menu items
    menu.addAction(tr("Copy"), [&](){ CClipboard(this).PutString(name.c_str()); });
    menu.addAction(tr("Export to HTR"), [&](){ ExportCAF2HTR(name); });

    if (!m_pInstance_SKEL)
    {
        return;
    }

    IAnimationSet* pAnimations = m_pInstance_SKEL->GetIAnimationSet();
    if (!pAnimations)
    {
        return;
    }

    uint32 nAnimId = pAnimations->GetAnimIDByName(name);

    if (pAnimations->GetAnimationFlags(nAnimId) & CA_ASSET_LMG_VALID)
    {
        menu.addAction(tr("Save VGrid"), [&](){ ExportVGrid(name); });
    }

    if (pAnimations->GetAnimationFlags(nAnimId) & CA_AIMPOSE)
    {
        menu.addAction(tr("Reload for testing"), [&](){ RegenerateAimGrid(name); });
    }

    // track menu
    menu.exec(m_wndReport->mapToGlobal(pos));
}

void AnimationBrowserModel::NotifyAnimUnloaded(const int globalAnimId)
{
    const QModelIndex topLeft = index(globalAnimId, 0);
    const QModelIndex bottomRight = index(globalAnimId, columnCount() - 1);
    emit dataChanged(topLeft, bottomRight);
}

void AnimationBrowserModel::NotifyAnimLoaded(const int globalAnimId)
{
    const QModelIndex topLeft = index(globalAnimId, 0);
    const QModelIndex bottomRight = index(globalAnimId, columnCount() - 1);
    emit dataChanged(topLeft, bottomRight);
}

void AnimationBrowserModel::NotifyAnimReloaded(const int globalAnimId)
{
    const QModelIndex topLeft = index(globalAnimId, 0);
    const QModelIndex bottomRight = index(globalAnimId, columnCount() - 1);
    emit dataChanged(topLeft, bottomRight);
}


void CAnimationBrowser::UpdateAnimationHistory(const TDSelectedAnimations& anims)
{
    //------------------------------------------------------------------------------
    // if it's full, remove anims.size() items from the box
    if (m_comboHistory->count() >= MAXIMUM_HISTORY_SIZE)
    {
        for (int i = 0; i < (int)anims.size(); ++i)
        {
            m_comboHistory->removeItem(m_comboHistory->count() - 1);
        }
    }

    //------------------------------------------------------------------------------
    // Add new history at the beginning
    for (int i = 0; i < (int)anims.size(); ++i)
    {
        if (m_comboHistory->findText(anims[i]) == -1)
        {
            m_comboHistory->insertItem(0, anims[i]);
        }
    }

    m_comboHistory->setCurrentIndex(0);

    m_comboHistory->setToolTip(m_comboHistory->currentText());
}

#include <CharacterEditor/AnimationBrowser.moc>