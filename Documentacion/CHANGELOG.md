# Changelog - Write Your Journey

## [Documentacion - AGENT_CONTEXT.md Completo] - 2026-09-03

### Revision y actualizacion de documentacion para IA

**Cambio principal: AGENT_CONTEXT.md reescrito completamente**

El archivo `Agente/AGENT_CONTEXT.md` fue reescrito desde cero para incluir TODOS los sistemas implementados en el proyecto. La version anterior (v1.0) solo documentaba el journal basico y mencionaba CustomBooks de forma superficial. La nueva version cubre:

**Secciones nuevas agregadas:**

1. **Tabla de archivos completa con lineas** - Cada archivo del proyecto ahora tiene su conteo de lineas y descripcion detallada
2. **Sistema CustomBooks completo** - Inventory carousel, lazy loading, Index de capitulos, barra de busqueda, bookmarks, seleccion de paginas, modo edicion, paginas ripeadas/restauradas, ediciones de texto
3. **Sistema Sheets completo** - Rip de paginas (P hold 3s), overlay de hoja, discoverables en el mundo, caminata+crouch para pickup, animaciones (rip/restore/flip/crouch), KeepSheet, LeaveSheet, paginas restauradas con overlay de dano
4. **Configuracion [RipSheets]** - Seccion INI para activar/desactivar sistema de sheets y configurar tecla de pickup
5. **Arquitectura y flujo de ejecucion** - Diagrama de hilos (script, render, DllMain), reglas de hilos, flujo completo de Render() con orden de prioridad
6. **Integracion entre sistemas** - Win32.cpp input forwarding, script.cpp bloqueo de controles, menu.cpp render order
7. **Hook de Vulkan detallado** - Tracking de swapchains, FullImGuiReset, fix de focus check
8. **Fuentes** - Indices 0/1/2 y cuando se cargan
9. **Configuracion de libros custom** - Formato completo de config.ini con todos los campos
10. **Flujo de archivos de sheets** - Discoverables, ripped_pages.ini, damaged_pages.ini

**Archivos modificados:**
- `Agente/AGENT_CONTEXT.md` - Reescrito completamente (de 195 lineas a ~330 lineas de documentacion densa)
- `Documentacion/CHANGELOG.md` - Actualizado con esta entrada

---

## [Build - Testing Fixes Batch 7] - 2026-09-03

### Fixes based on testing feedback (Batch 7)

**Fix 1: Restored page damage overlay now draws OVER text**
- Changed from `GetBackgroundDrawList()` to `GetForegroundDrawList()` for damage overlay
- Damage marks (wrinkles, stains, tears) now render on top of page text content
- Applied to both journal and custombooks

**Fix 2: KeepSheet now restores page to original journal/custombook**
- When pressing K on a discoverable sheet, the page is now restored to its original location
- Added logic in `KeepSheet()` to mark the original page as restored (with damaged visual)
- Page will show the wrinkled/torn visual when viewed in journal/custombook after keeping

**Fix 3: Index reverted to I then ENTER**
- Reverted Index opening to require I key first, then ENTER to confirm
- Added `s_indexPending` flag to track when I is pressed
- This fixes the bug where opening Index directly caused issues

**Fix 4: Random page lazy loading fixed**
- Fixed targetLine calculation: removed incorrect `* 2` multiplier
- Now correctly loads the chunk for the random page in large books

**Fix 5: Improved ripped page visuals**
- Journal: Added "Page Ripped" text overlay on torn page slots
- Both journal and custombooks: Enhanced torn edge with larger jag size (8 vs 6), more stains (8 vs 5), and added scratch marks (3 diagonal lines)
- Darker background color for better contrast

**Fix 6: Mouse cursor handling**
- Journal: Mouse cursor disabled when in Read mode with page selected (keyboard navigation only)
- CustomBooks: Mouse cursor enabled when inventory is open (for search bar interaction)
- Added `#include "menu.h"` to custombooks.cpp for `SetShouldDrawMouse()` access

### Archivos modificados
- `src/ImGuiRDR2Hook/menu.cpp` - Foreground draw list for damage overlay, "Page Ripped" text, improved torn slot visual, mouse disable in Read mode
- `src/ImGuiRDR2Hook/custombooks.cpp` - Foreground draw list for damage overlay, improved torn slot visual, Index I+ENTER flow, random page fix, mouse enable in inventory
- `src/ImGuiRDR2Hook/sheets.cpp` - KeepSheet restores page to original journal/custombook
- `Documentacion/CHANGELOG.md` - Actualizado con Batch 7
- `Testing.md` - Actualizado con Batch 7

---

## [Build - Testing Fixes Batch 6] - 2026-09-03

### Fixes based on testing feedback (Batch 6)

**Fix 1: R key no se detectaba para pickup de sheets**
- Causa raiz: `GetAsyncKeyState('R')` se llamaba al inicio del loop para el reload (R+P), consumiendo el bit `0x0001` (just-pressed). Cuando despues se verificaba `keyJustPressed` para el pickup, ya era 0.
- Solucion: Reemplazado `0x0001` por un tracker manual de estado (`s_pickupKeyWasDown`). Ahora se detecta la transicion de no-pressionado a presionado correctamente, sin importar cuantas veces se llame `GetAsyncKeyState` antes.
- `Sheets::Init()` ahora se llama al inicio de `main()` (antes del loop), no solo en `OpenSession()`. Esto asegura que las discoverables se cargan al iniciar el juego, no solo al abrir el journal.

**Fix 2: Pagina restaurada se veia perfecta en journal/custombook**
- Causa raiz: `DrawRestoredPage()` dibujaba el fondo daniado, pero el texto se renderizaba ENCIMA, cubriendo todo el dano visual.
- Solucion: Separado en dos funciones:
  - `DrawRestoredPage()` - solo dibuja el fondo con color mas oscuro
  - `DrawRestoredPageDamageOverlay()` - dibuja arrugas, manchas, tajos y bordes rotos
- El overlay de dano ahora se dibuja DESPUES del texto, haciendo las marcas visibles sobre el contenido.
- Aplicado tanto en journal (menu.cpp) como en custombooks (custombooks.cpp).

**Fix 3: PickupMessage redundante en location.ini**
- Eliminado `PickupMessage` de la generacion de location.ini en `LeaveSheetAtPlayer()`.
- El mensaje mostrado al jugador es dinamico desde `WJConfig::Sheet_Nearby`, no necesita estar hardcodeado en cada location.ini.

### Archivos modificados
- `src/ImGuiRDR2Hook/script.cpp` - Tracker manual de key state, Sheets::Init() al inicio de main()
- `src/ImGuiRDR2Hook/menu.cpp` - DrawRestoredPage simplificado, DrawRestoredPageDamageOverlay nuevo, llamado despues del texto
- `src/ImGuiRDR2Hook/custombooks.cpp` - DrawRestoredPage simplificado, DrawRestoredPageDamageOverlay nuevo, llamado despues del texto
- `src/ImGuiRDR2Hook/sheets.cpp` - Eliminado PickupMessage de LeaveSheetAtPlayer()
- `Documentacion/CHANGELOG.md` - Actualizado con Batch 6
- `Testing.md` - Actualizado con Batch 6

### Checklist de Testing

#### R key pickup - Fix critico
- [ ] Caminar hacia una sheet -> aparece icono de tecla
- [ ] Presionar R una vez -> personaje camina inmediatamente hacia la sheet
- [ ] El pickup funciona desde el primer momento (sin necesidad de abrir journal primero)
- [ ] Sheets de sesiones anteriores se detectan correctamente al iniciar el juego

#### Pagina restaurada - Visual sobre texto
- [ ] Rippear pagina del journal -> escribir algo -> presionar ESC para restaurar
- [ ] Navegar a la pagina restaurada -> ¿se ven las arrugas SOBRE el texto?
- [ ] Las marcas de dano (arrugas, manchas, tajos) son visibles encima del contenido
- [ ] Mismo test en custombook -> ¿se ven las marcas sobre el texto?

