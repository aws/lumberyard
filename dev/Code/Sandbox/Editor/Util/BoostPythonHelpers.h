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

#ifndef CRYINCLUDE_EDITOR_UTIL_BOOSTPYTHONHELPERS_H
#define CRYINCLUDE_EDITOR_UTIL_BOOSTPYTHONHELPERS_H
#pragma once

// Suppress warning in pymath.h where a conflict with round exists with VS 12.0 math.h ::  pymath.h(22) : warning C4273: 'round' : inconsistent dll linkage
#pragma warning(disable: 4273)
#pragma warning(disable: 4068) // Disable unknown pragma, it's worthless
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#include <boost/python.hpp>
#pragma clang diagnostic pop
#pragma warning( default: 4273)

// Forward Declarations
class CBaseObject;
class CObjectLayer;
class CMaterial;
class CVegetationObject;

struct SPyWrappedProperty
{
    void SetProperties(IVariable* pVar);

    struct SColor
    {
        int r, g, b;
    };

    struct SVec
    {
        float x, y, z, w;
    };

    struct STime
    {
        int hour, min;
    };

    enum EType
    {
        eType_Bool,
        eType_Int,
        eType_Float,
        eType_String,
        eType_Vec3,
        eType_Vec4,
        eType_Color,
        eType_Time
    };

    union UProperty
    {
        bool boolValue;
        int intValue;
        float floatValue;
        SColor colorValue;
        SVec vecValue;
        STime timeValue;
    };

    EType type;
    UProperty property;
    QString stringValue;
};

typedef boost::shared_ptr<SPyWrappedProperty> pSPyWrappedProperty;

// Dynamic Class types to emulate key engine objects.
struct SPyWrappedClass
{
    enum EType
    {
        eType_ActorEntity,
        eType_Area,
        eType_Brush,
        eType_Camera,
        eType_Decal,
        eType_Entity,
        eType_Particle,
        eType_Prefab,
        eType_Group,
        eType_None,
    } type;

    boost::shared_ptr<void> ptr;
};
typedef boost::shared_ptr<SPyWrappedClass> pSPyWrappedClass;

class SANDBOX_API CAutoRegisterPythonModuleHelper
{
public:
    CAutoRegisterPythonModuleHelper(const char* pModuleName, void (*initFunc)())
    {
        SModule module;
        module.name = pModuleName;
        module.initFunc = initFunc;
        s_modules.push_back(module);
    }

    struct SModule
    {
        string name;
        void (* initFunc)();

        bool operator == (const SModule& other) const
        {
            return name == other.name;
        }
    };
    typedef std::vector<SModule> ModuleList;
    static ModuleList s_modules;
};

class SANDBOX_API CAutoRegisterPythonCommandHelper
{
public:
    CAutoRegisterPythonCommandHelper(const char* pModuleName,
        const char* pFunctionName,
        void (*registerFunc)(),
        const char* pDescription,
        const char* pUsageExample);

    // An index to CAutoRegisterPythonModuleHelper::s_modules
    const char* m_pModuleName;
    int m_moduleIndex;
    QString m_name;
    QString m_description;
    QString m_example;
    void (* m_registerFunc)();

    CAutoRegisterPythonCommandHelper* m_pNext;

    static CAutoRegisterPythonCommandHelper* s_pFirst;
    static CAutoRegisterPythonCommandHelper* s_pLast;

    void ResolveModuleIndex();
    static void RegisterFunctionsForModule(const char* pModuleName);
};

// Engine Classes Wrapped for easy get \ set \ update functionality to Python.
//
// PyBaseObject
//      -PyGameLayer
//      -PyGameMaterial
//          -PyGameSubMaterial Array
//              -PyGameTexture Array
//      -PyGameClass
//          -PyGameBrush
//          -PyGameEntity
//          -PyGameSolid
//          -PyGameGroup
//          -PyGamePrefab
//          -PyGameCamera
//      -PyGameVegetation
//          -PyGameVegetationInstance

