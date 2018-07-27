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

// Description : A report control to edit items


#include "StdAfx.h"
#include "../Include/IResourceSelectorHost.h"
#include "Objects/EntityObject.h"
#include "GenericSelectItemDialog.h"
#include "DialogScriptView.h"
#include "Editor/Objects/EntityObject.h"
#include "DialogScriptView.h"
#include "DialogManager.h"
#include "DialogEditorDialog.h"
#include "ComboBoxItemDelegate.h"

#include <ICryAnimation.h>
#include <IFacialAnimation.h>
#include <IGameFramework.h>
#include <ICryMannequin.h>

#include <QtUtil.h>
#include <QtUtilWin.h>

#include <StringUtils.h>
#include <QMenu>
#include <QHeaderView>
#include <QEvent>
#include <QContextMenuEvent>
#include <QItemSelectionModel>
#include <QDebug>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QString>
#include <QApplication>

static QLatin1String s_resetString("#RESET#");

static QStringList columnNames()
{
    static const QStringList names = {
        "Line", "Actor", "AudioID", "Animation", "Type", "EP", "Sync",
        "FacialExpr.", "Weight", "Fade", "LookAt", "Sticky", "Delay", "Description"
    };

    Q_ASSERT(names.size() == DialogScriptModel::NumberOfColumns);
    return names;
}

LineEditWithDelegate::LineEditWithDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* LineEditWithDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
    QPushButton* button = new QPushButton("...", editor);
    const int length = 20;
    button->setGeometry(option.rect.width() - length, 0, length, option.rect.height());
    const int row = index.row();
    connect(button, &QPushButton::clicked, this, [this, row]() { const_cast<LineEditWithDelegate*>(this)->buttonClicked(row); });
    return editor;
}

Audio::TAudioControlID DialogScriptView::ms_currentPlayLine = INVALID_AUDIO_CONTROL_ID;

DialogScriptView::DialogScriptView(QWidget* parent)
    : QTableView(parent)
    , m_headerPopup(new QMenu(this))
    , m_editor(nullptr)
    , m_label(new QLabel(this))
    , m_model(nullptr)
    , m_allowEdit(true)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_label);
    layout->setMargin(horizontalHeader()->height());
    m_label->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    m_label->setAttribute(Qt::WA_TransparentForMouseEvents);

    setSelectionMode(QAbstractItemView::SingleSelection);
    int i = 0;
    connect(m_headerPopup, &QMenu::triggered, this, &DialogScriptView::ToggleColumnVisibility);
    connect(this, &DialogScriptView::currentColumnChanged, this, &DialogScriptView::OnSelectedColumnChanged);
    horizontalHeader()->installEventFilter(this);
    setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    horizontalHeader()->setDragEnabled(true);
    horizontalHeader()->setSectionsMovable(true);

    verticalHeader()->sectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setDefaultSectionSize(25);

    Audio::AudioSystemRequestBus::BroadcastResult(m_pIAudioProxy, &Audio::AudioSystemRequestBus::Events::GetFreeAudioProxy);
    if (m_pIAudioProxy)
    {
        m_pIAudioProxy->Initialize("Dialog Lines trigger preview");
        m_pIAudioProxy->SetObstructionCalcType(Audio::eAOOCT_IGNORE);
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::AddRequestListener,
            &DialogScriptView::OnAudioTriggerFinished,
            m_pIAudioProxy,
            Audio::eART_AUDIO_CALLBACK_MANAGER_REQUEST,
            Audio::eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE);
    }
    UpdateNoItemText();
    qRegisterMetaType<QVector<int> >("QVector<int>");
    qRegisterMetaTypeStreamOperators<QVector<int> >("QVector<int>");
}

DialogScriptView::~DialogScriptView()
{
    StopSound();

    if (m_pIAudioProxy)
    {
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::RemoveRequestListener,
            &DialogScriptView::OnAudioTriggerFinished, m_pIAudioProxy);
        m_pIAudioProxy->Release();
    }
}

bool DialogScriptView::CanAddRow() const
{
    return IsAllowEdit();
}

bool DialogScriptView::CanDeleteRow() const
{
    return IsAllowEdit() && m_model->rowCount() > 0 && !selectedIndexes().isEmpty();
}