#### PickupMessage eliminado
- [ ] Dejar una sheet en el mundo (L en overlay)
- [ ] Verificar location.ini generado -> ¿NO tiene linea PickupMessage?
- [ ] Acercarse a la sheet -> mensaje dinamico sigue apareciendo correctamente

---

## [Build - Testing Fixes Batch 5] - 2026-09-03

### Fixes based on Testing.md feedback (Batch 5)

**Fix 1: Pickup con R - ahora es instantaneo (no requiere mantener)**
- Cambiado el sistema de "mantener R por 3s" a "presionar R una vez"
- Al presionar R, el personaje camina inmediatamente hacia la sheet, se agacha y muestra la hoja
- Agregado try-catch alrededor de la caminata y la animacion de crouch
- Si la animacion de crouch falla, el sistema no se rompe y continua normalmente
- Si la caminata falla, se cancela limpiamente con CLEAR_PED_TASKS
- La barra de progreso de hold ya no aparece (ahora es instantaneo)

**Fix 2: Pagina restaurada - visual mejorado (mas daniada)**
- Mejorado DrawRestoredPage() en journal y custombooks para ser mucho mas visible
- Color base mas oscuro: (175,165,140) en vez de (195,185,160)
- Lineas de arrugas aumentadas de 8 a 14, con mayor opacidad (120 vs 80) y grosor (1.5 vs 1.0)
- Manchas de suciedad aumentadas de 5 a 8, con mayor radio (12 vs 8) y opacidad (90 vs 60)
- Agregadas 5 lineas de "pliegues" cruzados para efecto de hoja arrugada
- Bordes rasgados mas grandes (tearSize * 3 vs 2) y mas densos (step 2.5 vs 3)
- Tajos laterales aumentados de 3 a 6, mas largos (16 vs 10) y gruesos (2.0 vs 1.5)
- Agregadas 4 manchas circulares adicionales para efecto de hoja usada
- La pagina restaurada ahora se ve claramente daniada: arrugada, sucia, con bordes rotos

**Fix 3: Sistema de sheets configurable desde INI ([RipSheets])**
- Agregada seccion [RipSheets] en WriteYourJourney.ini
- `enableRipSheetSystem=1` (1=activado, 0=desactivado) - controla todo el sistema de sheets
- `ripSheetPickupKey=R` (configurable A-Z) - tecla para recoger sheets del suelo
- Al cambiar la tecla, se actualiza automaticamente el icono mostrado en el prompt
- Si enableRipSheetSystem=0:
  - No se inicializa el sistema de sheets (Sheets::Init() retorna inmediatamente)
  - No aparece el prompt de pickup al acercarse a una sheet
  - No se puede arrancar paginas con P (en journal ni custombooks)
  - No se renderiza nada del sistema de sheets
  - El texto "P: Rip Page" desaparece de la ayuda del journal y custombooks

### Archivos modificados
- `src/ImGuiRDR2Hook/config.h` - Agregadas variables RipSheetsEnabled y RipSheetPickupKey, parsing de [RipSheets]
- `src/ImGuiRDR2Hook/script.cpp` - Pickup instantaneo con R, try-catch en walk/crouch, gate RipSheetsEnabled
- `src/ImGuiRDR2Hook/sheets.cpp` - Gate RipSheetsEnabled en Init/HandleInput/Render/UpdatePickupPrompt/RenderPickupPrompt, key configurable en prompt
- `src/ImGuiRDR2Hook/menu.cpp` - DrawRestoredPage mejorado, gate P key y help text con RipSheetsEnabled
- `src/ImGuiRDR2Hook/custombooks.cpp` - DrawRestoredPage mejorado, gate P key y hint text con RipSheetsEnabled
- `Documentacion/CHANGELOG.md` - Actualizado con Batch 5
- `Testing.md` - Actualizado con Batch 5

### Checklist de Testing

#### Pickup instantaneo con R
- [ ] Caminar por el mundo -> acercarse a una sheet -> aparece icono de tecla configurable
- [ ] Presionar R una vez (sin mantener) -> personaje camina inmediatamente hacia la sheet
- [ ] Al llegar -> personaje se agacha (crouch animation)
- [ ] Al terminar crouch -> hoja se muestra + controles bloqueados
- [ ] Si la animacion de crouch falla -> no se rompe todo, continua normalmente
- [ ] Moverse mientras personaje camina -> se cancela limpiamente

#### Pagina restaurada - Visual mejorado
- [ ] Rippear pagina del journal -> presionar ESC en overlay -> animacion de hoja volviendo
- [ ] Pagina restaurada -> visual claramente daniada: arrugada, sucia, bordes rotos
- [ ] Color mas oscuro que paginas normales
- [ ] Lineas de arrugas visibles (14 lineas)
- [ ] Manchas de suciedad visibles (8 manchas)
- [ ] Bordes rasgados grandes y densos
- [ ] Tajos laterales visibles (6 tajos)
- [ ] Rippear pagina de custombook -> presionar ESC -> mismo visual mejorado

#### [RipSheets] Configuracion INI
- [ ] Agregar [RipSheets] con enableRipSheetSystem=0 -> sistema completamente desactivado
- [ ] Con sistema desactivado -> no aparece prompt de pickup
- [ ] Con sistema desactivado -> P no arranca paginas en journal
- [ ] Con sistema desactivado -> P no arranca paginas en custombooks
- [ ] Con sistema desactivado -> "P: Rip Page" no aparece en ayuda
- [ ] Cambiar ripSheetPickupKey=F -> icono muestra "F" en vez de "R"
- [ ] Con tecla cambiada -> presionar F recoge la sheet (no R)
- [ ] enableRipSheetSystem=1 (o sin seccion) -> sistema funciona normalmente

---

## [Build - Testing Fixes Batch 4] - 2026-09-03

### Fixes based on Testing.md feedback (Batch 4)

**Fix 1: Pickup con R - ahora funciona en mundo libre**
- Movido el codigo de pickup de sheets fuera del bloque de CustomBooksEnabled
- Ahora el pickup con R funciona siempre que el journal no este abierto
- El jugador puede caminar por el mundo y al acercarse a una sheet, aparece el texto y puede mantener R por 3s
- El personaje camina, se agacha y muestra la hoja correctamente

**Fix 2: Pagina restaurada con visual unica**
- Agregado sistema para trackear paginas que fueron restauradas (s_restoredJournalPages, s_restoredCustomPages)
- Agregada funcion IsPageRestored() para verificar si una pagina fue restaurada
- Modificado RestorePage() para agregar las paginas al set de restauradas
- Implementada funcion DrawRestoredPage() con visual unica:
  - Color base mas oscuro (195,185,160) en vez de crema (210,200,175)
  - Lineas de arrugas aleatorias (8 lineas con opacidad 80)
  - Manchas de suciedad (5 circulos con opacidad 60)
  - Bordes rasgados en la parte superior (tajos de 8-14px)
  - Bordes rasgados en la parte inferior (tajos de 8-14px)
  - Tajos laterales (3 lineas diagonales con opacidad 150)
- Aplicado visual de pagina restaurada tanto en journal como en custombooks
- Las paginas restauradas ahora tienen una apariencia unica: arrugada, sucia y con tajos en los extremos

### Archivos modificados
- `src/ImGuiRDR2Hook/script.cpp` - Pickup con R movido fuera del bloque CustomBooksEnabled
- `src/ImGuiRDR2Hook/sheets.cpp` - Sistema de paginas restauradas, IsPageRestored(), DrawRestoredPage()
- `src/ImGuiRDR2Hook/sheets.h` - Agregada declaracion de IsPageRestored()
- `src/ImGuiRDR2Hook/menu.cpp` - Aplicado DrawRestoredPage() para paginas restauradas en journal
- `src/ImGuiRDR2Hook/custombooks.cpp` - Aplicado DrawRestoredPage() para paginas restauradas en custombooks
- `Documentacion/CHANGELOG.md` - Actualizado con Batch 4
- `Testing.md` - Actualizado con resultados de Batch 4

