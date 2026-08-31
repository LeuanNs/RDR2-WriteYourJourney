// =====================================================================
//  "Write your Journey" - El diario personal de Arthur Morgan (1899)
//
//  Todo el arte de esta interfaz esta dibujado con primitivas de
//  ImDrawList (AddRectFilled, AddLine, AddQuadFilled, gradientes y
//  sombras simuladas). NO se usa ninguna textura externa ni
//  ImGui::Image.
//
//  Estados:
//    - Cover : portada de cuero cerrada  (ENTER para abrir)
//    - Open  : pliego de dos paginas     (W escribir / R leer / ESC 2s cerrar)
// =====================================================================

#ifndef NOMINMAX
#define NOMINMAX // Evita que windows.h defina macros min/max
#endif

#include "menu.h"
#include "config.h"
#include "custombooks.h"
#include "Hook/Manager.h"

#include <algorithm>
#include <atomic>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

bool CImGuiMenu::sm_bMenuOpen = false;
bool CImGuiMenu::sm_bDrawMouse = false;
std::string CImGuiMenu::s_journalTitle = "Arthur's Journey";

namespace
{
	// Mitigacion de antivirus: validar ventana activa antes de leer teclado
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

	// -----------------------------------------------------------------
	//  Maquina de estados
	// -----------------------------------------------------------------
	enum class eUiState  { Cover, Open };            // Estado 1 / Estado 2
	enum class ePageMode { Read, Write, Draw };       // Sub-estado de una pagina SELECCIONADA (TODO #6 agrega Draw)
	enum class ePageFocus{ None,  Left,  Right };     // Foco de navegacion en la vista general (TODO #3)
	enum class eTextAlign{ Left, Center, Right };     // TODO #9

	eUiState   s_state      = eUiState::Cover;
	ePageMode  s_mode       = ePageMode::Read;
	float      s_fadeIn     = 0.f;           // Aparicion de la sesion
	float      s_transition = 0.f;           // 0 = portada, 1 = libro abierto
	bool       s_focusInput = false;         // Enfocar InputText al entrar en modo W
	float      s_autoSaveT  = 0.f;           // Auto-guardado periodico (red de seguridad; ver TODO #2)

	// TODO #3 - navegacion / seleccion de pagina
	ePageFocus s_navFocus     = ePageFocus::None; // resaltado (glow) en vista general
	int        s_pagePair     = 1;                // pagina IMPAR mostrada a la izquierda (1 => par 1/2)
	int        s_selectedPage = 0;                // 0 = ninguna seleccionada; si no, numero de pagina activo
	float      s_pairFlipT    = 0.f;               // 0..1 animacion de "pase de hoja"
	int        s_pairFlipDir  = 0;                 // -1 retrocede, +1 avanza, 0 sin animar

	// TODO #5 - zoom / fuente sobre la pagina seleccionada
	bool       s_zoomed         = false;
	float      s_zoomT          = 0.f;    // 0..1 animacion de camara
	bool       s_zoomUseAltFont = false;  // F: alterna fuente caligrafica <-> legible

	// TODO #9 - formato de texto avanzado (SHIFT durante escritura)
	bool       s_showFormatPanel = false;
	int        s_textSize        = 0;     // 0 = normal, 1 = titulo
	bool       s_bold            = false;
	bool       s_italic          = false;
	bool       s_underline       = false;
	bool       s_strikethrough   = false;
	int        s_alignment       = 0;     // 0 = izquierda, 1 = centro, 2 = derecha
	bool       s_shiftWasDown    = false; // TODO p2#6: debounce para SHIFT

	// TODO #6 - modo dibujo (D) y herramientas avanzadas (SHIFT)
	enum class eBrushType { Pen, Graphite, Crayon };

	struct DrawingLine
	{
		std::vector<ImVec2> points;
		ImU32 color = IM_COL32(48, 38, 30, 255);
		float thickness = 2.0f;
		eBrushType brush = eBrushType::Pen;
	};

	struct PageDrawing
	{
		std::vector<DrawingLine> lines;
		bool loaded = false;
		bool needsLegacyConvert = false;
	};

	std::unordered_map<int, PageDrawing> s_drawingCache;
	ImU32  s_drawColor        = IM_COL32(48, 38, 30, 255);
	float  s_drawThickness    = 2.0f;
	bool   s_showDrawTools    = false;
	bool   s_isDrawing        = false;
	bool   s_drawingsOnTop    = false; // TODO p2#4: control de Z-Order
	eBrushType s_currentBrush = eBrushType::Pen;
	bool   s_eraserMode       = false; // E: Goma de borrar
	float  s_eraserRadius     = 20.0f;
	constexpr float ERASER_RADIUS_MIN = 8.0f;
	constexpr float ERASER_RADIUS_MAX = 80.0f;
	constexpr float ERASER_RADIUS_STEP = 4.0f;

	bool   s_appreciatingView = false; // V: Apreciar la vista

	// TODO #11: hora del mundo (0-23), la escribe script.cpp cada frame
	// via CImGuiMenu::SetWorldHour (los natives solo corren en ese hilo).
	std::atomic<int> s_worldHour{ 12 };

	// TODO #12: capitulo actual (1..6), lo escribe script.cpp cada frame
	// via CImGuiMenu::SetChapter. Cambiar de capitulo invalida el cache
	// de paginas en memoria para que la proxima lectura venga del disco
	// correcto (myjourney/C<capitulo>/pagX.txt).
	std::atomic<int> s_chapter{ 1 };
	int s_lastChapterSeen = 1;

	std::atomic<int> s_playerHonor{ 0 };
	std::atomic<bool> s_isJohn{ false };

	std::atomic<bool>  s_escDown{ false };     // ESC mantenido (leido por el script)
	std::atomic<float> s_escProgress{ 0.f };   // Progreso 0..1 del mantener-ESC

	// -----------------------------------------------------------------
	//  Texto por pagina + guardado (TODO #1: cada pagina se guarda y
	//  carga de forma independiente para poder alinearla a su lado
	//  correcto del libro; TODO #2: guardado inmediato tras cada cambio)
	// -----------------------------------------------------------------
	constexpr size_t TEXT_BUF_SIZE = 32768;

	struct PageBuffer
	{
		char text[TEXT_BUF_SIZE] = {};
		bool loaded = false;
	};

	std::unordered_map<int, PageBuffer> s_pageCache; // numero de pagina -> buffer
	std::mutex s_fileMutex;

	namespace fs = std::filesystem;

	// TODO #12: directorio dinamico por capitulo (myjourney/C<capitulo>).
	// El capitulo lo entrega script.cpp via CImGuiMenu::SetChapter (los
	// natives que lo determinan solo pueden correr en el hilo de script).
	fs::path SaveDirPath()
	{
		const int chapter = std::max(1, s_chapter.load());
		return fs::path("myjourney") / "Myself" / ("C" + std::to_string(chapter));
	}

	fs::path PageFilePath(int page)
	{
		return SaveDirPath() / ("pag" + std::to_string(page) + ".txt");
	}

	fs::path DrawingFilePath(int page)
	{
		return SaveDirPath() / ("pag" + std::to_string(page) + "_draw.dat");
	}

	void SavePageToFile(int page, const PageBuffer& pb)
	{
		std::lock_guard<std::mutex> lock(s_fileMutex);

		std::ofstream out(PageFilePath(page), std::ios::binary | std::ios::trunc);
		if (out)
			out.write(pb.text, (std::streamsize)std::strlen(pb.text));
	}

	void LoadPageFromFile(int page, PageBuffer& pb)
	{
		std::lock_guard<std::mutex> lock(s_fileMutex);
		pb.text[0] = '\0';

		std::ifstream in(PageFilePath(page), std::ios::binary);
		if (!in)
			return;

		std::ostringstream ss;
		ss << in.rdbuf();
		std::string data = ss.str();
		if (data.size() > TEXT_BUF_SIZE - 1)
			data.resize(TEXT_BUF_SIZE - 1);

		std::memcpy(pb.text, data.data(), data.size());
		pb.text[data.size()] = '\0';
	}

	// Devuelve (y crea/carga si hace falta) el buffer de una pagina concreta.
	PageBuffer& GetPageBuffer(int page)
	{
		auto it = s_pageCache.find(page);
		if (it != s_pageCache.end())
			return it->second;

		PageBuffer& pb = s_pageCache[page];
		LoadPageFromFile(page, pb);
		pb.loaded = true;
		return pb;
	}

	void SaveAllLoadedPages()
	{
		for (auto& [page, pb] : s_pageCache)
			if (pb.loaded)
				SavePageToFile(page, pb);
	}

	// TODO #6: guardado/carga de datos de dibujo por pagina
	constexpr uint32_t DRAWING_MAGIC = 0x574A4402;

	void SaveDrawingToFile(int page, const PageDrawing& pd)
	{
		std::lock_guard<std::mutex> lock(s_fileMutex);

		std::ofstream out(DrawingFilePath(page), std::ios::binary | std::ios::trunc);
		if (!out) return;

		out.write((const char*)&DRAWING_MAGIC, sizeof(DRAWING_MAGIC));
		const uint32_t lineCount = (uint32_t)pd.lines.size();
		out.write((const char*)&lineCount, sizeof(lineCount));

		for (const auto& line : pd.lines)
		{
			out.write((const char*)&line.color, sizeof(line.color));
			out.write((const char*)&line.thickness, sizeof(line.thickness));
			out.write((const char*)&line.brush, sizeof(line.brush));
			const uint32_t ptCount = (uint32_t)line.points.size();
			out.write((const char*)&ptCount, sizeof(ptCount));
			if (ptCount > 0)
				out.write((const char*)line.points.data(), ptCount * sizeof(ImVec2));
		}
	}

	bool LoadDrawingFromFile(int page, PageDrawing& pd)
	{
		std::lock_guard<std::mutex> lock(s_fileMutex);
		pd.lines.clear();
		pd.needsLegacyConvert = false;

		std::ifstream in(DrawingFilePath(page), std::ios::binary);
		if (!in) return false;

		uint32_t magic = 0;
		in.read((char*)&magic, sizeof(magic));

		const bool isLegacy = (magic != DRAWING_MAGIC);
		if (isLegacy)
			in.seekg(0);

		uint32_t lineCount = 0;
		in.read((char*)&lineCount, sizeof(lineCount));

		for (uint32_t i = 0; i < lineCount; ++i)
		{
			DrawingLine line;
			in.read((char*)&line.color, sizeof(line.color));
			in.read((char*)&line.thickness, sizeof(line.thickness));
			if (!isLegacy)
				in.read((char*)&line.brush, sizeof(line.brush));
			uint32_t ptCount = 0;
			in.read((char*)&ptCount, sizeof(ptCount));
			if (ptCount > 0 && ptCount < 100000)
			{
				line.points.resize(ptCount);
				in.read((char*)line.points.data(), ptCount * sizeof(ImVec2));
			}
			pd.lines.push_back(std::move(line));
		}

		pd.needsLegacyConvert = isLegacy;
		return isLegacy;
	}

