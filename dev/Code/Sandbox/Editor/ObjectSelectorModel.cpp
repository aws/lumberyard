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


#include "StdAfx.h"
#include "ObjectSelectorModel.h"
#include "Objects/EntityObject.h"
#include "Objects/BaseObject.h"
#include "Objects/ObjectLayer.h"
#include "Objects/Group.h"
#include "Objects/GeomEntity.h"
#include "Objects/BrushObject.h"
#include "Material/Material.h"
#include "HyperGraph/FlowGraphHelpers.h"
#include "HyperGraph/FlowGraphManager.h"
#include "HyperGraph/FlowGraph.h"
#include "HyperGraph/FlowGraphNode.h"
#include "TrackView/TrackViewAnimNode.h"
#include "TrackView/TrackViewSequence.h"
#include "TrackView/TrackViewSequenceManager.h"

#include <QtUtil.h>
#include <QtUtilWin.h>
#include <StringUtils.h>

#include <QSize>
#include <QDebug>
#include <QStringList>
#include <QIcon>

#include <map>

#define CLOSED_GRP_STR " (Closed)]"

static int g_sortColumn = ObjectSelectorModel::NameColumn;
static Qt::SortOrder g_sortOrder = Qt::DescendingOrder;

ObjectSelectorModel::TGeometryCountMap ObjectSelectorModel::m_mGeomCountMap;
ObjectSelectorModel::ObjToStrMap ObjectSelectorModel::s_flowGraphMap;
ObjectSelectorModel::ObjToStrMap ObjectSelectorModel::s_trackViewMap;

static bool lessThanEmptyLast(const QString& s1, const QString& s2, Qt::SortOrder order)
{
    if (s1.isEmpty() ^ s2.isEmpty())
    {
        return s2.isEmpty();
    }

    return order == Qt::AscendingOrder ? s1 < s2 : s1 > s2;
}

static bool objectLessThan(CBaseObject* obj1, CBaseObject* obj2)
{
    QString text1 = ObjectSelectorModel::data(obj1, Qt::DisplayRole, g_sortColumn).toString().toLower();
    QString text2 = ObjectSelectorModel::data(obj2, Qt::DisplayRole, g_sortColumn).toString().toLower();

    switch (g_sortColumn)
    {
    case ObjectSelectorModel::SelectedColumn:
        if (obj1->IsSelected() ^ obj1->IsSelected())
        {
            return obj1->IsSelected();
        }
        break;
    case ObjectSelectorModel::DefaultMaterialColumn:
    case ObjectSelectorModel::CustomMaterialColumn:
    case ObjectSelectorModel::GeometryColumn:
        text1.replace(WIN_SLASH, "");
        text2.replace(WIN_SLASH, "");
        text1.replace(UNIX_SLASH, "");
        text2.replace(UNIX_SLASH, "");
        if (text1 != text2)
        {
            return lessThanEmptyLast(text1, text2, g_sortOrder);
        }
        break;
    case ObjectSelectorModel::InstancesInLevel:
    case ObjectSelectorModel::LODsColumn:
    {
        bool ok;
        const int lod1 = text1.isEmpty() ? MAXINT : text1.toInt(&ok);
        const int lod2 = text2.isEmpty() ? MAXINT : text2.toInt(&ok);
        return g_sortOrder == Qt::AscendingOrder ? lod1 < lod2 : lod1 >= lod2;
    }
    case ObjectSelectorModel::AIGroupColumn:
    {
        int gropid1 = ObjectSelectorModel::GetAIGroupID(obj1);
        int gropid2 = ObjectSelectorModel::GetAIGroupID(obj2);
        if (gropid1 == -1)
        {
            gropid1 = MAXINT;
        }
        if (gropid2 == -1)
        {
            gropid2 = MAXINT;
        }

        if (gropid1 != gropid2)
        {
            return g_sortOrder == Qt::AscendingOrder ? gropid1 < gropid2 : gropid1 >= gropid2;
        }

        break;
    }
    }

    return lessThanEmptyLast(text1, text2, g_sortOrder);
}

QString ObjectSelectorModel::GetObjectName(CBaseObject* pObject)
{
    QString resultBuffer;

    if (!pObject)
    {
        return QString();
    }

    CGroup* pObjGroup = pObject->GetGroup();

    if (pObjGroup)
    {
        resultBuffer = "[";
        resultBuffer += pObjGroup->GetName();

        if (pObjGroup->IsOpen())
        {
            resultBuffer += "]";
        }
        else
        {
            resultBuffer += CLOSED_GRP_STR;
        }

        resultBuffer += pObject->GetName();
        return resultBuffer;
    }
    else if ((pObject->GetType() == OBJTYPE_GROUP) || (pObject->GetType() == OBJTYPE_PREFAB))
    {
        resultBuffer = "[";
        resultBuffer += pObject->GetName();

        CGroup* pGroupObj = (CGroup*)pObject;

        if (pGroupObj)
        {
            if (pGroupObj->IsOpen())
            {
                resultBuffer += "]";
            }
            else
            {
                resultBuffer += CLOSED_GRP_STR;
            }
        }

        return resultBuffer;
    }
    else
    {
        return pObject->GetName();
    }
}