### Checklist de Testing

#### Pickup con R - Mundo libre
- [ ] Caminar por el mundo sin abrir nada -> acercarse a una sheet -> aparece texto "Press R to pick it up"
- [ ] Mantener R por 3s -> barra de progreso se llena
- [ ] Al completar hold -> personaje camina hacia la sheet
- [ ] Al llegar -> personaje se agacha (crouch animation)
- [ ] Al terminar crouch -> hoja se muestra + controles bloqueados

#### Pagina restaurada - Visual unica
- [ ] Rippear pagina del journal -> presionar ESC en overlay -> animacion de hoja volviendo
- [ ] Pagina restaurada -> visual unica: arrugada, sucia, con tajos en extremos
- [ ] Rippear pagina de custombook -> presionar ESC -> misma animacion y visual
- [ ] Verificar que la pagina restaurada se ve diferente a las paginas normales

---

## [Build - Testing Fixes Batch 3] - 2026-09-03

### Fixes based on Testing.md feedback (Batch 3)

**Fix 1: Index bug - se bugueaba al abrir directamente con I**
- Agregada llamada a OpenBook() despues de CloseIndex() en RenderIndex()
- Ahora al presionar Enter en un capitulo del Index, se abre el libro automaticamente
- El Index ya no se buguea al abrir directamente con I desde el inventory

**Fix 2: Glow - separado glow normal vs glow fuerte para paginas faltantes**
- Revertido DrawPageGlow() a la version original (2 capas, alpha 130-190) para paginas seleccionadas
- Creada nueva funcion DrawRippedPageGlow() (4 capas, alpha 180-255) para paginas ripped
- Aplicado glow fuerte solo a paginas ripped en journal y custombooks
- El glow normal ahora es sutil como antes, el glow fuerte solo marca paginas faltantes

**Fix 3: Random page - no tomaba todas las paginas del libro**
- Corregido calculo de totalPages usando book.lazyTotalLines para libros grandes
- Ahora el random page considera todas las paginas del libro, no solo las cargadas
- Fix aplicado tanto para random page (R) como para Index

**Fix 4: ENTER selection mode - a veces no respondia**
- Separado input de ENTER y click mouse en RenderBook()
- ENTER ahora selecciona leftPage (o rightPage si left esta ripped) directamente
- Click mouse sigue funcionando como antes para seleccionar pagina especifica
- El ENTER ahora funciona siempre que el custombook este abierto

**Fix 5: Night tint - demasiado oscuro**
- Ajustado tinte nocturno de (50,45,35,200) a (60,55,45,160) - mas claro
- Agregado tinte diurno sutil (100,90,75,100) para mejor consistencia visual
- El journal ahora tiene mejor balance entre dia y noche

**Fix 6: ESC restore - animacion de arrugada implementada**
- Agregadas variables s_restoreAnimating, s_restoreAnimT, s_restoreCache
- Modificado RestorePage() para iniciar animacion en vez de cerrar inmediatamente
- Creada funcion DrawRestoreAnimation() con efecto de hoja volviendo al journal
- Animacion incluye: movimiento desde esquina superior derecha al centro, efecto de arrugado con sin(t*PI), lineas de arruga que aparecen y desaparecen
- Duracion: 0.9s (igual que rip animation)

### Archivos modificados
- `src/ImGuiRDR2Hook/custombooks.cpp` - Fix Index, fix random page, fix ENTER selection, glow separado
- `src/ImGuiRDR2Hook/menu.cpp` - Glow separado, night tint ajustado
- `src/ImGuiRDR2Hook/sheets.cpp` - Animacion ESC restore implementada
- `Documentacion/CHANGELOG.md` - Actualizado con Batch 3
- `Testing.md` - Actualizado con resultados de Batch 3

### Checklist de Testing

#### Index - Fix critico
- [ ] Abrir Index directamente con I desde inventory -> ya no se buguea
- [ ] Presionar Enter en capitulo -> abre libro automaticamente en esa pagina
- [ ] ESC en Index -> cierra Index sin problemas

#### Glow - Separado normal vs fuerte
- [ ] Paginas seleccionadas -> glow sutil como antes (2 capas)
- [ ] Paginas ripped -> glow fuerte (4 capas, alpha 180-255)
- [ ] Verificar que el glow fuerte marca bien la forma de la pagina faltante

#### Random page - Fix paginas lejanas
- [ ] Presionar R en satchel -> abre pagina aleatoria de TODO el libro
- [ ] Verificar que puede abrir paginas mas alla de la 100
- [ ] Lazy loading funciona correctamente desde pagina aleatoria

#### ENTER selection mode - Fix
- [ ] Abrir custombook -> presionar ENTER -> selecciona pagina izquierda
- [ ] Si pagina izquierda esta ripped -> selecciona pagina derecha
- [ ] ENTER funciona siempre que el custombook este abierto

#### Night tint - Ajustado
- [ ] De dia -> tinte sutil (100,90,75,100)
- [ ] De noche (21:00-06:00) -> tinte mas claro (60,55,45,160)
- [ ] Verificar que no deslumbra de noche ni es demasiado oscuro

#### ESC restore - Animacion
- [ ] Rippear pagina -> presionar ESC en overlay -> animacion de hoja volviendo
- [ ] Animacion muestra efecto de arrugado (lineas que aparecen/desaparecen)
- [ ] Hoja vuelve al journal sin danos

---

## [Build - Testing Fixes Batch 2] - 2026-09-02

### Fixes based on Testing.md feedback (Batch 2)

**Fix 1: Pickup con R key (antes E)**
- Cambiado todas las referencias de 'E' a 'R' en script.cpp para sheet pickup
- Cambiado icono de tecla de "E" a "R" en sheets.cpp RenderPickupPrompt() y RenderEHoldPrompt()
- Cambiado HandleInput() en sheets.cpp para usar 'R' en vez de 'E'
- Cambiado texto default en config.h de "Press E to pick it up" a "Press R to pick it up"
- Motivo: La tecla E ya no funcionaba, se reasigno a R

**Fix 2: Index bug - controles desbloqueados, ImGui no respondia**
- Añadido CustomBooks::IsIndexOpen() al check de bloqueo de controles en script.cpp
- Añadido CustomBooks::CloseIndex() al cleanup cuando el juego pierde foco
- Ahora el Index mantiene el bloqueo de controles como el inventory y book
- Motivo: Al abrir Index, el personaje recuperaba control, ImGui dejaba de responder, ESC no funcionaba

**Fix 3: Strikethrough bug - aplicaba a pagina siguiente**
- Corregido edit.lineIndex en custombooks.cpp de `book.lazyStartLine + startLine` a solo `startLine`
- El calculo de actualLineIdx en el render ya compensa por lazyStartLine, no hay que duplicar el offset
- Motivo: Al rippear pagina con tachado, el tachado se aplicaba a pagina siguiente

**Fix 4: Lazy loading - paginas lejanas en blanco / random iba a pag 1-2**
- Fix random page (R): s_currentPage ahora se setea DESPUES de OpenBook() (que lo reseteaba a 0)
- Fix bookmark (K): s_currentPage se setea DESPUES de OpenBook() y se carga el chunk correcto
- Fix Index: calculo de linesPerPage ahora usa la misma formula que RenderBook() en vez de hardcodear 12
- Añadida carga de chunk despues de setear s_currentPage en random y bookmark
- Motivo: Al abrir pagina aleatoria o capitulo lejano via Index, todo salia en blanco

**Fix 5: Texto "ENTER: Selection Mode" faltante**
- Añadido "ENTER: Selection Mode" al help string en custombooks.cpp RenderBook()
- Se muestra tanto cuando hay pagina seleccionada como cuando no
- Motivo: Falta indicacion visual de como entrar en modo seleccion

