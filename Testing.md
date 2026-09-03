# Testing - Write Your Journey - Guia Rapida para Leuan
> Ultima Build: 2026-09-03 (Testing Fixes Batch 7 - Restored Page Overlay Fix, KeepSheet Restore, Index Fix, Mouse Handling)
> Como usar: entra al juego, ve seccion por seccion. Marca [x] si OK, deja [ ] si falla y anota al lado que viste.

---

### 0) Hojas Arrancadas (SHEETS) - Journal

#### Restored page damage overlay - FIX CRITICO BATCH 7
**Preparacion:** abre el journal (J), escribe algo en una pagina, arranca la pagina (P 3s), presiona ESC en overlay

- [ ] Rippear pagina del journal -> presionar ESC en overlay -> ¿animacion de hoja volviendo? - NO
- [x] Navegar a la pagina restaurada -> ¿se ven las arrugas SOBRE el texto? - FIX BATCH 7: ahora usa GetForegroundDrawList() - SI, con comentarios en el prompt pasado
- [x] Las marcas de dano (arrugas, manchas, tajos) son visibles ENCIMA del contenido de texto
- [x] Rippear pagina de custombook -> presionar ESC -> ¿mismo visual sobre el texto?

#### KeepSheet restores page - FIX CRITICO BATCH 7
**Preparacion:** deja una sheet en el mundo (L en overlay), cierra journal, ve a la sheet

- [x] Acercarse a la sheet -> presionar R -> personaje camina y recoge
- [x] En overlay de discoverable -> presionar K para keepar
- [x] Abrir journal/custombook al que pertenece la pagina -> ¿la pagina aparece restaurada?
- [x] La pagina restaurada tiene el visual daniado (arrugas, manchas, tajos)
- [x] El texto de la pagina es legible debajo del dano

#### Ripped page visual mejorado - BATCH 7
**Preparacion:** arranca una pagina del journal o custombook

- [x] Pagina arrancada en journal -> ¿muestra texto "Page Ripped" centrado?
- [x] Borde rasgado mas visible (jag size 8 vs 6 anterior)
- [x] Mas manchas de suciedad (8 vs 5 anterior)
- [x] Lineas de tajos diagonales visibles (3 lineas)
- [x] Mismo visual mejorado en custombooks

#### Index I+ENTER - FIX CRITICO BATCH 7 + CRASH FIX
**Preparacion:** abre satchel (B 3s), selecciona libro con index

- [x] Presionar I -> ¿NO abre index directamente? (debe mostrar hint) - SI ABRIO, PERO CRASHEO EL JUEGO - **FIX: try-catch en LoadChunk y Enter handler**
- [x] Presionar ENTER despues de I -> ¿abre index correctamente? - SI ABRIO, PERO CRASHEO EL JUEGO - **FIX: s_currentPage se setea DESPUES de OpenBook()**
- [x] ESC en index -> ¿cierra sin bugear?
- [x] Seleccionar capitulo lejano (ej: Efesios) -> ¿NO crashea?
- [x] Lazy loading funciona desde Index -> texto se renderiza

#### Random page lazy loading - FIX BATCH 7
**Preparacion:** abre satchel (B 3s), selecciona libro grande (ej: Biblia)

- [ ] Presionar R en satchel -> ¿abre pagina aleatoria?
- [ ] La pagina aleatoria muestra texto correctamente (no en blanco)
- [ ] Lazy loading funciona desde la pagina aleatoria

#### Mouse handling - BATCH 7
**Preparacion:** abre journal y custombooks

- [x] Journal en modo Read con pagina seleccionada -> ¿mouse cursor OCULTO?
- [x] Journal en overview (seleccionando con flechas) -> ¿mouse cursor VISIBLE? - SI, PERO ESTO DEBE SER "OCULTO" ACA EN LA SELECCIONANDO CON FLECHAS
- [x] CustomBooks inventory abierto -> ¿mouse cursor VISIBLE para search bar?
- [x] CustomBooks libro abierto -> ¿navegacion con flechas funciona?

---

### 0.5) Hojas Arrancadas (SHEETS) - Batch 6 Tests

#### R key pickup - FIX CRITICO BATCH 6
**Preparacion:** deja una sheet en el mundo (L en overlay) o ve a una coordenada con sheet conocida

- [x] Caminar por el mundo sin abrir nada -> acercarse a una sheet -> ¿aparece icono de tecla?
- [x] Presionar R UNA VEZ (sin mantener) -> ¿personaje camina inmediatamente hacia la sheet? - FIX BATCH 6: tracker manual de key state
- [x] El pickup funciona desde el primer momento (sin necesidad de abrir journal primero) - FIX BATCH 6: Sheets::Init() al inicio de main()
- [x] Sheets de sesiones anteriores se detectan correctamente al iniciar el juego - FIX BATCH 6
- [ ] Al llegar -> ¿personaje se agacha (crouch animation)? - NO, NUNCA SE AGACHA
- [x] Al terminar crouch -> ¿hoja se muestra + controles bloqueados?
- [x] Moverse mientras personaje camina -> ¿se cancela limpiamente?

