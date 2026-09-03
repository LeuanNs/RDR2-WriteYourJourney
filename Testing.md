# Testing - Write Your Journey - Guia Rapida para Leuan
> Ultima Build: 2026-09-03 (Testing Fixes Batch 7 - Restored Page Overlay Fix, KeepSheet Restore, Index Fix, Mouse Handling)
> Como usar: entra al juego, ve seccion por seccion. Marca [x] si OK, deja [ ] si falla y anota al lado que viste.

---

### 0) Hojas Arrancadas (SHEETS) - Journal

#### Restored page damage overlay - FIX CRITICO BATCH 7
**Preparacion:** abre el journal (J), escribe algo en una pagina, arranca la pagina (P 3s), presiona ESC en overlay

- [ ] Rippear pagina del journal -> presionar ESC en overlay -> ¿animacion de hoja volviendo?
- [ ] Navegar a la pagina restaurada -> ¿se ven las arrugas SOBRE el texto? - FIX BATCH 7: ahora usa GetForegroundDrawList()
- [ ] Las marcas de dano (arrugas, manchas, tajos) son visibles ENCIMA del contenido de texto
- [ ] Rippear pagina de custombook -> presionar ESC -> ¿mismo visual sobre el texto?

#### KeepSheet restores page - FIX CRITICO BATCH 7
**Preparacion:** deja una sheet en el mundo (L en overlay), cierra journal, ve a la sheet

- [ ] Acercarse a la sheet -> presionar R -> personaje camina y recoge
- [ ] En overlay de discoverable -> presionar K para keepar
- [ ] Abrir journal/custombook al que pertenece la pagina -> ¿la pagina aparece restaurada?
- [ ] La pagina restaurada tiene el visual daniado (arrugas, manchas, tajos)
- [ ] El texto de la pagina es legible debajo del dano

#### Ripped page visual mejorado - BATCH 7
**Preparacion:** arranca una pagina del journal o custombook

- [ ] Pagina arrancada en journal -> ¿muestra texto "Page Ripped" centrado?
- [ ] Borde rasgado mas visible (jag size 8 vs 6 anterior)
- [ ] Mas manchas de suciedad (8 vs 5 anterior)
- [ ] Lineas de tajos diagonales visibles (3 lineas)
- [ ] Mismo visual mejorado en custombooks

#### Index I+ENTER - FIX BATCH 7
**Preparacion:** abre satchel (B 3s), selecciona libro con index

- [ ] Presionar I -> ¿NO abre index directamente? (debe mostrar hint)
- [ ] Presionar ENTER despues de I -> ¿abre index correctamente?
- [ ] ESC en index -> ¿cierra sin bugear?

#### Random page lazy loading - FIX BATCH 7
**Preparacion:** abre satchel (B 3s), selecciona libro grande (ej: Biblia)

- [ ] Presionar R en satchel -> ¿abre pagina aleatoria?
- [ ] La pagina aleatoria muestra texto correctamente (no en blanco)
- [ ] Lazy loading funciona desde la pagina aleatoria

#### Mouse handling - BATCH 7
**Preparacion:** abre journal y custombooks

- [ ] Journal en modo Read con pagina seleccionada -> ¿mouse cursor OCULTO?
- [ ] Journal en overview (seleccionando con flechas) -> ¿mouse cursor VISIBLE?
- [ ] CustomBooks inventory abierto -> ¿mouse cursor VISIBLE para search bar?
- [ ] CustomBooks libro abierto -> ¿navegacion con flechas funciona?

---

### 0.5) Hojas Arrancadas (SHEETS) - Batch 6 Tests

#### R key pickup - FIX CRITICO BATCH 6
**Preparacion:** deja una sheet en el mundo (L en overlay) o ve a una coordenada con sheet conocida

- [ ] Caminar por el mundo sin abrir nada -> acercarse a una sheet -> ¿aparece icono de tecla?
- [ ] Presionar R UNA VEZ (sin mantener) -> ¿personaje camina inmediatamente hacia la sheet? - FIX BATCH 6: tracker manual de key state
- [ ] El pickup funciona desde el primer momento (sin necesidad de abrir journal primero) - FIX BATCH 6: Sheets::Init() al inicio de main()
- [ ] Sheets de sesiones anteriores se detectan correctamente al iniciar el juego - FIX BATCH 6
- [ ] Al llegar -> ¿personaje se agacha (crouch animation)?
- [ ] Al terminar crouch -> ¿hoja se muestra + controles bloqueados?
- [ ] Moverse mientras personaje camina -> ¿se cancela limpiamente?

---
