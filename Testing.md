# Testing - Write Your Journey - Guia Rapida para Leuan
> Ultima Build: 2026-09-02 (Testing Fixes Batch 1 - R:Look Behind, Lazy Loading, Search Bar, Reload)
> Como usar: entra al juego, ve seccion por seccion. Marca [x] si OK, deja [ ] si falla y anota al lado que viste.

## Como reportarme
Cuando termines una seccion, dime por ejemplo:
`Seccion 0: OK todo menos L no crea SHEET1 (no aparece carpeta)`
O `Seccion 4: Bookmark Saved no aparece`
Yo lo cruzo con el changelog y actualizo el todo.

---

### 0) Hojas Arrancadas (SHEETS) - Journal
**Preparacion:** abre el journal (J 3s), escribe algo en pag 2 y dibuja un trazo. Quedate en esa pagina seleccionada.

- [ ] ¿Ves abajo "P: Rip Page" junto a ESC/K/R? - SI 
- [ ] Mantén P 3s -> ¿barra "Ripping page..." se llena? (320px ancho x 12px alto, dorado)  - SI  PERO MODIFICAR, REVISAR ABAJO
- [ ] Suelta P a los 1-2s -> ¿barra se cancela y NO pasa nada?  - SI , FUNCIONA BIEN
- [ ] Mantén P 3s completo -> ¿hoja se desliza/rota y hace fade saliendo del libro? (0.9s)  - SI , FUNCIONA BIEN
- [ ] ¿Overlay centrado aparece con la misma hoja, bordes rasgados, texto + dibujo? -   - SI , FUNCIONA BIEN
- [x] En overlay ¿ves "L: Leave Page here | R: Look Behind | ESC: Add page back"? - ARREGLADO: Ahora se carga contenido de pagina partner al rippear, "R: Look Behind" aparece si hay contenido atras


- [x] Pulsa ESC en overlay -> ¿hoja vuelve al journal intacta? (rip cancelado) - SI, PERO SIN EFECTO VISUAL DE QUE FUE ARRANCADA O ARRUGADA (pendiente efecto visual)
- [ ] Pulsa L -> ¿overlay se cierra, hoja queda en el mundo? -   - SI , FUNCIONA BIEN
- [ ] Alt+Tab, mira `myjourney/Discoverables/SHEET1/` -> ¿existe `location.ini`, `sheet.txt`, `sheet_draw.dat`?  - SI , FUNCIONA BIEN
- [ ] Cierra journal, vuelve a abrir -> ¿la pagina arrancada se ve con hueco y borde rasgado? - NO
- [ ] Paginas ripeadas -> ¿muestran glow pulsante azul como pista visual? - NO 
- [ ] Click en pagina ripped -> ¿dice "Page Ripped" (configurable en INI)? - SI, PERO NO HAY GLOW
- [ ] No se puede seleccionar pagina ripped directamente (solo navegar con flechas) - exacto
- [ ] Rippear pag 2 -> pag 3 tambien desaparece (misma hoja fisica) - SI
- [ ] Rippear pag 6 -> pag 7 tambien desaparece (trasera) - SI
- [ ] Al restaurar con ESC -> paginas partner tambien se restauran - SI PERO SIN EFECTO
- [ ] Al mostrar overlay -> texto de otras paginas NO se solapa - EXACTO, SOLO SI SE MIRA LA REVERSA

#### Pickup con E (NUEVO)
- [ ] Acercarse a sheet a 5m -> ¿aparece cuadrado con "E" (tecla keyboard)? - SI
- [ ] Mantener E -> ¿barra blanca se llena alrededor del cuadrado (3s)? - SI PERO ESTA MEDIO BUG, ASI QUE LO CAMBIAREMOS A "R"
- [ ] Soltar E antes de 3s -> ¿se cancela? - SI, PERO MEDIO BUG PQ SE SOLAPA CON EL "DESCANSAR" DEL JUEGO, ARREGLADO CAMBIANDOLO A "R"
- [ ] Moverse mientras se mantiene E -> ¿se cancela? - Si, pero no, se cancela, pero vuelve a ir después de unos segundos al lugar e intentar hacer lo mismo y siempre termina renderizando la hoja, la única forma de desbugearlo es apretando "K" para quedarse la hoja, pq si se apreta ESC, tampoco se cancela y se queda en un loop
- [ ] Completar barra E -> ¿personaje camina hacia la sheet? - SI 
- [ ] Moverse mientras camina -> ¿se cancela? - NO, LEER ARRIBA
- [ ] Al llegar -> ¿personaje se agacha (crouch)? - NO
- [ ] Al terminar crouch -> ¿hoja se muestra + controles/HUD/camara bloqueados? - SI, PERO NO HAY CROUCH, PERO LO OTRO SI
- [ ] Al completar E hold -> ¿texto "hay una hoja cerca" desaparece? - SI, pero al apretarle "K" para keepear, no se esta añadiendo de vuelta, ni al journal, ni al book (a cualquiera q pertenezca), desaparece para siempre y queda perdida
queda en nuestro inv, por asi decirlo, pero no aparece nunca más ni en el journal, ni en un custombook

