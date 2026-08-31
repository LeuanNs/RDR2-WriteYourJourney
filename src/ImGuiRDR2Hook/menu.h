#pragma once
#include "script.h"
#include "imgui/imgui.h"
#include <string>
#include <atomic>

// =====================================================================
//  "Write your Journey" - El diario de Arthur Morgan
//  Clase de la interfaz. El nombre se mantiene como CImGuiMenu para
//  conservar la compatibilidad con Hook/DX12.cpp, Hook/Vulkan.cpp y
//  Hook/Win32.cpp.
// =====================================================================
class CImGuiMenu
{
private:
	static bool sm_bMenuOpen;   // La UI del diario esta visible
	static bool sm_bDrawMouse;  // Dibujar el cursor de ImGui
	static std::string s_journalTitle;
	static std::atomic<float> s_reloadMessageTimer;

public:
	// Llamado cada frame desde el hook de Present (DX12/Vulkan)
	static void Render();

	// Ciclo de vida de la sesion (llamado desde el hilo de script)
	static void OpenSession();   // Carga pag1.txt y muestra la portada (Estado 1)
	static void CloseSession();  // Oculta la UI por completo
	static void SaveText();      // Guarda el texto en myjourney/C1/pag1.txt

	// Puente ESC: ImGui detecta la tecla (hilo de render) y el script
	// mide los 2000ms con SETTIMERA/TIMERA
	static bool  IsEscDown();
	static void  SetEscHoldProgress(float p); // 0..1 para dibujar la barra
	static float GetEscHoldProgress();

	// Modo escritura activo (usado por el script para la animacion de escribir)
	static bool IsWriteMode();

	// V: Apreciar la vista (usado por el script para bloquear controles excepto camara)
	static bool IsAppreciatingView();
	static void SetAppreciatingView(bool enabled);
	static void ToggleAppreciatingView();

	// TODO #11: hora del mundo (0-23) para tinte día/noche
	static void SetWorldHour(int hour);

	// TODO #12: capítulo actual de la historia para directorio dinámico
	static void SetChapter(int chapter);

	static void SetJournalTitle(const std::string& title);
	static const std::string& GetJournalTitle() { return s_journalTitle; }

	static void ShowReloadMessage();

	static inline bool GetIsOpen() { return sm_bMenuOpen; }
	static inline void SetIsOpen(bool open) { sm_bMenuOpen = open; }
	static inline bool ShouldDrawMouse() { return sm_bDrawMouse; }
	static inline void SetShouldDrawMouse(bool draw) { sm_bDrawMouse = draw; }
};
