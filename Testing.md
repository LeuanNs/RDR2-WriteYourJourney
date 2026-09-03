# Testing - Write Your Journey - Guia Rapida para Leuan
> Ultima Build: 2026-09-02 (Fixes Masivos Sheets + CustomBooks Page Selection/Rip/Edit)
> Como usar: entra al juego, ve seccion por seccion. Marca [x] si OK, deja [ ] si falla y anota al lado que viste.

## Como reportarme
Cuando termines una seccion, dime por ejemplo:
`Seccion 0: OK todo menos L no crea SHEET1 (no aparece carpeta)`
O `Seccion 4: Bookmark Saved no aparece`
Yo lo cruzo con el changelog y actualizo el todo.

---

### 0) Hojas Arrancadas (SHEETS) - Journal
**Preparacion:** abre el journal (J 3s), escribe algo en pag 2 y dibuja un trazo. Quedate en esa pagina seleccionada.

- [ ] ¿Ves abajo "P: Rip Page" junto a ESC/K/R?
- [ ] Mantén P 3s -> ¿barra "Ripping page..." se llena? (320px ancho x 12px alto, dorado)
- [ ] Suelta P a los 1-2s -> ¿barra se cancela y NO pasa nada?
- [ ] Mantén P 3s completo -> ¿hoja se desliza/rota y hace fade saliendo del libro? (0.9s)
- [ ] ¿Overlay centrado aparece con la misma hoja, bordes rasgados, texto + dibujo?
- [ ] En overlay ¿ves "L: Leave Page here | R: Look Behind | ESC: Add page back"?
- [ ] Pulsa ESC en overlay -> ¿hoja vuelve al journal intacta? (rip cancelado)
- [ ] Pulsa L -> ¿overlay se cierra, hoja queda en el mundo?
- [ ] Alt+Tab, mira `myjourney/Discoverables/SHEET1/` -> ¿existe `location.ini`, `sheet.txt`, `sheet_draw.dat`?
- [ ] Cierra journal, vuelve a abrir -> ¿la pagina arrancada se ve con hueco y borde rasgado?
- [ ] Paginas ripeadas -> ¿muestran glow pulsante azul como pista visual?
- [ ] Click en pagina ripped -> ¿dice "Page Ripped" (configurable en INI)?
- [ ] No se puede seleccionar pagina ripped directamente (solo navegar con flechas)
- [ ] Rippear pag 2 -> pag 3 tambien desaparece (misma hoja fisica)
- [ ] Rippear pag 6 -> pag 7 tambien desaparece (trasera)
- [ ] Al restaurar con ESC -> paginas partner tambien se restauran
- [ ] Al mostrar overlay -> texto de otras paginas NO se solapa

#### Pickup con E (NUEVO)
- [ ] Acercarse a sheet a 5m -> ¿aparece cuadrado con "E" (tecla keyboard)?
- [ ] Mantener E -> ¿barra blanca se llena alrededor del cuadrado (3s)?
- [ ] Soltar E antes de 3s -> ¿se cancela?
- [ ] Moverse mientras se mantiene E -> ¿se cancela?
- [ ] Completar barra E -> ¿personaje camina hacia la sheet?
- [ ] Moverse mientras camina -> ¿se cancela?
- [ ] Al llegar -> ¿personaje se agacha (crouch)?
- [ ] Al terminar crouch -> ¿hoja se muestra + controles/HUD/camara bloqueados?
- [ ] Al completar E hold -> ¿texto "hay una hoja cerca" desaparece?

#### Overlay mejorado (NUEVO)
- [ ] En overlay -> "R: Look Behind" visible si hay contenido atras
- [ ] Presionar R -> ¿animacion 3D de giro?
- [ ] Al terminar giro -> ¿muestra contenido de pagina trasera?
- [ ] Presionar R de nuevo -> ¿gira de vuelta al frente?
- [ ] En overlay de discoverable -> "K: Keep the Sheet" visible
- [ ] Presionar K -> ¿overlay desaparece, control vuelve al jugador?
- [ ] Despues de K -> sheet NO aparece al volver a pasar
- [ ] Si pagina vacia -> NO se muestra "(empty page)"

---

### 0.5) Index de CustomBooks
**Preparacion:** abre satchel (B 3s), selecciona un libro con index.json

