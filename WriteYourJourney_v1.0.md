# Write Your Journey - v1.0

## Resumen del Mod

**Write Your Journey** es un mod para Red Dead Redemption 2 que permite al jugador escribir un diario personal in-game, con la estética de un cuaderno de cuero de 1899. Todo el arte está dibujado con primitivas de ImGui (ImDrawList) - sin texturas externas.

El mod se integra con el juego mediante hooks de Vulkan/DX12 y ScriptHookRDR2, permitiendo al jugador abrir un diario, escribir en sus páginas, dibujar, navegar entre capítulos, y guardar todo en archivos de texto.

---

## Estructura de Archivos

### Archivos Principales

| Archivo | Descripción |
|---------|-------------|
| `main.cpp` | Entry point del DLL. Lee configuración (system.xml o config.txt), detecta API gráfica (Vulkan/DX12), inicializa hooks y registra el script. |
| `script.cpp` | Hilo de ScriptHook. Maneja input de teclas (Y para abrir, ESC para cerrar, SOS), bloqueo de controles del juego, animaciones del diario, y el loop principal. |
| `menu.cpp` | UI completa del diario. Máquina de estados (Cover/Open), dibujo del libro con ImDrawList, modos Read/Write/Draw, zoom, navegación de páginas, guardado/carga. |
| `menu.h` | Declaración de `CImGuiMenu` con API pública: `Render()`, `OpenSession()`, `CloseSession()`, `SaveText()`, getters/setters para estado. |
| `config.h` | Sistema de configuración INI. Carga teclas, nombres, localización, y valida la "bendición" del mod. |
| `keyboard.h` | Funciones de input: `IsKeyJustUp()`, `IsKeyDown()`, `ResetKeyState()` usando VK codes. |

### Hooks de Renderizado

| Archivo | Descripción |
|---------|-------------|
| `Hook/Manager.h` | Clase `CImGuiHookManager` que orquesta hooks. Contiene `GetGameWindow()` con detección de ventana invalidada. |
| `Hook/Vulkan.cpp` | Hook de Vulkan (kiero+MinHook). Intercepta `vkQueuePresentKHR`, `vkCreateSwapchainKHR`, etc. Contiene el fix de reset completo de ImGui al cambiar ventana/resolución. |
| `Hook/DX12.cpp` | Hook de DirectX 12 (kiero). Intercepta `Present()` y `ResizeBuffers()`. |
| `Hook/Win32.cpp` | Subclass del WndProc para forwarding de input a ImGui cuando el diario está abierto. |

### Soporte

| Archivo | Descripción |
|---------|-------------|
| `imgui/` | ImGui 1.89.5 + backends (Win32, DX12, Vulkan). |
| `kiero/` | Librería para hooking de APIs gráficas. |
| `minhook/` | Librería MinHook para hooking de funciones. |

---

## Features Actuales (v1.0)

### Diario
- **Y** (configurable): Abrir diario
- **ENTER**: Pasar de portada a libro abierto
- **W**: Modo escritura (InputTextMultiline invisible sobre la página)
- **R**: Modo lectura
- **D**: Modo dibujo (canvas de trazos)
- **ESC 5s**: Guardar y cerrar
- **Y 10s (SOS)**: Cierre forzoso (failsafe)

### Navegación
- **Flechas**: Enfocar página izquierda/derecha (2da vez: pasar hoja)
- **ENTER**: Seleccionar página
- **R**: Modo zoom (scroll vertical para leer)
- **F**: Toggle fuente caligráfica/legible (en zoom)
- **ESC**: Deseleccionar página

### Dibujo
- **D**: Entrar/salir de modo dibujo
- **E**: Goma de borrar
- **Z/X**: Ajustar tamaño de goma
- **SHIFT**: Panel de herramientas (pincel, grafito, crayón)
- Trazos se guardan normalizados (0..1) para ser resolution-independent

### Formato de Texto (SHIFT en escritura)
- Tamaño: Normal / Título
- Estilos: Negrita, Cursiva, Subrayado, Tachado
- Alineación: Izquierda, Centro, Derecha

### Modo "Apreciar la Vista"
- **V**: Congela controles pero permite mirar alrededor con la cámara
- **V/ESC**: Salir del modo

### Sistema de Archivos
- Guardado por página: `myjourney/C<capitulo>/pag<num>.txt`
- Guardado de dibujos: `myjourney/C<capitulo>/pag<num>_draw.dat` (formato binario con magic number)
- Auto-guardado inmediato tras cada carácter
- Cache en memoria para páginas cargadas

### Configuración (WriteYourJourney.ini)
```ini
[Settings]
OpenJournalKey=J          ; Tecla para abrir (una letra)
InstantOpen=0             ; 0=mantener 3s, 1=inmediato
EnableDeveloperLog=0      ; Log a WriteYourJourney-ByLeuan.log
MyName=                   ; Nombre para el título del diario
ReloadButton=VK_F5        ; Tecla para recargar config

[Localization]
Help_Cover=...
Help_Overview=...
; etc.
```

