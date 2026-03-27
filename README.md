# Robotfoci szimuláció — Tactics Sim

## Projekt áttekintés

Ez a projekt egy **kétdimenziós robotfoci szimuláció**: a **fizikai világ és az időlépés C nyelven** fut, a **játékosok viselkedése pedig Lua taktikai szkriptekben** van leírva. Minden robothoz külön Lua állomány (`tactics.lua` + `tacapi.lua`) fut; a C motor a pályán lévő testeket (robotok + labda) lépteti, és **egy központi `c_api` függvényen** keresztül adja vissza a pozíciókat, illetve fogadja a mozgás- és rúgásparancsokat.

**Összkép:**

| Réteg | Szerep |
|--------|--------|
| **C (`game.c`, `main.c`)** | Meccsállapot, idő, fizikai integráció, Lua VM-ek, `c_api` hívások |
| **Lua taktika (`api_repo/tactics/`)** | `OnInit` / `OnUpdate`: szerepek, célzás, mikor rúgjunk |
| **Megjelenítés (`render.c`)** | GLFW ablak, OpenGL 2.x-szerű rajzolás: pálya, robotok, labda |

---

## Modulok részletes magyarázata

### `include/types.h` és `src/types.c` — vektorok és entitások

> **Megjegyzés:** A közös fejléc **`include/types.h`**. A `src/types.c` **nem** tartalmazza újra a típusdefiníciókat — csak a vektor-matematikai segédfüggvényeket implementálja.

#### `include/types.h`

- **`vec2_t`**: kétdimenziós vektor (`x`, `y`) — pozíció vagy sebesség komponensei.
- **`body_t`**: fizikai test — `center` (pozíció) és `speed` (sebesség). **A labda és a robotok ugyanezt a struktúrát használják** a mozgáshoz.
- **`robotdata_t`**: robot metaadatai — csapat, id, `body_t cs`, tájolás, utolsó ütközés ideje, **saját Lua állapot** (`lua_State *L`).
- **`teamdata_t` / `matchdata_t`**: csapatok, labda, bedobás, félidő, utolsó rúgás időbélyege, **`MatchState`** enumeráció.

**Labda vs. robot a memóriában:** a labda **nem** külön „entity típus” enumként van tárolva a struktúrában; a **`match->ball`** egy `body_t`, a robotok pedig `match->team[t].robot[r].cs` alatt. A megkülönböztetés **indexeléssel** történik a fizikai ciklusban (lásd `game.c`).

#### `src/types.c` — vektor-matematika

- **`vec_length`**: vektor hossza \(\sqrt{x^2 + y^2}\).
- **`vec_norm`**: **egységvektor** (normalizálás) — nullvektor esetén `{0,0}` visszaadása, hogy elkerüljük a nullával való osztást.

Ezeket főleg a **`game_update`** fizikai része használja (pl. lépés irányának normalizálása az ütközésnél).

---

### `src/game.c` — fizikai ciklus és ütközés-viselkedés

#### Fő elemek

1. **`lua_api`**: a Lua `tacapi.lua` által hívott **C központi kapu**. Az első egész argumentum a **parancskód** (pl. `10` csapattárs pozíció, `12` labda, `20` mozgás, `21` rúgás).
2. **`game_update`**: minden képkockán:
   - kiszámolja a **`deltaTime`** értéket;
   - lefuttatja az összes robot **`OnUpdate(dt)`** Lua függvényét;
   - **frissíti a pozíciókat** a sebesség alapján.

#### Ütközés / „szűrés” — mit csinál a jelenlegi kód?

A fizikai rész **egyszerűsített**: a külső ciklus végigmegy **minden testen** (összes robot + labda). A **belső ciklus csak a robotindexeken** (`b < TEAMS*PLAYERS`) megy végig.

