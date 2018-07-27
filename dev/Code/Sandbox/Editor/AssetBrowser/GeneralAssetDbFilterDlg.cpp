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
#include "GeneralAssetDbFilterDlg.h"
#include "Util/EditorUtils.h"
#include "AssetBrowserDialog.h"
#include "AssetBrowserManager.h"
#include "QtUtilWin.h"

#include <QCheckBox>
#include <QMessageBox>
#include <QListWidgetItem>

#include <AssetBrowser/ui_GeneralAssetDbFilterDlg.h>
namespace AssetBrowser
{
    const char* kFilterPresetsFilename = "@engroot@/Editor/AssetBrowserFilterPresets.xml";
};

const char* presetNoneReference = "<None>";

CGeneralAssetDbFilterDlg::CGeneralAssetDbFilterDlg(QWidget* pParent /*=NULL*/)
    : QWidget(pParent)
    , ui(new Ui::GeneralAssetDbFilterDlg)
{
    ui->setupUi(this);

    // TODO : find out where are this value setup in the original code
    ui->ASSET_FILTERS_GENERAL_MIN_FILESIZE_COMBO->addItems(QStringList() << "0" << "10" << "100" << "1000" << "10000" << "20000" << "30000");
    ui->ASSET_FILTERS_GENERAL_MAX_FILESIZE_COMBO->addItems(QStringList() << "0" << "10" << "100" << "1000" << "10000" << "20000" << "30000" << "150000");
    ui->ASSET_FILTERS_GENERAL_MIN_FILESIZE_COMBO->setCurrentIndex(0);
    ui->ASSET_FILTERS_GENERAL_MAX_FILESIZE_COMBO->setCurrentIndex(ui->ASSET_FILTERS_GENERAL_MAX_FILESIZE_COMBO->count() - 1);

    LoadFilterPresets();
    FillPresetList();
    FillDatabases();

    connect(ui->ASSET_FILTERS_PRESET_COMBO, SIGNAL(currentIndexChanged(QString)), this, SLOT(OnCbnSelchangeComboPreset()));
    connect(ui->ASSET_FILTERS_PRESET_SAVE_BTN, &QPushButton::clicked, this, &CGeneralAssetDbFilterDlg::OnBnClickedButtonSavePreset);
    connect(ui->ASSET_FILTERS_PRESET_REMOVE_BTN, &QPushButton::clicked, this, &CGeneralAssetDbFilterDlg::OnBnClickedButtonRemovePreset);
    connect(ui->ASSET_FILTERS_GENERAL_USED_IN_LVL, &QCheckBox::clicked, this, &CGeneralAssetDbFilterDlg::OnBnClickedCheckUsedInLevel);
    connect(ui->ASSET_FILTERS_GENERAL_UPDATE_LVL_BTN, &QCheckBox::clicked, this, &CGeneralAssetDbFilterDlg::OnBnClickedButtonUpdateUsedInLevel);
    connect(ui->ASSET_FILTERS_GENERAL_MIN_FILESIZE_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMinimumFilesize(int)));
    connect(ui->ASSET_FILTERS_GENERAL_MAX_FILESIZE_COMBO, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCbnSelchangeComboMaximumFilesize(int)));
}

CGeneralAssetDbFilterDlg::~CGeneralAssetDbFilterDlg()
{
}

void CGeneralAssetDbFilterDlg::UpdateFilterUI()
{
    auto filters = m_pAssetViewer->GetCurrentFilters();

    {
        SAssetField& field = filters["filesize"];
        QString str;

        str = QString::number(field.m_filterValue.toInt() / 1024);
        int indexFound = ui->ASSET_FILTERS_GENERAL_MIN_FILESIZE_COMBO->findText(str);
        if (indexFound != -1)
        {
            ui->ASSET_FILTERS_GENERAL_MIN_FILESIZE_COMBO->setCurrentIndex(indexFound);
        }

        str = QString::number(field.m_maxFilterValue.toInt() / 1024);
        indexFound = ui->ASSET_FILTERS_GENERAL_MAX_FILESIZE_COMBO->findText(str);
        if (indexFound != -1)
        {
            ui->ASSET_FILTERS_GENERAL_MAX_FILESIZE_COMBO->setCurrentIndex(indexFound);
        }
    }

    {
        SAssetField& field = filters["usedinlevel"];
        ui->ASSET_FILTERS_GENERAL_USED_IN_LVL->setCheckState(field.m_filterValue == "Yes" ? Qt::Checked : Qt::Unchecked);
    }
}

