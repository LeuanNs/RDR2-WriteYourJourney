# Testing - Write Your Journey - Guia Rapida para Leuan
> Ultima Build: 2026-09-02 (Testing Fixes Batch 2 - R:Look Behind, Lazy Loading, Search Bar, Reload, Pickup R, Index, Strikethrough, Glow, Progress Bar, Night Tint)
> Como usar: entra al juego, ve seccion por seccion. Marca [x] si OK, deja [ ] si falla y anota al lado que viste.

---

### 0) Hojas Arrancadas (SHEETS) - Journal
**Preparacion:** abre el journal (J 3s), escribe algo en pag 2 y dibuja un trazo. Quedate en esa pagina seleccionada.

#### Pickup con R (CAMBIADO de E a R) - RE-TESTEAR
- [ ] Mantener R -> ¿barra blanca se llena alrededor del cuadrado (3s) sin bug visual? - CAMBIADO: ahora usa R en vez de E, texto actualizado a "Press R to pick it up"
- [ ] Soltar R antes de 3s -> ¿se cancela correctamente?
- [ ] Moverse mientras se mantiene R -> ¿se cancela y NO vuelve a intentar solo al lugar?
- [ ] Moverse mientras personaje camina hacia la sheet -> ¿se cancela correctamente?
- [ ] Al llegar a la sheet -> ¿personaje se agacha (crouch `amb_rest@world_handle_bottle_pickup`)?
- [ ] Al terminar crouch -> ¿hoja se muestra + controles/HUD/camara bloqueados?
- [ ] Al completar hold + K Keep -> ¿hoja se añade de vuelta al journal/book correspondiente?

#### Visual de paginas ripeadas - MEJORADO
- [ ] Paginas ripeadas -> ¿muestran glow pulsante azul MAS FUERTE como pista visual? - MEJORADO: ahora tiene 4 capas de glow con alpha mas alto (180-255) para marcar mejor la forma de la pagina faltante
- [ ] Click en pagina ripped -> ¿dice "Page Ripped" + glow pulsante?

#### Barra de progreso Rip - AJUSTADO
- [ ] Mantén P 3s -> barra "Ripping page..." ¿menos gruesa y brillante? - AJUSTADO: reducido de 320x12 a 280x8, colores mas sutiles (200,170,90 en vez de 255,200,80), texto mas pequeno (1.1x en vez de 1.3x)

#### Efecto visual ESC restore - PENDIENTE
- [ ] Pulsa ESC en overlay -> ¿hoja vuelve al journal con efecto visual de arrancada/arrugada? - NO IMPLEMENTADO AUN, se queda perfecta como se muestra

---

### 0.5) Index de CustomBooks - FIX CRITICO
**Preparacion:** abre satchel (B 3s), selecciona un libro con index.json

- [ ] Index se abre -> ¿ImGui sigue respondiendo, controles bloqueados, ESC funciona? - FIX: añadido CustomBooks::IsIndexOpen() al bloqueo de controles en script.cpp
- [ ] Abrir capitulo en pagina lejana (ej: 940) -> ¿lazy loading carga chunk correcto? - FIX: calculo de linesPerPage ahora usa la misma formula que RenderBook() en vez de hardcodear 12
- [ ] Abrir libro con bookmark (K) -> ¿abre en pagina del bookmark? - FIX: s_currentPage se setea DESPUES de OpenBook() en vez de antes (que lo reseteaba a 0)
- [ ] Abrir libro random (R) -> ¿abre en pagina aleatoria? - FIX: s_currentPage se setea DESPUES de OpenBook() y se carga el chunk correcto

---

### 0.6) CustomBooks Page Selection + Rip + Edit
**Preparacion:** abre satchel (B 3s), selecciona cualquier libro, ENTER para abrirlo

- [ ] Click en pagina izquierda/derecha -> glow azul pulsante + texto "ENTER: Selection Mode" visible - FIX: añadido texto "ENTER: Selection Mode" al help string
- [ ] Con pagina seleccionada -> P -> arranca pagina (overlay + barra progreso)
- [ ] Escribir texto + ENTER -> tachado + correccion en pequeño arriba - FIX: edit.lineIndex ahora es `s_cbSelectedPage * linesPerPage` sin el offset de lazyStartLine (que causaba que se aplicara a la pagina siguiente)
- [ ] Paginas ripeadas en custombook -> ¿borde rasgado + "Page Ripped" visible? - FIX: añadido CustomBooks::RipPage() en ConfirmRip() para sincronizar tracking entre sheets.cpp y custombooks.cpp

---

### 2) Journal - Colores
- [ ] De noche (21:00-06:00) -> tinte nocturno MAS OSCURO - FIX: cambiado de (80,70,55,180) a (50,45,35,200) para mejor oscuridad sin deslumbrar

---

