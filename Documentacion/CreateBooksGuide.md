# Guía para Crear Libros Custom

Esta guía explica cómo crear libros personalizados para el mod "Write Your Journey" sin necesidad de modificar el código fuente.

## Estructura de Carpetas

Cada libro debe estar en su propia carpeta dentro de:
```
MyJourney/Books/[NombreInterno]/
```

La carpeta debe contener:
- `body.txt` - El contenido del libro
- `config.ini` - Configuración del libro
- `index.json` - Índice de capítulos (opcional)

## Archivo config.ini

### Sección [Metadata]

| Campo | Descripción | Ejemplo |
|-------|-------------|---------|
| `DisplayTitle` | Título mostrado en la portada | `DisplayTitle=La Biblia NT 1858` |
| `Author` | Autor del libro | `Author=Reina Valera` |
| `Year` | Año de publicación | `Year=1858` |
| `Category` | Categoría para búsqueda | `Category=Religion` |
| `isOwned` | 1 = ya poseído, 0 = encontrable | `isOwned=1` |

### Sección [Rendering]

| Campo | Descripción | Valores |
|-------|-------------|---------|
| `CoverColorRGB` | Color de la portada (R,G,B) | `CoverColorRGB=80,40,20` |
| `FontType` | Tipo de fuente | `FontType=1` |
| `FontSizeOverride` | Ajuste de tamaño (-2 a 4) | `FontSizeOverride=-2` |
| `InkColor` | Color de tinta | `InkColor=Sepia` o `Black` o `FadedBlue` |
| `TextAlignment` | Alineación | `0=Izquierda`, `1=Centro`, `2=Derecha` |
| `PreserveLineBreaks` | Mantener saltos de línea | `1` o `0` |

### Sección [Navigation]

| Campo | Descripción | Valores |
|-------|-------------|---------|
| `AllowsOpenRandomPage` | Permitir abrir en página aleatoria | `1` o `0` |
| `HasIndex` | Tiene índice de capítulos | `1` o `0` |
| `AutoOrderPages` | Ordenar páginas automáticamente | `1` o `0` |

### Sección [Location] (Para libros encontrables)

| Campo | Descripción | Ejemplo |
|-------|-------------|---------|
| `Findable` | 1 = encontrable en coordenadas | `Findable=1` |
| `X` | Coordenada X del mundo | `X=-1842.0` |
| `Y` | Coordenada Y del mundo | `Y=-1038.0` |
| `Z` | Coordenada Z del mundo | `Z=180.0` |
| `PickupRadius` | Radio de recogida (metros) | `PickupRadius=10.0` |
| `PickupMessage` | Mensaje al estar cerca | `PickupMessage=Presiona E para obtener...` |

## Archivo index.json

Formato JSON con los capítulos del libro:

```json
{
  "chapters": [
    {"title": "Mateo", "line": 6},
    {"title": "Marcos", "line": 2166},
    {"title": "Lucas", "line": 3509}
  ]
}
```

- `title`: Nombre del capítulo
- `line`: Número de línea donde inicia (0-indexed)

## Archivo body.txt

Contiene el texto completo del libro. Cada línea del archivo corresponde a una línea del libro.

## Tipos de Libros

### 1. Libros Poseídos (isOwned=1)

Aparecen inmediatamente en el satchel del jugador.

```ini
[Metadata]
DisplayTitle=Mi Libro
Author=Autor
isOwned=1

[Location]
Findable=0
```

### 2. Libros Encontrables (isOwned=0, Findable=1)

El jugador debe encontrarlos en coordenadas específicas del mapa.

```ini
[Metadata]
DisplayTitle=Libro Perdido
Author=Autor Antiguo
isOwned=0

[Location]
Findable=1
X=-1842.0
Y=-1038.0
Z=180.0
PickupRadius=10.0
PickupMessage=Presiona E para obtener el libro antiguo
```

**Comportamiento:**
- Cuando el jugador se acerca a las coordenadas, aparece el mensaje
- Al presionar E, el libro se marca como `isOwned=1` en el config.ini
- El libro aparece permanentemente en el satchel

## Cómo Encontrar Coordenadas

1. Abre el juego y ve a la ubicación deseada
2. Usa un mod de coordenadas o la consola para obtener X, Y, Z
3. Anota las coordenadas en el config.ini

## Ejemplo Completo

### config.ini
```ini
[Metadata]
DisplayTitle=Diario del Explorador
Author=John Doe
Year=1899
Category=Aventura
isOwned=0

[Rendering]
CoverColorRGB=100,60,30
FontType=1
FontSizeOverride=0
InkColor=Sepia
TextAlignment=0
PreserveLineBreaks=1

[Navigation]
AllowsOpenRandomPage=1
HasIndex=1
AutoOrderPages=1

[Location]
Findable=1
X=-500.0
Y=800.0
Z=250.0
PickupRadius=8.0
PickupMessage=Presiona E para tomar el diario viejo
```

### index.json
```json
{
  "chapters": [
    {"title": "Capítulo I - El Inicio", "line": 0},
    {"title": "Capítulo II - El Viaje", "line": 50},
    {"title": "Capítulo III - El Descubrimiento", "line": 120}
  ]
}
```

### body.txt
```
Era una mañana fría cuando partí...

[Línea 50]
El viaje fue largo y peligroso...

[Línea 120]
Finalmente encontré lo que buscaba...
```

## Notas Importantes

1. **No modificar código**: Todo se configura desde los archivos INI y JSON
2. **Libros grandes**: El sistema usa lazy loading para libros con +3000 caracteres
3. **Resolución independiente**: Los dibujos se guardan normalizados
4. **Localización**: Los mensajes pueden traducirse en WriteYourJourney.ini

## Localización de Mensajes

En `WriteYourJourney.ini`, sección `[Localization]`:

```ini
[Localization]
CB_NavHint=<- -> : Navegar   |   K: Bookmark   |   ESC : Cerrar
CB_SearchHint=Buscar: título, autor, categoría...
CB_NoBooks=No hay libros en MyJourney/Books/
CB_NoMatch=No hay libros que coincidan
CB_SatchelOpening=Abriendo Satchel...
BookmarkSaved=Bookmark Guardado
BookmarkRemoved=Bookmark Eliminado
```

## Troubleshooting

**El libro no aparece en el satchel:**
- Verificar que `isOwned=1` en config.ini
- Verificar que existe config.ini en la carpeta del libro

**El libro no aparece en el mundo:**
- Verificar que `Findable=1` y `isOwned=0`
- Verificar que las coordenadas son correctas
- Aumentar `PickupRadius` si es necesario

**El índice no funciona:**
- Verificar que index.json tiene formato JSON válido
- Verificar que los números de línea son correctos

---

*Made with love By Leuan... May god bless you all*
