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

- [ ] ¿Ves abajo "P: Rip Page" junto a ESC/K/R? (NO debe ser cuadro, solo texto) - FUNCIONA 
- [ ] Mantén P 3s -> ¿barra "Ripping page..." se llena? **AHORA MAS VISIBLE:** 320px ancho x 12px alto, texto grande dorado, fondo oscuro con borde redondeado, barra con gradiente dorado brillante - FUNCIONA
- [ ] Suelta P a los 1-2s -> ¿barra se cancela y NO pasa nada? - FUNCIONA
- [ ] Mantén P 3s completo -> ¿hoja se desliza/rota y hace fade saliendo del libro? (0.9s) - FUNCIONA
- [ ] ¿Overlay centrado aparece con la misma hoja, bordes rasgados/irregulares, color pergamino, texto + dibujo igual? - FUNCIONA
- [ ] En overlay ¿ves abajo "L: Leave Page here | R: Read | ESC: Add page back"? - FUNCIONA [ACA REMOVER EL "R: READ" ES INNECESARIO
- [ ] Pulsa ESC en overlay -> ¿hoja vuelve al journal intacta? (rip cancelado) - FUNCIONA
- [ ] Repite rip P 3s, ahora pulsa L -> ¿overlay se cierra sin error? - FUNCIONA
- [ ] Alt+Tab fuera del juego, mira `myjourney/Discoverables/SHEET1/` -> ¿existe `location.ini` con X/Y/Z, `sheet.txt` con tu texto, `sheet_draw.dat`? -FUNCIONA 
- [ ] Cierra journal (ESC 3s), vuelve a abrir -> ¿la pagina que arrancaste ahora se ve con hueco y borde rasgado/pedacitos en el lomo? No debe verse crema completa. - FUNCIONA
- [ ] Selecciona esa pagina ripped -> ¿dice "(Page ripped)" centrado? - NO FUNCIONA DETALLADO ABAJO
- [ ] Quedate parado donde diste L, ¿ves prompt "Hay una hoja arrancada cerca" + "Press E to pick it up" a 10m? - SI, FUNCIONA, PERO MODIFICAR, DETALLADO ABAJO
- [ ] Pulsa E -> ¿overlay muestra la hoja encontrada? - SI, FUNCIONA PERFECTO 
- [ ] ESC en ese overlay de discoverable -> ¿cierra sin crashear? - SI FUNCIONA PERFECTO
- [ ] **Easter egg manual:** crea `myjourney/Discoverables/SHEET99/` con `sheet.txt` (hola mundo) y `location.ini` ([Location] X=-1842 Y=-1038 Z=180 PickupRadius=10), ve a esas coords -> ¿aparece prompt y puedes recogerla? - SI, FUNCIONA PERFECTO
- [ ] Prueba con Alt+Tab y con daño (que te golpee un NPC) con overlay abierto -> ¿cierra todo sin crash? - SI, FUNCIONA PERFCTO
- [ ] Prueba que P/E/L/R no rompen CustomBooks ni Journal (ver secc 10)

**Si algo falla anota:** ¿barra no aparece? ¿animacion trabada? ¿archivos no creados? ¿overlay vacio?
Al rippear una página y apretar esc para devolverla, no queda con cambios visuales en comparación a las otras hojas, y mucho peor, si paso de p´pagina a otras, esta hoja ripeada se sigue dibujando encima de todas las págs., por ej; si ripie la pág 3 (izq)
y luego la pegue de nuevo, aun que vaya a la pág 7 y 8, en la 7 ( izq) se vera el contenido de la pág 3 (arreglar esto}

También arreglar que apenas se aprete "E" no se vaya directamente a la zona, si no que en vez del texto de abajo "Press E to pick it up" idealmente poner un cuadrado con la letra "E" (simulando una tecla de teclado, como lo hace rdr2 en sus menus) y que al apretarla, alrededor del cuadrado, se empiece a llenar una barrita blanca, indicando que se debe mantener presionada la "E" hasta que se llene la barrita (3s), y al llenarse, el personaje recién ahi, se dirigirá a buscar la sheet… hay que asegurarnos de no quitarle
el movimiento al usuario, si el usuario se mueve mientras el personaje iba caminando hacia la sheet, la acción se cancela y la hoja nunca se muestra, ni el personaje sigue avanzando automáticamente... al haber intercepción de controles por el usuario, se cancela la acción - otra cosa, al ejecutar exitosamente el apretar la E hasta que se llene la barra cuadrada, hacer desaparecer el texto en pantalla completamente de que hay una página cerca... y solo volver a mostrarlo si el jugador 1.- ya vió la página y 2.- no decidió guardarla en su diario... si decidió guardarla en su diario (ver más abajo, donde se detalla lo de "K: Keep the Sheet"), pues en el "discoverables/sheetX" hay que poner en su location.ini, que ya no está disponible para ser encontrada, ni leida, ni el sistema la tomará, ya que se asume que el usuario la posee ahora

y vamos a reducir la distancia en la que aparece el texto, pq está de muy lejos, lo mismo con radio, lo dejaremos solo a 5m que aparezca.

Otra cosa, el personaje está caminando perfectamente a la coordenada, pero al llegar, no se agacha (crouch), hay que hacerlo agacharse, para simular que recoge algo
(y recién al terminar de reproducirse la animación del crouch, ahi recién se muestra la hoja, y cuando se muestra la hoja, se bloquea el control completo al jugador + hud + cámara, como siempre

Otra cosa, al apretar "I" para el index, no esperar el "ENTER"; apenas se aprete i para ver el index, abrir el index y mostrarlo. - también faltó que el index se esta dibujando como un menú, sin cuaderno
la idea del index es que simula ser una "hoja" del mismo book, que se abrió... se entiende?

Otra cosa, revisar que al abrir una página en el index, por ejemplo de la biblia 1858, está en blanco (lazyloading se aplicó solo para las primeras páginas, hay que hacer que se aplique el lazyloading siempre en la página actual abierta, si se abre de la nada en la 940 hay que aplicar la normativa de nuestro lazyloading ya hecha, desde la 940
y así [otra cosa dentro de sto mismo, es que no me dejaba avanzar para la derecha  (páginas adelante), solo me dejaba retroceder páginas para la izquierda (retroceder páginas)]

Otra cosa, textos muy largos no se están renderizando (me imagino problema lazy loading igual), por ejemplo en la biblia, llega hasta la pág 58 con lo ultimo escrito "lo que había pasado con los endemoniados" y no hay más, no puedo seguir avanzando  páginas (pero si retroceder)

otra cosa, al rippear una hoja y mostrarla, hay que seguir mostrando el journal/book atrás, pero dejar de renderizar el texto de las otras páginas que no se arrancaron, pq como se ve en la imagen se solapa el texto de la página no arrancada, con la arrancada

otra cosa, al hacer click por ejemplo, en la página 4 (luego de que la 2 haya sido ripeada), dice infinitamente "page ripped" rápidamente y se oculta.. pero no da ninguna indicación visual de que ahi hubo una página arrancada
La idea es que  en el lugar de donde estaría pág 2 (o la página arrancada en otros casos), quede  dibujada como en los bordes, pedazos de papel como arrancados, y que haya una mini capa por así decirlo y que no deje seleccionar la pág siguiente (pág 4), desde ahi, la siguiente página mostrándose ya que se rippeo, solo se puede acceder, si se pasa de página a su correspondiente (/con las flechas avanzar a la 3 y 4 en este ejemplo)… mientras se este en pág 1 y 2 (o las págs. rippeadas), solo es visual
[y obvio, si hay una página ripeada, igual la vamos a seleccionar completa (al menos la silueta de la hoja completa), para darle a entender al usuario con ese glow, una minipista visual, de que falta una hoja completa... y que si intenta seleccionar esa hoja
que ahi si salga "Page Ripped" [configurable texto en el .ini]

otra cosa, bajar un poco más el texto de "bookmark saved / deleted" pero solamente en el journal, bajarlo unos 6px más

otra cosa, al rippear una página, por ejemplo, ripie la página 6 (de la 5 y 6), y luego hice todo... resulta que se perdió el dibujo /contenido de la pág 7 (la izquierda)
ya que la pág 5 esta izquierda, la 6 derecha, la 7 izquierda ( atrás de la 6 ) y la 8 derecha...
bueno, al ripearla, coherentemente con la realidad, se perdió la (7) la que estaba atrás de la 6... [en realidad eran una misma página, solo que escritas por adelante y atrás]

esto no quiero borrarlo, pero me gustaría que cuando se esté mostrando la "página arrancada" mostrarhojaarrancada, exista el "R: mirar atrás", y que si se apreta R, se haga una animación de girar la hoja en 3d, y que muestre la parte de atrás (la pag 7 en este caso) y su contenido...

otra cosa, al encontrar una sheet en el mundo, favor permitir "K: Keep the Sheet" | si se apreta "K", mientras se lee la hoja arrancada (se muestra), desaparece el imgui de mostrarhojaarrancada, se devuelve el control al jugador y agregamos de vuelta la página
al journal/book, exactamente donde pertenece... pero dejándola marcada de que fue pegada a mano pq estaba arrancada (marcada significa agregarle visualmente que está rota, arrugada, etc)

otra cosa, remover todo tipo de texto que insinue "Empty Page" "Nothing written" o un texto irreal, que muestre que no fue escrito nada, si no hay escrito nada, no mostrar nada... simple

otra cosa, descubrí que al rippear una sheet, por ejemplo pondremos la 1 y 2 y 3 y 4
la 1 y 2, rippeo la 2 y la dejo en el mundo

al abrir de nuevo e journal, cuando estoy en la página 1 y 2, no solo puedo ver como te comenté arriba sin distinción clara que la 4 está atrás, solo que fue rippeada, si no que acabno de darme cuenta que puedo ver la 3 transcrita y sobrepuesta en la 1
(al parecer como renderizamos la 4, se cargo la 3 también y se puso sobre la 1), esto no debería pasar nunca, pq las ripeadas fueron en este ejmplo la 2 y 3, entonces como va a aparecer la 3 (izquierda) en la página 1?

asegurarse de que al ripear una sheet, se vaya por el frente la pág que se ripeo + la página matemáticamente correcta que le corresponda (si se ripea la 2, la 3 le corresponde la parte trasera, si se ripea la 50, la 51 le corresponde parte trasera) y así

otra cosa, descubri que al estar leyendo un custombook, no hay GLOW para decir que página estamos seleccionando, solo se lee todo y se pasa de página al apretar la flecha
como en el journal (copiar sistema de glow y selección de páginas), se debe poder seleccionar y mover entre las páginas dndividuales, permitiendo sobreescribir y sobredibujar en un custombook, en una página
[se supone que lo que se sobreescribe o sobredibuja, queda guardado en un .txt y un .dat en la misma carpeta donde existe ese libro (body, config, index, etc) y deben mostrarse siempre...
(y obvio, si el usuario borra/modifica un texto del body.txt, no lo cambiaremos completamente) si no que, aplicaremos un efecto donde "tachamos" la palabra u frase que el usuario cambio/modifico, y arriba muy pequeño, escribimos lo nuevo, simbolizando como en los cuadernos o libros de la vida real, que no se puede borrar lo que esta escrito en un libro, pero se puede tachar y escribir arriba en pequeñito la corrección)

[que lo pequeñito quede acorde a la palabra/frase tachada y no se vea feo, ni colinde con el texto de arriba de el]

otra cosa, al no poder elegir página en un custombook, tampoco puedo rippear la página, ni testear nada de eso... favor aplicar también a los custombook esta mecánica...

Recuerda no tocar sistemas antiguos o ya funcionales, es solo implementación, lo único que sirven los sistemas o códigos ya funcionando  son de referencia
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