static QString GetFlowGraphNames(CBaseObject* pObject)
{
    auto it = ObjectSelectorModel::s_flowGraphMap.find(pObject);
    if (it == ObjectSelectorModel::s_flowGraphMap.end())
    {
        return QString();
    }
    else
    {
        return it->second;
    }
}

static QString GetTrackViewName(CBaseObject* pObject)
{
    auto it = ObjectSelectorModel::s_trackViewMap.find(pObject);
    if (it == ObjectSelectorModel::s_trackViewMap.end())
    {
        return QString();
    }
    else
    {
        return it->second;
    }
}

static void GetExtraLightInfo(CBaseObject* pObject, QString& csText)
{
    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntity = (CEntityObject*)pObject;
        if (pEntity)
        {
            IObjectManager* objMan = GetIEditor()->GetObjectManager();

            if (objMan)
            {
                if (objMan->IsLightClass(pEntity))
                {
                    if (!pEntity->GetEntityPropertyString("file_deferred_clip_geom").isEmpty())
                    {
                        csText += ".clip";
                    }

                    if (pEntity->GetEntityPropertyBool("bAmbient"))
                    {
                        csText += ".ambient";
                    }

                    if (pEntity->GetEntityPropertyBool("bAreaLight"))
                    {
                        csText += ".arealight";
                    }

                    if (!pEntity->GetEntityPropertyString("texture_Texture").isEmpty())
                    {
                        csText += ".projector";
                    }

                    int nLightType = pEntity->GetEntityPropertyInteger("nCastShadows");

                    if (nLightType > 0)
                    {
                        csText += ".shadow";

                        switch (nLightType)
                        {
                        case 1:
                            csText += ".lowSpec";
                            break;
                        case 2:
                            csText += ".mediumSpec";
                            break;
                        case 3:
                            csText += ".highSpec";
                            break;
                        case 4:
                            csText += ".veryHighSpec";
                            break;
                        case 7:
                            csText += ".detailSpec";
                            break;
                        }
                    }
                }
            }
        }
    }
}

static QString ComputeTrackViewName(CBaseObject* pObject)
{
    QString result;
    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntity = static_cast<CEntityObject*>(pObject);
        CTrackViewAnimNodeBundle bundle = GetIEditor()->GetSequenceManager()->GetAllRelatedAnimNodes(pEntity);
        const int count = bundle.GetCount();
        for (unsigned int i = 0; i < count; ++i)
        {
            if (!result.isEmpty())
            {
                result += ",";
            }
            CTrackViewSequence* pSequence = bundle.GetNode(i)->GetSequence();
            if (pSequence)
            {
                result += pSequence->GetName();
            }
        }
        return result;
    }

    return QString();
}

static QString ComputeFlowGraphNames(CBaseObject* pObject)
{
    QString result;
    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntity = static_cast<CEntityObject*>(pObject);
        std::vector<CFlowGraph*> flowgraphs;
        CFlowGraph* pEntityFG = 0;
        FlowGraphHelpers::FindGraphsForEntity(pEntity, flowgraphs, pEntityFG);
        QString name("");
        const int count = flowgraphs.size();
        for (size_t i = 0; i < count; ++i)
        {
            if (i > 0)
            {
                result += ",";
            }
            FlowGraphHelpers::GetHumanName(flowgraphs[i], name);
            result += name;
            if (flowgraphs[i] == pEntityFG)
            {
                result += "*";      // A special mark for an entity flow graph
            }
        }
        return result;
    }

    return QString();
}

static QString GetMaterialName(CBaseObject* pObject, bool bGetCustomMaterial = false)
{
    static QString sEffect;
    static QString custMaterialStr;
    static QString defMaterialStr;

    sEffect.clear();
    custMaterialStr.clear();
    defMaterialStr.clear();

    // Get default material stored in CGF file
    if (qobject_cast<CBrushObject*>(pObject))
    {
        CBrushObject* pBrush = (CBrushObject*)pObject;

        if (!pBrush)
        {
            return QString();
        }

        if (!pBrush->GetIStatObj())
        {
            return QString();
        }

        if (!pBrush->GetIStatObj()->GetMaterial())
        {
            return QString();
        }

        defMaterialStr = pBrush->GetIStatObj()->GetMaterial()->GetName();
        defMaterialStr = defMaterialStr.toLower();
        defMaterialStr.replace(UNIX_SLASH, WIN_SLASH);
    }

    // Get material assigned by the user
    CMaterial* pMtl = pObject->GetMaterial();
    if (pMtl)
    {
        custMaterialStr = pMtl->GetName();
        custMaterialStr = custMaterialStr.toLower();
        custMaterialStr.replace(UNIX_SLASH, WIN_SLASH);
    }

    if (!defMaterialStr.isEmpty() || !custMaterialStr.isEmpty())
    {
        if (bGetCustomMaterial)
        {
            if (!custMaterialStr.isEmpty() && custMaterialStr != defMaterialStr)
            {
                return custMaterialStr;
            }
            else
            {
                return QString();
            }
        }
        else
        {
            return defMaterialStr;
        }
    }

    // Entity
    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntity = (CEntityObject*)pObject;
        if (pEntity->GetProperties())
        {
            IVariable* pVar = pEntity->GetProperties()->FindVariable("ParticleEffect");
            if (pVar)
            {
                pVar->Get(sEffect);
                return sEffect;
            }
        }
    }

    return QString();
}

