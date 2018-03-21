#include "stdafx.h"

#include "PropertyGenericCtrl.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QStringListModel>

#include "AIDialog.h"
#include "AIAnchorsDialog.h"
#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
#include "AICharactersDialog.h"
#endif
#include "SmartObjectClassDialog.h"
#include "SmartObjectStateDialog.h"
#include "SmartObjectPatternDialog.h"
#include "SmartObjectHelperDialog.h"
#include "SmartObjectActionDialog.h"
#include "SmartObjectEventDialog.h"
#include "SmartObjectTemplateDialog.h"
#include "ShadersDialog.h"
#include "Material/MaterialManager.h"
#include "Particles/ParticleManager.h"
#include "Particles/ParticleItem.h"
#include "Particles/ParticleDialog.h"
#include "SelectLightAnimationDialog.h"
#include "SelectGameTokenDialog.h"
#include "SelectSequenceDialog.h"
#include "SelectMissionObjectiveDialog.h"
#include "GenericSelectItemDialog.h"
#include "EquipPackDialog.h"
#include "SelectEAXPresetDlg.h"
#include "CustomActions/CustomActionDialog.h"
#include "DataBaseDialog.h"
#include "QtViewPaneManager.h"
#include <ILocalizationManager.h>

#include "AIPFPropertiesListDialog.h"
#include "AIEntityClassesDialog.h"

GenericPopupPropertyEditor::GenericPopupPropertyEditor(QWidget *pParent, bool showTwoButtons)
    :QWidget(pParent)
{
    m_valueLabel = new QLabel;

    QToolButton *mainButton = new QToolButton;
    mainButton->setText("..");
    connect(mainButton, &QToolButton::clicked, this, &GenericPopupPropertyEditor::onEditClicked);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(m_valueLabel, 1);
    mainLayout->addWidget(mainButton);
    mainLayout->setContentsMargins(1,1,1,1);

    if (showTwoButtons)
    {
        QToolButton *button2 = new QToolButton;
        button2->setText("<");
        connect(button2, &QToolButton::clicked, this, &GenericPopupPropertyEditor::onButton2Clicked);
        mainLayout->insertWidget(1, button2);
    }
}

void GenericPopupPropertyEditor::SetValue(const QString &value, bool notify)
{
    if (m_value != value)
    {
        m_value = value;
        m_valueLabel->setText(m_value);
        if (notify)
            emit ValueChanged(m_value);
    }
}

void GenericPopupPropertyEditor::SetPropertyType(PropertyType type)
{
    m_propertyType = type;
}

void AIAnchorPropertyEditor::onEditClicked()
{
    CAIAnchorsDialog aiDlg(this); 
    aiDlg.SetAIAnchor(GetValue());
    if (aiDlg.exec() == QDialog::Accepted)
    {
        SetValue(aiDlg.GetAIAnchor());
    }
}


void SOClassPropertyEditor::onEditClicked()
{
    CSmartObjectClassDialog soDlg(this);
    soDlg.SetSOClass(GetValue());
    if (soDlg.exec() == QDialog::Accepted)
        SetValue(soDlg.GetSOClass());
}

void SOClassesPropertyEditor::onEditClicked()
{
    CSmartObjectClassDialog soDlg(this, true);
    soDlg.SetSOClass(GetValue());
    if (soDlg.exec() == QDialog::Accepted)
        SetValue(soDlg.GetSOClass());
}

void SOStatePropertyEditor::onEditClicked()
{
    CSmartObjectStateDialog soDlg(GetValue(), this, false);
    if (soDlg.exec())
        SetValue(soDlg.GetSOState());
}

void SOStatesPropertyEditor::onEditClicked()
{
    CSmartObjectStateDialog soDlg(GetValue(), this, true );
    if (soDlg.exec())
        SetValue(soDlg.GetSOState());
}

void SOStatePatternPropertyEditor::onEditClicked()
{
    QString sPattern = GetValue();
    CSmartObjectPatternDialog soPatternDlg(this);
    if (sPattern.indexOf('|') < 0)
    {
        CSmartObjectStateDialog soDlg(sPattern, this, true, true);
        switch (soDlg.exec())
        {
        case QDialog::Accepted:
            SetValue(soDlg.GetSOState());
            return;
        case QDialog::Rejected:
            return;
        case CSmartObjectStateDialog::ADVANCED_MODE_REQUESTED:
            sPattern = soDlg.GetSOState();
            break;
        }
    }
    soPatternDlg.SetPattern(sPattern);
    if (soPatternDlg.exec() == QDialog::Accepted)
        SetValue(soPatternDlg.GetPattern());
}

