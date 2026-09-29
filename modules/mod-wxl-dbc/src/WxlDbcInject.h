/*
 * mod-wxl-dbc — inject continuation rows into an AzerothCore DBC store.
 */

#ifndef MOD_WXL_DBC_INJECT_H
#define MOD_WXL_DBC_INJECT_H

#include "DBCFileLoader.h"
#include "DBCStore.h"
#include "Log.h"

#include <vector>

namespace ModWxlDbc
{
    inline std::vector<char*>& GetStringPool()
    {
        static std::vector<char*> pool;
        return pool;
    }

    template<typename T>
    uint32 InjectContinuationFile(DBCStorage<T>& store, char const* path, char const* format)
    {
        DBCFileLoader dbc;
        if (!dbc.Load(path, format))
        {
            LOG_ERROR("module.wxl-dbc", "Failed to load continuation file: {}", path);
            return 0;
        }

        if (DBCFileLoader::GetFormatRecordSize(format) != sizeof(T))
        {
            LOG_ERROR("module.wxl-dbc", "Structure size mismatch for {} (WDBC layout vs core struct)", path);
            return 0;
        }

        uint32 indexTableSize = 0;
        char** indexTable = nullptr;
        char* dataTable = dbc.AutoProduceData(format, indexTableSize, indexTable);
        if (!dataTable || !indexTable)
        {
            LOG_ERROR("module.wxl-dbc", "Failed to parse rows in continuation file: {}", path);
            return 0;
        }

        if (char* stringBlock = dbc.AutoProduceStrings(format, dataTable))
            GetStringPool().push_back(stringBlock);

        int32 indexPos = -1;
        DBCFileLoader::GetFormatRecordSize(format, &indexPos);

        uint32 injected = 0;
        for (uint32 row = 0; row < dbc.GetNumRows(); ++row)
        {
            uint32 id = (indexPos >= 0)
                ? dbc.getRecord(row).getUInt(static_cast<std::size_t>(indexPos))
                : row;

            char* raw = indexTable[id];
            if (!raw)
                continue;

            T* entry = new T(*reinterpret_cast<T*>(raw));
            store.SetEntry(id, entry);
            ++injected;
        }

        delete[] dataTable;
        delete[] indexTable;
        return injected;
    }
}

#endif