static void GetMaterialBreakability(std::set<QString>* breakTypes, CMaterial* pMaterial)
{
    if (pMaterial)
    {
        const QString surfaceTypeName = pMaterial->GetSurfaceTypeName();
        ISurfaceTypeManager* pSurfaceManager = GetIEditor()->Get3DEngine()->GetMaterialManager()->GetSurfaceTypeManager();
        if (!pSurfaceManager)
        {
            return;
        }

        ISurfaceType* pSurfaceType = pSurfaceManager->GetSurfaceTypeByName(surfaceTypeName.toUtf8().data());
        if (pSurfaceType && pSurfaceType->GetBreakability() != 0)
        {
            breakTypes->insert(pSurfaceType->GetType());
        }

        int subMaterialCount = pMaterial->GetSubMaterialCount();
        for (int i = 0; i < subMaterialCount; ++i)
        {
            CMaterial* pChild = pMaterial->GetSubMaterial(i);
            if (!pChild)
            {
                continue;
            }
            GetMaterialBreakability(breakTypes, pChild);
        }
    }
}

static QString GetObjectBreakability(CBaseObject* pObject)
{
    CMaterial* pMaterial = pObject->GetMaterial();

    if (!pMaterial)
    {
        return QString();
    }
    std::set<QString> breakabilityTypes;
    GetMaterialBreakability(&breakabilityTypes, pMaterial);

    QString result;
    {
        for (auto it = breakabilityTypes.begin(); it != breakabilityTypes.end(); ++it)
        {
            if (!result.isEmpty())
            {
                result += ", ";
            }
            result += *it;
        }
    }
    return result;
}

static QString GetGeometryFile(CBaseObject* pObject)
{
    QString resultBuffer;

    if (qobject_cast<CBrushObject*>(pObject))
    {
        CBrushObject* pObj = (CBrushObject*)pObject;
        resultBuffer = pObj->GetGeometryFile().toLower();
        resultBuffer.replace(UNIX_SLASH, WIN_SLASH);
        return resultBuffer;
    }
    else if (qobject_cast<CGeomEntity*>(pObject))
    {
        CGeomEntity* pObj = (CGeomEntity*)pObject;
        resultBuffer = pObj->GetGeometryFile().toLower();
        resultBuffer.replace(UNIX_SLASH, WIN_SLASH);
        return resultBuffer;
    }
    else if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntity = static_cast<CEntityObject*>(pObject);
        IRenderNode* pEngineNode = pEntity->GetEngineNode();

        if (pEngineNode)
        {
            IStatObj* pEntityStatObj = pEngineNode->GetEntityStatObj();
            ICharacterInstance* iEntityCharacter = pEngineNode->GetEntityCharacter(0);

            if (pEntityStatObj)
            {
                resultBuffer = pEntityStatObj->GetFilePath();
            }

            if (resultBuffer.isEmpty() && iEntityCharacter)
            {
                resultBuffer = iEntityCharacter->GetFilePath();
            }

            if (resultBuffer.isEmpty() && pEntity->GetProperties())
            {
                IVariable* pVar = pEntity->GetProperties()->FindVariable("Model");
                if (pVar)
                {
                    pVar->Get(resultBuffer);
                }
            }

            if (!resultBuffer.isEmpty())
            {
                resultBuffer = resultBuffer.toLower();
                resultBuffer.replace(UNIX_SLASH, WIN_SLASH);
            }
        }
    }

    return resultBuffer;
}

bool GetGeomCount(CBaseObject* pObject, int& val)
{
    val = -1;

    if (pObject)
    {
        QString sGeomFileStr = GetGeometryFile(pObject);
        if (!sGeomFileStr.isEmpty())
        {
            val = stl::find_in_map(ObjectSelectorModel::m_mGeomCountMap, sGeomFileStr, 0);
            return true;
        }
    }

    return false;
}

