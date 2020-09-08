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
#include <InitializeCloudCanvasProject.h>
#include <AWSProfileModel.h>
#include <IAWSResourceManager.h>
#include <QAWSCredentialsEditor.h>
#include <ResourceManagementView.h>
#include <MaglevControlPanelPlugin.h>
#include <IFileSourceControlModel.h>

#include "IEditor.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRegExp>
#include <QLineEdit>
#include <QUrl>
#include <QVBoxLayout>

#include <AzQtComponents/Components/StyleManager.h>

#include <Util/PathUtil.h>

#include <InitializeCloudCanvasProject.moc>

static const char* PROJECT_ADMIN_LABEL = "You must be an AWS admin to initialize your Cloud Canvas project.";
static const char* PROJECT_DETAILS_IAM_CREDENTIALS_LABEL =
    "You must provide IAM credentials for an AWS account admin. "
    "Click <a style='color:#4285F4; text-decoration:none' href=\"http://docs.aws.amazon.com/lumberyard/latest/developerguide/cloud-canvas-tutorial.html\">here</a> "
    "for instructions on locating existing credentials or setting up an AWS account admin.";

static const char* AFFIRM_REVIEW_BUTTON_LABEL = "Cloud Canvas will deploy AWS resources to your account using your CloudFormation templates.  There is no additional charge for Cloud Canvas or CloudFormation.  You pay for AWS resources created using Cloud Canvas and CloudFormation in the same manner as if you created them manually.  You only pay for what you use, as you use it; there are no minimum fees and no required upfront commitments, and most services include a free tier.  <a style='color:#4285F4;' href='https ://docs.aws.amazon.com/lumberyard/userguide/cloud-canvas'>Learn more</a>";

static const char* ADMIN_ROLES_REVIEW_LABEL = "Create the optional ProjectAdmin and ProjectOwner roles, which could impact security. <a style='color:#4285F4;' href='https://docs.aws.amazon.com/lumberyard/latest/userguide/cloud-canvas-built-in-roles-and-policies.html'>Learn more</a>.";


static const char* CREATE_BUTTON = "Create Button";
static const char* DEPLOYMENT_SELECTOR = "Deployment region";
static const char* ADMIN_ROLES_CHECKBOX = "Admin Checkbox";
static const char* PROJECT_NAME_EDIT = "Project name edit";
static const char* PROFILE_NAME_EDIT = "Profile name edit";
static const char* ACCESS_KEY_EDIT = "Access key edit";
static const char* SECRET_KEY_EDIT = "Secret key edit";

// Constants for creating edit controls
static const int windowMaxHeight = 480;
static const int boxHeight = 24;
static const int boxWidth = (InitializeCloudCanvasProject::defaultWidth * 2 / 3) - 15;
static const int radioWidth = boxWidth;

InitializeCloudCanvasProject::InitializeCloudCanvasProject(QWidget* parent)
    : QMainWindow(parent)
    , m_model(GetIEditor()->GetAWSResourceManager()->GetProfileModel())
{
    setMaximumHeight(windowMaxHeight);
    AzQtComponents::StyleManager::setStyleSheet(this, "style:CloudCanvas.qss");
    InitializeWindow();
}

InitializeCloudCanvasProject::~InitializeCloudCanvasProject()
{
}

// Full redraw call - setCentralWidget is used at the end so this can be called again while the window is up
// to rebuild everything and destroy previous QT controls
void InitializeCloudCanvasProject::InitializeWindow()
{

    setObjectName("InitializeCloudCanvasProject");

    setWindowTitle(QString(GetWindowTitle()));
    setFixedWidth(defaultWidth);

    // Our overall top to bottom page layout
    QVBoxLayout* verticalLayout = new QVBoxLayout;

    // Switch to decide between profile selection or creation is done in here
    AddProjectDetails(verticalLayout);

    AddAffirmReview(verticalLayout);

    AddCreateCancelButtons(verticalLayout);

    QWidget* centralWidget = new QWidget;
    centralWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    centralWidget->setObjectName("Content");
    centralWidget->setLayout(verticalLayout);

    setCentralWidget(centralWidget);

    UpdateCreateButtonStatus();

}

bool InitializeCloudCanvasProject::HasProfile() const
{
    return (m_model->rowCount(QModelIndex()) != 0);
}

