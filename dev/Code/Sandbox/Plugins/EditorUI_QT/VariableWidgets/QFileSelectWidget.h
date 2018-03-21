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
#ifndef QFILESELECTWIDGET_H
#define QFILESELECTWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>

namespace Ui {
    class QFileSelectWidget;
}

class QPushButton;

class QFileSelectWidget
    : public QWidget
{
    Q_OBJECT

public:
    explicit QFileSelectWidget(QWidget* parent = 0);
    virtual ~QFileSelectWidget();

    virtual void setPath(const QString& path);
    QString getDisplayPath() const;
    QString getFullPath() const;
    bool addUniqueItem(const QString& text, const QVariant& userData = QVariant());

protected:
    virtual void onOpenSelectDialog();
    virtual void onReturnPressed();
    virtual void onSelectedIndexChanged(int index);
    virtual QString pathFilter(const QString& path);
    void addWidget(QWidget* widget, int row, int col, int rowspan, int colspan);
    void setMainButton(const QString& caption, const QString& tooltip, const QIcon* icon = NULL, int row = 0, int col = 5, int rowspan = 1, int colspan = 1);
    QPushButton* addButton(const QString& caption, const QString& tooltip, int row, int col, int rowspan, int colspan, const QIcon* icon = NULL);

    virtual bool eventFilter(QObject * watched, QEvent * event) override;

protected:
    QString m_lookInFolder;
    QString m_dialogCaption;
    QString m_fileFilter;
private slots:
    void on_buttonOpenFileDialog_clicked();
    void onCurrentIdxChanged(int);

protected:
    Ui::QFileSelectWidget* ui;
};

#endif // QFILESELECTWIDGET_H