int ObjectSelectorModel::GetAIGroupID(CBaseObject* obj)
{
    if (qobject_cast<CEntityObject*>(obj))
    {
        CEntityObject* pEnt = (CEntityObject*)obj;
        CVarBlock* properties = pEnt->GetProperties2();
        if (properties)
        {
            IVariable* varEn = properties->FindVariable("groupid");
            if (varEn)
            {
                int groupid;
                varEn->Get(groupid);
                return groupid;
            }
        }
    }
    return -1;
}

static QString GetSmartObject(CBaseObject* pObject)
{
    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntity = (CEntityObject*)pObject;
        return pEntity->GetSmartObjectClass();
    }

    return QString();
}

bool GETLODNumber(CBaseObject* pObject, int& nLodNumber)
{
    IStatObj::SStatistics stats;
    nLodNumber = -1;

    if (qobject_cast<CBrushObject*>(pObject))
    {
        CBrushObject* pBrush = (CBrushObject*)pObject;

        if (!pBrush)
        {
            return false;
        }

        IStatObj* pStatObj = pBrush->GetIStatObj();

        if (pStatObj)
        {
            pStatObj->GetStatistics(stats);
            nLodNumber = stats.nLods;
            return true;
        }
    }
    else if (qobject_cast<CGeomEntity*>(pObject))
    {
        CGeomEntity* pGeomEntity = (CGeomEntity*)pObject;
        IRenderNode* pEngineNode = pGeomEntity->GetEngineNode();

        if (pEngineNode)
        {
            IStatObj* pEntityStatObj = pEngineNode->GetEntityStatObj();
            if (pEntityStatObj)
            {
                pEntityStatObj->GetStatistics(stats);
                nLodNumber = stats.nLods;
                return true;
            }
        }
    }
    else if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntity = static_cast<CEntityObject*>(pObject);
        IRenderNode* pEngineNode = pEntity->GetEngineNode();

        if (pEngineNode)
        {
            IStatObj* pEntityStatObj = pEngineNode->GetEntityStatObj();

            if (pEntityStatObj)
            {
                pEntityStatObj->GetStatistics(stats);
                nLodNumber = stats.nLods;
                return true;
            }

            ICharacterInstance* iEntityCharacter = pEngineNode->GetEntityCharacter(0);

            if (iEntityCharacter)
            {
                const IDefaultSkeleton& rIDefaultSkeleton = iEntityCharacter->GetIDefaultSkeleton();
                {
                    uint32 numInstances = gEnv->pCharacterManager->GetNumInstancesPerModel(rIDefaultSkeleton);
                    if (numInstances > 0)
                    {
                        ICharacterInstance* pCharInstance = gEnv->pCharacterManager->GetICharInstanceFromModel(rIDefaultSkeleton, 0);
                        if (pCharInstance)
                        {
                            if (ISkeletonPose* pSkeletonPose = pCharInstance->GetISkeletonPose())
                            {
                                IDefaultSkeleton& rIDefaultSkeleton = pCharInstance->GetIDefaultSkeleton();
                                uint32 numJoints = rIDefaultSkeleton.GetJointCount();

                                // check StatObj attachments
                                for (uint32 i = 0; i < numJoints; ++i)
                                {
                                    IStatObj* pStatObj = (IStatObj*)pSkeletonPose->GetStatObjOnJoint(i);

                                    if (pStatObj)
                                    {
                                        pStatObj->GetStatistics(stats);
                                        nLodNumber = stats.nLods;
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}

ObjectSelectorModel::ObjectSelectorModel(QObject* parent)
    : QAbstractTableModel(parent)
    , m_objectTypeMask(0)
    , m_displayMode(ObjectSelectorModel::DisplayModeVisible)
    , m_matchPropertyName(true)
{
    connect(this, &QAbstractItemModel::rowsInserted, this, &ObjectSelectorModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &ObjectSelectorModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &ObjectSelectorModel::countChanged);
    connect(this, &QAbstractItemModel::layoutChanged, this, &ObjectSelectorModel::countChanged);
}

ObjectSelectorModel::~ObjectSelectorModel()
{
    Clear();
}

void ObjectSelectorModel::Clear()
{
    beginResetModel();
    m_mGeomCountMap.clear();
    s_flowGraphMap.clear();
    s_trackViewMap.clear();
    m_objects.clear();
    m_indentState.clear();
    endResetModel();
}

QStringList ObjectSelectorModel::ColumnNames()
{
    static const QStringList names = {
        "Name", "Selected", "Type", "Layer", "Default Material", "Custom Material",
        "Breakability", "Smart Object", "Track View", "FlowGraph", "Geometry",
        "Instances In Level", "Number of LODs", "Spec", "AI GroupID"
    };

    Q_ASSERT(names.size() == ObjectSelectorModel::NumberOfColumns);
    return names;
}

void ObjectSelectorModel::Reload(bool rebuildMaps)
{
    beginResetModel();
    m_objects.clear();
    m_indentState.clear();
    const int numObjects = GetIEditor()->GetObjectManager()->GetObjectCount();
    m_objects.reserve(numObjects);
    m_indentState.reserve(numObjects);
    std::vector<CBaseObject*> objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects);

    std::sort(objects.begin(), objects.end(), objectLessThan);

    // Filter the object list properly.
    for (CBaseObject* obj : objects)
    {
        if (m_treeModeEnabled && obj->GetParent() && !m_bSearchInsideObjects)
        {
            continue;
        }

        if (m_treeModeEnabled || m_bSearchInsideObjects)
        {
            AddObjectRecursively(obj, 0);
        }
        else
        {
            AddObject(obj, 0);
        }
    }

    UpdateGeomCount();

    if (rebuildMaps)
    {
        BuildMaps();
    }

    endResetModel();
}

void ObjectSelectorModel::AddObjectToMaps(CBaseObject* pObject)
{
    QString tv = ComputeTrackViewName(pObject);
    if (!tv.isEmpty())
    {
        s_trackViewMap[pObject] = tv;
    }

    QString fg = ComputeFlowGraphNames(pObject);
    if (!fg.isEmpty())
    {
        s_flowGraphMap[pObject] = fg;
    }
}

int ObjectSelectorModel::rowCount(const QModelIndex& parent) const
{
    return m_objects.size();
}

int ObjectSelectorModel::columnCount(const QModelIndex& parent) const
{
    return NumberOfColumns;
}

QVariant ObjectSelectorModel::data(CBaseObject* object, int role, int col)
{
    if (object == nullptr)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch ((Column)col)
        {
        case NameColumn:
            return GetObjectName(object);
        case SelectedColumn:
            return object->IsSelected() ? QStringLiteral("x") : QString();
        case TypeColumn:
        {
            QString result = object->GetTypeDescription();
            GetExtraLightInfo(object, result);
            return result;
        }
        case LayerColumn:
        {
            auto layer = object->GetLayer();
            return layer ? layer->GetName() : QString();
        }
        case DefaultMaterialColumn:
            return GetMaterialName(object);
        case CustomMaterialColumn:
            return GetMaterialName(object, true);
        case BreakabilityColumn:
            return GetObjectBreakability(object);
        case SmartObjectColumn:
            return GetSmartObject(object);
        case TrackViewColumn:
            return GetTrackViewName(object);
        case FlowGraphColumn:
            return GetFlowGraphNames(object);
        case GeometryColumn:
            return GetGeometryFile(object);
        case InstancesInLevel:
        {
            int nVal = -1;
            GetGeomCount(object, nVal);
            if (nVal != -1)
            {
                return nVal;
            }
            else
            {
                return "";
            }
        }
        case LODsColumn:
        {
            int nLODNo1 = -1;
            if (GETLODNumber(object, nLODNo1))
            {
                return nLODNo1;
            }
            return QString();
        }
        case SpecColumn:
        {
            static const QStringList specs = { "All", "Low", "Medium/Console", "PC High", "PC Very High", "Detail" };
            return specs.value(object->GetMinSpec());
        }
        case AIGroupColumn:
        {
            const int groupId = GetAIGroupID(object);
            if (groupId != -1)
            {
                return groupId;
            }
            else
            {
                return QString();
            }
        }
        default:
            return QVariant();
        }
    }
    else if (role == ObjectSelectorModel::ObjectRole)
    {
        return QVariant::fromValue<CBaseObject*>(object);
    }

    return QVariant();
}

QVariant ObjectSelectorModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    const int row = index.row();
    const int col = index.column();
    if (row < 0 || row >= rowCount() || col < 0 || col >= columnCount())
    {
        return QVariant();
    }
    auto object = m_objects.at(row);

    if (role == IdentationRole)
    {
        return m_indentState.at(row);
    }
    else if (role == Qt::DecorationRole && col == NameColumn)
    {
        if (m_treeModeEnabled)
        {
            FirstColumnImage image = LIST_BITMAP_ANY;
            switch (object->GetType())
            {
            case OBJTYPE_GROUP:
                image = LIST_BITMAP_GROUP;
                break;
            case OBJTYPE_TAGPOINT:
                image = LIST_BITMAP_TAGPOINT;
                break;
            case OBJTYPE_ENTITY:
                image = LIST_BITMAP_ENTITY;
                break;
            case OBJTYPE_SHAPE:
                image = LIST_BITMAP_PATH;
                break;
            case OBJTYPE_VOLUME:
                image = LIST_BITMAP_VOLUME;
                break;
            case OBJTYPE_BRUSH:
                image = LIST_BITMAP_BRUSH;
                break;
            }

            QIcon icon(QString(":/ObjectSelector/icons/tile_%1.png").arg(image));
            return QVariant(icon);
        }
        else
        {
            return QVariant();
        }
    }

    return data(object, role, col);
}

QVariant ObjectSelectorModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0 || section >= NumberOfColumns)
    {
        return QVariant();
    }

    if (orientation == Qt::Vertical)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        return ColumnNames().at(section);
    }
    else if (role == Qt::TextAlignmentRole)
    {
        return QVariant(Qt::AlignLeft);
    }

    return QVariant();
}

Qt::ItemFlags ObjectSelectorModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return 0;
    }
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void ObjectSelectorModel::EnableTreeMode(bool enable)
{
    if (m_treeModeEnabled != enable)
    {
        m_treeModeEnabled = enable;
        Reload();
    }
}

void ObjectSelectorModel::UpdateGeomCount()
{
    ObjectSelectorModel::m_mGeomCountMap.clear();

    std::vector<CBaseObject*> objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects);

    const int size = objects.size();
    for (size_t i = 0; i < size; ++i)
    {
        CBaseObject* pObj = objects[i];

        if (pObj)
        {
            QString sGeomFileStr = GetGeometryFile(pObj);

            if (!sGeomFileStr.isEmpty())
            {
                int nGeomCount = stl::find_in_map(ObjectSelectorModel::m_mGeomCountMap, sGeomFileStr, 0);
                ObjectSelectorModel::m_mGeomCountMap[sGeomFileStr] = nGeomCount + 1;
            }
        }
    }
}