class PyGameLayer
{
public:
    PyGameLayer(void* layerPtr);
    ~PyGameLayer();
    void* GetPtr() const { return m_layerPtr; }
    QString GetName() const { return m_layerName; }
    QString GetPath() const { return m_layerPath; }
    GUID GetGUID() const { return m_layerGUID; }
    bool IsVisible() const { return m_layerVisible; }
    bool IsFrozen() const { return m_layerFrozen; }
    bool IsExternal() const { return m_layerExternal; };
    bool IsExportable() const { return m_layerExportable; };
    bool IsExportLayerPak() const { return m_layerExportLayerPak; };
    bool IsDefaultLoaded() const { return m_layerDefaultLoaded; };
    bool IsPhysics() const { return m_layerPhysics; }
    std::vector<boost::shared_ptr<PyGameLayer> > GetChildren() const { return m_layerChildren; }

    // Many setters ignored here as they can better be handled in other areas.
    void SetName(QString name) { m_layerName = name; };
    void SetVisible(bool visible) { m_layerVisible = visible; };
    void SetFrozen(bool frozen) { m_layerFrozen = frozen; };
    void SetExternal(bool external) { m_layerExternal = external; };
    void SetExportable(bool exportable) { m_layerExportable = exportable; };
    void SetExportLayerPak(bool exportLayerPak) { m_layerExportLayerPak = exportLayerPak; };
    void SetDefaultLoaded(bool loaded) { m_layerDefaultLoaded = loaded; };
    void SetHavePhysics(bool physics) { m_layerPhysics = physics; }

    void UpdateLayer();

private:
    void* m_layerPtr;
    QString m_layerName;
    QString m_layerPath;
    GUID m_layerGUID;
    std::vector<boost::shared_ptr<PyGameLayer> > m_layerChildren;
    bool m_layerVisible;
    bool m_layerFrozen;
    bool m_layerExternal;
    bool m_layerExportable;
    bool m_layerExportLayerPak;
    bool m_layerDefaultLoaded;
    bool m_layerPhysics;
};
typedef boost::shared_ptr<PyGameLayer> pPyGameLayer;

class PyGameTexture
{
public:
    PyGameTexture(void* pTexture);
    ~PyGameTexture();
    void* GetPtr() const { return m_texPtr; }
    QString GetName() const { return m_texName; }

    void SetName(QString name) { m_texName = name; }

    void UpdateTexture();

private:
    void* m_texPtr;
    QString m_texName;
};
typedef boost::shared_ptr<PyGameTexture> pPyGameTexture;

class PyGameSubMaterial
{
public:
    PyGameSubMaterial(void* pMat, int id);
    ~PyGameSubMaterial();
    void* GetPtr() const { return m_matPtr; }
    QString GetName() const { return m_matName; }
    QString GetShader() const { return m_matShader; }
    QString GetSurfaceType() const { return m_matSurfaceType; }
    std::map<QString, pSPyWrappedProperty> GetMatParams() const { return m_matParams; }
    AZStd::unordered_map<ResourceSlotIndex, pPyGameTexture> GetTextures() const { return m_matTextures; }

    void SetName(QString name) { m_matName = name; }
    void SetShader(QString shader) { m_matShader = shader; }
    void SetSurfaceType(QString surface) { m_matSurfaceType = surface; }
    void SetTextures(AZStd::unordered_map<ResourceSlotIndex, pPyGameTexture> textures) { m_matTextures = textures; }
    void SetMatParams(std::map<QString, pSPyWrappedProperty> params) { m_matParams = params; }

    void UpdateSubMaterial();

private:
    void* m_matPtr;
    int m_matId;
    QString m_matName;
    QString m_matPath;
    QString m_matShader;
    QString m_matSurfaceType;
    AZStd::unordered_map<ResourceSlotIndex, pPyGameTexture>	m_matTextures;  // map by texture slot id
    std::map<QString, pSPyWrappedProperty>  m_matParams;
};
typedef boost::shared_ptr<PyGameSubMaterial> pPyGameSubMaterial;

class PyGameMaterial
{
public:
    PyGameMaterial(void* pMat);
    ~PyGameMaterial();