void SOActionPropertyEditor::onEditClicked()
{
    CSmartObjectActionDialog soDlg(this);
    soDlg.SetSOAction(GetValue());
    if (soDlg.exec() == QDialog::Accepted)
        SetValue(soDlg.GetSOAction());
}

void SOHelperPropertyEditor::onEditClicked()
{
    const PropertyType propertyType = GetPropertyType();
    const bool bAllowEmpty = (propertyType == ePropertySOHelper || propertyType == ePropertySOAnimHelper);
    const bool bFromTemplate = (propertyType == ePropertySONavHelper);
    const bool bShowAuto = (propertyType == ePropertySOAnimHelper);

    CSmartObjectHelperDialog soDlg(this, bAllowEmpty, bFromTemplate, bShowAuto);
    const QString value = GetValue();
    int f = value.indexOf(':');
    if (f > 0)
    {
        soDlg.SetSOHelper(value.left(f), value.mid(f + 1));
        if (soDlg.exec() == QDialog::Accepted)
            SetValue(soDlg.GetSOHelper());
    }
    else
    {
        QMessageBox::warning(this, tr("No Class Selected"), tr("This field can not be edited because it needs the smart object class.\nPlease, choose a smart object class first..."));
    }
}

void SOEventPropertyEditor::onEditClicked()
{
    CSmartObjectEventDialog soDlg(this);
    soDlg.SetSOEvent(GetValue());
    if (soDlg.exec() == QDialog::Accepted)
        SetValue(soDlg.GetSOEvent());
}

void SOTemplatePropertyEditor::onEditClicked()
{
    CSmartObjectTemplateDialog soDlg;
    soDlg.SetSOTemplate(GetValue());
    if (soDlg.exec())
        SetValue(soDlg.GetSOTemplate());
}

void ShaderPropertyEditor::onEditClicked()
{
    CShadersDialog cShaders(GetValue());
    if (cShaders.exec() == QDialog::Accepted)
    {
        SetValue(cShaders.GetSelection());
    }

}
void MaterialPropertyEditor::onEditClicked()
{
    QString name = GetValue();
    IDataBaseItem *pItem = GetIEditor()->GetMaterialManager()->FindItemByName(name);
    GetIEditor()->OpenDataBaseLibrary(EDB_TYPE_MATERIAL, pItem);
}

void MaterialPropertyEditor::onButton2Clicked()
{
    // Open material browser dialog.
    IDataBaseItem *pItem = GetIEditor()->GetMaterialManager()->GetSelectedItem();
    if (pItem)
    {
        QString value = pItem->GetName();
        value.replace('\\', '/');
        if (value.length() >= MAX_PATH)
            value = value.left(MAX_PATH);
        SetValue(value);
    }
    else
        SetValue(QString());
}

void AiBehaviorPropertyEditor::onEditClicked()
{
    CAIDialog aiDlg(this);
    aiDlg.SetAIBehavior(GetValue());
    if (aiDlg.exec() == QDialog::Accepted)
    {
        SetValue(aiDlg.GetAIBehavior());
    }
}

#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
void AiCharacterPropertyEditor::onEditClicked()
{
    CAICharactersDialog aiDlg(this);
    aiDlg.SetAICharacter( GetValue() );
    if (aiDlg.exec() == QDialog::Accepted)
    {
        SetValue( aiDlg.GetAICharacter() );
    }

}
#endif

void EquipPropertyEditor::onEditClicked()
{
    CEquipPackDialog EquipDlg(this);
    EquipDlg.SetCurrEquipPack(GetValue());
    EquipDlg.SetEditMode(false);
    if (EquipDlg.exec() == QDialog::Accepted)
    {
        SetValue(EquipDlg.GetCurrEquipPack());
    }
}

void ReverbPresetPropertyEditor::onEditClicked()
{
    CSelectEAXPresetDlg PresetDlg(this);
    PresetDlg.SetCurrPreset(GetValue());
    if (PresetDlg.exec() == QDialog::Accepted)
    {
        SetValue(PresetDlg.GetCurrPreset());
    }
}

void CustomActionPropertyEditor::onEditClicked()
{
    CCustomActionDialog customActionDlg(this);
    customActionDlg.SetCustomAction(GetValue());
    if (customActionDlg.exec() == QDialog::Accepted)
        SetValue(customActionDlg.GetCustomAction());
}

void GameTokenPropertyEditor::onEditClicked()
{
    CSelectGameTokenDialog gtDlg(this);
    gtDlg.PreSelectGameToken(GetValue());
    if (gtDlg.exec() == QDialog::Accepted)
        SetValue(gtDlg.GetSelectedGameToken());
}


void MissionObjPropertyEditor::onEditClicked()
{
    CSelectMissionObjectiveDialog gtDlg(this);
    gtDlg.PreSelectItem(GetValue());
    if (gtDlg.exec() == QDialog::Accepted)
        SetValue(gtDlg.GetSelectedItem());
}