void InitializeCloudCanvasProject::AddProjectDetails(QVBoxLayout* verticalLayout)
{
    if (!verticalLayout)
    {
        return;
    }

    QFrame* groupBox = new QFrame();
    groupBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    groupBox->setObjectName("ContentFrame");

    // Two varieties of this control based on whether profiles exist or need to be created
    if (HasProfile())
    {
        AddProjectDetailsWithProfile(groupBox);
    }
    else
    {
        AddProjectDetailsNoProfile(groupBox);
    }
    verticalLayout->addWidget(groupBox);

}

// We have several types of horizontal combo Label/Edit controls to add which pass through here for consistency
void InitializeCloudCanvasProject::AddHorizontalLabelEdit(QVBoxLayout* verticalLayout, const char* labelText, const char* editName, const char* editDefault)
{
    QHBoxLayout* horizontalLayout = new QHBoxLayout;
    QLabel* nameLabel = new QLabel(labelText);
    horizontalLayout->addWidget(nameLabel, 0, Qt::AlignRight);

    QLineEdit* editControl = new QLineEdit;
    if (editDefault)
    {
        editControl->setText(QString(editDefault));
    }

    editControl->setFixedSize(boxWidth, boxHeight);
    editControl->setObjectName(editName);

    horizontalLayout->addWidget(editControl, 0, Qt::AlignRight);
    horizontalLayout->setSizeConstraint(QLayout::SetFixedSize);

    verticalLayout->addLayout(horizontalLayout);
}

void InitializeCloudCanvasProject::AddDeploymentRegionBox(QVBoxLayout* verticalLayout)
{
    QHBoxLayout* deploymentRegionLayout = new QHBoxLayout;
    QLabel* deploymentRegionLabel = new QLabel("AWS region:");
    deploymentRegionLabel->setMinimumWidth(100);
    deploymentRegionLabel->setAlignment(Qt::AlignRight);

    deploymentRegionLayout->addWidget(deploymentRegionLabel, 0, Qt::AlignRight);

    QComboBox* deploymentRegionBox = new QComboBox;
    deploymentRegionBox->setFixedSize(boxWidth, boxHeight);
    deploymentRegionBox->setEditable(true);
    deploymentRegionBox->setToolTip("This list contains the supported regions for our core services including CloudFormation, Cognito, DynamoDB, Kinesis, Lambda, S3, SNS, SQS and STS. You can also edit the region according to the services you need.");

    GetIEditor()->GetAWSResourceManager()->GetRegionList();

    disconnect(m_conn);
    m_conn = connect(GetIEditor()->GetAWSResourceManager(), &AWSResourceManager::SupportedRegionsObtained, this, [deploymentRegionBox](QStringList regionList) {
        deploymentRegionBox->clear();
        deploymentRegionBox->addItems(regionList);
        deploymentRegionBox->setCurrentIndex(regionList.indexOf("us-east-1"));
    });

    deploymentRegionBox->setObjectName(DEPLOYMENT_SELECTOR);

    deploymentRegionLayout->addWidget(deploymentRegionBox, 0, Qt::AlignRight);
    deploymentRegionLayout->setAlignment(Qt::AlignRight);
    deploymentRegionLayout->setSizeConstraint(QLayout::SetFixedSize);

    verticalLayout->addLayout(deploymentRegionLayout);
}

void InitializeCloudCanvasProject::AddProjectDetailsWithProfile(QFrame* groupBox)
{
    if (!groupBox)
    {
        return;
    }

    QVBoxLayout* boxLayout = new QVBoxLayout;
    QLabel* credsLabel = new QLabel(PROJECT_ADMIN_LABEL);
    credsLabel->setObjectName("TopLabel");
    credsLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    credsLabel->setWordWrap(true);
    boxLayout->addWidget(credsLabel, 0);

    // Default populate our project edit box from CVAR if it exists
    ICVar* gameFolderVar = gEnv->pConsole->GetCVar("sys_game_folder");
    AddHorizontalLabelEdit(boxLayout, "Project stack name: ", PROJECT_NAME_EDIT, gameFolderVar ? gameFolderVar->GetString() : "");

    AddProfileSelectionLayout(boxLayout);

    AddAddProfileEditButtons(boxLayout);

    AddDeploymentRegionBox(boxLayout);

    AddAdminRoleControl(boxLayout);

    groupBox->setLayout(boxLayout);

}

