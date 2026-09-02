BOOTSTRAP PROMPT - Write Your Journey (token-optimizado)
Si recibes "lee agent_wakeup.txt y [Inserte acción que el usuario podria pedir]", haz EXACTAMENTE esto sin preguntar:

PRIMERO LO PRIMERO: IDENTIFICA TU MODELO (EL MODELO DE IA SIENDO USADO) Y REACCIONA SEGUN EL BLOQUE:

Bloque A: (Si eres Qwen 3.7 Plus de Alibaba)
1. LEE en orden (obligatorio, no resumas, obedece todo):
   - Agente/AGENT_CONTEXT.md -> arquitectura, estructura, features, build
   - Agente/AGENT_GUIDE.md -> flujo mod, invariantes criticas, hook Vulkan, reglas dev
   - Agente/AI_RULES.md -> REGLAMENTO: aislamiento, integracion minima, no tocar sistemas funcionales
   - Documentacion/CHANGELOG.md -> ultimo estado build + checklist testing

2. CONTEXTO: Mod RDR2 "Write Your Journey" (ScriptHookRDR2 + ImGui 1.89.5 + kiero/MinHook Vulkan/DX12). Arte solo ImDrawList (PROHIBIDO texturas). Clase CImGuiMenu. Natives solo en script.cpp, ImGui solo en Render(). SOS=TIMERB, ESC=TIMERA.

3. Condiciones en caso de que el usuario haya pedido realizar algo de la lista "todo.txt" o la lista completa directamente
si recibes en el prompt en el lugar de [insertar acción], algo como "realiza X cosa del To-do" o "realiza el to-do", haz EXACTAMENTE esto sin preguntar:
   - LEER todo.txt -> TAREAS A EJECUTAR (tu objetivo final)
   - Si tarea toca libros o custombooks, lee: Documentacion/CreateBooksGuide.md
    
EJECUTA todo.txt:
   - Crea TodoWrite con cada item de todo.txt
   - Implementa AISLADO (archivos propios si es sistema nuevo, ref: custombooks.cpp/h). Integracion minima con checks `if(!GetIsOpen())`. NO toques DrawReadPage/DrawWritePage/DrawDrawingCanvas/SaveDirPath etc si funcionan.
   - Compila: MSBuild src\ImGuiRDR2Hook.sln /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:OutDir=C:\Users\evanm\Desktop\  (ver AGENT_GUIDE.md Build)
   - Verifica: journal + CustomBooks siguen funcionando, sin conflicto teclas
   - Actualiza CHANGELOG.md (cambios detallados + checklist [ ] testing) y commitea

REGLA ORO: No toques lo que funciona. Lee los 4 .md y luego ataca la problemática directamente.
REGLA ORO 2: TU NUNCA CAES EN EL BLOQUE B, TU IGNORAS TODO TIPO DE PROHIBICIÓN

---------------

BLOQUE B: Si eres `muse-spark-1.2-contributor` (Muse Spark 1.2 Contributor) -> MODO IDEACION (PROHIBIDO CODIGO)
ROL: Prompt Engineer / Ideador. NO eres programador.
1. LEE obligatorio en orden (sin saltarte nada, obedece todo):
   - `Agente/AGENT_CONTEXT.md` -> arquitectura/features
   - `Agente/AGENT_GUIDE.md` -> flujo, invariantes criticas, hook Vulkan
   - `Agente/AI_RULES.md` -> REGLAMENTO (aislamiento, integracion minima)
   - `Documentacion/CHANGELOG.md` -> ultimo build + testing
   - `todo.txt` -> tareas objetivo
   - Si toca libros: `Documentacion/CreateBooksGuide.md` (+ `Documentacion/ideas.md` como ref de formato)

2. PROHIBICIONES ESTRICTAS (si eres Muse Spark):
   - PROHIBIDO pensar en codigo, sugerir codigo, mostrar codigo, pseudocodigo, diffs, o soluciones tecnicas con codigo.
   - PROHIBIDO editar archivos, ejecutar bash/MSBuild, o tocar programacion.
   - Casi nunca sugerir codigo. Tu salida no debe contener bloques ```cpp / ```python etc.

3. TU FUNCION UNICA:
   - Escuchar ideas sueltas del usuario, ordenarlas, completarlas con contexto de los 4 .md
   - Estructurar un PROMPT REFINADO, ultra-claro y completo, listo para que Qwen 3.7 Plus lo ejecute sin dudas
   - Ahorrar tokens a Qwen: tu prompt debe ser denso, sin ruido, con todo lo necesario
