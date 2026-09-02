#define NOMINMAX
#include "sheets.h"
#include "config.h"
#include "imgui/imgui.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <cstring>
#include <unordered_set>
#include <unordered_map>

namespace fs = std::filesystem;

namespace Sheets
{
	static constexpr uint32_t SHEET_DRAWING_MAGIC = 0x574A4402;
	static constexpr float RIP_HOLD_TIME = 3.0f;
	static constexpr float RIP_ANIM_DURATION = 0.9f;

	static std::atomic<float> s_playerX{ 0.f };
	static std::atomic<float> s_playerY{ 0.f };
	static std::atomic<float> s_playerZ{ 0.f };

	static bool s_ripping = false;
	static float s_ripProgress = 0.f;
	static RippedSheetCache s_ripCache;

	static bool s_ripAnimating = false;
	static float s_ripAnimT = 0.f;

	static bool s_showingOverlay = false;
	static bool s_viewingDiscoverable = false;
	static int s_viewingSheetId = -1;
	static RippedSheetCache s_overlayCache;

	static std::unordered_set<int> s_rippedJournalPages;
	static std::unordered_map<std::string, std::unordered_set<int>> s_rippedCustomPages;

	static std::vector<DiscoverableSheet> s_discoverableSheets;
	static int s_nearbySheetId = -1;
	static bool s_showPickupPrompt = false;
	static int s_nextSheetId = 1;
	static bool s_scanned = false;

	static std::mutex s_sheetsMutex;

	static fs::path GetDiscoverablesDir()
	{
		return fs::path(WJConfig::GetModuleDir()) / "myjourney" / "Discoverables";
	}

	static fs::path GetRippedPagesPath()
	{
		return fs::path(WJConfig::GetModuleDir()) / "myjourney" / "ripped_pages.ini";
	}

	static std::string Trim(const std::string& s)
	{
		size_t a = s.find_first_not_of(" \t\r\n");
		if (a == std::string::npos) return "";
		size_t b = s.find_last_not_of(" \t\r\n");
		return s.substr(a, b - a + 1);
	}