- [ ] En satchel, ¿ves "I: Index"?
- [ ] Pulsa I -> ¿abre Index INMEDIATAMENTE (sin ENTER)?
- [ ] Index se ve como pagina del mismo libro (cover, pergamino, spine)
- [ ] Flechas arriba/abajo -> navega entre capitulos
- [ ] Enter en capitulo -> cierra Index, abre libro en esa pagina
- [ ] Abrir capitulo en pagina lejana (ej: 940) -> lazy loading funciona

---

### 0.6) NUEVO - CustomBooks Page Selection + Rip + Edit
**Preparacion:** abre satchel (B 3s), selecciona cualquier libro, ENTER para abrirlo

- [ ] Sin pagina seleccionada -> flechas pasan hojas normalmente (pares)
- [ ] Click en pagina izquierda o derecha -> ¿glow azul pulsante aparece?
- [ ] Con pagina seleccionada -> flechas izq/der mueven entre paginas INDIVIDUALES
- [ ] Con pagina seleccionada -> P -> ¿arranca la pagina? (overlay con contenido)
- [ ] Con pagina seleccionada -> E -> ¿modo edicion? (cuadro texto arriba)
- [ ] Escribir texto + ENTER -> ¿tachado + correccion en pequeno arriba?
- [ ] Verificar `edits.txt` en carpeta del libro
- [ ] Reabrir libro -> ediciones siguen visibles
- [ ] Paginas ripeadas en custombook -> borde rasgado + "Page Ripped"
- [ ] ESC -> deselecciona pagina, vuelve a navegacion normal

---

### 1) Journal - Bookmarks
- [ ] En portada: ¿dice "K: Open at Bookmark" SIN "(Page XX)"?
- [ ] Con bookmark guardado, pulsa K en portada -> ¿abre DIRECTO en esa pagina?
- [ ] Pulsa R en portada -> ¿abre pagina aleatoria directo?
- [ ] En una pagina, pulsa K -> ¿liston rojo + "Bookmark Saved" 2s?
- [ ] Pulsa K otra vez -> ¿liston se contrae + "Bookmark Removed" 2s? (6px mas abajo que antes)
- [ ] Cierra y reabre con bookmark -> ¿liston queda expandido?

### 2) Journal - Colores
- [ ] Cover ¿cuero oscuro (no marron claro)?
- [ ] Paginas ¿crema apagado (210,200,175)?
- [ ] De noche ¿tinte nocturno no deslumbra?

### 3) Journal - Tiempo Cierre
- [ ] Mantén ESC -> ¿cierra en ~3s?

### 4) CustomBooks - Busqueda (Satchel B 3s)
- [ ] Mantén B 3s -> ¿abre satchel?
- [ ] Barra busqueda NO enfocada al abrir
- [ ] Click dentro barra -> se enfoca
- [ ] Click fuera -> se desenfoca

### 5) CustomBooks - Bookmarks
- [ ] En satchel, pulsa K -> ¿abre libro DIRECTO en bookmark o principio?
- [ ] Animacion liston roja funciona

### 6) CustomBooks - Colores
- [ ] Covers ¿mas oscuros (55%)?

### 7) Lazy Loading (Libros Grandes)
- [ ] Abre LaBibliaNT1858 -> ¿sin bajon a 1fps?
- [ ] Navegar hacia ADELANTE -> ¿funciona? (antes solo retroceder)
- [ ] Llegar a pag 58+ -> ¿texto sigue renderizando?
- [ ] Navegar atras y adelante -> ¿chunks se cargan?

### 8) Libros Encontrables
- [ ] Al inicio ¿LaBibliaViajeroTiempo NO esta en satchel?
- [ ] Ve a coords -> ¿a 5m aparece mensaje?
- [ ] Pulsa E -> ¿config.ini cambia a isOwned=1?

### 9) Integracion General
- [ ] Journal abierto -> pulsa B 3s -> ¿NO abre satchel?
- [ ] Satchel abierto -> pulsa J -> ¿NO abre journal?
- [ ] Alt+Tab -> ¿cierra todo instant?
- [ ] Que te golpeen -> ¿cierra?
- [ ] 5x ESC rapido -> ¿cierra todo?

### 10) Localizacion
- [ ] Edita WriteYourJourney.ini cambia textos -> F5 -> ¿cambia en juego?

---

## Notas rapidas
- Todo lo de Sheets esta en `src/ImGuiRDR2Hook/sheets.cpp/h`
- CustomBooks en `src/ImGuiRDR2Hook/custombooks.cpp/h`
- Edits de CustomBooks se guardan en `MyJourney/Books/[Nombre]/edits.txt`

---
*Marca y pegame el resultado, yo actualizo todo.txt y CHANGELOG*