**Fix 6: Ripped pages visual en custombook - nada visual**
- Añadido CustomBooks::RipPage() en sheets.cpp ConfirmRip() para sincronizar tracking
- Añadido CustomBooks::RestorePage() en sheets.cpp RestorePage() para sincronizar tracking
- Ahora s_rippedCustomBookPages en custombooks.cpp se actualiza correctamente
- Motivo: Paginas ripeadas en custombook no mostraban borde rasgado ni "Page Ripped"

**Fix 7: Ripped page glow - muy tenue, no marcaba bien la forma**
- Mejorado DrawPageGlow() en custombooks.cpp y menu.cpp
- Añadidas 4 capas de glow en vez de 2 (2 rellenos + 2 bordes)
- Aumentado alpha de 130-190 a 180-255 para glow mas visible
- Añadidos rectangulos rellenos para efecto de glow mas solido
- Motivo: El glow no era suficiente para dar pista visual de pagina faltante

**Fix 8: Rip progress bar - muy gruesa y brillante**
- Reducido tamaño de 320x12 a 280x8
- Reducido texto de 1.3x a 1.1x font size
- Cambiado colores de (255,200,80) a (200,170,90) para tonos mas sutiles
- Reducido border thickness de 2.5f a 1.5f
- Reducido padding y roundness
- Motivo: Los colores eran muy fuertes, parecia "de niño"

**Fix 9: Night tint - muy claro, deslumbraba de noche**
- Cambiado de (80,70,55,180) a (50,45,35,200)
- RGB mas bajo para oscuridad aumentada
- Alpha ligeramente mayor para mejor cobertura
- Motivo: Tinte nocturno no era lo suficientemente oscuro

### Archivos modificados
- `src/ImGuiRDR2Hook/script.cpp` - Pickup con R, Index control lock
- `src/ImGuiRDR2Hook/sheets.cpp` - Pickup con R, progress bar toned down, sincronizacion con custombooks
- `src/ImGuiRDR2Hook/custombooks.cpp` - Index fix, lazy loading fix, strikethrough fix, glow mejorado, texto selection mode
- `src/ImGuiRDR2Hook/menu.cpp` - Glow mejorado, night tint mas oscuro
- `src/ImGuiRDR2Hook/config.h` - Texto default "Press R to pick it up"
- `Testing.md` - Actualizado con fixes de Batch 2

### Checklist de Testing

#### Sheets - Pickup con R (CAMBIADO de E)
- [ ] Mantener R -> barra blanca se llena alrededor del cuadrado (3s)
- [ ] Soltar R antes de 3s -> se cancela correctamente
- [ ] Moverse mientras se mantiene R -> se cancela
- [ ] Moverse mientras personaje camina -> se cancela
- [ ] Al llegar -> personaje se agacha (crouch)
- [ ] Al completar hold + K Keep -> hoja se añade de vuelta

#### Sheets - Visual mejorado
- [ ] Paginas ripeadas -> glow pulsante azul MAS FUERTE (4 capas)
- [ ] Barra "Ripping page..." -> menos gruesa (280x8) y colores mas sutiles

#### CustomBooks - Index fix
- [ ] Abrir Index -> ImGui sigue respondiendo, controles bloqueados, ESC funciona
- [ ] Abrir capitulo lejano -> lazy loading carga chunk correcto
- [ ] Abrir con bookmark (K) -> abre en pagina del bookmark
- [ ] Abrir random (R) -> abre en pagina aleatoria

#### CustomBooks - Edicion y rip
- [ ] Click en pagina -> texto "ENTER: Selection Mode" visible
- [ ] Escribir texto + ENTER -> tachado se aplica a pagina correcta (no siguiente)
- [ ] Paginas ripeadas -> borde rasgado + "Page Ripped" visible

#### Journal - Colores
- [ ] De noche (21:00-06:00) -> tinte nocturno mas oscuro (50,45,35)

#### Lazy Loading
- [ ] Abrir Biblia NT 1858 -> navegar adelante sin cortes
- [ ] Llegar a pag 58+ -> texto sigue renderizando
- [ ] Abrir en pagina aleatoria (R) -> lazy loading funciona

---

## [Build - Testing Fixes Batch 1] - 2026-09-02

### Fixes based on Testing.md feedback

**Fix 1: R:Look Behind - Populate back content when ripping pages**
- Modified `StartRipPage()` to accept back content parameters (backText, backDrawing)
- When ripping a journal page, now loads partner page content and passes it
- When ripping a custombook page, now loads partner page content and passes it
- "R: Look Behind" now shows in overlay when partner page has content
- 3D flip animation now displays partner page content correctly

**Fix 2: Custombook rip rendering - Add Sheets::Render() calls**
- Moved Sheets::Render() call before custombook early returns in menu.cpp
- Rip progress bar and animation now show when ripping custombook pages
- Overlay now displays correctly when ripping from custombooks

**Fix 3: Index - Close inventory when opening**
- Added `CloseInventory()` call when opening Index with I key
- Index now renders immediately without needing to press ENTER
- Inventory state is properly cleared when transitioning to Index

**Fix 4: Index - Correct page calculation for lazy loading**
- Changed page calculation from `targetLine / linesPerPage` to `targetLine / (linesPerPage * 2)`
- s_currentPage is the page PAIR index, not individual page index
- Jumping to far chapters via Index now loads correct chunk
- Pages render correctly instead of showing blank

**Fix 5: Lazy loading - Adjust startLine for chunk offset**
- Modified drawPage lambda to subtract `lazyStartLine` from startLine
- Added bounds check: `if (startLine < 0 || startLine >= wrappedLines.size()) return;`
- Forward navigation now renders text correctly beyond page 58
- Chunk offset is properly accounted for in all page rendering

**Fix 6: Search bar - Add space key support**
- Added ImGuiKey_Space handling in search bar input
- Can now search for multi-word titles like "la biblia"
- Space character is properly added to search buffer

**Fix 7: Search bar - Add visual feedback when focused**
- Added blinking "|" cursor when search bar is focused
- Cursor blinks at 0.5s intervals using ImGui::GetTime()
- Cursor appears after text if buffer is not empty
- Clear visual indication that search bar is active

**Fix 8: Search bar - Unfocus on ESC/close**
- Added `s_searchFocused = false` when ESC is pressed in inventory
- Added `s_searchFocused = false` in CloseInventory()
- Search bar properly loses focus when inventory closes
- Prevents stuck focused state after closing

**Fix 9: Night tint - Make it darker**
- Changed night tint from `IM_COL32(120, 110, 95, 130)` to `IM_COL32(80, 70, 55, 180)`
- Increased alpha from 130 to 180 for more opacity
- Decreased RGB values for darker appearance
- Night time journal is now properly dimmed

**Fix 10: Change reload from F5 to R+P hold 5s**
- Removed single-key reload check
- Added R+P hold detection with 5 second timer
- Both keys must be held simultaneously for 5 seconds
- Prevents accidental reloads during gameplay
- Timer resets if either key is released

### Archivos modificados
- `src/ImGuiRDR2Hook/sheets.h` - Added back content parameters to StartRipPage()
- `src/ImGuiRDR2Hook/sheets.cpp` - Store back content in rip cache
- `src/ImGuiRDR2Hook/menu.cpp` - Load partner page content, fix Render() order, darker night tint
- `src/ImGuiRDR2Hook/custombooks.cpp` - Load partner content, fix Index, fix lazy loading, fix search bar
- `src/ImGuiRDR2Hook/script.cpp` - R+P hold 5s reload logic

### Checklist de Testing

#### Sheets - R:Look Behind
- [ ] Rippear pag 2 del journal -> overlay muestra "R: Look Behind" si pag 3 tiene contenido
- [ ] Presionar R -> animacion 3D de giro (escala horizontal)
- [ ] Al terminar giro -> muestra contenido de pag 3 (trasera)
- [ ] Presionar R de nuevo -> gira de vuelta al frente (pag 2)
- [ ] Rippear pagina de custombook -> mismo comportamiento con pagina partner

#### CustomBooks - Rip rendering
- [ ] Seleccionar pagina en custombook -> mantener P 3s
- [ ] Barra de progreso "Ripping page..." debe aparecer
- [ ] Animacion de hoja saliendo debe verse
- [ ] Overlay con contenido debe aparecer despues

