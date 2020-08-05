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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialog>
#include <QListWidget>
#include <QButtonGroup>
#include <QVector>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/unordered_map.h>

class QScrollArea;

namespace QAWSQTControls
{
    class QAWSNoDataLabel
        : public QLabel
    {
        Q_OBJECT
    };
}

class ScrollSelectionDialog
    : public QDialog
{
    Q_OBJECT

public:
    static const int scrollRowHeight = 30;
    static const int verticalHeadingMarginSize = 2 * 11;
    static const int defaultWidth = 300;
    static const int defaultScrollWidth = defaultWidth - 28;
    static const int defaultHeight = 148;
    static const int defaultScrollHeight = 80;
    static const int windowBuffer = 20;
    static const int cancelOkButtonWidth = 60;
    static const int scrollBuffer = 20;

    ScrollSelectionDialog(QWidget* parent = nullptr);
    virtual ~ScrollSelectionDialog();

    static const char* GetPaneName() { return "Scroll Selection"; }
    virtual const char* GetWindowName() const { return GetPaneName(); }

    virtual void Display();

    virtual int GetDefaultWidth() const { return defaultWidth; }
    virtual int GetDefaultHeight() const { return defaultHeight; }

    virtual int GetRowCount() const = 0;
    virtual bool IsSelected(int rowNum) const = 0;

    QString GetSelectedButtonText() const;

    void DoDynamicResize();

public slots:

    void UpdateView();

    void OnCancel();
    void OnOK();

protected slots:

    virtual void OnRadioButtonClicked(int id);
    virtual void OnModelReset();
protected:

    void InitializeWindow();
    void BindData();
    void UpdateSelection(const QString&);
    void CloseWindow();

    QWidget& GetMainWidget() { return m_mainWidget; }
    QVBoxLayout& GetMainContainer() { return m_mainContainer; }

    virtual void AddButtonRow(QVBoxLayout* mainLayout);
    virtual void AddOkCancel(QHBoxLayout* mainLayout);
    virtual void AddRow(int rowNum, QVBoxLayout* scrollLayout);
    virtual void OnBindData() {}

    void AddExtraScrollWidget(QWidget* addWidget);
    void AddNoDataLabel(QVBoxLayout* scrollLayout);
    void SetupLabel(QLabel* label, QVBoxLayout* scrollLayout);
    virtual void SetupNoDataConnection() {}
    virtual void AddScrollRow(QVBoxLayout*) {}
    virtual void AddScrollHeadings(QVBoxLayout*) {}
    virtual void SetupConnections() {}
    virtual int AddScrollColumnHeadings(QVBoxLayout*) { return 0; }
    virtual int AddLowerScrollControls(QVBoxLayout*) { return 0; }

    virtual void AddHorizontalSeparator(QVBoxLayout* mainLayout);

    virtual void SetCurrentSelection(QString& selectedText) const = 0;
    virtual QString GetDataForRow(int rowNum) const = 0;

    virtual const char* GetNoDataText() const { return nullptr; }
    virtual const char* GetHasDataText() const { return nullptr; }
    QLabel* GetNoDataLabel() const { return m_noDataLabel; }
    void SetNoDataLabel(QLabel* dataLabel) { m_noDataLabel = dataLabel; }

    QButtonGroup& GetButtonGroup() { return m_buttonGroup; }

    void MoveCenter();
private:

    QWidget m_mainWidget;
    QVBoxLayout m_mainContainer;
    QButtonGroup m_buttonGroup;
    QVector<QWidget*> m_extraScrollWidgets; // Things we need to clean up each redraw of the scroll area (Often added by AddScrollRow)

    QWidget* m_scrollWidget {
        nullptr
    };
    QVBoxLayout* m_scrollLayout {
        nullptr
    };
    QScrollArea* m_scrollArea {
        nullptr
    };
    QLabel* m_noDataLabel {
        nullptr
    };
};


