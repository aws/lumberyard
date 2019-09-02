
#pragma once

#include <ITerrain.h>

template<typename _Class, typename _Interface>
class RegisterTerrain:public _Interface
{
public:
    static ITerrain *create(const STerrainInfo &terrainInfo) { return (ITerrain *)new _Class(terrainInfo); }

    static size_t m_terrainTypeId;
};

struct TerrainFactoryHidden;
class TerrainFactory
{
private:
    TerrainFactory() {};
public:
    ~TerrainFactory() {};

    typedef ITerrain *(*FactoryFunction)(const STerrainInfo &);

    AZ_DLL_EXPORT static size_t getTerrainId(const char *name);
    AZ_DLL_EXPORT static const char *getTerrainName(size_t id);
    AZ_DLL_EXPORT static size_t terrainSize();

    AZ_DLL_EXPORT static std::vector<std::string> getTerrainNames()
    {
        std::vector<std::string> names;

        for(size_t i=0; i<terrainSize(); ++i)
        {
            names.emplace_back(getTerrainName(i));
        }
        return names;
    }

    static ITerrain *create(size_t id, const STerrainInfo &terrainInfo);
    AZ_DLL_EXPORT static size_t registerTerrain(const char *name, FactoryFunction func);
    template<typename _Class, typename _Interface> 
    static size_t registerTerrain()
    {
        return registerTerrain(_Class::Name(), &RegisterTerrain<_Class, _Interface>::create);
    }

private:
    static TerrainFactoryHidden *hidden;
};

template<typename _Class, typename _Interface>
size_t RegisterTerrain<_Class, _Interface>::m_terrainTypeId=\
TerrainFactory::registerTerrain<_Class, _Interface>();
