# Write Your Journey - Contexto Completo para IA

## Resumen del Mod

**Write Your Journey** es un mod para Red Dead Redemption 2 que permite al jugador escribir un diario personal in-game, con la estetica de un cuaderno de cuero de 1899. Todo el arte esta dibujado con primitivas de ImGui (ImDrawList) - sin texturas externas.

El mod se integra con el juego mediante hooks de Vulkan/DX12 y ScriptHookRDR2, permitiendo al jugador abrir un diario, escribir en sus paginas, dibujar, navegar entre capitulos, leer libros custom, arrancar hojas y dejarlas en el mundo, y guardar todo en archivos de texto.

---

## Estructura de Archivos

### Archivos Principales

| Archivo | Lineas | Descripcion |
|---------|--------|-------------|
| `main.cpp` | 195 | Entry point del DLL. Lee configuracion (system.xml o config.txt), detecta API grafica (Vulkan/DX12), inicializa hooks y registra el script. `DllMain` espera ventana del juego antes de hookear (fix May 2026). |
| `script.cpp` | 675 | Hilo de ScriptHook. Maneja input de teclas (Y para abrir, ESC para cerrar, SOS), bloqueo de controles, animaciones, reload R+P 5s, pickup de sheets con caminata+crouch, y loop principal. |
| `menu.cpp` | 2521 | UI completa del diario. Maquina de estados (Cover/Open), dibujo del libro con ImDrawList, modos Read/Write/Draw, zoom, navegacion de paginas, guardado/carga, integracion con Sheets y CustomBooks. |
| `menu.h` | 59 | Declaracion de `CImGuiMenu` con API publica: `Render()`, `OpenSession()`, `CloseSession()`, `SaveText()`, getters/setters para estado. |
| `config.h` | 244 | Sistema de configuracion INI. Carga teclas, nombres, localizacion, secciones [Settings], [CustomBooks], [RipSheets], [Localization]. Valida la "bendicion" del mod. |
| `keyboard.h` | 16 | Declaraciones de input: `IsKeyJustUp()`, `IsKeyDown()`, `ResetKeyState()` usando VK codes. |
| `keyboard.cpp` | 53 | Implementacion de input con array de estados de teclas. |

### Sistema CustomBooks (Satchel de Libros)

| Archivo | Lineas | Descripcion |
|---------|--------|-------------|
| `custombooks.cpp` | 1966 | Sistema completo de libros custom. Inventory (carousel con cover), lectura de libros con lazy loading, Index de capitulos, busqueda, bookmarks, seleccion de paginas, modo edicion, tracking de paginas ripeadas/restauradas. |
| `custombooks.h` | 112 | Structs: `BookConfig`, `BookChapter`, `TextEdit`, `CustomBook`, `RibbonAnim`. API del namespace `CustomBooks`. |

### Sistema Sheets (Hojas Arrancadas)

| Archivo | Lineas | Descripcion |
|---------|--------|-------------|
| `sheets.cpp` | 1524 | Sistema de hojas arrancadas. Rip de paginas (P hold 3s), overlay de hoja, discoverables en el mundo, caminata+crouch para pickup, animaciones de rip/restore/flip, KeepSheet, LeaveSheet. |
| `sheets.h` | 123 | Structs: `SheetDrawingLine`, `SheetDrawing`, `RippedSheetCache`, `DiscoverableSheet`. API del namespace `Sheets`. |

### Hooks de Renderizado

| Archivo | Lineas | Descripcion |
|---------|--------|-------------|
| `Hook/Manager.h` | 141 | Clase `CImGuiHookManager` que orquesta hooks. Contiene `GetGameWindow()` con deteccion de ventana invalidada. Funcion `Log()` para debugging. |
| `Hook/Manager.cpp` | 39 | Statics de `CImGuiHookManager`. `Initialize()` y `Shutdown()` delegan al hook correspondiente. |
| `Hook/Vulkan.cpp` | 1142 | Hook de Vulkan (kiero+MinHook). Intercepta `vkQueuePresentKHR`, `vkCreateSwapchainKHR`, `vkAcquireNextImageKHR`. Tracking de swapchains, fix de reset completo de ImGui al cambiar ventana/resolucion. |
| `Hook/DX12.cpp` | 380 | Hook de DirectX 12 (kiero). Intercepta `Present()`, `ResizeBuffers()`, `ExecuteCommandLists()`. |
| `Hook/Win32.cpp` | 27 | Subclass del WndProc para forwarding de input a ImGui cuando el diario, satchel, book, o sheet overlay estan abiertos. |