void InitializeCloudCanvasProject::AddAddProfileEditButtons(QVBoxLayout* boxLayout)
{
    QHBoxLayout* addEditLayout = new QHBoxLayout;
    addEditLayout->setContentsMargins(0, 0, 0, 0);

    QPushButton* addProfile = new QPushButton("Add profile");
    addProfile->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    addEditLayout->addWidget(addProfile);
    QWidget::connect(addProfile, &QPushButton::clicked, this, &InitializeCloudCanvasProject::OnAddProfileClicked);

    QPushButton* editProfile = new QPushButton("Edit");
    editProfile->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    addEditLayout->addWidget(editProfile);
    QWidget::connect(editProfile, &QPushButton::clicked, this, &InitializeCloudCanvasProject::OnEditClicked);

    addEditLayout->addStretch();

    QWidget* buttonWidget = new QWidget;
    buttonWidget->setObjectName("ProfileEditButtons");
    buttonWidget->setLayout(addEditLayout);
    buttonWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    buttonWidget->setFixedWidth(boxWidth);
    boxLayout->addWidget(buttonWidget, 0, Qt::AlignRight);

}

void InitializeCloudCanvasProject::AddProfileSelectionLayout(QVBoxLayout* boxLayout)
{
    QHBoxLayout* profileSelectionLayout = new QHBoxLayout;
    QLabel* profileSelectLabel = new QLabel("AWS profile:");
    profileSelectionLayout->addWidget(profileSelectLabel, 0, Qt::AlignRight | Qt::AlignTop);

    QVBoxLayout* radioLayout = new QVBoxLayout;
    radioLayout->setContentsMargins(0, 0, 0, 0);
    m_profileSelectorButtons.clear();

    bool foundProfile = false;
    for (int row = 0; row < m_model->rowCount(); ++row)
    {
        QString profileName = m_model->data(row, AWSProfileColumn::Name).toString();
        QRadioButton* newButton = new QRadioButton(profileName);

        if (!foundProfile && m_model->data(row, AWSProfileColumn::Default).value<bool>())
        {
            newButton->setChecked(true);
            foundProfile = true;
        }

        QWidget::connect(newButton, &QRadioButton::clicked, this, [this, profileName] { SwitchToProfile(profileName);});

        radioLayout->addWidget(newButton);
        m_profileSelectorButtons.push_back(newButton);
    }

    if (!foundProfile && m_profileSelectorButtons.size())
    {
        QRadioButton* firstButton = m_profileSelectorButtons.front();
        if (firstButton)
        {
            firstButton->setChecked(true);
            m_model->SetDefaultProfile(firstButton->text());
        }
    }

    radioLayout->addStretch();

    QWidget* radioWidget = new QWidget;
    radioWidget->setObjectName("Profiles");
    radioWidget->setLayout(radioLayout);
    radioWidget->setFixedWidth(radioWidth);
    radioWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    profileSelectionLayout->addWidget(radioWidget);

    QWidget* profileSelectWidget = new QWidget;
    profileSelectWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    profileSelectWidget->setLayout(profileSelectionLayout);

    boxLayout->addWidget(profileSelectWidget);

}

void InitializeCloudCanvasProject::AddProjectDetailsNoProfile(QFrame* groupBox)
{
    if (!groupBox)
    {
        return;
    }

    QVBoxLayout* boxLayout = new QVBoxLayout;
    QLabel* credsLabel = new QLabel(PROJECT_DETAILS_IAM_CREDENTIALS_LABEL);
    credsLabel->setObjectName("TopLabel");
    credsLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    credsLabel->setWordWrap(true);
    boxLayout->addWidget(credsLabel, 0);

    // Default populate our project edit box from CVAR if it exists
    ICVar* gameFolderVar = gEnv->pConsole->GetCVar("sys_game_folder");
    AddHorizontalLabelEdit(boxLayout, "Project stack name:", PROJECT_NAME_EDIT, gameFolderVar ? gameFolderVar->GetString() : "");

    AddHorizontalLabelEdit(boxLayout, "AWS Profile name:", PROFILE_NAME_EDIT, "CloudCanvas");

    AddHorizontalLabelEdit(boxLayout, "AWS Access Key:", ACCESS_KEY_EDIT);

    AddHorizontalLabelEdit(boxLayout, "AWS Secret Key:", SECRET_KEY_EDIT);

    AddDeploymentRegionBox(boxLayout);

    AddAdminRoleControl(boxLayout);

    groupBox->setLayout(boxLayout);

}


