#include <StdAfx.h>
#include "EditorTerrainFactory.h"

struct EditorTerrainInfo
{
    EditorTerrainInfo(const char *name, EditorTerrainFactory::FactoryFunction func):name(name), func(func) {}

    std::string name;
    EditorTerrainFactory::FactoryFunction func;
};

struct EditorTerrainFactoryHidden
{
    std::vector<EditorTerrainInfo> terrains;

    static EditorTerrainFactoryHidden &getInstance()
    {
        static EditorTerrainFactoryHidden hidden;

        return hidden;
    }
};

size_t EditorTerrainFactory::getTerrainId(const char *name)
{
    for(size_t i=0; i<EditorTerrainFactoryHidden::getInstance().terrains.size(); ++i)
    {
        if(EditorTerrainFactoryHidden::getInstance().terrains[i].name==name)
            return i;
    }
    return std::numeric_limits<size_t>::max();
}

const char *EditorTerrainFactory::getTerrainName(size_t id)
{
    assert(id>=0&&id<EditorTerrainFactoryHidden::getInstance().terrains.size());

    return EditorTerrainFactoryHidden::getInstance().terrains[id].name.c_str();
}

size_t EditorTerrainFactory::terrainSize()
{
    return EditorTerrainFactoryHidden::getInstance().terrains.size();
}

IEditorTerrain *EditorTerrainFactory::create(size_t id)
{
    assert(id>=0&&id<EditorTerrainFactoryHidden::getInstance().terrains.size());

    return EditorTerrainFactoryHidden::getInstance().terrains[id].func();
}

size_t EditorTerrainFactory::registerTerrain(const char *name, FactoryFunction func)
{
    size_t id=EditorTerrainFactoryHidden::getInstance().terrains.size();

    EditorTerrainFactoryHidden::getInstance().terrains.emplace_back(name, func);
    return id;
}