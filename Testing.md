# Testing - Write Your Journey - Guia Rapida para Leuan
> Ultima Build: 2026-09-05 (Sistema de Cartas - Letters)
> Como usar: entra al juego, ve seccion por seccion. Marca [x] si OK, deja [ ] si falla y anota al lado que viste.

---

### 0) Sistema de Cartas (LETTERS) - NUEVO

#### Estado 1: Hints cambiados en overlay de hoja
**Preparacion:** rippear pagina del journal (P hold 3s)

- [ ] Overlay de hoja arrancada muestra "D: Leave here | L: Save as Letter"
- [ ] D → deja hoja en el mundo (flujo normal, sin cambios)
- [ ] L → inicia flujo de cartas (animacion fold comienza)
- [ ] ESC → restaura pagina (flujo normal, sin cambios)

#### Estado 2: Animaciones fold + envelope
**Preparacion:** presionar L en overlay de hoja arrancada

- [ ] Anim 1: hoja se dobla a la mitad (scaleY 1.0→0.5, 0.9s)
- [ ] Linea central oscura visible durante el fold (pliegue)
- [ ] Anim 2: hoja se inserta en sobre (0.6s)
- [ ] Sobre visible: rectangulo color (210,200,175) con solapa triangular
- [ ] Hoja hace lerp hacia centro del sobre con alpha 1→0
- [ ] Al terminar ambas animaciones → overlay de sobre aparece

#### Estado 3: Overlay de escritura sobre sobre
**Preparacion:** esperar a que terminen animaciones del Estado 2

- [ ] Sobre centrado w=0.5*DisplaySize.x visible
- [ ] Sello rojo (180,40,30) en esquina superior derecha del sobre
- [ ] Campos "From:" y "To:" visibles sobre el sobre
- [ ] Click en From: → campo se enfoca (fondo amarillo sutil)
- [ ] Click en To: → campo se enfoca
- [ ] Escribir en From: → texto aparece (ej: "Arthur")
- [ ] Escribir en To: → texto aparece (ej: "Mary")
- [ ] **W**: Activa modo escritura, enfoca automáticamente "To" si ninguno enfocado
- [ ] **D**: Activa modo dibujo en todo el sobre (canvas completo)
- [ ] **E** (en modo dibujo): Toggle borrador (circulo blanco visible)
- [ ] **Z/X** (en modo dibujo con borrador): Ajustar radio (8-40px)
- [ ] Dibujar con mouse en modo D → trazos visibles sobre todo el sobre
- [ ] Texto From/To siempre visible por encima de los dibujos
- [ ] Cuando From+To tienen ≥1 char → aparece "S: Save Letter"
- [ ] ESC → cancela flujo y vuelve al overlay normal de hoja

#### Estado 4: Save Letter (persistencia)
**Preparacion:** escribir From y To, presionar S

- [ ] S → guarda en myjourney/Letters/Sent/LETTER<N>/
- [ ] Verificar envelope.ini: from, to, date, originalPage, bookName
- [ ] Verificar letter.txt: contiene texto original de la pagina
- [ ] Overlay desaparece, pagina marcada como daniada (damageCount++)
- [ ] NO se creo carpeta en myjourney/Discoverables/ (es Letter, no Sheet)
- [ ] Cerrar y reabrir juego → LETTER<N> sigue en myjourney/Letters/Sent/

#### Estado 5: Polling de postboxes (coordenadas hardcodeadas)
**Preparacion:** caminar a coordenadas de postbox (ej: -1842, -1038, 180)

- [ ] Acercarse a <3m de coordenada → aparece prompt "Post Office nearby"
- [ ] Prompt muestra "Press E to open inbox" (tecla configurable)
- [ ] Cambiar InteractKey=F en INI → prompt muestra "Press F"
- [ ] Press E (o tecla configurada) → abre inbox
- [ ] Alejarse >3m → prompt desaparece

#### Estado 6: Inbox de cartas (carousel)
**Preparacion:** abrir inbox cerca de postbox

- [ ] Inbox abierto → carousel de sobres visible
- [ ] Cada sobre muestra: To: X, From: Y, fecha
- [ ] Flechas ← → → navegan entre cartas
- [ ] Contador "1 / 5" visible y actualizado
- [ ] TAB → cambia entre Sent/Received
- [ ] ENTER sobre sobre → abre carta seleccionada
- [ ] ESC → cierra inbox y devuelve control

#### Estado 7: Lectura de carta
**Preparacion:** abrir carta desde inbox (ENTER)