void ObjectSelectorModel::EmitDataChanged(CBaseObject* obj)
{
    const int row = std::find(m_objects.cbegin(), m_objects.cend(), obj) - m_objects.cbegin();
    if (row >= 0 && row < m_objects.size())
    {
        emit dataChanged(index(row, 0), index(row, columnCount() - 1));
    }
}

void ObjectSelectorModel::EmitDataChanged()
{
    if (!m_objects.empty())
    {
        emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    }
}

void ObjectSelectorModel::EmitDataChanged(const QModelIndex& ndx)
{
    if (ndx.isValid() && ndx.row() >= 0 && ndx.row() < m_objects.size())
        emit dataChanged(index(ndx.row(), 0), index(ndx.row(), columnCount() - 1));
}

void ObjectSelectorModel::RemoveObject(CBaseObject* obj)
{
    const int row = std::find(m_objects.cbegin(), m_objects.cend(), obj) - m_objects.cbegin();
    if (row >= 0 && row < m_objects.size())
    {
        beginRemoveRows(QModelIndex(), row, row);
        m_objects.erase(m_objects.cbegin() + row);
        m_indentState.erase(m_indentState.cbegin() + row);
        endRemoveRows();
    }
}

bool ObjectSelectorModel::AddObject(CBaseObject* obj, int level)
{
    if (!AcceptsObject(obj))
    {
        return false;
    }

    const int count = m_objects.size();
    beginInsertRows(QModelIndex(), count, count);
    m_objects.push_back(obj);
    m_indentState.push_back(level);
    endInsertRows();

    return true;
}