    void* GetPtr() const { return m_matPtr; }
    QString GetName() const { return m_matName; }
    QString GetPath() const { return m_matPath; }
    std::vector<pPyGameSubMaterial> GetSubMaterials() const { return m_matSubMaterials; }

    void SetName(QString name) { m_matName = name; };
    //void SetPath(QString path) { m_matPath = path; };
    // No setters for this class as the engine handles this way better.

    void UpdateMaterial();

private:
    void* m_matPtr;
    QString m_matName;
    QString m_matPath;
    std::vector<pPyGameSubMaterial> m_matSubMaterials;
};
typedef boost::shared_ptr<PyGameMaterial> pPyGameMaterial;

class PyGameObject
{
public:
    PyGameObject(void* objPtr);

    typedef boost::shared_ptr<PyGameObject> pPyGameObject;

    pSPyWrappedClass GetClassObject() { return m_objClass; }
    void* GetPtr() const { return m_objPtr; }
    QString GetName() const { return m_objName; }
    QString GetType() const { return m_objType; }
    GUID GetGUID() const { return m_objGUID; }
    pPyGameLayer GetLayer() const { return m_objLayer; }
    pPyGameMaterial GetMaterial() const { return m_objMaterial; }
    pPyGameObject GetParent();
    Vec3 GetPosition() const { return m_objPosition; }
    Vec3 GetRotation() const { return m_objRotation; }
    Vec3 GetScale() const { return m_objScale; }
    AABB GetBounds() const { return m_objBounds; }
    bool IsInGroup() const { return m_objInGroup; }
    bool IsVisible() const { return m_objVisible; }
    bool IsFrozen() const { return m_objFrozen; }
    bool IsSelected() const { return m_objSelected; }

    void SetName(QString name) { m_objName = name; };
    void SetLayer(pPyGameLayer pLayer) { m_objLayer = pLayer; };
    void SetMaterial(pPyGameMaterial pMat) { m_objMaterial = pMat; };
    void SetParent(pPyGameObject pParent);
    void SetPosition(Vec3 position) { m_objPosition = position; };
    void SetRotation(Vec3 rotation) { m_objRotation = rotation; };
    void SetScale(Vec3 scale) { m_objScale = scale; };
    void SetVisible(bool visible) { m_objVisible = visible; };
    void SetFrozen(bool frozen) { m_objFrozen = frozen; };
    void SetSelected(bool selected) { m_objSelected = selected; };

    void UpdateObject(); // store the class ref as a void pointer. in update reinterpret cast the object back to its baseclass, in this case CBaseObject.

private:
    void* m_objPtr;
    pSPyWrappedClass m_objClass;
    QString m_objName;
    QString m_objType;
    GUID m_objGUID;
    Vec3 m_objPosition;
    Vec3 m_objRotation;
    Vec3 m_objScale;
    AABB m_objBounds;
    bool m_objInGroup;
    bool m_objVisible;
    bool m_objFrozen;
    bool m_objSelected;
    pPyGameLayer m_objLayer;
    pPyGameMaterial m_objMaterial;
    pPyGameObject m_objParent;
};
typedef boost::shared_ptr<PyGameObject> pPyGameObject;

class PyGameBrush
{
public:
    PyGameBrush(void* brushPtr, pSPyWrappedClass sharedPtr);
    ~PyGameBrush();
    QString GetModel() const { return m_brushModel; }
    int GetRatioLod() const { return m_brushRatioLod; }
    float GetViewDistanceMultiplier() const { return m_brushViewDistanceMultiplier; }
    int GetLodCount() const { return m_brushLodCount; }

    void SetModel(QString model) { m_brushModel = model; }
    void SetRatioLod(int ratio) { m_brushRatioLod = ratio; }
    void SetViewDistanceMultiplier(float multiplier) { m_brushViewDistanceMultiplier = multiplier; }

    void Reload();
    void UpdateBrush();

private:
    void* m_brushPtr;
    QString m_brushModel;
    int m_brushRatioLod;
    float m_brushViewDistanceMultiplier;
    int m_brushLodCount;
};

