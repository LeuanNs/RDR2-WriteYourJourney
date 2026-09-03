# Changelog - Write Your Journey

## [Idea 2 - Sheets/Discoverables] - 2026-09-02

### Nuevo Sistema: Hojas Arrancadas / Discoverables (Sheets)

**Archivos nuevos:**
- `src/ImGuiRDR2Hook/sheets.h` - Header con structs (SheetDrawing, RippedSheetCache, DiscoverableSheet) y API del namespace Sheets
- `src/ImGuiRDR2Hook/sheets.cpp` - Implementacion completa del sistema (~800 lineas)

**Archivos modificados:**
- `src/ImGuiRDR2Hook/config.h` - Nuevas claves de localizacion: Sheet_RipHint, RippingProgress, Sheet_LeaveHint, Sheet_ReadHint, Sheet_RestoreHint, Sheet_CloseHint, Sheet_Nearby, Sheet_PressE
- `src/ImGuiRDR2Hook/menu.cpp` - Include sheets.h, integracion en Render() con checks de estado, P key para rip, "P: Rip Page" en ayuda, efecto visual de paginas arrancadas en DrawOpenBook(), "Page ripped" al seleccionar pagina arrancada, Sheets::Init() en OpenSession()
- `src/ImGuiRDR2Hook/script.cpp` - Include sheets.h, SetPlayerCoords cada frame (con journal abierto y cerrado), UpdatePickupPrompt para sheets, TryPickupSheet con E, bloqueo de controles cuando overlay de sheet esta abierto sin journal
- `src/ImGuiRDR2Hook/ImGuiRDR2Hook.vcxproj` - Añadidos sheets.cpp y sheets.h al proyecto

**Flujo completo implementado:**

1. **Rip de pagina (P hold 3s):**
   - En journal abierto con pagina seleccionada (no zoom, no cover), se muestra "P: Rip Page" en la ayuda inferior
   - Mantener P durante 3 segundos exactos con barra de progreso (estilo ESC/CustomBooks hold)
   - Si se suelta P antes de 3s, se cancela sin efecto
   - Al completar, se inicia animacion de hoja saliendo del libro (0.9s, deslizamiento + rotacion + alpha fade)

2. **Efecto visual de pagina arrancada:**
   - En DrawOpenBook(), si s_pagePair o s_pagePair+1 esta ripped, se dibuja slot con borde rasgado (jitter deterministico con Rng) en vez de pagina crema completa
   - Paginacion no cambia: la pagina ripped deja hueco visible
   - Al seleccionar pagina ripped, se muestra "(Page ripped)" centrado
   - No se renderiza texto ni dibujos en paginas ripped

3. **Overlay de hoja arrancada (estilo zoom):**
   - Tras animacion, overlay centrado con fondo oscurecido
   - Hoja con bordes irregulares (poligono con jitter), color pergamino (210,200,175)
   - Muestra texto completo y trazos de dibujo escalados
   - Ayudas: "L: Leave Page here | R: Read | ESC: Add page back"
   - ESC: restaura pagina original (cancela rip)
   - L: deja hoja en coords actuales del jugador (crea Discoverable)

4. **Sistema Discoverables (hojas en el mundo):**
   - Estructura: `myjourney/Discoverables/SHEET<N>/` con `location.ini`, `sheet.txt`, `sheet_draw.dat`
   - `location.ini`: [Location] con X, Y, Z, PickupRadius=10.0, PickupMessage, Author, Source
   - Generacion incremental: SHEET1, SHEET2, ... (nunca reutiliza numeros)
   - Persistencia de paginas ripped en `myjourney/ripped_pages.ini`
   - Escaneo al iniciar sesion (Sheets::Init/ScanSheets)
   - Prompt de proximidad: al acercarse (<10m), muestra "Hay una hoja arrancada cerca" + "Press E to pick it up"
   - E para recoger: abre overlay con contenido de la hoja
   - Creacion manual: modder puede crear SHEET99/ a mano con archivos y aparece en el sistema

