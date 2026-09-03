// =====================================================================
//  "Write your Journey" - logica de script
//
//  Y            : saca el diario y abre la UI
//  ENTER        : abre el diario (Estado 1 -> Estado 2)   [en menu.cpp]
//  W / R        : escribir / leer                         [en menu.cpp]
//  Mantener ESC : 2000ms (SETTIMERA/TIMERA) -> guarda, oculta la UI
//                 y libera controles
//  SOS          : mantener Y 10s fuerza el cierre total (failsafe)
// =====================================================================

#ifndef NOMINMAX
#define NOMINMAX // Evita que windows.h defina macros min/max
#endif

#include "script.h"
#include "keyboard.h"
#include "menu.h"
#include "custombooks.h"
#include "sheets.h"
#include "config.h"
#include "Hook/Manager.h"

#include <algorithm>

namespace
{
	// -----------------------------------------------------------------
	//  Diccionario y clips oficiales del diario de Arthur
	//  TODO p2#7: Usar animaciones descubiertas en RDR3
	// -----------------------------------------------------------------
	constexpr const char* JOURNAL_DICT      = "mech_inventory@item@fallbacks@journaljournal_w15_8h20_6@base";
	constexpr const char* ANIM_UNHOLSTER    = "base_loop";
	constexpr const char* ANIM_HOLSTER      = "base_loop";
	constexpr const char* ANIM_HOLD_LOOP    = "base_loop";
	constexpr const char* ANIM_WRITING_LOOP = "base_loop";

	// eScriptedAnimFlags (RDR2)
	// https://github.com/Halen84/RDR3-Native-Flags-And-Enums/tree/main/eScriptedAnimFlags
	constexpr int AF_LOOPING           = (1 << 0);
	constexpr int AF_HOLD_LAST_FRAME   = (1 << 1);
	constexpr int AF_NOT_INTERRUPTABLE = (1 << 2);
	constexpr int AF_UPPERBODY         = (1 << 3);
	constexpr int AF_SECONDARY         = (1 << 4);

	constexpr int UNHOLSTER_MS = 3000; // Espera tras sacar el diario
	constexpr int HOLSTER_MS   = 2000; // Espera tras guardar el diario
	constexpr int ESC_HOLD_MS  = 3000; // ESC mantenido 3s para cierre
	constexpr int SOS_HOLD_MS  = 10000; // SOS: Y mantenido 10s para desatascarse

	// TODO #10: Modo Pánico - 5 toques de ESC en < 2 segundos
	constexpr int ESC_PANIC_COUNT = 5;
	constexpr int ESC_PANIC_WINDOW_MS = 2000;

	constexpr bool USE_JOURNAL_ANIMS = false;

	bool s_active     = false; // Sesion de diario activa (controles bloqueados)
	bool s_escHolding = false;
	bool s_sosHolding = false;
	int  s_lastAnim   = -1;    // -1 ninguno, 0 = sosteniendo, 1 = escribiendo

	DWORD s_keyDownStart  = 0;
	bool  s_keyTracking   = false;
	constexpr int OPEN_HOLD_MS = 3000;

	// TODO #10: tracking de toques de ESC para modo pánico
	int  s_escPressCount = 0;
	int  s_escLastPressTime = 0;

	inline Ped PlayerPed() { return PLAYER::PLAYER_PED_ID(); }

	bool GameHasFocus()
	{
		HWND gameHwnd = CImGuiHookManager::GetGameWindow();
		if (!gameHwnd)
			return false;
		return GetForegroundWindow() == gameHwnd;
	}

	short SafeGetAsyncKeyState(int vKey)
	{
		if (!GameHasFocus())
			return 0;
		return GetAsyncKeyState(vKey);
	}

	bool PlayerCanUseJournal()
	{
		const Player ply = PLAYER::PLAYER_ID();
		return PLAYER::IS_PLAYER_PLAYING(ply) &&
		       !PLAYER::IS_PLAYER_DEAD(ply) &&
		       !HUD::IS_PAUSE_MENU_ACTIVE();
	}