void CGeneralAssetDbFilterDlg::ApplyFilter()
{
    auto filters = m_pAssetViewer->GetCurrentFilters();
    {
        SAssetField& field = filters["filesize"];

        field.m_fieldName = "filesize";
        field.m_filterCondition = SAssetField::eCondition_InsideRange;

        QString minFileSize = ui->ASSET_FILTERS_GENERAL_MIN_FILESIZE_COMBO->currentText();
        int filesize = minFileSize.toInt();
        field.m_filterValue = minFileSize;

        QString maxFileSize = ui->ASSET_FILTERS_GENERAL_MAX_FILESIZE_COMBO->currentText();
        filesize = maxFileSize.toInt();
        maxFileSize = QString::number(filesize * 1024);
        field.m_maxFilterValue = maxFileSize;
        field.m_fieldType = SAssetField::eType_Int32;
    }

    {
        bool bChecked = ui->ASSET_FILTERS_GENERAL_USED_IN_LVL->isChecked();

        if (bChecked)
        {
            SAssetField& field = filters["usedinlevel"];

            field.m_fieldName = "usedinlevel";
            field.m_filterCondition = SAssetField::eCondition_Equal;
            field.m_filterValue = "Yes";
            field.m_fieldType = SAssetField::eType_Bool;
        }
        else
        {
            auto iter = filters.find("usedinlevel");

            if (iter != filters.end())
            {
                filters.erase(iter);
            }
        }
    }

    m_pAssetViewer->ApplyFilters(filters);
}

bool CGeneralAssetDbFilterDlg::LoadFilterPresets()
{
    return LoadFilterPresetsTo(m_filterPresets);
}

bool CGeneralAssetDbFilterDlg::SaveFilterPresets()
{
    return SaveFilterPresetsFrom(m_filterPresets);
}

void CGeneralAssetDbFilterDlg::SaveCurrentPreset()
{
    UpdateFilterPreset(m_currentPresetName.toUtf8().data());
    SaveFilterPresetsFrom(m_filterPresets);
}

void CGeneralAssetDbFilterDlg::FillPresetList()
{
    ui->ASSET_FILTERS_PRESET_COMBO->clear();
    ui->ASSET_FILTERS_PRESET_COMBO->addItem(presetNoneReference);

    for (auto iter = m_filterPresets.begin(), iterEnd = m_filterPresets.end(); iter != iterEnd; ++iter)
    {
        auto presetName = iter->first;
        ui->ASSET_FILTERS_PRESET_COMBO->addItem(presetName);
    }

    ui->ASSET_FILTERS_PRESET_COMBO->setEditText(m_currentPresetName);
}

void CGeneralAssetDbFilterDlg::FillDatabases()
{
    TAssetDatabases dbs = CAssetBrowserManager::Instance()->GetAssetDatabases();
    foreach (auto checkbox, ui->ASSET_FILTERS_GENERAL_VISIBLE_DATABASE_OPTIONS_WIDGET->findChildren<QCheckBox*>())
    {
        ui->ASSET_FILTERS_GENERAL_VISIBLE_DATABASE_OPTIONS_LAYOUT->removeWidget(checkbox);
        checkbox->deleteLater();
    }

    int col = -1;
    for (size_t i = 0; i < dbs.size(); ++i)
    {
        auto dbCheckBox = new QCheckBox(QtUtil::ToQString(dbs[i]->GetDatabaseName()), ui->ASSET_FILTERS_GENERAL_VISIBLE_DATABASE_OPTIONS_WIDGET);
        dbCheckBox->setCheckState(Qt::Checked);
        connect(dbCheckBox, &QCheckBox::stateChanged, this, &CGeneralAssetDbFilterDlg::OnLvnItemchangedListDatabases);
        if (i % 3 == 0)
        {
            col++;             // take an other column every 3 rows
        }
        ui->ASSET_FILTERS_GENERAL_VISIBLE_DATABASE_OPTIONS_LAYOUT->addWidget(dbCheckBox, i % 3, col);
    }
}

