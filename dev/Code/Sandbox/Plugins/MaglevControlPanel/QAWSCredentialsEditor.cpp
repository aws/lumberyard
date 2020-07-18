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
#include "stdafx.h"
#include <QAWSCredentialsEditor.h>

#include <AWSProfileModel.h>
#include <IAWSResourceManager.h>
#include <MaglevControlPanelPlugin.h>

#include "IEditor.h"

#include <AzQtComponents/Components/Style.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QScrollArea>
#include <QUrl>
#include <QMessageBox>

#include <Resource.h>

#include <QAWSCredentialsEditor.moc>
#include <aws/core/utils/memory/stl/AWSSet.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/FileSystemUtils.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <fstream>
#include <algorithm>

#include "AzCore/std/smart_ptr/make_shared.h"

const char* AWS_ACCESS_KEY = "aws_access_key_id";
const char* AWS_SECRET_KEY = "aws_secret_access_key";

const char* ProfileSelector::SELECTED_PROFILE_KEY = "AWS/selectedProfile";

static const char* ADD_PROFILE_STRING = "Add an AWS profile";
static const char* EDIT_PROFILE_STRING = "Edit an AWS profile";

static const char* SETTINGS_KEY_FIRST_AWS_PROFILE = "FirstAWSProfileEntered";
static const char* DEFAULT_LUMBERYARD_LABEL = "Lumberyard enables you to easily build games that utilize AWS services.  "
    "You are responsible for charges for AWS services and for adhering to the "
    "applicable service terms.  <a href=\"https://docs.aws.amazon.com/lumberyard/userguide/\" >Learn more.</a>";
static const char* NO_PROFILE_TEXT = "Add one or more AWS profiles to the editor to utilize AWS resources in your game.";

void QAWSProfileRow::SetupRow(QLabel& rowLabel, QAWSQTControls::QAWSProfileLineEdit& editControl)
{
    addWidget(&rowLabel, 1, Qt::AlignRight);
    addWidget(&editControl, 1, Qt::AlignRight);

    editControl.setFixedWidth(AddProfileDialog::profileRowWidth);

    setAlignment(Qt::AlignRight);
    setSizeConstraint(QLayout::SetFixedSize);
}

const char* AddProfileDialog::GetLumberyardText()
{
    return DEFAULT_LUMBERYARD_LABEL;
}

ProfileSelector::ProfileSelector(QWidget* parent)
    : ScrollSelectionDialog(parent)
    , m_model(GetIEditor()->GetAWSResourceManager()->GetProfileModel())
{
    InitializeWindow();
    setObjectName("ProfileSelector");
    connect(m_model.data(), &IAWSProfileModel::DeleteProfileFailed, this, &ProfileSelector::OnDeleteProfileFailed);
}


void ProfileSelector::SetupConnections()
{
    QWidget::connect(&*m_model, &IAWSProfileModel::modelReset, this, &ProfileSelector::OnModelReset);
}

int ProfileSelector::AddScrollColumnHeadings(QVBoxLayout* scrollLayout)
{
    static const int controlHeight = 1; // How many "rows" of controls are we adding (just 1 horizontal row atm)
    QHBoxLayout* headingsLayout = new QHBoxLayout();

    QLabel* profilesLabel = new QLabel("Profiles");
    headingsLayout->addWidget(profilesLabel, 0, Qt::AlignLeft);

    AddExtraScrollWidget(profilesLabel);

    QLabel* removeLabel = new QLabel("Remove");
    headingsLayout->addWidget(removeLabel, 1, Qt::AlignRight);

    AddExtraScrollWidget(removeLabel);

    scrollLayout->addLayout(headingsLayout);
    return controlHeight;
}

void ProfileSelector::AddScrollHeadings(QVBoxLayout* scrollLayout)
{
    QHBoxLayout* headingsLayout = new QHBoxLayout();

    m_editLabel = new QLabel("<a href=\"Edit selected profile\" > Edit selected profile</a>");
    m_editLabel->setObjectName("EditSelectedProfile");
    m_editLabel->setTextFormat(Qt::AutoText);
    m_editLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    headingsLayout->addWidget(m_editLabel, 1, Qt::AlignRight);
    QWidget::connect(m_editLabel, &QLabel::linkActivated, this, &ProfileSelector::EditProfile);
    if (!GetRowCount())
    {
        m_editLabel->hide();
    }
    scrollLayout->addLayout(headingsLayout);
}

void ProfileSelector::CreateNewProfile()
{
    m_addedProfile = OpenAddProfileDialog(this);
}

QString ProfileSelector::OpenAddProfileDialog(QWidget* parentWidget)
{
    AddProfileDialog* profileDialg = new AddProfileDialog(parentWidget, {});
    profileDialg->exec();
    return profileDialg->AddedProfile();
}