5. **Localizacion completa (INI):**
   - Sheet_RipHint="P: Rip Page"
   - RippingProgress="Ripping page..."
   - Sheet_LeaveHint="L: Leave Page here"
   - Sheet_ReadHint="R: Read"
   - Sheet_RestoreHint="Add page back"
   - Sheet_CloseHint="Close"
   - Sheet_Nearby="There is a ripped page nearby"
   - Sheet_PressE="Press E to pick it up"

**Invariantes respetadas:**
- Sistema aislado en sheets.cpp/h (patron CustomBooks)
- No se tocan DrawReadPage/DrawWritePage/DrawDrawingCanvas/SaveDirPath/PageFilePath/DrawingFilePath
- Natives solo en script.cpp (coords via atomic)
- ImGui solo en Render()
- Arte solo con ImDrawList (sin texturas)
- Integracion minima con checks de estado

---

## [Idea 2 - Mejoras Sheets + Index CustomBooks] - 2026-09-02

### Mejoras en Sistema de Hojas Arrancadas

**Barra de progreso mejorada:**
- Barra de rip (P hold 3s) ahora es mas visible: 320px ancho x 12px alto (antes 200px x 7px)
- Texto "Ripping page..." mas grande (1.3x font size) y mas brillante (255,230,180)
- Fondo oscuro con borde redondeado (5px) para mejor contraste
- Barra de progreso con gradiente dorado (255,200,80 -> 255,180,60)
- Borde exterior mas grueso (2.5px) y brillante (255,215,100)

**Integracion con CustomBooks Index:**
- Al arrancar pagina de CustomBook, se marca capitulo correspondiente como "Ripped Sheet" en index.json
- Funcion `CustomBooks::MarkChapterAsRipped(bookName, lineIndex)` actualiza titulo del capitulo y reescribe index.json
- Funcion `CustomBooks::GetNextValidLineIndex(bookName, lineIndex)` para saltar capitulos ripped

### Nuevo Sistema: Index de CustomBooks

**Archivos modificados:**
- `src/ImGuiRDR2Hook/custombooks.h` - Nuevas funciones: OpenIndex(), CloseIndex(), IsIndexOpen(), RenderIndex(), MarkChapterAsRipped(), GetNextValidLineIndex()
- `src/ImGuiRDR2Hook/custombooks.cpp` - Implementacion completa del Index (~150 lineas)
- `src/ImGuiRDR2Hook/menu.cpp` - Integracion de CustomBooks::IsIndexOpen() y RenderIndex() en Render()
- `src/ImGuiRDR2Hook/config.h` - Nueva clave: Sheet_RippedChapter="Ripped Sheet"
- `src/ImGuiRDR2Hook/sheets.cpp` - Include custombooks.h, llamada a MarkChapterAsRipped() en ConfirmRip()

**Flujo del Index:**

1. **Acceso al Index:**
   - En satchel de CustomBooks, si el libro tiene `hasIndex=1` y capitulos definidos, se muestra "I: Index" junto a "K: Bookmark | R: Random Page"
   - Presionar I abre el Index

2. **Vista del Index:**
   - Pantalla completa con fondo oscuro (10,8,5,230 alpha)
   - Titulo "Index" centrado arriba (2x font size, color dorado 230,205,160)
   - Lista de capitulos centrados verticalmente
   - Capitulo seleccionado: texto dorado (255,215,0) y mas grande (1.3x)
   - Otros capitulos: texto crema (200,180,140) y tamaño normal (1.1x)
   - Navegacion con flechas arriba/abajo
   - Enter para ir al capitulo seleccionado
   - ESC para cerrar Index

3. **Navegacion automatica:**
   - Al presionar Enter en un capitulo, se calcula la pagina: `s_currentPage = targetLine / linesPerPage`
   - Se cierra el Index automaticamente y se muestra el libro en esa pagina
   - Si el capitulo fue arrancado (titulo contiene "Ripped Sheet"), igual se puede navegar a el

4. **Persistencia de capitulos arrancados:**
   - Al arrancar pagina de CustomBook, se llama `MarkChapterAsRipped(bookName, lineIndex)`
   - Busca el capitulo con `lineIndex` coincidente
   - Cambia titulo a "Ripped Sheet (TituloOriginal)"
   - Reescribe `index.json` con el nuevo titulo
   - Al reabrir el libro, el capitulo arrancado aparece como "Ripped Sheet" en el Index

