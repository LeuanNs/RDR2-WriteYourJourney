# Testing - Write Your Journey - Guia Rapida para Leuan
> Ultima Build: 2026-09-02 (Sheets + Index CustomBooks + Barra Mejorada)
> Como usar: entra al juego, ve seccion por seccion. Marca [x] si OK, deja [ ] si falla y anota al lado que viste.

## Como reportarme
Cuando termines una seccion, dime por ejemplo:
`Seccion 0: OK todo menos L no crea SHEET1 (no aparece carpeta)`
O `Seccion 4: Bookmark Saved no aparece`
Yo lo cruzo con el changelog y actualizo el todo.

---

### 0) NUEVO - Hojas Arrancadas (SHEETS) - LO MAS IMPORTANTE
**Preparacion:** abre el journal (J 3s), escribe algo en pag 2 y dibuja un trazo. Quedate en esa pagina seleccionada.

- [ ] ¿Ves abajo "P: Rip Page" junto a ESC/K/R? (NO debe ser cuadro, solo texto)
- [ ] Mantén P 3s -> ¿barra "Ripping page..." se llena? **AHORA MAS VISIBLE:** 320px ancho x 12px alto, texto grande dorado, fondo oscuro con borde redondeado, barra con gradiente dorado brillante
- [ ] Suelta P a los 1-2s -> ¿barra se cancela y NO pasa nada?
- [ ] Mantén P 3s completo -> ¿hoja se desliza/rota y hace fade saliendo del libro? (0.9s)
- [ ] ¿Overlay centrado aparece con la misma hoja, bordes rasgados/irregulares, color pergamino, texto + dibujo igual?
- [ ] En overlay ¿ves abajo "L: Leave Page here | R: Read | ESC: Add page back"?
- [ ] Pulsa ESC en overlay -> ¿hoja vuelve al journal intacta? (rip cancelado)
- [ ] Repite rip P 3s, ahora pulsa L -> ¿overlay se cierra sin error?
- [ ] Alt+Tab fuera del juego, mira `myjourney/Discoverables/SHEET1/` -> ¿existe `location.ini` con X/Y/Z, `sheet.txt` con tu texto, `sheet_draw.dat`?
- [ ] Cierra journal (ESC 3s), vuelve a abrir -> ¿la pagina que arrancaste ahora se ve con hueco y borde rasgado/pedacitos en el lomo? No debe verse crema completa.
- [ ] Selecciona esa pagina ripped -> ¿dice "(Page ripped)" centrado?
- [ ] Quedate parado donde diste L, ¿ves prompt "Hay una hoja arrancada cerca" + "Press E to pick it up" a 10m?
- [ ] Pulsa E -> ¿overlay muestra la hoja encontrada?
- [ ] ESC en ese overlay de discoverable -> ¿cierra sin crashear?
- [ ] **Easter egg manual:** crea `myjourney/Discoverables/SHEET99/` con `sheet.txt` (hola mundo) y `location.ini` ([Location] X=-1842 Y=-1038 Z=180 PickupRadius=10), ve a esas coords -> ¿aparece prompt y puedes recogerla?
- [ ] Prueba con Alt+Tab y con daño (que te golpee un NPC) con overlay abierto -> ¿cierra todo sin crash?
- [ ] Prueba que P/E/L/R no rompen CustomBooks ni Journal (ver secc 10)

**Si algo falla anota:** ¿barra no aparece? ¿animacion trabada? ¿archivos no creados? ¿overlay vacio?

---

### 0.5) NUEVO - Index de CustomBooks
**Preparacion:** abre satchel (B 3s), selecciona un libro que tenga index.json con capitulos (ej: LaBibliaNT1858)

- [ ] En satchel, con libro seleccionado, ¿ves "I: Index" junto a "K: Bookmark | R: Random Page"?
- [ ] Pulsa I -> ¿abre pantalla completa con fondo oscuro y titulo "Index" centrado arriba?
- [ ] ¿Lista de capitulos aparece centrada verticalmente?
- [ ] Flecha arriba/abajo -> ¿navegas entre capitulos? Capitulo seleccionado se ve dorado (255,215,0) y mas grande
- [ ] Enter en un capitulo -> ¿cierra Index y abre libro en esa pagina?
- [ ] ESC en Index -> ¿cierra sin abrir libro?
- [ ] **Test de capitulo arrancado:** arranca una pagina de CustomBook (P 3s + L), vuelve a satchel, abre Index -> ¿el capitulo correspondiente ahora dice "Ripped Sheet (TituloOriginal)"?
- [ ] Mira `MyJourney/Books/[Nombre]/index.json` -> ¿el titulo del capitulo arrancado fue actualizado?