void ProfileSelector::EditProfile()
{
    QString currentProfile = GetSelectedButtonText();
    if (currentProfile.length())
    {
        OpenEditProfileDialog(currentProfile, this);
    }
}

void ProfileSelector::OpenEditProfileDialog(const QString& selectedText, QWidget* parentWidget)
{
    QDialog* profileDialg = new EditProfileDialog(parentWidget, selectedText);

    profileDialg->exec();
}

void ProfileSelector::SetCurrentSelection(QString& selectedText) const
{
    m_model->SetDefaultProfile(selectedText);
}

bool ProfileSelector::IsSelected(int rowNum) const
{
    return m_model->data(rowNum, AWSProfileColumn::Default).value<bool>();
}

QString ProfileSelector::GetDataForRow(int rowNum) const
{
    QString returnString;
    if (rowNum < GetRowCount())
    {
        returnString = m_model->data(rowNum, AWSProfileColumn::Name).value<QString>();
    }
    return returnString;
}

const char* ProfileSelector::GetNoDataText() const
{
    return NO_PROFILE_TEXT;
}

void ProfileSelector::OnBindData()
{
    if (GetRowCount())
    {
        m_editLabel->show();
    }
    else
    {
        m_editLabel->hide();
    }
}

void AddProfileDialog::SaveNewProfile()
{
    // If there are already profiles available OR if we have a default profile selected (indicating they created a profile in this project and deleted it)
    // we know this user has probably added a profile before.
    // TODO: Previously this also assumed that if a default profile had been set then this wasn't
    // the user's first AWS profile.
    if (!m_model->AWSCredentialsFileExists())
    {
        GetIEditor()->Notify(eNotify_OnFirstAWSUse);
    }

    if (m_profileNameEdit.text().simplified().isEmpty() || m_secretKeyEdit.text().simplified().isEmpty() || m_accessKeyEdit.text().simplified().isEmpty())
    {
        QMessageBox::critical(this, "Could not add profile", "A Profile must have a name, an AWS Access Key, and an AWS Secret Key");
    }
    else
    {
        m_model->AddProfile(m_profileNameEdit.text(), m_secretKeyEdit.text().simplified(), m_accessKeyEdit.text().simplified(), false);
    }
}

void AddProfileDialog::OnAddProfileSucceeded()
{
    GetIEditor()->Notify(eNotify_OnAddAWSProfile);
    m_addedProfile = m_profileNameEdit.text();
    ReturnToProfile();
}

void AddProfileDialog::OnAddProfileFailed(const QString& message)
{
    QMessageBox::critical(this, "Add Profile Failed", message);
}

void AddProfileDialog::OnUpdateProfileSucceeded()
{
    ReturnToProfile();
}

void AddProfileDialog::OnUpdateProfileFailed(const QString& message)
{
    QMessageBox::critical(this, "Update Profile Failed", message);
}

void AddProfileDialog::SaveCurrentProfile()
{
    int rowNum = GetEditingProfileRow();
    if (rowNum >= 0)
    {

        if (m_profileNameEdit.text().simplified().isEmpty() || m_secretKeyEdit.text().simplified().isEmpty() || m_accessKeyEdit.text().simplified().isEmpty())
        {
            QMessageBox::critical(this, "Could not save profile", "A Profile must have a name, an AWS Access Key, and an AWS Secret Key");
        }
        else
        {
            QString oldName = m_model->data(rowNum, AWSProfileColumn::Name).toString();
            QString newName = m_profileNameEdit.text();

            m_model->UpdateProfile(oldName, newName, m_secretKeyEdit.text(), m_accessKeyEdit.text());
        }

    }
}

int AddProfileDialog::GetEditingProfileRow()
{
    for (int rowNum = 0; rowNum < m_model->rowCount(); ++rowNum)
    {
        QString thisName = m_model->data(rowNum, AWSProfileColumn::Name).toString();
        if (thisName == m_editProfile)
        {
            return rowNum;
        }
    }
    return -1;
}

int ProfileSelector::GetRowCount() const
{
    return m_model->rowCount();
}