void CGeneralAssetDbFilterDlg::OnCbnSelchangeComboPreset()
{
    IAssetItemDatabase::TAssetFields fields;
    if (ui->ASSET_FILTERS_PRESET_COMBO->count() > 0 && ui->ASSET_FILTERS_PRESET_COMBO->currentIndex() > 0)
    {
        m_currentPresetName = ui->ASSET_FILTERS_PRESET_COMBO->currentText();
        fields = m_filterPresets[m_currentPresetName].fields;
        auto preset = m_filterPresets[m_currentPresetName];

        foreach (auto item, ui->ASSET_FILTERS_GENERAL_VISIBLE_DATABASE_OPTIONS_WIDGET->findChildren<QCheckBox*>())
        {
            auto iter = std::find(preset.checkedDatabaseNames.begin(), preset.checkedDatabaseNames.end(), item->text());
            item->setCheckState(iter != preset.checkedDatabaseNames.end() ? Qt::Checked : Qt::Unchecked);
        }
        ui->ASSET_FILTERS_GENERAL_USED_IN_LVL->setCheckState(preset.bUsedInLevel ? Qt::Checked : Qt::Unchecked);
    }
    else
    {
        m_currentPresetName = "";
    }

    IAssetItemDatabase::TAssetFieldFiltersMap filters;

    for (size_t i = 0; i < fields.size(); ++i)
    {
        filters[fields[i].m_fieldName] = fields[i];
    }

    CAssetBrowserDialog::Instance()->GetAssetViewer().SetFilters(filters);
    CAssetBrowserDialog::Instance()->ApplyAllFiltering();
    UpdateFilterUI();
    CAssetBrowserDialog::Instance()->GetAssetFiltersDlg().UpdateAllFiltersUI();
}

bool CGeneralAssetDbFilterDlg::LoadFilterPresetsTo(TPresetNamePresetMap& rOutFilters, bool bClearMap)
{
    if (bClearMap)
    {
        rOutFilters.clear();
    }

    if (!CFileUtil::FileExists(QString(AssetBrowser::kFilterPresetsFilename)))
    {
        return false;
    }

    XmlNodeRef xmlRoot = GetISystem()->LoadXmlFromFile(AssetBrowser::kFilterPresetsFilename);
    XmlString xmlStr;
    QStringList strings;
    QString strTemp;

    if (!xmlRoot)
    {
        return false;
    }

    for (size_t i = 0, iCount = xmlRoot->getChildCount(); i < iCount; ++i)
    {
        SFieldFiltersPreset preset;

        xmlRoot->getChild(i)->getAttr("name", xmlStr);
        preset.presetName = xmlStr;
        xmlRoot->getChild(i)->getAttr("bUsedInLevel", preset.bUsedInLevel);
        xmlRoot->getChild(i)->getAttr("checkedDbs", strTemp);
        SplitString(strTemp, preset.checkedDatabaseNames);

        for (size_t j = 0, jCount = xmlRoot->getChild(i)->getChildCount(); j < jCount; ++j)
        {
            SAssetField field;
            XmlNodeRef xmlFilter = xmlRoot->getChild(i)->getChild(j);

            xmlFilter->getAttr("displayName", xmlStr);
            field.m_displayName = xmlStr;
            xmlFilter->getAttr("fieldType", xmlStr);
            field.m_fieldType = (SAssetField::EAssetFieldType)atoi(xmlStr.c_str());
            xmlFilter->getAttr("fieldName", xmlStr);
            field.m_fieldName = xmlStr;
            xmlFilter->getAttr("filterCondition", (int&)field.m_filterCondition);
            xmlFilter->getAttr("filterValue", xmlStr);
            field.m_filterValue = xmlStr;
            xmlFilter->getAttr("maxFilterValue", xmlStr);
            field.m_maxFilterValue = xmlStr;
            xmlFilter->getAttr("parentDB", xmlStr);
            field.m_parentDatabaseName = xmlStr;
            xmlFilter->getAttr("enumValues", xmlStr);
            SplitString(QString(xmlStr.c_str()), field.m_enumValues);
            xmlFilter->getAttr("useEnumValues", (int&)field.m_bUseEnumValues);

            preset.fields.push_back(field);
        }

        rOutFilters[preset.presetName] = preset;
    }

    return true;
}

