
#pragma once

#include <Editor/Terrain/IEditorTerrain.h>

template<typename _Class, typename _Interface>
class RegisterEditorTerrain:public _Interface
{
public:
    static IEditorTerrain *create() { return (IEditorTerrain *)new _Class(); }

    static size_t m_terrainTypeId;
};

struct EditorTerrainFactoryHidden;
class EditorTerrainFactory
{
private:
    EditorTerrainFactory() {};
public:
    ~EditorTerrainFactory() {};

    typedef IEditorTerrain *(*FactoryFunction)();

    AZ_DLL_EXPORT static size_t getTerrainId(const char *name);
    
    static size_t getTerrainId(const std::string &name)
    {
        return getTerrainId(name.c_str());
    }

    AZ_DLL_EXPORT static const char *getTerrainName(size_t id);
    AZ_DLL_EXPORT static size_t terrainSize();

    static std::vector<std::string> getTerrainNames()
    {
        std::vector<std::string> names;

        for(size_t i=0; i<terrainSize(); ++i)
        {
            names.emplace_back(getTerrainName(i));
        }
        return names;
    }

    static IEditorTerrain *create(size_t id);
    AZ_DLL_EXPORT static size_t registerTerrain(const char *name, FactoryFunction func);
    template<typename _Class, typename _Interface> 
    static size_t registerTerrain()
    {
        return registerTerrain(_Class::Name(), &RegisterEditorTerrain<_Class, _Interface>::create);
    }

private:
    static EditorTerrainFactoryHidden *hidden;
};

template<typename _Class, typename _Interface>
size_t RegisterEditorTerrain<_Class, _Interface>::m_terrainTypeId=\
EditorTerrainFactory::registerTerrain<_Class, _Interface>();
