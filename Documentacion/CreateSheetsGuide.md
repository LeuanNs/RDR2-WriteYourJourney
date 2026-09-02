# Guia para Crear Hojas Arrancadas (Sheets / Discoverables)

Esta guia explica como crear hojas arrancadas personalizadas para el mod "Write Your Journey" sin necesidad de modificar el codigo fuente. Las hojas arrancadas son paginas que el jugador puede escribir/dibujar en su Journal o CustomBooks, arrancar, y dejar fisicamente en el mundo del juego para que otros jugadores (o el mismo) las encuentren.

## Estructura de Carpetas

Cada hoja debe estar en su propia carpeta dentro de:
```
myjourney/Discoverables/SHEET<N>/
```

Donde `<N>` es un numero unico incremental (1, 2, 3, ... 99, 100, etc).

La carpeta debe contener:
- `location.ini` - Ubicacion y configuracion de la hoja en el mundo
- `sheet.txt` - Texto de la hoja (contenido escrito)
- `sheet_draw.dat` - Trazos de dibujo (opcional, formato binario)

## Archivo location.ini

### Seccion [Location]

| Campo | Descripcion | Ejemplo |
|-------|-------------|---------|
| `X` | Coordenada X del mundo | `X=-1234.5` |
| `Y` | Coordenada Y del mundo | `Y=567.8` |
| `Z` | Coordenada Z del mundo | `Z=100.0` |
| `PickupRadius` | Radio de recogida en metros | `PickupRadius=10.0` |
| `PickupMessage` | Mensaje al estar cerca | `PickupMessage=Presiona E para recoger la hoja` |
| `Author` | Autor de la hoja | `Author=Arthur Morgan` |
| `Source` | Origen de la hoja | `Source=Journal` o `Source=CustomBook:MiLibro` |

## Archivo sheet.txt

Contiene el texto completo de la hoja arrancada. Es texto plano, cada linea se muestra tal cual en el overlay de la hoja.

```
Querido diario,

Hoy encontre algo increible en las montanas.
Un viejo cofre enterrado bajo el arbol de roble...

- Arthur, 1899
```

## Archivo sheet_draw.dat

Archivo binario con los trazos de dibujo de la hoja. Formato identico a `pagX_draw.dat` del journal:

- Magic number: `0x574A4402` (4 bytes)
- Numero de lineas: `uint32_t`
- Por cada linea:
  - Color: `ImU32` (4 bytes, formato RGBA)
  - Grosor: `float` (4 bytes)
  - Tipo de pincel: `int` (4 bytes, 0=Pen, 1=Graphite, 2=Crayon)
  - Numero de puntos: `uint32_t` (4 bytes)
  - Puntos: array de `ImVec2` (8 bytes cada uno, x e y normalizados 0..1)

**Nota:** Este archivo es opcional. Si no existe, la hoja solo tendra texto.

## Como Crear Sheets Manualmente (Easter Eggs)

### Paso 1: Elegir ubicacion

1. Abre el juego y ve a la ubicacion deseada
2. Usa un mod de coordenadas o la consola para obtener X, Y, Z
3. Anota las coordenadas

### Paso 2: Crear carpeta

```
myjourney/Discoverables/SHEET99/
```

El numero (99 en este ejemplo) debe ser unico. No reutilices numeros de sheets existentes.

### Paso 3: Crear location.ini

```ini
[Location]
X=-1234.5
Y=567.8
Z=100.0
PickupRadius=10.0
PickupMessage=Presiona E para recoger la nota vieja
Author=Un desconocido
Source=EasterEgg
```

### Paso 4: Crear sheet.txt

```
Si lees esto, significa que encontraste mi secreto.
Busca bajo el tercer arbol al norte del campamento.
El tesoro es tuyo.

- Un viejo amigo
```

### Paso 5: (Opcional) Crear sheet_draw.dat

Si quieres incluir dibujos, necesitas un editor binario o un script que genere el archivo con el formato correcto. Alternativamente, puedes:

1. Escribir/dibujar en una pagina del journal
2. Arrancar la pagina (P hold 3s)
3. Dejarla en el mundo (L)
4. Mover la carpeta SHEET<N> generada a la ubicacion deseada
5. Editar el `location.ini` con las coordenadas correctas

## Como Crear Sheets Desde el Juego

### Desde el Journal

1. Abre el journal (tecla J configurable)
2. Navega a una pagina con texto o dibujo
3. Mantener P durante 3 segundos (barra de progreso)
4. Animacion de hoja saliendo del libro
5. Overlay de hoja arrancada aparece
6. Presionar L para dejar la hoja en tu ubicacion actual
7. Se crea automaticamente `myjourney/Discoverables/SHEET<N>/` con todos los archivos

### Desde CustomBooks

El proceso es identico pero la hoja conserva el contenido de la pagina del libro custom.

## Comportamiento en el Juego

### Descubrir una Sheet

1. El jugador se acerca a las coordenadas (dentro de `PickupRadius`, default 10m)
2. Aparece el mensaje: "Hay una hoja arrancada cerca" + "Press E to pick it up"
3. Al presionar E, se abre el overlay mostrando el contenido de la hoja
4. El overlay muestra texto y dibujos con bordes rasgados (estilo pagina arrancada)
5. ESC para cerrar el overlay