	PageDrawing& GetPageDrawing(int page)
	{
		auto it = s_drawingCache.find(page);
		if (it != s_drawingCache.end())
			return it->second;

		PageDrawing& pd = s_drawingCache[page];
		LoadDrawingFromFile(page, pd);
		pd.loaded = true;
		return pd;
	}

	void SaveAllLoadedDrawings()
	{
		for (auto& [page, pd] : s_drawingCache)
			if (pd.loaded)
				SaveDrawingToFile(page, pd);
	}

	// TODO #1: Pagina IMPAR (1, 3, 5...) siempre a la IZQUIERDA,
	//          pagina PAR (2, 4, 6...) siempre a la DERECHA.
	bool IsPageOnRight(int page) { return (page % 2) == 0; }

	// -----------------------------------------------------------------
	//  Fuentes
	// -----------------------------------------------------------------
	//  Para la caligrafia cursiva, carga tu .ttf al inicializar ImGui
	//  (en Hook/DX12.cpp y Hook/Vulkan.cpp, junto a AddFontDefault):
	//
	//      io.Fonts->AddFontFromFileTTF("mi_cursiva.ttf", 34.0f,
	//          nullptr, io.Fonts->GetGlyphRangesDefault());
	//
	//  La primera fuente agregada (indice 1) se usa automaticamente.
	//  Si no hay ninguna, se usa la fuente por defecto de ImGui.
	ImFont* GetJournalFont()
	{
		ImGuiIO& io = ImGui::GetIO();
		if (io.Fonts->Fonts.Size > 1)
			return io.Fonts->Fonts[1]; // MV Boli (fuente manuscrita)
		return ImGui::GetFont();
	}

	// TODO p2#3: Fuente alternativa para el toggle F en modo zoom
	ImFont* GetAltJournalFont()
	{
		ImGuiIO& io = ImGui::GetIO();
		if (io.Fonts->Fonts.Size > 2)
			return io.Fonts->Fonts[2]; // Fuente default de ImGui
		return ImGui::GetFont();
	}

	// -----------------------------------------------------------------
	//  Paleta 1899
	// -----------------------------------------------------------------
	constexpr ImU32 COL_INK       = IM_COL32(48, 38, 30, 255);    // Tinta
	constexpr ImU32 COL_INK_FADED = IM_COL32(76, 62, 48, 200);    // Tinta desvanecida
	constexpr ImU32 COL_GOLD      = IM_COL32(220, 188, 130, 255); // Pan de oro
	constexpr ImU32 COL_HELP      = IM_COL32(228, 216, 192, 150); // Textos de ayuda
	constexpr ImU32 COL_RULE      = IM_COL32(112, 86, 56, 58);    // Lineas del cuaderno

	// -----------------------------------------------------------------
	//  Utilidades de dibujo
	// -----------------------------------------------------------------
	ImU32 FadeCol(ImU32 col, float a)
	{
		if (a >= 1.f) return col;
		if (a <= 0.f) return col & IM_COL32(255, 255, 255, 0);
		const int alpha = (int)(((col >> 24) & 0xFF) * a);
		return (col & 0x00FFFFFFu) | ((ImU32)alpha << 24);
	}

	ImU32 LerpCol(ImU32 a, ImU32 b, float t)
	{
		auto ch = [&](int shift) {
			const int ca = (a >> shift) & 0xFF;
			const int cb = (b >> shift) & 0xFF;
			return (int)(ca + (cb - ca) * t);
		};
		return IM_COL32(ch(IM_COL32_R_SHIFT), ch(IM_COL32_G_SHIFT),
		                ch(IM_COL32_B_SHIFT), ch(IM_COL32_A_SHIFT));
	}

	// Pseudo-aleatorio determinista (las manchas no parpadean entre frames)
	float Rng(unsigned& seed)
	{
		seed = seed * 1664525u + 1013904223u;
		return (float)((seed >> 8) & 0xFFFFFF) / (float)0xFFFFFF;
	}

	void DrawVGradient(ImDrawList* dl, const ImVec2 mn, const ImVec2 mx,
	                   ImU32 top, ImU32 bot, int steps = 20)
	{
		if (mx.y <= mn.y || steps <= 0) return;
		const float h = (mx.y - mn.y) / (float)steps;
		for (int i = 0; i < steps; i++)
		{
			const float f = (i + 0.5f) / (float)steps;
			dl->AddRectFilled({ mn.x, mn.y + h * i }, { mx.x, mn.y + h * (i + 1) },
			                  LerpCol(top, bot, f));
		}
	}

	void DrawDashedLine(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 col,
	                    float dash, float gap, float thickness)
	{
		const float dx = b.x - a.x, dy = b.y - a.y;
		const float len = std::sqrt(dx * dx + dy * dy);
		if (len < 1.f || dash <= 0.f) return;

		const float ux = dx / len, uy = dy / len;
		float t = 0.f;
		while (t < len)
		{
			const float t1 = std::min(t + dash, len);
			dl->AddLine({ a.x + ux * t,  a.y + uy * t },
			            { a.x + ux * t1, a.y + uy * t1 }, col, thickness);
			t = t1 + gap;
		}
	}

	void DrawDashedRect(ImDrawList* dl, ImVec2 mn, ImVec2 mx, ImU32 col,
	                    float dash, float gap, float thickness)
	{
		DrawDashedLine(dl, mn, { mx.x, mn.y }, col, dash, gap, thickness);
		DrawDashedLine(dl, { mx.x, mn.y }, mx, col, dash, gap, thickness);
		DrawDashedLine(dl, mx, { mn.x, mx.y }, col, dash, gap, thickness);
		DrawDashedLine(dl, { mn.x, mx.y }, mn, col, dash, gap, thickness);
	}