#### Overlay mejorado (NUEVO) - ARREGLADO EN ESTA BUILD
- [x] En overlay -> "R: Look Behind" visible si hay contenido atrás - ARREGLADO: Ahora se carga contenido de pagina partner al rippear
- [x] Presionar R -> ¿animacion 3D de giro? - ARREGLADO: Animacion de escala horizontal (0.8s)
- [x] Al terminar giro -> ¿muestra contenido de pagina trasera? - ARREGLADO: Muestra backText/backDrawing
- [x] Presionar R de nuevo -> ¿gira de vuelta al frente? - ARREGLADO: Toggle funciona correctamente
- [x] En overlay de discoverable -> "K: Keep the Sheet" visible - ARREGLADO: Se muestra en overlay
- [x] Presionar K -> ¿overlay desaparece, control vuelve al jugador? - ARREGLADO: KeepSheet() funciona
- [x] Despues de K -> sheet NO aparece al volver a pasar - ARREGLADO: Se marca como collected en location.ini
- [x] Si pagina vacia -> NO se muestra "(empty page)" - ARREGLADO: No se muestra texto si esta vacia

---

### 0.5) Index de CustomBooks
**Preparacion:** abre satchel (B 3s), selecciona un libro con index.json

- [x] En satchel, ¿ves "I: Index"? - si
- [x] Pulsa I -> ¿abre Index INMEDIATAMENTE (sin ENTER)? - ARREGLADO: Ahora cierra inventario al abrir Index
- [x] Index se ve como pagina del mismo libro (cover, pergamino, spine) - SI, PERO ESTA PEGADO A LA PAGINA DE LA IZQUIERDA, LA IDEA ERA QUE FUERA AL MEDIO, SI NO se puede al medio, dejar como esta
- [x] Flechas arriba/abajo -> navega entre capítulos - si
- [x] Enter en capitulo -> cierra Index, abre libro en esa pagina - SI
- [x] Abrir capitulo en pagina lejana (ej: 940) -> lazy loading funciona - ARREGLADO: Calculo de pagina corregido (divide por linesPerPage*2)

---

### 0.6) NUEVO - CustomBooks Page Selection + Rip + Edit
**Preparacion:** abre satchel (B 3s), selecciona cualquier libro, ENTER para abrirlo