**Localizacion nueva (INI):**
- Sheet_RippedChapter="Ripped Sheet" (texto que se agrega al titulo del capitulo arrancado)

---

## [Última Build] - 2026-09-02

### Cambios en Journal (Diario Principal)

**Bookmarks mejorados:**
- El texto "K: Open at Bookmark" ya no muestra "(Page XX)", solo el texto limpio
- Al presionar K en la portada (Cover), el journal se abre DIRECTAMENTE en la página del bookmark, sin esperar ENTER
- Lo mismo para R (página aleatoria) - abre directamente sin esperar ENTER
- El texto "Bookmark Saved" / "Bookmark Removed" se bajó 5px para mejor posicionamiento visual

**Colores oscurecidos (estética RDR2):**
- Cover del journal: cuero oscurecido de `(56,37,23)` a `(35,22,12)` - tono cuero oscuro sin ser negro
- Gradiente del cuero: de `(76,52,33)` a `(50,32,18)` y de `(45,29,17)` a `(28,16,8)`
- Páginas: de `(234,223,197)` a `(210,200,175)` - menos brillantes, más realistas
- Envejecido de esquinas: tonos reducidos ~10-15% para mejor integración con el juego

**Animación del listón rojo:**
- Al colocar bookmark (K), el listón rojo se expande por el spine del journal
- Texto "Bookmark Saved" aparece por 2 segundos
- Al quitar bookmark (K en página ya marcada), el listón se contrae
- Texto "Bookmark Removed" aparece por 2 segundos
- Si hay bookmark activo, el listón permanece expandido al abrir el journal

**Tiempo de cierre:**
- ESC hold reducido de 5 segundos a 3 segundos para cerrar el journal

### Cambios en CustomBooks (Satchel de Libros)

**Sistema de búsqueda mejorado:**
- La barra de búsqueda SOLO se activa al hacer click en ella
- Click fuera de la barra desenfoca el input
- Las teclas ya no escriben en la búsqueda a menos que la barra esté enfocada
- Esto evita que al presionar K (bookmark) se escriba "K" en la búsqueda

**Bookmarks en CustomBooks:**
- Al presionar K en el satchel, el libro se abre DIRECTAMENTE (sin esperar ENTER)
- Si tiene bookmark, abre en la página del bookmark
- Si no tiene bookmark, abre desde el principio
- Animación del listón rojo + texto "Bookmark Saved" también funciona en custombooks

**Colores oscurecidos (estética RDR2):**
- Covers de custombooks oscurecidos al 55% del color original (antes 100%)
- Sombras de covers al 35% (antes 60%)
- Los colores ahora pegan mejor con la estética oscura de RDR2

**Lazy Loading para libros grandes:**
- Libros con +3000 caracteres usan lazy loading (carga 500 líneas por chunk)
- Al acercarse al final de un chunk, se carga el siguiente automáticamente
- Esto evita lag/FPS drops al abrir libros grandes como la Biblia

### Sistema de Libros Encontrables

**Nuevos campos en config.ini:**
```ini
[Location]
Findable=1              ; 1 = encontrable en coordenadas, 0 = ya poseído
X=-1842.0               ; Coordenada X del mundo
Y=-1038.0               ; Coordenada Y del mundo
Z=180.0                 ; Coordenada Z del mundo
PickupRadius=10.0       ; Radio de recogida en metros
PickupMessage=Presiona E para obtener el libro
```

**Comportamiento:**
- Cuando el jugador se acerca a las coordenadas (dentro del PickupRadius), aparece el mensaje
- Al presionar E, el libro se marca como `isOwned=1` permanentemente
- El libro aparece en el satchel y puede leerse normalmente
- No requiere modificar código, solo configurar el config.ini del libro

**Libros configurados:**
- `LaBibliaNT1858`: owned=1 (ya poseído), index.json con 27 libros del NT
- `LaBibliaViajeroTiempo`: findable=1, coordenadas placeholder (-1842, -1038, 180), index.json con AT+NT
- `TheBibleNT` y `TheLastRider`: actualizados con sección [Location] vacía

