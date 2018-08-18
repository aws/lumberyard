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

// Description : Implementation of AssetBrowserSearchDlg.h

#include "StdAfx.h"
#include "AssetBrowserSearchDlg.h"
#include "AssetBrowserDialog.h"
#include "AssetBrowserManager.h"

#include <AssetBrowser/ui_AssetBrowserSearchDlg.h>

namespace AssetBrowser
{
    const UINT kSearchEditBoxOffset = 0;
    const UINT  kSearchFileFontSize = 100;
    const UINT  kSearchEditBoxMargin = 4;
    const char* kSearchTextDefault = "Type your search text here (you can use * and ? as wildcards)";
    const QColor kSearchTextDefaultForeColor(150, 150, 150);
    const QColor kSearchTextDefaultBackColor(255, 255, 170);
    const QColor kSearchTextForeColor(0, 0, 0);
    const QColor kSearchTextBackColor(170, 255, 170);
    const QColor kSearchNoResultsForeColor(55, 0, 0);
    const QColor kSearchNoResultsBackColor(255, 70, 50);
};

// CAssetBrowserSearchDlg dialog

CAssetBrowserSearchDlg::CAssetBrowserSearchDlg(QWidget* pParent /*=NULL*/)
    : QWidget(pParent)
    , m_ui(new Ui::AssetBrowserSearchDlg)
{
    m_ui->setupUi(this);

    m_pAssetBrowserDlg = NULL;
    m_bForceFilterUsedInLevel = m_bForceShowFavorites = m_bForceHideLods = false;
    m_assetSearchType = AssetBrowser::AssetBrowserSearchTypes_Tags;

    connect(m_ui->m_edSearchText, &QLineEdit::returnPressed, this, &CAssetBrowserSearchDlg::OnEnChangeEditSearchText);

    OnInitDialog();
}

CAssetBrowserSearchDlg::~CAssetBrowserSearchDlg()
{
}


// CAssetBrowserSearchDlg message handlers

void CAssetBrowserSearchDlg::OnInitDialog()
{
    m_ui->m_edSearchText->setFont(QFont(QStringLiteral("Arial"), AssetBrowser::kSearchFileFontSize / 10.0));
    m_ui->m_edSearchText->setPlaceholderText(AssetBrowser::kSearchTextDefault);
    layout()->setContentsMargins(AssetBrowser::kSearchEditBoxOffset, AssetBrowser::kSearchEditBoxMargin, AssetBrowser::kSearchEditBoxMargin, AssetBrowser::kSearchEditBoxMargin);
}

void CAssetBrowserSearchDlg::OnEnChangeEditSearchText()
{
    const QString text = m_ui->m_edSearchText->text();

    if (!text.isEmpty() && text[0] == ':')
    {
        HandleDebugCommand(text.mid(1));
    }
    else
    {
        Search();
    }
}

void CAssetBrowserSearchDlg::HandleDebugCommand(const QString& command)
{
    if (m_pAssetBrowserDlg == NULL)
    {
        return;
    }

    if (command.isEmpty() || command[command.length() - 1] != ';')
    {
        return;
    }

    QString command1 = command.left(command.length() - 1);

    /* add new debug commands here when required */
}

