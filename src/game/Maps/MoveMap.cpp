/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Log.h"
#include "World.h"
#include "VMapFactory.h"
#include "MoveMap.h"
#include "MoveMapSharedDefines.h"

namespace MMAP
{
// ######################## MMapFactory ########################
// our global singelton copy
MMapManager *g_MMapManager = nullptr;

MMapManager* MMapFactory::createOrGetMMapManager()
{
    if (g_MMapManager == nullptr)
        g_MMapManager = new MMapManager();

    return g_MMapManager;
}

void MMapFactory::clear()
{
    if (g_MMapManager)
    {
        delete g_MMapManager;
        g_MMapManager = nullptr;
    }
}

// ######################## MMapManager ########################
MMapManager::~MMapManager()
{
    for (const auto& loadedMMap : loadedMMaps)
        delete loadedMMap.second;

    // by now we should not have maps loaded
    // if we had, tiles in MMapData->mmapLoadedTiles, their actual data is lost!
}

bool MMapManager::loadMapData(uint32 mapId)
{
    std::shared_lock<std::shared_mutex> rlock(loadedMMaps_lock);
    // we already have this map loaded?
    if (loadedMMaps.find(mapId) != loadedMMaps.end())
    {
        return true;
    }
    rlock.unlock();

    if (!sWorld.getConfig(CONFIG_BOOL_MMAP_ENABLED))
        return false;

    // load and init dtNavMesh - read parameters from file
    char fileSuffix[64];
    snprintf(fileSuffix, sizeof(fileSuffix), "mmaps/%03u.mmap", mapId);
    std::string fileName = sWorld.GetDataPath();
    fileName += fileSuffix;

    FILE* file = fopen(fileName.c_str(), "rb");
    if (!file)
    {
        DEBUG_LOG("MMAP:loadMapData: Error: Could not open mmap file '%s'", fileName.c_str());
        return false;
    }

    dtNavMeshParams params;
    fread(&params, sizeof(dtNavMeshParams), 1, file);
    fclose(file);

    dtNavMesh* mesh = dtAllocNavMesh();
    MANGOS_ASSERT(mesh);
    dtStatus dtResult = mesh->init(&params);
    if (dtStatusFailed(dtResult))
    {
        dtFreeNavMesh(mesh);
        sLog.outError("MMAP:loadMapData: Failed to initialize dtNavMesh for mmap %03u from file %s with %u tiles. Result 0x%x.", mapId, fileName.c_str(), params.maxTiles, dtResult);
        return false;
    }

    DETAIL_LOG("MMAP:loadMapData: Loaded %03i.mmap", mapId);

    // store inside our map list
    MMapData* mmap_data = new MMapData(mesh);
    mmap_data->mmapLoadedTiles.clear();

    std::unique_lock<std::shared_mutex> wlock(loadedMMaps_lock);
    if (loadedMMaps.find(mapId) == loadedMMaps.end())
        loadedMMaps.insert(std::pair<uint32, MMapData*>(mapId, mmap_data));
    else
        delete mmap_data;

    return true;
}

uint32 MMapManager::packTileID(int32 x, int32 y)
{
    return uint32(x << 16 | y);
}

bool MMapManager::loadMap(uint32 mapId, int32 x, int32 y)
{
    // make sure the mmap is loaded and ready to load tiles
    if (!loadMapData(mapId))
        return false;

    // get this mmap data
    std::shared_lock<std::shared_mutex> rlock(loadedMMaps_lock);
    MMapData* mmap = loadedMMaps[mapId];
    rlock.unlock();
    MANGOS_ASSERT(mmap->navMesh);

    // check if we already have this tile loaded
    uint32 packedGridPos = packTileID(x, y);
    std::unique_lock<std::mutex> wlock(mmap->tilesLoading_lock);
    if (mmap->mmapLoadedTiles.find(packedGridPos) != mmap->mmapLoadedTiles.end())
        return false;

    // load this tile :: mmaps/MMMXXYY.mmtile
    char fileSuffix[64];
    snprintf(fileSuffix, sizeof(fileSuffix), "mmaps/%03u%02i%02i.mmtile", mapId, y, x);
    std::string fileName = sWorld.GetDataPath();
    fileName += fileSuffix;

    FILE *file = fopen(fileName.c_str(), "rb");
    if (!file)
    {
        //mmaps not generated on every tile. But it's often generating, where vmap placed (most of the time)
        if (VMAP::VMapFactory::createOrGetVMapManager()->existsMap((sWorld.GetDataPath() + "vmaps").c_str(), mapId, x, y))
        {
            DEBUG_LOG("MMAP:loadMap: Could not open mmtile file '%s' and vmap is exist in this tile", fileName.c_str());
        }
        return false;
    }

    // read header
    MmapTileHeader fileHeader;
    fread(&fileHeader, sizeof(MmapTileHeader), 1, file);

    if (fileHeader.mmapMagic != MMAP_MAGIC)
    {
        sLog.outError("MMAP:loadMap: Bad header in mmap %03u%02i%02i.mmtile", mapId, x, y);
        fclose(file);
        return false;
    }

    if (fileHeader.mmapVersion != MMAP_VERSION)
    {
        sLog.outError("MMAP:loadMap: %03u%02i%02i.mmtile was built with generator v%i, expected v%i",
                      mapId, x, y, fileHeader.mmapVersion, MMAP_VERSION);
        fclose(file);
        return false;
    }

    unsigned char* data = (unsigned char*)dtAlloc(fileHeader.size, DT_ALLOC_PERM);
    MANGOS_ASSERT(data);

    size_t result = fread(data, fileHeader.size, 1, file);
    if (!result)
    {
        sLog.outError("MMAP:loadMap: Bad header or data in mmap %03u%02i%02i.mmtile", mapId, x, y);
        fclose(file);
        return false;
    }

    fclose(file);
    dtTileRef tileRef = 0;

    // memory allocated for data is now managed by detour, and will be deallocated when the tile is removed
    dtStatus dResult = mmap->navMesh->addTile(data, fileHeader.size, DT_TILE_FREE_DATA, 0, &tileRef);
    if (dtStatusSucceed(dResult))
    {
        mmap->mmapLoadedTiles.insert(std::pair<uint32, dtTileRef>(packedGridPos, tileRef));
        ++loadedTiles;
        return true;
    }
    else
    {
        sLog.outError("MMAP:loadMap: Could not load %03u%02i%02i.mmtile into navmesh [result 0x%x]", mapId, x, y, dResult);
        dtFree(data);
        return false;
    }

    return false;
}

bool MMapManager::unloadMap(uint32 mapId, int32 x, int32 y)
{
    // check if we have this map loaded
    auto const mapIt = loadedMMaps.find(mapId);
    if (mapIt == loadedMMaps.end())
    {
        // file may not exist, therefore not loaded
        DEBUG_LOG("MMAP:unloadMap: Asked to unload not loaded navmesh map. %03u%02i%02i.mmtile", mapId, x, y);
        return false;
    }

    MMapData* mmap = mapIt->second;

    // check if we have this tile loaded
    uint32 packedGridPos = packTileID(x, y);
    auto const tileIt = mmap->mmapLoadedTiles.find(packedGridPos);
    if (tileIt == mmap->mmapLoadedTiles.end())
    {
        // file may not exist, therefore not loaded
        DEBUG_LOG("MMAP:unloadMap: Asked to unload not loaded navmesh tile. %03u%02i%02i.mmtile", mapId, x, y);
        return false;
    }

    dtTileRef tileRef = tileIt->second;

    // unload, and mark as non loaded
    dtStatus dtResult = mmap->navMesh->removeTile(tileRef, nullptr, nullptr);
    if (dtStatusFailed(dtResult))
    {
        // this is technically a memory leak
        // if the grid is later reloaded, dtNavMesh::addTile will return error but no extra memory is used
        // we cannot recover from this error - assert out
        sLog.outError("MMAP:unloadMap: Could not unload %03u%02i%02i.mmtile from navmesh", mapId, x, y);
        MANGOS_ASSERT(false);
    }
    else
    {
        mmap->mmapLoadedTiles.erase(tileIt);
        --loadedTiles;
        return true;
    }

    return false;
}

bool MMapManager::unloadMap(uint32 mapId)
{
    auto const mapIt = loadedMMaps.find(mapId);
    if (mapIt == loadedMMaps.end())
    {
        // file may not exist, therefore not loaded
        DEBUG_LOG("MMAP:unloadMap: Asked to unload not loaded navmesh map %03u", mapId);
        return false;
    }

    // unload all tiles from given map
    MMapData* mmap = mapIt->second;
    for (MMapTileSet::iterator i = mmap->mmapLoadedTiles.begin(); i != mmap->mmapLoadedTiles.end(); ++i)
    {
        uint32 x = (i->first >> 16);
        uint32 y = (i->first & 0x0000FFFF);
        dtStatus dtResult = mmap->navMesh->removeTile(i->second, nullptr, nullptr);
        if (dtStatusFailed(dtResult))
            sLog.outError("MMAP:unloadMap: Could not unload %03u%02i%02i.mmtile from navmesh", mapId, x, y);
        else
            --loadedTiles;
    }

    delete mmap;
    loadedMMaps.erase(mapIt);
    DETAIL_LOG("MMAP:unloadMap: Unloaded %03i.mmap", mapId);

    return true;
}

bool MMapManager::unloadMapInstance(uint32 mapId, std::thread::id instanceId)
{
    // check if we have this map loaded
    auto const mapIt = loadedMMaps.find(mapId);
    if (mapIt == loadedMMaps.end())
    {
        // file may not exist, therefore not loaded
        DEBUG_LOG("MMAP:unloadMapInstance: Asked to unload not loaded navmesh map %03u", mapId);
        return false;
    }

    MMapData* mmap = mapIt->second;
    auto const queryIt = mmap->navMeshQueries.find(instanceId);
    if (queryIt == mmap->navMeshQueries.end())
    {
        DEBUG_LOG("MMAP:unloadMapInstance: Asked to unload not loaded dtNavMeshQuery mapId %03u instanceId %u", mapId, instanceId);
        return false;
    }

    dtNavMeshQuery* query = queryIt->second;

    dtFreeNavMeshQuery(query);
    mmap->navMeshQueries.erase(queryIt);
    DETAIL_LOG("MMAP:unloadMapInstance: Unloaded mapId %03u instanceId %u", mapId, instanceId);

    return true;
}

dtNavMesh const* MMapManager::GetNavMesh(uint32 mapId)
{
    auto const mapIt = loadedMMaps.find(mapId);
    if (mapIt == loadedMMaps.end())
        return nullptr;

    return mapIt->second->navMesh;
}

dtNavMeshQuery const* MMapManager::GetNavMeshQuery(uint32 mapId)
{
    auto const mapIt = loadedMMaps.find(mapId);
    if (mapIt == loadedMMaps.end())
        return nullptr;

    std::thread::id tid= std::this_thread::get_id();
    MMapData* mmap = mapIt->second;
    std::shared_lock<std::shared_mutex> lock(mmap->navMeshQueries_lock);

    NavMeshQuerySet::iterator it = mmap->navMeshQueries.find(tid);
    dtNavMeshQuery* navMeshQuery = nullptr;
    if (it == mmap->navMeshQueries.end())
    {
        lock.unlock();
        std::unique_lock<std::shared_mutex> ulock(mmap->navMeshQueries_lock);

        // allocate mesh query
        navMeshQuery = dtAllocNavMeshQuery();
        MANGOS_ASSERT(navMeshQuery);
        dtStatus dtResult = navMeshQuery->init(mmap->navMesh, 2048);
        if (dtStatusFailed(dtResult))
        {
            ulock.unlock();
            dtFreeNavMeshQuery(navMeshQuery);
            sLog.outError("MMAP:GetNavMeshQuery: Failed to initialize dtNavMeshQuery for mapId %03u thread %u", mapId, tid);
            return nullptr;
        }

        DETAIL_LOG("MMAP:GetNavMeshQuery: created dtNavMeshQuery for mapId %03u thread %u", mapId, tid);
        mmap->navMeshQueries.insert(std::pair<std::thread::id, dtNavMeshQuery*>(tid, navMeshQuery));
    }
    else
        navMeshQuery = it->second;

    return navMeshQuery;
}

bool MMapManager::loadGameObject(uint32 displayId)
{
    // we already have this map loaded?
    if (loadedModels.find(displayId) != loadedModels.end())
        return true;

    // load and init dtNavMesh - read parameters from file
    char fileSuffix[64];
    snprintf(fileSuffix, sizeof(fileSuffix), "mmaps/go%04u.mmap", displayId);
    std::string fileName = sWorld.GetDataPath();
    fileName += fileSuffix;

    FILE* file = fopen(fileName.c_str(), "rb");
    if (!file)
    {
        DEBUG_LOG("MMAP:loadGameObject: Error: Could not open mmap file %s", fileName.c_str());
        return false;
    }

    MmapTileHeader fileHeader;
    fread(&fileHeader, sizeof(MmapTileHeader), 1, file);

    if (fileHeader.mmapMagic != MMAP_MAGIC)
    {
        sLog.outError("MMAP:loadGameObject: Bad header in mmap %s", fileName.c_str());
        fclose(file);
        return false;
    }

    if (fileHeader.mmapVersion != MMAP_VERSION)
    {
        sLog.outError("MMAP:loadGameObject: %s was built with generator v%i, expected v%i",
                      fileName.c_str(), fileHeader.mmapVersion, MMAP_VERSION);
        fclose(file);
        return false;
    }
    unsigned char* data = (unsigned char*)dtAlloc(fileHeader.size, DT_ALLOC_PERM);
    MANGOS_ASSERT(data);

    size_t result = fread(data, fileHeader.size, 1, file);
    if (!result)
    {
        sLog.outError("MMAP:loadGameObject: Bad header or data in mmap %s", fileName.c_str());
        fclose(file);
        return false;
    }

    fclose(file);

    dtNavMesh* mesh = dtAllocNavMesh();
    MANGOS_ASSERT(mesh);
    dtStatus r = mesh->init(data, fileHeader.size, DT_TILE_FREE_DATA);
    if (dtStatusFailed(r))
    {
        dtFreeNavMesh(mesh);
        sLog.outError("MMAP:loadGameObject: Failed to initialize dtNavMesh from file %s. Result 0x%x.", fileName.c_str(), r);
        return false;
    }
    DETAIL_LOG("MMAP:loadGameObject: Loaded file %s [size=%u]", fileName.c_str(), fileHeader.size);

    MMapData* mmap_data = new MMapData(mesh);
    loadedModels.insert(std::pair<uint32, MMapData*>(displayId, mmap_data));
    return true;
}

dtNavMeshQuery const* MMapManager::GetModelNavMeshQuery(uint32 displayId)
{
    auto const modelIt = loadedModels.find(displayId);
    if (modelIt == loadedModels.end())
        return nullptr;

    std::thread::id tid = std::this_thread::get_id();
    MMapData* mmap = modelIt->second;
    std::shared_lock<std::shared_mutex> lock(mmap->navMeshQueries_lock);

    NavMeshQuerySet::const_iterator it = mmap->navMeshQueries.find(tid);
    if (it != mmap->navMeshQueries.end())
        return it->second;

    lock.unlock();
    std::unique_lock<std::shared_mutex> ulock(mmap->navMeshQueries_lock);
    it = mmap->navMeshQueries.find(tid);
    if (it != mmap->navMeshQueries.end())
        return it->second;

    dtNavMeshQuery* query = dtAllocNavMeshQuery();
    MANGOS_ASSERT(query);
    if (dtStatusFailed(query->init(mmap->navMesh, 2048)))
    {
        dtFreeNavMeshQuery(query);
        sLog.outError("MMAP:GetNavMeshQuery: Failed to initialize dtNavMeshQuery for displayid %03u tid %u", displayId, tid);
        return nullptr;
    }

    DETAIL_LOG("MMAP:GetNavMeshQuery: created dtNavMeshQuery for displayid %03u tid %u", displayId, tid);
    mmap->navMeshQueries.emplace(tid, query);
    return query;
}
}
