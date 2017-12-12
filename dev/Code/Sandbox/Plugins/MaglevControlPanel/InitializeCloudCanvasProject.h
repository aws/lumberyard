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
#include <QMainWindow>

#include <WinWidget/WinWidgetId.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>

class IAWSProfileModel;
class QVBoxLayout;
class QFrame;
class QRadioButton;
class QPushButton;
class QCheckBox;

class InitializeCloudCanvasProject
    : public QMainWindow
{
    Q_OBJECT
public:
    static const int windowBuffer = 20;
    static const int defaultWidth = 400;
    static const int defaultWindowWidth = defaultWidth + windowBuffer;
    static WinWidgetId GetWWId() { return WinWidgetId::INITIALIZE_PROJECT; }

    InitializeCloudCanvasProject(QWidget* parent = 0);
    virtual ~InitializeCloudCanvasProject();

    static const char* GetWindowTitle() { return "Initialize Cloud Canvas Resource Manager"; }

    static const GUID& GetClassID()
    {
        // {86C1A74B-37A5-4083-A262-614E043C3F34}
        static const GUID guid =
        {
            0x86c1a74b, 0x37a5, 0x4083, { 0xa2, 0x62, 0x61, 0x4e, 0x4, 0x3c, 0x3f, 0x34 }
        };

        return guid;
    }

    void Display();
public slots:

    // Button controls
    void OnAddProfileClicked();
    void OnEditClicked();
    void OnCancelClicked();
    void OnCreateClicked();
    void OnLearnMoreClicked();
    void SourceChangedAttemptCreate();

    void SourceUpdatedInitializeProject(const QString& regionName, const QString& projectName, const QString& accessKey, const QString& secretKey);
    void SourceChangedInitializeProject(const QString& regionName, const QString& projectName, const QString& accessKey, const QString& secretKey);

private:

    bool HasProfile() const;

    // We may be initializing a profile on the fly while hitting create - if so we need to save and switch to it
    bool InitializeProfileOnCreate(QString& accessKey, QString& secretKey);

    void AddHorizontalLabelEdit(QVBoxLayout* verticalLayout, const char* labelText, const char* editName, const char* defaultText = nullptr);
    void AddDeploymentRegionBox(QVBoxLayout* verticalLayout);
    void AddProfileSelectionLayout(QVBoxLayout* verticalLayout);
    void AddProjectDetails(QVBoxLayout* verticalLayout);
    void AddAddProfileEditButtons(QVBoxLayout* verticalLayout);
    void AddAffirmReview(QVBoxLayout* verticalLayout);
    void AddCreateCancelButtons(QVBoxLayout* verticalLayout);

    void AddProjectDetailsWithProfile(QFrame* groupBox);
    void AddProjectDetailsNoProfile(QFrame* groupBox);

    bool ValidateProjectName() const;
    bool ValidateProjectNameLength(const QString& projectName) const;
    bool ValidateProjectNameFormat(const QString& projectName) const;

    // Full window control initialization
    void InitializeWindow();

    void SwitchToProfile(QString selectedProfileName);

    // We're entering a profile that didn't previously exist - we need to save that as a new profile
    void SaveNewProfile(const QString& profileName, const QString& accessKey, const QString& secretKey);

    // Decide based on state whether the create button should be enabled or not
    void UpdateCreateButtonStatus();

    QPushButton* GetCreateButton() const;

    QString GetSelectedDeploymentRegion() const;
    QString GetEditControlText(const char* controlText) const;

    void ValidateSourceInitializeProject(const QString& regionName, const QString& projectName, const QString& accessKey, const QString& secretKey);
    void DoInitializeProject(const QString& regionName, const QString& projectName, const QString& accessKey, const QString& secretKey);

    QSharedPointer<IAWSProfileModel> m_model;
    AZStd::vector<QRadioButton*> m_profileSelectorButtons;

    QMetaObject::Connection m_conn;
};