#### CustomBooks - Index
- [ ] En satchel con libro seleccionado -> presionar I
- [ ] Index debe abrirse INMEDIATAMENTE (sin ENTER)
- [ ] Abrir capitulo en pagina lejana (ej: 940) -> lazy loading funciona, pagina se ve

#### CustomBooks - Lazy Loading
- [ ] Abrir Biblia NT 1858 -> navegar hacia adelante
- [ ] Llegar a pag 58+ -> texto sigue renderizando (no se corta)
- [ ] Navegar hacia atras y adelante -> chunks se cargan correctamente
- [ ] Abrir libro en pagina aleatoria (R) -> lazy loading funciona

#### CustomBooks - Search bar
- [ ] Click en barra de busqueda -> aparece "|" cursor parpadeante
- [ ] Escribir "la biblia" -> espacio funciona, busca correctamente
- [ ] Presionar ESC -> barra se desenfoca
- [ ] Cerrar satchel -> barra se desenfoca

#### Journal - Night tint
- [ ] Esperar a que sea de noche (21:00 - 06:00)
- [ ] Abrir journal -> tinte nocturno mas oscuro que antes
- [ ] No deslumbra de noche

#### General - Reload
- [ ] Mantener R+P durante 5s -> config se recarga
- [ ] Soltar antes de 5s -> no pasa nada
- [ ] Journal abierto + R+P 5s -> journal se cierra y config se recarga

---

## [Build - Fixes Masivos Sheets + CustomBooks] - 2026-09-02

### Fixes en Sistema de Sheets (Hojas Arrancadas)

**Fix 1/15: Ghost rendering al restaurar pagina con ESC**
- `RestorePage()` ahora restaura TAMBIEN la pagina partner (la de atras)
- Antes solo borraba la pagina fuente del set de ripped, dejando la partner marcada
- Formula de partner corregida: par N → N+1, impar N>1 → N-1

**Fix 2: Pickup con tecla E - visual de teclado + barra de hold**
- Reemplazado texto "Press E to pick it up" por un cuadrado con la letra "E" (simula tecla de teclado RDR2)
- Al mantener E, una barra blanca se llena alrededor del cuadrado (3 segundos)
- Si el jugador se mueve mientras mantiene E, la accion se cancela
- Si el jugador se mueve mientras el personaje camina hacia la sheet, la accion se cancela
- Al completar la barra, el texto desaparece completamente

**Fix 3: Distancia de pickup reducida a 5m**
- `PICKUP_PROMPT_RADIUS` y `PICKUP_RADIUS_DEFAULT` ahora son 5.0 (antes 10.0)

**Fix 4: Animacion de crouch al llegar a la sheet**
- Al llegar a la coordenada, el personaje se agacha (animacion `amb_rest@world_human_bottle_pickup`)
- Solo al terminar la animacion de crouch, se muestra la hoja
- Cuando se muestra la hoja, se bloquea control completo + HUD + camara

**Fix 8: Overlay no solapa texto de paginas no arrancadas**
- Al mostrar overlay de hoja arrancada, se deja de renderizar previews de paginas del journal
- El libro se ve detras (fondo), pero sin texto de paginas solapado

**Fix 9: Visual de pagina arrancada mejorado**
- Paginas ripeadas muestran glow pulsante (azul) como pista visual de que falta una hoja
- Texto "Page Ripped" ahora es configurable via INI: `Sheet_PageRipped`
- Bloqueo de seleccion: no se puede seleccionar una pagina ripped directamente
- Solo se puede acceder a la siguiente pagina no-ripped pasando con flechas

**Fix 10: Bookmark text position**
- "Bookmark Saved / Deleted" bajado 6px mas en el journal (de +5f a +11f)

**Fix 11: Pagina arrancada incluye frente + trasera correcta**
- `GetPartnerPage()` implementado: par N → N+1, impar N>1 → N-1
- Al rippear pagina 6, la 7 (trasera) tambien se marca como ripped
- Al rippear pagina 50, la 51 (trasera) tambien se marca como ripped
- `RestorePage()` restaura ambas paginas (fuente + partner)

**Fix 12: R: Mirar atras - animacion 3D de giro**
- En overlay de hoja arrancada, "R: Look Behind" disponible
- Al presionar R, animacion de giro 3D (escala horizontal coseno, 0.8s)
- Muestra el contenido de la pagina trasera (backText/backDrawing)
- Si la pagina trasera esta vacia, no se muestra la opcion

**Fix 13: K: Keep the Sheet**
- En overlay de discoverable, "K: Keep the Sheet" disponible
- Al presionar K: overlay desaparece, control vuelve al jugador
- Sheet se marca como "Collected=1" en location.ini
- El sistema la ignora en futuras busquedas

**Fix 14: Remover textos "Empty Page" / "Nothing written"**
- Eliminado texto "(empty page)" del overlay de sheets
- Eliminado texto "Empty_Page" del journal (DrawReadPage)
- Si no hay contenido, simplemente no se muestra nada

### Fixes en CustomBooks

**Fix 5: Index apertura instantanea + estilo de pagina de libro**
- Al presionar I en satchel, el Index se abre inmediatamente (sin esperar ENTER)
- Index ahora se dibuja como una pagina del mismo libro (mismo cover, pergamino, spine)
- Capitulos listados con highlight sutil en la seleccion

**Fix 6/7: Lazy loading para TODAS las paginas + navegacion forward**
- `OpenBook()` ahora cuenta lineas totales leyendo el archivo completo
- `RenderBook()` trigger de lazy loading basado en posicion absoluta (targetLine)
- Navegacion forward arreglada: usa `lazyTotalLines` para limites en vez de chunk cargado
- Al saltar a pagina via Index, se carga el chunk correcto inmediatamente
- Libros largos (Biblia) ahora navegan correctamente hacia adelante Y atras

### Nuevas claves de localizacion (INI)
- `Sheet_PageRipped` - Texto al seleccionar pagina arrancada (default: "Page Ripped")
- `Sheet_KeepSheet` - Texto para mantener hoja (default: "K: Keep the Sheet")
- `Sheet_LookBehind` - Texto para mirar atras (default: "R: Look Behind")

### Archivos modificados
- `src/ImGuiRDR2Hook/sheets.h` - Nuevas declaraciones: GetPartnerPage, EHold, Crouch, Flip, Keep, Collected tracking
- `src/ImGuiRDR2Hook/sheets.cpp` - Reescrito con todos los fixes (~900 lineas)
- `src/ImGuiRDR2Hook/menu.cpp` - Fix overlay overlap, glow ripped pages, bookmark position, remove empty text
- `src/ImGuiRDR2Hook/custombooks.cpp` - Fix lazy loading, forward nav, index as book page
- `src/ImGuiRDR2Hook/script.cpp` - E hold logic, crouch anim, movement cancel, control block
- `src/ImGuiRDR2Hook/config.h` - Nuevas claves de localizacion

### Checklist de Testing

#### Sheets - Fixes principales
- [ ] Rippear pag 3, presionar ESC para restaurar -> navegar a pag 7/8 -> pag 7 NO debe mostrar contenido de pag 3
- [ ] Rippear pag 2 -> pag 3 tambien debe desaparecer (misma hoja fisica)
- [ ] Rippear pag 6 -> pag 7 tambien debe desaparecer (trasera de la misma hoja)
- [ ] Rippear pag 50 -> pag 51 tambien debe desaparecer
- [ ] Al restaurar pag 3 con ESC -> pag 2 (o 4) tambien debe restaurarse
- [ ] Al mostrar overlay de hoja arrancada -> el texto de las otras paginas del journal NO debe verse solapado
- [ ] Paginas ripeadas deben mostrar glow pulsante azul como pista visual
- [ ] Click en pagina ripped -> muestra "Page Ripped" (texto configurable en INI)
- [ ] No se puede seleccionar directamente una pagina ripped (solo navegar con flechas)