void ProfileSelector::ShowConfirmDeleteDialog(QString profileName)
{
    m_confirmDeleteDialog = new QDialog(this, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::Dialog | Qt::WindowCloseButtonHint);

    m_confirmDeleteDialog->setWindowTitle("Delete profile?");

    QString deleteStr = "Are you sure you want to delete the \"";
    deleteStr += profileName;
    deleteStr += "\" profile?";

    QVBoxLayout* dialogLayout = new QVBoxLayout;

    QLabel* deleteLabel = new QLabel;
    deleteLabel->setAlignment(Qt::AlignCenter);
    deleteLabel->setText(deleteStr);

    dialogLayout->addWidget(deleteLabel, 0, Qt::AlignCenter);

    QHBoxLayout* responseLayout = new QHBoxLayout;

    QPushButton* confirmButton = new QPushButton("Yes");
    QPushButton* declineButton = new QPushButton("No");

    QWidget::connect(confirmButton, &QPushButton::clicked, this, [this, profileName] { ConfirmDelete(profileName);
        });
    QWidget::connect(declineButton, &QPushButton::clicked, this, &ProfileSelector::HideConfirmDeleteDialog);

    responseLayout->addWidget(confirmButton, 0, Qt::AlignCenter);
    responseLayout->addWidget(declineButton, 0, Qt::AlignCenter);

    dialogLayout->addLayout(responseLayout);

    m_confirmDeleteDialog->setLayout(dialogLayout);

    m_confirmDeleteDialog->exec();
}

void ProfileSelector::ConfirmDelete(QString profileName)
{
    m_model->DeleteProfile(profileName);
    HideConfirmDeleteDialog();
}

void ProfileSelector::OnDeleteProfileFailed(const QString& message)
{
    QMessageBox::critical(this, "Delete Profile Failed", message);
}

void ProfileSelector::OnModelReset()
{
    if (m_addedProfile.isEmpty())
    {
        m_selectedButtonText = GetSelectedButtonText();
    }
    else
    {
        m_selectedButtonText = m_addedProfile;
        m_addedProfile.clear();
    }
    BindData();

}

void ProfileSelector::HideConfirmDeleteDialog()
{
    if (m_confirmDeleteDialog)
    {
        m_confirmDeleteDialog->close();
        m_confirmDeleteDialog = nullptr;
    }
}

void AddProfileDialog::OnCloudCanvasClicked()
{
    QDesktopServices::openUrl(QUrl(QString(MaglevControlPanelPlugin::GetHelpLink())));
}

void AddProfileDialog::OnLearnMoreClicked()
{
    QDesktopServices::openUrl(QUrl(QString(MaglevControlPanelPlugin::GetHelpLink())));
}

int ProfileSelector::AddLowerScrollControls(QVBoxLayout* scrollLayout)
{
    static const int controlHeight = 1; // How many "rows" of controls are we adding (just 1 horizontal row atm)

    QPushButton* addProfileButton = new QPushButton("Add profile");
    scrollLayout->addWidget(addProfileButton, 0, Qt::AlignLeft | Qt::AlignBottom);
    QWidget::connect(addProfileButton, &QPushButton::clicked, this, &ProfileSelector::CreateNewProfile);

    AddExtraScrollWidget(addProfileButton);

    return controlHeight;
}

void ProfileSelector::AddRow(int rowNum, QVBoxLayout* scrollLayout)
{
    QString profileName = m_model->data(rowNum, AWSProfileColumn::Name).toString();

    qDebug() << "profile name" << profileName;

    QRadioButton* newButton = new QRadioButton(profileName);
    qDebug() << "default" << m_model->data(rowNum, AWSProfileColumn::Default);
    if (m_selectedButtonText.isEmpty())
    {
        if (m_model->data(rowNum, AWSProfileColumn::Default).value<bool>())
        {
            newButton->setChecked(true);
        }
    }
    else
    {
        if (m_selectedButtonText == profileName)
        {
            newButton->setChecked(true);
        }
    }

    QHBoxLayout* buttonRowLayout = new QHBoxLayout();
    buttonRowLayout->addWidget(newButton, 0, Qt::AlignLeft);

    QPushButton* deleteButton = new QPushButton();
    deleteButton->setObjectName("DeleteProfile");
    deleteButton->setIcon(QIcon(":/stylesheet/img/UI20/titlebar-close.svg"));
    deleteButton->setIconSize(QSize(11, 11));
    AzQtComponents::Style::addClass(deleteButton, "flat");
    QString tooltipStr("Remove");
    if (profileName.length())
    {
        tooltipStr += " " + profileName;
    }
    deleteButton->setToolTip(tooltipStr);

    GetButtonGroup().addButton(newButton);
    AddExtraScrollWidget(deleteButton);

    buttonRowLayout->addWidget(deleteButton, 0, Qt::AlignRight);

    deleteButton->ensurePolished();

    QWidget::connect(deleteButton, &QPushButton::clicked, this, [this, profileName] { ShowConfirmDeleteDialog(profileName); });

    scrollLayout->addLayout(buttonRowLayout);
}

AddProfileDialog::AddProfileDialog(QWidget* parent)
    : AddProfileDialog(parent, {})
{
}