void DialogScriptView::SetDialogEditor(CDialogEditorDialog* editor)
{
    m_editor = editor;
    emit CanDeleteRowChanged();
    emit CanAddRowChanged();
}

void DialogScriptView::OnAudioTriggerFinished(Audio::SAudioRequestInfo const* const pAudioRequestInfo)
{
    //Remark: called from audio thread
    if (ms_currentPlayLine == pAudioRequestInfo->nAudioControlID)  //the currently playing line has finished
    {
        ms_currentPlayLine = INVALID_AUDIO_CONTROL_ID;
    }
}

bool DialogScriptView::IsAllowEdit() const
{
    return m_model && m_model->GetScript() && m_allowEdit;
}

void DialogScriptView::AllowEdit(bool allow)
{
    m_allowEdit = allow;
}

void DialogScriptView::mouseDoubleClickEvent(QMouseEvent* ev)
{
    QModelIndex index = indexAt(ev->pos());
    if (!index.isValid()) // We didn't click on any row, so add a line
    {
        AddNewRow(true);
    }
    else
    {
        if (index.column() == DialogScriptModel::LineColumn)
        {
            PlayLine(index.row());
        }
        else
        {
            QTableView::mouseDoubleClickEvent(ev);
        }
    }
}

void DialogScriptView::mousePressEvent(QMouseEvent* ev)
{
    QModelIndex index = indexAt(ev->pos());
    if (index.isValid() && index.column() == DialogScriptModel::LineColumn)
    {
        QItemSelectionModel* selModel = selectionModel();
        selModel->setCurrentIndex(index, QItemSelectionModel::Clear);
        selModel->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
    else
    {
        QTableView::mousePressEvent(ev);
        edit(index);
    }
}

void DialogScriptView::OnSelectedColumnChanged()
{
    emit helpTextChanged(GetCurrentHelpText());
}

void DialogScriptView::ToggleColumnVisibility(QAction* action)
{
    if (action->isChecked())
    {
        showColumn(action->data().toInt());
    }
    else
    {
        hideColumn(action->data().toInt());
    }

    SaveColumnsVisibility();
}

void DialogScriptView::SetScript(CEditorDialogScript* script)
{
    StopSound();
    m_model->SetScript(script);
    OnSelectedColumnChanged();
    SetModified(false);
    emit CanDeleteRowChanged();
    emit CanAddRowChanged();
}

void DialogScriptView::setModel(QAbstractItemModel* model)
{
    m_model = qobject_cast<DialogScriptModel*>(model);
    QTableView::setModel(model);
    InitializeColumnWidths();
    CreateHeaderPopup();

    setItemDelegateForColumn(DialogScriptModel::ActorColumn, new ActorComboBoxItemDelegate(this));
    setItemDelegateForColumn(DialogScriptModel::TypeColumn, new TypeComboBoxItemDelegate(this));
    setItemDelegateForColumn(DialogScriptModel::LookAtColumn, new LookAtComboBoxItemDelegate(this));

    auto audioIdDelegate = new LineEditWithDelegate(this);
    connect(audioIdDelegate, &LineEditWithDelegate::buttonClicked, this, &DialogScriptView::OnBrowseAudioTrigger);
    auto animationDelegate = new LineEditWithDelegate(this);
    connect(animationDelegate, &LineEditWithDelegate::buttonClicked, this, &DialogScriptView::OnBrowseAG);
    auto facialExprDelegate = new LineEditWithDelegate(this);
    connect(facialExprDelegate, &LineEditWithDelegate::buttonClicked, this, &DialogScriptView::OnBrowseFacial);

    setItemDelegateForColumn(DialogScriptModel::AudioIDColumn, audioIdDelegate);
    setItemDelegateForColumn(DialogScriptModel::FacialExprColumn, facialExprDelegate);
    setItemDelegateForColumn(DialogScriptModel::AnimationColumn, animationDelegate);

    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &DialogScriptView::currentColumnChanged, Qt::UniqueConnection);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &DialogScriptView::CanDeleteRowChanged, Qt::UniqueConnection);
    connect(m_model, &DialogScriptModel::modelModified, this, [this]() { this->SetModified(true); });
    connect(m_model, &DialogScriptModel::modelModified, this, &DialogScriptView::UpdateNoItemText);
    connect(m_model, &DialogScriptModel::modelModified, this, &DialogScriptView::CanDeleteRowChanged);

    connect(m_model, &QAbstractItemModel::modelReset, this, &DialogScriptView::UpdateNoItemText);
    connect(m_model, &QAbstractItemModel::modelReset, this, &DialogScriptView::CanDeleteRowChanged);

    UpdateNoItemText();
    emit CanDeleteRowChanged();
    emit CanAddRowChanged();
}