	bool LoadAnimDict(const char* dict, int timeoutMs = 5000)
	{
		STREAMING::REQUEST_ANIM_DICT(dict);
		BUILTIN::SETTIMERA(0);
		while (!STREAMING::HAS_ANIM_DICT_LOADED(dict))
		{
			if (BUILTIN::TIMERA() >= timeoutMs)
				break;
			WAIT(0);
		}
		return STREAMING::HAS_ANIM_DICT_LOADED(dict) != FALSE;
	}

	void PlayJournalAnim(const char* anim, int flags, int durationMs)
	{
		TASK::TASK_PLAY_ANIM(PlayerPed(), JOURNAL_DICT, anim,
		                     2.0f, -2.0f, durationMs, flags,
		                     0.0f, FALSE, 0, FALSE, nullptr, FALSE);
	}

	// Bloqueo total por frame: sin movimiento, sin HUD, camara congelada
	void LockControlsThisFrame()
	{
		PAD::DISABLE_ALL_CONTROL_ACTIONS(0);
		HUD::HIDE_HUD_AND_RADAR_THIS_FRAME();
		CAM::_FREEZE_GAMEPLAY_CAM_THIS_FRAME();
	}

	// Modo "Apreciar la vista": bloquea todo excepto camara y cambio de perspectiva
	void AppreciateViewControlsThisFrame()
	{
		PAD::DISABLE_ALL_CONTROL_ACTIONS(0);
		HUD::HIDE_HUD_AND_RADAR_THIS_FRAME();
		// Habilitar solo controles de camara (look X/Y) y cambio de perspectiva (V)
		PAD::ENABLE_CONTROL_ACTION(0, 1, TRUE);  // INPUT_LOOK_LR
		PAD::ENABLE_CONTROL_ACTION(0, 2, TRUE);  // INPUT_LOOK_UD
		PAD::ENABLE_CONTROL_ACTION(0, 199, TRUE); // INPUT_TOGGLE_FP_VIEW (V)
	}

	// Deteccion de V y ESC para salir del modo "Apreciar la vista"
	// (movido aqui porque ImGui no procesa input cuando s_appreciatingView es true)
	void HandleAppreciateViewExit()
	{
		if (!CImGuiMenu::IsAppreciatingView())
			return;

		const bool vDown = (SafeGetAsyncKeyState('V') & 0x0001) != 0;
		const bool escDown = (SafeGetAsyncKeyState(VK_ESCAPE) & 0x0001) != 0;

		if (vDown || escDown)
		{
			CImGuiMenu::SetAppreciatingView(false);
		}
	}

	// TODO FASE7#1: calcula y aplica el titulo del diario segun el INI o el
	// modelo del jugador. Se llama ANTES de OpenSession() para evitar que el
	// primer frame renderizado muestre el titulo por defecto ("Arthur's
	// Journey") mientras el hilo de script todavia no lo actualizo.
	void UpdateJournalTitle()
	{
		std::string title;
		if (!WJConfig::MyName.empty())
		{
			title = WJConfig::MyName + "'s Journey";
		}
		else
		{
			const Hash playerModel = ENTITY::GET_ENTITY_MODEL(PLAYER::PLAYER_PED_ID());
			const Hash arthurHash = MISC::GET_HASH_KEY("player_zero");
			const Hash johnHash = MISC::GET_HASH_KEY("player_three");

			if (playerModel == arthurHash)
				title = "Arthur's Journey";
			else if (playerModel == johnHash)
				title = "John's Journey";
			else
				title = "My Journey";
		}
		CImGuiMenu::SetJournalTitle(title);
	}

	// Cede el hilo durante `ms` manteniendo el bloqueo de controles
	// (el hook de Present sigue dibujando ImGui en el hilo de render)
	void WaitLocked(DWORD ms)
	{
		const DWORD end = GetTickCount() + ms;
		do
		{
			LockControlsThisFrame();
			WAIT(0);
		} while (GetTickCount() < end);
	}