void SequencePropertyEditor::onEditClicked()
{
    CSelectSequenceDialog gtDlg(this);
    gtDlg.PreSelectItem(GetValue());
    if (gtDlg.exec() == QDialog::Accepted)
        SetValue(gtDlg.GetSelectedItem());
}

void SequenceIdPropertyEditor::onEditClicked()
{
    CSelectSequenceDialog gtDlg;
    uint32 id = GetValue().toUInt();
    IAnimSequence *pSeq = GetIEditor()->GetMovieSystem()->FindSequenceById(id);
    if (pSeq)
        gtDlg.PreSelectItem(pSeq->GetName());
    if (gtDlg.exec() == QDialog::Accepted)
    {
        pSeq = GetIEditor()->GetMovieSystem()->FindLegacySequenceByName(gtDlg.GetSelectedItem().toUtf8().data());
        assert(pSeq);
        if (pSeq->GetId() > 0)	// This sequence is a new one with a valid ID.
        {
            SetValue(QString::number(pSeq->GetId()));
        }
        else										// This sequence is an old one without an ID.
        {
            QMessageBox::warning(this, tr("Old Sequence"), tr("This is an old sequence without an ID.\nSo it cannot be used with the new ID-based linking."));
        }
    }
}

void LocalStringPropertyEditor::onEditClicked()
{
    std::vector<IVariable::IGetCustomItems::SItem> items;
    ILocalizationManager* pMgr = gEnv->pSystem->GetLocalizationManager();
    if (!pMgr)
        return;
    int nCount = pMgr->GetLocalizedStringCount();
    if (nCount <= 0)
        return;
    items.reserve(nCount);
    IVariable::IGetCustomItems::SItem item;
    SLocalizedInfoEditor sInfo;
    for (int i = 0; i < nCount; ++i)
    {
        if (pMgr->GetLocalizedInfoByIndex(i, sInfo))
        {
            item.desc = tr("English Text:\r\n");
            item.desc += QString::fromWCharArray(Unicode::Convert<wstring>(sInfo.sUtf8TranslatedText).c_str());
            item.name = sInfo.sKey;
            items.push_back(item);
        }
    }
    CGenericSelectItemDialog gtDlg;
    const bool bUseTree = true;
    if (bUseTree)
    {
        gtDlg.SetMode(CGenericSelectItemDialog::eMODE_TREE);
        gtDlg.SetTreeSeparator("/");
    }
    gtDlg.SetItems(items);
    gtDlg.setWindowTitle(tr("Choose Localized String"));
    QString preselect = GetValue();
    if (!preselect.isEmpty() && preselect.at(0) == '@')
        preselect = preselect.mid(1);
    gtDlg.PreSelectItem(preselect);
    if (gtDlg.exec() == QDialog::Accepted)
    {
        preselect = "@";
        preselect += gtDlg.GetSelectedItem();
        SetValue(preselect);
    }
}

void LightAnimationPropertyEditor::onEditClicked()
{
    // First, check if there is any light animation defined.
    bool bLightAnimationExists = false;
    IMovieSystem *pMovieSystem = GetIEditor()->GetMovieSystem();
    for (int i = 0; i < pMovieSystem->GetNumSequences(); ++i)
    {
        IAnimSequence *pSequence = pMovieSystem->GetSequence(i);
        if (pSequence->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet)
        {
            bLightAnimationExists = pSequence->GetNodeCount() > 0;
            break;
        }
    }

    if (bLightAnimationExists) // If exists, show the selection dialog.
    {
        CSelectLightAnimationDialog dlg;
        dlg.PreSelectItem(GetValue());
        if (dlg.exec() == QDialog::Accepted)
            SetValue(dlg.GetSelectedItem());
    }
    else                       // If not, remind the user of creating one in TrackView.
    {
        QMessageBox::warning(this, tr("No Available Animation"), tr("There is no available light animation.\nPlease create one in TrackView, first."));
    }
}