	void DrawDiamond(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
	{
		dl->AddQuadFilled({ c.x, c.y - r }, { c.x + r, c.y },
		                  { c.x, c.y + r }, { c.x - r, c.y }, col);
	}

	// Ornamento horizontal: linea con rombos (floritura de 1899)
	void DrawFlourish(ImDrawList* dl, ImVec2 c, float halfW, ImU32 col)
	{
		const float gap = std::max(8.f, halfW * 0.16f);
		dl->AddLine({ c.x - halfW, c.y }, { c.x - gap, c.y }, col, 1.2f);
		dl->AddLine({ c.x + gap, c.y }, { c.x + halfW, c.y }, col, 1.2f);
		DrawDiamond(dl, c, std::max(3.f, halfW * 0.075f), col);
		DrawDiamond(dl, { c.x - halfW, c.y }, std::max(2.f, halfW * 0.04f), col);
		DrawDiamond(dl, { c.x + halfW, c.y }, std::max(2.f, halfW * 0.04f), col);
	}

	ImVec2 TextTopLeft(ImFont* f, float size, ImVec2 center, const char* text)
	{
		const ImVec2 sz = f->CalcTextSizeA(size, FLT_MAX, 0.f, text);
		return { center.x - sz.x * 0.5f, center.y - sz.y * 0.5f };
	}

	void TextCentered(ImDrawList* dl, ImFont* f, float size, ImVec2 center,
	                  ImU32 col, const char* text)
	{
		dl->AddText(f, size, TextTopLeft(f, size, center, text), col, text);
	}

	// -----------------------------------------------------------------
	//  Geometria del libro abierto
	// -----------------------------------------------------------------
	struct BookGeom
	{
		ImVec2 spreadMin, spreadMax;   // Pliego completo
		ImVec2 leftMin,   leftMax;     // Pagina izquierda
		ImVec2 rightMin,  rightMax;    // Pagina derecha
		float  lineH;                  // Separacion de las lineas del cuaderno
		float  padX, padTop, padBot;   // Margenes del area de texto
	};

	BookGeom ComputeBookGeom(const ImVec2 ds)
	{
		BookGeom g{};

		float spreadH = ds.y * 0.85f;
		float pageW   = spreadH * 0.66f;
		float spineW  = spreadH * 0.030f;
		float spreadW = pageW * 2.f + spineW;

		const float fit = (ds.x * 0.94f) / spreadW;
		if (fit < 1.f)
		{
			spreadH *= fit; pageW *= fit; spineW *= fit; spreadW *= fit;
		}

		g.spreadMin = { (ds.x - spreadW) * 0.5f, (ds.y - spreadH) * 0.5f };
		g.spreadMax = { g.spreadMin.x + spreadW, g.spreadMin.y + spreadH };
		g.leftMin   = g.spreadMin;
		g.leftMax   = { g.spreadMin.x + pageW, g.spreadMax.y };
		g.rightMin  = { g.leftMax.x + spineW, g.spreadMin.y };
		g.rightMax  = g.spreadMax;

		g.padX   = pageW * 0.085f;
		g.padTop = spreadH * 0.115f;
		g.padBot = spreadH * 0.085f;

		// La separacion de las lineas se calcula dinamicamente para que coincida
		// con el tamano real de la fuente manuscrita reducida al 70% (0.7f).
		ImFont* f = GetJournalFont();
		const float availableH = spreadH - g.padTop - g.padBot;
		const float maxLines = 20.0f;
		g.lineH = availableH / maxLines;
		// Asegurar que sea al menos el tamano de la fuente reducida + spacing
		g.lineH = std::max(g.lineH, f->FontSize * 0.7f + 4.0f);
		// Limitar maximo proporcionalmente al alto del libro (evita lineas muy separadas en 4K)
		g.lineH = std::min(g.lineH, spreadH * 0.045f);
		return g;
	}

	// TODO #1: coordenadas del rectangulo de una pagina segun su paridad,
	// dentro del pliego actualmente mostrado (g.leftMin/Max = pag. s_pagePair,
	// g.rightMin/Max = pag. s_pagePair+1).
	ImVec2 PageMin(const BookGeom& g, int page) { return IsPageOnRight(page) ? g.rightMin : g.leftMin; }
	ImVec2 PageMax(const BookGeom& g, int page) { return IsPageOnRight(page) ? g.rightMax : g.leftMax; }

	void EnsureNormalized(PageDrawing& pd, const BookGeom& g, int page)
	{
		if (!pd.needsLegacyConvert || pd.lines.empty())
			return;

		const ImVec2 pmin = PageMin(g, page);
		const ImVec2 pmax = PageMax(g, page);
		const float pw = pmax.x - pmin.x;
		const float ph = pmax.y - pmin.y;
		if (pw > 0.f && ph > 0.f)
		{
			for (auto& line : pd.lines)
				for (auto& pt : line.points)
					pt = ImVec2((pt.x - pmin.x) / pw, (pt.y - pmin.y) / ph);
		}
		pd.needsLegacyConvert = false;
	}

	// -----------------------------------------------------------------
	//  TODO #11 - Iluminacion dia/noche (segun CLOCK::GET_CLOCK_HOURS,
	//  entregado por script.cpp via CImGuiMenu::SetWorldHour).
	// -----------------------------------------------------------------
	bool IsNightHour(int hour) { return hour >= 21 || hour < 6; }

	// De noche se superpone un tinte calido/oscuro sobre el pergamino para
	// no deslumbrar; de dia no se aplica nada (se deja el pergamino crema
	// brillante original).
	ImU32 PageAmbientTint(float A)
	{
		if (!IsNightHour(s_worldHour.load()))
			return 0;
		return FadeCol(IM_COL32(120, 110, 95, 130), A);
	}

	void DrawRuledLines(ImDrawList* dl, const BookGeom& g,
	                    ImVec2 pgMin, ImVec2 pgMax, float A)
	{
		const float x0 = pgMin.x + g.padX;
		const float x1 = pgMax.x - g.padX;
		const float yEnd = pgMax.y - g.padBot;
		for (float y = pgMin.y + g.padTop + g.lineH; y <= yEnd + 0.5f; y += g.lineH)
			dl->AddLine({ x0, y }, { x1, y }, FadeCol(COL_RULE, A), 1.f);
	}

	// -----------------------------------------------------------------
	//  ESTADO 1 - Portada (cuero marron desgastado, correa y titulo)
	// -----------------------------------------------------------------
	void DrawCover(ImDrawList* dl, const ImVec2 ds, float A)
	{
		if (A <= 0.002f) return;

		float bookH = ds.y * 0.72f;
		float bookW = bookH * 0.75f;
		if (bookW > ds.x * 0.88f) { bookW = ds.x * 0.88f; bookH = bookW / 0.75f; }

		const ImVec2 bmin((ds.x - bookW) * 0.5f, (ds.y - bookH) * 0.5f);
		const ImVec2 bmax(bmin.x + bookW, bmin.y + bookH);
		const float round = std::max(6.f, bookW * 0.03f);

		// Sombra proyectada
		dl->AddRectFilled(
			{ bmin.x + bookW * 0.022f, bmin.y + bookH * 0.034f },
			{ bmax.x + bookW * 0.026f, bmax.y + bookH * 0.042f },
			FadeCol(IM_COL32(0, 0, 0, 115), A), round * 1.3f);

		// Cuero base + gradiente vertical de desgaste
		dl->AddRectFilled(bmin, bmax, FadeCol(IM_COL32(56, 37, 23, 255), A), round);
		DrawVGradient(dl,
			{ bmin.x + round * 0.7f, bmin.y + round * 0.7f },
			{ bmax.x - round * 0.7f, bmax.y - round * 0.7f },
			FadeCol(IM_COL32(76, 52, 33, 255), A),
			FadeCol(IM_COL32(45, 29, 17, 255), A), 24);

		// Manchas y desgaste (deterministas)
		dl->PushClipRect(bmin, bmax, true);
		unsigned seed = 1899u;
		for (int i = 0; i < 72; i++)
		{
			const float rx = Rng(seed), ry = Rng(seed), rr = Rng(seed);
			const ImVec2 p(bmin.x + rx * bookW, bmin.y + ry * bookH);
			const ImU32 c = (i & 1) ? IM_COL32(28, 17, 9, 26) : IM_COL32(122, 86, 54, 20);
			dl->AddCircleFilled(p, 5.f + rr * bookW * 0.055f, FadeCol(c, A), 10);
		}
		// Borde aclarado por el uso
		dl->AddRect({ bmin.x + 2.f, bmin.y + 2.f }, { bmax.x - 2.f, bmax.y - 2.f },
		            FadeCol(IM_COL32(152, 112, 72, 42), A), round - 2.f, 0, 3.f);
		dl->PopClipRect();

		// Doble marco grabado en relieve (sombra clara + trazo oscuro)
		{
			const float f = bookW * 0.085f;
			const ImVec2 f0(bmin.x + f, bmin.y + f);
			const ImVec2 f1(bmax.x - f, bmax.y - f);
			dl->AddRect({ f0.x + 1.5f, f0.y + 2.f }, { f1.x + 1.5f, f1.y + 2.f },
			            FadeCol(IM_COL32(158, 118, 78, 55), A), round * 0.5f);
			dl->AddRect(f0, f1, FadeCol(IM_COL32(16, 9, 4, 200), A), round * 0.5f, 0, 2.f);

			const float f2 = f + bookW * 0.018f;
			dl->AddRect({ bmin.x + f2, bmin.y + f2 }, { bmax.x - f2, bmax.y - f2 },
			            FadeCol(IM_COL32(16, 9, 4, 120), A), round * 0.4f, 0, 1.f);

			const float dr = std::max(2.5f, bookW * 0.016f);
			const ImU32 dc = FadeCol(IM_COL32(190, 150, 95, 150), A);
			DrawDiamond(dl, f0, dr, dc);
			DrawDiamond(dl, { f1.x, f0.y }, dr, dc);
			DrawDiamond(dl, { f0.x, f1.y }, dr, dc);
			DrawDiamond(dl, f1, dr, dc);
		}

		// Correa de cuero vertical con hebilla
		{
			const float strapW = bookW * 0.115f;
			const float strapX = bmax.x - bookW * 0.315f;

			dl->AddRectFilled({ strapX - 5.f, bmin.y - 2.f },
			                  { strapX + strapW - 5.f, bmax.y + 2.f },
			                  FadeCol(IM_COL32(0, 0, 0, 70), A));
			DrawVGradient(dl,
				{ strapX, bmin.y - 3.f }, { strapX + strapW, bmax.y + 3.f },
				FadeCol(IM_COL32(46, 28, 16, 255), A),
				FadeCol(IM_COL32(29, 17, 9, 255), A), 16);
			dl->AddLine({ strapX, bmin.y }, { strapX, bmax.y },
			            FadeCol(IM_COL32(12, 7, 3, 190), A), 2.f);
			dl->AddLine({ strapX + strapW, bmin.y }, { strapX + strapW, bmax.y },
			            FadeCol(IM_COL32(12, 7, 3, 190), A), 2.f);
			DrawDashedLine(dl, { strapX + 4.f, bmin.y + 6.f }, { strapX + 4.f, bmax.y - 6.f },
			               FadeCol(IM_COL32(200, 165, 120, 80), A),
			               bookW * 0.025f, bookW * 0.018f, 1.f);
		DrawDashedLine(dl, { strapX + strapW - 4.f, bmin.y + 6.f },
		               { strapX + strapW - 4.f, bmax.y - 6.f },
		               FadeCol(IM_COL32(200, 165, 120, 80), A),
		               bookW * 0.025f, bookW * 0.018f, 1.f);
	}

		// Titulo grabado dinamico
		ImFont* f = GetJournalFont();
		const char* title = CImGuiMenu::GetJournalTitle().c_str();
		const float titleSize = bookH * 0.085f;
		const ImVec2 tc(ds.x * 0.5f, bmin.y + bookH * 0.40f);
		const ImVec2 tp = TextTopLeft(f, titleSize, tc, title);
		dl->AddText(f, titleSize, { tp.x + 1.f, tp.y + titleSize * 0.045f },
		            FadeCol(IM_COL32(12, 6, 3, 175), A), title);           // Sombra
		dl->AddText(f, titleSize, tp, FadeCol(COL_GOLD, A), title);       // Oro

		const ImU32 flourish = FadeCol(IM_COL32(196, 162, 110, 175), A);
		DrawFlourish(dl, { ds.x * 0.5f, tc.y - titleSize * 1.40f }, bookW * 0.27f, flourish);
		DrawFlourish(dl, { ds.x * 0.5f, tc.y + titleSize * 1.40f }, bookW * 0.27f, flourish);

		TextCentered(dl, f, bookH * 0.040f, { ds.x * 0.5f, tc.y + titleSize * 2.20f },
		             FadeCol(IM_COL32(206, 178, 136, 225), A), "- 1899 -");

		// Sello del autor (parte inferior)
		TextCentered(dl, f, bookH * 0.034f, { ds.x * 0.5f, bmin.y + bookH * 0.905f },
		             FadeCol(IM_COL32(206, 178, 136, 220), A), "Blessed are those who hunger and thirst for righteousness.");
	}

	// -----------------------------------------------------------------
	//  ESTADO 2 - Libro abierto (pliego crema, reglas, lomo sombreado)
	// -----------------------------------------------------------------
	void DrawOpenBook(ImDrawList* dl, const ImVec2 ds, float A)
	{
		if (A <= 0.002f) return;

		const BookGeom g = ComputeBookGeom(ds);
		const float spreadW = g.spreadMax.x - g.spreadMin.x;
		const float spreadH = g.spreadMax.y - g.spreadMin.y;
		const float pageW   = g.leftMax.x - g.leftMin.x;
		const float spineW  = g.rightMin.x - g.leftMax.x;

		// Tapa de cuero detras de las paginas
		const float m = spreadH * 0.026f;
		const ImVec2 cmin(g.spreadMin.x - m, g.spreadMin.y - m);
		const ImVec2 cmax(g.spreadMax.x + m, g.spreadMax.y + m);
		dl->AddRectFilled({ cmin.x + m * 0.9f, cmin.y + m * 1.5f },
		                  { cmax.x + m, cmax.y + m * 1.7f },
		                  FadeCol(IM_COL32(0, 0, 0, 120), A), m);
		dl->AddRectFilled(cmin, cmax, FadeCol(IM_COL32(52, 34, 21, 255), A), m * 0.8f);
		dl->AddRect(cmin, cmax, FadeCol(IM_COL32(14, 8, 4, 220), A), m * 0.8f, 0, 2.f);

		// Grosor del bloque de paginas (hojas apiladas)
		for (int i = 3; i >= 1; --i)
		{
			const float o = i * std::max(1.6f, spreadH * 0.0022f);
			const ImU32 c = FadeCol(IM_COL32(206, 190, 158, 255), A);
			dl->AddRectFilled({ g.leftMin.x + o, g.leftMin.y + o },
			                  { g.leftMax.x + o, g.leftMax.y + o }, c);
			dl->AddRectFilled({ g.rightMin.x + o, g.rightMin.y + o },
			                  { g.rightMax.x + o, g.rightMax.y + o }, c);
		}

		// Paginas crema / pergamino
		dl->AddRectFilled(g.leftMin, g.leftMax, FadeCol(IM_COL32(234, 223, 197, 255), A));
		dl->AddRectFilled(g.rightMin, g.rightMax, FadeCol(IM_COL32(230, 218, 191, 255), A));

		// Envejecido por esquinas
		const ImU32 ageHi = FadeCol(IM_COL32(242, 233, 210, 255), A);
		const ImU32 ageLo = FadeCol(IM_COL32(216, 200, 168, 255), A);
		dl->AddRectFilledMultiColor(g.leftMin, g.leftMax, ageHi, ageHi, ageLo, ageHi);
		dl->AddRectFilledMultiColor(g.rightMin, g.rightMax, ageHi, ageHi, ageHi, ageLo);

		// Bordes tostados (gradiente hacia el exterior)
		{
			const float ew = pageW * 0.075f;
			const float eh = spreadH * 0.050f;
			const ImU32 t0 = FadeCol(IM_COL32(122, 92, 56, 64), A);
			const ImU32 t1 = FadeCol(IM_COL32(122, 92, 56, 0), A);

			dl->AddRectFilledMultiColor(g.leftMin, { g.leftMin.x + ew, g.leftMax.y }, t0, t1, t1, t0);
			dl->AddRectFilledMultiColor({ g.rightMax.x - ew, g.rightMin.y }, g.rightMax, t1, t0, t0, t1);
			dl->AddRectFilledMultiColor(g.leftMin, { g.leftMax.x, g.leftMin.y + eh }, t0, t0, t1, t1);
			dl->AddRectFilledMultiColor({ g.leftMin.x, g.leftMax.y - eh }, g.leftMax, t1, t1, t0, t0);
			dl->AddRectFilledMultiColor(g.rightMin, { g.rightMax.x, g.rightMin.y + eh }, t0, t0, t1, t1);
			dl->AddRectFilledMultiColor({ g.rightMin.x, g.rightMax.y - eh }, g.rightMax, t1, t1, t0, t0);
		}

		// Manchas de edad
		{
			unsigned seed = 777u;
			for (int i = 0; i < 26; i++)
			{
				const float rx = Rng(seed), ry = Rng(seed), rr = Rng(seed);
				const ImVec2 p(g.spreadMin.x + rx * spreadW, g.spreadMin.y + ry * spreadH);
				dl->AddCircleFilled(p, 4.f + rr * spreadH * 0.03f,
				                    FadeCol(IM_COL32(140, 108, 66, 16), A), 9);
			}
		}

		// TODO #11: tinte de atenuacion nocturno (de dia no se dibuja nada)
		if (const ImU32 tint = PageAmbientTint(A))
		{
			dl->AddRectFilled(g.leftMin, g.leftMax, tint);
			dl->AddRectFilled(g.rightMin, g.rightMax, tint);
		}

		// Lineas de cuaderno en ambas paginas
		DrawRuledLines(dl, g, g.leftMin, g.leftMax, A);
		DrawRuledLines(dl, g, g.rightMin, g.rightMax, A);

		// Lomo: sombra central para dar profundidad
		{
			dl->AddRectFilled({ g.leftMax.x, g.spreadMin.y }, { g.rightMin.x, g.spreadMax.y },
			                  FadeCol(IM_COL32(30, 19, 10, 255), A));

			const float shW = pageW * 0.15f;
			const ImU32 d0 = FadeCol(IM_COL32(36, 23, 11, 0), A);
			const ImU32 d1 = FadeCol(IM_COL32(36, 23, 11, 145), A);
			dl->AddRectFilledMultiColor({ g.leftMax.x - shW, g.spreadMin.y }, g.leftMax, d0, d1, d1, d0);
			dl->AddRectFilledMultiColor(g.rightMin, { g.rightMin.x + shW, g.spreadMax.y }, d1, d0, d0, d1);

			const float cx = (g.leftMax.x + g.rightMin.x) * 0.5f;
			dl->AddLine({ cx, g.spreadMin.y + 3.f }, { cx, g.spreadMax.y - 3.f },
			            FadeCol(IM_COL32(10, 6, 3, 170), A), 2.f);
			dl->AddLine({ cx - spineW * 0.28f, g.spreadMin.y + 6.f },
			            { cx - spineW * 0.28f, g.spreadMax.y - 6.f },
			            FadeCol(IM_COL32(180, 150, 105, 40), A), 1.f);
			dl->AddLine({ cx + spineW * 0.28f, g.spreadMin.y + 6.f },
			            { cx + spineW * 0.28f, g.spreadMax.y - 6.f },
			            FadeCol(IM_COL32(180, 150, 105, 40), A), 1.f);
		}

		// Cinta marcapaginas
		{
			const float rbW = spreadH * 0.026f;
			const float rbX = g.leftMax.x + spineW * 0.30f;
			const float rbY0 = g.spreadMin.y - m * 0.4f;
			const float rbY1 = g.spreadMin.y + spreadH * 0.30f;
			const ImVec2 pts[5] = {
				{ rbX, rbY0 }, { rbX + rbW, rbY0 }, { rbX + rbW, rbY1 },
				{ rbX + rbW * 0.5f, rbY1 - rbW * 0.85f }, { rbX, rbY1 } };
			dl->AddConvexPolyFilled(pts, 5, FadeCol(IM_COL32(116, 28, 24, 240), A));
			dl->AddPolyline(pts, 5, FadeCol(IM_COL32(84, 18, 15, 240), A),
			                ImDrawFlags_None, 1.2f);
		}

		// Cabecera caligrafica de la pagina izquierda
		ImFont* f = GetJournalFont();
		const float cx = (g.leftMin.x + g.leftMax.x) * 0.5f;
		TextCentered(dl, f, spreadH * 0.040f, { cx, g.spreadMin.y + spreadH * 0.048f },
		             FadeCol(COL_INK, A), CImGuiMenu::GetJournalTitle().c_str());
		DrawFlourish(dl, { cx, g.spreadMin.y + spreadH * 0.076f }, pageW * 0.26f,
		             FadeCol(COL_INK_FADED, A));
		TextCentered(dl, f, spreadH * 0.023f, { cx, g.spreadMin.y + spreadH * 0.096f },
		             FadeCol(COL_INK_FADED, A), "1899");

		// Numeros de pagina (TODO #1: impar a la izquierda, par a la derecha)
		{
			const std::string leftNum  = std::to_string(s_pagePair);
			const std::string rightNum = std::to_string(s_pagePair + 1);
			TextCentered(dl, f, spreadH * 0.020f, { cx, g.spreadMax.y - g.padBot * 0.45f },
			             FadeCol(COL_INK_FADED, A), leftNum.c_str());
			TextCentered(dl, f, spreadH * 0.020f,
			             { (g.rightMin.x + g.rightMax.x) * 0.5f, g.spreadMax.y - g.padBot * 0.45f },
			             FadeCol(COL_INK_FADED, A), rightNum.c_str());
		}

		// Contorno de las paginas
		dl->AddRect(g.leftMin, g.leftMax, FadeCol(IM_COL32(96, 74, 48, 90), A));
		dl->AddRect(g.rightMin, g.rightMax, FadeCol(IM_COL32(96, 74, 48, 90), A));
	}

	// -----------------------------------------------------------------
	//  Textos de ayuda (esquinas inferiores) + barra de mantener ESC
	// -----------------------------------------------------------------
	void DrawHelp(ImDrawList* dl, const ImVec2 ds, float A)
	{
		// Escala responsiva: fuente mas grande en resoluciones altas, mas pequena en bajas
		ImFont* df = ImGui::GetFont();
		const float refH = 1080.f;
		const float scaleFactor = std::clamp(ds.y / refH, 0.6f, 1.5f);
		const float scale = 1.2f * scaleFactor;
		const float fh = df->FontSize * scale;
		const float y = ds.y - fh * 1.9f;
		const float xm = std::max(ds.x * 0.018f, 10.f);

		const char* left;
		if (s_state != eUiState::Open)
			left = WJConfig::Help_Cover.c_str();
		else if (s_selectedPage == 0)
			left = WJConfig::Help_Overview.c_str();
		else if (s_zoomed)
			left = WJConfig::Help_Zoom.c_str();
		else if (s_mode == ePageMode::Draw)
			left = WJConfig::Help_Draw.c_str();
		else
			left = WJConfig::Help_Write.c_str();
		dl->AddText(df, fh, { xm, y }, FadeCol(COL_HELP, A), left);

		const char* right = "Write your Journey - 1899";
		const ImVec2 rsz = df->CalcTextSizeA(fh, FLT_MAX, 0.f, right);
		dl->AddText(df, fh, { ds.x - rsz.x - xm, y }, FadeCol(COL_HELP, A), right);
	}

	void DrawEscProgress(ImDrawList* dl, const ImVec2 ds, float A)
	{
		const float p = s_escProgress.load();
		if (p <= 0.f) return;

		ImFont* df = ImGui::GetFont();
		const float w = std::min(320.f, ds.x * 0.20f);
		const float h = 7.f;
		const ImVec2 c(ds.x * 0.5f, ds.y - df->FontSize * 4.4f);

		const char* msg = "Guardando...";
		const ImVec2 msz = df->CalcTextSizeA(df->FontSize, FLT_MAX, 0.f, msg);
		dl->AddText({ c.x - msz.x * 0.5f, c.y - h - df->FontSize * 1.2f },
		            FadeCol(COL_HELP, A), msg);

		dl->AddRectFilled({ c.x - w * 0.5f, c.y - h * 0.5f },
		                  { c.x + w * 0.5f, c.y + h * 0.5f },
		                  FadeCol(IM_COL32(20, 14, 8, 190), A), 3.f);
		if (p > 0.01f)
			dl->AddRectFilled({ c.x - w * 0.5f + 1.5f, c.y - h * 0.5f + 1.5f },
			                  { c.x - w * 0.5f + 1.5f + (w - 3.f) * p, c.y + h * 0.5f - 1.5f },
			                  FadeCol(COL_GOLD, A), 2.f);
		dl->AddRect({ c.x - w * 0.5f, c.y - h * 0.5f }, { c.x + w * 0.5f, c.y + h * 0.5f },
		            FadeCol(IM_COL32(206, 172, 126, 140), A), 3.f, 0, 1.f);
	}

	// -----------------------------------------------------------------
	//  TODO #3 - Resplandor azul de la pagina bajo foco/mouse en la
	//  vista general (sin pagina seleccionada todavia).
	// -----------------------------------------------------------------
	void DrawPageGlow(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float A, float pulse)
	{
		const int alpha = (int)(A * (130.f + 60.f * pulse));
		const ImU32 glow = IM_COL32(0, 180, 255, std::clamp(alpha, 0, 255));
		dl->AddRect({ mn.x - 3.f, mn.y - 3.f }, { mx.x + 3.f, mx.y + 3.f }, glow, 4.f, 0, 3.f);
		dl->AddRect({ mn.x - 7.f, mn.y - 7.f }, { mx.x + 7.f, mx.y + 7.f },
		            FadeCol(glow, 0.4f), 5.f, 0, 1.5f);
	}

	void DrawPageOverviewGlow(ImDrawList* dl, const BookGeom& g, float A)
	{
		if (s_navFocus == ePageFocus::None) return;

		const float pulse = 0.5f + 0.5f * std::sin((float)ImGui::GetTime() * 4.0f);
		if (s_navFocus == ePageFocus::Left)
			DrawPageGlow(dl, g.leftMin, g.leftMax, A, pulse);
		else
			DrawPageGlow(dl, g.rightMin, g.rightMax, A, pulse);
	}

	// -----------------------------------------------------------------
	//  TODO #4 - Salto de linea automatico: fuerza un '\n' cada ~15
	//  caracteres dentro de una racha continua sin espacios (evita que
	//  una palabra/cadena larga se salga del margen del cuaderno).
	// -----------------------------------------------------------------
	constexpr int kHardWrapRun = 17;

	bool ForceWrapLongRuns(char* buf, size_t bufCap)
	{
		size_t len = std::strlen(buf);
		int run = 0;
		bool changed = false;

		for (size_t i = 0; i < len; ++i)
		{
			const char c = buf[i];
			if (c == '\n' || c == ' ' || c == '\t')
			{
				run = 0;
				continue;
			}

			++run;
			if (run > kHardWrapRun)
			{
				if (len + 1 >= bufCap) // sin espacio para insertar, se corta
					break;

				std::memmove(buf + i + 1, buf + i, len - i + 1); // +1 incluye el '\0'
				buf[i] = '\n';
				++len;
				run = 0;
				changed = true;
			}
		}
		return changed;
	}

	// -----------------------------------------------------------------
	//  Ventanas ImGui para leer / escribir sobre la pagina seleccionada
	//  (cromo totalmente transparente: solo se ve la tinta)
	// -----------------------------------------------------------------
	constexpr ImGuiWindowFlags kPageWinFlags =
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

	void PushPageWindowStyle(float A)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(110, 86, 58, 130));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, IM_COL32(110, 86, 58, 170));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, IM_COL32(110, 86, 58, 200));
		ImGui::PushStyleColor(ImGuiCol_Text, FadeCol(COL_INK, A));
		ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, FadeCol(IM_COL32(150, 120, 70, 110), A));
		ImGui::PushStyleColor(ImGuiCol_Separator, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_NavHighlight, IM_COL32(0, 0, 0, 0));
	}

	void PopPageWindowStyle()
	{
		ImGui::PopStyleColor(14);
		ImGui::PopStyleVar(7);
	}

	// -----------------------------------------------------------------
	//  TODO p2#1 - Previsualizacion de contenido en vista general.
	//  Dibuja el texto y los trazos de una pagina en modo solo lectura
	//  (sin interactividad) para que el usuario vea como lucen sus
	//  paginas antes de seleccionarlas.
	// -----------------------------------------------------------------
	void DrawPagePreview(const BookGeom& g, int page, float A)
	{
		const ImVec2 pmin = PageMin(g, page);
		const ImVec2 pmax = PageMax(g, page);
		const ImVec2 tmin(pmin.x + g.padX, pmin.y + g.padTop + 2.0f);
		const ImVec2 tmax(pmax.x - g.padX, pmax.y - g.padBot);

		PageBuffer& pb = GetPageBuffer(page);
		PageDrawing& pd = GetPageDrawing(page);
		EnsureNormalized(pd, g, page);

		// Dibujar los trazos primero (si existen)
		if (!pd.lines.empty())
		{
			const float pw = pmax.x - pmin.x;
			const float ph = pmax.y - pmin.y;
			ImDrawList* dl = ImGui::GetBackgroundDrawList();
			dl->PushClipRect(pmin, pmax, true);
			for (const auto& line : pd.lines)
			{
				if (line.points.size() < 2) continue;
				for (size_t i = 1; i < line.points.size(); ++i)
				{
					const ImVec2 p1 = { pmin.x + line.points[i - 1].x * pw, pmin.y + line.points[i - 1].y * ph };
					const ImVec2 p2 = { pmin.x + line.points[i].x * pw,     pmin.y + line.points[i].y * ph };
					dl->AddLine(p1, p2, FadeCol(line.color, A * 0.7f), line.thickness);
				}
			}
			dl->PopClipRect();
		}

		// Dibujar el texto encima (si existe) - misma logica que DrawReadPage
		if (pb.text[0])
		{
			const std::string winId = "##JourneyPreview" + std::to_string(page);
			ImGui::SetNextWindowPos(tmin);
			ImGui::SetNextWindowSize({ tmax.x - tmin.x, tmax.y - tmin.y });
			PushPageWindowStyle(A);
			ImGuiWindowFlags flags = kPageWinFlags | ImGuiWindowFlags_NoMouseInputs;
			if (ImGui::Begin(winId.c_str(), nullptr, flags))
			{
				ImFont* f = GetJournalFont();
				ImGui::PushFont(f);
				ImGui::SetWindowFontScale(g.lineH / f->FontSize);
				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (tmax.x - tmin.x));
				ImGui::TextUnformatted(pb.text);
				ImGui::PopTextWrapPos();
				ImGui::PopFont();
			}
			ImGui::End();
			PopPageWindowStyle();
		}
	}

	// TODO #1: usa PageMin/PageMax segun la paridad de `page`, en vez del
	// lado derecho fijo que tenia antes.
	void DrawReadPage(const BookGeom& g, int page, float A)
	{
		const ImVec2 pmin = PageMin(g, page);
		const ImVec2 pmax = PageMax(g, page);
		const ImVec2 tmin(pmin.x + g.padX, pmin.y + g.padTop + 2.0f);
		const ImVec2 tmax(pmax.x - g.padX, pmax.y - g.padBot);

		PageBuffer& pb = GetPageBuffer(page);

		ImGui::SetNextWindowPos(tmin);
		ImGui::SetNextWindowSize({ tmax.x - tmin.x, tmax.y - tmin.y });
		PushPageWindowStyle(A);
		if (ImGui::Begin("##JourneyRead", nullptr, kPageWinFlags))
		{
			ImFont* f = GetJournalFont();
			ImGui::PushFont(f);
			ImGui::SetWindowFontScale(g.lineH / f->FontSize);
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (tmax.x - tmin.x)); // TODO #4

			if (pb.text[0])
			{
				ImGui::TextUnformatted(pb.text);
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Text, FadeCol(COL_INK_FADED, A));
				ImGui::TextUnformatted(WJConfig::Empty_Page.c_str());
				ImGui::PopStyleColor();
			}

			ImGui::PopTextWrapPos();
			ImGui::PopFont();
		}
		ImGui::End();
		PopPageWindowStyle();
	}

	void DrawWritePage(const BookGeom& g, int page, float A)
	{
		const ImVec2 pmin = PageMin(g, page);
		const ImVec2 pmax = PageMax(g, page);
		const ImVec2 tmin(pmin.x + g.padX, pmin.y + g.padTop + 2.0f);
		const ImVec2 tmax(pmax.x - g.padX, pmax.y - g.padBot);

		PageBuffer& pb = GetPageBuffer(page);

		ImGui::SetNextWindowPos(tmin);
		ImGui::SetNextWindowSize({ tmax.x - tmin.x, tmax.y - tmin.y });
		PushPageWindowStyle(A);
		if (ImGui::Begin("##JourneyWrite", nullptr, kPageWinFlags))
		{
			ImFont* f = GetJournalFont();
			ImGui::PushFont(f);
			ImGui::SetWindowFontScale(g.lineH / f->FontSize);
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (tmax.x - tmin.x)); // TODO #4

			if (s_focusInput)
			{
				ImGui::SetKeyboardFocusHere();
				s_focusInput = false;
			}

			// TODO p2#2: callback para hard-wrap en tiempo real
			auto wrapCallback = [](ImGuiInputTextCallbackData* data) -> int {
				if (data->EventFlag != ImGuiInputTextFlags_CallbackEdit)
					return 0;

				// Buscar el inicio de la ultima linea
				const char* text = data->Buf;
				const int len = data->BufTextLen;
				int lineStart = 0;
				for (int i = len - 1; i >= 0; --i)
				{
					if (text[i] == '\n')
					{
						lineStart = i + 1;
						break;
					}
				}

				// Contar caracteres sin espacios en la ultima linea
				int runLen = 0;
				for (int i = lineStart; i < len; ++i)
				{
					const char c = text[i];
					if (c == ' ' || c == '\t')
						runLen = 0;
					else
						++runLen;
				}

				// Si excede el limite, insertar un salto de linea
				if (runLen > kHardWrapRun && len + 1 < (int)data->BufSize)
				{
					data->InsertChars(len, "\n");
				}
				return 0;
			};

			const bool changed = ImGui::InputTextMultiline(
				"##page", pb.text, TEXT_BUF_SIZE, ImVec2(-1.f, -1.f),
				ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackEdit,
				wrapCallback);

			if (changed)
			{
				// TODO #4: fuerza el salto de linea en rachas largas sin espacios
				ForceWrapLongRuns(pb.text, TEXT_BUF_SIZE);
				// TODO #2: guardado inmediato tras CADA caracter introducido
				SavePageToFile(page, pb);
			}

			ImGui::PopTextWrapPos();
			ImGui::PopFont();
		}
		ImGui::End();
		PopPageWindowStyle();
	}

	// -----------------------------------------------------------------
	//  TODO #9 - Panel de formato avanzado (SHIFT durante escritura)
	//  Permite aplicar modificadores al texto: tamaño, estilos y alineación.
	// -----------------------------------------------------------------
	void DrawFormatPanel(const ImVec2 ds, float A)
	{
		if (!s_showFormatPanel)
			return;

		const float pw = ds.x * 0.22f;
		const float ph = ds.y * 0.32f;
		ImGui::SetNextWindowPos({ ds.x * 0.5f - pw * 0.5f, ds.y * 0.15f }, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize({ pw, ph }, ImGuiCond_FirstUseEver);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 12.f, 12.f });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(30, 25, 20, 240));
		ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(180, 140, 90, 180));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(234, 223, 197, 255));
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(60, 48, 35, 200));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(90, 72, 52, 220));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(120, 96, 70, 240));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(45, 36, 26, 200));
		ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(80, 64, 46, 180));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(100, 80, 58, 200));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(120, 96, 70, 220));

		if (ImGui::Begin("##FormatPanel", nullptr,
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar))
		{
			ImFont* f = ImGui::GetFont();
			ImGui::PushFont(f);

			ImGui::TextColored({ 0.95f, 0.85f, 0.7f, 1.f }, WJConfig::Format_Title.c_str());
			ImGui::Separator();
			ImGui::Spacing();

			// Tamaño de texto
			ImGui::Text("Tamaño:");
			ImGui::SameLine();
			if (ImGui::RadioButton("Normal", s_textSize == 0)) s_textSize = 0;
			ImGui::SameLine();
			if (ImGui::RadioButton("Titulo", s_textSize == 1)) s_textSize = 1;
			ImGui::Spacing();

			// Estilos
			ImGui::Text("Estilos:");
			ImGui::Checkbox("Negrita", &s_bold);
			ImGui::SameLine();
			ImGui::Checkbox("Cursiva", &s_italic);
			ImGui::SameLine();
			ImGui::Checkbox("Subrayado", &s_underline);
			ImGui::SameLine();
			ImGui::Checkbox("Tachado", &s_strikethrough);
			ImGui::Spacing();

			// Alineación
			ImGui::Text("Alineacion:");
			ImGui::SameLine();
			if (ImGui::RadioButton("Izq", s_alignment == 0)) s_alignment = 0;
			ImGui::SameLine();
			if (ImGui::RadioButton("Centro", s_alignment == 1)) s_alignment = 1;
			ImGui::SameLine();
			if (ImGui::RadioButton("Der", s_alignment == 2)) s_alignment = 2;

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Botón cerrar
			if (ImGui::Button("Cerrar (SHIFT)", { -1, 30 }))
			{
				s_showFormatPanel = false;
			}

			ImGui::PopFont();
		}
		ImGui::End();

		ImGui::PopStyleColor(10);
		ImGui::PopStyleVar(2);
	}

	// -----------------------------------------------------------------
	// TODO #5 - Modo Zoom (R): camara/enfoque sobre la pagina
	//  seleccionada, con scroll vertical para leer textos largos, y
	//  cambio de fuente (F).
	// -----------------------------------------------------------------
	void DrawZoomPage(const ImVec2 ds, int page, float A, float zoomT)
	{
		PageBuffer& pb = GetPageBuffer(page);
		PageDrawing& pd = GetPageDrawing(page);
		const BookGeom g = ComputeBookGeom(ds);
		EnsureNormalized(pd, g, page);

		// Marco ampliado centrado en pantalla; zoomT anima la aparicion.
		const float baseW = ds.x * 0.5f, baseH = ds.y * 0.78f;
		const float w = baseW * (0.75f + 0.25f * zoomT);
		const float h = baseH * (0.75f + 0.25f * zoomT);
		const ImVec2 mn{ (ds.x - w) * 0.5f, (ds.y - h) * 0.5f };
		const ImVec2 mx{ mn.x + w, mn.y + h };

		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		dl->AddRectFilled({ 0, 0 }, ds, IM_COL32(0, 0, 0, (int)(120.f * A * zoomT)));
		dl->AddRectFilled(mn, mx, FadeCol(IM_COL32(234, 223, 197, 255), A * zoomT), 4.f);
		if (const ImU32 tint = PageAmbientTint(A * zoomT))
			dl->AddRectFilled(mn, mx, tint, 4.f);
		dl->AddRect(mn, mx, FadeCol(IM_COL32(96, 74, 48, 160), A * zoomT), 4.f, 0, 2.f);

		// Dibujar trazos escalados dentro del marco de zoom
		if (!pd.lines.empty() && zoomT > 0.5f)
		{
			const ImVec2 pmin = PageMin(g, page);
			const ImVec2 pmax = PageMax(g, page);
			const float scaleX = w / (pmax.x - pmin.x);
			const float scaleY = h / (pmax.y - pmin.y);

			dl->PushClipRect(mn, mx, true);
			for (const auto& line : pd.lines)
			{
				if (line.points.size() < 2) continue;
				ImU32 col = FadeCol(line.color, A * zoomT);
				float thick = line.thickness * std::min(scaleX, scaleY);

				for (size_t i = 1; i < line.points.size(); ++i)
				{
					ImVec2 p1 = { mn.x + line.points[i - 1].x * w, mn.y + line.points[i - 1].y * h };
					ImVec2 p2 = { mn.x + line.points[i].x * w,     mn.y + line.points[i].y * h };
					dl->AddLine(p1, p2, col, thick);
				}
			}
			dl->PopClipRect();
		}

		if (zoomT < 0.98f)
			return; // esperar a que termine la animacion de camara antes de mostrar texto

		const float pad = w * 0.06f;
		const ImVec2 tmin(mn.x + pad, mn.y + pad);
		const ImVec2 tmax(mx.x - pad, mx.y - pad);

		ImGui::SetNextWindowPos(tmin);
		ImGui::SetNextWindowSize({ tmax.x - tmin.x, tmax.y - tmin.y });
		PushPageWindowStyle(A);
		ImGuiWindowFlags flags = kPageWinFlags & ~ImGuiWindowFlags_NoScrollbar & ~ImGuiWindowFlags_NoScrollWithMouse;
		if (ImGui::Begin("##JourneyZoom", nullptr, flags))
		{
			// TODO p2#3: Toggle F alterna entre MV Boli y fuente default
			ImFont* f = s_zoomUseAltFont ? GetAltJournalFont() : GetJournalFont();
			ImGui::PushFont(f);
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (tmax.x - tmin.x));
			ImGui::TextUnformatted(pb.text[0] ? pb.text
			                                   : "Las paginas estan en blanco...");
			ImGui::PopTextWrapPos();
			ImGui::PopFont();
		}
		ImGui::End();
		PopPageWindowStyle();
	}

	// -----------------------------------------------------------------
	//  TODO #6 - Modo Dibujo (D): canvas de trazos continuos sobre la
	//  pagina seleccionada, con herramientas avanzadas (SHIFT).
	// -----------------------------------------------------------------
	void DrawDrawingCanvas(const BookGeom& g, int page, float A)
	{
		PageDrawing& pd = GetPageDrawing(page);
		EnsureNormalized(pd, g, page);
		const ImVec2 pmin = PageMin(g, page);
		const ImVec2 pmax = PageMax(g, page);

		const float pw = pmax.x - pmin.x;
		const float ph = pmax.y - pmin.y;
		// TODO FASE7#2: los puntos se guardan normalizados (0..1 relativos a
		// la pagina); desnormalizarlos contra el pmin/pmax ACTUAL para que el
		// trazo sea proporcional sin importar la resolucion en la que se dibujo.
		auto ToScreen = [&](const ImVec2& p) -> ImVec2 {
			return { pmin.x + p.x * pw, pmin.y + p.y * ph };
		};

		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		dl->PushClipRect(pmin, pmax, true);

		for (const auto& line : pd.lines)
		{
			if (line.points.size() < 2) continue;

			ImU32 col = FadeCol(line.color, A);
			float thick = line.thickness;

			// Ajustar estilo segun el tipo de pincel
			if (line.brush == eBrushType::Graphite)
			{
				col = FadeCol(IM_COL32(100, 100, 100, (int)(A * 150)), 1.0f);
				thick *= 0.7f;
			}
			else if (line.brush == eBrushType::Crayon)
			{
				thick *= 1.5f;
			}

			for (size_t i = 1; i < line.points.size(); ++i)
			{
				const ImVec2 p1 = ToScreen(line.points[i - 1]);
				const ImVec2 p2 = ToScreen(line.points[i]);

				if (line.brush == eBrushType::Crayon)
				{
					// Simular textura rugosa con multiples lineas desfasadas
					for (int j = 0; j < 3; ++j)
					{
						float offset = (j - 1) * 0.5f;
						ImVec2 o1 = { p1.x + offset, p1.y + offset };
						ImVec2 o2 = { p2.x + offset, p2.y + offset };
						dl->AddLine(o1, o2, col, thick * 0.6f);
					}
				}
				else
				{
					dl->AddLine(p1, p2, col, thick);
				}
			}
		}

		dl->PopClipRect();
	}

	void DrawDrawingToolsPanel(const ImVec2 ds, float A)
	{
		if (!s_showDrawTools)
			return;

		const float pw = ds.x * 0.20f;
		const float ph = ds.y * 0.22f;
		ImGui::SetNextWindowPos({ ds.x * 0.5f - pw * 0.5f, ds.y * 0.70f }, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize({ pw, ph }, ImGuiCond_FirstUseEver);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10.f, 10.f });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(30, 25, 20, 230));
		ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(180, 140, 90, 180));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(234, 223, 197, 255));

		if (ImGui::Begin("##DrawTools", nullptr,
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar))
		{
			ImFont* f = ImGui::GetFont();
			ImGui::PushFont(f);

			ImGui::Text(WJConfig::Draw_Tools.c_str());
			ImGui::Separator();
			ImGui::Spacing();

			float col[4] = {
				(float)((s_drawColor >> IM_COL32_R_SHIFT) & 0xFF) / 255.f,
				(float)((s_drawColor >> IM_COL32_G_SHIFT) & 0xFF) / 255.f,
				(float)((s_drawColor >> IM_COL32_B_SHIFT) & 0xFF) / 255.f,
				(float)((s_drawColor >> IM_COL32_A_SHIFT) & 0xFF) / 255.f
			};
			if (ImGui::ColorEdit4("Color", col, ImGuiColorEditFlags_NoInputs))
			{
				s_drawColor = IM_COL32(
					(int)(col[0] * 255.f), (int)(col[1] * 255.f),
					(int)(col[2] * 255.f), (int)(col[3] * 255.f));
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Change ink color");

			ImGui::SliderFloat("Thickness", &s_drawThickness, 1.0f, 10.0f, "%.1f");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Adjust stroke thickness");

			ImGui::Text("Brush:");
			ImGui::SameLine();
			if (ImGui::RadioButton("Pen", s_currentBrush == eBrushType::Pen))
			{
				s_currentBrush = eBrushType::Pen;
				s_drawThickness = 2.0f;
				s_drawColor = (s_drawColor & 0x00FFFFFFu) | IM_COL32(0, 0, 0, 255);
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Sharp ink pen");
			ImGui::SameLine();
			if (ImGui::RadioButton("Graphite", s_currentBrush == eBrushType::Graphite))
			{
				s_currentBrush = eBrushType::Graphite;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Soft graphite pencil");
			ImGui::SameLine();
			if (ImGui::RadioButton("Crayon", s_currentBrush == eBrushType::Crayon))
			{
				s_currentBrush = eBrushType::Crayon;
				s_drawThickness = 5.0f;
				s_drawColor = (s_drawColor & 0x00FFFFFFu) | IM_COL32(0, 0, 0, 200);
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Rough wax crayon");

			ImGui::Checkbox("Drawings on top of text", &s_drawingsOnTop);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Toggle Z-order: drawings render above or below text");

			ImGui::PopFont();
		}
		ImGui::End();

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);
	}

	void HandleDrawingInput(const BookGeom& g, int page)
	{
		if (s_mode != ePageMode::Draw) return;

		PageDrawing& pd = GetPageDrawing(page);
		EnsureNormalized(pd, g, page);
		const ImVec2 pmin = PageMin(g, page);
		const ImVec2 pmax = PageMax(g, page);

		ImGuiIO& io = ImGui::GetIO();
		const ImVec2 mp = io.MousePos;
		const bool inPage = mp.x >= pmin.x && mp.x <= pmax.x &&
		                    mp.y >= pmin.y && mp.y <= pmax.y;

		if (ImGui::IsKeyPressed(ImGuiKey_LeftShift, false))
		{
			s_showDrawTools = !s_showDrawTools;
		}

		// E: Alternar goma de borrar
		if (ImGui::IsKeyPressed(ImGuiKey_E, false))
		{
			s_eraserMode = !s_eraserMode;
		}

		if (s_eraserMode)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
				s_eraserRadius = (s_eraserRadius + ERASER_RADIUS_STEP < ERASER_RADIUS_MAX)
					? s_eraserRadius + ERASER_RADIUS_STEP : ERASER_RADIUS_MAX;
			if (ImGui::IsKeyPressed(ImGuiKey_X, false))
				s_eraserRadius = (s_eraserRadius - ERASER_RADIUS_STEP > ERASER_RADIUS_MIN)
					? s_eraserRadius - ERASER_RADIUS_STEP : ERASER_RADIUS_MIN;

			ImDrawList* bgDl = ImGui::GetBackgroundDrawList();
			bgDl->AddCircle(mp, s_eraserRadius, IM_COL32(255, 255, 255, 220), 24, 1.5f);

			const ImVec2 fs = ImGui::GetIO().DisplaySize;
			bgDl->AddText(ImVec2(fs.x * 0.5f - 40.f, fs.y * 0.92f), IM_COL32(255, 255, 255, 200), "Z: + | X: -");

			if (inPage && !s_showDrawTools && ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				const float r2 = s_eraserRadius * s_eraserRadius;
				bool modified = false;
				std::vector<DrawingLine> newLines;
				newLines.reserve(pd.lines.size());

				for (auto& line : pd.lines)
				{
					std::vector<ImVec2> currentSeg;
					currentSeg.reserve(line.points.size());

					for (const auto& pt : line.points)
					{
						const ImVec2 abs{
							pmin.x + pt.x * (pmax.x - pmin.x),
							pmin.y + pt.y * (pmax.y - pmin.y)
						};
						const float dx = abs.x - mp.x;
						const float dy = abs.y - mp.y;
						const bool inside = (dx * dx + dy * dy < r2);

						if (!inside)
						{
							currentSeg.push_back(pt);
						}
						else
						{
							if (currentSeg.size() >= 2)
							{
								newLines.push_back({ currentSeg, line.color, line.thickness, line.brush });
							}
							currentSeg.clear();
							modified = true;
						}
					}
					if (currentSeg.size() >= 2)
					{
						newLines.push_back({ currentSeg, line.color, line.thickness, line.brush });
					}
					else if (!currentSeg.empty())
					{
						modified = true;
					}
				}

				if (modified)
				{
					pd.lines = std::move(newLines);
					SaveDrawingToFile(page, pd);
				}
			}
		}

		if (inPage && !s_showDrawTools && !s_eraserMode)
		{
			// Modo pincel: dibujar trazos
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				if (!s_isDrawing)
				{
					s_isDrawing = true;
					pd.lines.emplace_back();
					pd.lines.back().color = s_drawColor;
					pd.lines.back().thickness = s_drawThickness;
					pd.lines.back().brush = s_currentBrush;
				}
				// TODO FASE7#2: normalizar a 0..1 relativo a la pagina actual
				// (en vez de guardar pixeles absolutos) para que el trazo sea
				// 100% proporcional al libro sin importar la resolucion.
				const ImVec2 norm{
					(mp.x - pmin.x) / (pmax.x - pmin.x),
					(mp.y - pmin.y) / (pmax.y - pmin.y)
				};
				pd.lines.back().points.push_back(norm);
			}
			else if (s_isDrawing)
			{
				s_isDrawing = false;
				SaveDrawingToFile(page, pd);
			}
		}
		else if (s_isDrawing)
		{
			s_isDrawing = false;
			SaveDrawingToFile(page, pd);
		}
	}

	// Selecciona una pagina de la vista general (TODO #3). Se llama al
	// presionar ENTER sobre una pagina enfocada, o al hacer clic sobre ella.
	void SelectPage(int page)
	{
		s_selectedPage = page;
		s_mode = ePageMode::Read;
		s_zoomed = false;
		s_zoomT = 0.f;
		s_zoomUseAltFont = false;
		GetPageBuffer(page); // precarga desde disco
	}

	// Deselecciona (vuelve a la vista general). Unica forma: ESC.
	void DeselectPage()
	{
		if (s_mode == ePageMode::Write)
			SavePageToFile(s_selectedPage, GetPageBuffer(s_selectedPage)); // TODO #2
		if (s_mode == ePageMode::Draw)
			SaveAllLoadedDrawings(); // TODO #6

		s_selectedPage = 0;
		s_mode = ePageMode::Read;
		s_zoomed = false;
		s_zoomT = 0.f;
		s_showDrawTools = false;
		s_isDrawing = false;
		s_eraserMode = false;
	}

	// TODO #3: navegacion por teclado en la vista general (sin pagina
	// seleccionada). Flechas mueven el foco; una segunda pulsacion hacia
	// el mismo borde ya enfocado pasa de hoja.
	void HandleOverviewNav()
	{
		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false))
		{
			if (s_navFocus == ePageFocus::Right)
			{
				s_pagePair += 2; // pase de hoja hacia adelante
				s_pairFlipDir = 1;
				s_pairFlipT = 0.f;
			}
			else
			{
				s_navFocus = ePageFocus::Right;
			}
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
		{
			if (s_navFocus == ePageFocus::Left)
			{
				if (s_pagePair > 1)
				{
					s_pagePair -= 2; // pase de hoja hacia atras
					s_pairFlipDir = -1;
					s_pairFlipT = 0.f;
				}
			}
			else
			{
				s_navFocus = ePageFocus::Left;
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
		    ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
		{
			if (s_navFocus != ePageFocus::None)
				SelectPage(s_navFocus == ePageFocus::Left ? s_pagePair : s_pagePair + 1);
		}
	}

	// -----------------------------------------------------------------
	//  Entrada (teclado via ImGui, en el hilo de render)
	// -----------------------------------------------------------------
	void HandleInput()
	{
		s_escDown.store(ImGui::IsKeyDown(ImGuiKey_Escape));

		if (s_state == eUiState::Cover)
		{
			// Estado 1 -> Estado 2 con ENTER
			if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
			    ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
			{
				s_state = eUiState::Open;
				s_mode = ePageMode::Read;
			}
			return;
		}

		// TODO #3: sin pagina seleccionada -> vista general (navegacion)
		if (s_selectedPage == 0)
		{
			// ESC en vista general la maneja script.cpp (mantener 2s = cerrar)
			HandleOverviewNav();
			return;
		}

		// TODO #3: solo se puede leer/escribir/dibujar tras seleccionar.
		// La UNICA forma de deseleccionar es ESC.
		if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
		{
			DeselectPage();
			return;
		}

		// TODO #5: modo zoom activo sobre la pagina seleccionada
		if (s_zoomed)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_R, false))
			{
				s_zoomed = false;
				s_zoomT = 0.f;
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_F, false))
			{
				s_zoomUseAltFont = !s_zoomUseAltFont;
			}
			return;
		}

		// Mientras el InputText tiene el foco, las teclas van al papel
		if (ImGui::GetIO().WantTextInput)
			return;

		if (ImGui::IsKeyPressed(ImGuiKey_W, false))
		{
			s_mode = ePageMode::Write;
			s_focusInput = true;
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_R, false))
		{
			// TODO #5: R sobre una pagina ya seleccionada = zoom/camara
			// (si se esta escribiendo, primero vuelve a modo lectura)
			if (s_mode == ePageMode::Write)
			{
				SavePageToFile(s_selectedPage, GetPageBuffer(s_selectedPage)); // TODO #2
				s_mode = ePageMode::Read;
			}
			else if (s_mode == ePageMode::Draw)
			{
				SaveAllLoadedDrawings(); // TODO #6
				s_mode = ePageMode::Read;
			}
			else
			{
				s_zoomed = true;
				s_zoomT = 0.f;
			}
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_D, false))
		{
			// TODO #6: D entra/sale del modo dibujo
			if (s_mode == ePageMode::Draw)
			{
				SaveAllLoadedDrawings();
				s_mode = ePageMode::Read;
				s_showDrawTools = false;
			}
			else
			{
				if (s_mode == ePageMode::Write)
					SavePageToFile(s_selectedPage, GetPageBuffer(s_selectedPage));
				s_mode = ePageMode::Draw;
				GetPageDrawing(s_selectedPage);
			}
		}

		// TODO p2#6: SHIFT durante escritura abre/cierra el panel de formato
		// Usar SafeGetAsyncKeyState para validar ventana activa (mitigacion AV)
		if (s_mode == ePageMode::Write)
		{
			const bool shiftDown = (SafeGetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			if (shiftDown && !s_shiftWasDown)
			{
				s_showFormatPanel = !s_showFormatPanel;
			}
			s_shiftWasDown = shiftDown;
		}
		else
		{
			s_shiftWasDown = false;
		}
	}
}

