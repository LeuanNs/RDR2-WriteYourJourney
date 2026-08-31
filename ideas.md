# Ideas para futuras versiones

## 1. Texto dinámico según honor

**Cómo funcionaría**: Cambiar el texto de la portada según el nivel de honor del jugador.
- Honor alto con Arthur (>0): "Blessed are those who hunger and thirst for righteousness."
- Honor bajo con Arthur (<0): "Blessed are those who mourn"

CONDICIONAL SOLO CON JOHN MARSTON: 
Si el jugador es John Marston, vamos a tomar lo ultimo de Arthur (solo revisar como quedo antes, si el de honor alto u honor bajo y luego visualmente tacharlo y abajo escribir)
- "Blessed are the peacemakers"

**TODO**:
- [ ] Crear `std::atomic<int> s_playerHonor` en `menu.cpp`
- [ ] Crear `std::atomic<bool> s_isJohn` en `menu.cpp`
- [ ] En `script.cpp`, leer honor con native `PLAYER::_GET_HONOR()` cada frame
- [ ] En `script.cpp`, detectar personaje con `PED::GET_ENTITY_MODEL(PLAYER::PLAYER_PED_ID())`
- [ ] Pasar valores a `menu.cpp` via setters
- [ ] En `menu.cpp` línea ~660, reemplazar texto hardcodeado con condicional según honor/personaje

---

## 2. Notas en el mapa

**Cómo funcionaría**: Jugador escribe nota, guarda coords actuales. Al acercarse a esas coords (<2m), puede leer la nota con tecla interacción.

**TODO**:
- [ ] Crear sistema de "waypoints personalizados" con estructura: `{coords, pageId, author}`
- [ ] Guardar en `myjourney/waypoints.dat` (formato binario similar a `_draw.dat`)
- [ ] En `script.cpp`, cada frame: obtener coords con `GET_ENTITY_COORDS(PLAYER_PED_ID())`
- [ ] Calcular distancia a cada waypoint guardado
- [ ] Si distancia < 2m, mostrar prompt "Press F to read note"
- [ ] Al presionar F, abrir diario en página específica (reutilizar `OpenSession()` + `SelectPage()`)
- [ ] UI: añadir opción en menú de dibujo "Save note here" que capture coords actuales

---

## 3. Diarios de NPCs al lootear

**Cómo funcionaría**: Al lootear cadáver, % de encontrar "diario del NPC" con contenido lore-friendly. Algunas notas dan pistas de tesoros.

**TODO**:
- [ ] Detectar evento de loot (hook de native `_GET_PED_MONEY` o detectar inventario de cadáver)
- [ ] Crear array de templates de diarios NPC en `myjourney/custom/npc_diaries/`
- [ ] Cada archivo: `{npcHash, content, hasTreasureHint, treasureCoords}`
- [ ] Al lootear, roll % (ej: 15%), si success: añadir "NPC Diary" al inventario
- [ ] UI: al abrir inventario, mostrar item "Diary" que al usarlo abra el diario en página específica
- [ ] Sistema de pistas: si `hasTreasureHint=true`, mostrar coords en mapa como waypoint

---

## 4. Sistema de cartas

**Cómo funcionaría**: Escribir carta, guardarla como "sello" (item). Acercarse a cartero/NPC específico, enviar a personaje (Mary, Dutch, etc). Algunas cartas reciben respuesta hardcodeada.

**TODO**:
- [ ] Crear `myjourney/letters/` con cartas escritas (formato: `{to, from, content, sent}`)
- [ ] UI: en modo escritura, añadir opción "Save as letter" que cree item en inventario
- [ ] Detectar NPCs de correo con `PED::GET_ENTITY_MODEL()` (hash de carteros)
- [ ] Al acercarse, mostrar menú: "Send letter to..." con lista de personajes
- [ ] Sistema de respuestas: mapa `{characterName, responseLetterId}` con cartas hardcodeadas
- [ ] Al enviar, marcar carta como `sent=true`, crear nueva carta de respuesta en inventario
- [ ] UI: al abrir inventario, mostrar "Unread letters" que al usar abra el diario en esa página

---

## 5. Detectar misiones completadas para otorgar items

**Cómo funcionaría**: Tras completar misión específica, otorgar item único (ej: Biblia tras misión con Swanson).

**TODO**:
- [ ] Crear array de "mission rewards": `{missionHash, itemId, alreadyGranted}`
- [ ] En `script.cpp`, cada frame: `STATS::STAT_GET_INT(missionHash)` para detectar completada
- [ ] Si completada y `!alreadyGranted`: añadir item a inventario, marcar como granted
- [ ] Guardar estado en `myjourney/mission_progress.dat` para persistencia
- [ ] UI: mostrar notificación "You received: [item]" con fade-in/out

---

## 6. Fuente diferente según personaje

**Cómo funcionaría**: Arthur usa fuente caligráfica actual, John usa fuente más legible/redondeada.

**TODO**:
- [ ] Cargar segunda fuente `.ttf` en init de DX12/Vulkan (ej: fuente manuscrita legible)
- [ ] Crear `std::atomic<bool> s_isJohn` (reutilizar de idea #1)
- [ ] En `menu.cpp`, al renderizar texto: seleccionar `io.Fonts->Fonts[1]` (Arthur) o `Fonts[2]` (John)
- [ ] Aplicar solo en modo escritura/lectura, no en UI

---

## 7. Sistema de contenido custom (v1.5)

**Cómo funcionaría**: Carpeta `myjourney/custom/` con subcarpetas para contenido de la comunidad. Al cargar, parsear archivos y añadir al juego sin redistribuir .asi.

**TODO**:
- [ ] Crear estructura: `myjourney/custom/diaries/`, `treasures/`, `letters/`, `npc_diaries/`
- [ ] Definir schema JSON simple para cada tipo (ej: `treasure.json`: `{coords, hint, reward}`)
- [ ] Crear `ContentLoader::LoadAll()` que escanee carpetas al iniciar sesión
- [ ] Parsear con librería ligera (nlohmann/json header-only o parser manual simple)
- [ ] Integrar con sistemas existentes: tesoros → waypoints, diarios → páginas, cartas → inventario
- [ ] Documentación: `myjourney/custom/README.md` con ejemplos de cómo crear contenido

---

## Restricciones generales

- **NO tocar**: sistema de render actual, hooks de Vulkan/DX12, máquina de estados Cover/Open
- **NO modificar**: `DrawDrawingCanvas()`, `DrawReadPage()`, `DrawWritePage()`, `Render()`
- **Priorizar**: reutilizar `std::atomic<>` para comunicación script→render, `std::filesystem` para I/O
- **Tokens**: mantener código conciso, evitar comentarios redundantes