### Soporte (Librerias Externas)

| Directorio | Descripcion |
|------------|-------------|
| `imgui/` | ImGui 1.89.5 + backends (Win32, DX12, Vulkan). |
| `kiero/` | Libreria para hooking de APIs graficas. |
| `minhook/` | Libreria MinHook para hooking de funciones. |
| `inc/` | ScriptHookRDR2 SDK: `natives.h`, `types.h`, `enums.h`, `main.h`, `nativeCaller.h`. |

---

## Features Actuales

### 1. Diario (Journal)

**Controles principales:**
- **J** (configurable): Abrir diario (mantener 3s o inmediato segun `InstantOpen`)
- **ENTER**: Pasar de portada (Estado 1/Cover) a libro abierto (Estado 2/Open)
- **W**: Modo escritura (`InputTextMultiline` invisible sobre la pagina)
- **R**: Modo lectura / Modo zoom (si ya esta en lectura)
- **D**: Modo dibujo (canvas de trazos)
- **ESC**: Deseleccionar pagina (en Estado 2) / Cerrar (mantener 3s desde vista general)
- **Y 10s (SOS)**: Cierre forzoso (failsafe)
- **V**: Apreciar la vista (congela controles pero permite mirar con la camara)
- **K**: Bookmark (guardar/ir a pagina marcada)
- **P**: Arrancar pagina (hold 3s) - solo si `[RipSheets] enableRipSheetSystem=1`
- **R+P 5s**: Recargar configuracion (ambas teclas simultaneas)

**Navegacion en vista general:**
- **Flechas**: Enfocar pagina izquierda/derecha (2da vez: pasar hoja)
- **ENTER**: Seleccionar pagina enfocada
- **Click mouse**: Seleccionar pagina (en vista general con transicion > 0.65)

**Modo Zoom (R sobre pagina seleccionada):**
- Scroll vertical para leer textos largos
- **F**: Toggle fuente caligrafica/legible
- **R**: Salir del zoom
- **ESC**: Deseleccionar pagina

**Modo Dibujo (D):**
- **E**: Toggle goma de borrar
- **Z/X**: Aumentar/disminuir radio de goma (8-80px, step 4px)
- **SHIFT**: Panel de herramientas (pincel, grafito, crayon, color, grosor, Z-order)
- Trazos se guardan normalizados (0..1) para ser resolution-independent
- Formato binario con magic number `0x574A4402`

**Formato de Texto (SHIFT en escritura):**
- Tamano: Normal / Titulo
- Estilos: Negrita, Cursiva, Subrayado, Tachado
- Alineacion: Izquierda, Centro, Derecha

**Sistema de Archivos del Journal:**
- Guardado por pagina: `myjourney/Myself/C<capitulo>/pag<num>.txt`
- Guardado de dibujos: `myjourney/Myself/C<capitulo>/pag<num>_draw.dat`
- Auto-guardado inmediato tras cada caracter + red de seguridad cada 8s
- Cache en memoria (`s_pageCache`, `s_drawingCache`) por numero de pagina

### 2. CustomBooks (Satchel de Libros)

**Archivos de libros:** `MyJourney/Books/[NombreInterno]/` con:
- `body.txt` - Contenido del libro (lineas de texto)
- `index.json` - Capitulos con `{"title": "...", "line": N}`
- `config.ini` - Configuracion del libro (ver abajo)
- `edits.txt` - Ediciones de texto (formato `lineIndex|startChar|endChar|original|replacement`)

**Configuracion del libro (config.ini):**
```ini
[Display]
DisplayTitle=La Biblia NT 1858
Author=Varias
Year=1858
Category=Religion
CoverColorRGB=139,69,19
FontType=1
FontSizeOverride=0
InkColor=Sepia
TextAlignment=0
PreserveLineBreaks=1
AllowsOpenRandomPage=1
HasIndex=1
AutoOrderPages=1
isOwned=1

[Location]
Findable=0
X=-1842.0
Y=-1038.0
Z=180.0
PickupRadius=10.0
PickupMessage=Presiona E para obtener el libro
```

