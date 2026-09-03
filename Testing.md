# Testing - Write Your Journey - Guia Rapida para Leuan
> Ultima Build: 2026-09-03 (Testing Fixes Batch 5 - Instant R Pickup, Enhanced Restored Page, [RipSheets] Config)
> Como usar: entra al juego, ve seccion por seccion. Marca [x] si OK, deja [ ] si falla y anota al lado que viste.

---

### 0) Hojas Arrancadas (SHEETS) - Journal

#### Pickup instantaneo con R - FIX CRITICO BATCH 5
**Preparacion:** deja una sheet en el mundo (L en overlay) o ve a una coordenada con sheet conocida

- [ ] Caminar por el mundo sin abrir nada -> acercarse a una sheet -> ¿aparece icono de tecla?
- [ ] Presionar R UNA VEZ (sin mantener) -> ¿personaje camina inmediatamente hacia la sheet? - FIX: ya no requiere mantener 3s
- [ ] Al llegar -> ¿personaje se agacha (crouch animation)?
- [ ] Al terminar crouch -> ¿hoja se muestra + controles bloqueados?
- [ ] Moverse mientras personaje camina -> ¿se cancela limpiamente?
- [ ] Si la animacion de crouch falla -> ¿no se rompe todo? - FIX: try-catch en walk y crouch

#### Pagina restaurada - Visual mejorado - FIX CRITICO BATCH 5
**Preparacion:** abre el journal (J 3s), escribe algo en pag 2, arranca la pagina (P 3s), presiona ESC en overlay

- [ ] Rippear pagina del journal -> presionar ESC en overlay -> ¿animacion de hoja volviendo?
- [ ] Pagina restaurada -> ¿visual CLARAMENTE daniada? - FIX: mejorado significativamente
  - [ ] Color base mas oscuro (175,165,140) vs paginas normales (210,200,175)
  - [ ] Lineas de arrugas visibles (14 lineas con opacidad 120)
  - [ ] Manchas de suciedad visibles (8 manchas con radio 12)
  - [ ] Bordes rasgados grandes y densos (tearSize * 3)
  - [ ] Tajos laterales visibles (6 tajos de grosor 2.0)
  - [ ] Manchas circulares adicionales (4 manchas)
- [ ] Rippear pagina de custombook -> presionar ESC -> ¿mismo visual mejorado?

#### [RipSheets] Configuracion INI - NUEVO BATCH 5
**Preparacion:** editar WriteYourJourney.ini, agregar seccion [RipSheets]

- [ ] Agregar `[RipSheets]` con `enableRipSheetSystem=0` -> recargar (R+P 5s)
- [ ] Con sistema desactivado -> ¿no aparece prompt de pickup al acercarse a sheet?
- [ ] Con sistema desactivado -> ¿P no arranca paginas en journal?
- [ ] Con sistema desactivado -> ¿"P: Rip Page" no aparece en ayuda del journal?
- [ ] Con sistema desactivado -> ¿P no arranca paginas en custombooks?
- [ ] Cambiar a `enableRipSheetSystem=1` -> recargar -> ¿sistema funciona normalmente?
- [ ] Cambiar `ripSheetPickupKey=F` -> recargar -> ¿icono muestra "F" en vez de "R"?
- [ ] Con tecla F -> presionar F cerca de sheet -> ¿recoge la sheet?
- [ ] Con tecla F -> presionar R cerca de sheet -> ¿NO pasa nada?

---

### 0.5) Index de CustomBooks - FIX CRITICO
**Preparacion:** abre satchel (B 3s), selecciona un libro con index.json

- [ ] Abrir Index directamente con I desde inventory -> ¿ya no se buguea? - FIX: agregada llamada a OpenBook() despues de CloseIndex()
- [ ] Presionar Enter en capitulo -> ¿abre libro automaticamente en esa pagina?
- [ ] ESC en Index -> ¿cierra Index sin problemas?

---

### 0.6) CustomBooks Page Selection + Rip + Edit
**Preparacion:** abre satchel (B 3s), selecciona cualquier libro, ENTER para abrirlo

- [ ] Abrir custombook -> presionar ENTER -> ¿selecciona pagina izquierda?
- [ ] Si pagina izquierda esta ripped -> ¿selecciona pagina derecha?
- [ ] ENTER funciona siempre que el custombook este abierto

---

### 0.7) Random Page - FIX PAGINAS LEJANAS
**Preparacion:** abre satchel (B 3s), selecciona libro grande (ej: Biblia)

- [ ] Presionar R en satchel -> ¿abre pagina aleatoria de TODO el libro?
- [ ] Verificar que puede abrir paginas mas alla de la 100
- [ ] Lazy loading funciona correctamente desde pagina aleatoria

---

### 2) Journal - Colores
- [ ] De dia -> tinte sutil (100,90,75,100)
- [ ] De noche (21:00-06:00) -> tinte mas claro (60,55,45,160)
- [ ] Verificar que no deslumbra de noche ni es demasiado oscuro

---

### 8) Libros Encontrables - TODA LA SECCION 8 NO HA SIDO TESTEADA, DEJAR INTACTA
- [x] Al inicio ¿LaBibliaViajeroTiempo NO esta en satchel? - nop, no esta, esto esta bien
- [ ] Ve a coords X=-1842, Y=-1038, Z=180 -> ¿a 5m aparece mensaje? - no testeado aun
- [ ] Pulsa E -> ¿config.ini cambia a isOwned=1? - no testeado aun

### 9) Integracion General
- [ ] Que te golpeen -> ¿cierra journal/satchel? (no testeado)
- [ ] 5x ESC rapido -> ¿cierra todo? (reportado nop)

### 10) Localizacion
- [ ] Edita WriteYourJourney.ini cambia textos -> R+P hold 5s -> ¿cambia en juego? - no, no funciona

---

## Notas rapidas
- Todo lo de Sheets esta en `src/ImGuiRDR2Hook/sheets.cpp/h`
- CustomBooks en `src/ImGuiRDR2Hook/custombooks.cpp/h`
- Edits de CustomBooks se guardan en `MyJourney/Books/[Nombre]/edits.txt`
- Config de RipSheets en `[RipSheets]` section del INI

---

## Cambios realizados en Batch 5

### Fix 1: Pickup instantaneo con R
- Cambiado de "mantener R 3s" a "presionar R una vez"
- Al presionar R, personaje camina inmediatamente, se agacha y muestra hoja
- Try-catch en walk y crouch para que si falla la anim no se rompa todo
- La barra de progreso ya no aparece (ahora es instantaneo)

### Fix 2: Pagina restaurada visual mejorado
- Color base mas oscuro: (175,165,140) vs (195,185,160) anterior
- 14 lineas de arrugas (vs 8), opacidad 120 (vs 80), grosor 1.5 (vs 1.0)
- 8 manchas de suciedad (vs 5), radio 12 (vs 8), opacidad 90 (vs 60)
- 5 lineas de pliegues cruzados para efecto de hoja arrugada
- Bordes rasgados 3x mas grandes y mas densos
- 6 tajos laterales (vs 3), mas largos y gruesos
- 4 manchas circulares adicionales
- La pagina restaurada ahora se ve CLARAMENTE daniada

### Fix 3: [RipSheets] Config INI
- Nueva seccion [RipSheets] en WriteYourJourney.ini
- enableRipSheetSystem=1/0 controla todo el sistema
- ripSheetPickupKey=R (A-Z) configurable
- Si disabled: no init, no prompt, no rip, no render, no help text
- Icono de tecla se actualiza automaticamente al cambiar key

---

*Marca y pegame el resultado, yo actualizo todo.txt y CHANGELOG*
