# Testing - Write Your Journey - Guia Rapida para Leuan
> Ultima Build: 2026-09-03 (Testing Fixes Batch 4 - Pickup R Fix, Restored Page Visual)
> Como usar: entra al juego, ve seccion por seccion. Marca [x] si OK, deja [ ] si falla y anota al lado que viste.

---

### 0) Hojas Arrancadas (SHEETS) - Journal

#### Pickup con R - Mundo libre - FIX CRITICO
**Preparacion:** deja una sheet en el mundo (L en overlay) o ve a una coordenada con sheet conocida

- [ ] Caminar por el mundo sin abrir nada -> acercarse a una sheet -> ¿aparece texto "Press R to pick it up"? - FIX: pickup movido fuera del bloque CustomBooksEnabled
- [ ] Mantener R por 3s -> ¿barra de progreso se llena?
- [ ] Al completar hold -> ¿personaje camina hacia la sheet?
- [ ] Al llegar -> ¿personaje se agacha (crouch animation)?
- [ ] Al terminar crouch -> ¿hoja se muestra + controles bloqueados?

#### Pagina restaurada - Visual unica - IMPLEMENTADO
**Preparacion:** abre el journal (J 3s), escribe algo en pag 2, arranca la pagina (P 3s), presiona ESC en overlay

- [ ] Rippear pagina del journal -> presionar ESC en overlay -> ¿animacion de hoja volviendo? - IMPLEMENTADO
- [ ] Pagina restaurada -> ¿visual unica: arrugada, sucia, con tajos en extremos? - IMPLEMENTADO: color mas oscuro, lineas de arrugas, manchas, bordes rasgados
- [ ] Rippear pagina de custombook -> presionar ESC -> ¿misma animacion y visual? - IMPLEMENTADO
- [ ] Verificar que la pagina restaurada se ve diferente a las paginas normales

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

---

## Cambios realizados en Batch 4

### Fix 1: Pickup con R - ahora funciona en mundo libre
- Movido el codigo de pickup de sheets fuera del bloque de CustomBooksEnabled
- Ahora el pickup con R funciona siempre que el journal no este abierto
- El jugador puede caminar por el mundo y al acercarse a una sheet, aparece el texto y puede mantener R por 3s
- El personaje camina, se agacha y muestra la hoja correctamente

### Fix 2: Pagina restaurada con visual unica
- Agregado sistema para trackear paginas que fueron restauradas (s_restoredJournalPages, s_restoredCustomPages)
- Agregada funcion IsPageRestored() para verificar si una pagina fue restaurada
- Modificado RestorePage() para agregar las paginas al set de restauradas
- Implementada funcion DrawRestoredPage() con visual unica:
  - Color base mas oscuro (195,185,160) en vez de crema (210,200,175)
  - Lineas de arrugas aleatorias (8 lineas con opacidad 80)
  - Manchas de suciedad (5 circulos con opacidad 60)
  - Bordes rasgados en la parte superior (tajos de 8-14px)
  - Bordes rasgados en la parte inferior (tajos de 8-14px)
  - Tajos laterales (3 lineas diagonales con opacidad 150)
- Aplicado visual de pagina restaurada tanto en journal como en custombooks
- Las paginas restauradas ahora tienen una apariencia unica: arrugada, sucia y con tajos en los extremos

---

*Marca y pegame el resultado, yo actualizo todo.txt y CHANGELOG*