**Controles del Satchel:**
- **B** (configurable): Mantener 3s para abrir satchel de libros
- **Flechas <- ->**: Navegar entre libros (carousel con cover grande)
- **ENTER**: Abrir libro seleccionado
- **K**: Abrir en bookmark (o desde inicio si no tiene)
- **R**: Abrir pagina aleatoria
- **I**: Abrir Index (requiere I + ENTER para confirmar)
- **ESC**: Cerrar satchel
- **Barra de busqueda**: Click para enfocar, filtra por titulo/autor/categoria

**Controles dentro del libro:**
- **Flechas**: Pasar pagina (navegacion por pares)
- **K**: Bookmark (con animacion de liston rojo)
- **ENTER**: Seleccionar pagina (modo seleccion)
- **Click mouse**: Seleccionar pagina especifica
- **ESC**: Deseleccionar pagina / Cerrar libro (vuelve al satchel)

**Controles con pagina seleccionada:**
- **P**: Arrancar pagina (hold 3s via Sheets system)
- **E**: Modo edicion (escribir texto de reemplazo)
- **ENTER**: Confirmar edicion (tachado + texto arriba)
- **ESC**: Deseleccionar

**Lazy Loading (libros grandes):**
- Libros con +3000 caracteres usan carga por chunks (500 lineas por chunk)
- Trigger: al acercarse al final del chunk cargado
- `lazyTotalLines` se calcula leyendo el archivo completo en `OpenBook()`
- `lazyTotalChars` determina si se usa lazy loading

**Index de Capitulos:**
- Se abre con I (desde satchel con libro seleccionado) + ENTER
- Se dibuja como pagina del mismo libro (cover, pergamino, spine)
- Capitulos listados con highlight en la seleccion
- Flechas arriba/abajo para navegar
- Enter para ir al capitulo (calcula pagina: `targetLine / (linesPerPage * 2)`)
- Capitulos arrancados muestran "Ripped Sheet (TituloOriginal)"

**Paginas Ripeadas en CustomBooks:**
- Tracking en `s_rippedCustomBookPages` (mapa bookName -> set de paginas)
- Visual: borde rasgado + "Page Ripped" + glow pulsante azul fuerte (4 capas)
- No se puede seleccionar directamente una pagina ripped
- Partner page (par N -> N+1, impar N>1 -> N-1) tambien se marca

**Paginas Restauradas:**
- Tracking en `s_damagedCustomPages` y via `Sheets::IsPageRestored()`
- Visual: fondo mas oscuro (175,165,140) + overlay de dano (arrugas, manchas, tajos, bordes rotos)
- El overlay de dano se dibuja con `GetForegroundDrawList()` DESPUES del texto

**Ediciones de Texto:**
- Se guardan en `edits.txt` con formato pipe-delimited
- Visual: texto original tachado + texto de reemplazo arriba en tamano pequeno (0.6x)
- Persisten entre sesiones

### 3. Sistema de Sheets (Hojas Arrancadas)

**Flujo de Rip de pagina (P hold 3s):**
1. En journal/custombook con pagina seleccionada, se muestra "P: Rip Page"
2. Mantener P 3s con barra de progreso
3. Animacion de hoja saliendo del libro (0.9s, deslizamiento + rotacion + alpha)
4. Overlay centrado con bordes irregulares muestra contenido
5. Partner page (trasera) tambien se marca como ripped

**Overlay de hoja arrancada:**
- Fondo oscurecido + hoja con bordes poligonales irregulares
- Muestra texto completo y trazos de dibujo escalados
- Ayudas: "L: Leave Page here | R: Look Behind | ESC: Add page back"
- **ESC**: Restaura pagina original (con animacion de hoja volviendo + overlay de dano permanente)
- **L**: Deja hoja en coords actuales del jugador (crea Discoverable)
- **R**: Flip 3D (escala horizontal coseno, 0.8s) para ver trasera
- **K**: Keep the Sheet (solo discoverables) - restaura contenido a ubicacion original + marca como daniada permanentemente