- **Robot–robot:** ha két robot előre vetített pozíciója **túl közel** van egymáshoz (kb. `2 × MARKER_RADIUS` távolság), a lépésvektor (`maxStep`) **rövidül** — ez egy **enyhe átfedés-elkerülés**, nem teljes rigid-body motor.
- **Labda indexnél** (`a == TEAMS*PLAYERS`) a feltétel **`a == TEAMS*PLAYERS` ⇒ `continue`** minden `b`-re: a belső ciklus **nem módosítja** a labda lépését robotokkal ebben a blokkban. A labda mozgása főleg **sebesség integrálás** + **súrlódás-szerű csillapítás**; a **rúgás** a **`c_api` 21-es ágában** állítja a labda sebességét.

**Fontos:** ez **nem** „ghosting” a C rétegen (a robotok továbbra is ütköznek egymással ebben a modellben). A **lágy, folyékony kitérést** a Lua oldalon az **elkerülő kormányzás** (`applyAvoidance` a `tactics.lua`-ban) segíti.

---

### `src/render.c` — megjelenítés

- **GLFW** ablak létrehozása, **OpenGL** kontextus (`Render_Create` / `Render_Destroy`).
- **`Render_Update`**: puffercsere, események, háttérszín, viewport — a főciklusban minden frame-ben hívódik.
- **Koordináta-transzformáció** (`_render_vertex`): a világkoordinátákat **NDC**-be (-1…1) képezi az ablak méretével és skálázással.
- **`drawField`**: a pálya elemei — középvonal, középkör, büntetőpontok keresztjei, téglalap kontúrok: pálya széle, kapuk, tizenhatosok, tizenegyes területek (`field.h` konstansok alapján).
- **`drawMarker`**: egy `body_t` kirajzolása **rombusz** (négy csúcs) alakkal; **sebességvektor** egy rövid szegmens; opcionálisan „leütött” állapot (`fallen`) keretezése.

A **`main.c`** minden frame-ban: `game_update` → `drawField` → robotok és labda `drawMarker` hívásai.

---

### `api_repo/tactics/` — Lua API, taktika, logika

#### `tacapi.lua` — C API burkoló

- A **`c_api(...)`** hívásokat **olvasható nevekre** fedi (`API.Locate.Ball`, `API.Robot.Move`, stb.).
- **Szögkonvenció:** a taktika **fokban** számol; a **`Move` / `Kick`** hívás előtt **radiánra vált** (`DEG2RAD`), mert a C oldal `cos` / `sin` így várja.

#### `tactics.lua` — központi állapotgép (magas szint)

- **Kapus** (`player == KEEPER_ID`): `Keeper.Step` eredménye → `API.Robot.Move`, szükség esetén `API.Robot.Kick`.
- **Mezőny:** **egy fő támadó** (`attackerId` — a labdához legközelebbi mezőnyjátékos), **hiszterézis** (`attackerLock`) a szerepcsere villogásának csökkentésére.
- **Támogató** (`supporterId`): második legközelebbi — előretolt vagy visszazáró célpont.
- **Labda „burkoló”:** `ballObj = { id = "ball", type = "ball", x, y }` — **kizárólag** a `API.Locate.Ball()` C-ből jövő adat másolata; **nem** másik robot pozíciója.

#### `logic/utils.lua` — geometria

- Távolság, irányszög (fok), pont–szakasz távolság (passzsáv ellenőrzéshez).

#### `logic/striker.lua` / `logic/keeper.lua`

- **`is_valid_ball_object(ballPos)`**: ellenőrzi, hogy a kapott tábla **érvényes koordinátákat** tartalmaz-e (`x`, `y`). Ez **nem** C-beli entity-ID, hanem **védelmi ellenőrzés** — a taktika a labdát mindig a **`Ball()` API-ból** tölti.
- **Rúgás** csak akkor kerül a visszatérési táblába, ha a döntés **labda–robot távolságon** alapul (edge trigger), nem másik játékos távolságán.

#### „isBall” és téves trigger elkerülése

