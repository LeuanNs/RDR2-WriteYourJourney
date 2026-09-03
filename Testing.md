# Testing - Write Your Journey - Guia Rapida para Leuan
> Ultima Build: 2026-09-03 (Testing Fixes Batch 3 - Index, Glow, Random Page, ENTER Selection, Night Tint, ESC Restore Animation)
> Como usar: entra al juego, ve seccion por seccion. Marca [x] si OK, deja [ ] si falla y anota al lado que viste.

---

### 0) Hojas Arrancadas (SHEETS) - Journal

#### ESC Restore Animation - IMPLEMENTADO
- [ ] Rippear pagina -> presionar ESC en overlay -> ¿animacion de hoja volviendo al journal? - IMPLEMENTADO: animacion de 0.9s con efecto de arrugado
- [ ] Animacion muestra efecto de arrugado (lineas que aparecen/desaparecen) - IMPLEMENTADO
- [ ] Hoja vuelve al journal sin danos - verificar

#### Visual de paginas ripeadas - SEPARADO
- [ ] Paginas seleccionadas -> glow sutil como antes (2 capas) - REVERTIDO a version original
- [ ] Paginas ripped -> glow fuerte (4 capas, alpha 180-255) - IMPLEMENTADO
- [ ] Verificar que el glow fuerte marca bien la forma de la pagina faltante

---

### 0.5) Index de CustomBooks - FIX CRITICO
**Preparacion:** abre satchel (B 3s), selecciona un libro con index.json

- [ ] Abrir Index directamente con I desde inventory -> ¿ya no se buguea? - FIX: agregada llamada a OpenBook() despues de CloseIndex()
- [ ] Presionar Enter en capitulo -> ¿abre libro automaticamente en esa pagina? - FIX: ahora abre el libro despues de cerrar Index
- [ ] ESC en Index -> ¿cierra Index sin problemas?

---

### 0.6) CustomBooks Page Selection + Rip + Edit
**Preparacion:** abre satchel (B 3s), selecciona cualquier libro, ENTER para abrirlo

- [ ] Abrir custombook -> presionar ENTER -> ¿selecciona pagina izquierda? - FIX: ENTER ahora selecciona directamente
- [ ] Si pagina izquierda esta ripped -> ¿selecciona pagina derecha?
- [ ] ENTER funciona siempre que el custombook este abierto - FIX: separado input de ENTER y click mouse

---

### 0.7) Random Page - FIX PAGINAS LEJANAS
**Preparacion:** abre satchel (B 3s), selecciona libro grande (ej: Biblia)

- [ ] Presionar R en satchel -> ¿abre pagina aleatoria de TODO el libro? - FIX: calculo de totalPages usa lazyTotalLines
- [ ] Verificar que puede abrir paginas mas alla de la 100
- [ ] Lazy loading funciona correctamente desde pagina aleatoria

---

### 2) Journal - Colores
- [ ] De dia -> tinte sutil (100,90,75,100) - AGREGADO tinte diurno
- [ ] De noche (21:00-06:00) -> tinte mas claro (60,55,45,160) - AJUSTADO de (50,45,35,200)
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

---

## Cambios realizados en Batch 3

### Fix 1: Index bug - se bugueaba al abrir directamente con I
- Agregada llamada a OpenBook() despues de CloseIndex() en RenderIndex()
- Ahora al presionar Enter en un capitulo del Index, se abre el libro automaticamente
- El Index ya no se buguea al abrir directamente con I desde el inventory

### Fix 2: Glow - separado glow normal vs glow fuerte para paginas faltantes
- Revertido DrawPageGlow() a la version original (2 capas, alpha 130-190) para paginas seleccionadas
- Creada nueva funcion DrawRippedPageGlow() (4 capas, alpha 180-255) para paginas ripped
- Aplicado glow fuerte solo a paginas ripped en journal y custombooks
- El glow normal ahora es sutil como antes, el glow fuerte solo marca paginas faltantes

### Fix 3: Random page - no tomaba todas las paginas del libro
- Corregido calculo de totalPages usando book.lazyTotalLines para libros grandes
- Ahora el random page considera todas las paginas del libro, no solo las cargadas
- Fix aplicado tanto para random page (R) como para Index

### Fix 4: ENTER selection mode - a veces no respondia
- Separado input de ENTER y click mouse en RenderBook()
- ENTER ahora selecciona leftPage (o rightPage si left esta ripped) directamente
- Click mouse sigue funcionando como antes para seleccionar pagina especifica
- El ENTER ahora funciona siempre que el custombook este abierto

### Fix 5: Night tint - demasiado oscuro
- Ajustado tinte nocturno de (50,45,35,200) a (60,55,45,160) - mas claro
- Agregado tinte diurno sutil (100,90,75,100) para mejor consistencia visual
- El journal ahora tiene mejor balance entre dia y noche

### Fix 6: ESC restore - animacion de arrugada implementada
- Agregadas variables s_restoreAnimating, s_restoreAnimT, s_restoreCache
- Modificado RestorePage() para iniciar animacion en vez de cerrar inmediatamente
- Creada funcion DrawRestoreAnimation() con efecto de hoja volviendo al journal
- Animacion incluye: movimiento desde esquina superior derecha al centro, efecto de arrugado con sin(t*PI), lineas de arruga que aparecen y desaparecen
- Duracion: 0.9s (igual que rip animation)

---

*Marca y pegame el resultado, yo actualizo todo.txt y CHANGELOG*
