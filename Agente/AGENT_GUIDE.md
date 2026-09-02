# "Write your Journey" — Guia para IA (contexto rapido)

Mod "Write Your Journey" sobre el proyecto base ImGuiRDR2Hook
(ScriptHookRDR2 + ImGui 1.89.5, hooks kiero+MinHook para DX12/Vulkan).
Todo el arte es primitivas ImDrawList: PROHIBIDO texturas/ImGui::Image.

## Flujo del mod
- **J**: abre el diario (animaciones desactivadas: `USE_JOURNAL_ANIMS=false`).
- **ENTER**: portada (Estado 1) -> libro abierto (Estado 2).
- **W**: modo escritura (`InputTextMultiline` invisible sobre pagina derecha). **R**: modo lectura.
- **ESC 2s**: guarda y cierra. **SOS: Y 10s** fuerza cierre total (failsafe).
- Guardado: `myjourney/Myself/C1/pag1.txt` (relativo a la raiz del juego), C++17 `<filesystem>/<fstream>`.

## Sistemas adicionales

### CustomBooks (Satchel de libros)
Sistema aislado para leer libros custom. NO modifica el journal.
- **B** (configurable): Mantener 3s para abrir satchel de libros.
- **Flechas ← →**: Navegar entre libros (uno a la vez, cover grande estilo journal).
- **ENTER**: Abrir libro seleccionado.
- **ESC**: Cerrar satchel.
- Archivos: `MyJourney/Books/[NombreInterno]/` con `body.txt`, `index.json`, `config.ini`.
- Configuración en INI: `[CustomBooks]` section con `Enabled=1` y `Key=B`.
- Bloqueo de controles independiente en `script.cpp` (línea ~384).

### Eraser dinámico (modo dibujo)
- **E**: Alternar modo goma.
- **Z/X**: Aumentar/disminuir radio de borrado (8-80px, step 4px).
- Círculo blanco vacío muestra el área de acción.
- Borra solo píxeles dentro del radio (no líneas completas).
- Texto hardcodeado "Z: + | X: -" en pantalla cuando está activo.

## Archivos a leer (en este orden)
1. `src/ImGuiRDR2Hook/script.cpp` — hilo de ScriptHook: tecla J, SOS, ESC 2s
   con `SETTIMERA/TIMERA`, bloqueo por frame y guardado/cierre.
   **Importante**: Línea ~384 tiene bloqueo de controles para CustomBooks (independiente del journal).
2. `src/ImGuiRDR2Hook/menu.cpp` (+`menu.h`) — maquina de estados (Cover/Open x Read/Write),
   todo el dibujo ImDrawList y las ventanas invisibles de lectura/escritura.
   **CustomBooks**: Solo tiene `#include "custombooks.h"` y llamadas en `Render()` con checks de estado.
3. `src/ImGuiRDR2Hook/custombooks.cpp` (+`custombooks.h`) — Sistema de libros custom COMPLETAMENTE AISLADO.
   Tiene su propio `HandleInput()`, `RenderInventory()`, `RenderBook()`.
   NO modificar lógica del journal para añadir funcionalidad de books.
4. `src/ImGuiRDR2Hook/Hook/Vulkan.cpp` — hook de render del usuario (juega en Vulkan).
5. `src/ImGuiRDR2Hook/keyboard.h` — `IsKeyJustUp/IsKeyDown/ResetKeyState` (VK codes).
6. `inc/natives.h` — natives con namespace: `PLAYER::`, `TASK::`, `STREAMING::`, `HUD::`,
   `PAD::DISABLE_ALL_CONTROL_ACTIONS`, `CAM::_FREEZE_GAMEPLAY_CAM_THIS_FRAME`, `BUILTIN::SETTIMERA/TIMERA/TIMERB`.
7. Si toca DX12: `Hook/DX12.cpp`. `Hook/Win32.cpp` reenvia input a ImGui cuando el diario O satchel esta abierto.

## Invariantes CRITICAS (no romper)
- Clase se llama `CImGuiMenu` porque DX12/Vulkan/Win32 la referencian. API clave:
  `Render()`, `GetIsOpen/SetIsOpen`, `OpenSession/CloseSession`, `SaveText`,
  `SetEscHoldProgress`, `IsWriteMode`, `ShouldDrawMouse`.
- Natives SOLO en el hilo de script (script.cpp). ImGui SOLO en `Render()` (hilo de render).
- ESC se lee en script.cpp con `GetAsyncKeyState(VK_ESCAPE)` (no depender de ImGui/WndProc).
- SOS usa `TIMERB`; ESC usa `TIMERA` (no mezclar). Tras SOS: `ResetKeyState('J')` o se reabre al soltar.
- Font: se usa `io.Fonts->Fonts[1]` si existe (el usuario carga su .ttf cursiva en la init
  de DX12.cpp/Vulkan.cpp, junto a `AddFontDefault()`); si no, default.
- `menu.h`/`Win32.cpp`: NO hay F10 ni toggles legacy; no re-introducirlos.
- Cerrar = `CloseSession()` (oculta UI) -> `SaveText()` -> anim opcional -> `s_active=false`.
- **CustomBooks**: Sistema aislado. NO tocar `DrawReadPage()`, `DrawWritePage()`, `DrawDrawingCanvas()`, `HandleDrawingInput()` del journal para añadir funcionalidad de books.
- **Rutas de guardado**: `myjourney/Myself/C1/pagX.txt` y `pagX_draw.dat`. NO cambiar `SaveDirPath()`, `PageFilePath()`, `DrawingFilePath()`.

## Estructura de archivos del proyecto