void ObjectSelectorModel::AddObjectRecursively(CBaseObject* obj, int level)
{
    bool parentObjectAdded = AddObject(obj, level);
    std::vector<CBaseObject*> children;
    const int numChildren = obj->GetChildCount();
    children.reserve(obj->GetChildCount());
    for (int i = 0; i < numChildren; i++)
    {
        children.push_back(obj->GetChild(i));
    }

    std::sort(children.begin(), children.end(), objectLessThan);

    for (CBaseObject* childObj : children)
    {
        AddObjectRecursively(childObj, parentObjectAdded ? level + 1 : level);
    }
}

bool ObjectSelectorModel::IsTreeMode() const
{
    return m_treeModeEnabled;
}

void ObjectSelectorModel::SetObjectTypeMask(int mask)
{
    if (mask != m_objectTypeMask)
    {
        m_objectTypeMask = mask;
        Reload(false);
    }
}

void ObjectSelectorModel::SetFilterText(const QString& text)
{
    if (text != m_filterText)
    {
        m_filterText = text;
        Reload(false);
    }
}

bool ObjectSelectorModel::IsPropertyMatchVariable(IVariable* pVar) const
{
    QString propertyFilter = m_propertyFilter;
    if (m_matchPropertyName)
    {
        if (pVar->GetHumanName().contains(propertyFilter, Qt::CaseInsensitive))
        {
            return true;
        }
    }
    else
    {
        QString value;
        pVar->Get(value);
        if (value.contains(propertyFilter, Qt::CaseInsensitive))
        {
            return true;
        }
    }
    if (pVar->GetNumVariables() > 0)
    {
        for (int i = 0; i < pVar->GetNumVariables(); i++)
        {
            IVariable* pChildVar = pVar->GetVariable(i);
            if (IsPropertyMatchVariable(pChildVar))
            {
                return true;
            }
        }
    }
    return false;
}

