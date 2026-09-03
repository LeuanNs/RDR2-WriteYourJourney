# Testing - Write Your Journey - Guia Rapida para Leuan
> Ultima Build: 2026-09-03 (Testing Fixes Batch 6 - R Key Fix, Restored Page Overlay, Init Fix)
> Como usar: entra al juego, ve seccion por seccion. Marca [x] si OK, deja [ ] si falla y anota al lado que viste.

---

### 0) Hojas Arrancadas (SHEETS) - Journal

#### R key pickup - FIX CRITICO BATCH 6
**Preparacion:** deja una sheet en el mundo (L en overlay) o ve a una coordenada con sheet conocida

- [ ] Caminar por el mundo sin abrir nada -> acercarse a una sheet -> ¿aparece icono de tecla?
- [ ] Presionar R UNA VEZ (sin mantener) -> ¿personaje camina inmediatamente hacia la sheet? - FIX BATCH 6: tracker manual de key state
- [ ] El pickup funciona desde el primer momento (sin necesidad de abrir journal primero) - FIX BATCH 6: Sheets::Init() al inicio de main()
- [ ] Sheets de sesiones anteriores se detectan correctamente al iniciar el juego - FIX BATCH 6
- [ ] Al llegar -> ¿personaje se agacha (crouch animation)?
- [ ] Al terminar crouch -> ¿hoja se muestra + controles bloqueados?
- [ ] Moverse mientras personaje camina -> ¿se cancela limpiamente?

#### Pagina restaurada - Visual sobre texto - FIX CRITICO BATCH 6
**Preparacion:** abre el journal (J), escribe algo en una pagina, arranca la pagina (P 3s), presiona ESC en overlay

- [ ] Rippear pagina del journal -> presionar ESC en overlay -> ¿animacion de hoja volviendo?
- [ ] Navegar a la pagina restaurada -> ¿se ven las arrugas SOBRE el texto? - FIX BATCH 6: overlay dibujado DESPUES del texto
- [ ] Las marcas de dano (arrugas, manchas, tajos) son visibles encima del contenido
- [ ] Rippear pagina de custombook -> presionar ESC -> ¿mismo visual sobre el texto?

#### [RipSheets] Configuracion INI - BATCH 5
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

#### PickupMessage eliminado - FIX BATCH 6
**Preparacion:** dejar una sheet en el mundo (L en overlay)

- [ ] Dejar una sheet en el mundo -> verificar location.ini generado
- [ ] ¿NO tiene linea PickupMessage? - FIX BATCH 6: eliminado porque es dinamico
- [ ] Acercarse a la sheet -> ¿mensaje dinamico sigue apareciendo correctamente?

---

### 0.5) Index de CustomBooks - FIX CRITICO
**Preparacion:** abre satchel (B 3s), selecciona un libro con index.json

- [ ] Abrir Index directamente con I desde inventory -> ¿ya no se buguea?
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

## Cambios realizados en Batch 6

### Fix 1: R key no se detectaba
- Causa: `GetAsyncKeyState('R')` se llamaba al inicio del loop para reload (R+P), consumiendo el bit `0x0001`
- Solucion: Tracker manual de key state (`s_pickupKeyWasDown`) en vez de `0x0001`
- `Sheets::Init()` ahora se llama al inicio de `main()`, no solo en `OpenSession()`
- Discoverables se cargan al iniciar el juego, no solo al abrir journal

### Fix 2: Pagina restaurada se veia perfecta
- Causa: `DrawRestoredPage()` dibujaba el fondo daniado, pero el texto se renderizaba ENCIMA
- Solucion: Separado en `DrawRestoredPage()` (fondo) + `DrawRestoredPageDamageOverlay()` (marcas)
- Overlay de dano se dibuja DESPUES del texto, haciendo marcas visibles
- Aplicado en journal y custombooks

### Fix 3: PickupMessage redundante
- Eliminado `PickupMessage` de location.ini en `LeaveSheetAtPlayer()`
- Mensaje es dinamico desde `WJConfig::Sheet_Nearby`

---

*Marca y pegame el resultado, yo actualizo todo.txt y CHANGELOG*