class PyGameEntity
{
public:
    PyGameEntity(void* entityPtr, pSPyWrappedClass sharedPtr);
    ~PyGameEntity();
    QString GetModel() const { return m_entityModel; }
    QString GetSubClass() const { return m_entitySubClass; }
    int GetRatioLod() const { return m_entityRatioLod; }
    int GetRatioViewDist() const { return m_entityRatioviewDist; }
    std::map<QString, pSPyWrappedProperty> GetProperties() { return m_entityProps; }

    void SetModel(QString model) { m_entityModel = model; }
    void SetSubClass(QString cls) { m_entitySubClass = cls; }
    void SetRatioLod(int ratio) { m_entityRatioLod = ratio; }
    void SetRatioViewDist(int ratio) { m_entityRatioviewDist = ratio; }
    void SetProperties(std::map<QString, pSPyWrappedProperty> props) { m_entityProps = props; }

    void Reload();
    void UpdateEntity();

private:
    void* m_entityPtr;
    int m_entityId;
    QString m_entitySubClass;
    QString m_entityModel;
    float m_entityRatioLod;
    float m_entityRatioviewDist;
    std::map<QString, pSPyWrappedProperty> m_entityProps;
};

class PyGamePrefab
{
public:
    PyGamePrefab(void* prefabPtr, pSPyWrappedClass sharedPtr);
    ~PyGamePrefab();

    bool IsOpen();
    QString GetName() const { return m_prefabName; }
    std::vector<pPyGameObject> GetChildren();

    void SetName(QString name) { m_prefabName = name; }

    void AddChild(pPyGameObject pObj);
    void RemoveChild(pPyGameObject pObj);

    void Open();
    void Close();
    void ExtractAll();

    void UpdatePrefab();

private:
    void* m_prefabPtr;
    QString m_prefabName;
    std::vector<pPyGameObject> m_prefabChildren;
};

class PyGameGroup
{
public:
    PyGameGroup(void* groupPtr, pSPyWrappedClass sharedPtr);
    ~PyGameGroup();

    bool IsOpen();
    std::vector<pPyGameObject> GetChildren(); // const { return m_groupChildren; }

    void AddChild(pPyGameObject pObj);
    void RemoveChild(pPyGameObject pObj);

    void Open();
    void Close();

    void UpdateGroup();

private:
    void* m_groupPtr;
    std::vector<pPyGameObject> m_groupChildren;
};

class PyGameCamera
{
public:
    PyGameCamera(void* cameraPtr, pSPyWrappedClass sharedPtr);
    ~PyGameCamera();

    void UpdateCamera();

private:
    void* m_cameraPtr;
};

class PyGameVegetationInstance
{
public:
    PyGameVegetationInstance(void* vegPtr);
    ~PyGameVegetationInstance();

    Vec3 GetPosition() const { return m_vegPosition; }
    float GetAngle() const { return m_vegAngle; }
    float GetScale() const { return m_vegScale; }
    float GetBrightness() const { return m_vegBrightness; }

    void SetPosition(Vec3 position) { m_vegPosition = position; }
    void SetAngle(float angle) { m_vegAngle = angle; }
    void SetScale(float scale) { m_vegScale = scale; }
    void SetBrightness(float brightness) { m_vegBrightness = brightness; }

    void UpdateVegetationInstance();

private:
    void* m_vegPtr;
    Vec3 m_vegPosition;
    float m_vegAngle;
    float m_vegScale;
    float m_vegBrightness;
};
typedef boost::shared_ptr<PyGameVegetationInstance> pPyGameVegetationInstance;

class PyGameVegetation
{
public:
    PyGameVegetation(void* vegPtr);
    ~PyGameVegetation();

