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
    TerrainFactoryHidden() 
    {
        assert(true);
    }
    ~TerrainFactoryHidden()
    {
        assert(true);
    }

    static TerrainFactoryHidden &getInstance()
    {
        static TerrainFactoryHidden hidden;
        
        return hidden;
    }

    std::vector<TerrainInfo> terrains;
};

//TerrainFactoryHidden g_terrainFactoryHidden;

size_t TerrainFactory::getTerrainId(const char *name)
{
    for(size_t i=0; i<TerrainFactoryHidden::getInstance().terrains.size(); ++i)
    {
        if(TerrainFactoryHidden::getInstance().terrains[i].name==name)
            return i;
    }
    return std::numeric_limits<size_t>::max();
}

const char *TerrainFactory::getTerrainName(size_t id)
{
    assert(id>=0&&id<TerrainFactoryHidden::getInstance().terrains.size());

    return TerrainFactoryHidden::getInstance().terrains[id].name.c_str();
}

size_t TerrainFactory::terrainSize()
{
    return TerrainFactoryHidden::getInstance().terrains.size();
}

ITerrain *TerrainFactory::create(size_t id, const STerrainInfo &terrainInfo)
{
    assert(id>=0&&id<TerrainFactoryHidden::getInstance().terrains.size());

    return TerrainFactoryHidden::getInstance().terrains[id].func(terrainInfo);
}

size_t TerrainFactory::registerTerrain(const char *name, FactoryFunction func)
{
    size_t id=TerrainFactoryHidden::getInstance().terrains.size();

    TerrainFactoryHidden::getInstance().terrains.emplace_back(name, func);
    return id;
}