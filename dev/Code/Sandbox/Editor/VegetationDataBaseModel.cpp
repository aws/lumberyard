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
#include "StdAfx.h"
#include "VegetationDataBaseModel.h"

#include "VegetationObject.h"

#include <QFont>
#include <QPalette>

CVegetationDataBaseModel::CVegetationDataBaseModel(QObject* parent)
    : AbstractSortModel(parent)
{
    m_reportImageList.push_back(QPixmap(":/res/layer_editor_layer_buttons-0.png"));
    m_reportImageList.push_back(QPixmap(":/res/layer_editor_layer_buttons-1.png"));
}

int CVegetationDataBaseModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_objects.size();
}

int CVegetationDataBaseModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 10;
}

QVariant CVegetationDataBaseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
    {
        return {};
    }

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (section)
        {
        case ColumnVisible:
            return QString();
        case ColumnObject:
            return tr("Object");
        case ColumnCategory:
            return tr("Category");
        case ColumnCount:
            return tr("Count");
        case ColumnTexSize:
            return tr("TexMem Usage");
        case ColumnMaterial:
            return tr("Material");
        case ColumnElevationMin:
            return tr("Elevation Min");
        case ColumnElevationMax:
            return tr("Elevation Max");
        case ColumnSlopeMin:
            return tr("Slope Min");
        case ColumnSlopeMax:
            return tr("Slope Max");
        default:
            return {};
        }
    }
    case Qt::DecorationRole:
    {
        switch (section)
        {
        case ColumnVisible:
            return m_reportImageList.value(1);
        }
    }
    }

    return {};
}

QVariant CVegetationDataBaseModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_objects.size())
    {
        return {};
    }

    CVegetationObject* pObject = m_objects[index.row()];

    switch (role)
    {
    case Qt::DisplayRole:
    {
        QString mtlName;
        QString texSize;
        if (pObject->GetObject())
        {
            _smart_ptr<IMaterial> pMtl = pObject->GetObject()->GetMaterial();
            if (pMtl)
            {
                mtlName = Path::GetFileName(pMtl->GetName());
            }

            int nTexSize = pObject->GetTextureMemUsage();
            texSize = QStringLiteral("%1 K").arg(nTexSize / 1024);
        }

        switch (index.column())
        {
        case ColumnObject:
            return pObject->GetFileName();
        case ColumnCategory:
            return pObject->GetCategory();
        case ColumnCount:
            return pObject->GetNumInstances();
        case ColumnTexSize:
            return texSize;
        case ColumnMaterial:
            return mtlName;
        case ColumnElevationMin:
            return pObject->GetElevationMin();
        case ColumnElevationMax:
            return pObject->GetElevationMax();
        case ColumnSlopeMin:
            return pObject->GetSlopeMin();
        case ColumnSlopeMax:
            return pObject->GetSlopeMax();
        }
        break;
    }
    case Qt::DecorationRole:
    {
        if (index.column() == ColumnVisible)
        {
            static QPixmap empty(m_reportImageList.value(0).size());
            empty.fill(Qt::transparent);
            return pObject->IsHidden() ? empty : m_reportImageList.value(0);
        }
        break;
    }
    case Qt::ForegroundRole:
    {
        if (index.column() == ColumnMaterial)
        {
            return QPalette().color(QPalette::Link);
        }
        break;
    }
    case VegetationObjectRole:
    {
        return QVariant::fromValue(pObject);
    }
    }

    return {};
}

void CVegetationDataBaseModel::setVegetationObjects(const std::vector<CVegetationObject*>& objects)
{
    beginResetModel();
    m_objects = objects;
    endResetModel();
}

#include <VegetationDataBaseModel.moc>
