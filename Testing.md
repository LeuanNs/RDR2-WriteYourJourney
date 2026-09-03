# Testing - Write Your Journey - Guia Rapida para Leuan
> Ultima Build: 2026-09-03 (Batch 9 - Overlay Text Fix + Visual Improvements + Crouch Animation)
> Como usar: entra al juego, ve seccion por seccion. Marca [x] si OK, deja [ ] si falla y anota al lado que viste.

---

### 0) Hojas Arrancadas (SHEETS) - Journal

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

### 1) Random page lazy loading - BATCH 7 (PENDIENTE)
**Preparacion:** abre satchel (B 3s), selecciona libro grande (ej: Biblia)

- [ ] Presionar R en satchel → ¿abre pagina aleatoria?
- [ ] La pagina aleatoria muestra texto correctamente (no en blanco)
- [ ] Lazy loading funciona desde la pagina aleatoria

---
