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
#ifndef CRYINCLUDE_EDITOR_VEGETATIONDATABASEMODEL_H
#define CRYINCLUDE_EDITOR_VEGETATIONDATABASEMODEL_H

#pragma once

#include "Util/AbstractSortModel.h"

class CVegetationObject;

class CVegetationDataBaseModel
    : public AbstractSortModel
{
    Q_OBJECT

public:
    enum Column
    {
        ColumnVisible,
        ColumnObject,
        ColumnCategory,
        ColumnCount,
        ColumnTexSize,
        ColumnMaterial,
        ColumnElevationMin,
        ColumnElevationMax,
        ColumnSlopeMin,
        ColumnSlopeMax
    };

    enum Roles
    {
        VegetationObjectRole = Qt::UserRole + 1
    };

    CVegetationDataBaseModel(QObject* parent = 0);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void setVegetationObjects(const std::vector<CVegetationObject*>& objects);

private:
    std::vector<CVegetationObject*> m_objects;
    QVector<QPixmap> m_reportImageList;
};

#endif // CRYINCLUDE_EDITOR_VEGETATIONDATABASEMODEL_H