void InitializeCloudCanvasProject::AddAffirmReview(QVBoxLayout* verticalLayout)
{
    if (!verticalLayout)
    {
        return;
    }

    QLabel* affirmText = new QLabel(AFFIRM_REVIEW_BUTTON_LABEL);
    affirmText->setObjectName("BottomLabel");
    QWidget::connect(affirmText, &QLabel::linkActivated, this, &InitializeCloudCanvasProject::OnLearnMoreClicked);
    affirmText->setWordWrap(true);

    verticalLayout->addWidget(affirmText);
}

void InitializeCloudCanvasProject::AddAdminRoleControl(QVBoxLayout* verticalLayout)
{
    static const char* detailWidgetBackgroundColor = "#303030";

    QFrame* adminRoleBox = new QFrame();
    adminRoleBox->setStyleSheet("border:0;");
    adminRoleBox->setStyleSheet(QString("background-color:%1;").arg(detailWidgetBackgroundColor));

    QHBoxLayout* adminLayout = new QHBoxLayout;
    adminLayout->setSpacing(3);

    QCheckBox* rolecheckbox = new QCheckBox();
    rolecheckbox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    rolecheckbox->setChecked(false);
    rolecheckbox->setObjectName(ADMIN_ROLES_CHECKBOX);

    adminLayout->addWidget(rolecheckbox, Qt::AlignLeft);

    QLabel* adminText = new QLabel(ADMIN_ROLES_REVIEW_LABEL);
    adminText->setObjectName("ConfirmLabel");
    QWidget::connect(adminText, &QLabel::linkActivated, this, &InitializeCloudCanvasProject::OnProjectAdminLearnMoreClicked);
    adminText->setWordWrap(true);
    adminLayout->addWidget(adminText, Qt::AlignCenter);

    adminRoleBox->setLayout(adminLayout);
    verticalLayout->addWidget(adminRoleBox, 0);
}

bool InitializeCloudCanvasProject::GetCreateAdminRoleStatus() const
{
    QCheckBox* adminBox = centralWidget()->findChild<QCheckBox*>(QString(ADMIN_ROLES_CHECKBOX), Qt::FindChildrenRecursively);
    if (adminBox)
    {
        return adminBox->isChecked();
    }
    return false;
}

QPushButton* InitializeCloudCanvasProject::GetCreateButton() const
{
    return centralWidget()->findChild<QPushButton*>(QString(CREATE_BUTTON), Qt::FindChildrenRecursively);
}

QString InitializeCloudCanvasProject::GetSelectedDeploymentRegion() const
{
    QComboBox* regionBox = centralWidget()->findChild<QComboBox*>(QString(DEPLOYMENT_SELECTOR), Qt::FindChildrenRecursively);
    if (regionBox)
    {
        return regionBox->currentText();
    }
    return QString {};
}


QString InitializeCloudCanvasProject::GetEditControlText(const char* controlName) const
{
    QLineEdit* editBox = centralWidget()->findChild<QLineEdit*>(QString(controlName), Qt::FindChildrenRecursively);
    if (editBox)
    {
        return editBox->text();
    }
    return QString {};
}

void InitializeCloudCanvasProject::SwitchToProfile(QString profileName)
{
    m_model->SetDefaultProfile(profileName);
}

void InitializeCloudCanvasProject::Display()
{
    show();
}

void InitializeCloudCanvasProject::AddCreateCancelButtons(QVBoxLayout* verticalLayout)
{
    if (!verticalLayout)
    {
        return;
    }

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    QPushButton* cancelButton = new QPushButton("Cancel");
    buttonLayout->addWidget(cancelButton, 0, Qt::AlignRight);
    QWidget::connect(cancelButton, &QPushButton::clicked, this, &InitializeCloudCanvasProject::OnCancelClicked);

    QPushButton* createButton = new QPushButton("Create");
    createButton->setProperty("class", "Primary");
    createButton->setObjectName(CREATE_BUTTON);

    buttonLayout->addWidget(createButton, 0, Qt::AlignRight);
    QWidget::connect(createButton, &QPushButton::clicked, this, &InitializeCloudCanvasProject::OnCreateClicked);

    verticalLayout->addLayout(buttonLayout, 0);
}