bool CGeneralAssetDbFilterDlg::SaveFilterPresetsFrom(TPresetNamePresetMap& rFilters)
{
    QString         strTemp;
    XmlNodeRef  xmlRoot = XmlHelpers::CreateXmlNode("filterPresets");
    XmlString       xmlStr;

    if (!xmlRoot)
    {
        return false;
    }

    for (TPresetNamePresetMap::iterator iter = rFilters.begin(), iterEnd = rFilters.end(); iter != iterEnd; ++iter)
    {
        XmlNodeRef xmlFilterPreset = XmlHelpers::CreateXmlNode("filterPreset");
        xmlFilterPreset->setAttr("name", iter->first.toUtf8().data());
        xmlFilterPreset->setAttr("bUsedInLevel", iter->second.bUsedInLevel);
        JoinStrings(iter->second.checkedDatabaseNames, strTemp);
        xmlFilterPreset->setAttr("checkedDbs", strTemp.toUtf8().data());

        for (size_t i = 0, iCount = iter->second.fields.size(); i < iCount; ++i)
        {
            XmlNodeRef xmlFilter = XmlHelpers::CreateXmlNode("fieldFilter");
            xmlFilter->setAttr("displayName", iter->second.fields[i].m_displayName.toUtf8().data());
            xmlFilter->setAttr("fieldName", iter->second.fields[i].m_fieldName.toUtf8().data());
            xmlFilter->setAttr("fieldType", (int)iter->second.fields[i].m_fieldType);
            xmlFilter->setAttr("filterCondition", (int)iter->second.fields[i].m_filterCondition);
            xmlFilter->setAttr("filterValue", iter->second.fields[i].m_filterValue.toUtf8().data());
            xmlFilter->setAttr("maxFilterValue", iter->second.fields[i].m_maxFilterValue.toUtf8().data());
            xmlFilter->setAttr("parentDB", iter->second.fields[i].m_parentDatabaseName.toUtf8().data());
            strTemp = "";
            JoinStrings(iter->second.fields[i].m_enumValues, strTemp);
            xmlFilter->setAttr("enumValues", strTemp.toUtf8().data());
            xmlFilter->setAttr("useEnumValues", (int)iter->second.fields[i].m_bUseEnumValues);

            xmlFilterPreset->addChild(xmlFilter);
        }

        xmlRoot->addChild(xmlFilterPreset);
    }

    bool bResult = xmlRoot->saveToFile(AssetBrowser::kFilterPresetsFilename);

    return bResult;
}

bool CGeneralAssetDbFilterDlg::AddFilterPreset(const char* pPresetName)
{
    SFieldFiltersPreset newPreset;

    if (!strcmp(pPresetName, ""))
    {
        return false;
    }

    if (!GetFilterPresetByName(pPresetName))
    {
        // insert a new one
        newPreset.presetName = pPresetName;
        m_filterPresets[pPresetName] = newPreset;
        m_currentPresetName = pPresetName;
        SaveCurrentPreset();

        return true;
    }

    SaveFilterPresets();

    return false;
}

