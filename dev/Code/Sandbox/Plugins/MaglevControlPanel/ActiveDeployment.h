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

#include <RoleSelectionView.h>

#include <QString>
#include <QSharedPointer>
#include <WinWidget/WinWidgetId.h>

class IAWSDeploymentModel;

class ActiveDeployment
    : public ScrollSelectionDialog
{
    Q_OBJECT

public:
    ActiveDeployment(QWidget* parent = nullptr);
    virtual ~ActiveDeployment() {}

    static const GUID& GetClassID()
    {
        static const GUID guid =
        {
            0x17caa679, 0xbc5a, 0x4812, { 0x83, 0x5, 0x43, 0xa0, 0x6f, 0xb4, 0xa7, 0x73 }
        };
        return guid;
    }

    static const char* GetPaneName() { return "Select deployment"; }
    virtual const char* GetWindowName() const override { return GetPaneName(); }

    static WinWidgetId GetWWId() { return WinWidgetId::ACTIVE_DEPLOYMENT; }

    virtual void SetCurrentSelection(QString& selectedText) const override;

    virtual int GetRowCount() const override;
    bool IsSelected(int rowNum) const override;

public slots:

    void OnSetupDeploymentClicked();

protected:


    virtual void AddScrollHeadings(QVBoxLayout* boxLayout) override;
    virtual void AddScrollRow(QVBoxLayout*) override {}
    virtual void SetupConnections();
    virtual const char* GetNoDataText() const override;
    virtual const char* GetHasDataText() const override;
    virtual void SetupNoDataConnection() override;

    virtual QString GetDataForRow(int rowNum) const override;
private:

    QSharedPointer<IAWSDeploymentModel> m_model;
};