    void* GetPtr() const { return m_vegPtr; }
    QString GetName() const { return m_vegName; }
    QString GetCategory() const { return m_vegCategory; }
    int GetID() const { return m_vegID; }
    int GetNumInstances() const { return m_vegInstCount; }
    std::vector<pPyGameVegetationInstance> GetInstances() const { return m_vegInstances; }
    bool IsSelected() const { return m_vegSelected; }
    bool IsVisible() const { return m_vegVisible; }
    bool IsFrozen() const { return m_vegFrozen; }
    bool IsCastShadow() const { return m_vegCastShadows; }
    bool IsReceiveShadow() const { return m_vegReceiveShadows; }
    bool IsAutoMerged() const { return m_vegAutoMerged; }
    bool IsHideable() const { return m_vegHideable; }

    void SetName(QString name) { m_vegName = name; }
    void SetCategory(QString category) { m_vegCategory = category; }
    void SetSelected(bool selected) { m_vegSelected = selected; }
    void SetVisible(bool visible) { m_vegVisible = visible; }
    void SetFrozen(bool frozen) { m_vegFrozen = frozen; }
    void SetCastShadow(bool castShadows) { m_vegCastShadows = castShadows; }
    void SetReceiveShadow(bool receiveShadows) { m_vegReceiveShadows = receiveShadows = receiveShadows; }
    void SetHideable(bool hideable) { m_vegHideable = hideable; }

    void Load();
    void Unload();

    void UpdateVegetation();

private:
    void* m_vegPtr;
    QString m_vegName;
    QString m_vegCategory;
    int m_vegID;
    int m_vegInstCount;
    bool m_vegSelected;
    bool m_vegVisible;
    bool m_vegFrozen;
    bool m_vegCastShadows;
    bool m_vegReceiveShadows;
    bool m_vegAutoMerged;
    bool m_vegHideable;
    std::vector<pPyGameVegetationInstance> m_vegInstances;
};
typedef boost::shared_ptr<PyGameVegetation> pPyGameVegetation;

// Python Engine Objects Cache Template.
template<class SHAREDPTR, class PTR>
struct SPyWrapperCache
{
public:
    ~SPyWrapperCache() { ClearCache(); }
    bool IsCached(PTR index)
    {
        for (auto iter = m_Cache.begin(); iter != m_Cache.end(); iter++)
        {
            if (iter->get()->GetPtr() == index)
            {
                return true;
            }
        }
        return false;
    }
    SHAREDPTR GetCachedSharedPtr(PTR index)
    {
        SHAREDPTR result;
        for (auto iter = m_Cache.begin(); iter != m_Cache.end(); iter++)
        {
            if (iter->get()->GetPtr() == index)
            {
                return *iter;
            }
        }
        return result;
    }
    void AddToCache(SHAREDPTR sharedPtr) { m_Cache.push_back(sharedPtr); }
    void RemoveFromCache(SHAREDPTR sharedPtr)
    {
        for (auto iter = m_Cache.begin(); iter != m_Cache.end(); iter++)
        {
            if (iter->get()->GetPtr() == index)
            {
                m_Cache.resize(std::remove(m_Cache.begin(), m_Cache.end(), iter) - m_Cache.begin());
            }
        }
    }
    void ClearCache() { m_Cache.clear(); }

private:
    std::vector<SHAREDPTR> m_Cache;
};

// Creating necessary Caches for all Editor <-> Python Objects.
typedef SPyWrapperCache<pPyGameObject, CBaseObject*> PyObjCache;
typedef SPyWrapperCache<pPyGameLayer, CObjectLayer*> PyLyrCache;
typedef SPyWrapperCache<pPyGameMaterial, CMaterial*> PyMatCache;
typedef SPyWrapperCache<pPyGameSubMaterial, CMaterial*> PySubMatCache;
typedef SPyWrapperCache<pPyGameTexture, SEfResTexture*> PyTextureCache;
typedef SPyWrapperCache<pPyGameVegetation, CVegetationObject*> PyVegCache;

/////////////////////////////////////////////////////////////////////////
// Python List and Dictionary Wrapper to avoid issues inherent in using the map indexing suite.
template<class KEY, class VAL>
struct map_item
{
    typedef std::map<KEY, VAL> Map;

