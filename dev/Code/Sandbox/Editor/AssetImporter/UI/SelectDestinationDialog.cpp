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

#include "StdAfx.h"
#include "SelectDestinationDialog.h"
#include <AssetImporter/UI/ui_SelectDestinationDialog.h>
#include <QtWidgets>
#include <QValidator>
#include <AzQtComponents/Components/EditorProxyStyle.h>
#include <Editor/Controls/QToolTipWidget.h>

static const char* g_assetProcessorLink = "<a href=\"https://docs.aws.amazon.com/console/lumberyard/userguide/asset-processor\">Asset Processor</a>";
static const char* g_copyFilesMessage = "The original file will remain outside of the project and the %1 will not monitor the file.";
static const char* g_moveFilesMessage = "The original file will be moved inside of the project and the %1 will monitor the file for changes.";
static const char* g_toolTip = "<p style='white-space:pre'> <span style=\"color:#cccccc;\">Invalid path. Please put the file(s) in your game project.</span> </p>";
static const char* g_selectDestinationFilesPath = "AssetImporter/SelectDestinationFilesPath";

namespace
{
    class DestinationValidator
        : public QValidator
    {
    public:
        DestinationValidator(QObject* parent)
            : QValidator(parent) {}

        State validate(QString& input, int& pos) const override
        {
            QString normalizedInput = QDir::fromNativeSeparators(input);
            QByteArray str = normalizedInput.toUtf8();
            const char* path = str.constData();

            QDir gameRoot(Path::GetEditingGameDataFolder().c_str());
            QString gameRootAbsPath = gameRoot.absolutePath();

            if (input.isEmpty())
            {
                return QValidator::Acceptable;
            }

            if (GetIEditor()->GetFileUtil()->PathExists(path) && normalizedInput.startsWith(gameRootAbsPath, Qt::CaseInsensitive))
            {
                return QValidator::Acceptable;
            }

            return QValidator::Intermediate;
        }
    };
}

SelectDestinationDialog::SelectDestinationDialog(QString message, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::SelectDestinationDialog)
    , m_validator(new DestinationValidator(this))
{
    m_ui->setupUi(this);

    QString radioButtonMessage = QString(g_copyFilesMessage).arg(g_assetProcessorLink);
    m_ui->RaidoButtonMessage->setText(radioButtonMessage);

    SetPreviousDestinationDirectory();

    m_ui->DestinationLineEdit->setValidator(m_validator);
    m_ui->DestinationLineEdit->setAlignment(Qt::AlignVCenter);

    // Based on the current code structure, in order to prevent texts from overlapping the 
    // invalid icon, intentionally insert an empty icon at the end of the line edit field can do the trick.
    m_ui->DestinationLineEdit->addAction(QIcon(""), QLineEdit::TrailingPosition);

    connect(m_ui->DestinationLineEdit, &QLineEdit::textChanged, this, &SelectDestinationDialog::ValidatePath);
    connect(m_ui->BrowseButton, &QPushButton::clicked, this, &SelectDestinationDialog::OnBrowseDestinationFilePath, Qt::UniqueConnection);
    connect(m_ui->CopyFileRadioButton, &QRadioButton::toggled, this, &SelectDestinationDialog::ShowMessage);

    UpdateMessage(message);
    InitializeButtons();
}

SelectDestinationDialog::~SelectDestinationDialog()
{
}

void SelectDestinationDialog::InitializeButtons()
{
    m_ui->CopyFileRadioButton->setChecked(true);

    m_ui->buttonBox->setContentsMargins(0, 0, 16, 16);

    QPushButton* importButton = m_ui->buttonBox->addButton(tr("Import"), QDialogButtonBox::AcceptRole);
    QPushButton* cancelButton = m_ui->buttonBox->addButton(QDialogButtonBox::Cancel);

    importButton->setProperty("class", "Primary");
    importButton->setDefault(true);

    cancelButton->setProperty("class", "AssetImporterButton");
    cancelButton->style()->unpolish(cancelButton);
    cancelButton->style()->polish(cancelButton);
    cancelButton->update();

    connect(importButton, &QPushButton::clicked, this, &SelectDestinationDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &SelectDestinationDialog::reject);
    connect(this, &SelectDestinationDialog::UpdateImportButtonState, importButton, &QPushButton::setEnabled);

    // To make sure the import button state is up to date
    ValidatePath();
    AzQtComponents::EditorProxyStyle::UpdatePrimaryButtonStyle(importButton);

}

void SelectDestinationDialog::SetPreviousDestinationDirectory()
{
    QDir gameRoot(Path::GetEditingGameDataFolder().c_str());
    QString gameRootAbsPath = gameRoot.absolutePath();
    QSettings settings;
    QString previousDestination = settings.value(g_selectDestinationFilesPath).toString();

    // Case 1: if currentDestination is empty at this point, that means this is the first time
    //         users using the Asset Importer, set the default directory to be the current game project's root folder
    // Case 2: if the current folder directory stored in the registry doesn't exist anymore,
    //         that means users have removed the directory already (deleted or use the Move feature).
    // Case 3: if it's a directory outside of the game root folder, then in general,
    //         users have modified the folder directory in the registry. It should not be happening.
    if (previousDestination.isEmpty() || !QDir(previousDestination).exists() || !previousDestination.startsWith(gameRootAbsPath, Qt::CaseInsensitive))
    {
        previousDestination = gameRootAbsPath;
    }

    m_ui->DestinationLineEdit->setText(QDir::toNativeSeparators(previousDestination));
}

void SelectDestinationDialog::accept()
{
    QDialog::accept();

    // This prevent users from not editing the destination line edit (manually type the directory or browse for the directory)
    Q_EMIT SetDestinationDiretory(DestinationDirectory());

    if (m_ui->CopyFileRadioButton->isChecked())
    {
        Q_EMIT DoCopyFiles();
    }
    else if (m_ui->MoveFileRadioButton->isChecked())
    {
        Q_EMIT DoMoveFiles();
    }
}

void SelectDestinationDialog::reject()
{
    Q_EMIT Cancel();
    QDialog::reject();
}

void SelectDestinationDialog::ShowMessage()
{
    QString message = m_ui->CopyFileRadioButton->isChecked() ? g_copyFilesMessage : g_moveFilesMessage;
    m_ui->RaidoButtonMessage->setText(message.arg(g_assetProcessorLink));
}

void SelectDestinationDialog::OnBrowseDestinationFilePath()
{
    Q_EMIT BrowseDestinationPath(m_ui->DestinationLineEdit);
}

void SelectDestinationDialog::UpdateMessage(QString message)
{
    m_ui->NumberOfFilesMessage->setText(message);
}

void SelectDestinationDialog::ValidatePath()
{
    if (!m_ui->DestinationLineEdit->hasAcceptableInput())
    {
        m_ui->DestinationLineEdit->setToolTip(tr(g_toolTip));
        Q_EMIT UpdateImportButtonState(false);
    }
    else
    {
        QString destinationDirectory = DestinationDirectory();
        int strLength = destinationDirectory.length();

        // store the updated acceptable destination directory into the registry,
        // so that when users manually modify the directory,
        // the Asset Importer will remember it
        Q_EMIT SetDestinationDiretory(destinationDirectory);

        m_ui->DestinationLineEdit->setToolTip("");
        Q_EMIT UpdateImportButtonState(strLength > 0);   
    } 
}

QString SelectDestinationDialog::DestinationDirectory() const
{
    return QDir::fromNativeSeparators(m_ui->DestinationLineEdit->text());
}

#include <AssetImporter/UI/SelectDestinationDialog.moc>