### Persistencia

- Las sheets no se borran al recogerlas (pueden releerse)
- Las sheets generadas por el jugador (rip + leave) persisten entre sesiones
- Las paginas ripped del journal persisten en `myjourney/ripped_pages.ini`
- Los numeros de SHEET no se reutilizan (SHEET1, SHEET2, ... incremental)

## Ejemplo Completo: Nota de Tesoro

### Estructura
```
myjourney/Discoverables/SHEET42/
  location.ini
  sheet.txt
```

### location.ini
```ini
[Location]
X=-1842.0
Y=-1038.0
Z=180.0
PickupRadius=8.0
PickupMessage=Hay una nota clavada en el arbol
Author=Jeb
Source=EasterEgg
```

### sheet.txt
```
Para el aventurero que encuentre esto:

Cava tres pasos al este de la gran roca,
bajo la sombra al atardecer.

La recompensa sera tuya.

- Jeb, 1899
```

### Resultado en el juego

- El jugador se acerca a (-1842, -1038, 180)
- Ve: "Hay una nota clavada en el arbol"
- Presiona E
- Lee la nota con bordes rasgados
- Puede seguir las pistas para encontrar el tesoro

## Ejemplo Completo: Carta de Amor

### Estructura
```
myjourney/Discoverables/SHEET7/
  location.ini
  sheet.txt
  sheet_draw.dat
```

### location.ini
```ini
[Location]
X=500.0
Y=1200.0
Z=250.0
PickupRadius=5.0
PickupMessage=Una carta doblada en el suelo
Author=Mary
Source=Journal
```

### sheet.txt
```
Mi querido Arthur,

Te extrano cada dia. Las noches son frias sin ti.
Por favor, vuelve a casa pronto.

Con todo mi amor,
Mary
```

### sheet_draw.dat

(Incluye un pequeno corazon dibujado a mano)

## Localizacion de Mensajes

En `WriteYourJourney.ini`, seccion `[Localization]`:

```ini
[Localization]
Sheet_RipHint=P: Rip Page
RippingProgress=Ripping page...
Sheet_LeaveHint=L: Leave Page here
Sheet_ReadHint=R: Read
Sheet_RestoreHint=Add page back
Sheet_CloseHint=Close
Sheet_Nearby=Hay una hoja arrancada cerca
Sheet_PressE=Press E to pick it up
```

Todos los textos son traducibles. Cambia los valores para personalizar los mensajes.

## Notas Importantes

1. **No modificar codigo**: Todo se configura desde los archivos INI y TXT
2. **Numeros unicos**: Cada SHEET<N> debe tener un numero unico, no reutilizar
3. **Coordenadas precisas**: Usa un mod de coordenadas para obtener X/Y/Z exactos
4. **Radio de recogida**: Default 10m, ajustar segun necesidad (5m para notas pequenas, 15m para carteles grandes)
5. **Resolucion independiente**: Los dibujos se guardan normalizados (0..1), se ven bien en cualquier resolucion
6. **Escaneo automatico**: Las sheets se escanean al iniciar sesion del journal
7. **Persistencia**: Las sheets no se borran, pueden releerse cuantas veces quieras

## Troubleshooting

**La sheet no aparece en el mundo:**
- Verificar que existe `location.ini` con seccion `[Location]`
- Verificar que X, Y, Z son numeros validos (sin comas, con punto decimal)
- Verificar que existe `sheet.txt` (aunque este vacio)
- Reiniciar el journal (cerrar y abrir) para re-escanear

**El mensaje de pickup no aparece:**
- Verificar que `PickupRadius` es suficientemente grande (probar con 20.0)
- Verificar que las coordenadas son correctas
- Acercarse mas al punto exacto

**El texto no se muestra:**
- Verificar que `sheet.txt` existe y no esta vacio
- Verificar que el archivo esta en encoding UTF-8 o ASCII
- Verificar que no hay caracteres especiales no soportados

**El dibujo no se muestra:**
- Verificar que `sheet_draw.dat` existe
- Verificar que el formato binario es correcto (magic number 0x574A4402)
- Alternativa: crear la sheet desde el juego (rip + leave) para generar el archivo automaticamente

**Quiero borrar una sheet:**
- Simplemente eliminar la carpeta `myjourney/Discoverables/SHEET<N>/`
- Reiniciar el journal para que desaparezca del mundo

**Quiero mover una sheet a otra ubicacion:**
- Editar `location.ini` y cambiar X, Y, Z
- Reiniciar el journal para aplicar los cambios

## Ideas Creativas

- **Pistas de tesoro**: Dejar notas con coordenadas o acertijos
- **Cartas de NPCs**: Crear easter eggs con cartas de personajes del juego
- **Diarios perdidos**: Hojas con fragmentos de historias
- **Mapas dibujados**: Usar sheet_draw.dat para dibujar mapas del tesoro
- **Mensajes secretos**: Dejar notas en lugares escondidos del mapa
- **Historia colaborativa**: Dejar hojas para que otros jugadores continuen la historia

---

*Made with love By Leuan... May god bless you all*
