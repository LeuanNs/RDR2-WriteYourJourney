# "Write your Journey" — Guia para IA (contexto rapido)

Mod "Write Your Journey" sobre el proyecto base ImGuiRDR2Hook
(ScriptHookRDR2 + ImGui 1.89.5, hooks kiero+MinHook para DX12/Vulkan).
Todo el arte es primitivas ImDrawList: PROHIBIDO texturas/ImGui::Image.

## Flujo del mod
- **Y**: abre el diario (animaciones desactivadas: `USE_JOURNAL_ANIMS=false`).
- **ENTER**: portada (Estado 1) -> libro abierto (Estado 2).
- **W**: modo escritura (`InputTextMultiline` invisible sobre pagina derecha). **R**: modo lectura.
- **ESC 2s**: guarda y cierra. **SOS: Y 10s** fuerza cierre total (failsafe).
- Guardado: `myjourney/C1/pag1.txt` (relativo a la raiz del juego), C++17 `<filesystem>/<fstream>`.

## Archivos a leer (en este orden)
1. `src/ImGuiRDR2Hook/script.cpp` — hilo de ScriptHook: tecla Y, SOS, ESC 2s
   con `SETTIMERA/TIMERA`, bloqueo por frame y guardado/cierre.
2. `src/ImGuiRDR2Hook/menu.cpp` (+`menu.h`) — maquina de estados (Cover/Open x Read/Write),
   todo el dibujo ImDrawList y las ventanas invisibles de lectura/escritura.
3. `src/ImGuiRDR2Hook/Hook/Vulkan.cpp` — hook de render del usuario (juega en Vulkan).
4. `src/ImGuiRDR2Hook/keyboard.h` — `IsKeyJustUp/IsKeyDown/ResetKeyState` (VK codes).
5. `inc/natives.h` — natives con namespace: `PLAYER::`, `TASK::`, `STREAMING::`, `HUD::`,
   `PAD::DISABLE_ALL_CONTROL_ACTIONS`, `CAM::_FREEZE_GAMEPLAY_CAM_THIS_FRAME`, `BUILTIN::SETTIMERA/TIMERA/TIMERB`.
6. Si toca DX12: `Hook/DX12.cpp`. `Hook/Win32.cpp` reenvia input a ImGui cuando el diario esta abierto.

## Invariantes CRITICAS (no romper)
- Clase se llama `CImGuiMenu` porque DX12/Vulkan/Win32 la referencian. API clave:
  `Render()`, `GetIsOpen/SetIsOpen`, `OpenSession/CloseSession`, `SaveText`,
  `SetEscHoldProgress`, `IsWriteMode`, `ShouldDrawMouse`.
- Natives SOLO en el hilo de script (script.cpp). ImGui SOLO en `Render()` (hilo de render).
- ESC se lee en script.cpp con `GetAsyncKeyState(VK_ESCAPE)` (no depender de ImGui/WndProc).
- SOS usa `TIMERB`; ESC usa `TIMERA` (no mezclar). Tras SOS: `ResetKeyState('Y')` o se reabre al soltar.
- Font: se usa `io.Fonts->Fonts[1]` si existe (el usuario carga su .ttf cursiva en la init
  de DX12.cpp/Vulkan.cpp, junto a `AddFontDefault()`); si no, default.
- `menu.h`/`Win32.cpp`: NO hay F10 ni toggles legacy; no re-introducirlos.
- Cerrar = `CloseSession()` (oculta UI) -> `SaveText()` -> anim opcional -> `s_active=false`.

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

## Gotchas conocidos
- `WAIT()` (main.h) solo en el hilo de script; los yields fuera del bucle usan `WaitLocked()`.
- `DISABLE_ALL_CONTROL_ACTIONS` esta en `PAD::`, no en `MISC::`.
- windows.h define macros min/max: los cpp nuevos llevan `#define NOMINMAX` arriba.
- Warnings C4326 (`void main`) y C4267 del DX12 son pre-existentes del SDK, ignorar.