// =====================================================================
//  API publica
// =====================================================================
void CImGuiMenu::Render()
{
	CustomBooks::HandleInput();

	if (CustomBooks::IsInventoryOpen())
	{
		CustomBooks::RenderInventory();
		return;
	}

	if (CustomBooks::IsBookOpen())
	{
		CustomBooks::RenderBook();
		return;
	}

	if (!GetIsOpen())
		return;

	// V: Apreciar la vista - no dibujar ImGui
	if (s_appreciatingView)
		return;

	ImGuiIO& io = ImGui::GetIO();
	const ImVec2 ds = io.DisplaySize;
	if (ds.x < 320.f || ds.y < 240.f)
		return;

	// TODO #12: si el capitulo cambio desde el ultimo frame (ej. el
	// jugador avanzo la historia con el diario ya abierto), guarda lo que
	// hubiera cargado del capitulo anterior y limpia el cache para que la
	// proxima pagina se lea de myjourney/C<nuevo>/.
	{
		const int chapter = std::max(1, s_chapter.load());
		if (chapter != s_lastChapterSeen)
		{
			SaveAllLoadedPages();
			SaveAllLoadedDrawings(); // TODO #6
			s_pageCache.clear();
			s_drawingCache.clear(); // TODO #6
			s_lastChapterSeen = chapter;
			if (s_selectedPage != 0)
				GetPageBuffer(s_selectedPage); // recarga desde el nuevo capitulo
		}
	}

	// TODO #3: en vista general, el mouse tambien puede enfocar una pagina
	// (tiene prioridad sobre el teclado mientras se mueve el cursor).
	if (s_state == eUiState::Open && s_selectedPage == 0 && s_transition > 0.65f)
	{
		const BookGeom gHover = ComputeBookGeom(ds);
		const ImVec2 mp = io.MousePos;
		auto inRect = [](ImVec2 p, ImVec2 mn, ImVec2 mx) {
			return p.x >= mn.x && p.x <= mx.x && p.y >= mn.y && p.y <= mx.y;
		};
		if (inRect(mp, gHover.leftMin, gHover.leftMax))
			s_navFocus = ePageFocus::Left;
		else if (inRect(mp, gHover.rightMin, gHover.rightMax))
			s_navFocus = ePageFocus::Right;

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && s_navFocus != ePageFocus::None)
			SelectPage(s_navFocus == ePageFocus::Left ? s_pagePair : s_pagePair + 1);
	}

	HandleInput();

	// Animaciones de entrada y de apertura del libro
	s_fadeIn = std::min(1.f, s_fadeIn + io.DeltaTime * 2.4f);
	const float target = (s_state == eUiState::Open) ? 1.f : 0.f;
	const float dv = io.DeltaTime * 3.0f;
	if (target > s_transition)      s_transition = std::min(target, s_transition + dv);
	else if (target < s_transition) s_transition = std::max(target, s_transition - dv);

	// TODO #3: animacion de pase de hoja (simple fundido; el layout ya
	// usa s_pagePair actualizado, esto solo suaviza el cambio visual)
	if (s_pairFlipDir != 0)
	{
		s_pairFlipT = std::min(1.f, s_pairFlipT + io.DeltaTime * 3.5f);
		if (s_pairFlipT >= 1.f)
			s_pairFlipDir = 0;
	}

	// TODO #5: animacion de camara del modo zoom
	if (s_zoomed) s_zoomT = std::min(1.f, s_zoomT + io.DeltaTime * 4.0f);
	else          s_zoomT = std::max(0.f, s_zoomT - io.DeltaTime * 4.0f);

	ImDrawList* dl = ImGui::GetBackgroundDrawList();

	// Oscuridad para centrar la atencion en el diario
	dl->AddRectFilled({ 0.f, 0.f }, ds, IM_COL32(6, 4, 3, (int)(168.f * s_fadeIn)));

	DrawCover(dl, ds, s_fadeIn * (1.f - s_transition));
	DrawOpenBook(dl, ds, s_fadeIn * s_transition);

	if (s_state == eUiState::Open && s_transition > 0.65f)
	{
		const BookGeom g = ComputeBookGeom(ds);

		if (s_selectedPage == 0)
		{
			// TODO p2#1: vista general -> previsualizacion de contenido + resplandor
			DrawPagePreview(g, s_pagePair, s_fadeIn);
			DrawPagePreview(g, s_pagePair + 1, s_fadeIn);
			DrawPageOverviewGlow(dl, g, s_fadeIn);
		}
		else if (!s_zoomed && s_zoomT <= 0.01f)
		{
			// Renderizar la pagina opuesta (compañera) como preview de solo lectura
			const int companionPage = IsPageOnRight(s_selectedPage) ? s_selectedPage - 1 : s_selectedPage + 1;
			DrawPagePreview(g, companionPage, s_fadeIn);

			// TODO p2#4: Coexistencia de texto y dibujos con control de Z-Order
			// Renderizar texto y dibujos simultaneamente segun s_drawingsOnTop
			if (s_drawingsOnTop)
			{
				// Texto primero, dibujos encima
				if (s_mode == ePageMode::Write)
				{
					DrawWritePage(g, s_selectedPage, s_fadeIn);
					DrawFormatPanel(ds, s_fadeIn);
				}
				else
					DrawReadPage(g, s_selectedPage, s_fadeIn);
				// Dibujos encima del texto
				DrawDrawingCanvas(g, s_selectedPage, s_fadeIn);
				if (s_mode == ePageMode::Draw)
				{
					HandleDrawingInput(g, s_selectedPage);
					DrawDrawingToolsPanel(ds, s_fadeIn);
				}
			}
			else
			{
				// Dibujos primero, texto encima
				DrawDrawingCanvas(g, s_selectedPage, s_fadeIn);
				if (s_mode == ePageMode::Write)
				{
					DrawWritePage(g, s_selectedPage, s_fadeIn);
					DrawFormatPanel(ds, s_fadeIn);
				}
				else
					DrawReadPage(g, s_selectedPage, s_fadeIn);
				if (s_mode == ePageMode::Draw)
				{
					HandleDrawingInput(g, s_selectedPage);
					DrawDrawingToolsPanel(ds, s_fadeIn);
				}
			}
		}
	}

	// TODO #5: overlay de zoom (por encima del pliego, con su propia animacion)
	if (s_selectedPage != 0 && (s_zoomed || s_zoomT > 0.01f))
		DrawZoomPage(ds, s_selectedPage, s_fadeIn, s_zoomT);

	DrawHelp(dl, ds, s_fadeIn);
	DrawEscProgress(dl, ds, s_fadeIn);

	// Guardado periodico de red de seguridad (el guardado principal ya
	// ocurre tras cada caracter en DrawWritePage, ver TODO #2)
	if (s_state == eUiState::Open && s_mode == ePageMode::Write && s_selectedPage != 0)
	{
		s_autoSaveT += io.DeltaTime;
		if (s_autoSaveT >= 8.f)
		{
			s_autoSaveT = 0.f;
			SavePageToFile(s_selectedPage, GetPageBuffer(s_selectedPage));
		}
	}
}