void DialogScriptView::CreateHeaderPopup()
{
    m_settings.beginGroup("DialogEditor");
    QVector<int> hiddenColumns = m_settings.value("hiddenColumns").value<QVector<int> >();
    m_settings.endGroup();

    const QStringList colNames = columnNames();
    for (int c = 0; c < DialogScriptModel::NumberOfColumns; ++c)
    {
        QAction* action = m_headerPopup->addAction(colNames[c]);
        action->setData(QVariant(c));
        const bool isHidden = hiddenColumns.contains(c);
        action->setCheckable(true);
        action->setChecked(!isHidden);
        ToggleColumnVisibility(action);
    }
}

void DialogScriptView::SaveColumnsVisibility()
{
    m_settings.beginGroup("DialogEditor");

    QVector<int> hiddenColumns;
    auto actions = m_headerPopup->actions();
    for (int i = 0; i < actions.size(); ++i)
    {
        if (!actions.at(i)->isChecked())
        {
            hiddenColumns.push_back(i);
        }
    }

    m_settings.setValue("hiddenColumns", QVariant::fromValue(hiddenColumns));
    m_settings.endGroup();
    m_settings.sync();
}

void DialogScriptView::InitializeColumnWidths()
{
    static const int widths[] = { 30, 45, 150, 100, 50, 30, 30, 80, 50, 40, 60, 40, 40, 100 };
    static_assert(sizeof(widths) / sizeof(int) == DialogScriptModel::NumberOfColumns, "Wrong array size");

    for (int c = 0; c < DialogScriptModel::NumberOfColumns; ++c)
    {
        setColumnWidth(c, widths[c]);
    }
}

bool DialogScriptView::eventFilter(QObject* watched, QEvent* ev)
{
    if (ev->type() == QEvent::ContextMenu)
    {
        QContextMenuEvent* e = static_cast<QContextMenuEvent*>(ev);
        m_headerPopup->popup(mapToGlobal(e->pos()));
        return true;
    }
    return false;
}

int DialogScriptView::SelectedRow() const
{
    auto indexList = selectedIndexes();
    return indexList.isEmpty() ? -1 : indexList.at(0).row();
}

void DialogScriptView::AddNewRow(bool forceEnd)
{
    if (m_model == nullptr)
    {
        return;
    }

    auto script = m_model->Script();
    if (!IsAllowEdit() || script == nullptr)
    {
        return;
    }

    static const CEditorDialogScript::SScriptLine newLine;
    int selectedRow = SelectedRow();
    if (selectedRow != -1 && !forceEnd)
    {
        m_model->InsertAt(selectedRow + 1, CEditorDialogScript::SScriptLine());
    }
    else
    {
        m_model->Append(CEditorDialogScript::SScriptLine());
    }

    // TODO: Re-set focused row
}

void DialogScriptView::RemoveSelectedRow()
{
    int selectedRow = SelectedRow();
    if (selectedRow != -1)
    {
        m_model->RemoveAt(selectedRow);
    }
}

int DialogScriptView::GetSelectedColumn() const
{
    if (!model() || !selectionModel())
    {
        return -1;
    }
    const auto indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
    {
        return -1;
    }

    return indexes.first().column();
}

QString DialogScriptView::GetCurrentHelpText() const
{
    const int selectedColumn = GetSelectedColumn();
    if (selectedColumn == -1)
    {
        return {};
    }

    const QString caption = model()->headerData(selectedColumn, Qt::Horizontal, Qt::DisplayRole).toString();
    const QString help = model()->headerData(selectedColumn, Qt::Horizontal, Qt::WhatsThisRole).toString();
    return QStringLiteral("[%1]\r\n[%2]").arg(caption, help);
}