    static VAL get(Map& self, const KEY idx)
    {
        if (self.find(idx) == self.end())
        {
            PyErr_SetString(PyExc_KeyError, "Map key not found");
            boost::python::throw_error_already_set();
        }
        return self[idx];
    }

    static void set(Map& self, const KEY idx, const VAL val) { self[idx] = val; }
    static void del(Map& self, const KEY n)                  { self.erase(n); }
    static bool in (Map const& self, const KEY n)            { return self.find(n) != self.end(); }

    static boost::python::list keys(Map const& self)
    {
        boost::python::list t;
        for (typename Map::const_iterator it = self.begin(); it != self.end(); ++it)
        {
            t.append(it->first);
        }
        return t;
    }

    static boost::python::list values(Map const& self)
    {
        boost::python::list t;
        for (typename Map::const_iterator it = self.begin(); it != self.end(); ++it)
        {
            t.append(it->second);
        }
        return t;
    }

    static boost::python::list items(Map const& self)
    {
        boost::python::list t;
        for (typename Map::const_iterator it = self.begin(); it != self.end(); ++it)
        {
            t.append(boost::python::make_tuple(it->first, it->second));
        }
        return t;
    }
};

#define STL_MAP_WRAPPING_PTR(KEY_TYPE, VALUE_TYPE, PYTHON_TYPE_NAME)                                                             \
    boost::python::class_<std::pair<const KEY_TYPE, VALUE_TYPE> >((std::string(PYTHON_TYPE_NAME) + std::string("DATA")).c_str()) \
        .def_readonly ("key", &std::pair<const KEY_TYPE, VALUE_TYPE>::first)                                                     \
        .def_readwrite("value", &std::pair<const KEY_TYPE, VALUE_TYPE>::second)                                                  \
    ;                                                                                                                            \
    boost::python::class_<std::map<KEY_TYPE, VALUE_TYPE> >(PYTHON_TYPE_NAME)                                                     \
        .def("__len__", &std::map<KEY_TYPE, VALUE_TYPE>::size)                                                                   \
        .def("__iter__", boost::python::iterator<std::map<KEY_TYPE, VALUE_TYPE>, boost::python::return_internal_reference<> >()) \
        .def("__getitem__", &map_item<KEY_TYPE, VALUE_TYPE>().get, boost::python::return_internal_reference<>())                 \
        .def("__setitem__", &map_item<KEY_TYPE, VALUE_TYPE>().set)                                                               \
        .def("__delitem__", &map_item<KEY_TYPE, VALUE_TYPE>().del)                                                               \
        .def("__contains__", &map_item<KEY_TYPE, VALUE_TYPE>().in)                                                               \
        .def("clear", &std::map<KEY_TYPE, VALUE_TYPE>::clear)                                                                    \
        .def("has_key", &map_item<KEY_TYPE, VALUE_TYPE>().in)                                                                    \
        .def("keys", &map_item<KEY_TYPE, VALUE_TYPE>().keys)                                                                     \
        .def("values", &map_item<KEY_TYPE, VALUE_TYPE>().values)                                                                 \
        .def("items", &map_item<KEY_TYPE, VALUE_TYPE>().items)                                                                   \
    ;