AddProfileDialog::AddProfileDialog(QWidget* parent, const QString& addEditProfile)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::Dialog | Qt::WindowCloseButtonHint)
    , m_model(GetIEditor()->GetAWSResourceManager()->GetProfileModel())
    , m_deleteCredentialsButton("Delete")
    , m_localProfileBox(ADD_PROFILE_STRING)
    , m_addProfileButton("Add profile")
    , m_editProfileButton("Edit profile")
    , m_profileNameLabel("Profile name")
    , m_accessKeyLabel("AWS access key")
    , m_secretKeyLabel("AWS secret key")
    , m_editProfile(addEditProfile)
{
    setWindowTitle(GetWindowName());

    m_lumberyardLabel.setText(GetLumberyardText());
    m_lumberyardLabel.setWordWrap(true);
    m_lumberyardLabel.setTextFormat(Qt::AutoText);
    m_lumberyardLabel.setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    connect(&m_lumberyardLabel, &QLabel::linkActivated, this, &AddProfileDialog::OnLearnMoreClicked);

    // SET UP ADD LOCAL PROFILE BOX

    m_nameRow.SetupRow(m_profileNameLabel, m_profileNameEdit);
    m_accessKeyRow.SetupRow(m_accessKeyLabel, m_accessKeyEdit);
    m_secretKeyRow.SetupRow(m_secretKeyLabel, m_secretKeyEdit);

    m_localProfileBoxLayout.addLayout(&m_nameRow);
    m_localProfileBoxLayout.addLayout(&m_accessKeyRow);
    m_localProfileBoxLayout.addLayout(&m_secretKeyRow);

    m_localProfileBoxLayout.addWidget(&m_lumberyardLabel);

    m_localProfileBox.setLayout(&m_localProfileBoxLayout);

    m_addLocalProfileLayout.addWidget(&m_localProfileBox);

    m_addLocalProfileLayout.setSizeConstraint(QLayout::SetFixedSize);

    QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);

    connect(pButtonBox, &QDialogButtonBox::accepted, this, &AddProfileDialog::OnSaveClicked);
    connect(pButtonBox, &QDialogButtonBox::rejected, this, &AddProfileDialog::OnCancelClicked);

    m_addLocalProfileLayout.addWidget(pButtonBox);

    setLayout(&m_addLocalProfileLayout);

    setFixedWidth(defaultWidth);
    setFixedHeight(defaultHeight);

    MoveCenter();

    connect(m_model.data(), &IAWSProfileModel::AddProfileSucceeded, this, &AddProfileDialog::OnAddProfileSucceeded);
    connect(m_model.data(), &IAWSProfileModel::AddProfileFailed, this, &AddProfileDialog::OnAddProfileFailed);
    connect(m_model.data(), &IAWSProfileModel::UpdateProfileSucceeded, this, &EditProfileDialog::OnUpdateProfileSucceeded);
    connect(m_model.data(), &IAWSProfileModel::UpdateProfileFailed, this, &EditProfileDialog::OnUpdateProfileFailed);


}

void AddProfileDialog::reject()
{
    ReturnToProfile();
    QDialog::reject();
}

AddProfileDialog::~AddProfileDialog()
{
}

void AddProfileDialog::Display()
{
    exec();
}

void AddProfileDialog::OpenProfileDialog()
{
    GetIEditor()->OpenWinWidget(ProfileSelector::GetWWId());
}

void AddProfileDialog::ReturnToProfile()
{
    // Must be careful with anything after CloseWindow here
    CloseWindow();
}

void AddProfileDialog::CloseWindow()
{
    disconnect(m_model.data(), 0, this, 0);
    setParent(nullptr);
    close();
}

void AddProfileDialog::OnCancelClicked()
{
    ReturnToProfile();
}

void AddProfileDialog::SetBoxTitle(const QString& newValue)
{
    m_localProfileBox.setTitle(newValue);
}

void AddProfileDialog::MoveCenter()
{
    QRect scr = QApplication::desktop()->screenGeometry(0);
    move(scr.center() - rect().center());
}

void AddProfileDialog::OnSaveClicked()
{
    SaveProfile();
}

void AddProfileDialog::SaveProfile()
{
    SaveNewProfile();
}

EditProfileDialog::EditProfileDialog(QWidget* parent, const QString& addEditProfile)
    : AddProfileDialog(parent, addEditProfile)
{
    setWindowTitle(GetWindowName());

    SetBoxTitle(EDIT_PROFILE_STRING);

    SetDefaultValues();
}

void EditProfileDialog::SetDefaultValues()
{
    int rowNum = GetEditingProfileRow();
    if (rowNum >= 0 && rowNum < m_model->rowCount())
    {
        m_profileNameEdit.setText(m_model->data(rowNum, AWSProfileColumn::Name).toString());
        m_accessKeyEdit.setText(m_model->data(rowNum, AWSProfileColumn::AccessKey).toString());
        m_secretKeyEdit.setText(m_model->data(rowNum, AWSProfileColumn::SecretKey).toString());
    }
}

void EditProfileDialog::SaveProfile()
{
    SaveCurrentProfile();
}