**Sistema Discoverables (hojas en el mundo):**
- Directorio: `myjourney/Discoverables/SHEET<N>/` con:
  - `location.ini` - [Location] con X, Y, Z, PickupRadius, Author, Source, OriginalPage, FromJournal, BookName, Chapter, Collected
  - `sheet.txt` - Texto de la hoja
  - `sheet_draw.dat` - Dibujos de la hoja
  - `sheet_back.txt` - Texto de la trasera
  - `sheet_back_draw.dat` - Dibujos de la trasera
- Generacion incremental: SHEET1, SHEET2, ... (nunca reutiliza numeros)
- Persistencia de paginas ripped en `myjourney/ripped_pages.ini`
- Persistencia de paginas daniadas en `myjourney/damaged_pages.ini`
- Escaneo al iniciar (`Sheets::Init()` se llama al inicio de `main()` en script.cpp)

**Pickup de sheets del mundo:**
- Prompt de proximidad: al acercarse (<5m), muestra mensaje + icono de tecla
- Presionar tecla configurable (default R): personaje camina hacia la sheet
- Al llegar: animacion de crouch (1.2s, `amb_rest@world_human_bottle_pickup`)
- Al terminar crouch: overlay se muestra + controles bloqueados
- Cancelacion: si el jugador se mueve durante caminata, se cancela
- Tracker manual de key state (`s_pickupKeyWasDown`) para evitar bug de `0x0001` consumido

**KeepSheet (K en discoverable):**
- Restaura contenido a ubicacion original (journal o custombook)
- Para journal: copia sheet.txt -> pagX.txt, sheet_draw.dat -> pagX_draw.dat
- Para custombook: reemplaza lineas en body.txt
- Marca pagina como daniada permanentemente (visual de arrugas sobre texto)
- Elimina carpeta del discoverable
- Marca como collected en location.ini

**Paginas Restauradas (ESC en overlay):**
- Se marcan en `s_damagedJournalPages` / `s_damagedCustomPages`
- Visual unico: fondo mas oscuro (175,165,140) + overlay de dano
- `DrawRestoredPage()` dibuja solo el fondo
- `DrawRestoredPageDamageOverlay()` dibuja arrugas (14 lineas), manchas (8 circulos), pliegues (5 lineas), bordes rasgados (top+bottom con poligonos), tajos laterales (6 lineas), manchas circulares (4)
- El overlay se dibuja con `GetForegroundDrawList()` DESPUES del texto

**Animaciones:**
- Rip: 0.9s (deslizamiento + rotacion + alpha fade)
- Restore: 0.9s (hoja volviendo al journal con efecto de arrugado sin(t*PI))
- Flip: 0.8s (escala horizontal coseno para ver trasera)
- Crouch: 1.2s (animacion de agacharse)

### 4. Iluminacion Dinamica

- Tinte nocturno sobre el pergamino (21:00 - 06:00) segun `CLOCK::GET_CLOCK_HOURS()`
- De noche: `IM_COL32(60, 55, 45, 160)` - calido y oscuro
- De dia: `IM_COL32(100, 90, 75, 100)` - sutil
- `script.cpp` actualiza `CImGuiMenu::SetWorldHour()` cada frame

### 5. Configuracion (WriteYourJourney.ini)

```ini
[Settings]
OpenJournalKey=J          ; Tecla para abrir (una letra A-Z)
InstantOpen=0             ; 0=mantener 3s, 1=inmediato
EnableDeveloperLog=0      ; Log a WriteYourJourney-ByLeuan.log
MyName=                   ; Nombre para el titulo del diario (vacio = detectar Arthur/John)
ReloadButton=VK_F5        ; Tecla para recargar config (F1-F12 o letra)

[CustomBooks]
Enabled=1                 ; 1=activado, 0=desactivado
Key=B                     ; Tecla para abrir satchel (A-Z)

[RipSheets]
enableRipSheetSystem=1    ; 1=activado, 0=desactivado (controla todo el sistema)
ripSheetPickupKey=R       ; Tecla para recoger sheets del mundo (A-Z)

[Localization]
Help_Cover=...
Help_Overview=...
Help_Zoom=...
Help_Draw=...
Help_Write=...
CB_NavHint=...
CB_SearchHint=...
Sheet_RipHint=P: Rip Page
Sheet_Nearby=There is a ripped page nearby
Sheet_PressE=Press R to pick it up
Sheet_PageRipped=Page Ripped
Sheet_KeepSheet=K: Keep the Sheet
Sheet_LookBehind=R: Look Behind
; ... (ver config.h para lista completa de 30+ claves)
```