// Button controls
void InitializeCloudCanvasProject::OnAddProfileClicked()
{
    ProfileSelector::OpenAddProfileDialog(this);
}

void InitializeCloudCanvasProject::OnEditClicked()
{
    ProfileSelector::OpenEditProfileDialog(m_model->GetDefaultProfile(), this);
}

void InitializeCloudCanvasProject::OnCancelClicked()
{
    close();
}

void InitializeCloudCanvasProject::OnProjectAdminLearnMoreClicked()
{
    QDesktopServices::openUrl(QUrl(QString(MaglevControlPanelPlugin::GetRolesHelpLink())));
}

void InitializeCloudCanvasProject::SaveNewProfile(const QString& profileName, const QString& accessKey, const QString& secretKey)
{
    if (!profileName.length())
    {
        return;
    }

    // This currently replicates functionality of the Permissions and deployments window
    bool hasAddedProfileBefore = m_model->AWSCredentialsFileExists();

    m_model->AddProfile(profileName, secretKey, accessKey, true);

    GetIEditor()->Notify(eNotify_OnAddAWSProfile);

    if (!hasAddedProfileBefore)
    {
        GetIEditor()->Notify(eNotify_OnFirstAWSUse);
    }
}

void InitializeCloudCanvasProject::SourceChangedAttemptCreate()
{
    QWidget::disconnect(&*GetIEditor()->GetAWSResourceManager()->GetProjectSettingsSourceModel(), &IFileSourceControlModel::SourceControlStatusChanged, this, &InitializeCloudCanvasProject::OnCreateClicked);
    OnCreateClicked();
}

void InitializeCloudCanvasProject::OnCreateClicked()
{
    if (!ValidateProjectName())
    {
        return;
    }

    // accessKey and secretKey will be empty if an existing default profile
    // is being used. However, since new (default) profile are created async,
    // if that happens we provide the accessKey and secretKey to directly to
    // the initialize project command.

    QString accessKey;
    QString secretKey;

    if (!InitializeProfileOnCreate(accessKey, secretKey))
    {
        return;
    }

    QString region = GetSelectedDeploymentRegion();
    QString projectName = GetEditControlText(PROJECT_NAME_EDIT);
    bool createAdminRoles = GetCreateAdminRoleStatus();

    ValidateSourceInitializeProject(region, projectName, accessKey, secretKey, createAdminRoles);
}

void InitializeCloudCanvasProject::ValidateSourceInitializeProject(const QString& regionName, const QString& projectName, const QString& accessKey, const QString& secretKey, bool createAdminRoles)
{
    QObject::connect(&*GetIEditor()->GetAWSResourceManager()->GetProjectSettingsSourceModel(), &IFileSourceControlModel::SourceControlStatusUpdated, this, [this, regionName, projectName, accessKey, secretKey, createAdminRoles]() {
        SourceUpdatedInitializeProject(regionName, projectName, accessKey, secretKey, createAdminRoles);
    });
    GetIEditor()->GetAWSResourceManager()->UpdateSourceControlStates();
}

void InitializeCloudCanvasProject::SourceUpdatedInitializeProject(const QString& regionName, const QString& projectName, const QString& accessKey, const QString& secretKey, bool createAdminRoles)
{
    QObject::disconnect(&*GetIEditor()->GetAWSResourceManager()->GetProjectSettingsSourceModel(), &IFileSourceControlModel::SourceControlStatusUpdated, this, 0);

    if (GetIEditor()->GetAWSResourceManager()->ProjectSettingsNeedsCheckout())
    {
        QString fileName = GetIEditor()->GetAWSResourceManager()->GetProjectModel()->GetProjectSettingsFile();
        QString checkoutString = fileName + ResourceManagementView::GetSourceCheckoutString();
        auto reply = QMessageBox::question(
                this,
                "File needs checkout",
                checkoutString,
                QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            QObject::connect(&*GetIEditor()->GetAWSResourceManager()->GetProjectSettingsSourceModel(), &IFileSourceControlModel::SourceControlStatusChanged, this, [this, regionName, projectName, accessKey, secretKey, createAdminRoles]() {
                SourceChangedInitializeProject(regionName, projectName, accessKey, secretKey, createAdminRoles);
            });
            GetIEditor()->GetAWSResourceManager()->RequestEditProjectSettings();
        }
        return;
    }
    DoInitializeProject(regionName, projectName, accessKey, secretKey, createAdminRoles);
}

