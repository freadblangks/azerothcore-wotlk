# mod-wxl-dbc

Load WXL-style DBC continuation files into AzerothCore **in memory** after `LoadDBCStores()`.

- Base `data/dbc/*.dbc` files are **never modified**
- Continuation files live in a separate folder (default `data/dbc-continuations/`)
- Same naming and merge rules as the WXL client extension (`wxl-extended-dbc`)
- New row IDs and single-row overrides both work before world startup validation

---

## Prerequisites

1. Copy this folder to your AzerothCore tree as `modules/mod-wxl-dbc/` (from `server-files/module/mod-wxl-dbc/` in this repository).
2. **One-time core patch** (4 files, below). Required before compile. Short reference: [../../core-patch/README.md](../../core-patch/README.md).
3. Copy `conf/mod_wxl_dbc.conf.dist` into your `etc/modules/` or merge into `worldserver.conf`.

---

## Core patch (required, apply before building)

**4 files. Copy-paste each change below.** Search the file for the **NOW** block; replace it with **CHANGE TO**.

Then rebuild `worldserver`.

---

### File 1: `src/server/game/Scripting/ScriptDefines/WorldScript.h`

#### Edit 1a: end of `enum WorldHook`

**NOW:**
```cpp
    WORLDHOOK_ON_BEFORE_FINALIZE_PLAYER_WORLD_SESSION,
    WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED,
    WORLDHOOK_END
```

**CHANGE TO:**
```cpp
    WORLDHOOK_ON_BEFORE_FINALIZE_PLAYER_WORLD_SESSION,
    WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED,
    WORLDHOOK_ON_AFTER_LOAD_DBC_STORES,
    WORLDHOOK_END
```

#### Edit 1b: end of `class WorldScript`

**NOW:**
```cpp
    /**
     * @brief This hook runs after all scripts loading and before itialized
     */
    virtual void OnBeforeWorldInitialized() { }
};
```

**CHANGE TO:**
```cpp
    /**
     * @brief This hook runs after all scripts loading and before itialized
     */
    virtual void OnBeforeWorldInitialized() { }

    virtual void OnAfterLoadDBCStores() { }
};
```

---

### File 2: `src/server/game/Scripting/ScriptDefines/WorldScript.cpp`

#### Edit 2: after `OnBeforeWorldInitialized()`

**NOW:**
```cpp
void ScriptMgr::OnBeforeWorldInitialized()
{
    CALL_ENABLED_HOOKS(WorldScript, WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED, script->OnBeforeWorldInitialized());
}

WorldScript::WorldScript(char const* name, std::vector<uint16> enabledHooks)
```

**CHANGE TO:**
```cpp
void ScriptMgr::OnBeforeWorldInitialized()
{
    CALL_ENABLED_HOOKS(WorldScript, WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED, script->OnBeforeWorldInitialized());
}

void ScriptMgr::OnAfterLoadDBCStores()
{
    CALL_ENABLED_HOOKS(WorldScript, WORLDHOOK_ON_AFTER_LOAD_DBC_STORES, script->OnAfterLoadDBCStores());
}

WorldScript::WorldScript(char const* name, std::vector<uint16> enabledHooks)
```

> **Note:** If search-replace fails here, your fork may use `const char*` instead of `char const*` on the `WorldScript::WorldScript(...)` line. Same thing. Keep whichever spelling your file already has; only insert the new `OnAfterLoadDBCStores()` function between the two blocks.

---

### File 3: `src/server/game/Scripting/ScriptMgr.h`

#### Edit 3: in `public: /* WorldScript */`

**NOW:**
```cpp
    void OnBeforeWorldInitialized();
    void OnAfterUnloadAllMaps();
```

**CHANGE TO:**
```cpp
    void OnBeforeWorldInitialized();
    void OnAfterLoadDBCStores();
    void OnAfterUnloadAllMaps();
```

---

### File 4: `src/server/game/World/World.cpp`

#### Edit 4: after `LoadDBCStores`

**NOW:**
```cpp
    LOG_INFO("server.loading", "Initialize Data Stores...");
    LoadDBCStores(_dataPath);
    DetectDBCLang();
```

**CHANGE TO:**
```cpp
    LOG_INFO("server.loading", "Initialize Data Stores...");
    LoadDBCStores(_dataPath);
    sScriptMgr->OnAfterLoadDBCStores();
    DetectDBCLang();
```

---

## Server layout

```
data/
  dbc/                          ← vanilla extract (never touched by this module)
  dbc-continuations/            ← your continuation files (configurable)
    wxl-dbc.manifest            ← optional ordering
    Spell.dbc1-test             ← example: new or overridden rows
    CreatureDisplayInfo.dbc1-myproject
    DBFilesClient/              ← subfolders also scanned
      ItemDisplayInfo.dbc2-artpass
```

Config (`mod_wxl_dbc.conf` or `worldserver.conf`):

```ini
[WxlDbc]
WxlDbc.Enable = 1
WxlDbc.ContinuationPath = dbc-continuations
```

---

## Continuation naming (WXL contract)

```
{Table}.dbc                 base (in data/dbc/, unchanged)
{Table}.dbc{N}-{project}    continuation, N = 1..9
```

Examples:

```
Spell.dbc1-hotfix
CreatureDisplayInfo.dbc1-artpass
Item.dbc3-shared-lib
```

### Merge order (later wins on duplicate row ID)

1. Base row already loaded from `data/dbc/{Table}.dbc` (+ any `*_dbc` DB overlay)
2. Tier `1`, then `2`, … `9`
3. Within a tier: manifest line order, then project slug A–Z
4. Same row ID: **later continuation wins**

Backup files (`.bak`, `.backup`, `.old`, `.orig`, `.tmp`) are ignored.

---

## Manifest (optional on server)

On the server, the module **auto-scans** `dbc-continuations/` for `*.dbc[1-9]-*` files. You usually do not need a manifest here. Optional `wxl-dbc.manifest` in that folder only if you want explicit load order (same format as the client extension uses for MPQ packs).

---

## Verify

After starting `worldserver`, check the log for:

```
Applying N continuation file(s) from ...
  Spell.dbc1-hotfix -> X row(s) into Spell.dbc
Done. ... Base data/dbc/ files were not modified.
```

---

## Client side

This module is the **server half** of the WXL client extension in this repository ([../../../README.md](../../../README.md)). Same continuation files, same naming, same merge rules. Deploy the same `.dbc1-*` binaries on the client (loose `Data/DBFilesClient/` or MPQ) with `wxl-extended-dbc` installed. Row IDs must match on both sides.

---

## Notes

- Continuations apply **after** file load and **after** AC's `*_dbc` DB overlays, so continuation rows **win** over DB overlays for the same ID.
- Tables must match the server's WotLK DBC layout (same as today).
- **Custom forks:** if your core loads DBC stores that stock AzerothCore does not, register them in `src/WxlDbcRegistry.cpp` (same `WXL_DBC(...)` pattern as the existing entries). Otherwise continuations for those tables are skipped with "No server store registered".
- A few tables build extra in-memory indexes at the end of `LoadDBCStores()` (e.g. `MapDifficulty` → `sMapDifficultyMap`). `LookupEntry()` on the store is updated; those derived indexes are not rebuilt. For most tables this does not matter.