### Iluminación Dinámica
- Tinte nocturno sobre el pergamino (21:00 - 06:00) según `CLOCK::GET_CLOCK_HOURS()`
- No deslumbra de noche

### Seguridad
- Validación de "bendición" en el INI (anti-tampering básico)
- Failsafe contra Alt+Tab (cierra el diario inmediatamente)
- Cierre por daño recibido (TODO #7 - parcial)

---

## Fix de Resolución/Window Mode (v1.0)

**Problema**: Al hacer alt+tab, cambiar resolución, o alternar fullscreen/windowed/borderless, el ImGui dejaba de dibujarse permanentemente.

**Causa**: RDR2 recrea su HWND al cambiar modo de pantalla. ImGui guardaba el HWND viejo en `bd->hWnd`, y `GetClientRect(HWND_destruido)` devolvía `{0,0,0,0}`, congelando `io.DisplaySize` en (0,0).

**Solución**: En `Vulkan.cpp`, detectar cambio de HWND cada frame y hacer reset completo de ImGui:
1. Esperar GPU (fences)
2. Destruir contexto + backends (Vulkan + Win32)
3. Limpiar render targets
4. Recrear todo desde cero con el HWND nuevo
5. Re-subclass del WndProc

---

## TODO / Features Futuras

### Prioridad Alta
- [ ] **F5 Reload Button**: Implementar recarga completa de config sin cerrar el juego. Actualmente solo recarga el INI pero no reinicializa fuentes ni otros recursos.
- [ ] **Capítulo Dinámico**: Detectar capítulo actual de la campaña (TODO #12) para guardar en `myjourney/C<cap>/` correcto.
- [ ] **Animaciones de Diario**: Usar animaciones descubiertas en RDR3 (TODO #7).

### Prioridad Media
- [ ] **Formato de Texto Avanzado**: Aplicar negrita/cursiva/subrayado/tachado al texto (TODO #9 - UI lista, render pendiente).
- [ ] **Más Pinceles**: Implementar acuarela, pluma, etc. (TODO #6).
- [ ] **Exportar Diario**: Generar PDF/HTML con todas las páginas y dibujos.
- [ ] **Multi-idioma**: Sistema de localización completo.

### Prioridad Baja
- [ ] **Sincronización Cloud**: Subir/bajar diarios a Steam Cloud o similar.
- [ ] **Modo Foto**: Capturar página actual como imagen PNG.
- [ ] **Sonidos**: Páginas pasando, pluma escribiendo.

---

## Cómo Añadir Features

### Nueva Tecla de Atajo
1. Añadir variable en `config.h` (ej: `inline int MiTecla = VK_F6;`)
2. Parsear en `WJConfig::Load()` con `GetPrivateProfileStringA()`
3. Usar en `script.cpp` con `SafeGetAsyncKeyState(WJConfig::MiTecla)`

### Nuevo Modo de Página
1. Añadir enum en `menu.cpp` (ej: `enum class ePageMode { Read, Write, Draw, MiModo };`)
2. Crear función `DrawMiModo(const BookGeom& g, int page, float A)`
3. Llamar desde `Render()` según `s_mode`
4. Añadir ayuda en `config.h` (`FB_Help_MiModo`)

### Nuevo Recurso Gráfico
1. **NO usar texturas** (prohibido por AGENT_GUIDE.md)
2. Usar `ImDrawList`: `AddRectFilled`, `AddLine`, `AddCircleFilled`, `AddQuadFilled`, gradientes con `AddRectFilledMultiColor`
3. Para "texturas" simuladas: múltiples líneas/puntos con opacidad variada

### Nueva Nativa de RDR2
1. Declarar en `inc/natives.h` con el namespace correcto (`PLAYER::`, `TASK::`, etc.)
2. **SOLO llamar desde `script.cpp`** (hilo de ScriptHook)
3. Si necesitas el valor en `menu.cpp`, usar `std::atomic<>` o setter/getter thread-safe

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
- ScriptHookRDR2 SDK (librería `ScriptHookRDR2.lib`)

**Output**: `WriteYourJourney.asi` -> copiar a carpeta del juego

---

## Créditos

- **Base**: [ImGuiRDR2Hook](https://github.com/Halen84/ImGuiRDR2Hook) por Halen84
- **DX12**: [Sh0ckFR/Universal-Dear-ImGui-Hook](https://github.com/Sh0ckFR/Universal-Dear-ImGui-Hook)
- **Vulkan**: [bruhmoment21/UniversalHookX](https://github.com/bruhmoment21/UniversalHookX)
- **Mod**: Write Your Journey por Leuan

---

*Made with love By Leuan... May god bless you all*