void InitializeCloudCanvasProject::DoInitializeProject(const QString& regionName, const QString& projectName, const QString& accessKey, const QString& secretKey, bool createAdminRoles)
{
    GetIEditor()->GetAWSResourceManager()->InitializeProject(regionName, projectName, accessKey, secretKey, createAdminRoles);
    close();
}

void InitializeCloudCanvasProject::SourceChangedInitializeProject(const QString& regionName, const QString& projectName, const QString& accessKey, const QString& secretKey, bool createAdminRoles)
{
    QObject::disconnect(&*GetIEditor()->GetAWSResourceManager()->GetProjectSettingsSourceModel(), &IFileSourceControlModel::SourceControlStatusUpdated, this, 0);
    QObject::disconnect(&*GetIEditor()->GetAWSResourceManager()->GetProjectSettingsSourceModel(), &IFileSourceControlModel::SourceControlStatusChanged, this, 0);
    if (!GetIEditor()->GetAWSResourceManager()->ProjectSettingsNeedsCheckout())
    {
        DoInitializeProject(regionName, projectName, accessKey, secretKey, createAdminRoles);
    }
}

bool InitializeCloudCanvasProject::InitializeProfileOnCreate(QString& accessKey, QString& secretKey)
{
    if (!HasProfile())
    {
        QString profileName = GetEditControlText(PROFILE_NAME_EDIT);
        accessKey = GetEditControlText(ACCESS_KEY_EDIT).simplified();
        secretKey = GetEditControlText(SECRET_KEY_EDIT).simplified();

        if (!profileName.length() || !accessKey.length() || !secretKey.length())
        {
            QMessageBox box(QMessageBox::NoIcon,
                "Invalid AWS credentials",
                "Please provide a profile name, access key, and secret key.",
                QMessageBox::Ok,
                Q_NULLPTR,
                Qt::Popup);
            box.exec();
            return false;
        }

        SaveNewProfile(profileName, accessKey, secretKey);
    }

    return true;
}

bool InitializeCloudCanvasProject::ValidateProjectNameLength(const QString& projectName) const
{
    if (!ResourceManagementView::ValidateStackNameLength(projectName))
    {
        QString outputStr("Project stack name too long (");
        outputStr += QString::number(projectName.length()) + ") limit " + QString::number(ResourceManagementView::maxStackNameLength) + " characters";
        ResourceManagementView::ShowBasicErrorBox("Invalid project stack name length", outputStr);
        return false;
    }
    return true;
}

bool InitializeCloudCanvasProject::ValidateProjectNameFormat(const QString& projectName) const
{
    if (!ResourceManagementView::ValidateStackNameFormat(projectName))
    {
        ResourceManagementView::ShowBasicErrorBox(
            "Invalid project stack name",
            "Please provide a project stack name that starts with a letter followed by alphanumeric characters or hyphens");
        return false;
    }
    return true;
}

bool InitializeCloudCanvasProject::ValidateProjectName() const
{
    QString projectName = GetEditControlText(PROJECT_NAME_EDIT);

    if (!ValidateProjectNameLength(projectName))
    {
        return false;
    }

    if (!ValidateProjectNameFormat(projectName))
    {
        return false;
    }
    return true;
}

void InitializeCloudCanvasProject::OnLearnMoreClicked()
{
    QDesktopServices::openUrl(QUrl(QString(MaglevControlPanelPlugin::GetHelpLink())));
}

void InitializeCloudCanvasProject::UpdateCreateButtonStatus()
{
    QPushButton* createButton = GetCreateButton();

    if (createButton)
    {
        bool enabledState = true;

        // Enable this restriction when the hand off from CREATING to FAILED or SUCCESS works properly - Currently it will stop you from attempting again on fails incorrectly
        // enabledState &= (GetInitializationState() == InitializeProjectState::DEFAULT || GetInitializationState() == InitializeProjectState::FAILED);

        createButton->setDisabled(enabledState == 0);
    }
}