bool DialogScriptView::IsModified() const
{
    return m_modified;
}

void DialogScriptView::TryDeleteRow()
{
    if (!IsAllowEdit())
    {
        return;
    }

    auto indexes = selectedIndexes();
    if (indexes.isEmpty())
    {
        return;
    }
    QMessageBox msgBox(this);
    msgBox.setText(tr("Delete current line?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    int ret = msgBox.exec();
    if (ret != QMessageBox::Yes)
    {
        return;
    }

    QModelIndex index = indexes.first();
    m_model->RemoveAt(index.row());
    QModelIndex indexToSelect = model()->index(index.row(), index.column()); // index is stale, get a new one
    if (indexToSelect.isValid())
    {
        selectionModel();
    }
}
void  DialogScriptView::SetModified(bool modified)
{
    if (modified != m_modified)
    {
        m_modified = modified;
        emit modifiedChanged(modified);
    }
}

void DialogScriptView::OnBrowseAudioTrigger(int row)
{
    if (row < 0 || row >= m_model->rowCount())
    {
        return;
    }

    SResourceSelectorContext x;
    x.typeName = "AudioTrigger";
    x.parentWidget = this;

    QString value;
    value = GetIEditor()->GetResourceSelectorHost()->SelectResource(x, value);
    m_model->setData(m_model->index(row, DialogScriptModel::AudioIDColumn), value, Qt::EditRole);
}

struct MsgHelper
{
    explicit MsgHelper(const QString& msg, QWidget* parent)
        : m_msg(msg)
        , m_show(true)
        , m_parent(parent)
    {}

    ~MsgHelper()
    {
        if (m_show)
        {
            QMessageBox box(m_parent);
            box.setText(m_msg);
            box.setStandardButtons(QMessageBox::Ok);
            box.exec();
        }
    }
    QString m_msg;
    bool m_show;
    QWidget* m_parent;
};

void DialogScriptView::OnBrowseFacial(int row)
{
    if (row < 0 || row >= m_model->rowCount())
    {
        return;
    }

    string value;
    MsgHelper msg(tr("Please select an Entity to be used\nas reference for facial expression."), this);

    CBaseObject* pObject = GetIEditor()->GetSelectedObject();
    if (pObject == nullptr)
    {
        return;
    }

    if (qobject_cast<CEntityObject*>(pObject) == nullptr)
    {
        return;
    }

    CEntityObject* pCEntity = static_cast<CEntityObject*> (pObject);
    IEntity* pEntity = pCEntity->GetIEntity();
    if (pEntity == nullptr)
    {
        return;
    }

    ICharacterInstance* pChar = pEntity->GetCharacter(0);
    if (pChar == nullptr)
    {
        return;
    }

    IFacialInstance* pFacialInstance = pChar->GetFacialInstance();
    if (pFacialInstance == nullptr)
    {
        return;
    }

    IFacialModel* pFacialModel = pFacialInstance->GetFacialModel();
    if (pFacialModel == nullptr)
    {
        return;
    }

    IFacialEffectorsLibrary* pLibrary = pFacialModel->GetLibrary();
    if (pLibrary == nullptr)
    {
        return;
    }

    struct Recurser
    {
        void Recurse(IFacialEffector* pEff, QString path, std::set<QString>& itemSet)
        {
            const EFacialEffectorType efType = pEff->GetType();
            if (efType != EFE_TYPE_GROUP)
            {
                QString val = path;
                if (val.isEmpty() == false)
                {
                    val += ".";
                }
                val += pEff->GetName();
                itemSet.insert(val);
            }
            else
            {
                if (path.isEmpty() == false)
                {
                    path += ".";
                }
                if (strcmp(pEff->GetName(), "Root") != 0)
                {
                    path += pEff->GetName();
                }
                for (int i = 0; i < pEff->GetSubEffectorCount(); ++i)
                {
                    Recurse(pEff->GetSubEffector(i), path, itemSet);
                }
            }
        }
    };

    std::set<QString> itemSet;
    Recurser recurser;
    recurser.Recurse(pLibrary->GetRoot(), "", itemSet);
    if (itemSet.empty())
    {
        msg.m_msg = tr("Entity has no facial expressions.");
        return;
    }

    msg.m_show = false;

    CGenericSelectItemDialog dlg;
    QStringList items;
    items.reserve(itemSet.size());
    std::copy(itemSet.begin(), itemSet.end(), std::back_inserter(items));

    // in dialog, items are with path, but value is without
    // as facial expressions have a unique name
    QString valString = value.c_str();
    if (valString.isEmpty() == false)
    {
        QStringList::iterator iter = items.begin();
        while (iter != items.end())
        {
            const QString& s = *iter;
            if (s.contains(valString))
            {
                valString = s;
                break;
            }
            ++iter;
        }
    }

    dlg.SetMode(CGenericSelectItemDialog::eMODE_TREE);
    dlg.SetTreeSeparator(".");
    dlg.SetItems(items);
    dlg.setWindowTitle(tr("Choose Facial Expression"));
    dlg.PreSelectItem(valString);
    if (dlg.exec() == QDialog::Accepted)
    {
        valString = dlg.GetSelectedItem();
        int delim = valString.lastIndexOf('.');
        if (delim > 0)
        {
            valString = valString.mid(delim + 1);
        }
        value = valString.toUtf8().data();
    }

    m_model->setData(m_model->index(row, DialogScriptModel::FacialExprColumn), QVariant(QtUtil::ToQString(value)), Qt::EditRole);
}

void DialogScriptView::OnBrowseAG(int row)
{
    if (row < 0 || row >= m_model->rowCount())
    {
        return;
    }

    string value;
    MsgHelper msg(tr("Please select an Entity to be used\nas reference for AnimationGraph actions/signals."), this);

    CBaseObject* pObject = GetIEditor()->GetSelectedObject();
    if (pObject == 0)
    {
        return;
    }

    if (qobject_cast<CEntityObject*>(pObject) == nullptr)
    {
        return;
    }

    CEntityObject* pCEntity = static_cast<CEntityObject*> (pObject);
    IEntity* pEntity = pCEntity->GetIEntity();
    if (pEntity == 0)
    {
        return;
    }

    if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
    {
        IMannequin& mannequin = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

        IActionController* pActionController = mannequin.FindActionController(*pEntity);
        if (pActionController)
        {
            msg.m_msg = tr("Dialogs do not support starting Mannequin fragments");
            return;
        }
    }

    m_model->setData(m_model->index(row, DialogScriptModel::AnimationColumn), QVariant(QtUtil::ToQString(value)), Qt::EditRole);
}

void DialogScriptView::UpdateNoItemText()
{
    if (!m_model)
    {
        return;
    }

    const int numRows = m_model->rowCount();
    const bool hasScript = m_model->GetScript() != nullptr;
    if (hasScript && numRows == 0)
    {
        m_label->setText(tr("No Dialog Lines yet.\nUse double click or toolbar to add a new line."));
    }
    else if (!hasScript)
    {
        m_label->setText(tr("No Dialog Script selected.\nCreate a new one or use browser on the left to select one."));
    }
    else
    {
        m_label->setText(QString());
    }

    m_label->setVisible(!m_label->text().isEmpty());
}

void DialogScriptView::PlayLine(int row)
{
    StopSound();
    const CEditorDialogScript::SScriptLine* pLine = m_model->getLineAt(row);
    if (!pLine)
    {
        return;
    }

    Audio::TAudioControlID audioTriggerID = INVALID_AUDIO_CONTROL_ID;
    Audio::AudioSystemRequestBus::BroadcastResult(audioTriggerID, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, pLine->m_audioTriggerName);
    if (audioTriggerID != INVALID_AUDIO_CONTROL_ID && m_pIAudioProxy)
    {
        const CCamera& camera = GetIEditor()->GetSystem()->GetViewCamera();
        m_pIAudioProxy->SetPosition(Audio::SATLWorldPosition(camera.GetMatrix()));
        m_pIAudioProxy->ExecuteTrigger(audioTriggerID, eLSM_None);
        ms_currentPlayLine = audioTriggerID;
    }
}

void DialogScriptView::StopSound()
{
    if (ms_currentPlayLine != INVALID_AUDIO_CONTROL_ID)
    {
        if (m_pIAudioProxy)
        {
            m_pIAudioProxy->StopTrigger(ms_currentPlayLine);
        }
        ms_currentPlayLine = INVALID_AUDIO_CONTROL_ID;
    }
}

DialogScriptModel::DialogScriptModel(QObject* parent)
    : QAbstractTableModel(parent)
    , m_script(nullptr)
{
    connect(this, &QAbstractItemModel::dataChanged, this, &DialogScriptModel::modelModified);
    connect(this, &QAbstractItemModel::rowsInserted, this, &DialogScriptModel::modelModified);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &DialogScriptModel::modelModified);
}

void DialogScriptModel::Reset()
{
    beginResetModel();
    m_rows.clear();

    if (m_script != nullptr)
    {
        const int count = m_script->GetNumLines();
        m_rows.reserve(count);
        for (int i = 0; i < count; ++i)
        {
            m_rows.push_back(*(m_script->GetLine(i)));
        }
    }

    endResetModel();
}

int DialogScriptModel::rowCount(const QModelIndex& parent) const
{
    return m_rows.count();
}

int DialogScriptModel::columnCount(const QModelIndex& parent) const
{
    return NumberOfColumns;
}

QVariant DialogScriptModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || m_script == nullptr)
    {
        return QVariant();
    }

    const int row = index.row();
    const int col = index.column();
    if (row < 0 || row >= rowCount() || col < 0 || col >= columnCount())
    {
        return QVariant();
    }

    const auto rowContents = m_rows.at(row);

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch ((Column)col)
        {
        case LineColumn:
            return index.row() + 1;
        case ActorColumn:
            return tr("Actor %1").arg(rowContents.m_actor + 1);
        case AudioIDColumn:
        {
            return QtUtil::ToQString(rowContents.m_audioTriggerName);
        }
        case AnimationColumn:
            return QtUtil::ToQString(rowContents.m_anim);
        case TypeColumn:
        {
            return rowContents.m_flagAGSignal ? tr("Signal") : tr("Action");
        }
        case FacialExprColumn:
            return QtUtil::ToQString(rowContents.m_facial);
        case WeightColumn:
            return QString::number(rowContents.m_facialWeight, 'f', 2);
        case FadeColumn:
            return QString::number(rowContents.m_facialFadeTime, 'f', 2);
        case LookAtColumn:
        {
            switch (rowContents.m_lookatActor)
            {
            case CEditorDialogScript::NO_ACTOR_ID:
                return QStringLiteral("---");
            case CEditorDialogScript::STICKY_LOOKAT_RESET_ID:
                return s_resetString;
            default:
                return tr("Actor%1").arg(rowContents.m_lookatActor + 1);
            }

            return true;
        }
        case DelayColumn:
            return QString::number(rowContents.m_delay, 'f', 2);
        case DescriptionColumn:
            return QtUtil::ToQString(rowContents.m_desc);
        case NumberOfColumns:
        case EPColumn:
        case SyncColumn:
        case StickyColumn:
        default:
            return QVariant();
        }
    }
    else if (role == Qt::CheckStateRole)
    {
        switch ((Column)col)
        {
        case EPColumn:
            return QVariant(rowContents.m_flagAGEP ? Qt::Checked : Qt::Unchecked);
        case SyncColumn:
            return QVariant(rowContents.m_flagSoundStopsAnim ? Qt::Checked : Qt::Unchecked);
        case StickyColumn:
            return QVariant(rowContents.m_flagLookAtSticky ? Qt::Checked : Qt::Unchecked);
        default:
            return QVariant();
        }
    }
    else if (role == Qt::ForegroundRole)
    {
        switch ((Column)col)
        {
        case FacialExprColumn:
        case LookAtColumn:
        {
            if (data(index, Qt::DisplayRole).toString() == s_resetString)
            {
                return QBrush(QColor(0xA0, 0x20, 0x20));
            }
        }
        }
    }
    else if (role == Qt::FontRole)
    {
        switch ((Column)col)
        {
        case FacialExprColumn:
        case LookAtColumn:
        {
            if (data(index, Qt::DisplayRole).toString() == s_resetString)
            {
                QFont f = qApp->font();
                f.setBold(true);
                return f;
            }
        }
        }
    }

    return QVariant();
}

