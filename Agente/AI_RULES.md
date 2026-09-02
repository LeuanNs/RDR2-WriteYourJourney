# Reglamento de IA para "Write Your Journey"

## Principio Fundamental
**NO TOCAR SISTEMAS FUNCIONALES.** Si algo funciona, no se modifica. Los nuevos sistemas se implementan de forma aislada y asíncrona a los existentes.

---

## Regla 1: Aislamiento de Sistemas

Cada sistema nuevo debe ser completamente independiente. Ejemplo de referencia: `CustomBooks`.

**Cómo se hace correctamente (CustomBooks)**:
- Archivos propios: `custombooks.cpp`, `custombooks.h`
- No modifica la lógica interna del journal
- Se integra mínimamente en `Render()` con checks de estado
- Tiene su propio input handling (`HandleInput()`)
- Tiene su propio render (`RenderInventory()`, `RenderBook()`)
- El journal no sabe que existe CustomBooks

**Cómo NO hacerlo**:
- NO modificar funciones existentes del journal para añadir lógica nueva
- NO mezclar variables de estado entre sistemas
- NO cambiar rutas, fórmulas de render, o lógica de guardado del journal
- NO tocar `DrawReadPage()`, `DrawWritePage()`, `DrawDrawingCanvas()`, etc.

---

## Regla 2: Integración Mínima

Cuando un sistema nuevo necesita coexistir con uno existente:

1. **Input**: Cada sistema maneja su propio input. Si hay conflicto de teclas, el sistema activo tiene prioridad.
2. **Render**: Usar checks de estado (`if (!GetIsOpen())`) para decidir qué renderizar.
3. **Controles**: El bloqueo de controles del jugador se hace en `script.cpp`, no en el render.
4. **Estado**: Cada sistema tiene sus propias variables estáticas. No compartir estado.

Ejemplo correcto (script.cpp):
```cpp
// Bloqueo para satchel (independiente del journal)
if (CustomBooks::IsInventoryOpen() || CustomBooks::IsBookOpen())
{
    LockControlsThisFrame();
    WAIT(0);
    continue;
}
```

---

## Regla 3: No Modificar Rutas Ni Guardado

El sistema de guardado del journal (`myjourney/Myself/C1/pagX.txt`) está probado y funciona. NO cambiar:
- `SaveDirPath()`
- `PageFilePath()`
- `DrawingFilePath()`
- `SavePageToFile()`
- `LoadPageFromFile()`

Si un sistema nuevo necesita guardar datos, crea sus propias funciones en sus propios archivos.

---

## Regla 4: Commits Ordenados

Cada cambio debe subirse a GitHub con un commit descriptivo.

**Formato del commit**:
```
[Tema]: Descripción corta del cambio

- Detalle 1
- Detalle 2
- Detalle 3
```

**Ejemplos**:
```
Fix satchel inventory: mini-journal covers, keyboard navigation, ESC close

- Replace grid with carousel showing book cover
- Add ENTER to open book, arrows to browse
- Fix ESC to close inventory/book properly
```

```
Add word wrap to custom books rendering

- Implement WrapText() function
- Fix line overlap in book pages
- Use proper pagination based on wrapped lines
```

**Frecuencia**: Commit después de cada feature completada o fix verificado. No acumular cambios.

---

## Regla 5: Sistemas de Referencia

Cuando implementes un sistema nuevo, usa estos como referencia de cómo hacerlo bien:

### CustomBooks (Sistema de Libros)
- **Archivos**: `custombooks.cpp`, `custombooks.h`
- **Integración**: 
  - `menu.cpp`: Solo `#include "custombooks.h"` y llamadas en `Render()` con checks
  - `script.cpp`: Bloqueo de controles independiente
  - `Win32.cpp`: Forwarding de input cuando está abierto
- **Aislamiento**: El journal no se modifica para soportar books

### Journal (Sistema Principal)
- **NO TOCAR**: `DrawReadPage()`, `DrawWritePage()`, `DrawDrawingCanvas()`, `HandleDrawingInput()`
- **Referencia**: Usar como ejemplo de cómo hacer render, guardado, y manejo de input
- **Rutas**: `myjourney/Myself/C1/pagX.txt` y `pagX_draw.dat`

### Eraser (Sistema de Borrado)
- **Ubicación**: Dentro de `HandleDrawingInput()` en `menu.cpp`
- **Variables**: `s_eraserRadius`, `ERASER_RADIUS_MIN/MAX/STEP`
- **Referencia**: Ejemplo de cómo añadir funcionalidad a un sistema existente sin romperlo

---

## Regla 6: Testing Antes de Commit

Antes de hacer commit:
1. Compilar sin errores
2. Probar el sistema nuevo aislado
3. Probar que el journal sigue funcionando
4. Probar que no hay conflictos de input
5. Verificar que no hay crashes al cambiar entre sistemas

---

## Regla 7: Documentación de Cambios

Si un cambio afecta la integración entre sistemas, documentarlo en el commit:
```
Fix journal crash when opening with J

- Move CustomBooks::HandleInput() to only run when journal closed
- Add state checks before rendering inventory/book
- Ensure journal transition Cover->Open is not interrupted
```

---

## Resumen para IA

Cuando recibas una tarea:
1. **Identifica** qué sistema existente se ve afectado
2. **Aísla** el nuevo código en archivos propios si es posible
3. **Integra** mínimamente con checks de estado
4. **No modifiques** lógica funcional existente
5. **Testea** que todo sigue funcionando
6. **Commitea** con descripción clara
7. **Documenta** si hay cambios de integración

**Recuerda**: El journal funciona. CustomBooks funciona. El eraser funciona. No los rompas.