#### Sheets - Pickup con E
- [ ] Acercarse a sheet a 5m -> aparece cuadrado con "E" (no texto "Press E...")
- [ ] Mantener E -> barra blanca se llena alrededor del cuadrado (3s)
- [ ] Soltar E antes de 3s -> barra se cancela, no pasa nada
- [ ] Moverse mientras se mantiene E -> accion se cancela
- [ ] Completar barra E -> personaje camina hacia la sheet
- [ ] Moverse mientras personaje camina -> accion se cancela, hoja no se muestra
- [ ] Al llegar -> personaje se agacha (animacion crouch)
- [ ] Al terminar crouch -> hoja se muestra + controles/HUD/camara bloqueados
- [ ] Al completar E hold -> texto de "hay una hoja cerca" desaparece completamente

#### Sheets - Overlay mejorado
- [ ] En overlay de hoja arrancada -> "R: Look Behind" visible si hay contenido atras
- [ ] Presionar R -> animacion 3D de giro (escala horizontal)
- [ ] Al terminar giro -> muestra contenido de la pagina trasera
- [ ] Presionar R de nuevo -> gira de vuelta al frente
- [ ] En overlay de discoverable -> "K: Keep the Sheet" visible
- [ ] Presionar K -> overlay desaparece, control vuelve al jugador
- [ ] Despues de K -> sheet marcada como collected en location.ini
- [ ] Volver a pasar por el mismo lugar -> sheet NO aparece (ya fue recogida)
- [ ] Si pagina esta vacia -> NO se muestra "(empty page)" ni ningun texto

#### CustomBooks - Index
- [ ] En satchel con libro seleccionado -> presionar I -> Index se abre inmediatamente (sin ENTER)
- [ ] Index se ve como pagina del mismo libro (cover, pergamino, spine)
- [ ] Flechas arriba/abajo -> navega entre capitulos
- [ ] Enter en capitulo -> cierra Index, abre libro en esa pagina
- [ ] Abrir capitulo en pagina 940 -> lazy loading carga chunk correcto desde pag 940

#### CustomBooks - Lazy Loading
- [ ] Abrir Biblia NT 1858 -> navegar hacia adelante sin problemas
- [ ] Llegar a pag 58+ -> texto sigue renderizando (no se corta)
- [ ] Navegar hacia atras y adelante -> chunks se cargan correctamente
- [ ] Abrir libro grande en pagina aleatoria (R) -> lazy loading funciona desde esa pagina

#### CustomBooks - Page Selection + Glow + Rip + Edit (NUEVO)
- [ ] Abrir un custombook -> hacer click en una pagina -> glow azul pulsante aparece
- [ ] Con pagina seleccionada -> flechas izq/der mueven seleccion entre paginas individuales
- [ ] Con pagina seleccionada -> P -> arranca la pagina (overlay con contenido)
- [ ] Con pagina seleccionada -> E -> modo edicion (cuadro de texto arriba)
- [ ] Escribir texto + ENTER -> texto tachado aparece en la pagina original con correccion arriba en pequeno
- [ ] Verificar que edits.txt se crea en la carpeta del libro
- [ ] Reabrir el libro -> las ediciones siguen visibles
- [ ] Paginas ripeadas en custombook -> muestran borde rasgado + "Page Ripped"
- [ ] ESC deselecciona pagina -> vuelve a navegacion normal por pares

#### Journal - Bookmark
- [ ] "Bookmark Saved / Deleted" aparece 6px mas abajo que antes (solo en journal)

#### General
- [ ] Journal sigue funcionando correctamente
- [ ] CustomBooks siguen funcionando correctamente
- [ ] No hay conflictos de teclas
- [ ] Alt+Tab cierra todo sin crash
- [ ] Recibir dano cierra todo sin crash

---

## [Idea 2 - Sheets/Discoverables] - 2026-09-02

### Nuevo Sistema: Hojas Arrancadas / Discoverables (Sheets)

**Archivos nuevos:**
- `src/ImGuiRDR2Hook/sheets.h` - Header con structs (SheetDrawing, RippedSheetCache, DiscoverableSheet) y API del namespace Sheets
- `src/ImGuiRDR2Hook/sheets.cpp` - Implementacion completa del sistema (~800 lineas)

**Archivos modificados:**
- `src/ImGuiRDR2Hook/config.h` - Nuevas claves de localizacion: Sheet_RipHint, RippingProgress, Sheet_LeaveHint, Sheet_ReadHint, Sheet_RestoreHint, Sheet_CloseHint, Sheet_Nearby, Sheet_PressE
- `src/ImGuiRDR2Hook/menu.cpp` - Include sheets.h, integracion en Render() con checks de estado, P key para rip, "P: Rip Page" en ayuda, efecto visual de paginas arrancadas en DrawOpenBook(), "Page ripped" al seleccionar pagina arrancada, Sheets::Init() en OpenSession()
- `src/ImGuiRDR2Hook/script.cpp` - Include sheets.h, SetPlayerCoords cada frame (con journal abierto y cerrado), UpdatePickupPrompt para sheets, TryPickupSheet con E, bloqueo de controles cuando overlay de sheet esta abierto sin journal
- `src/ImGuiRDR2Hook/ImGuiRDR2Hook.vcxproj` - Añadidos sheets.cpp y sheets.h al proyecto

**Flujo completo implementado:**

1. **Rip de pagina (P hold 3s):**
   - En journal abierto con pagina seleccionada (no zoom, no cover), se muestra "P: Rip Page" en la ayuda inferior
   - Mantener P durante 3 segundos exactos con barra de progreso (estilo ESC/CustomBooks hold)
   - Si se suelta P antes de 3s, se cancela sin efecto
   - Al completar, se inicia animacion de hoja saliendo del libro (0.9s, deslizamiento + rotacion + alpha fade)

2. **Efecto visual de pagina arrancada:**
   - En DrawOpenBook(), si s_pagePair o s_pagePair+1 esta ripped, se dibuja slot con borde rasgado (jitter deterministico con Rng) en vez de pagina crema completa
   - Paginacion no cambia: la pagina ripped deja hueco visible
   - Al seleccionar pagina ripped, se muestra "(Page ripped)" centrado
   - No se renderiza texto ni dibujos en paginas ripped

3. **Overlay de hoja arrancada (estilo zoom):**
   - Tras animacion, overlay centrado con fondo oscurecido
   - Hoja con bordes irregulares (poligono con jitter), color pergamino (210,200,175)
   - Muestra texto completo y trazos de dibujo escalados
   - Ayudas: "L: Leave Page here | R: Read | ESC: Add page back"
   - ESC: restaura pagina original (cancela rip)
   - L: deja hoja en coords actuales del jugador (crea Discoverable)

4. **Sistema Discoverables (hojas en el mundo):**
   - Estructura: `myjourney/Discoverables/SHEET<N>/` con `location.ini`, `sheet.txt`, `sheet_draw.dat`
   - `location.ini`: [Location] con X, Y, Z, PickupRadius=10.0, PickupMessage, Author, Source
   - Generacion incremental: SHEET1, SHEET2, ... (nunca reutiliza numeros)
   - Persistencia de paginas ripped en `myjourney/ripped_pages.ini`
   - Escaneo al iniciar sesion (Sheets::Init/ScanSheets)
   - Prompt de proximidad: al acercarse (<10m), muestra "Hay una hoja arrancada cerca" + "Press E to pick it up"
   - E para recoger: abre overlay con contenido de la hoja
   - Creacion manual: modder puede crear SHEET99/ a mano con archivos y aparece en el sistema

5. **Localizacion completa (INI):**
   - Sheet_RipHint="P: Rip Page"
   - RippingProgress="Ripping page..."
   - Sheet_LeaveHint="L: Leave Page here"
   - Sheet_ReadHint="R: Read"
   - Sheet_RestoreHint="Add page back"
   - Sheet_CloseHint="Close"
   - Sheet_Nearby="There is a ripped page nearby"
   - Sheet_PressE="Press E to pick it up"