### Localización Completa

**Nuevos strings en WriteYourJourney.ini:**
- `CB_NavHint`, `CB_NavHintRandom`, `CB_SearchHint`, `CB_NoBooks`, `CB_NoMatch`
- `CB_OpenLabel`, `CB_BookmarkPage`, `CB_SetBookmarkFirst`, `CB_OpenBeginning`, `CB_OpenRandom`
- `CB_BookNavHint`, `CB_OpenBookLabel`, `CB_OpenAtBookmark`, `CB_OpenFromStart`
- `CB_SatchelOpening`, `BookmarkSaved`, `BookmarkRemoved`

Todos los textos del sistema ahora son traducibles desde el INI.

---

## Lista de Testing Requerido

### 0. Sheets - Sistema de Hojas Arrancadas (NUEVO)
- [ ] Abrir journal, navegar a pagina con texto/dibujo, verificar "P: Rip Page" en ayuda inferior
- [ ] Mantener P durante 3s: barra de progreso se llena, al completar animacion de hoja saliendo
- [ ] Soltar P antes de 3s: barra se resetea, sin efecto
- [ ] Tras rip exitoso: overlay centrado con bordes rasgados muestra texto y dibujos de la pagina
- [ ] En overlay, presionar ESC: pagina se restaura (vuelve al journal sin danos)
- [ ] En overlay, presionar L: hoja se deja en coords actuales, se crea myjourney/Discoverables/SHEET1/
- [ ] Verificar archivos creados: location.ini (con X/Y/Z), sheet.txt, sheet_draw.dat
- [ ] Cerrar y reabrir journal: pagina arrancada sigue faltante (persistencia en ripped_pages.ini)
- [ ] En vista general, la pagina ripped muestra borde rasgado en vez de pagina crema
- [ ] Seleccionar pagina ripped: muestra "(Page ripped)" centrado
- [ ] Navegar a coords donde se dejo SHEET1: prompt "Hay una hoja arrancada cerca" + "Press E"
- [ ] Presionar E: overlay muestra contenido de la hoja encontrada
- [ ] ESC en overlay de discoverable: cierra sin recoger
- [ ] Crear manualmente myjourney/Discoverables/SHEET99/ con sheet.txt + location.ini (X/Y/Z conocidos)
- [ ] Ir a esas coords: verificar que aparece el prompt y se puede recoger
- [ ] Verificar que no hay conflicto de teclas P/E/L/R/ESC con journal y custombooks
- [ ] Alt+Tab con overlay abierto: cierra todo sin crash
- [ ] Recibir dano con overlay abierto: cierra todo sin crash

### 1. Journal - Bookmarks
- [ ] En portada, el texto debe decir "K: Open at Bookmark" (SIN "(Page XX)")
- [ ] Presionar K en portada debe abrir el journal DIRECTAMENTE en la página del bookmark (sin esperar ENTER)
- [ ] Presionar R en portada debe abrir página aleatoria directamente
- [ ] El texto "Bookmark Saved" debe aparecer 5px más abajo que antes (verificar posición visual)
- [ ] El texto "Bookmark Removed" también debe estar 5px más abajo

### 2. Journal - Colores
- [ ] El cover del journal debe verse como cuero oscuro (no marrón claro como antes)
- [ ] Las páginas deben tener un tono crema más suave, menos brillante
- [ ] Verificar que los colores peguen bien con la estética oscura de RDR2
- [ ] El tinte nocturno debe seguir funcionando correctamente

### 3. Journal - Tiempo de Cierre
- [ ] Mantener ESC debe cerrar el journal en 3 segundos (no 5 como antes)
- [ ] La barra de progreso debe llenarse más rápido

### 4. Journal - Animación del Listón
- [ ] Al presionar K en una página, el listón rojo debe expandirse por el spine
- [ ] Texto "Bookmark Saved" debe aparecer por 2 segundos
- [ ] Al presionar K en página ya marcada, el listón debe contraerse
- [ ] Texto "Bookmark Removed" debe aparecer por 2 segundos
- [ ] Al reabrir el journal con bookmark activo, el listón debe permanecer expandido

