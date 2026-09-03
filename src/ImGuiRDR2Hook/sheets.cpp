#define NOMINMAX
#include "sheets.h"
#include "custombooks.h"
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
	static constexpr float E_HOLD_TIME = 3.0f;
	static constexpr float FLIP_ANIM_DURATION = 0.8f;
	static constexpr float CROUCH_DURATION_MS = 1200.f;
	static constexpr float PICKUP_RADIUS_DEFAULT = 5.f;
	static constexpr float PICKUP_PROMPT_RADIUS = 5.f;

	static std::atomic<float> s_playerX{ 0.f };
	static std::atomic<float> s_playerY{ 0.f };
	static std::atomic<float> s_playerZ{ 0.f };

	static bool s_ripping = false;
	static float s_ripProgress = 0.f;
	static RippedSheetCache s_ripCache;

	static bool s_ripAnimating = false;
	static float s_ripAnimT = 0.f;

	static bool s_restoreAnimating = false;
	static float s_restoreAnimT = 0.f;
	static RippedSheetCache s_restoreCache;

	static bool s_showingOverlay = false;
	static bool s_viewingDiscoverable = false;
	static int s_viewingSheetId = -1;
	static RippedSheetCache s_overlayCache;

	static bool s_showingBack = false;
	static bool s_flipAnimating = false;
	static float s_flipAnimT = 0.f;
	static bool s_flipToFront = false;

	static std::unordered_set<int> s_rippedJournalPages;
	static std::unordered_map<std::string, std::unordered_set<int>> s_rippedCustomPages;
	
	static std::unordered_set<int> s_restoredJournalPages;
	static std::unordered_map<std::string, std::unordered_set<int>> s_restoredCustomPages;

	static std::vector<DiscoverableSheet> s_discoverableSheets;
	static int s_nearbySheetId = -1;
	static bool s_showPickupPrompt = false;
	static int s_nextSheetId = 1;
	static bool s_scanned = false;

	static bool s_walkingToSheet = false;
	static float s_walkTargetX = 0.f, s_walkTargetY = 0.f, s_walkTargetZ = 0.f;
	static int s_walkTargetId = -1;
	static DWORD s_walkStartMs = 0;
	static float s_walkStartPX = 0.f, s_walkStartPY = 0.f, s_walkStartPZ = 0.f;

	static bool s_eHoldActive = false;
	static float s_eHoldProgress = 0.f;
	static int s_eHoldSheetId = -1;
	static bool s_eHoldComplete = false;

	static bool s_crouching = false;
	static DWORD s_crouchStartMs = 0;

	static bool s_sheetKept = false;
	static bool s_sheetViewed = false;

	static std::unordered_set<int> s_collectedSheets;

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

	int GetPartnerPage(int page)
	{
		if (page <= 0) return 0;
		if (page % 2 == 0) return page + 1;
		if (page > 1) return page - 1;
		return 0;
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
			else if (key == "OriginalPage") sheet.originalPage = std::stoi(val);
			else if (key == "FromJournal") sheet.fromJournal = (val == "1");
			else if (key == "BookName") sheet.bookName = val;
			else if (key == "Chapter") sheet.chapter = std::stoi(val);
			else if (key == "Collected") sheet.collected = (val == "1");
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
			sheet.radius = PICKUP_RADIUS_DEFAULT;
			ParseLocationIni(locPath, sheet);

			if (sheet.collected)
			{
				s_collectedSheets.insert(id);
				continue;
			}

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

			fs::path backTextPath = entry.path() / "sheet_back.txt";
			if (fs::exists(backTextPath))
			{
				std::ifstream tf(backTextPath);
				if (tf)
				{
					std::ostringstream ss;
					ss << tf.rdbuf();
					sheet.backText = ss.str();
				}
			}

			fs::path backDrawPath = entry.path() / "sheet_back_draw.dat";
			if (fs::exists(backDrawPath))
				sheet.backDrawing = LoadDrawingFromFile(backDrawPath);

			if (sheet.pickupMessage.empty())
				sheet.pickupMessage = WJConfig::Sheet_PressE;

			s_discoverableSheets.push_back(std::move(sheet));
		}
		s_scanned = true;
	}

	void Init()
	{
		if (!WJConfig::RipSheetsEnabled) return;
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
	void DismissOverlay()
	{
		s_showingOverlay = false;
		s_viewingDiscoverable = false;
		s_viewingSheetId = -1;
		s_overlayCache = RippedSheetCache();
		s_showingBack = false;
		s_flipAnimating = false;
	}
	int GetRipSourcePage() { return s_ripCache.sourcePage; }
	bool GetRipFromJournal() { return s_ripCache.fromJournal; }
	bool IsViewingDiscoverable() { return s_viewingDiscoverable; }
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

	bool IsPageRestored(int page, bool isJournal, const std::string& bookName)
	{
		if (isJournal)
			return s_restoredJournalPages.count(page) > 0;
		auto it = s_restoredCustomPages.find(bookName);
		if (it == s_restoredCustomPages.end()) return false;
		return it->second.count(page) > 0;
	}

	int GetNextVisiblePage(int startPage)
	{
		for (int p = startPage; p < startPage + 20; ++p)
		{
			if (p < 1) continue;
			if (!IsPageRipped(p, true)) return p;
		}
		return startPage;
	}

	bool IsWalkingToSheet() { return s_walkingToSheet; }

	float GetEHoldProgress() { return s_eHoldProgress; }
	bool IsEHoldActive() { return s_eHoldActive; }
	bool IsEHoldComplete() { return s_eHoldComplete; }
	bool IsCrouching() { return s_crouching; }
	bool IsFlipAnimating() { return s_flipAnimating; }
	float GetFlipAnimT() { return s_flipAnimT; }
	bool IsShowingBack() { return s_showingBack; }
	bool IsSheetKept() { return s_sheetKept; }
	bool WasSheetViewed() { return s_sheetViewed; }
	void SetSheetViewed(bool v) { s_sheetViewed = v; }

	void StartEHold()
	{
		if (s_nearbySheetId < 0) return;
		s_eHoldActive = true;
		s_eHoldProgress = 0.f;
		s_eHoldSheetId = s_nearbySheetId;
		s_eHoldComplete = false;
	}

	void CancelEHold()
	{
		s_eHoldActive = false;
		s_eHoldProgress = 0.f;
		s_eHoldSheetId = -1;
		s_eHoldComplete = false;
	}

	void StartCrouch()
	{
		s_crouching = true;
		s_crouchStartMs = GetTickCount();
	}

	bool UpdateCrouch()
	{
		if (!s_crouching) return false;
		DWORD elapsed = GetTickCount() - s_crouchStartMs;
		if (elapsed >= (DWORD)CROUCH_DURATION_MS)
		{
			s_crouching = false;
			return true;
		}
		return false;
	}

	void StartFlipAnim()
	{
		if (s_flipAnimating) return;
		s_flipAnimating = true;
		s_flipAnimT = 0.f;
		s_flipToFront = s_showingBack;
	}

	void ToggleBackSide()
	{
		s_showingBack = !s_showingBack;
	}

	void GetNearbySheetCoords(float& x, float& y, float& z)
	{
		std::lock_guard<std::mutex> lock(s_sheetsMutex);
		for (const auto& s : s_discoverableSheets)
		{
			if (s.id == s_nearbySheetId)
			{
				x = s.x; y = s.y; z = s.z;
				return;
			}
		}
		x = y = z = 0.f;
	}

	void StartWalkToSheet()
	{
		if (s_nearbySheetId < 0) return;
		std::lock_guard<std::mutex> lock(s_sheetsMutex);
		for (const auto& s : s_discoverableSheets)
		{
			if (s.id == s_nearbySheetId)
			{
				s_walkTargetX = s.x;
				s_walkTargetY = s.y;
				s_walkTargetZ = s.z;
				s_walkTargetId = s.id;
				s_walkingToSheet = true;
				s_walkStartMs = GetTickCount();
				s_walkStartPX = s_playerX.load();
				s_walkStartPY = s_playerY.load();
				s_walkStartPZ = s_playerZ.load();
				break;
			}
		}
	}

	void CancelWalk()
	{
		s_walkingToSheet = false;
		s_walkTargetId = -1;
	}

	bool UpdateWalk(float px, float py, float pz)
	{
		if (!s_walkingToSheet) return false;
		float dxStart = px - s_walkStartPX;
		float dyStart = py - s_walkStartPY;
		float dzStart = pz - s_walkStartPZ;
		float playerMoved = std::sqrt(dxStart * dxStart + dyStart * dyStart + dzStart * dzStart);
		if (playerMoved > 2.0f)
		{
			s_walkingToSheet = false;
			s_walkTargetId = -1;
			return false;
		}
		float dx = px - s_walkTargetX;
		float dy = py - s_walkTargetY;
		float dz = pz - s_walkTargetZ;
		float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
		DWORD elapsed = GetTickCount() - s_walkStartMs;
		if (dist < 1.2f || elapsed > 8000)
		{
			s_walkingToSheet = false;
			return true;
		}
		return false;
	}

	void StartRipPage(const std::string& text, const SheetDrawing& drawing, int page, bool fromJournal, const std::string& bookName, int chapter, const std::string& backText, const SheetDrawing& backDrawing)
	{
		if (s_ripping || s_ripAnimating || s_showingOverlay) return;
		s_ripCache.text = text;
		s_ripCache.drawing = drawing;
		s_ripCache.sourcePage = page;
		s_ripCache.fromJournal = fromJournal;
		s_ripCache.bookName = bookName;
		s_ripCache.chapter = chapter;
		s_ripCache.backText = backText;
		s_ripCache.backDrawing = backDrawing;
		s_ripCache.backPage = GetPartnerPage(page);
		s_ripping = true;
		s_ripProgress = 0.f;
	}

	void ConfirmRip()
	{
		if (!s_ripping && !s_ripAnimating) return;

		int page = s_ripCache.sourcePage;
		int partner = GetPartnerPage(page);
		s_ripCache.backPage = partner;

		if (s_ripCache.fromJournal)
		{
			s_rippedJournalPages.insert(page);
			if (partner > 0) s_rippedJournalPages.insert(partner);
		}
		else
		{
			s_rippedCustomPages[s_ripCache.bookName].insert(page);
			if (partner > 0) s_rippedCustomPages[s_ripCache.bookName].insert(partner);
			CustomBooks::RipPage(s_ripCache.bookName, page);
			int linesPerPage = 12;
			int lineIndex = page * linesPerPage;
			CustomBooks::MarkChapterAsRipped(s_ripCache.bookName, lineIndex);
		}

		SaveRippedPagesIndex();

		s_ripping = false;
		s_ripAnimating = false;
		s_ripAnimT = 0.f;
		s_ripProgress = 0.f;

		s_overlayCache = s_ripCache;
		s_showingOverlay = true;
		s_viewingDiscoverable = false;
		s_viewingSheetId = -1;
		s_showingBack = false;
		s_flipAnimating = false;
		s_sheetViewed = false;
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
		int partner = GetPartnerPage(page);
		if (s_overlayCache.fromJournal)
		{
			s_rippedJournalPages.erase(page);
			if (partner > 0) s_rippedJournalPages.erase(partner);
			s_restoredJournalPages.insert(page);
			if (partner > 0) s_restoredJournalPages.insert(partner);
		}
		else
		{
			auto it = s_rippedCustomPages.find(s_overlayCache.bookName);
			if (it != s_rippedCustomPages.end())
			{
				it->second.erase(page);
				if (partner > 0) it->second.erase(partner);
			}
			s_restoredCustomPages[s_overlayCache.bookName].insert(page);
			if (partner > 0) s_restoredCustomPages[s_overlayCache.bookName].insert(partner);
			CustomBooks::RestorePage(s_overlayCache.bookName, page);
		}

		SaveRippedPagesIndex();
		
		s_restoreCache = s_overlayCache;
		s_restoreAnimating = true;
		s_restoreAnimT = 0.f;
		s_showingOverlay = false;
		s_overlayCache = RippedSheetCache();
		s_showingBack = false;
		s_flipAnimating = false;
	}

	void KeepSheet()
	{
		if (!s_showingOverlay) return;

		if (s_viewingDiscoverable && s_viewingSheetId >= 0)
		{
			MarkSheetCollected(s_viewingSheetId);

			std::lock_guard<std::mutex> lock(s_sheetsMutex);
			for (auto& sheet : s_discoverableSheets)
			{
				if (sheet.id == s_viewingSheetId)
				{
					fs::path dir = GetDiscoverablesDir() / ("SHEET" + std::to_string(sheet.id));
					fs::path locPath = dir / "location.ini";
					if (fs::exists(locPath))
					{
						std::ifstream in(locPath);
						std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
						in.close();
						size_t pos = content.find("Collected=");
						if (pos != std::string::npos)
						{
							size_t endLine = content.find('\n', pos);
							if (endLine != std::string::npos)
								content.replace(pos, endLine - pos, "Collected=1");
							else
								content.replace(pos, content.size() - pos, "Collected=1");
						}
						else
						{
							content += "\nCollected=1\n";
						}
						std::ofstream out(locPath, std::ios::trunc);
						if (out) out << content;
					}
					sheet.collected = true;
					break;
				}
			}
		}

		s_sheetKept = true;
		s_showingOverlay = false;
		s_viewingDiscoverable = false;
		s_viewingSheetId = -1;
		s_overlayCache = RippedSheetCache();
		s_showingBack = false;
		s_flipAnimating = false;
		s_showPickupPrompt = false;
	}

	void MarkSheetCollected(int sheetId)
	{
		s_collectedSheets.insert(sheetId);
	}

	bool IsSheetCollected(int sheetId)
	{
		return s_collectedSheets.count(sheetId) > 0;
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
				loc << "PickupRadius=" << PICKUP_RADIUS_DEFAULT << "\n";
				loc << "Author=Player\n";
				if (s_overlayCache.fromJournal)
					loc << "Source=Journal\n";
				else
					loc << "Source=CustomBook:" << s_overlayCache.bookName << "\n";
				loc << "OriginalPage=" << s_overlayCache.sourcePage << "\n";
				loc << "FromJournal=" << (s_overlayCache.fromJournal ? "1" : "0") << "\n";
				loc << "BookName=" << s_overlayCache.bookName << "\n";
				loc << "Chapter=" << s_overlayCache.chapter << "\n";
			}
		}

		{
			std::ofstream txt(sheetDir / "sheet.txt");
			if (txt) txt << s_overlayCache.text;
		}

		if (!s_overlayCache.drawing.lines.empty())
			SaveDrawingToFile(sheetDir / "sheet_draw.dat", s_overlayCache.drawing);

		if (!s_overlayCache.backText.empty())
		{
			std::ofstream btxt(sheetDir / "sheet_back.txt");
			if (btxt) btxt << s_overlayCache.backText;
		}

		if (!s_overlayCache.backDrawing.lines.empty())
			SaveDrawingToFile(sheetDir / "sheet_back_draw.dat", s_overlayCache.backDrawing);

		s_showingOverlay = false;
		s_overlayCache = RippedSheetCache();
		s_showingBack = false;

		ScanSheets();
	}

	void UpdatePickupPrompt(float px, float py, float pz)
	{
		if (!WJConfig::RipSheetsEnabled) return;

		std::lock_guard<std::mutex> lock(s_sheetsMutex);
		s_nearbySheetId = -1;
		s_showPickupPrompt = false;

		for (const auto& sheet : s_discoverableSheets)
		{
			if (sheet.collected) continue;
			if (IsSheetCollected(sheet.id)) continue;
			float dx = px - sheet.x;
			float dy = py - sheet.y;
			float dz = pz - sheet.z;
			float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (dist <= PICKUP_PROMPT_RADIUS)
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
			if (dist > sheet.radius + 2.f) return false;

			s_overlayCache.text = sheet.text;
			s_overlayCache.drawing = sheet.drawing;
			s_overlayCache.sourcePage = sheet.originalPage;
			s_overlayCache.fromJournal = sheet.fromJournal;
			s_overlayCache.bookName = sheet.bookName;
			s_overlayCache.chapter = sheet.chapter;
			s_overlayCache.backText = sheet.backText;
			s_overlayCache.backDrawing = sheet.backDrawing;
			s_overlayCache.backPage = GetPartnerPage(sheet.originalPage);

			s_showingOverlay = true;
			s_viewingDiscoverable = true;
			s_viewingSheetId = sheet.id;
			s_showPickupPrompt = false;
			s_showingBack = false;
			s_flipAnimating = false;
			s_sheetViewed = false;
			s_sheetKept = false;
			return true;
		}
		return false;
	}

	void HandleInput()
	{
		if (!WJConfig::RipSheetsEnabled) return;

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

		if (s_restoreAnimating)
		{
			s_restoreAnimT += ImGui::GetIO().DeltaTime;
			if (s_restoreAnimT >= RIP_ANIM_DURATION)
			{
				s_restoreAnimating = false;
				s_restoreAnimT = 0.f;
				s_restoreCache = RippedSheetCache();
			}
			return;
		}

		if (s_flipAnimating)
		{
			s_flipAnimT += ImGui::GetIO().DeltaTime;
			if (s_flipAnimT >= FLIP_ANIM_DURATION)
			{
				s_flipAnimating = false;
				s_flipAnimT = 0.f;
				ToggleBackSide();
			}
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
					s_showingBack = false;
					s_flipAnimating = false;
					s_sheetViewed = true;
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
			else if (ImGui::IsKeyPressed(ImGuiKey_R, false))
			{
				if (!s_flipAnimating)
				{
					bool hasBack = s_showingBack ?
						(!s_overlayCache.text.empty() || !s_overlayCache.drawing.lines.empty()) :
						(!s_overlayCache.backText.empty() || !s_overlayCache.backDrawing.lines.empty());
					if (hasBack)
						StartFlipAnim();
				}
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_K, false))
			{
				if (s_viewingDiscoverable)
				{
					KeepSheet();
				}
			}
			return;
		}

		if (s_eHoldActive)
		{
			bool rDown = (GetAsyncKeyState('R') & 0x8000) != 0;
			if (rDown)
			{
				s_eHoldProgress += ImGui::GetIO().DeltaTime;
				if (s_eHoldProgress >= E_HOLD_TIME)
				{
					s_eHoldComplete = true;
					s_eHoldActive = false;
					s_showPickupPrompt = false;
				}
			}
			else
			{
				CancelEHold();
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

	static void DrawSheetContent(ImDrawList* dl, ImFont* f, const std::string& text, const SheetDrawing& drawing,
		ImVec2 mn, ImVec2 mx, float w, float h, float A, bool showEmpty)
	{
		if (!drawing.lines.empty())
		{
			dl->PushClipRect(mn, mx, true);
			for (const auto& line : drawing.lines)
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
		float textW = tmax.x - tmin.x;
		float fontSize = f->FontSize * 0.85f;
		float lineH = fontSize * 1.45f;
		float curY = tmin.y;

		dl->PushClipRect(mn, mx, true);

		if (text.empty())
		{
			// Fix 14: Don't show "empty page" text
		}
		else
		{
			std::istringstream stream(text);
			std::string rawLine;
			while (std::getline(stream, rawLine))
			{
				if (curY + lineH > tmax.y) break;
				if (rawLine.empty())
				{
					curY += lineH;
					continue;
				}
				std::istringstream wordStream(rawLine);
				std::string word;
				std::string curLine;
				auto flushLine = [&](bool last)
				{
					if (curLine.empty()) return;
					dl->AddText(f, fontSize, { tmin.x, curY }, FadeCol(IM_COL32(48, 38, 30, 255), A), curLine.c_str());
					curY += lineH;
					curLine.clear();
				};
				while (wordStream >> word)
				{
					std::string testLine = curLine.empty() ? word : curLine + " " + word;
					ImVec2 sz = f->CalcTextSizeA(fontSize, FLT_MAX, 0.f, testLine.c_str());
					if (sz.x > textW && !curLine.empty())
					{
						flushLine(false);
						if (curY + lineH > tmax.y) break;
						curLine = word;
					}
					else
					{
						curLine = testLine;
					}
				}
				if (!curLine.empty() && curY + lineH <= tmax.y + 0.1f)
					flushLine(true);
				else if (!curLine.empty())
					flushLine(true);
			}
		}
		dl->PopClipRect();
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

		float flipScale = 1.f;
		if (s_flipAnimating)
		{
			float t = s_flipAnimT / FLIP_ANIM_DURATION;
			if (t > 1.f) t = 1.f;
			flipScale = std::abs(std::cos(t * 3.14159f));
			if (flipScale < 0.02f) flipScale = 0.02f;
		}

		float sheetW = w * flipScale;
		ImVec2 sheetMn{ mn.x + (w - sheetW) * 0.5f, mn.y };
		ImVec2 sheetMx{ sheetMn.x + sheetW, mx.y };

		unsigned seed = 4242u;
		float jag = 6.f;

		std::vector<ImVec2> poly;
		poly.push_back(sheetMn);
		float step = 12.f;
		for (float x = sheetMn.x + step; x < sheetMx.x; x += step)
		{
			float jy = sheetMn.y + (Rng(seed) - 0.5f) * jag;
			poly.push_back({ x, jy });
		}
		poly.push_back({ sheetMx.x, sheetMn.y });
		for (float y = sheetMn.y + step; y < sheetMx.y; y += step)
		{
			float jx = sheetMx.x + (Rng(seed) - 0.5f) * jag;
			poly.push_back({ jx, y });
		}
		poly.push_back({ sheetMx.x, sheetMx.y });
		for (float x = sheetMx.x - step; x > sheetMn.x; x -= step)
		{
			float jy = sheetMx.y + (Rng(seed) - 0.5f) * jag;
			poly.push_back({ x, jy });
		}
		poly.push_back(sheetMn);
		for (float y = sheetMx.y - step; y > sheetMn.y; y -= step)
		{
			float jx = sheetMn.x + (Rng(seed) - 0.5f) * jag;
			poly.push_back({ jx, y });
		}

		dl->AddConvexPolyFilled(poly.data(), (int)poly.size(), FadeCol(IM_COL32(210, 200, 175, 255), A));
		dl->AddPolyline(poly.data(), (int)poly.size(), FadeCol(IM_COL32(160, 140, 110, 200), A), ImDrawFlags_None, 1.5f);

		bool showBack = s_showingBack || (s_flipAnimating && !s_flipToFront);

		if (flipScale > 0.1f)
		{
			if (showBack)
			{
				DrawSheetContent(dl, f, s_overlayCache.backText, s_overlayCache.backDrawing,
					sheetMn, sheetMx, sheetW, h, A, false);
			}
			else
			{
				DrawSheetContent(dl, f, s_overlayCache.text, s_overlayCache.drawing,
					sheetMn, sheetMx, sheetW, h, A, false);
			}
		}

		ImFont* df = ImGui::GetFont();
		const float refH = 1080.f;
		const float scaleFactor = std::clamp(ds.y / refH, 0.6f, 1.5f);
		const float fh = df->FontSize * 1.2f * scaleFactor;
		const float helpY = ds.y - fh * 1.9f;
		const float xm = std::max(ds.x * 0.018f, 10.f);

		std::string helpStr;
		if (s_viewingDiscoverable)
		{
			helpStr = WJConfig::Sheet_KeepSheet;
			bool hasBack = showBack ?
				(!s_overlayCache.text.empty() || !s_overlayCache.drawing.lines.empty()) :
				(!s_overlayCache.backText.empty() || !s_overlayCache.backDrawing.lines.empty());
			if (hasBack)
				helpStr += "   |   " + WJConfig::Sheet_LookBehind;
			helpStr += "   |   ESC: " + WJConfig::Sheet_CloseHint;
		}
		else
		{
			helpStr = WJConfig::Sheet_LeaveHint;
			bool hasBack = showBack ?
				(!s_overlayCache.text.empty() || !s_overlayCache.drawing.lines.empty()) :
				(!s_overlayCache.backText.empty() || !s_overlayCache.backDrawing.lines.empty());
			if (hasBack)
				helpStr += "   |   " + WJConfig::Sheet_LookBehind;
			helpStr += "   |   ESC: " + WJConfig::Sheet_RestoreHint;
		}
		dl->AddText(df, fh, { xm, helpY }, FadeCol(IM_COL32(228, 216, 192, 150), A), helpStr.c_str());
	}

	static void DrawRipProgressBar(const ImVec2 ds, float A)
	{
		if (!s_ripping) return;

		ImFont* df = ImGui::GetFont();
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		float w = std::min(280.f, ds.x * 0.22f);
		float h = 8.f;
		ImVec2 c(ds.x * 0.5f, ds.y - df->FontSize * 5.0f);
		float progress = s_ripProgress / RIP_HOLD_TIME;

		const char* msg = WJConfig::RippingProgress.c_str();
		ImVec2 msz = df->CalcTextSizeA(df->FontSize * 1.1f, FLT_MAX, 0.f, msg);
		dl->AddText(df, df->FontSize * 1.1f, { c.x - msz.x * 0.5f, c.y - h - df->FontSize * 1.8f },
			FadeCol(IM_COL32(220, 200, 160, 230), A), msg);

		dl->AddRectFilled({ c.x - w * 0.5f - 2.f, c.y - h * 0.5f - 2.f },
			{ c.x + w * 0.5f + 2.f, c.y + h * 0.5f + 2.f },
			FadeCol(IM_COL32(0, 0, 0, 180), A), 4.f);

		dl->AddRectFilled({ c.x - w * 0.5f, c.y - h * 0.5f },
			{ c.x + w * 0.5f, c.y + h * 0.5f },
			FadeCol(IM_COL32(40, 30, 20, 200), A), 3.f);

		if (progress > 0.01f)
		{
			float fillW = (w - 3.f) * progress;
			dl->AddRectFilledMultiColor(
				{ c.x - w * 0.5f + 1.5f, c.y - h * 0.5f + 1.5f },
				{ c.x - w * 0.5f + 1.5f + fillW, c.y + h * 0.5f - 1.5f },
				FadeCol(IM_COL32(200, 170, 90, 230), A),
				FadeCol(IM_COL32(180, 150, 70, 230), A),
				FadeCol(IM_COL32(180, 150, 70, 230), A),
				FadeCol(IM_COL32(200, 170, 90, 230), A));
		}

		dl->AddRect({ c.x - w * 0.5f, c.y - h * 0.5f }, { c.x + w * 0.5f, c.y + h * 0.5f },
			FadeCol(IM_COL32(180, 160, 100, 180), A), 3.f, 0, 1.5f);
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

	static void DrawRestoreAnimation(const ImVec2 ds, float A)
	{
		if (!s_restoreAnimating) return;

		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		float t = s_restoreAnimT / RIP_ANIM_DURATION;
		if (t > 1.f) t = 1.f;

		float pageW = ds.x * 0.18f;
		float pageH = ds.y * 0.35f;
		float startX = ds.x * 0.65f;
		float startY = ds.y * 0.15f;
		float endX = ds.x * 0.5f;
		float endY = ds.y * 0.5f;

		float cx = startX + (endX - startX) * t;
		float cy = startY + (endY - startY) * t;
		float alpha = t;
		float angle = (1.f - t) * 0.5f;
		float crumple = std::sin(t * 3.14159f) * 0.15f;

		float cosA = std::cos(angle);
		float sinA = std::sin(angle);
		float hw = pageW * 0.5f * (1.f - crumple);
		float hh = pageH * 0.5f * (1.f + crumple * 0.5f);

		ImVec2 corners[4] = {
			{ cx + (-hw * cosA - (-hh) * sinA), cy + (-hw * sinA + (-hh) * cosA) },
			{ cx + (hw * cosA - (-hh) * sinA),  cy + (hw * sinA + (-hh) * cosA) },
			{ cx + (hw * cosA - hh * sinA),     cy + (hw * sinA + hh * cosA) },
			{ cx + (-hw * cosA - hh * sinA),    cy + (-hw * sinA + hh * cosA) }
		};

		dl->AddConvexPolyFilled(corners, 4, FadeCol(IM_COL32(210, 200, 175, 255), A * alpha));
		dl->AddPolyline(corners, 4, FadeCol(IM_COL32(160, 140, 110, 200), A * alpha), ImDrawFlags_Closed, 1.5f);

		if (t > 0.3f && t < 0.7f)
		{
			float wrinkleAlpha = (t - 0.3f) / 0.4f;
			wrinkleAlpha = 1.f - std::abs(wrinkleAlpha - 0.5f) * 2.f;
			for (int i = 0; i < 3; ++i)
			{
				float wy = cy - hh + (hh * 2.f) * (i + 1) / 4.f;
				dl->AddLine({ cx - hw * 0.8f, wy }, { cx + hw * 0.8f, wy },
				            FadeCol(IM_COL32(140, 120, 90, 150), A * wrinkleAlpha), 1.f);
			}
		}
	}

	void Render()
	{
		if (!WJConfig::RipSheetsEnabled) return;

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

		if (s_restoreAnimating)
		{
			ImGuiIO& io = ImGui::GetIO();
			DrawRestoreAnimation(io.DisplaySize, 1.f);
			return;
		}

		if (s_ripping)
		{
			ImGuiIO& io = ImGui::GetIO();
			DrawRipProgressBar(io.DisplaySize, 1.f);
		}
	}

	static void DrawKeyIcon(ImDrawList* dl, ImFont* f, const char* key, ImVec2 center, float size, float progress, ImU32 bgCol, ImU32 borderCol, ImU32 fillCol, ImU32 textCol)
	{
		float half = size * 0.5f;
		ImVec2 mn{ center.x - half, center.y - half };
		ImVec2 mx{ center.x + half, center.y + half };
		float round = size * 0.15f;

		dl->AddRectFilled(mn, mx, bgCol, round);
		dl->AddRect(mn, mx, borderCol, round, 0, 2.f);

		if (progress > 0.01f)
		{
			float barH = 4.f;
			float barPad = 3.f;
			ImVec2 barMn{ mn.x + barPad, mx.y - barPad - barH };
			ImVec2 barMx{ mx.x - barPad, mx.y - barPad };
			dl->AddRectFilled(barMn, barMx, IM_COL32(30, 25, 20, 180), 2.f);
			float fillW = (barMx.x - barMn.x) * progress;
			dl->AddRectFilled({ barMn.x, barMn.y }, { barMn.x + fillW, barMx.y }, fillCol, 2.f);
			dl->AddRect(barMn, barMx, IM_COL32(200, 200, 200, 150), 2.f, 0, 1.f);
		}

		ImVec2 tsz = f->CalcTextSizeA(size * 0.5f, FLT_MAX, 0.f, key);
		float textY = center.y - tsz.y * 0.5f - (progress > 0.01f ? 3.f : 0.f);
		dl->AddText(f, size * 0.5f, { center.x - tsz.x * 0.5f, textY }, textCol, key);
	}

	void RenderPickupPrompt()
	{
		if (!WJConfig::RipSheetsEnabled) return;
		if (!s_showPickupPrompt || s_nearbySheetId < 0) return;
		if (s_showingOverlay || s_ripping || s_ripAnimating) return;
		if (s_eHoldActive || s_eHoldComplete) return;

		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		ImVec2 ds = io.DisplaySize;
		ImFont* f = io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];
		ImFont* df = ImGui::GetFont();

		const char* msg1 = WJConfig::Sheet_Nearby.c_str();

		ImVec2 s1 = f->CalcTextSizeA(f->FontSize * 1.2f, FLT_MAX, 0.f, msg1);

		float y = ds.y * 0.75f;
		dl->AddText(f, f->FontSize * 1.2f, { ds.x * 0.5f - s1.x * 0.5f, y }, IM_COL32(234, 223, 197, 255), msg1);

		char keyStr[2] = { WJConfig::RipSheetPickupKey, '\0' };
		float keySize = 40.f;
		ImVec2 keyCenter{ ds.x * 0.5f, y + f->FontSize * 1.5f + keySize * 0.5f + 5.f };
		DrawKeyIcon(dl, df, keyStr, keyCenter, keySize, 0.f,
			IM_COL32(20, 16, 12, 220),
			IM_COL32(200, 185, 155, 220),
			IM_COL32(255, 255, 255, 200),
			IM_COL32(234, 223, 197, 255));
	}

	void RenderEHoldPrompt()
	{
		if (!s_eHoldActive) return;

		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		ImVec2 ds = io.DisplaySize;
		ImFont* f = io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];
		ImFont* df = ImGui::GetFont();

		const char* msg1 = WJConfig::Sheet_Nearby.c_str();
		ImVec2 s1 = f->CalcTextSizeA(f->FontSize * 1.2f, FLT_MAX, 0.f, msg1);

		float y = ds.y * 0.75f;
		dl->AddText(f, f->FontSize * 1.2f, { ds.x * 0.5f - s1.x * 0.5f, y }, IM_COL32(234, 223, 197, 255), msg1);

		float keySize = 48.f;
		ImVec2 keyCenter{ ds.x * 0.5f, y + f->FontSize * 1.5f + keySize * 0.5f + 5.f };
		float progress = s_eHoldProgress / E_HOLD_TIME;
		DrawKeyIcon(dl, df, "R", keyCenter, keySize, progress,
			IM_COL32(20, 16, 12, 220),
			IM_COL32(255, 215, 0, 240),
			IM_COL32(255, 255, 255, 230),
			IM_COL32(255, 255, 255, 255));
	}
}