**Invariantes respetadas:**
- Sistema aislado en sheets.cpp/h (patron CustomBooks)
- No se tocan DrawReadPage/DrawWritePage/DrawDrawingCanvas/SaveDirPath/PageFilePath/DrawingFilePath
- Natives solo en script.cpp (coords via atomic)
- ImGui solo en Render()
- Arte solo con ImDrawList (sin texturas)
- Integracion minima con checks de estado

---

## [Idea 2 - Mejoras Sheets + Index CustomBooks] - 2026-09-02

### Mejoras en Sistema de Hojas Arrancadas

**Barra de progreso mejorada:**
- Barra de rip (P hold 3s) ahora es mas visible: 320px ancho x 12px alto (antes 200px x 7px)
- Texto "Ripping page..." mas grande (1.3x font size) y mas brillante (255,230,180)
- Fondo oscuro con borde redondeado (5px) para mejor contraste
- Barra de progreso con gradiente dorado (255,200,80 -> 255,180,60)
- Borde exterior mas grueso (2.5px) y brillante (255,215,100)

**Integracion con CustomBooks Index:**
- Al arrancar pagina de CustomBook, se marca capitulo correspondiente como "Ripped Sheet" en index.json
- Funcion `CustomBooks::MarkChapterAsRipped(bookName, lineIndex)` actualiza titulo del capitulo y reescribe index.json
- Funcion `CustomBooks::GetNextValidLineIndex(bookName, lineIndex)` para saltar capitulos ripped

### Nuevo Sistema: Index de CustomBooks

**Archivos modificados:**
- `src/ImGuiRDR2Hook/custombooks.h` - Nuevas funciones: OpenIndex(), CloseIndex(), IsIndexOpen(), RenderIndex(), MarkChapterAsRipped(), GetNextValidLineIndex()
- `src/ImGuiRDR2Hook/custombooks.cpp` - Implementacion completa del Index (~150 lineas)
- `src/ImGuiRDR2Hook/menu.cpp` - Integracion de CustomBooks::IsIndexOpen() y RenderIndex() en Render()
- `src/ImGuiRDR2Hook/config.h` - Nueva clave: Sheet_RippedChapter="Ripped Sheet"
- `src/ImGuiRDR2Hook/sheets.cpp` - Include custombooks.h, llamada a MarkChapterAsRipped() en ConfirmRip()

**Flujo del Index:**

1. **Acceso al Index:**
   - En satchel de CustomBooks, si el libro tiene `hasIndex=1` y capitulos definidos, se muestra "I: Index" junto a "K: Bookmark | R: Random Page"
   - Presionar I abre el Index

2. **Vista del Index:**
   - Pantalla completa con fondo oscuro (10,8,5,230 alpha)
   - Titulo "Index" centrado arriba (2x font size, color dorado 230,205,160)
   - Lista de capitulos centrados verticalmente
   - Capitulo seleccionado: texto dorado (255,215,0) y mas grande (1.3x)
   - Otros capitulos: texto crema (200,180,140) y tamaño normal (1.1x)
   - Navegacion con flechas arriba/abajo
   - Enter para ir al capitulo seleccionado
   - ESC para cerrar Index

3. **Navegacion automatica:**
   - Al presionar Enter en un capitulo, se calcula la pagina: `s_currentPage = targetLine / linesPerPage`
   - Se cierra el Index automaticamente y se muestra el libro en esa pagina
   - Si el capitulo fue arrancado (titulo contiene "Ripped Sheet"), igual se puede navegar a el

4. **Persistencia de capitulos arrancados:**
   - Al arrancar pagina de CustomBook, se llama `MarkChapterAsRipped(bookName, lineIndex)`
   - Busca el capitulo con `lineIndex` coincidente
   - Cambia titulo a "Ripped Sheet (TituloOriginal)"
   - Reescribe `index.json` con el nuevo titulo
   - Al reabrir el libro, el capitulo arrancado aparece como "Ripped Sheet" en el Index

**Localizacion nueva (INI):**
- Sheet_RippedChapter="Ripped Sheet" (texto que se agrega al titulo del capitulo arrancado)

---

## [Última Build] - 2026-09-02

### Cambios en Journal (Diario Principal)

**Bookmarks mejorados:**
- El texto "K: Open at Bookmark" ya no muestra "(Page XX)", solo el texto limpio
- Al presionar K en la portada (Cover), el journal se abre DIRECTAMENTE en la página del bookmark, sin esperar ENTER
- Lo mismo para R (página aleatoria) - abre directamente sin esperar ENTER
- El texto "Bookmark Saved" / "Bookmark Removed" se bajó 5px para mejor posicionamiento visual

**Colores oscurecidos (estética RDR2):**
- Cover del journal: cuero oscurecido de `(56,37,23)` a `(35,22,12)` - tono cuero oscuro sin ser negro
- Gradiente del cuero: de `(76,52,33)` a `(50,32,18)` y de `(45,29,17)` a `(28,16,8)`
- Páginas: de `(234,223,197)` a `(210,200,175)` - menos brillantes, más realistas
- Envejecido de esquinas: tonos reducidos ~10-15% para mejor integración con el juego

**Animación del listón rojo:**
- Al colocar bookmark (K), el listón rojo se expande por el spine del journal
- Texto "Bookmark Saved" aparece por 2 segundos
- Al quitar bookmark (K en página ya marcada), el listón se contrae
- Texto "Bookmark Removed" aparece por 2 segundos
- Si hay bookmark activo, el listón permanece expandido al abrir el journal

**Tiempo de cierre:**
- ESC hold reducido de 5 segundos a 3 segundos para cerrar el journal

### Cambios en CustomBooks (Satchel de Libros)

**Sistema de búsqueda mejorado:**
- La barra de búsqueda SOLO se activa al hacer click en ella
- Click fuera de la barra desenfoca el input
- Las teclas ya no escriben en la búsqueda a menos que la barra esté enfocada
- Esto evita que al presionar K (bookmark) se escriba "K" en la búsqueda

**Bookmarks en CustomBooks:**
- Al presionar K en el satchel, el libro se abre DIRECTAMENTE (sin esperar ENTER)
- Si tiene bookmark, abre en la página del bookmark
- Si no tiene bookmark, abre desde el principio
- Animación del listón rojo + texto "Bookmark Saved" también funciona en custombooks

**Colores oscurecidos (estética RDR2):**
- Covers de custombooks oscurecidos al 55% del color original (antes 100%)
- Sombras de covers al 35% (antes 60%)
- Los colores ahora pegan mejor con la estética oscura de RDR2

**Lazy Loading para libros grandes:**
- Libros con +3000 caracteres usan lazy loading (carga 500 líneas por chunk)
- Al acercarse al final de un chunk, se carga el siguiente automáticamente
- Esto evita lag/FPS drops al abrir libros grandes como la Biblia

### Sistema de Libros Encontrables

**Nuevos campos en config.ini:**
```ini
[Location]
Findable=1              ; 1 = encontrable en coordenadas, 0 = ya poseído
X=-1842.0               ; Coordenada X del mundo
Y=-1038.0               ; Coordenada Y del mundo
Z=180.0                 ; Coordenada Z del mundo
PickupRadius=10.0       ; Radio de recogida en metros
PickupMessage=Presiona E para obtener el libro
```

**Comportamiento:**
- Cuando el jugador se acerca a las coordenadas (dentro del PickupRadius), aparece el mensaje
- Al presionar E, el libro se marca como `isOwned=1` permanentemente
- El libro aparece en el satchel y puede leerse normalmente
- No requiere modificar código, solo configurar el config.ini del libro

**Libros configurados:**
- `LaBibliaNT1858`: owned=1 (ya poseído), index.json con 27 libros del NT
- `LaBibliaViajeroTiempo`: findable=1, coordenadas placeholder (-1842, -1038, 180), index.json con AT+NT
- `TheBibleNT` y `TheLastRider`: actualizados con sección [Location] vacía

### Localización Completa