	// Alterna entre el loop de sostener el diario y el de escribir
	void UpdateHoldingAnim()
	{
		if (!USE_JOURNAL_ANIMS)
			return;

		const int want = CImGuiMenu::IsWriteMode() ? 1 : 0;
		if (want == s_lastAnim)
			return;
		s_lastAnim = want;

		if (STREAMING::HAS_ANIM_DICT_LOADED(JOURNAL_DICT))
			PlayJournalAnim(want ? ANIM_WRITING_LOOP : ANIM_HOLD_LOOP,
			                AF_LOOPING | AF_UPPERBODY | AF_SECONDARY, -1);
	}

	// Tecla Y: sacar el diario -> UI (Estado 1 - Portada)
	void OpenJournal()
	{
		s_active = true;
		LockControlsThisFrame();

		UpdateJournalTitle(); // TODO FASE7#1: calcular antes de que la UI se vuelva visible

		if (USE_JOURNAL_ANIMS)
		{
			if (LoadAnimDict(JOURNAL_DICT))
			{
				PlayJournalAnim(ANIM_UNHOLSTER,
				                AF_NOT_INTERRUPTABLE | AF_UPPERBODY | AF_SECONDARY,
				                UNHOLSTER_MS);
				WaitLocked(UNHOLSTER_MS);

				s_lastAnim = 0;
				PlayJournalAnim(ANIM_HOLD_LOOP,
				                AF_LOOPING | AF_UPPERBODY | AF_SECONDARY, -1);
			}
			else
			{
				WaitLocked(1200); // Failsafe: abrir igualmente sin animacion
			}
		}

		CImGuiMenu::OpenSession(); // Carga myjourney/C1/pag1.txt y muestra la portada
	}

	// TODO #10: Mantener ESC 5s: guardar, ocultar UI, guardar el diario y liberar
	void CloseJournal()
	{
		s_escHolding = false;
		s_lastAnim = -1;

		CImGuiMenu::CloseSession(); // Oculta ImGui
		CImGuiMenu::SaveText();     // myjourney/C1/pag1.txt

		if (USE_JOURNAL_ANIMS)
		{
			if (LoadAnimDict(JOURNAL_DICT, 2000))
				PlayJournalAnim(ANIM_HOLSTER,
				                AF_NOT_INTERRUPTABLE | AF_UPPERBODY | AF_SECONDARY,
				                HOLSTER_MS);

			WaitLocked(HOLSTER_MS);

			// Limpia las animaciones y devuelve el control al jugador
			const Ped ped = PlayerPed();
			TASK::STOP_ANIM_TASK(ped, JOURNAL_DICT, ANIM_HOLSTER, 2.0f);
			TASK::STOP_ANIM_TASK(ped, JOURNAL_DICT, ANIM_HOLD_LOOP, 2.0f);
			TASK::STOP_ANIM_TASK(ped, JOURNAL_DICT, ANIM_WRITING_LOOP, 2.0f);
			TASK::CLEAR_PED_TASKS(ped, TRUE, FALSE);
		}

		s_active = false;
	}

	// Cierre sin animaciones (muerte, pausa, UI cerrada externamente...)
	void ForceCloseJournal()
	{
		s_escHolding = false;
		s_lastAnim = -1;
		CImGuiMenu::CloseSession();
		CImGuiMenu::SaveText();
		s_active = false;
	}