void CAssetBrowserSearchDlg::Search()
{
    static volatile bool s_bAlreadySearching = false;

    if (s_bAlreadySearching)
    {
        return;
    }

    assert(m_pAssetBrowserDlg);

    if (!m_pAssetBrowserDlg)
    {
        return;
    }

    s_bAlreadySearching = true;

    IAssetItemDatabase::TAssetFieldFiltersMap fields = CAssetBrowserDialog::Instance()->GetAssetViewer().GetCurrentFilters();

    for (size_t i = 0, iCount = m_fields.size(); i < iCount; ++i)
    {
        fields[m_fields[i].m_fieldName] = m_fields[i];
    }

    const QString originalSearchText = m_ui->m_edSearchText->text();
    bool bNoTextToSearch = false;

    const QString searchText = originalSearchText;

    if (searchText.isEmpty())
    {
        bNoTextToSearch = true;
    }

    fields["fullsearchtext"].m_fieldName = "fullsearchtext";
    fields["fullsearchtext"].m_fieldType = SAssetField::eType_String;
    fields["fullsearchtext"].m_filterValue = searchText;
    fields["fullsearchtext"].m_filterCondition = SAssetField::eCondition_Contains;

    //
    // also force other filters, which are usually found on the ribbon bar of the asset browser
    // kind of quick filtering checkboxes
    //
    if (m_bForceFilterUsedInLevel)
    {
        fields["usedinlevel"].m_fieldName = "usedinlevel";
        fields["usedinlevel"].m_fieldType = SAssetField::eType_Bool;
        fields["usedinlevel"].m_filterValue = m_bForceFilterUsedInLevel ? "Yes" : "No";
        fields["usedinlevel"].m_filterCondition = SAssetField::eCondition_Equal;
        fields["usedinlevel"].m_bPostFilter = true;
    }

    if (m_bForceShowFavorites)
    {
        fields["favorite"].m_fieldName = "favorite";
        fields["favorite"].m_fieldType = SAssetField::eType_Bool;
        fields["favorite"].m_filterValue = m_bForceShowFavorites ? "Yes" : "No";
        fields["favorite"].m_filterCondition = SAssetField::eCondition_Equal;
        fields["favorite"].m_bPostFilter = true;
    }

    if (m_bForceHideLods)
    {
        fields["islod"].m_fieldName = "islod";
        fields["islod"].m_fieldType = SAssetField::eType_Bool;
        fields["islod"].m_filterValue = m_bForceHideLods ? "No" : "Yes";
        fields["islod"].m_filterCondition = SAssetField::eCondition_Equal;
        fields["islod"].m_bPostFilter = true;
    }

    if (m_bAllowOnlyFilenames)
    {
        fields["fullfilepath"].m_fieldName = "fullfilepath";
        fields["fullfilepath"].m_fieldType = SAssetField::eType_String;
        fields["fullfilepath"].m_filterValue = m_allowedFilenames;
        fields["fullfilepath"].m_filterCondition = SAssetField::eCondition_ContainsOneOfTheWords;
    }

    //
    // this is the true magic happens, really, filtering the assets with the given field filters !
    //
    m_pAssetBrowserDlg->GetAssetViewer().ApplyFilters(fields);

    //
    // we change the colors of the search edit control, based on the filtering results
    //
    if (m_pAssetBrowserDlg->GetAssetViewer().GetAssetItems().empty() && !searchText.isEmpty())
    {
        m_ui->m_edSearchText->m_bNotFound = true;
        setStyleSheet(QString::fromLatin1("QLineEdit { color: %1; background: %2; }").arg(AssetBrowser::kSearchNoResultsForeColor.name(), AssetBrowser::kSearchNoResultsBackColor.name()));
    }
    else
    {
        m_ui->m_edSearchText->m_bNotFound = false;
        setStyleSheet(QString::fromLatin1("QLineEdit { color: %1; background: %2; }")
                .arg((bNoTextToSearch || (originalSearchText.isEmpty() && !m_ui->m_edSearchText->hasFocus()) ? AssetBrowser::kSearchTextDefaultForeColor : AssetBrowser::kSearchTextForeColor).name())
                .arg((bNoTextToSearch || originalSearchText.isEmpty() ? AssetBrowser::kSearchTextDefaultBackColor : AssetBrowser::kSearchTextBackColor).name()));
    }

    s_bAlreadySearching = false;
}

CSearchEditCtrl::CSearchEditCtrl(QWidget* parent)
    : QLineEdit(parent)
{
    setStyleSheet(QString::fromLatin1("QLineEdit { color: %1; background: %2; }").arg(AssetBrowser::kSearchTextDefaultForeColor.name(), AssetBrowser::kSearchTextDefaultBackColor.name()));
    m_bNotFound = false;
}

void CSearchEditCtrl::focusInEvent(QFocusEvent* event)
{
    QLineEdit::focusInEvent(event);

    if (text().isEmpty())
    {
        setStyleSheet(QString::fromLatin1("QLineEdit { color: %1; background: %2; }").arg(AssetBrowser::kSearchTextDefaultForeColor.name(), AssetBrowser::kSearchTextDefaultBackColor.name()));
    }
}

void CSearchEditCtrl::focusOutEvent(QFocusEvent* event)
{
    if (text().isEmpty())
    {
        setStyleSheet(QString::fromLatin1("QLineEdit { color: %1; background: %2; }").arg(AssetBrowser::kSearchTextDefaultForeColor.name(), AssetBrowser::kSearchTextDefaultBackColor.name()));
    }
    else if (!m_bNotFound)
    {
        setStyleSheet(QString::fromLatin1("QLineEdit { color: %1; background: %2; }").arg(AssetBrowser::kSearchTextForeColor.name(), AssetBrowser::kSearchTextBackColor.name()));
    }
}

#include <AssetBrowser/AssetBrowserSearchDlg.moc>