### 6. Seguridad y Failsafes

- Validacion de "bendicion" en el INI (busca string "- Made with love By Leuan...")
- Failsafe contra Alt+Tab (cierra el diario inmediatamente si pierde foco)
- Cierre por dano recibido (`IS_PED_INJURED` / `IS_PED_FATALLY_INJURED`)
- SOS: Y 10s fuerza cierre total sin validaciones
- Modo Panico: 5 toques de ESC en < 2s = cierre inmediato (TODO #10, implementado parcialmente)
- Check de foco leniente para borderless windowed (visible + no minimizada)

---

## Arquitectura y Flujo de Ejecucion

### Hilos

1. **Hilo de Script** (`script.cpp`): ScriptHookRDR2, corre natives, lee teclas con `GetAsyncKeyState`, bloquea controles
2. **Hilo de Render** (Hook/Vulkan.cpp o DX12.cpp): Present hook, dibuja ImGui, `CImGuiMenu::Render()`
3. **Hilo de DllMain** (`main.cpp`): Espera ventana del juego, inicializa hooks

### Reglas de Hilos (INVARIANTES CRITICAS)

- **Natives SOLO en script.cpp** (hilo de ScriptHook)
- **ImGui SOLO en Render()** (hilo de render)
- Comunicacion entre hilos via `std::atomic<>` (ej: `s_worldHour`, `s_chapter`, `s_escProgress`)
- ESC se lee en script.cpp con `GetAsyncKeyState(VK_ESCAPE)` (no depender de ImGui/WndProc)
- SOS usa `TIMERB`; ESC usa `TIMERA` (no mezclar)

### Flujo de Render (CImGuiMenu::Render())

```
Render()
  -> CustomBooks::HandleInput()
  -> Sheets::HandleInput()
  -> Si Sheets overlay/anim/rip: Sheets::Render() + return
  -> Si CustomBooks inventory abierto: RenderInventory() + return
  -> Si CustomBooks index abierto: RenderIndex() + return
  -> Si CustomBooks book abierto: RenderBook() + return
  -> CustomBooks::RenderPickupPrompt()
  -> Si Sheets E hold: Sheets::RenderEHoldPrompt()
  -> Si Sheets discoverable overlay (sin journal): Sheets::Render() + return
  -> Si journal no abierto: Sheets::RenderPickupPrompt() + return
  -> Si "Apreciar la vista": return (no dibujar ImGui)
  -> Check cambio de capitulo (guardar cache, limpiar)
  -> Check pagina seleccionada ripped (deseleccionar)
  -> HandleInput()
  -> Animaciones (fadeIn, transition, pairFlip, zoom)
  -> DrawCover() o DrawOpenBook()
  -> Si Open y transicion > 0.65:
      -> Previews de paginas / DrawReadPage / DrawWritePage / DrawDrawingCanvas
      -> HandleDrawingInput() si modo Draw
      -> DrawRestoredPageDamageOverlay() con ForegroundDrawList
  -> DrawZoomPage() si zoom activo
  -> Animacion de liston (bookmark)
  -> Sheets::Render() si overlay/anim/rip
  -> DrawHelp() + DrawEscProgress()
  -> Auto-guardado periodico (8s)
```

### Hook de Vulkan (detalles criticos)

- RDR2 presenta en MULTIPLES swapchains (ventana real + overlays Social Club/Steam)
- `hk_vkCreateSwapchainKHR` registra cada swapchain con su extent (mapa protegido por `g_SwapchainCS`)
- `g_MainSwapchain` se identifica si su extent coincide con `GetClientRect` de la ventana del juego
- `PickMainSwapchain()`: solo se dibuja en el principal; fallback = mayor area
- Contexto unico: `g_ImGuiContext` global; SIEMPRE `SetCurrentContext()` tras crearlo
- `FullImGuiReset()`: detecta cambio de HWND y recrea todo (contexto, backends, fuentes, WndProc)
- Fix de focus check ANTES de resetear fences (evita freeze en borderless windowed)
- `ImGui_ImplVulkan_Init` + `CreateFontsTexture` van DESPUES de `vkBeginCommandBuffer` y ANTES de `vkCmdBeginRenderPass`

### Fuentes

- Indice 0: Default de ImGui (fallback)
- Indice 1: MV Boli (`C:\Windows\Fonts\mvboli.ttf`, 34px) - manuscrita/cursiva
- Indice 2: Default de ImGui (alternativa para toggle F en zoom)
- Se cargan en la init de DX12.cpp/Vulkan.cpp y en `FullImGuiReset()`

---

## Integracion entre Sistemas

### Win32.cpp (Input Forwarding)
```cpp
if (CImGuiMenu::GetIsOpen() || CustomBooks::IsInventoryOpen() || 
    CustomBooks::IsBookOpen() || Sheets::IsShowingOverlay())
    -> forward a ImGui
```

### script.cpp (Bloqueo de Controles)
```
Loop principal:
  1. Check R+P 5s reload
  2. Si CustomBooks inventory/book/index abierto -> LockControls + continue
  3. Si Sheets overlay abierto (sin journal) -> LockControls + continue
  4. Si Sheets caminando -> manejar caminata + crouch + pickup + overlay loop
  5. Si journal no activo:
     - CustomBooks pickup prompt + pickup con E
     - Sheets pickup prompt + pickup con tecla configurable (R default)
  6. Si journal activo:
     - Apreciar la vista / LockControls
     - Failsafe Alt+Tab
     - SOS (Y 10s)
     - ESC hold (3s) + modo panico
     - HandleDamageInterrupt()
     - SetWorldHour() + SetChapter()
     - UpdateJournalTitle()
```

### menu.cpp (Render Order)
```
Render() respeta prioridad:
  1. Sheets overlay/anim/rip (maxima prioridad)
  2. CustomBooks inventory/index/book
  3. Sheets pickup prompt / E hold prompt
  4. Journal (si esta abierto)
  5. Sheets progress bar (si ripping mientras journal abierto)
```

---

## Fix de Resolucion/Window Mode

**Problema**: Al hacer alt+tab, cambiar resolucion, o alternar fullscreen/windowed/borderless, ImGui dejaba de dibujarse.

**Causa**: RDR2 recrea su HWND al cambiar modo. ImGui guardaba el HWND viejo, y `GetClientRect(HWND_destruido)` devolvia `{0,0,0,0}`.

**Solucion Vulkan**: En `Vulkan.cpp`, detectar cambio de HWND cada frame y hacer `FullImGuiReset()`:
1. Esperar GPU (fences)
2. Destruir contexto + backends (Vulkan + Win32)
3. Limpiar render targets
4. Recrear todo desde cero con el HWND nuevo
5. Re-subclass del WndProc

**Solucion DX12**: `hk_ResizeBuffers` marca `g_ResizePending` + guarda parametros. En el siguiente Present, se recrean render targets con el nuevo tamano.

---

## Build

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" `
  "src\ImGuiRDR2Hook.sln" /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 `
  "/p:OutDir=C:\Users\evanm\Desktop\"
```

**Requisitos**:
- Visual Studio 2022 Build Tools (v143)
- Vulkan SDK (`$env:VULKAN_SDK`)
- ScriptHookRDR2 SDK (libreria `ScriptHookRDR2.lib`)

**Output**: `WriteYourJourney.asi` -> copiar a carpeta del juego

**Notas**:
- El vcxproj dice toolset `v145` (no instalado) -> SIEMPRE override `/p:PlatformToolset=v143`
- OutDir por defecto es `C:\Program Files\Rockstar Games\...` -> sin admin da LNK1104
- `_LOGGING_ENABLED 1` en `Hook/Manager.h` -> log en `ImGuiRDR2Hook.log` (carpeta del juego)

---

## Creditos

- **Base**: [ImGuiRDR2Hook](https://github.com/Halen84/ImGuiRDR2Hook) por Halen84
- **DX12**: [Sh0ckFR/Universal-Dear-ImGui-Hook](https://github.com/Sh0ckFR/Universal-Dear-ImGui-Hook)
- **Vulkan**: [bruhmoment21/UniversalHookX](https://github.com/bruhmoment21/UniversalHookX)
- **Mod**: Write Your Journey por Leuan

---

*Made with love By Leuan... May god bless you all*