void CImGuiMenu::OpenSession()
{
	s_pageCache.clear();
	s_drawingCache.clear(); // TODO #6
	s_state = eUiState::Cover;
	s_mode = ePageMode::Read;
	s_navFocus = ePageFocus::None;
	s_pagePair = 1;
	s_selectedPage = 0;
	s_pairFlipT = 0.f;
	s_pairFlipDir = 0;
	s_zoomed = false;
	s_zoomT = 0.f;
	s_zoomUseAltFont = false;
	s_showDrawTools = false; // TODO #6
	s_isDrawing = false;     // TODO #6
	s_eraserMode = false;    // E: Goma de borrar
	s_fadeIn = 0.f;
	s_transition = 0.f;
	s_focusInput = false;
	s_autoSaveT = 0.f;
	s_escProgress.store(0.f);
	s_lastChapterSeen = std::max(1, s_chapter.load());
	GetPageBuffer(1); // precarga la primera pagina del capitulo actual
	CustomBooks::Init();
	SetIsOpen(true);
	SetShouldDrawMouse(true);
}

void CImGuiMenu::CloseSession()
{
	SaveAllLoadedPages(); // TODO #2: red de seguridad final antes de ocultar la UI
	SaveAllLoadedDrawings(); // TODO #6
	SetIsOpen(false);
	SetShouldDrawMouse(false);
	s_escDown.store(false);
	s_escProgress.store(0.f);
}