void ParticleNamePropertyEditor::onEditClicked()
{
    //BROWSE
    QString libraryName = "";
    CBaseLibraryItem* pItem = NULL;
    QString particleName(GetValue());

    if (!particleName.isEmpty())
    {
        libraryName = particleName.split(".").first();
        IEditorParticleManager* particleMgr = GetIEditor()->GetParticleManager();
        IDataBaseLibrary* pLibrary = particleMgr->FindLibrary(libraryName);
        if (pLibrary == NULL)
        {
            QString fullLibraryName = libraryName + ".xml";
            fullLibraryName = QString("Libs/Particles/") + fullLibraryName.toLower();
            GetIEditor()->SuspendUndo();
            pLibrary = particleMgr->LoadLibrary(fullLibraryName);
            GetIEditor()->ResumeUndo();
            if (pLibrary == NULL)
                return;
        }

        particleName.remove(0, libraryName.length() + 1);
        pItem = (CBaseLibraryItem*)pLibrary->FindItem(particleName);
        if (pItem == NULL)
        {
            QString leafParticleName(particleName);

            int lastDotPos = particleName.lastIndexOf('.');
            while (!pItem && lastDotPos > -1)
            {
                particleName.remove(lastDotPos, particleName.length() - lastDotPos);
                lastDotPos = particleName.lastIndexOf('.');
                pItem = (CBaseLibraryItem*)pLibrary->FindItem(particleName);
            }

            leafParticleName.replace(particleName, "");
            if (leafParticleName.isEmpty() || (leafParticleName.length() == 1 && leafParticleName[0] == '.'))
                return;
            if (leafParticleName[0] == '.')
                leafParticleName.remove(0, 1);
        }
    }

    QtViewPaneManager::instance()->OpenPane(LyViewPane::DatabaseView);

    CDataBaseDialog* pDataBaseDlg = FindViewPane<CDataBaseDialog>(LyViewPane::DatabaseView);
    if (!pDataBaseDlg)
        return;

    pDataBaseDlg->SelectDialog(EDB_TYPE_PARTICLE);

    CParticleDialog* particleDlg = (CParticleDialog*)pDataBaseDlg->GetCurrent();
    if (particleDlg == NULL)
        return;

    particleDlg->Reload();

    if (pItem && !libraryName.isEmpty())
        particleDlg->SelectLibrary(libraryName);

    particleDlg->SelectItem(pItem);
}

void ParticleNamePropertyEditor::onButton2Clicked()
{
    //APPLY
    CParticleDialog* rpoParticleBrowser = CParticleDialog::GetCurrentInstance();
    if (rpoParticleBrowser)
    {
        CParticleItem* currentParticle = rpoParticleBrowser->GetSelectedParticle();
        if (currentParticle)
        {
            rpoParticleBrowser->OnAssignToSelection();
            SetValue(currentParticle->GetFullName());
        }
    }
}


ListEditWidget::ListEditWidget(QWidget *pParent /*= nullptr*/)
    :QWidget(pParent)
{
    m_valueEdit = new QLineEdit;

    m_model = new QStringListModel(this);

    m_listView = new QListView;
    m_listView->setModel(m_model);
    m_listView->setMaximumHeight(50);
    m_listView->setVisible(false);

    QToolButton *expandButton = new QToolButton();
    expandButton->setCheckable(true);
    expandButton->setText("+");
    
    QToolButton *editButton = new QToolButton();
    editButton->setText("..");

    connect(editButton, &QAbstractButton::clicked, this, &ListEditWidget::OnEditClicked);
    connect(expandButton, &QAbstractButton::toggled, m_listView, &QWidget::setVisible);

    connect(m_model, &QAbstractItemModel::dataChanged, this, &ListEditWidget::OnModelDataChange);
    connect(m_valueEdit, &QLineEdit::editingFinished, [this](){SetValue(m_valueEdit->text(), true); } );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(expandButton);
    topLayout->addWidget(m_valueEdit,1);
    topLayout->addWidget(editButton);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_listView,1);
    mainLayout->setContentsMargins(1,1,1,1);
}

void ListEditWidget::SetValue(const QString &value, bool notify /*= true*/)
{
    if (m_value != value)
    {
        m_value = value;
        m_valueEdit->setText(value);
        QStringList list = m_value.split(",", QString::SkipEmptyParts);
        m_model->setStringList(list);

        if (notify)
            emit ValueChanged(m_value);
    }
}


void ListEditWidget::OnModelDataChange()
{
    m_value = m_model->stringList().join(",");
    m_valueEdit->setText(m_value);
    emit ValueChanged(m_value);
}


QWidget* ListEditWidget::GetFirstInTabOrder()
{
    return m_valueEdit;
}

QWidget* ListEditWidget::GetLastInTabOrder()
{
    return m_listView;
}

void AIPFPropertiesListEdit::OnEditClicked()
{
    CAIPFPropertiesListDialog dlg(this);
    dlg.SetPFPropertiesList(m_value);
    if (dlg.exec() == QDialog::Accepted)
        SetValue(dlg.GetPFPropertiesList());
}

void AIEntityClassesListEdit::OnEditClicked()
{
    CAIEntityClassesDialog dlg;
    dlg.SetAIEntityClasses(m_value);
    if (dlg.exec() == QDialog::Accepted)
    {
        SetValue(dlg.GetAIEntityClasses());
    }
}

#include <Controls/ReflectedPropertyControl/PropertyGenericCtrl.moc>