bool CGeneralAssetDbFilterDlg::UpdateFilterPreset(const char* pPresetName)
{
    if (!strcmp(pPresetName, ""))
    {
        return false;
    }

    if (m_filterPresets.empty())
    {
        return false;
    }

    SFieldFiltersPreset* pPreset = GetFilterPresetByName(pPresetName);

    if (!pPreset)
    {
        // no existing preset with that name
        return false;
    }

    IAssetItemDatabase::TAssetFieldFiltersMap filters = m_pAssetViewer->GetCurrentFilters();
    IAssetItemDatabase::TAssetFields fields;

    for (auto iter = filters.begin(); iter != filters.end(); ++iter)
    {
        fields.push_back(iter->second);
    }

    pPreset->fields = fields;
    pPreset->checkedDatabaseNames.clear();

    foreach (auto item, ui->ASSET_FILTERS_GENERAL_VISIBLE_DATABASE_OPTIONS_WIDGET->findChildren<QCheckBox*>())
    {
        if (item->checkState() == Qt::Checked)
        {
            pPreset->checkedDatabaseNames.push_back(item->text());
        }
    }

    pPreset->bUsedInLevel = ui->ASSET_FILTERS_GENERAL_USED_IN_LVL->isChecked();
    SaveFilterPresets();

    return true;
}

bool CGeneralAssetDbFilterDlg::DeleteFilterPreset(const char* pPresetName)
{
    m_filterPresets.erase(m_filterPresets.find(pPresetName));
    SaveFilterPresets();

    return true;
}

SFieldFiltersPreset* CGeneralAssetDbFilterDlg::GetFilterPresetByName(const char* pPresetName)
{
    if (m_filterPresets.end() == m_filterPresets.find(pPresetName))
    {
        return NULL;
    }

    return &m_filterPresets[pPresetName];
}

void CGeneralAssetDbFilterDlg::SelectPresetByName(const char* pName)
{
    int index = ui->ASSET_FILTERS_PRESET_COMBO->findText(pName);
    if (index != -1)
    {
        ui->ASSET_FILTERS_PRESET_COMBO->setCurrentIndex(index);
    }
}

void CGeneralAssetDbFilterDlg::OnBnClickedButtonSavePreset()
{
    QString presetName = ui->ASSET_FILTERS_PRESET_COMBO->currentText();
    //if we have not selected any valid preset name from the combo box or have not inputted any preset name than show the error message to the user
    if (ui->ASSET_FILTERS_PRESET_COMBO->currentIndex() == -1 || ui->ASSET_FILTERS_PRESET_COMBO->currentText() == presetNoneReference) // <None> case
    {
        QMessageBox::information(this, tr("Invalid preset"), tr("Please type in a valid preset name in the preset combobox edit."));
        return;
    }

    SFieldFiltersPreset* pPreset = GetFilterPresetByName(presetName.toUtf8().data());

    if (!pPreset)
    {
        if (AddFilterPreset(presetName.toUtf8().data()))
        {
            FillPresetList();
            SelectPresetByName(presetName.toUtf8().data());
            OnCbnSelchangeComboPreset();
        }
    }

    m_currentPresetName = presetName;
    SaveCurrentPreset();
    SaveFilterPresets();
    FillPresetList();
}

void CGeneralAssetDbFilterDlg::OnBnClickedButtonRemovePreset()
{
    if (ui->ASSET_FILTERS_PRESET_COMBO->currentIndex() <= 0)
    {
        QMessageBox::information(this, tr("No preset to delete"), tr("Please select a preset to delete."));
        return;
    }

    QString presetName;
    QString preset = ui->ASSET_FILTERS_PRESET_COMBO->currentText();
    presetName = preset;

    if (presetName != "")
    {
        QMessageBox msgBox;
        msgBox.setText(QString("Delete filter preset: %1 ?").arg(preset));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        if (msgBox.exec() == QMessageBox::Yes)
        {
            if (DeleteFilterPreset(presetName.toUtf8().data()))
            {
                FillPresetList();

                if (ui->ASSET_FILTERS_PRESET_COMBO->count() > 0)
                {
                    ui->ASSET_FILTERS_PRESET_COMBO->setCurrentIndex(0);
                }

                OnCbnSelchangeComboPreset();
                SaveFilterPresets();
            }
        }
    }
}