bool DialogScriptModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
    {
        return false;
    }

    auto column = static_cast<Column>(index.column());
    auto& rowContents = m_rows[index.row()];

    if (role == Qt::EditRole)
    {
        switch (column)
        {
        case ActorColumn:
            rowContents.m_actor = value.toInt();
            emit dataChanged(index, index);
            return true;
        case AudioIDColumn:
            rowContents.m_audioTriggerName = QtUtil::ToString(value.toString());
            emit dataChanged(index, index);
            return true;
        case AnimationColumn:
            rowContents.m_anim = QtUtil::ToString(value.toString());
            emit dataChanged(index, index);
            return true;
        case TypeColumn:
            rowContents.m_flagAGSignal = value.toBool();
            emit dataChanged(index, index);
            return true;
        case FacialExprColumn:
            rowContents.m_facial = QtUtil::ToString(value.toString());
            emit dataChanged(index, index);
            return true;
        case WeightColumn:
            rowContents.m_facialWeight = value.toString().toDouble();
            emit dataChanged(index, index);
            return true;
        case FadeColumn:
            rowContents.m_facialFadeTime = value.toString().toDouble();
            emit dataChanged(index, index);
            return true;
        case LookAtColumn:
        {
            rowContents.m_lookatActor = value.toInt();
            emit dataChanged(index, index);
            return true;
        }
        case DelayColumn:
            rowContents.m_delay = value.toString().toDouble();
            emit dataChanged(index, index);
            return true;
        case DescriptionColumn:
            rowContents.m_desc = QtUtil::ToString(value.toString());
            emit dataChanged(index, index);
            return true;
        }
    }
    else if (role == Qt::CheckStateRole)
    {
        switch (column)
        {
        case EPColumn:
            rowContents.m_flagAGEP = value.toBool();
            emit dataChanged(index, index);
            return true;
        case SyncColumn:
            rowContents.m_flagSoundStopsAnim = value.toBool();
            emit dataChanged(index, index);
            return true;
        case StickyColumn:
            rowContents.m_flagLookAtSticky = value.toBool();
            emit dataChanged(index, index);
            return true;
        }
    }

    return false;
}