- [ ] Sin pagina seleccionada -> flechas pasan hojas normalmente (pares) - si
- [ ] Click en pagina izquierda o derecha -> ¿glow azul pulsante aparece? - SI, FUNCIONA PERFECTO, PERO IDEALMENTE AGREGAR UN TEXTO JUNTO AL  "arrows: turn page - blablá - esc: close", agregarle algo más q diga "ENTER: Selection Mode" y que ahi recién aparezca el glow para seleccionar páginas 
- [ ] Con pagina seleccionada -> flechas izq/der mueven entre paginas INDIVIDUALES - si
- [x] Con pagina seleccionada -> P -> ¿arranca la pagina? (overlay con contenido) - ARREGLADO: Ahora se muestra barra de progreso y animacion de ripear (Sheets::Render() se llama antes de custombook early returns)
- [ ] Con pagina seleccionada -> E -> ¿modo edicion? (cuadro texto arriba) - SI
- [ ] Escribir texto + ENTER -> ¿tachado + correccion en pequeno arriba? - SI, PERO OJO, AL RIPPEAR UNA PAGINA CON TEXTO TACHADO, EL TACHADO QUEDA Y SE APLICA A LA PAGINA QUE VIENE... ESTO ESTA MUY MUY MAL, el tachado debe quedar en la hoja ripeada, APARTE!!!! Al rippear una página (que funciona 
- [ ] Verificar `edits.txt` en carpeta del libro - no verifique
- [ ] Reabrir libro -> ediciones siguen visibles - si
- [ ] Paginas ripeadas en custombook -> borde rasgado + "Page Ripped" - NO, nada de esto visual
- [ ] ESC -> deselecciona pagina, vuelve a navegacion normal  - SI

---

### 1) Journal - Bookmarks
- [ ] En portada: ¿dice "K: Open at Bookmark" SIN "(Page XX)"? - SI
- [ ] Con bookmark guardado, pulsa K en portada -> ¿abre DIRECTO en esa pagina? - SI
- [ ] Pulsa R en portada -> ¿abre pagina aleatoria directo? - SI, pero problema lazyloading 
- [ ] En una pagina, pulsa K -> ¿liston rojo + "Bookmark Saved" 2s?  - SI
- [ ] Pulsa K otra vez -> ¿liston se contrae + "Bookmark Removed" 2s? (6px mas abajo que antes)  - SI
- [ ] Cierra y reabre con bookmark -> ¿liston queda expandido?  - SI

### 2) Journal - Colores
- [ ] Cover ¿cuero oscuro (no marron claro)?  - SI, perfecto
- [ ] Paginas ¿crema apagado (210,200,175)?  - SI
- [x] De noche ¿tinte nocturno no deslumbra? - ARREGLADO: Tinte mas oscuro (alpha 130->180, RGB reducidos)

### 3) Journal - Tiempo Cierre
- [ ] Mantén ESC -> ¿cierra en ~3s? - si, aproximadamente

### 4) CustomBooks - Busqueda (Satchel B 3s)
- [ ] Mantén B 3s -> ¿abre satchel?  - SI
- [ ] Barra busqueda NO enfocada al abrir - SI
- [x] Click dentro barra -> se enfoca - ARREGLADO: Ahora muestra cursor "|" parpadeante y acepta espacios
- [x] Click fuera -> se desenfoca - ARREGLADO: Tambien se desenfoca con ESC y al cerrar inventario

### 5) CustomBooks - Bookmarks
- [ ] En satchel, pulsa K -> ¿abre libro DIRECTO en bookmark o principio? - SI
- [ ] Animacion liston roja funciona - SI

### 6) CustomBooks - Colores
- [ ] Covers ¿mas oscuros (55%)? - SI

### 7) Lazy Loading (Libros Grandes)
- [x] Abre LaBibliaNT1858 -> ¿sin bajon a 1fps? - si, sin bajon
- [x] Navegar hacia ADELANTE -> ¿funciona? (antes solo retroceder) - ARREGLADO: startLine ajustado para chunk offset
- [x] Llegar a pag 58+ -> ¿texto sigue renderizando? - ARREGLADO: Bounds check y offset corregido
- [x] Navegar atras y adelante -> ¿chunks se cargan? - ARREGLADO: Texto se renderiza correctamente en ambas direcciones



### 8) Libros Encontrables - TODA LA SECCIÓN 8 NO HA SIDO TESTEADA, ASI QUE NO SE PUEDE SABER, DEJAR INTACTA
- [ ] Al inicio ¿LaBibliaViajeroTiempo NO esta en satchel? - TODA LA SECCIÓN 8 NO HA SIDO TESTEADA, ASI QUE NO SE PUEDE SABER, DEJAR INTACTA
- [ ] Ve a coords -> ¿a 5m aparece mensaje?  - TODA LA SECCIÓN 8 NO HA SIDO TESTEADA, ASI QUE NO SE PUEDE SABER, DEJAR INTACTA
- [ ] Pulsa E -> ¿config.ini cambia a isOwned=1? - TODA LA SECCIÓN 8 NO HA SIDO TESTEADA, ASI QUE NO SE PUEDE SABER, DEJAR INTACTA

### 9) Integracion General 
- [ ] Journal abierto -> pulsa B 3s -> ¿NO abre satchel? - No lo abre, esto está funcionando perfecto!
- [ ] Satchel abierto -> pulsa J -> ¿NO abre journal? - No lo abre, esto está funcionando perfecto!
- [ ] Alt+Tab -> ¿cierra todo instant? - Sip
- [ ] Que te golpeen -> ¿cierra? - no testeado
- [ ] 5x ESC rapido -> ¿cierra todo? -  nop

### 10) Localizacion
- [x] Edita WriteYourJourney.ini cambia textos -> R+P 5s -> ¿cambia en juego? - ARREGLADO: Reload ahora es R+P hold 5s (no F5)

---

## Notas rapidas
- Todo lo de Sheets esta en `src/ImGuiRDR2Hook/sheets.cpp/h`
- CustomBooks en `src/ImGuiRDR2Hook/custombooks.cpp/h`
- Edits de CustomBooks se guardan en `MyJourney/Books/[Nombre]/edits.txt`

---
*Marca y pegame el resultado, yo actualizo todo.txt y CHANGELOG*