	// Deteccion de mantener ESC con los temporizadores nativos.
	// La tecla se lee con GetAsyncKeyState en ESTE hilo: es absoluto y no
	// depende de ImGui ni del WndProc (si el overlay no dibuja, el puente
	// IsEscDown() nunca se actualizaria y el jugador quedaria atrapado).
	// TODO #10: tambien detecta modo pánico (5 toques en < 2s)
	void HandleEscHold()
	{
		if (HUD::IS_PAUSE_MENU_ACTIVE())
		{
			s_escHolding = false;
			CImGuiMenu::SetEscHoldProgress(0.f);
			return;
		}

		const bool escDown = (SafeGetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
		const bool escJustPressed = (SafeGetAsyncKeyState(VK_ESCAPE) & 0x0001) != 0;

		// TODO #10: tracking de toques rápidos para modo pánico
		if (escJustPressed)
		{
			const int now = GetTickCount();
			if (now - s_escLastPressTime < ESC_PANIC_WINDOW_MS)
			{
				s_escPressCount++;
				if (s_escPressCount >= ESC_PANIC_COUNT)
				{
					// Modo pánico activado: cierre inmediato
					s_escPressCount = 0;
					s_escLastPressTime = 0;
					ForceCloseJournal();
					return;
				}
			}
			else
			{
				s_escPressCount = 1;
				s_escLastPressTime = now;
			}
		}

		if (escDown)
		{
			if (!s_escHolding)
			{
				s_escHolding = true;
				BUILTIN::SETTIMERA(0);
			}

			const int t = BUILTIN::TIMERA();
			CImGuiMenu::SetEscHoldProgress(std::min(1.f, t / (float)ESC_HOLD_MS));

			if (t >= ESC_HOLD_MS)
				CloseJournal();
		}
		else if (s_escHolding)
		{
			s_escHolding = false;
			CImGuiMenu::SetEscHoldProgress(0.f);
		}
	}

	// SOS (failsafe): mantener Y durante 10 segundos fuerza el cierre total
	// del diario y devuelve el control al instante, sin validaciones.
	void HandleSos()
	{
		const int vk = WJConfig::GetVKKey();
		if (SafeGetAsyncKeyState(vk) & 0x8000)
		{
			if (!s_sosHolding)
			{
				s_sosHolding = true;
				BUILTIN::SETTIMERB(0); // TIMERB reservado para el SOS (TIMERA es del ESC)
			}

			if (BUILTIN::TIMERB() >= SOS_HOLD_MS)
			{
				s_sosHolding = false;
				ResetKeyState(vk); // Evita que al soltar la tecla se reabra el diario
				ForceCloseJournal();
			}
		}
		else
		{
			s_sosHolding = false;
		}
	}

	// TODO #7: Interrupcion inmediata por dano recibido.
	// Si el jugador sufre dano mientras el diario esta abierto, guarda y cierra.
	void HandleDamageInterrupt()
	{
		const Ped ped = PlayerPed();
		// Verifica si el jugador esta herido (salud bajo umbral de injury)
		if (PED::IS_PED_INJURED(ped) || PED::IS_PED_FATALLY_INJURED(ped))
		{
			ForceCloseJournal();
		}
	}
}

void main()
{
	constexpr int RELOAD_HOLD_MS = 5000;
	bool s_reloadHolding = false;
	DWORD s_reloadHoldStart = 0;

	while (true)
	{
		bool rDown = (SafeGetAsyncKeyState('R') & 0x8000) != 0;
		bool pDown = (SafeGetAsyncKeyState('P') & 0x8000) != 0;
		
		if (rDown && pDown)
		{
			if (!s_reloadHolding)
			{
				s_reloadHolding = true;
				s_reloadHoldStart = GetTickCount();
			}
			else if (GetTickCount() - s_reloadHoldStart >= RELOAD_HOLD_MS)
			{
				WJConfig::Load();
				if (s_active)
					ForceCloseJournal();
				s_reloadHolding = false;
			}
		}
		else
		{
			s_reloadHolding = false;
		}

		// Bloqueo de controles para satchel/libros del mod (independiente del journal)
		if (CustomBooks::IsInventoryOpen() || CustomBooks::IsBookOpen())
		{
			LockControlsThisFrame();

			if (!GameHasFocus())
			{
				CustomBooks::CloseInventory();
				CustomBooks::CloseBook();
				WAIT(0);
				continue;
			}

			WAIT(0);
			continue;
		}

		if (Sheets::IsShowingOverlay() && !s_active)
		{
			LockControlsThisFrame();

			if (!GameHasFocus())
			{
				Sheets::RestorePage();
				WAIT(0);
				continue;
			}

			WAIT(0);
			continue;
		}

		if (Sheets::IsWalkingToSheet())
		{
			const Ped ped = PlayerPed();
			const Vector3 pos = ENTITY::GET_ENTITY_COORDS(ped, TRUE, TRUE);
			Sheets::SetPlayerCoords(pos.x, pos.y, pos.z);

			float dxInput = PAD::GET_CONTROL_NORMAL(0, 1) - PAD::GET_CONTROL_NORMAL(0, 0);
			float dyInput = PAD::GET_CONTROL_NORMAL(0, 3) - PAD::GET_CONTROL_NORMAL(0, 2);
			if (dxInput < -0.1f || dxInput > 0.1f || dyInput < -0.1f || dyInput > 0.1f)
			{
				Sheets::CancelWalk();
				TASK::CLEAR_PED_TASKS(ped, TRUE, FALSE);
				WAIT(0);
				continue;
			}

			if (Sheets::UpdateWalk(pos.x, pos.y, pos.z))
			{
				TASK::CLEAR_PED_TASKS(ped, TRUE, FALSE);
				Sheets::StartCrouch();
				if (STREAMING::HAS_ANIM_DICT_LOADED("amb_rest@world_human_bottle_pickup@male_a@base") == FALSE)
				{
					STREAMING::REQUEST_ANIM_DICT("amb_rest@world_human_bottle_pickup@male_a@base");
					BUILTIN::SETTIMERA(0);
					while (STREAMING::HAS_ANIM_DICT_LOADED("amb_rest@world_human_bottle_pickup@male_a@base") == FALSE && BUILTIN::TIMERA() < 1500)
						WAIT(0);
				}
				if (STREAMING::HAS_ANIM_DICT_LOADED("amb_rest@world_human_bottle_pickup@male_a@base"))
					TASK::TASK_PLAY_ANIM(ped, "amb_rest@world_human_bottle_pickup@male_a@base", "base", 2.0f, -2.0f, 1500, AF_HOLD_LAST_FRAME, 0.0f, FALSE, 0, FALSE, nullptr, FALSE);

				while (!Sheets::UpdateCrouch())
				{
					LockControlsThisFrame();
					WAIT(0);
				}

				TASK::CLEAR_PED_TASKS(ped, TRUE, FALSE);
				Sheets::TryPickupSheet();

				if (Sheets::IsShowingOverlay())
				{
					while (Sheets::IsShowingOverlay())
					{
						LockControlsThisFrame();
						if (!GameHasFocus())
						{
							Sheets::DismissOverlay();
							break;
						}
						WAIT(0);
					}
				}
			}
			else
			{
				LockControlsThisFrame();
			}
			WAIT(0);
			continue;
		}

		if (WJConfig::CustomBooksEnabled && !s_active && PlayerCanUseJournal())
		{
			const Ped ped = PlayerPed();
			const Vector3 pos = ENTITY::GET_ENTITY_COORDS(ped, TRUE, TRUE);
			CustomBooks::UpdatePickupPrompt(pos.x, pos.y, pos.z);
			Sheets::SetPlayerCoords(pos.x, pos.y, pos.z);
			Sheets::UpdatePickupPrompt(pos.x, pos.y, pos.z);

			if (CustomBooks::IsNearPickup() && (SafeGetAsyncKeyState('E') & 0x0001) != 0)
			{
				CustomBooks::TryPickupBook(pos.x, pos.y, pos.z);
			}

			if (Sheets::IsNearPickup())
			{
				bool eDown = (SafeGetAsyncKeyState('E') & 0x8000) != 0;
				bool eJustPressed = (SafeGetAsyncKeyState('E') & 0x0001) != 0;

				if (eJustPressed && !Sheets::IsEHoldActive())
				{
					Sheets::StartEHold();
				}

				if (Sheets::IsEHoldActive())
				{
					if (!eDown)
					{
						Sheets::CancelEHold();
					}
					else
					{
						Sheets::UpdatePickupPrompt(pos.x, pos.y, pos.z);
						if (!Sheets::IsNearPickup())
							Sheets::CancelEHold();
					}
				}

				if (Sheets::IsEHoldComplete())
				{
					Sheets::StartWalkToSheet();
					float sx, sy, sz;
					Sheets::GetNearbySheetCoords(sx, sy, sz);
					TASK::TASK_GO_STRAIGHT_TO_COORD(ped, sx, sy, sz, 1.0f, -1, 0.0f, 0.5f, 0);
				}
			}
		}
		else if (s_active && PlayerCanUseJournal())
		{
			const Ped ped = PlayerPed();
			const Vector3 pos = ENTITY::GET_ENTITY_COORDS(ped, TRUE, TRUE);
			Sheets::SetPlayerCoords(pos.x, pos.y, pos.z);
			if (Sheets::UpdateWalk(pos.x, pos.y, pos.z))
			{
				Sheets::TryPickupSheet();
			}
		}

		if (!s_active)
		{
			if (WJConfig::BlessingValid && PlayerCanUseJournal())
			{
				const int vk = WJConfig::GetVKKey();
				const bool keyDown = (SafeGetAsyncKeyState(vk) & 0x8000) != 0;

				if (keyDown)
				{
					if (!s_keyTracking)
					{
						s_keyTracking = true;
						s_keyDownStart = GetTickCount();

						if (WJConfig::InstantOpen == 1)
							OpenJournal();
					}
					else if (WJConfig::InstantOpen == 0)
					{
						if (GetTickCount() - s_keyDownStart >= (DWORD)OPEN_HOLD_MS)
							OpenJournal();
					}
				}
				else
				{
					s_keyTracking = false;
				}
			}
		}
		else
		{
			// Bucle constante de bloqueo mientras el diario esta en uso
			if (CImGuiMenu::IsAppreciatingView())
			{
				AppreciateViewControlsThisFrame();
				HandleAppreciateViewExit();
			}
			else
			{
				LockControlsThisFrame();

				// V: Apreciar la vista (solo si el diario esta abierto y en pagina)
				if (CImGuiMenu::GetIsOpen() && (SafeGetAsyncKeyState('V') & 0x0001) != 0)
				{
					CImGuiMenu::ToggleAppreciatingView();
				}
			}

			// Failsafe contra Alt+Tab: si el juego pierde el foco, cerrar inmediatamente
			if (!GameHasFocus())
			{
				ForceCloseJournal();
				WAIT(0);
				continue;
			}

			// SOS: salida de emergencia en cualquier momento (Y 10s)
			HandleSos();

			if (s_active) // El SOS pudo haber cerrado ya la sesion
			{
				if (PLAYER::IS_PLAYER_DEAD(PLAYER::PLAYER_ID()) ||
				    !CImGuiMenu::GetIsOpen())
				{
					ForceCloseJournal();
				}
				else
				{
					UpdateHoldingAnim();
					HandleEscHold();
					HandleDamageInterrupt(); // TODO #7: cierra si el jugador recibe dano
					// TODO #11: actualiza la hora del mundo para el tinte día/noche
					CImGuiMenu::SetWorldHour(CLOCK::GET_CLOCK_HOURS());
					// TODO #12: actualiza el capítulo actual para el directorio dinámico
					// (usando una nativa de la campaña, si está disponible; por ahora fija en 1)
					CImGuiMenu::SetChapter(1);

					UpdateJournalTitle(); // TODO FASE7#1: red de seguridad si el modelo cambia a mitad de sesion
				}
			}
		}

		WAIT(0);
	}
}

void ScriptMain()
{
	main();
}

#pragma warning(disable:28159)
void WaitAndRender(unsigned ms)
{
	DWORD time = GetTickCount() + ms;
	bool waited = false;
	while (GetTickCount() < time || !waited)
	{
		WAIT(0);

		// This doesn't really work that well (see #2 on GitHub)
		switch (CImGuiHookManager::GetHookType())
		{
			case eVULKAN:
				CImGuiHookManager::GetVulkan().Present();
				break;
			case eDX12:
				CImGuiHookManager::GetDX12().Present();
				break;
			default: break;
		}

		waited = true;
	}
}
#pragma warning(default:28159)