	static void SaveDrawingToFile(const fs::path& path, const SheetDrawing& sd)
	{
		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		if (!out) return;
		out.write((const char*)&SHEET_DRAWING_MAGIC, sizeof(SHEET_DRAWING_MAGIC));
		const uint32_t lineCount = (uint32_t)sd.lines.size();
		out.write((const char*)&lineCount, sizeof(lineCount));
		for (const auto& line : sd.lines)
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

	static SheetDrawing LoadDrawingFromFile(const fs::path& path)
	{
		SheetDrawing sd;
		std::ifstream in(path, std::ios::binary);
		if (!in) return sd;
		uint32_t magic = 0;
		in.read((char*)&magic, sizeof(magic));
		if (magic != SHEET_DRAWING_MAGIC) return sd;
		uint32_t lineCount = 0;
		in.read((char*)&lineCount, sizeof(lineCount));
		for (uint32_t i = 0; i < lineCount; ++i)
		{
			SheetDrawingLine line;
			in.read((char*)&line.color, sizeof(line.color));
			in.read((char*)&line.thickness, sizeof(line.thickness));
			in.read((char*)&line.brush, sizeof(line.brush));
			uint32_t ptCount = 0;
			in.read((char*)&ptCount, sizeof(ptCount));
			if (ptCount > 0 && ptCount < 100000)
			{
				line.points.resize(ptCount);
				in.read((char*)line.points.data(), ptCount * sizeof(ImVec2));
			}
			sd.lines.push_back(std::move(line));
		}
		return sd;
	}

	static void SaveRippedPagesIndex()
	{
		std::ofstream out(GetRippedPagesPath());
		if (!out) return;
		out << "[Journal]\n";
		for (int p : s_rippedJournalPages)
			out << "Page=" << p << "\n";
		for (const auto& [bookName, pages] : s_rippedCustomPages)
		{
			out << "\n[Custom:" << bookName << "]\n";
			for (int p : pages)
				out << "Page=" << p << "\n";
		}
	}

	static void LoadRippedPagesIndex()
	{
		s_rippedJournalPages.clear();
		s_rippedCustomPages.clear();
		std::ifstream in(GetRippedPagesPath());
		if (!in) return;
		std::string line;
		std::string currentBook;
		bool inJournal = false;
		while (std::getline(in, line))
		{
			line = Trim(line);
			if (line.empty()) continue;
			if (line == "[Journal]") { inJournal = true; currentBook.clear(); continue; }
			if (line.substr(0, 8) == "[Custom:")
			{
				inJournal = false;
				size_t end = line.find(']', 8);
				if (end != std::string::npos)
					currentBook = line.substr(8, end - 8);
				continue;
			}
			if (line.substr(0, 5) == "Page=")
			{
				int pg = std::atoi(line.substr(5).c_str());
				if (inJournal)
					s_rippedJournalPages.insert(pg);
				else if (!currentBook.empty())
					s_rippedCustomPages[currentBook].insert(pg);
			}
		}
	}

	static void ParseLocationIni(const fs::path& path, DiscoverableSheet& sheet)
	{
		std::ifstream f(path);
		if (!f) return;
		std::string line;
		while (std::getline(f, line))
		{
			line = Trim(line);
			if (line.empty() || line[0] == '[' || line[0] == ';') continue;
			size_t eq = line.find('=');
			if (eq == std::string::npos) continue;
			std::string key = Trim(line.substr(0, eq));
			std::string val = Trim(line.substr(eq + 1));
			if (key == "X") sheet.x = std::stof(val);
			else if (key == "Y") sheet.y = std::stof(val);
			else if (key == "Z") sheet.z = std::stof(val);
			else if (key == "PickupRadius") sheet.radius = std::stof(val);
			else if (key == "PickupMessage") sheet.pickupMessage = val;
			else if (key == "Author") sheet.author = val;
			else if (key == "Source") sheet.source = val;
		}
	}

	void ScanSheets()
	{
		std::lock_guard<std::mutex> lock(s_sheetsMutex);
		s_discoverableSheets.clear();
		s_nextSheetId = 1;

		fs::path dir = GetDiscoverablesDir();
		if (!fs::exists(dir)) return;

		for (const auto& entry : fs::directory_iterator(dir))
		{
			if (!entry.is_directory()) continue;
			std::string name = entry.path().filename().string();
			if (name.substr(0, 5) != "SHEET") continue;

			int id = 0;
			try { id = std::stoi(name.substr(5)); }
			catch (...) { continue; }

			if (id >= s_nextSheetId) s_nextSheetId = id + 1;

			fs::path locPath = entry.path() / "location.ini";
			if (!fs::exists(locPath)) continue;

			DiscoverableSheet sheet;
			sheet.id = id;
			ParseLocationIni(locPath, sheet);

			fs::path textPath = entry.path() / "sheet.txt";
			if (fs::exists(textPath))
			{
				std::ifstream tf(textPath);
				if (tf)
				{
					std::ostringstream ss;
					ss << tf.rdbuf();
					sheet.text = ss.str();
				}
			}

			fs::path drawPath = entry.path() / "sheet_draw.dat";
			if (fs::exists(drawPath))
				sheet.drawing = LoadDrawingFromFile(drawPath);

			if (sheet.pickupMessage.empty())
				sheet.pickupMessage = WJConfig::Sheet_PressE;

			s_discoverableSheets.push_back(std::move(sheet));
		}
		s_scanned = true;
	}

	void Init()
	{
		LoadRippedPagesIndex();
		ScanSheets();
	}

	void SetPlayerCoords(float x, float y, float z)
	{
		s_playerX.store(x);
		s_playerY.store(y);
		s_playerZ.store(z);
	}

	float GetRipProgress() { return s_ripProgress; }
	void SetRipProgress(float p) { s_ripProgress = p; }
	int GetRipSourcePage() { return s_ripCache.sourcePage; }
	bool GetRipFromJournal() { return s_ripCache.fromJournal; }
	const std::string& GetRipBookName() { return s_ripCache.bookName; }

	bool IsShowingOverlay() { return s_showingOverlay; }
	bool IsRipping() { return s_ripping; }
	bool IsAnimating() { return s_ripAnimating; }

	bool IsPageRipped(int page, bool isJournal, const std::string& bookName)
	{
		if (isJournal)
			return s_rippedJournalPages.count(page) > 0;
		auto it = s_rippedCustomPages.find(bookName);
		if (it == s_rippedCustomPages.end()) return false;
		return it->second.count(page) > 0;
	}

	void StartRipPage(const std::string& text, const SheetDrawing& drawing, int page, bool fromJournal, const std::string& bookName, int chapter)
	{
		if (s_ripping || s_ripAnimating || s_showingOverlay) return;
		s_ripCache.text = text;
		s_ripCache.drawing = drawing;
		s_ripCache.sourcePage = page;
		s_ripCache.fromJournal = fromJournal;
		s_ripCache.bookName = bookName;
		s_ripCache.chapter = chapter;
		s_ripping = true;
		s_ripProgress = 0.f;
	}

	void ConfirmRip()
	{
		if (!s_ripping && !s_ripAnimating) return;

		int page = s_ripCache.sourcePage;
		if (s_ripCache.fromJournal)
			s_rippedJournalPages.insert(page);
		else
			s_rippedCustomPages[s_ripCache.bookName].insert(page);

		SaveRippedPagesIndex();

		s_ripping = false;
		s_ripAnimating = false;
		s_ripAnimT = 0.f;
		s_ripProgress = 0.f;

		s_overlayCache = s_ripCache;
		s_showingOverlay = true;
		s_viewingDiscoverable = false;
		s_viewingSheetId = -1;
	}

	void CancelRip()
	{
		s_ripping = false;
		s_ripAnimating = false;
		s_ripAnimT = 0.f;
		s_ripProgress = 0.f;
		s_ripCache = RippedSheetCache();
	}

	void RestorePage()
	{
		if (!s_showingOverlay || s_viewingDiscoverable) return;

		int page = s_overlayCache.sourcePage;
		if (s_overlayCache.fromJournal)
			s_rippedJournalPages.erase(page);
		else
		{
			auto it = s_rippedCustomPages.find(s_overlayCache.bookName);
			if (it != s_rippedCustomPages.end())
				it->second.erase(page);
		}

		SaveRippedPagesIndex();
		s_showingOverlay = false;
		s_overlayCache = RippedSheetCache();
	}

	void LeaveSheetAtPlayer()
	{
		if (!s_showingOverlay) return;

		float px = s_playerX.load();
		float py = s_playerY.load();
		float pz = s_playerZ.load();

		fs::path dir = GetDiscoverablesDir();
		fs::create_directories(dir);

		int newId = s_nextSheetId++;
		std::string folderName = "SHEET" + std::to_string(newId);
		fs::path sheetDir = dir / folderName;
		fs::create_directories(sheetDir);

		{
			std::ofstream loc(sheetDir / "location.ini");
			if (loc)
			{
				loc << "[Location]\n";
				loc << "X=" << px << "\n";
				loc << "Y=" << py << "\n";
				loc << "Z=" << pz << "\n";
				loc << "PickupRadius=10.0\n";
				loc << "PickupMessage=" << WJConfig::Sheet_PressE << "\n";
				loc << "Author=Player\n";
				if (s_overlayCache.fromJournal)
					loc << "Source=Journal\n";
				else
					loc << "Source=CustomBook:" << s_overlayCache.bookName << "\n";
			}
		}

		{
			std::ofstream txt(sheetDir / "sheet.txt");
			if (txt) txt << s_overlayCache.text;
		}

		if (!s_overlayCache.drawing.lines.empty())
			SaveDrawingToFile(sheetDir / "sheet_draw.dat", s_overlayCache.drawing);

		int page = s_overlayCache.sourcePage;
		if (s_overlayCache.fromJournal)
			s_rippedJournalPages.erase(page);
		else
		{
			auto it = s_rippedCustomPages.find(s_overlayCache.bookName);
			if (it != s_rippedCustomPages.end())
				it->second.erase(page);
		}
		SaveRippedPagesIndex();

		s_showingOverlay = false;
		s_overlayCache = RippedSheetCache();

		ScanSheets();
	}

	void UpdatePickupPrompt(float px, float py, float pz)
	{
		std::lock_guard<std::mutex> lock(s_sheetsMutex);
		s_nearbySheetId = -1;
		s_showPickupPrompt = false;

		for (const auto& sheet : s_discoverableSheets)
		{
			if (sheet.collected) continue;
			float dx = px - sheet.x;
			float dy = py - sheet.y;
			float dz = pz - sheet.z;
			float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (dist <= sheet.radius)
			{
				s_nearbySheetId = sheet.id;
				s_showPickupPrompt = true;
				break;
			}
		}
	}

	bool IsNearPickup() { return s_showPickupPrompt; }

	bool TryPickupSheet()
	{
		if (!s_showPickupPrompt || s_nearbySheetId < 0) return false;

		std::lock_guard<std::mutex> lock(s_sheetsMutex);
		for (const auto& sheet : s_discoverableSheets)
		{
			if (sheet.id != s_nearbySheetId) continue;

			float px = s_playerX.load(), py = s_playerY.load(), pz = s_playerZ.load();
			float dx = px - sheet.x, dy = py - sheet.y, dz = pz - sheet.z;
			float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (dist > sheet.radius) return false;

			s_overlayCache.text = sheet.text;
			s_overlayCache.drawing = sheet.drawing;
			s_overlayCache.sourcePage = 0;
			s_overlayCache.fromJournal = false;
			s_overlayCache.bookName = "";
			s_overlayCache.chapter = 0;

			s_showingOverlay = true;
			s_viewingDiscoverable = true;
			s_viewingSheetId = sheet.id;
			s_showPickupPrompt = false;
			return true;
		}
		return false;
	}

	void HandleInput()
	{
		if (s_ripping)
		{
			bool pDown = (GetAsyncKeyState('P') & 0x8000) != 0;
			if (pDown)
			{
				s_ripProgress += ImGui::GetIO().DeltaTime;
				if (s_ripProgress >= RIP_HOLD_TIME)
				{
					s_ripping = false;
					s_ripAnimating = true;
					s_ripAnimT = 0.f;
				}
			}
			else
			{
				CancelRip();
			}
			return;
		}

		if (s_ripAnimating)
		{
			s_ripAnimT += ImGui::GetIO().DeltaTime;
			if (s_ripAnimT >= RIP_ANIM_DURATION)
				ConfirmRip();
			return;
		}

		if (s_showingOverlay)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			{
				if (s_viewingDiscoverable)
				{
					s_showingOverlay = false;
					s_viewingDiscoverable = false;
					s_viewingSheetId = -1;
					s_overlayCache = RippedSheetCache();
				}
				else
				{
					RestorePage();
				}
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_L, false))
			{
				if (!s_viewingDiscoverable)
					LeaveSheetAtPlayer();
			}
			return;
		}
	}

	static float Rng(unsigned& seed)
	{
		seed = seed * 1664525u + 1013904223u;
		return (float)((seed >> 8) & 0xFFFFFF) / (float)0xFFFFFF;
	}

	static ImU32 FadeCol(ImU32 col, float a)
	{
		if (a >= 1.f) return col;
		if (a <= 0.f) return col & IM_COL32(255, 255, 255, 0);
		int alpha = (int)(((col >> 24) & 0xFF) * a);
		return (col & 0x00FFFFFFu) | ((ImU32)alpha << 24);
	}

	static void DrawTornEdge(ImDrawList* dl, ImVec2 start, ImVec2 end, ImU32 col, float jagSize, unsigned& seed, bool flip)
	{
		float dx = end.x - start.x;
		float dy = end.y - start.y;
		float len = std::sqrt(dx * dx + dy * dy);
		if (len < 1.f) return;
		float ux = dx / len, uy = dy / len;
		float nx = -uy, ny = ux;

		std::vector<ImVec2> pts;
		pts.push_back(start);
		float t = 0.f;
		while (t < len)
		{
			float seg = 6.f + Rng(seed) * 10.f;
			t = std::min(t + seg, len);
			float jag = (Rng(seed) - 0.5f) * jagSize * 2.f;
			ImVec2 p = { start.x + ux * t + nx * jag, start.y + uy * t + ny * jag };
			pts.push_back(p);
		}
		pts.push_back(end);

		if (flip)
		{
			for (auto& p : pts)
			{
				p.x -= nx * jagSize * 1.5f;
				p.y -= ny * jagSize * 1.5f;
			}
		}

		dl->AddPolyline(pts.data(), (int)pts.size(), col, ImDrawFlags_None, 1.5f);
	}

	static void DrawRippedPageOverlay(const ImVec2 ds, float A)
	{
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		ImFont* f = ImGui::GetIO().Fonts->Fonts.Size > 1 ? ImGui::GetIO().Fonts->Fonts[1] : ImGui::GetFont();

		dl->AddRectFilled({ 0, 0 }, ds, IM_COL32(0, 0, 0, (int)(120.f * A)));

		float w = ds.x * 0.5f;
		float h = ds.y * 0.78f;
		ImVec2 mn{ (ds.x - w) * 0.5f, (ds.y - h) * 0.5f };
		ImVec2 mx{ mn.x + w, mn.y + h };

		unsigned seed = 4242u;
		float jag = 6.f;

		std::vector<ImVec2> poly;
		poly.push_back(mn);
		float step = 12.f;
		for (float x = mn.x + step; x < mx.x; x += step)
		{
			float jy = mn.y + (Rng(seed) - 0.5f) * jag;
			poly.push_back({ x, jy });
		}
		poly.push_back({ mx.x, mn.y });
		for (float y = mn.y + step; y < mx.y; y += step)
		{
			float jx = mx.x + (Rng(seed) - 0.5f) * jag;
			poly.push_back({ jx, y });
		}
		poly.push_back({ mx.x, mx.y });  
		for (float x = mx.x - step; x > mn.x; x -= step)
		{
			float jy = mx.y + (Rng(seed) - 0.5f) * jag;
			poly.push_back({ x, jy });
		}
		poly.push_back(mn);
		for (float y = mx.y - step; y > mn.y; y -= step)
		{
			float jx = mn.x + (Rng(seed) - 0.5f) * jag;
			poly.push_back({ jx, y });
		}

		dl->AddConvexPolyFilled(poly.data(), (int)poly.size(), FadeCol(IM_COL32(210, 200, 175, 255), A));
		dl->AddPolyline(poly.data(), (int)poly.size(), FadeCol(IM_COL32(160, 140, 110, 200), A), ImDrawFlags_None, 1.5f);

		if (!s_overlayCache.drawing.lines.empty())
		{
			dl->PushClipRect(mn, mx, true);
			for (const auto& line : s_overlayCache.drawing.lines)
			{
				if (line.points.size() < 2) continue;
				for (size_t i = 1; i < line.points.size(); ++i)
				{
					ImVec2 p1 = { mn.x + line.points[i - 1].x * w, mn.y + line.points[i - 1].y * h };
					ImVec2 p2 = { mn.x + line.points[i].x * w, mn.y + line.points[i].y * h };
					dl->AddLine(p1, p2, FadeCol(line.color, A * 0.7f), line.thickness);
				}
			}
			dl->PopClipRect();
		}

		float pad = w * 0.08f;
		ImVec2 tmin(mn.x + pad, mn.y + pad);
		ImVec2 tmax(mx.x - pad, mx.y - pad);

		ImGui::SetNextWindowPos(tmin);
		ImGui::SetNextWindowSize({ tmax.x - tmin.x, tmax.y - tmin.y });

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Text, FadeCol(IM_COL32(48, 38, 30, 255), A));

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoMouseInputs;

		if (ImGui::Begin("##RippedSheetOverlay", nullptr, flags))
		{
			ImGui::PushFont(f);
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (tmax.x - tmin.x));
			if (!s_overlayCache.text.empty())
				ImGui::TextUnformatted(s_overlayCache.text.c_str());
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Text, FadeCol(IM_COL32(76, 62, 48, 200), A));
				ImGui::TextUnformatted("(empty page)");
				ImGui::PopStyleColor();
			}
			ImGui::PopTextWrapPos();
			ImGui::PopFont();
		}
		ImGui::End();

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(5);

		ImFont* df = ImGui::GetFont();
		const float refH = 1080.f;
		const float scaleFactor = std::clamp(ds.y / refH, 0.6f, 1.5f);
		const float fh = df->FontSize * 1.2f * scaleFactor;
		const float helpY = ds.y - fh * 1.9f;
		const float xm = std::max(ds.x * 0.018f, 10.f);

		std::string helpStr;
		if (s_viewingDiscoverable)
		{
			helpStr = WJConfig::Sheet_ReadHint + "   |   ESC: " + WJConfig::Sheet_CloseHint;
		}
		else
		{
			helpStr = WJConfig::Sheet_LeaveHint + "   |   " + WJConfig::Sheet_ReadHint + "   |   ESC: " + WJConfig::Sheet_RestoreHint;
		}
		dl->AddText(df, fh, { xm, helpY }, FadeCol(IM_COL32(228, 216, 192, 150), A), helpStr.c_str());
	}

	static void DrawRipProgressBar(const ImVec2 ds, float A)
	{
		if (!s_ripping) return;

		ImFont* df = ImGui::GetFont();
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		float w = std::min(200.f, ds.x * 0.15f);
		float h = 7.f;
		ImVec2 c(ds.x * 0.5f, ds.y - df->FontSize * 4.4f);
		float progress = s_ripProgress / RIP_HOLD_TIME;

		const char* msg = WJConfig::RippingProgress.c_str();
		ImVec2 msz = df->CalcTextSizeA(df->FontSize, FLT_MAX, 0.f, msg);
		dl->AddText({ c.x - msz.x * 0.5f, c.y - h - df->FontSize * 1.2f },
			FadeCol(IM_COL32(228, 216, 192, 150), A), msg);

		dl->AddRectFilled({ c.x - w * 0.5f, c.y - h * 0.5f },
			{ c.x + w * 0.5f, c.y + h * 0.5f },
			FadeCol(IM_COL32(20, 14, 8, 190), A), 3.f);
		if (progress > 0.01f)
			dl->AddRectFilled({ c.x - w * 0.5f + 1.5f, c.y - h * 0.5f + 1.5f },
				{ c.x - w * 0.5f + 1.5f + (w - 3.f) * progress, c.y + h * 0.5f - 1.5f },
				FadeCol(IM_COL32(220, 188, 130, 255), A), 2.f);
		dl->AddRect({ c.x - w * 0.5f, c.y - h * 0.5f }, { c.x + w * 0.5f, c.y + h * 0.5f },
			FadeCol(IM_COL32(206, 172, 126, 140), A), 3.f, 0, 1.f);
	}

	static void DrawRipAnimation(const ImVec2 ds, float A)
	{
		if (!s_ripAnimating) return;

		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		float t = s_ripAnimT / RIP_ANIM_DURATION;
		if (t > 1.f) t = 1.f;

		float pageW = ds.x * 0.18f;
		float pageH = ds.y * 0.35f;
		float startX = ds.x * 0.5f;
		float startY = ds.y * 0.5f;
		float endX = ds.x * 0.65f;
		float endY = ds.y * 0.15f;

		float cx = startX + (endX - startX) * t;
		float cy = startY + (endY - startY) * t;
		float alpha = 1.f - t;
		float angle = t * 0.3f;

		float cosA = std::cos(angle);
		float sinA = std::sin(angle);
		float hw = pageW * 0.5f, hh = pageH * 0.5f;

		ImVec2 corners[4] = {
			{ cx + (-hw * cosA - (-hh) * sinA), cy + (-hw * sinA + (-hh) * cosA) },
			{ cx + (hw * cosA - (-hh) * sinA),  cy + (hw * sinA + (-hh) * cosA) },
			{ cx + (hw * cosA - hh * sinA),     cy + (hw * sinA + hh * cosA) },
			{ cx + (-hw * cosA - hh * sinA),    cy + (-hw * sinA + hh * cosA) }
		};

		dl->AddConvexPolyFilled(corners, 4, FadeCol(IM_COL32(210, 200, 175, 255), A * alpha));
		dl->AddPolyline(corners, 4, FadeCol(IM_COL32(160, 140, 110, 200), A * alpha), ImDrawFlags_Closed, 1.5f);
	}

	void Render()
	{
		if (s_showingOverlay)
		{
			ImGuiIO& io = ImGui::GetIO();
			DrawRippedPageOverlay(io.DisplaySize, 1.f);
			return;
		}

		if (s_ripAnimating)
		{
			ImGuiIO& io = ImGui::GetIO();
			DrawRipAnimation(io.DisplaySize, 1.f);
			return;
		}

		if (s_ripping)
		{
			ImGuiIO& io = ImGui::GetIO();
			DrawRipProgressBar(io.DisplaySize, 1.f);
		}
	}

	void RenderPickupPrompt()
	{
		if (!s_showPickupPrompt || s_nearbySheetId < 0) return;
		if (s_showingOverlay || s_ripping || s_ripAnimating) return;

		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		ImVec2 ds = io.DisplaySize;
		ImFont* f = io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];

		const char* msg1 = WJConfig::Sheet_Nearby.c_str();
		const char* msg2 = WJConfig::Sheet_PressE.c_str();

		ImVec2 s1 = f->CalcTextSizeA(f->FontSize * 1.2f, FLT_MAX, 0.f, msg1);
		ImVec2 s2 = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, msg2);

		float y = ds.y * 0.75f;
		dl->AddText(f, f->FontSize * 1.2f, { ds.x * 0.5f - s1.x * 0.5f, y }, IM_COL32(234, 223, 197, 255), msg1);
		dl->AddText(f, f->FontSize, { ds.x * 0.5f - s2.x * 0.5f, y + f->FontSize * 1.5f }, IM_COL32(200, 185, 155, 220), msg2);
	}
}