bool ObjectSelectorModel::IsPropertyMatch(CBaseObject* pObject) const
{
    CVarBlock* pVars = pObject->GetVarBlock();
    if (pVars)
    {
        for (int i = 0; i < pVars->GetNumVariables(); i++)
        {
            IVariable* pVar = pVars->GetVariable(i);
            if (IsPropertyMatchVariable(pVar))
            {
                return true;
            }
        }
    }

    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntity = (CEntityObject*)pObject;
        {
            CVarBlock* pVars = pEntity->GetProperties();
            if (pVars)
            {
                for (int i = 0; i < pVars->GetNumVariables(); i++)
                {
                    IVariable* pVar = pVars->GetVariable(i);
                    if (IsPropertyMatchVariable(pVar))
                    {
                        return true;
                    }
                }
            }
        }
        {
            CVarBlock* pVars = pEntity->GetProperties2();
            if (pVars)
            {
                for (int i = 0; i < pVars->GetNumVariables(); i++)
                {
                    IVariable* pVar = pVars->GetVariable(i);
                    if (IsPropertyMatchVariable(pVar))
                    {
                        return true;
                    }
                }
            }
        }

        if (pEntity)
        {
            QString propertyFilter = m_propertyFilter;
            QString sPropFilter = propertyFilter.toLower();
            sPropFilter.replace(UNIX_SLASH, WIN_SLASH);

            IRenderNode* pEngineNode = pEntity->GetEngineNode();

            if (pEngineNode)
            {
                QString sVarPath("");

                IStatObj* pEntityStatObj = pEngineNode->GetEntityStatObj();
                ICharacterInstance* iEntityCharacter = pEngineNode->GetEntityCharacter(0);

                if (pEntityStatObj)
                {
                    sVarPath = pEntityStatObj->GetFilePath();
                }

                if (sVarPath.isEmpty() && iEntityCharacter)
                {
                    sVarPath = iEntityCharacter->GetFilePath();
                }

                sVarPath = sVarPath.toLower();
                sVarPath.replace(UNIX_SLASH, WIN_SLASH);

                if (sVarPath.contains(sPropFilter))
                {
                    return true;
                }

                CVarBlock* pVars = pEntity->GetProperties();

                if (sVarPath.isEmpty() && pVars)
                {
                    IVariable* pVar = pVars->FindVariable(sPropFilter.toUtf8().data());

                    if (pVar)
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void ObjectSelectorModel::SetProperties(const QString& propertyText)
{
    if (m_propertyFilter != propertyText)
    {
        m_propertyFilter = propertyText;
        Reload(false);
    }
}

void ObjectSelectorModel::SetDisplayMode(ObjectSelectorModel::DisplayMode mode)
{
    if (mode != m_displayMode)
    {
        m_displayMode = mode;
        Reload(false);
    }
}

void ObjectSelectorModel::SetMatchPropertyName(bool match)
{
    if (match != m_matchPropertyName)
    {
        m_matchPropertyName = match;
        Reload(false);
    }
}

std::vector<CBaseObject*> ObjectSelectorModel::GetObjects() const
{
    return m_objects;
}

ObjectSelectorModel::DisplayMode ObjectSelectorModel::GetDisplayMode() const
{
    return m_displayMode;
}

void ObjectSelectorModel::SetIsLinkTool(bool is)
{
    m_bIsLinkTool = is;
}

void ObjectSelectorModel::SetSearchInsideObjects(bool search)
{
    if (m_bSearchInsideObjects != search)
    {
        m_bSearchInsideObjects = search;
        Reload(false);
    }
}

bool ObjectSelectorModel::GetSearchInsideObjects() const
{
    return m_bSearchInsideObjects;
}

bool ObjectSelectorModel::AcceptsObject(CBaseObject* obj) const
{
    if (m_bIsLinkTool && obj->IsSelected())
    {
        return false;
    }

    if (obj->GetGroup())
    {
        if (!obj->GetGroup()->IsOpen() && !m_bSearchInsideObjects)
        {
            return false;
        }
    }

    if (!(m_objectTypeMask & obj->GetType()))
    {
        return false;
    }

    if (m_displayMode == ObjectSelectorModel::DisplayModeVisible)
    {
        if (obj->IsHidden() || obj->IsFrozen())
        {
            return false;
        }
    }
    else if (m_displayMode == ObjectSelectorModel::DisplayModeHidden)
    {
        if (!obj->IsHidden())
        {
            return false;
        }
    }
    else if (m_displayMode == ObjectSelectorModel::DisplayModeFrozen)
    {
        if (!obj->IsFrozen())
        {
            return false;
        }
    }

    CObjectLayer* layer = obj->GetLayer();
    if (layer->IsFrozen())
    {
        return false;
    }

    if (!layer->IsVisible())
    {
        return false;
    }

    if (!m_filterText.isEmpty())
    {
        QString objName = ObjectSelectorModel::GetObjectName(obj);

        QString objTypeDescription = obj->GetTypeDescription();
        QString objLayerName = obj->GetLayer()->GetName();

        if (!objName.contains(m_filterText, Qt::CaseInsensitive) &&
            !objTypeDescription.contains(m_filterText, Qt::CaseInsensitive) &&
            !objLayerName.contains(m_filterText, Qt::CaseInsensitive))
        {
            return false;
        }
    }

    if (!m_propertyFilter.isEmpty())
    {
        if (!IsPropertyMatch(obj))
        {
            return false;
        }
    }

    return true;
}

void ObjectSelectorModel::UpdateFlowGraphInMaps(REFGUID guid)
{
    IObjectManager* objMan = GetIEditor()->GetObjectManager();
    CBaseObject* pObject = objMan->FindObject(guid);
    if (!pObject)
    {
        return;
    }
    s_flowGraphMap.erase(pObject);
    if (pObject->CheckFlags(OBJFLAG_DELETED))
    {
        return;
    }
    QString fg = ComputeFlowGraphNames(pObject);
    if (!fg.isEmpty())
    {
        s_flowGraphMap[pObject] = fg;
    }
}

void ObjectSelectorModel::OnHyperGraphManagerEvent(EHyperGraphEvent event, IHyperGraph* pGraph, IHyperNode* pINode)
{
    if (event == EHG_GRAPH_INVALIDATE || event == EHG_GRAPH_ADDED || event == EHG_GRAPH_REMOVED || event == EHG_GRAPH_UPDATE_ENTITY
        || event == EHG_NODE_ADD || event == EHG_NODE_DELETE || event == EHG_NODE_UPDATE_ENTITY)
    {
        if (pGraph)
        {
            if (((CHyperGraph*)pGraph)->IsFlowGraph())
            {
                CFlowGraph* pFlowGraph = (CFlowGraph*)pGraph;
                CEntityObject* pGraphObject = pFlowGraph->GetEntity();

                IHyperGraphEnumerator* pEnum = pFlowGraph->GetNodesEnumerator();
                for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
                {
                    if (((CHyperNode*)pINode)->IsFlowNode() && ((CHyperNode*)pINode)->GetFlowNodeId() != InvalidFlowNodeId)
                    {
                        CFlowNode* pFlowNode = (CFlowNode*)pINode;
                        CEntityObject* pNodeObject = pFlowNode->GetEntity();
                        if (pNodeObject && pNodeObject != pGraphObject)
                        {
                            m_modifiedFlowGraphObjects.push_back(pNodeObject->GetId());
                        }
                    }
                }
                pEnum->Release();
                if (pGraphObject)
                {
                    m_modifiedFlowGraphObjects.push_back(pGraphObject->GetId());
                }
            }
        }
        else if (pINode)
        {
            CHyperGraph* pFlowGraph = (CHyperGraph*)pINode->GetGraph();
            if (pFlowGraph && pFlowGraph->IsFlowGraph()
                && ((CHyperNode*)pINode)->IsFlowNode() && ((CHyperNode*)pINode)->GetFlowNodeId() != InvalidFlowNodeId)
            {
                CFlowNode* pFlowNode = (CFlowNode*)pINode;
                if (pFlowNode->GetEntity())
                {
                    m_modifiedFlowGraphObjects.push_back(pFlowNode->GetEntity()->GetId());
                }
            }
        }
    }
}

void ObjectSelectorModel::UpdateFlowGraphs()
{
    if (!m_modifiedFlowGraphObjects.empty())
    {
        for (size_t i = 0; i < m_modifiedFlowGraphObjects.size(); ++i)
        {
            UpdateFlowGraphInMaps(m_modifiedFlowGraphObjects[i]);
        }
        m_modifiedFlowGraphObjects.resize(0);
        EmitDataChanged();
    }
}

void ObjectSelectorModel::BuildMaps()
{
    s_flowGraphMap.clear();
    s_trackViewMap.clear();
    m_modifiedFlowGraphObjects.resize(0);

    for (size_t i = 0; i < m_objects.size(); ++i)
    {
        AddObjectToMaps(m_objects[i]);
    }

    // Now it has the latest data again.
    m_bTrackViewModified = false;
}

void ObjectSelectorModel::SetTrackViewModified(bool modified)
{
    m_bTrackViewModified = modified;
}

bool ObjectSelectorModel::IsTrackViewModified() const
{
    return m_bTrackViewModified;
}

int ObjectSelectorModel::IndexOfObject(CBaseObject* obj) const
{
    auto it = find(m_objects.cbegin(), m_objects.cend(), obj);
    return it == m_objects.cend() ? -1 : it - m_objects.cbegin();
}

void ObjectSelectorModel::sort(int column, Qt::SortOrder order)
{
    g_sortColumn = column;
    g_sortOrder = order;

    if (!m_objects.empty())
    {
        beginResetModel();
        std::sort(m_objects.begin(), m_objects.end(), objectLessThan);
        endResetModel();
    }
}

#include <ObjectSelectorModel.moc>