### 5. CustomBooks - Búsqueda
- [ ] Abrir el satchel (mantener B 3s)
- [ ] La barra de búsqueda NO debe estar enfocada por defecto
- [ ] Presionar K NO debe escribir "K" en la búsqueda
- [ ] Hacer click en la barra de búsqueda debe enfocarla
- [ ] Click fuera de la barra debe desenfocarla
- [ ] Solo cuando está enfocada, las teclas deben escribir en la búsqueda

### 6. CustomBooks - Bookmarks
- [ ] Presionar K en el satchel debe abrir el libro DIRECTAMENTE (sin esperar ENTER)
- [ ] Si el libro tiene bookmark, debe abrir en esa página
- [ ] Si no tiene bookmark, debe abrir desde el principio
- [ ] La animación del listón rojo debe funcionar igual que en el journal
- [ ] Texto "Bookmark Saved" debe aparecer por 2 segundos

### 7. CustomBooks - Colores
- [ ] Los covers de los libros deben verse más oscuros (55% del color original)
- [ ] Verificar que los colores peguen con la estética de RDR2
- [ ] Las sombras deben ser más oscuras también

### 8. Lazy Loading (Libros Grandes)
- [ ] Abrir LaBibliaNT1858 (957KB) NO debe causar lag
- [ ] Abrir LaBibliaViajeroTiempo (4MB) NO debe causar lag
- [ ] Navegar páginas debe cargar chunks adicionales sin congelar
- [ ] No debe haber FPS drops al abrir libros grandes

### 9. Libros Encontrables
- [ ] LaBibliaViajeroTiempo NO debe aparecer en el satchel inicialmente (isOwned=0)
- [ ] Ir a coordenadas X=-1842, Y=-1038, Z=180 (o ajustar si no son correctas)
- [ ] Al acercarse (radio 10m), debe aparecer el mensaje "Presiona E para obtener la Biblia del Viajero del Tiempo"
- [ ] Presionar E debe otorgar el libro (verificar que config.ini se actualiza a isOwned=1)
- [ ] El libro debe aparecer en el satchel permanentemente después de recogerlo

### 10. Integración General
- [ ] Journal abierto: CustomBooks no debe responder a B
- [ ] CustomBooks abierto: Journal no debe responder a J
- [ ] Alt+Tab debe cerrar journal/satchel inmediatamente
- [ ] Recibir daño debe cerrar el journal
- [ ] Modo pánico (5 ESC rápidos) debe cerrar todo

### 11. Localización
- [ ] Editar WriteYourJourney.ini y cambiar algunos strings
- [ ] Presionar F5 para recargar
- [ ] Verificar que los cambios se apliquen inmediatamente

---

## Notas para el Usuario

**Coordenadas de LaBibliaViajeroTiempo:**
Las coordenadas actuales (-1842, -1038, 180) son placeholder. Debes ajustarlas a una ubicación real del mapa relacionada con el "viajero del tiempo". Para encontrar coordenadas, usa un mod de coordenadas o la consola del juego.

**Crear hojas discoverables manualmente (Easter Eggs):**
Para crear una hoja que el jugador pueda encontrar en el mundo:
1. Crear carpeta `myjourney/Discoverables/SHEET<N>/` (N = numero unico, ej: 99)
2. Crear `sheet.txt` con el texto de la hoja
3. Crear `sheet_draw.dat` (opcional, binario con formato de dibujo)
4. Crear `location.ini` con:
   ```ini
   [Location]
   X=-1234.5
   Y=567.8
   Z=100.0
   PickupRadius=10.0
   PickupMessage=Presiona E para recoger la hoja
   Author=Leuan
   Source=EasterEgg
   ```
5. El sistema la escaneada automaticamente al iniciar sesion

**Documentacion:**
- `CreateSheetsGuide.md` - Guia completa para crear hojas arrancadas manualmente (easter eggs) sin tocar codigo

Ver `CreateBooksGuide.md` para instrucciones completas sobre cómo crear libros custom sin tocar código.

---

*Made with love By Leuan... May god bless you all*