#define STL_MAP_WRAPPING(KEY_TYPE, VALUE_TYPE, PYTHON_TYPE_NAME)                                                                 \
    boost::python::class_<std::pair<const KEY_TYPE, VALUE_TYPE> >((std::string(PYTHON_TYPE_NAME) + std::string("DATA")).c_str()) \
        .def_readonly ("key", &std::pair<const KEY_TYPE, VALUE_TYPE>::first)                                                     \
        .def_readwrite("value", &std::pair<const KEY_TYPE, VALUE_TYPE>::second)                                                  \
    ;                                                                                                                            \
    boost::python::class_<std::map<KEY_TYPE, VALUE_TYPE> >(PYTHON_TYPE_NAME)                                                     \
        .def("__len__", &std::map<KEY_TYPE, VALUE_TYPE>::size)                                                                   \
        .def("__iter__", boost::python::iterator<std::map<KEY_TYPE, VALUE_TYPE>, boost::python::return_internal_reference<> >()) \
        .def("__getitem__", &map_item<KEY_TYPE, VALUE_TYPE>().get)                                                               \
        .def("__setitem__", &map_item<KEY_TYPE, VALUE_TYPE>().set)                                                               \
        .def("__delitem__", &map_item<KEY_TYPE, VALUE_TYPE>().del)                                                               \
        .def("__contains__", &map_item<KEY_TYPE, VALUE_TYPE>().in)                                                               \
        .def("clear", &std::map<KEY_TYPE, VALUE_TYPE>::clear)                                                                    \
        .def("has_key", &map_item<KEY_TYPE, VALUE_TYPE>().in)                                                                    \
        .def("keys", &map_item<KEY_TYPE, VALUE_TYPE>().keys)                                                                     \
        .def("values", &map_item<KEY_TYPE, VALUE_TYPE>().values)                                                                 \
        .def("items", &map_item<KEY_TYPE, VALUE_TYPE>().items)                                                                   \
    ;


