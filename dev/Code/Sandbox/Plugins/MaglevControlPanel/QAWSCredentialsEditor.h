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
#pragma once
#include <QDialog>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QMainWindow>
#include <QSpinBox>
#include <QDataWidgetMapper>
#include <QList>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <RoleSelectionView.h>
#include <WinWidget/WinWidgetId.h>

class IAWSProfileModel;

/*
QAWSCredentialsEditor is a QT component that allows a user to edit their AWS credentials file on their local machine.
Known limitations:
Only accesskey and secret key are supported(IAM users). It does not currently support short-lived session credentials(session or federation credentials)
*/

namespace QAWSQTControls
{
    class QAWSProfileLineEdit
        : public QLineEdit
    {
        Q_OBJECT
    };
}

class QAWSProfileRow
    : public QHBoxLayout
{
public:
    void SetupRow(QLabel& rowLabel, QAWSQTControls::QAWSProfileLineEdit& editControl);
};

class ProfileSelector
    : public ScrollSelectionDialog
{
    Q_OBJECT

public:

    static const char* SELECTED_PROFILE_KEY;

    ProfileSelector(QWidget* parent = nullptr);
    virtual ~ProfileSelector() {}

    static const GUID& GetClassID()
    {
        // {B34951FA-5A30-40B4-
        // {1FFD849F-E3CA-477C-B611-FE70DC88E7EF}
        static const GUID guid =
        {
            0xb34951fa, 0x5a30, 0x40b4, { 0xb6, 0x11, 0xfe, 0x70, 0xdc, 0x88, 0xe7, 0xef }
        };
        return guid;
    }

    virtual void SetCurrentSelection(QString& selectedText) const;

    virtual int GetRowCount() const override;
    bool IsSelected(int rowNum) const override;
    virtual const char* GetWindowName() const override { return GetPaneName(); }

    static WinWidgetId GetWWId() { return WinWidgetId::PROFILE_SELECTOR; }

    static QString OpenAddProfileDialog(QWidget* parentWidget);
    static void OpenEditProfileDialog(const QString& selectedText, QWidget* parentWidget);

    static const char* GetPaneName() { return "Credentials Manager"; }
public slots:
    void CreateNewProfile();
    void EditProfile();

    void ShowConfirmDeleteDialog(QString profileName);
    void HideConfirmDeleteDialog();
    void ConfirmDelete(QString profileName);
    virtual void OnModelReset();
    void OnDeleteProfileFailed(const QString& message);

protected:

    virtual const char* GetNoDataText() const override;
    virtual void AddScrollHeadings(QVBoxLayout* scrollLayout) override;
    virtual int AddScrollColumnHeadings(QVBoxLayout* scrollLayout) override;
    void SetupConnections() override;
    virtual int AddLowerScrollControls(QVBoxLayout* scrollLayout) override;

    virtual QString GetDataForRow(int rowNum) const override;
    void AddRow(int rowNum, QVBoxLayout* scrollLayout) override;
    virtual void OnBindData() override;
private:

    QSharedPointer<IAWSProfileModel> m_model;
    QDialog* m_confirmDeleteDialog {
        nullptr
    };
    QLabel* m_editLabel {
        nullptr
    };

    QString m_addedProfile;
    QString m_selectedButtonText;

};

class AddProfileDialog
    : public QDialog
{
    Q_OBJECT
public:
    static const int defaultWidth = 335;
    static const int defaultHeight = 210;
    static const int windowBuffer = 20;
    static const int profileRowWidth = 200;

    AddProfileDialog(QWidget* parent);
    AddProfileDialog(QWidget* parent, const QString& editProfile);
    virtual ~AddProfileDialog();

    void reject() override;

    virtual const char* GetWindowName() const { return "Add Profile"; }

    void Display();
    static WinWidgetId GetWWId() { return WinWidgetId::ADD_PROFILE; }

    static const GUID& GetClassID()
    {
        static const GUID guid =
        {
            0xdf262dae, 0xd34c, 0x470e, { 0x81, 0x3d, 0x8e, 0x2b, 0x8b, 0x57, 0x22, 0x6d }
        };
        return guid;
    }

    const QString& AddedProfile() 
    {
        return m_addedProfile;
    }

public slots:

    void OnCloudCanvasClicked();
    void OnLearnMoreClicked();

    void OnCancelClicked();
    void OnSaveClicked();

protected:
    virtual void SaveProfile();
    // Edit the current profile to the new values
    void SaveCurrentProfile();
    //Add a new profile to the model(and dropdown box)
    void SaveNewProfile();
    void OpenProfileDialog();
    void ReturnToProfile();
    int GetEditingProfileRow();
    void CloseWindow();
    void MoveCenter();
    void OnAddProfileSucceeded();
    void OnAddProfileFailed(const QString& message);
    void OnUpdateProfileSucceeded();
    void OnUpdateProfileFailed(const QString& message);

    virtual void SetDefaultValues() {}

    void SetBoxTitle(const QString& newValue);

    QSharedPointer<IAWSProfileModel> m_model;

    QAWSQTControls::QAWSProfileLineEdit m_profileNameEdit;
    QAWSQTControls::QAWSProfileLineEdit m_accessKeyEdit;
    QAWSQTControls::QAWSProfileLineEdit m_secretKeyEdit;
private:

    const char* GetLumberyardText();


    QVBoxLayout m_addLocalProfileLayout;
    QPushButton m_deleteCredentialsButton;
    QGroupBox m_localProfileBox;
    QVBoxLayout m_localProfileBoxLayout;
    QAWSProfileRow m_nameRow;
    QAWSProfileRow m_accessKeyRow;
    QAWSProfileRow m_secretKeyRow;
    QPushButton m_addProfileButton;
    QPushButton m_editProfileButton;
    QHBoxLayout m_addEditLayout;

    QLabel m_lumberyardLabel;
    QLabel m_profileNameLabel;
    QLabel m_accessKeyLabel;
    QLabel m_secretKeyLabel;

    QString m_editProfile;

    QString m_addedProfile;

};

class EditProfileDialog
    : public AddProfileDialog
{
public:
    EditProfileDialog(QWidget* parent, const QString& addEditProfile);
    virtual ~EditProfileDialog() {}

    virtual const char* GetWindowName() const override { return "Edit Profile"; }

    static const GUID& GetClassID()
    {
        // {1E5191CE-8226-4EBF-9FF0-8DF9CE0362E2}
        static const GUID guid =
        {
            0x1e5191ce, 0x8226, 0x4ebf, { 0x9f, 0xf0, 0x8d, 0xf9, 0xce, 0x3, 0x62, 0xe2 }
        };
        return guid;
    }

protected:
    virtual void SetDefaultValues() override;
    virtual void SaveProfile() override;
};