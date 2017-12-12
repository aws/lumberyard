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

#ifndef CRYINCLUDE_EDITOR_LAYERSSELECTDIALOG_H
#define CRYINCLUDE_EDITOR_LAYERSSELECTDIALOG_H

#pragma once

#include <QDialog>
class CLayersListBox;

// CLayersSelectDialog dialog

class CLayersSelectDialog
    : public QDialog
{
public:
    CLayersSelectDialog(const QPoint& point, QWidget* parent = NULL);    // standard constructor
    virtual ~CLayersSelectDialog();

    void SetSelectedLayer(const QString& sel) { m_selectedLayer = sel; };
    QString GetSelectedLayer() { return m_selectedLayer; };

protected:
    void ReloadLayers();

    CLayersListBox* m_layers;
    void OnLbnSelchangeLayers();
    void focusOutEvent(QFocusEvent* e) override;

    QString m_selectedLayer;
    QPoint m_origin;
};

#endif // CRYINCLUDE_EDITOR_LAYERSSELECTDIALOG_H