### 7) Lazy Loading (Libros Grandes) - RE-TESTEAR
- [ ] Abrir Biblia NT 1858 -> navegar hacia ADELANTE sin cortes
- [ ] Llegar a pag 58+ -> texto sigue renderizando
- [ ] Navegar atras y adelante -> chunks se cargan correctamente
- [ ] Abrir libro en pagina aleatoria (R) -> ¿lazy loading funciona desde esa pagina? - FIX: ahora carga el chunk correcto basado en s_currentPage

---

### 8) Libros Encontrables - TODA LA SECCIÓN 8 NO HA SIDO TESTEADA, DEJAR INTACTA
- [ ] Al inicio ¿LaBibliaViajeroTiempo NO esta en satchel? - nop, no esta, esto esta bien
- [ ] Ve a coords X=-1842, Y=-1038, Z=180 -> ¿a 5m aparece mensaje? - no testeado aun
- [ ] Pulsa E -> ¿config.ini cambia a isOwned=1? - no testeado aun

### 9) Integracion General
- [ ] Que te golpeen -> ¿cierra journal/satchel? (no testeado)
- [ ] 5x ESC rapido -> ¿cierra todo? (reportado nop)

### 10) Localizacion
- [ ] Edita WriteYourJourney.ini cambia textos -> R+P hold 5s -> ¿cambia en juego?

---

## Notas rapidas
- Todo lo de Sheets esta en `src/ImGuiRDR2Hook/sheets.cpp/h`
- CustomBooks en `src/ImGuiRDR2Hook/custombooks.cpp/h`
- Edits de CustomBooks se guardan en `MyJourney/Books/[Nombre]/edits.txt`

---

## Cambios realizados en Batch 2

### Fix 1: Pickup con R key (antes E)
- Cambiado todas las referencias de 'E' a 'R' en script.cpp para sheet pickup
- Cambiado icono de tecla de "E" a "R" en sheets.cpp RenderPickupPrompt() y RenderEHoldPrompt()
- Cambiado HandleInput() en sheets.cpp para usar 'R' en vez de 'E'
- Cambiado texto default en config.h de "Press E to pick it up" a "Press R to pick it up"

### Fix 2: Index bug - controles desbloqueados
- Añadido CustomBooks::IsIndexOpen() al check de bloqueo de controles en script.cpp
- Añadido CustomBooks::CloseIndex() al cleanup cuando el juego pierde foco
- Ahora el Index mantiene el bloqueo de controles como el inventory y book

### Fix 3: Strikethrough bug - aplicaba a pagina siguiente
- Corregido edit.lineIndex en custombooks.cpp de `book.lazyStartLine + startLine` a solo `startLine`
- El calculo de actualLineIdx en el render ya compensa por lazyStartLine, no hay que duplicar el offset

### Fix 4: Lazy loading - paginas lejanas en blanco
- Fix random page (R): s_currentPage ahora se setea DESPUES de OpenBook() (que lo reseteaba a 0)
- Fix bookmark (K): s_currentPage se setea DESPUES de OpenBook() y se carga el chunk correcto
- Fix Index: calculo de linesPerPage ahora usa la misma formula que RenderBook() (pageTextH / lineH) en vez de hardcodear 12
- Añadida carga de chunk despues de setear s_currentPage en random y bookmark

### Fix 5: Texto "ENTER: Selection Mode" faltante
- Añadido "ENTER: Selection Mode" al help string en custombooks.cpp RenderBook()
- Se muestra tanto cuando hay pagina seleccionada como cuando no

### Fix 6: Ripped pages visual en custombook - nada visual
- Añadido CustomBooks::RipPage() en sheets.cpp ConfirmRip() para sincronizar tracking
- Añadido CustomBooks::RestorePage() en sheets.cpp RestorePage() para sincronizar tracking
- Ahora s_rippedCustomBookPages en custombooks.cpp se actualiza correctamente

### Fix 7: Ripped page glow - muy tenue
- Mejorado DrawPageGlow() en custombooks.cpp y menu.cpp
- Añadidas 4 capas de glow en vez de 2
- Aumentado alpha de 130-190 a 180-255 para glow mas visible
- Añadidos rectangulos rellenos para efecto de glow mas solido

### Fix 8: Rip progress bar - muy gruesa y brillante
- Reducido tamaño de 320x12 a 280x8
- Reducido texto de 1.3x a 1.1x font size
- Cambiado colores de (255,200,80) a (200,170,90) para tonos mas sutiles
- Reducido border thickness de 2.5f a 1.5f
- Reducido padding y roundness

### Fix 9: Night tint - muy claro
- Cambiado de (80,70,55,180) a (50,45,35,200)
- RGB mas bajo para oscuridad aumentada
- Alpha ligeramente mayor para mejor cobertura

---

*Marca y pegame el resultado, yo actualizo todo.txt y CHANGELOG*