- [ ] Carta abierta → muestra contenido de letter.txt escalado
- [ ] Solo lectura, sin edicion posible
- [ ] ESC → vuelve al inbox
- [ ] DEL → borra carta (fs::remove_all de LETTER<N>) y vuelve al inbox
- [ ] Reabrir inbox → carta borrada ya no aparece

#### Integracion con sistemas existentes
**Preparacion:** probar sistemas existentes despues de implementar cartas

- [ ] Journal abre/cierra normalmente (J)
- [ ] CustomBooks abre/cierra normalmente (B 3s)
- [ ] Sheets rip/leave/restore funciona normalmente
- [ ] No hay conflictos de teclas entre sistemas
- [ ] Input forwarding funciona (mouse en inbox)
- [ ] Bloqueo de controles activo cuando inbox abierto

---

### 1) Hojas Arrancadas (SHEETS) - Journal

#### Fix: Custombook overlay texto - BATCH 9
**Preparacion:** custombook abierto, navegar a pagina lejana (ej: 1095), rippear

- [ ] Custombook: navegar a pagina lejana (ej: 1095) → seleccionar pagina → mantener P 3s
- [ ] Overlay de hoja arrancada aparece → ¿muestra el TEXTO de la pagina?
- [ ] El texto corresponde al contenido real de esa pagina (no esta en blanco)
- [ ] Presionar L → hoja se deja en el mundo normalmente

#### Fix: Journal ripped slot con gris y ? - BATCH 9
**Preparacion:** journal abierto, rippear una pagina, navegar a la vista

- [ ] Journal: pagina ripped → ¿slot muestra fondo GRIS oscuro (80,75,70)?
- [ ] Lineas diagonales gruesas grises sutiles visibles dentro del slot
- [ ] Signos "?" gris transparente esparcidos con jitter dentro del slot
- [ ] Todo el contenido (lineas, ?) se mantiene DENTRO del marco de la pagina (no se sale)

#### Fix: Mouse cursor oculto en overview - BATCH 9
**Preparacion:** journal abierto en vista general (overview)

- [ ] Journal abierto en overview (sin pagina seleccionada) → ¿cursor OCULTO?
- [ ] Navegar con flechas → cursor sigue OCULTO
- [ ] Seleccionar pagina con ENTER → cursor sigue OCULTO
- [ ] Cerrar journal → cursor vuelve a aparecer

#### Fix: Crouch animation al recoger sheets - BATCH 9
**Preparacion:** dejar una sheet en el mundo (L en overlay), cerrar journal, ir a la sheet

- [ ] Acercarse a sheet → presionar R → personaje camina hacia la sheet
- [ ] Al llegar → ¿personaje se AGACHA (crouch animation)?
- [ ] La animacion es visible y rapida (no lenta como antes)
- [ ] Al terminar crouch → overlay de hoja se muestra
- [ ] Probar con journal ABIERTO tambien → crouch debe funcionar igual

#### Ajuste: Efecto restaurado mas claro - BATCH 9
**Preparacion:** rippear pagina, presionar ESC para restaurar

- [ ] Pagina restaurada → color (200,190,167) para damageCount=1
- [ ] Es ~5% mas oscuro que pagina normal (210,200,175) - diferencia sutil
- [ ] Con damageCount=5 → color (170,160,140) - mas oscuro pero no extremo

#### Ajuste: Tinte nocturno 7% - BATCH 9
**Preparacion:** esperar a que sea de dia y de noche en el juego

- [ ] De dia (06:00-21:00): tinte (100,90,75,100)
- [ ] De noche (21:00-06:00): tinte (93,84,70,100) - solo 7% menos brillante
- [ ] Diferencia muy sutil entre dia y noche

#### Mejora: Damage level 5 con trozos y clipping - BATCH 9
**Preparacion:** misma pagina, ciclar rip → ESC restore 5 veces

- [ ] DamageCount=5 → ¿aparecen pequenos TROZOS oscuros de hoja faltante?
- [ ] Los trozos son pequenos (no grandes) y en posiciones aleatorias
- [ ] Las lineas de arrugas/manchas/tajos NUNCA se salen del marco de la hoja
- [ ] Todo el dano esta contenido dentro de los limites de la pagina (clipping)

---

### 2) Random page lazy loading - BATCH 7 (PENDIENTE)
**Preparacion:** abre satchel (B 3s), selecciona libro grande (ej: Biblia)

- [ ] Presionar R en satchel → ¿abre pagina aleatoria?
- [ ] La pagina aleatoria muestra texto correctamente (no en blanco)
- [ ] Lazy loading funciona desde la pagina aleatoria

---