A projektben **explicit `Utils.isBall()` függvény opcionálisan** dokumentálható; a gyakorlati szabály:

1. A labda pozíciója **mindig** `API.Locate.Ball()` / `BallVel()`.
2. A taktika **`ballObj`** néven adja át a Striker/Keeper moduloknak ugyanazt a táblát.
3. A **Kick** csak akkor hívódik, ha a **striker/keeper logika** `kickAngleDeg` + `kickForce` mezőket ad vissza — ezek a **labda közelségéhez** kötöttek, nem robot–robot távolsághoz.

A **„ghosting” / elkerülés** részben Lua-ban van: **`applyAvoidance`** közeli játékoshoz képest **finomítja a mozgás irányát**, hogy kevésbé torlódjanak — **nem** vált taktikai állapotot és **nem** indít rúgást önmagában.

---

## Technikai részletek

### C ↔ Lua kommunikáció

1. **`game_init`**: minden robothoz `luaL_newstate`, `tacapi.lua` betöltése, **`lua_register(L, "c_api", lua_api)`**.
2. Globális Lua változók: **`team`**, **`player`**, **`match`** (light userdata a `matchdata_t*`-ra).
3. Minden frame: **`OnUpdate(dt)`** meghívása; a Lua a **`c_api(opcode, ...)`**-on keresztül olvas pozíciót / ír sebességet és rúgást.

### Mozgás: elkerülés (Lua)

- **`tactics.lua`**: `applyAvoidance` — ha van nagyon közeli másik játékos **és** a labda **nincs** „érintési” küszöbön, a kívánt mozgásirányhoz **merőleges komponens** keveredik → **„csúszás”** hatás.
- A C oldali robot–robot lépéskorrekció ettől **független**; együtt adják a viselkedést.

---

## Telepítés és futtatás

### Függőségek

- **Fordító:** Linuxon `gcc`, macOS-en `clang` (lásd `makefile`).
- **Könyvtárak:** **OpenGL**, **GLFW**, **Lua** (pl. Homebrew: `brew install glfw lua`).

A `makefile` az operációs rendszer szerint állítja a **`-L` / `-I` útvonalakat** (Linux: `/usr/...`, macOS: `/opt/homebrew/...`).

### Build és futtatás

```bash
make        # Létrehozza a build/sim futtathatót és másolja a Lua fájlokat build/ alá
make run    # cd build && ./sim — elindítja az ablakos szimulációt
make clean  # build/ mappa törlése
```

Az első fordítás után a **`build/`** könyvtárban lesz a **`sim`** és a másolt **`tactics.lua`**, **`tacapi.lua`**, **`logic/*.lua`** fájlok — a futó program innen tölti be őket.

### Hibakeresés

- Lua szintaxis- vagy futásidejű hiba: a konzolra **`Error: ...`** üzenet érkezhet a `game_init` / `game_update` során.
- macOS-en előfordulhatnak **GLFW / windowing** figyelmeztetések; ezek nem feltétlenül a taktikai kód hibái.

---

## Fájlok gyors tájékoztató

| Útvonal | Tartalom |
|---------|-----------|
| `main.c` | Főciklus: init → render → `game_update` → rajzolás |
| `include/types.h` | Adattípusok (meccs, robot, labda test) |
| `src/types.c` | `vec_length`, `vec_norm` |
| `src/game.c` | `lua_api`, fizika, meccslogika |
| `src/render.c` | Pálya és testek kirajzolása |
| `include/field.h` | Pályaméretek, `MARKER_RADIUS`, stb. |
| `api_repo/tactics/tactics.lua` | Fő taktika |
| `api_repo/tactics/tacapi.lua` | C API Lua burkoló |
| `api_repo/tactics/logic/*.lua` | Kapus / csatár / segédfüggvények |

---

*Dokumentum célja: csapatbemutató és onboarding — a pontos viselkedés mindig a forráskóddal egyeztetendő, ha a szimulációt módosítják.*