---

ENTONCES; COSAS POR FIXEAR MI ESTIMADO QWEN:
- El efecto de arrugado / roto, al restaurar una página está excelente!! PERO!!! un pquito muy oscuro, podríamos aclararlo solo un toquecito pequeño más? (pero que no sea del mismo clor que las hojas originales, si no q mas oscuro, pero no tanto como ahora)

- Podríamos poner el "glow" de la missing page, en rojo? y hacerlo el de journal como el de custombook (viste q custombook en vez de transparente, rellena con el mismo azul y pone "page ripped" o algo asi?, bueno hacer que los glows ahora sean rojos en vez de azul (solo para las páginas que están rippeadas y perdidas (no recogidas aun) y que el "relleno con el mismo azul" del cuerpo del cuadrado, en vez de azul, sea gris y leve muy levemente transparente... pero que predomine el gris/oscuro, y si podemos ponerle
sutiles líneas de arriba a abajo, cruzando en diagonal, líneas gruesas, simulando un poco debrillo, pero dentro de las líneas como de reflejo, hacer hartos signos "?" pequeños y grises transparentes... cosa de que no solo sea reflejo, si no que le de la estética "?"

- Otra cosa, en custombook, cuando ripeamos una hoja y apretamos "L" para dejarla en el mundo, la imgui del custombook vuelve, pero vuelve pegada y bugeada mostrando AMBAS páginas (la izq y derecha) como "ripped" y pone el glow, y no se puede hacer nada, hay que apretar "ESC" para salir al menú de selección de custombook, seleccionar de nuevo el libro y ahi si todo bien

- OTRA COSA MUY IMPORTANTE!!! en el journal (no sé muy bien en custombook, parece que aca funciona bien, pero revisar) al arrancar una página, por ejemplo estamos en la 1 y 2, y arranco la 2 y la dejo en el mundo...
se supone, en teoría, que lo correcto es que el journal después muestre solo la 1, en la 2 salga el glow rojo, con el relleno gris/transparente con "?", y si paso a la pág 3 y 4, la 3(izq) debería tener lo mismo de "page ripped", en glow rojo y faltando y la 4 debería verse bien, no?

bueno como funciona actualmente; se rippea la página 2, se deja en el mundo... al abrir el journal de nuevo, la pág 1 y 2 están como ripped, la 3 y 4 están normales (a veces la 3 está ripped, a veces no)
esto está pésimo, pq paso a llevar la 1? si no tenía nada que ver? (ahora, favor entiende que esto es un ejempl ode como testie, no es obligatoriamente con las págs. 1,2,3,4... uso estas 4 para referenciarte los lados (izq, derecha)

- otra cosa, la barrita de "ripping page" al apretar P en el custombook (solo custombook, la de journal esta perfecta), aparece atrás de los imgui y se ve horrible, podría la barrita copiarla para que sea igual visualmente uqe la del journal?

- otra cosa, descubrí que mientras se esta viendo el journal, todas las hojas son perfectas. pero si por ejemplo, selecciono con enter una página que fue arrancada, ahi recién se aplica lo visual de roto, arrugado, etc... la idea es que siempre se vea así, no solo cuando se entra a la página rasgada

la idea es que por ejemplo, si se está renderizando (aun sin seleccionar ninguna página) la pág 1 y 2 por ejemplo, y la 2 fue arrancada, ya en el preview (sin seleccionar página aun), ya se visualice lo arrancado, roto, arrugado, etc
[y que sea algo acumulable, si una página mientras más vaya siendo arrancada (si ya fue arrancada una vez, se puso restauro (Keep) y luego se volvió arrancar, se le aplice aun más intenso el efecto de roto, arrugado, etc
asi hasta acumular un total de 5 veces, y luego de 5 veces esté casi el 70% roto/arrugado, sea dificil leer (el texto está bajo estos "quiebres", haciendo que si hay una parte del texto, en la parte rota, este no se lea)
y de ahi se quede para siempre así, será imposible verla más rota, después de 5 veces, llegando al 70%, se estanca ahi esa página [y no tiene más cambios visuales]

- otra cosa, de noche sigue estando demasiado oscuro, súbele un poco más el brillo al journal y hojas cuando es de noche. (exactamente igual que cuando es de dia, pero algo asi como 15% menos)

- Muyr ugente, al apretar "I", luego "ENTER", y seleccionar algo random del index (por ejemplo efesios de la biblia nt owned=1), se pegó el juego y crasheó todo... ??? que pasó aca? el lazyloading o algo más? donde quedaron los try-catchs para evitar estos crashs?