#ifdef USE_PYTHON_SCRIPTING
#define DECLARE_PYTHON_MODULE(moduleName)                                          \
    BOOST_PYTHON_MODULE(moduleName)                                                \
    {                                                                              \
        CAutoRegisterPythonCommandHelper::RegisterFunctionsForModule(#moduleName); \
    }                                                                              \
    CAutoRegisterPythonModuleHelper g_AutoRegPythonModuleHelper##moduleName(#moduleName, init##moduleName)

#define REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(functionPtr, moduleName, functionName, description, example) \
    void RegisterPythonCommand##moduleName##functionName()                                                     \
    {                                                                                                          \
        using namespace boost::python;                                                                         \
        def(#functionName, functionPtr);                                                                       \
    }                                                                                                          \
    CAutoRegisterPythonCommandHelper g_AutoRegPythonCmdHelper##moduleName##functionName(#moduleName, #functionName, RegisterPythonCommand##moduleName##functionName, description, example)

#define REGISTER_PYTHON_OVERLOAD_COMMAND(functionPtr, moduleName, functionName, functionOverload, description, example) \
    void RegisterPythonCommand##moduleName##functionName()                                                              \
    {                                                                                                                   \
        using namespace boost::python;                                                                                  \
        def(#functionName, functionPtr, functionOverload());                                                            \
    }                                                                                                                   \
    CAutoRegisterPythonCommandHelper g_AutoRegPythonCmdHelper##moduleName##functionName(#moduleName, #functionName, RegisterPythonCommand##moduleName##functionName, description, example)

#define REGISTER_ONLY_PYTHON_COMMAND(functionPtr, moduleName, functionName, description) \
    REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(functionPtr, moduleName, functionName, description, "")

#define REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(functionPtr, moduleName, functionName, description, example)                       \
    void RegisterCommand##moduleName##functionName(CEditorCommandManager & cmdMgr)                                              \
    {                                                                                                                           \
        CommandManagerHelper::RegisterCommand(&cmdMgr, #moduleName, #functionName, description, example, functor(functionPtr)); \
        cmdMgr.SetCommandAvailableInScripting(#moduleName, #functionName);                                                      \
    }                                                                                                                           \
    CAutoRegisterCommandHelper g_AutoRegCmdHelper##moduleName##functionName(RegisterCommand##moduleName##functionName);         \
    REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(functionPtr, moduleName, functionName, description, example)

#define REGISTER_PYTHON_COMMAND(functionPtr, moduleName, functionName, description) \
    REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(functionPtr, moduleName, functionName, description, "")

#define REGISTER_PYTHON_ENUM_BEGIN(enumType, moduleName, enumName)                                                                                                         \
    void RegisterPythonEnum##moduleName##enumName();                                                                                                                       \
    CAutoRegisterPythonCommandHelper g_AutoRegPythonEnumHelper##moduleName##enumName(#moduleName, #enumName, RegisterPythonEnum##moduleName##enumName, "enumeration", ""); \
    void RegisterPythonEnum##moduleName##enumName()                                                                                                                        \
    {                                                                                                                                                                      \
        using namespace boost::python;                                                                                                                                     \
        enum_<enumType>(#enumName)

#define REGISTER_PYTHON_ENUM_ITEM(item, name) .value(#name, item)

#define REGISTER_PYTHON_ENUM_END ; }
#else // USE_PYTHON_SCRIPTING
#define DECLARE_PYTHON_MODULE(moduleName)
#define REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(functionPtr, moduleName, functionName, description, example)
#define REGISTER_PYTHON_OVERLOAD_COMMAND(functionPtr, moduleName, functionName, functionOverload, description, example)
#define REGISTER_ONLY_PYTHON_COMMAND(functionPtr, moduleName, functionName, description)
#define REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(functionPtr, moduleName, functionName, description, example)                       \
    void RegisterCommand##moduleName##functionName(CEditorCommandManager & cmdMgr)                                              \
    {                                                                                                                           \
        CommandManagerHelper::RegisterCommand(&cmdMgr, #moduleName, #functionName, description, example, functor(functionPtr)); \
        cmdMgr.SetCommandAvailableInScripting(#moduleName, #functionName);                                                      \
    }                                                                                                                           \
    CAutoRegisterCommandHelper g_AutoRegCmdHelper##moduleName##functionName(RegisterCommand##moduleName##functionName)
#define RGEISTER_PYTHON_COMMAND(functionPtr, moduleName, functionName, description) \
    REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(functionPtr, moduleName, functionName, description, "")
#define REGISTER_PYTHON_ENUM_BEGIN(enumType, moduleName, enumName)
#define REGISTER_PYTHON_ENUM_ITEM(item, name)
#define REGISTER_PYTHON_ENUM_END
#define PyMODINIT_FUNC void
#endif // USE_PYTHON_SCRIPTING

// Key Functions for use in Editor <-> Python Interface.
namespace PyScript
{
    struct IPyScriptListener
    {
        virtual void OnStdOut(const char* pString) = 0;
        virtual void OnStdErr(const char* pString) = 0;
    };

    // Initialize python for use in sandbox
    void InitializePython(const QString& rootEngineDir);

    // Shutdown python
    void ShutdownPython();

    // Acquire the global lock and set the thread state.
    void AcquirePythonLock();

    // Release the global lock and clear the thread state.
    void ReleasePythonLock();

    // Returns true if this thread has acquired the python lock already
    bool HasPythonLock();

    // Registers a IPyScriptListener listener
    void RegisterListener(IPyScriptListener* pListener);

    // Removes a IPyScriptListener listener
    void RemoveListener(IPyScriptListener* pListener);

    // Executes a given python string with a special logging.
    void Execute(const char* pString, ...);

    // Prints a message to the script terminal, if it's opened
    void PrintMessage(const char* pString);

    // Prints an error to the script terminal, if it's opened
    void PrintError(const char* pString);

    // Gets the value of a python variable as string/int/float/bool
    const char* GetAsString(const char* varName);
    int GetAsInt(const char* varName);
    float GetAsFloat(const char* varName);
    bool GetAsBool(const char* varName);

    // Registers all necessary python classes into the CryClasses module.
    void InitCppClasses();

    // Import all python exception types for later error handling.
    void InitCppExceptions();

    // Object Creation functions for External Use Only.
    pPyGameObject CreatePyGameObject(CBaseObject* pObj);
    pPyGameLayer CreatePyGameLayer(CObjectLayer* pLayer);
    pPyGameMaterial CreatePyGameMaterial(CMaterial* pMat);
    pPyGameSubMaterial CreatePyGameSubMaterial(CMaterial* pSubMat, int id);
    pPyGameTexture CreatePyGameTexture(SEfResTexture* pTex);
    pPyGameVegetation CreatePyGameVegetation(CVegetationObject* pVeg);
}

#endif // CRYINCLUDE_EDITOR_UTIL_BOOSTPYTHONHELPERS_H