**Nuevos strings en WriteYourJourney.ini:**
- `CB_NavHint`, `CB_NavHintRandom`, `CB_SearchHint`, `CB_NoBooks`, `CB_NoMatch`
- `CB_OpenLabel`, `CB_BookmarkPage`, `CB_SetBookmarkFirst`, `CB_OpenBeginning`, `CB_OpenRandom`
- `CB_BookNavHint`, `CB_OpenBookLabel`, `CB_OpenAtBookmark`, `CB_OpenFromStart`
- `CB_SatchelOpening`, `BookmarkSaved`, `BookmarkRemoved`

Todos los textos del sistema ahora son traducibles desde el INI.

---

## Lista de Testing Requerido

### 0. Sheets - Sistema de Hojas Arrancadas (NUEVO)
- [ ] Abrir journal, navegar a pagina con texto/dibujo, verificar "P: Rip Page" en ayuda inferior
- [ ] Mantener P durante 3s: barra de progreso se llena, al completar animacion de hoja saliendo
- [ ] Soltar P antes de 3s: barra se resetea, sin efecto
- [ ] Tras rip exitoso: overlay centrado con bordes rasgados muestra texto y dibujos de la pagina
- [ ] En overlay, presionar ESC: pagina se restaura (vuelve al journal sin danos)
- [ ] En overlay, presionar L: hoja se deja en coords actuales, se crea myjourney/Discoverables/SHEET1/
- [ ] Verificar archivos creados: location.ini (con X/Y/Z), sheet.txt, sheet_draw.dat
- [ ] Cerrar y reabrir journal: pagina arrancada sigue faltante (persistencia en ripped_pages.ini)
- [ ] En vista general, la pagina ripped muestra borde rasgado en vez de pagina crema
- [ ] Seleccionar pagina ripped: muestra "(Page ripped)" centrado
- [ ] Navegar a coords donde se dejo SHEET1: prompt "Hay una hoja arrancada cerca" + "Press E"
- [ ] Presionar E: overlay muestra contenido de la hoja encontrada
- [ ] ESC en overlay de discoverable: cierra sin recoger
- [ ] Crear manualmente myjourney/Discoverables/SHEET99/ con sheet.txt + location.ini (X/Y/Z conocidos)
- [ ] Ir a esas coords: verificar que aparece el prompt y se puede recoger
- [ ] Verificar que no hay conflicto de teclas P/E/L/R/ESC con journal y custombooks
- [ ] Alt+Tab con overlay abierto: cierra todo sin crash
- [ ] Recibir dano con overlay abierto: cierra todo sin crash

### 1. Journal - Bookmarks
- [ ] En portada, el texto debe decir "K: Open at Bookmark" (SIN "(Page XX)")
- [ ] Presionar K en portada debe abrir el journal DIRECTAMENTE en la página del bookmark (sin esperar ENTER)
- [ ] Presionar R en portada debe abrir página aleatoria directamente
- [ ] El texto "Bookmark Saved" debe aparecer 5px más abajo que antes (verificar posición visual)
- [ ] El texto "Bookmark Removed" también debe estar 5px más abajo

### 2. Journal - Colores
- [ ] El cover del journal debe verse como cuero oscuro (no marrón claro como antes)
- [ ] Las páginas deben tener un tono crema más suave, menos brillante
- [ ] Verificar que los colores peguen bien con la estética oscura de RDR2
- [ ] El tinte nocturno debe seguir funcionando correctamente

### 3. Journal - Tiempo de Cierre
- [ ] Mantener ESC debe cerrar el journal en 3 segundos (no 5 como antes)
- [ ] La barra de progreso debe llenarse más rápido

### 4. Journal - Animación del Listón
- [ ] Al presionar K en una página, el listón rojo debe expandirse por el spine
- [ ] Texto "Bookmark Saved" debe aparecer por 2 segundos
- [ ] Al presionar K en página ya marcada, el listón debe contraerse
- [ ] Texto "Bookmark Removed" debe aparecer por 2 segundos
- [ ] Al reabrir el journal con bookmark activo, el listón debe permanecer expandido

### 5. CustomBooks - Búsqueda
- [ ] Abrir el satchel (mantener B 3s)
- [ ] La barra de búsqueda NO debe estar enfocada por defecto
- [ ] Presionar K NO debe escribir "K" en la búsqueda
- [ ] Hacer click en la barra de búsqueda debe enfocarla
- [ ] Click fuera de la barra debe desenfocarla
- [ ] Solo cuando está enfocada, las teclas deben escribir en la búsqueda

### 6. CustomBooks - Bookmarks
- [ ] Presionar K en el satchel debe abrir el libro DIRECTAMENTE (sin esperar ENTER)
- [ ] Si el libro tiene bookmark, debe abrir en esa página
- [ ] Si no tiene bookmark, debe abrir desde el principio
- [ ] La animación del listón rojo debe funcionar igual que en el journal
- [ ] Texto "Bookmark Saved" debe aparecer por 2 segundos

### 7. CustomBooks - Colores
- [ ] Los covers de los libros deben verse más oscuros (55% del color original)
- [ ] Verificar que los colores peguen con la estética de RDR2
- [ ] Las sombras deben ser más oscuras también

### 8. Lazy Loading (Libros Grandes)
- [ ] Abrir LaBibliaNT1858 (957KB) NO debe causar lag
- [ ] Abrir LaBibliaViajeroTiempo (4MB) NO debe causar lag
- [ ] Navegar páginas debe cargar chunks adicionales sin congelar
- [ ] No debe haber FPS drops al abrir libros grandes

### 9. Libros Encontrables
- [ ] LaBibliaViajeroTiempo NO debe aparecer en el satchel inicialmente (isOwned=0)
- [ ] Ir a coordenadas X=-1842, Y=-1038, Z=180 (o ajustar si no son correctas)
- [ ] Al acercarse (radio 10m), debe aparecer el mensaje "Presiona E para obtener la Biblia del Viajero del Tiempo"
- [ ] Presionar E debe otorgar el libro (verificar que config.ini se actualiza a isOwned=1)
- [ ] El libro debe aparecer en el satchel permanentemente después de recogerlo

### 10. Integración General
- [ ] Journal abierto: CustomBooks no debe responder a B
- [ ] CustomBooks abierto: Journal no debe responder a J
- [ ] Alt+Tab debe cerrar journal/satchel inmediatamente
- [ ] Recibir daño debe cerrar el journal
- [ ] Modo pánico (5 ESC rápidos) debe cerrar todo

### 11. Localización
- [ ] Editar WriteYourJourney.ini y cambiar algunos strings
- [ ] Presionar F5 para recargar
- [ ] Verificar que los cambios se apliquen inmediatamente

---

## Notas para el Usuario

**Coordenadas de LaBibliaViajeroTiempo:**
Las coordenadas actuales (-1842, -1038, 180) son placeholder. Debes ajustarlas a una ubicación real del mapa relacionada con el "viajero del tiempo". Para encontrar coordenadas, usa un mod de coordenadas o la consola del juego.

**Crear hojas discoverables manualmente (Easter Eggs):**
Para crear una hoja que el jugador pueda encontrar en el mundo:
1. Crear carpeta `myjourney/Discoverables/SHEET<N>/` (N = numero unico, ej: 99)
2. Crear `sheet.txt` con el texto de la hoja
3. Crear `sheet_draw.dat` (opcional, binario con formato de dibujo)
4. Crear `location.ini` con:
   ```ini
   [Location]
   X=-1234.5
   Y=567.8
   Z=100.0
   PickupRadius=10.0
   PickupMessage=Presiona E para recoger la hoja
   Author=Leuan
   Source=EasterEgg
   ```
5. El sistema la escaneada automaticamente al iniciar sesion

**Documentacion:**
- `CreateSheetsGuide.md` - Guia completa para crear hojas arrancadas manualmente (easter eggs) sin tocar codigo

Ver `CreateBooksGuide.md` para instrucciones completas sobre cómo crear libros custom sin tocar código.

---

*Made with love By Leuan... May god bless you all*