QVariant DialogScriptModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0 || section >= NumberOfColumns)
    {
        return QVariant();
    }

    if (orientation == Qt::Vertical)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        return columnNames().at(section);
    }
    else if (role == Qt::TextAlignmentRole)
    {
        return QVariant(Qt::AlignLeft);
    }
    else if (role == Qt::ToolTipRole)
    {
        switch (section)
        {
        case LineColumn:
            return tr("Line");
        case ActorColumn:
            return tr("Actor");
        case AudioIDColumn:
            return tr("AudioID");
        case AnimationColumn:
            return tr("Animation");
        case TypeColumn:
            return tr("Animation Type: Signal or Action");
        case FacialExprColumn:
            return tr("FacialExpr.");
        case WeightColumn:
            return tr("Weight");
        case FadeColumn:
            return tr("Fade");
        case LookAtColumn:
            return tr("Look at Target");
        case DelayColumn:
            return tr("Delay");
        case DescriptionColumn:
            return tr("Description");
        case EPColumn:
            return tr("Use Exact Positioning");
        case SyncColumn:
            return tr("Stop Animation when Sounds Stops");
        case StickyColumn:
            return tr("Sticky LookAt");
        case NumberOfColumns:
        default:
            return QVariant();
        }
    }
    else if (role == Qt::WhatsThisRole)
    {
        switch (section)
        {
        case LineColumn:
            return tr("Current line. Click this column to select the whole line. You can then drag the line around.\r\nTo add a new line double click in free area or use [Add ScriptLine] from the toolbar.\r\nTo delete a line press [Delete] or use [Delete ScriptLine] from the toolbar.\r\nDoubleClick plays or stops the Sound of the Line.");
        case ActorColumn:
            return tr("Actor for this line. Select from the combo-box.\r\nAny Entity can be an Actor. An Entity can be assigned as Actor using the Dialog:PlayDialog flownode.");
        case AudioIDColumn:
            return tr("AudioControllID to be triggered. \r\nIf a sound is specified the time of 'Delay' is relative to the end of the sound.");
        case AnimationColumn:
            return tr("Name of Animation Signal or Action. You should select an entity first [in the main view] and then browse its signals/actions.");
        case TypeColumn:
            return tr("Signal = OneShot animation. Action = Looping animation. Animation can automatically be stopped, when [Sync] is enabled.");
        case FacialExprColumn:
            return tr("Facial expression. Normally, every sound is already lip-synced and may already contain facial expressions. So use with care.\r\nCan be useful when you don't play a sound, but simply want the Actor make look with a specific mood/expression.\r\nUse #RESET# to reset expression to default state.");
        case WeightColumn:
            return tr("Weight of the facial expression [0-1]. How strong the facial expression should be applied.\r\nWhen sound already contains a facial expression, use only small values < 0.5.");
        case FadeColumn:
            return tr("Fade-time of the facial expression in seconds. How fast the facial expression should be applied.");
        case LookAtColumn:
            return tr("Lookat target of the Actor. Actor will try to look at his target before he starts to talk/animate.\r\nSometimes this cannot be guaranteed. Also, while doing his animation or talking he may no longer face his target. For these cases use Sticky lookat. To disable sticky look-at use #RESET#.");
        case DelayColumn:
            return tr("Delay in seconds before advancing to the next line. When a sound is played, Delay is relative to the end of the sound.\r\nSo, to slightly overlap lines and to make dialog more natural, you can use negative delays.\r\nWhen no sound is specified, the delay is relative to the start of the line.");
        case DescriptionColumn:
            return tr("Description of the line.");
        case EPColumn:
            return tr("When checked, Exact Positioning [EP] will be used to play the animation. The target for the EP is the LookAt target.\r\nIf you want to make sure that an animation is exactly oriented, use this option.");
        case SyncColumn:
            return tr("When you tell an Actor to look at its target, it's only applied right at the beginning of the current script line.\r\nIf you want to make him constantly facing you, activate Sticky lookat.\r\nYou can reset Sticky lookat by #RESET# as LookAt target or a new lookat target with Sticky enabled.");
        case StickyColumn:
            return tr("Sticky LookAt");
        case NumberOfColumns:
        default:
            return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags DialogScriptModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = Qt::NoItemFlags;

    if (!index.isValid())
    {
        return flags;
    }

    const bool hasCheckBox = index.column() == SyncColumn ||
        index.column() == EPColumn   ||
        index.column() == StickyColumn;

    if (index.column() != LineColumn && !hasCheckBox) // First column isn't editable through line edit
    {
        flags |= Qt::ItemIsEditable;
    }

    if (hasCheckBox)
    {
        flags |= Qt::ItemIsUserCheckable;
    }

    return flags | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void DialogScriptModel::SetScript(CEditorDialogScript* script)
{
    if (script == m_script)
    {
        return;
    }

    m_script = script;
    Reset();
}

CEditorDialogScript* DialogScriptModel::GetScript() const
{
    return m_script;
}

void DialogScriptModel::InsertAt(int row, const CEditorDialogScript::SScriptLine& record)
{
    const int count = rowCount();
    if (row < 0 || row > count)
    {
        row = count;
    }
    beginInsertRows(QModelIndex(), row, row);
    m_rows.insert(row, record);
    endInsertRows();
}

void DialogScriptModel::Append(const CEditorDialogScript::SScriptLine& record)
{
    const int row = m_rows.count();
    InsertAt(row, record);
}

CEditorDialogScript* DialogScriptModel::Script() const
{
    return m_script;
}

void DialogScriptModel::SaveToScript()
{
    if (m_script == nullptr)
    {
        return;
    }

    m_script->RemoveAll();

    foreach(auto row, m_rows)
    m_script->AddLine(row);
}

void DialogScriptModel::RemoveAt(int row)
{
    if (row < 0 || row >= m_rows.count())
    {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_rows.removeAt(row);
    endRemoveRows();
}

const CEditorDialogScript::SScriptLine* DialogScriptModel::getLineAt(int row) const
{
    if (row < 0 || row >= m_rows.count())
    {
        return nullptr;
    }

    return &m_rows.at(row);
}

#include <DialogEditor/DialogScriptView.moc>