void CImGuiMenu::SaveText()
{
	SaveAllLoadedPages();
}

bool CImGuiMenu::IsEscDown()
{
	return s_escDown.load();
}

void CImGuiMenu::SetEscHoldProgress(float p)
{
	s_escProgress.store(p);
}

float CImGuiMenu::GetEscHoldProgress()
{
	return s_escProgress.load();
}

bool CImGuiMenu::IsWriteMode()
{
	return GetIsOpen() && s_state == eUiState::Open &&
	       s_selectedPage != 0 && s_mode == ePageMode::Write;
}

bool CImGuiMenu::IsAppreciatingView()
{
	return GetIsOpen() && s_appreciatingView;
}

void CImGuiMenu::SetAppreciatingView(bool enabled)
{
	s_appreciatingView = enabled;
}

void CImGuiMenu::ToggleAppreciatingView()
{
	s_appreciatingView = !s_appreciatingView;
}

void CImGuiMenu::SetWorldHour(int hour)
{
	s_worldHour.store(std::clamp(hour, 0, 23));
}

void CImGuiMenu::SetChapter(int chapter)
{
	s_chapter.store(std::max(1, chapter));
}

void CImGuiMenu::SetPlayerHonor(int honor)
{
	s_playerHonor.store(honor);
}

void CImGuiMenu::SetIsJohn(bool isJohn)
{
	s_isJohn.store(isJohn);
}

void CImGuiMenu::SetJournalTitle(const std::string& title)
{
	s_journalTitle = title;
}