### Sistema principal (Journal)
- `script.cpp`: Hilo de script, input de teclas, bloqueo de controles
- `menu.cpp`/`menu.h`: UI del journal, máquina de estados, render
- `config.h`: Configuración INI (`[Settings]`, `[Localization]`)
- `keyboard.h`: Funciones de input

### Sistema CustomBooks (Aislado)
- `custombooks.cpp`/`custombooks.h`: Sistema completo de libros custom
- Archivos de libros: `MyJourney/Books/[Nombre]/body.txt`, `index.json`, `config.ini`
- Configuración INI: `[CustomBooks]` section

### Hooks
- `Hook/Manager.h`: Clase `CImGuiHookManager`
- `Hook/Vulkan.cpp`: Hook de Vulkan
- `Hook/DX12.cpp`: Hook de DirectX 12
- `Hook/Win32.cpp`: Subclass WndProc para input forwarding

## Como funciona el hook de Vulkan (leer antes de tocar Vulkan.cpp)
El juego presenta en MULTIPLES swapchains (ventana real + overlays Social Club/Steam),
cada uno posiblemente en su propio hilo. Reglas:
- **Contexto unico**: los contextos ImGui son thread-local. Existe `g_ImGuiContext` global;
  SIEMPRE `ImGui::SetCurrentContext(g_ImGuiContext)` tras crearlo (NUNCA fiarse de `GetCurrentContext()`).
- **Filtro de swapchain**: `hk_vkCreateSwapchainKHR` registra cada swapchain con su extent real
  (mapa protegido por `g_SwapchainCS`) y marca `g_MainSwapchain` si su extent coincide con el
  client rect de la ventana del juego (`ExtentMatchesGameWindow`) o si reemplaza al anterior
  principal (`oldSwapchain == g_MainSwapchain`, cadena de resize/resolucion).
  `PickMainSwapchain()` en QueuePresent: solo se dibuja en el principal; si ninguno coincide,
  fallback = el presentado de MAYOR area. Nunca hay resoluciones hardcodeadas: el extent se toma
  siempre del swapchain real o de `GetClientRect` en runtime.
- El bucle de present hace `continue` en cualquier swapchain != principal.
- `ImGui_ImplVulkan_Init` + `CreateFontsTexture` van DESPUES de `vkBeginCommandBuffer` y ANTES de
  `vkCmdBeginRenderPass` (los mandos de transferencia no son validos dentro del render pass).
- Si el juego recrea el swapchain principal: se actualiza extent + `CleanupRenderTarget()` y el
  render target se reconstruye solo en el siguiente present (`g_Frames[0].Framebuffer == NULL`).

## Configuración INI (WriteYourJourney.ini)

### [Settings]
- `OpenJournalKey=J`: Tecla para abrir journal
- `InstantOpen=0`: 0=mantener 3s, 1=inmediato
- `EnableDeveloperLog=0`: Log de desarrollo
- `MyName=`: Nombre personalizado (vacío = detectar Arthur/John)
- `ReloadButton=F5`: Tecla para recargar config

### [CustomBooks]
- `Enabled=1`: 1=activado, 0=desactivado
- `Key=B`: Tecla para abrir satchel (A-Z)

### [Localization]
- Textos de la UI (Help_Cover, Help_Overview, etc.)

## Build
```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" `
  "src\ImGuiRDR2Hook.sln" /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 `
  "/p:OutDir=<carpeta local>\" 
```
- El vcxproj dice toolset `v145` (no instalado aqui) -> SIEMPRE override `/p:PlatformToolset=v143`.
- OutDir por defecto es `C:\Program Files\Rockstar Games\Red Dead Redemption 2` -> sin admin da
  LNK1104; usar `OutDir` alternativo y copiar el `.asi` manualmente al escritorio SIEMPRE idealmente.
- Requiere `$env:VULKAN_SDK` (`C:\VulkanSDK\...`).
- `_LOGGING_ENABLED 1` en `Hook/Manager.h` -> log en `ImGuiRDR2Hook.log` (carpeta del juego).

## Reglas de desarrollo (ver AI_RULES.md para detalles)
1. **NO tocar sistemas funcionales**: Si algo funciona, no se modifica.
2. **Aislamiento**: Sistemas nuevos en archivos propios (referencia: CustomBooks).
3. **Integración mínima**: Checks de estado, no modificar lógica existente.
4. **Commits ordenados**: Formato descriptivo, frecuencia después de cada feature/fix.
5. **Testing**: Verificar que todo funciona antes de commitear.
6. **CHANGELOG.md**: Después de cada compilación exitosa en el escritorio, ACTUALIZAR `CHANGELOG.md` con:
   - Lista detallada de todos los cambios realizados (qué se modificó, valores antes/después)
   - Lista de cosas a testear por el usuario (checklist con `[ ]` para que el usuario marque)
   - Notas importantes si las hay (coordenadas, configuración, etc.)
   - El changelog debe ser claro y específico para que el usuario sepa exactamente qué probar en el juego

## Gotchas conocidos
- `WAIT()` (main.h) solo en el hilo de script; los yields fuera del bucle usan `WaitLocked()`.
- `DISABLE_ALL_CONTROL_ACTIONS` esta en `PAD::`, no en `MISC::`.
- windows.h define macros min/max: los cpp nuevos llevan `#define NOMINMAX` arriba.
- Warnings C4326 (`void main`) y C4267 del DX12 son pre-existentes del SDK, ignorar.
- **CustomBooks**: `HandleInput()` se ejecuta siempre, pero `RenderInventory()`/`RenderBook()` solo si `!GetIsOpen()`.
- **Bloqueo de controles**: CustomBooks tiene su propio bloqueo en `script.cpp` línea ~384, independiente del journal.