void CGeneralAssetDbFilterDlg::UpdateVisibleDatabases()
{
    foreach (auto item, ui->ASSET_FILTERS_GENERAL_VISIBLE_DATABASE_OPTIONS_WIDGET->findChildren<QCheckBox*>())
    {
        QString dbName = item->text();
        IAssetItemDatabase* pDB = CAssetBrowserManager::Instance()->GetDatabaseByName(dbName.toUtf8().data());

        if (!pDB)
        {
            return;
        }

        if (item->checkState() == Qt::Checked)
        {
            CAssetBrowserDialog::Instance()->GetAssetViewer().AddDatabase(pDB);
        }
        else
        {
            CAssetBrowserDialog::Instance()->GetAssetViewer().RemoveDatabase(pDB);
        }
    }

    m_pAssetViewer->ApplyFilters(m_pAssetViewer->GetCurrentFilters());
    CAssetBrowserDialog::Instance()->GetAssetFiltersDlg().RefreshVisibleAssetDbsRollups();
}

void CGeneralAssetDbFilterDlg::SelectDatabase(const char* pDbName)
{
    foreach (auto item, ui->ASSET_FILTERS_GENERAL_VISIBLE_DATABASE_OPTIONS_WIDGET->findChildren<QCheckBox*>())
    {
        bool bCheck = item->text() == pDbName;
        item->setCheckState(bCheck ? Qt::Checked : Qt::Unchecked);
    }

    CAssetBrowserDialog::Instance()->GetAssetViewer().ClearDatabases();

    IAssetItemDatabase* pDB = CAssetBrowserManager::Instance()->GetDatabaseByName(pDbName);

    if (!pDB)
    {
        return;
    }

    TAssetDatabases dbs;

    dbs.push_back(pDB);
    CAssetBrowserDialog::Instance()->GetAssetViewer().SetDatabases(dbs);
    m_pAssetViewer->ApplyFilters(m_pAssetViewer->GetCurrentFilters());
}

void CGeneralAssetDbFilterDlg::OnBnClickedCheckUsedInLevel()
{
    ApplyFilter();
}

void CGeneralAssetDbFilterDlg::OnCbnSelchangeComboMinimumFilesize(int)
{
    int min = ui->ASSET_FILTERS_GENERAL_MIN_FILESIZE_COMBO->currentText().toInt();
    int max = ui->ASSET_FILTERS_GENERAL_MAX_FILESIZE_COMBO->currentText().toInt();
    if (min > max)
    {
        ui->ASSET_FILTERS_GENERAL_MIN_FILESIZE_COMBO->setCurrentIndex(ui->ASSET_FILTERS_GENERAL_MAX_FILESIZE_COMBO->currentIndex());
        QMessageBox::information(this, tr("Invalid filesize range"), tr("Please choose a smaller value than the one specified in 'Maximum filesize'"));
        return;
    }

    ApplyFilter();
}

void CGeneralAssetDbFilterDlg::OnCbnSelchangeComboMaximumFilesize(int)
{
    int min = ui->ASSET_FILTERS_GENERAL_MIN_FILESIZE_COMBO->currentText().toInt();
    int max = ui->ASSET_FILTERS_GENERAL_MAX_FILESIZE_COMBO->currentText().toInt();
    if (max < min)
    {
        ui->ASSET_FILTERS_GENERAL_MAX_FILESIZE_COMBO->setCurrentIndex(ui->ASSET_FILTERS_GENERAL_MIN_FILESIZE_COMBO->currentIndex());
        QMessageBox::information(this, tr("Invalid filesize range"), tr("Please choose a greater value than the one specified in 'Minimum filesize'"));
        return;
    }

    ApplyFilter();
}

void CGeneralAssetDbFilterDlg::OnLvnItemchangedListDatabases()
{
    UpdateVisibleDatabases();
}

void CGeneralAssetDbFilterDlg::OnBnClickedButtonUpdateUsedInLevel()
{
    CAssetBrowserManager::Instance()->MarkUsedInLevelAssets();
    ApplyFilter();
}


#include <AssetBrowser/GeneralAssetDbFilterDlg.moc>