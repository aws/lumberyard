#include "StdAfx.h"
#include "TerrainFactory.h"

struct TerrainInfo
{
    TerrainInfo(const char *name, TerrainFactory::FactoryFunction func):name(name), func(func) {}

    std::string name;
    TerrainFactory::FactoryFunction func;
};

struct TerrainFactoryHidden
{
    std::vector<TerrainInfo> terrains;
};
TerrainFactoryHidden g_terrainFactoryHidden;

size_t TerrainFactory::getTerrainId(const char *name)
{
    for(size_t i=0; i<g_terrainFactoryHidden.terrains.size(); ++i)
    {
        if(g_terrainFactoryHidden.terrains[i].name==name)
            return i;
    }
    return std::numeric_limits<size_t>::max();
}

const char *TerrainFactory::getTerrainName(size_t id)
{
    assert(id>=0&&id<g_terrainFactoryHidden.terrains.size());

    return g_terrainFactoryHidden.terrains[id].name.c_str();
}

size_t TerrainFactory::terrainSize()
{
    return g_terrainFactoryHidden.terrains.size();
}

ITerrain *TerrainFactory::create(size_t id, const STerrainInfo &terrainInfo)
{
    assert(id>=0&&id<g_terrainFactoryHidden.terrains.size());

    return g_terrainFactoryHidden.terrains[id].func(terrainInfo);
}

size_t TerrainFactory::registerTerrain(const char *name, FactoryFunction func)
{
    size_t id=g_terrainFactoryHidden.terrains.size();

    g_terrainFactoryHidden.terrains.emplace_back(name, func);
    return id;
}