---

### 1) Journal - Bookmarks
- [ ] En portada (sin abrir): ¿dice "K: Open at Bookmark" SIN "(Page XX)"?
- [ ] Con bookmark guardado, pulsa K en portada -> ¿abre DIRECTO en esa pagina sin dar ENTER?
- [ ] Pulsa R en portada -> ¿abre pagina aleatoria directo?
- [ ] En una pagina, pulsa K -> ¿liston rojo se expande por el lomo y sale "Bookmark Saved" 2s?
- [ ] Pulsa K otra vez en esa pagina -> ¿liston se contrae y sale "Bookmark Removed" 2s? (ambos 5px mas abajo que antes)
- [ ] Cierra y reabre con bookmark activo -> ¿liston queda expandido?

### 2) Journal - Colores
- [ ] Cover ¿se ve cuero oscuro (no marron claro)?
- [ ] Paginas ¿crema apagado (210,200,175) no blanco brillante?
- [ ] De noche (21:00-06:00) ¿tinte nocturno no deslumbra?

### 3) Journal - Tiempo Cierre
- [ ] Mantén ESC -> ¿cierra en ~3s (no 5) y barra corre mas rapido?

### 4) CustomBooks - Busqueda (Satchel B 3s)
- [ ] Mantén B 3s -> ¿abre satchel?
- [ ] ¿Barra busqueda NO esta enfocada al abrir?
- [ ] Pulsa K sin clickear barra -> ¿NO escribe K en la busqueda?
- [ ] Click dentro barra -> ¿se enfoca y ya puedes escribir?
- [ ] Click fuera -> ¿se desenfoca?

### 5) CustomBooks - Bookmarks
- [ ] En satchel, pulsa K -> ¿abre libro DIRECTO (sin ENTER) en bookmark o principio si no hay?
- [ ] ¿Animacion liston roja funciona igual que journal?

### 6) CustomBooks - Colores
- [ ] Covers ¿mas oscuros (55%) y sombras mas oscuras?

### 7) Lazy Loading (Libros Grandes)
- [ ] Abre LaBibliaNT1858 (957KB) -> ¿sin bajon a 1fps?
- [ ] Abre LaBibliaViajeroTiempo (4MB) -> ¿sin lag?
- [ ] Pasa paginas rapido -> ¿carga sin congelar?

### 8) Libros Encontrables
- [ ] Al inicio ¿LaBibliaViajeroTiempo NO esta en satchel?
- [ ] Ve a X=-1842 Y=-1038 Z=180 (o coords que hayas puesto) -> ¿a 10m aparece "Presiona E para obtener..."?
- [ ] Pulsa E -> ¿config.ini cambia a isOwned=1 y aparece permanente en satchel?

### 9) Integracion General (1 min)
- [ ] Journal abierto -> pulsa B 3s -> ¿NO abre satchel?
- [ ] Satchel abierto -> pulsa J -> ¿NO abre journal?
- [ ] Alt+Tab -> ¿cierra todo instant?
- [ ] Que te golpeen con journal abierto -> ¿cierra?
- [ ] 5x ESC rapido -> ¿cierra todo (panico)?

### 10) Localizacion (30 seg)
- [ ] Edita WriteYourJourney.ini cambia "P: Rip Page" por "TEST", pulsa F5 -> ¿cambia en juego al instante?

---

## Notas rapidas para ti
- Coordenadas Sheets: usa mod de coords o consola para anotar X/Y/Z de donde dejaste SHEET1.
- Si no ves prompts E, aumenta PickupRadius a 20 en location.ini.
- Todo lo de Sheets esta en `src/ImGuiRDR2Hook/sheets.cpp/h` aislado, no deberia romper lo viejo.

---
*Marca y pegame el resultado, yo actualizo todo.txt y CHANGELOG*
