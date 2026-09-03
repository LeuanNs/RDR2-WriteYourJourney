#define NOMINMAX
#include "custombooks.h"
#include "sheets.h"
#include "config.h"
#include "imgui/imgui.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <cmath>

namespace fs = std::filesystem;

namespace CustomBooks
{
	static std::vector<std::string> WrapText(const std::string& text, float maxWidth, ImFont* f, float fontSize);
	static void DrawPageGlow(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float pulse);
	static void DrawRippedPageSlot(ImDrawList* dl, ImVec2 pgMin, ImVec2 pgMax, bool isLeft);

	static std::vector<std::string> s_availableBooks;
	static std::vector<std::string> s_filteredBooks; // filtered list for search
	static std::unordered_map<std::string, CustomBook> s_loadedBooks;
	static std::unordered_map<std::string, bool> s_ownedBooks;
	static std::unordered_map<std::string, int> s_bookmarks; // bookmark page per book
	static bool s_inventoryOpen = false;
	static bool s_bookOpen = false;
	static std::string s_currentBook;
	static int s_currentPage = 0;
	static int s_totalPages = 0;
	static float s_bHoldTime = 0.f;
	static bool s_bHeld = false;
	static int s_selectedBookIdx = 0;
	static bool s_showBookmarkMenu = false;
	static char s_searchBuffer[128] = {};
	static bool s_searchFocused = false;
	constexpr int LAZY_CHUNK_SIZE = 500;
	constexpr int LAZY_THRESHOLD = 100;
	constexpr int LAZY_CHAR_THRESHOLD = 3000;
	static RibbonAnim s_ribbonAnim;
	static std::string s_nearbyBook;
	static bool s_showPickupPrompt = false;
	static bool s_indexOpen = false;
	static int s_selectedChapterIdx = 0;

	static std::unordered_map<std::string, std::unordered_set<int>> s_rippedCustomBookPages;
	static int s_cbSelectedPage = -1;
	static bool s_cbEditMode = false;
	static int s_cbEditLine = -1;
	static int s_cbEditStartChar = -1;
	static int s_cbEditEndChar = -1;
	static char s_cbEditBuffer[512] = {};

	static fs::path GetBooksDir()
	{
		return fs::path(WJConfig::GetModuleDir()) / "MyJourney" / "Books";
	}

	static std::string Trim(const std::string& s)
	{
		size_t start = s.find_first_not_of(" \t\r\n");
		if (start == std::string::npos) return "";
		size_t end = s.find_last_not_of(" \t\r\n");
		return s.substr(start, end - start + 1);
	}

	static std::string ToLower(const std::string& s)
	{
		std::string result = s;
		for (char& c : result)
			c = (char)std::tolower((unsigned char)c);
		return result;
	}

	static void ApplySearchFilter()
	{
		s_filteredBooks.clear();
		std::string query = ToLower(Trim(std::string(s_searchBuffer)));
		if (query.empty())
		{
			for (const auto& name : s_availableBooks)
			{
				if (IsBookOwned(name))
					s_filteredBooks.push_back(name);
			}
			return;
		}
		for (const auto& name : s_availableBooks)
		{
			if (!IsBookOwned(name)) continue;
			LoadBook(name);
			auto& book = s_loadedBooks[name];
			std::string title = ToLower(book.config.displayTitle);
			std::string author = ToLower(book.config.author);
			std::string category = ToLower(book.config.category);
			if (title.find(query) != std::string::npos ||
			    author.find(query) != std::string::npos ||
			    category.find(query) != std::string::npos)
			{
				s_filteredBooks.push_back(name);
			}
		}
	}

	static void ParseConfig(const fs::path& path, BookConfig& cfg)
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
			if (!val.empty() && val.front() == '"') val = val.substr(1);
			if (!val.empty() && val.back() == '"') val.pop_back();

			if (key == "DisplayTitle") cfg.displayTitle = val;
			else if (key == "Author") cfg.author = val;
			else if (key == "Year") cfg.year = val;
			else if (key == "Category") cfg.category = val;
			else if (key == "CoverColorRGB")
			{
				sscanf(val.c_str(), "%d,%d,%d", &cfg.coverColorRGB[0], &cfg.coverColorRGB[1], &cfg.coverColorRGB[2]);
			}
			else if (key == "FontType") cfg.fontType = std::stoi(val);
			else if (key == "FontSizeOverride") cfg.fontSizeOverride = std::stoi(val);
			else if (key == "InkColor") cfg.inkColor = val;
			else if (key == "TextAlignment") cfg.textAlignment = std::stoi(val);
			else if (key == "PreserveLineBreaks") cfg.preserveLineBreaks = (val == "1");
			else if (key == "AllowsOpenRandomPage") cfg.allowsOpenRandomPage = (val == "1");
			else if (key == "HasIndex") cfg.hasIndex = (val == "1");
			else if (key == "AutoOrderPages") cfg.autoOrderPages = (val == "1");
			else if (key == "isOwned") cfg.isOwned = (val == "1");
			else if (key == "Findable") cfg.findable = (val == "1");
			else if (key == "X") cfg.locationX = std::stof(val);
			else if (key == "Y") cfg.locationY = std::stof(val);
			else if (key == "Z") cfg.locationZ = std::stof(val);
			else if (key == "PickupRadius") cfg.pickupRadius = std::stof(val);
			else if (key == "PickupMessage") cfg.pickupMessage = val;
		}
	}

	static void ParseIndex(const fs::path& path, std::vector<BookChapter>& chapters)
	{
		std::ifstream f(path);
		if (!f) return;
		std::string line;
		while (std::getline(f, line))
		{
			if (line.find("\"title\"") == std::string::npos) continue;
			BookChapter ch;
			size_t t1 = line.find("\"title\"");
			size_t c1 = line.find(':', t1);
			size_t q1 = line.find('"', c1);
			size_t q2 = line.find('"', q1 + 1);
			if (q1 != std::string::npos && q2 != std::string::npos)
				ch.title = line.substr(q1 + 1, q2 - q1 - 1);
			size_t l1 = line.find("\"line\"");
			if (l1 != std::string::npos)
			{
				size_t c2 = line.find(':', l1);
				size_t n1 = line.find_first_of("0123456789", c2);
				if (n1 != std::string::npos)
					ch.lineIndex = std::stoi(line.substr(n1));
			}
			chapters.push_back(ch);
		}
	}

	static void ParseBody(const fs::path& path, std::vector<std::string>& lines)
	{
		std::ifstream f(path);
		if (!f) return;
		std::string line;
		while (std::getline(f, line))
			lines.push_back(line);
	}

	static void LoadChunk(CustomBook& book, int centerLine)
	{
		fs::path bodyPath = GetBooksDir() / book.internalName / "body.txt";
		std::ifstream f(bodyPath);
		if (!f) return;

		int halfChunk = LAZY_CHUNK_SIZE / 2;
		int startLine = std::max(0, centerLine - halfChunk);
		int endLine = startLine + LAZY_CHUNK_SIZE;

		book.lines.clear();
		std::string line;
		int lineNum = 0;
		while (std::getline(f, line))
		{
			if (lineNum >= startLine && lineNum < endLine)
				book.lines.push_back(line);
			lineNum++;
			if (lineNum >= endLine) break;
		}

		book.lazyStartLine = startLine;
		book.lazyLoadedCount = (int)book.lines.size();
	}

	void Init()
	{
		ScanBooks();
	}

	void ScanBooks()
	{
		s_availableBooks.clear();
		fs::path booksDir = GetBooksDir();
		if (!fs::exists(booksDir)) return;
		for (const auto& entry : fs::directory_iterator(booksDir))
		{
			if (entry.is_directory())
			{
				std::string name = entry.path().filename().string();
				fs::path configPath = entry.path() / "config.ini";
				if (fs::exists(configPath))
				{
					BookConfig tempCfg;
					ParseConfig(configPath, tempCfg);
					if (tempCfg.isOwned || tempCfg.findable)
					{
						s_availableBooks.push_back(name);
						s_ownedBooks[name] = tempCfg.isOwned;
					}
				}
			}
		}
	}

	void LoadBook(const std::string& internalName)
	{
		if (s_loadedBooks.count(internalName)) return;
		fs::path bookDir = GetBooksDir() / internalName;
		CustomBook book;
		book.internalName = internalName;
		ParseConfig(bookDir / "config.ini", book.config);
		ParseIndex(bookDir / "index.json", book.chapters);
		ParseBody(bookDir / "body.txt", book.lines);
		book.loaded = true;
		s_loadedBooks[internalName] = std::move(book);
	}

	void UnloadBook(const std::string& internalName)
	{
		s_loadedBooks.erase(internalName);
	}

	void OpenInventory()
	{
		s_inventoryOpen = true;
		s_selectedBookIdx = 0;
	}

	void CloseInventory()
	{
		s_inventoryOpen = false;
		s_searchFocused = false;
	}

	bool IsInventoryOpen()
	{
		return s_inventoryOpen;
	}

	void OpenBook(const std::string& internalName)
	{
		LoadBook(internalName);
		s_currentBook = internalName;
		s_currentPage = 0;
		s_bookOpen = true;
		s_inventoryOpen = false;
		s_cbSelectedPage = -1;
		s_cbEditMode = false;
		auto& book = s_loadedBooks[internalName];

		book.lazyTotalLines = 0;
		fs::path bodyPath = GetBooksDir() / internalName / "body.txt";
		std::ifstream countFile(bodyPath);
		if (countFile)
		{
			std::string line;
			while (std::getline(countFile, line))
				book.lazyTotalLines++;
		}

		book.lazyTotalChars = 0;
		for (const auto& l : book.lines) book.lazyTotalChars += (int)l.size();

		if (book.lazyTotalChars > LAZY_CHAR_THRESHOLD)
		{
			book.lazyStartLine = 0;
			book.lazyLoadedCount = std::min(LAZY_CHUNK_SIZE, book.lazyTotalLines);
			std::vector<std::string> chunk(book.lines.begin(), book.lines.begin() + book.lazyLoadedCount);
			book.lines = std::move(chunk);
		}
		else
		{
			book.lazyStartLine = 0;
			book.lazyLoadedCount = book.lazyTotalLines;
		}

		int linesPerPage = 12;
		s_totalPages = (int)((book.lines.size() + linesPerPage - 1) / linesPerPage);
		if (s_totalPages < 1) s_totalPages = 1;
	}

	void CloseBook()
	{
		s_bookOpen = false;
		s_currentBook.clear();
	}

	bool IsBookOpen()
	{
		return s_bookOpen;
	}

	const std::vector<std::string>& GetAvailableBooks()
	{
		return s_availableBooks;
	}

	void HandleInput()
	{
		if (!WJConfig::CustomBooksEnabled)
		{
			if (s_inventoryOpen) CloseInventory();
			if (s_bookOpen) CloseBook();
			return;
		}

		int vk = (int)(unsigned char)WJConfig::CustomBooksKey;
		bool bDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
		if (bDown)
		{
			if (!s_bHeld)
			{
				s_bHeld = true;
				s_bHoldTime = 0.f;
			}
			s_bHoldTime += ImGui::GetIO().DeltaTime;
			if (s_bHoldTime >= 3.f && !s_inventoryOpen && !s_bookOpen)
			{
				ScanBooks();
				OpenInventory();
				s_bHoldTime = 0.f;
			}
		}
		else
		{
			s_bHeld = false;
			s_bHoldTime = 0.f;
		}

		if (s_bHeld && s_bHoldTime > 0.1f && !s_inventoryOpen && !s_bookOpen)
		{
			ImGuiIO& io = ImGui::GetIO();
			ImDrawList* dl = ImGui::GetBackgroundDrawList();
			ImVec2 ds = io.DisplaySize;
			float progress = s_bHoldTime / 3.f;
			if (progress > 1.f) progress = 1.f;
			float barW = 200.f, barH = 8.f;
			float bx = ds.x * 0.5f - barW * 0.5f;
			float by = ds.y * 0.85f;
			dl->AddRectFilled({ bx, by }, { bx + barW, by + barH }, IM_COL32(40, 35, 30, 200), 4.f);
			dl->AddRectFilled({ bx, by }, { bx + barW * progress, by + barH }, IM_COL32(200, 170, 100, 240), 4.f);
			dl->AddRect({ bx, by }, { bx + barW, by + barH }, IM_COL32(120, 100, 70, 200), 4.f, 0, 1.5f);
			ImFont* f = io.Fonts->Fonts[1] ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];
			const char* label = WJConfig::CB_SatchelOpening.c_str();
			ImVec2 ls = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, label);
			dl->AddText(f, f->FontSize, { ds.x * 0.5f - ls.x * 0.5f, by - f->FontSize * 1.8f }, IM_COL32(234, 223, 197, (int)(200 * progress)), label);
		}
	}

	static ImU32 GetInkColor(const std::string& name)
	{
		if (name == "Sepia") return IM_COL32(112, 66, 20, 255);
		if (name == "FadedBlue") return IM_COL32(70, 90, 120, 255);
		return IM_COL32(30, 25, 20, 255);
	}

	static void DrawFlourish(ImDrawList* dl, ImVec2 center, float width, ImU32 col)
	{
		const float h = width * 0.15f;
		dl->AddBezierQuadratic({ center.x - width * 0.5f, center.y },
		                       { center.x - width * 0.25f, center.y - h },
		                       { center.x, center.y }, col, 1.5f);
		dl->AddBezierQuadratic({ center.x, center.y },
		                       { center.x + width * 0.25f, center.y - h },
		                       { center.x + width * 0.5f, center.y }, col, 1.5f);
	}

	static void DrawDiamond(ImDrawList* dl, ImVec2 pos, float size, ImU32 col)
	{
		const float s = size * 0.5f;
		ImVec2 pts[4] = {
			{ pos.x, pos.y - s },
			{ pos.x + s, pos.y },
			{ pos.x, pos.y + s },
			{ pos.x - s, pos.y }
		};
		dl->AddConvexPolyFilled(pts, 4, col);
	}

	static void DrawBookCover(ImDrawList* dl, const CustomBook& book, float bx, float by, float bookW, float bookH, float A)
	{
		// Oscurecer el color del cover para que pegue con la estética de RDR2
		ImU32 coverCol = IM_COL32(
			(int)(book.config.coverColorRGB[0] * 0.55f),
			(int)(book.config.coverColorRGB[1] * 0.55f),
			(int)(book.config.coverColorRGB[2] * 0.55f), 255);
		ImU32 darkCover = IM_COL32(
			(int)(book.config.coverColorRGB[0] * 0.35f),
			(int)(book.config.coverColorRGB[1] * 0.35f),
			(int)(book.config.coverColorRGB[2] * 0.35f), 255);

		const float round = bookW * 0.04f;
		dl->AddRectFilled({ bx, by }, { bx + bookW, by + bookH }, coverCol, round);

		const float f = bookW * 0.025f;
		const ImVec2 bmin(bx + f, by + f);
		const ImVec2 bmax(bx + bookW - f, by + bookH - f);

		dl->AddRect({ bmin.x + 1.5f, bmin.y + 2.f }, { bmax.x + 1.5f, bmax.y + 2.f },
		            IM_COL32(0, 0, 0, 70), round * 0.5f);
		dl->AddRect(bmin, bmax, IM_COL32(0, 0, 0, 200), round * 0.5f, 0, 2.f);

		const float f2 = f + bookW * 0.018f;
		dl->AddRect({ bmin.x + f2, bmin.y + f2 }, { bmax.x - f2, bmax.y - f2 },
		            IM_COL32(0, 0, 0, 120), round * 0.4f, 0, 1.f);

		const float dr = std::max(2.5f, bookW * 0.016f);
		const ImU32 dc = IM_COL32(190, 150, 95, 150);
		DrawDiamond(dl, bmin, dr, dc);
		DrawDiamond(dl, { bmax.x, bmin.y }, dr, dc);
		DrawDiamond(dl, { bmin.x, bmax.y }, dr, dc);
		DrawDiamond(dl, bmax, dr, dc);

		{
			const float strapW = bookW * 0.115f;
			const float strapX = bmax.x - bookW * 0.315f;

			dl->AddRectFilled({ strapX - 5.f, bmin.y - 2.f },
			                  { strapX + strapW - 5.f, bmax.y + 2.f },
			                  IM_COL32(0, 0, 0, 70));

			const int steps = 16;
			for (int i = 0; i < steps; ++i)
			{
				float t0 = (float)i / steps;
				float t1 = (float)(i + 1) / steps;
				ImU32 c0 = IM_COL32(
					(int)(46 * (1.f - t0) + 29 * t0),
					(int)(28 * (1.f - t0) + 17 * t0),
					(int)(16 * (1.f - t0) + 9 * t0), 255);
				ImU32 c1 = IM_COL32(
					(int)(46 * (1.f - t1) + 29 * t1),
					(int)(28 * (1.f - t1) + 17 * t1),
					(int)(16 * (1.f - t1) + 9 * t1), 255);
				float y0 = bmin.y - 3.f + (bmax.y - bmin.y + 6.f) * t0;
				float y1 = bmin.y - 3.f + (bmax.y - bmin.y + 6.f) * t1;
				dl->AddRectFilledMultiColor(
					{ strapX, y0 }, { strapX + strapW, y1 },
					c0, c1, c1, c0);
			}

			dl->AddLine({ strapX, bmin.y }, { strapX, bmax.y }, IM_COL32(12, 7, 3, 190), 2.f);
			dl->AddLine({ strapX + strapW, bmin.y }, { strapX + strapW, bmax.y }, IM_COL32(12, 7, 3, 190), 2.f);

			const float dashLen = bookW * 0.025f;
			const float gapLen = bookW * 0.018f;
			float y = bmin.y + 6.f;
			while (y < bmax.y - 6.f)
			{
				dl->AddLine({ strapX + 4.f, y }, { strapX + 4.f, y + dashLen }, IM_COL32(200, 165, 120, 80), 1.f);
				y += dashLen + gapLen;
			}
			y = bmin.y + 6.f;
			while (y < bmax.y - 6.f)
			{
				dl->AddLine({ strapX + strapW - 4.f, y }, { strapX + strapW - 4.f, y + dashLen }, IM_COL32(200, 165, 120, 80), 1.f);
				y += dashLen + gapLen;
			}
		}

		ImFont* font = ImGui::GetIO().Fonts->Fonts[1] ? ImGui::GetIO().Fonts->Fonts[1] : ImGui::GetIO().Fonts->Fonts[0];
		const char* title = book.config.displayTitle.c_str();
		const float titleSize = bookH * 0.085f;
		const ImVec2 tc(bx + bookW * 0.5f, by + bookH * 0.40f);

		ImVec2 ts = font->CalcTextSizeA(titleSize, FLT_MAX, 0.f, title);
		ImVec2 tp = { tc.x - ts.x * 0.5f + 1.f, tc.y - ts.y * 0.5f + titleSize * 0.045f };
		dl->AddText(font, titleSize, tp, IM_COL32(12, 6, 3, 175), title);
		dl->AddText(font, titleSize, { tp.x - 1.f, tp.y - titleSize * 0.045f }, IM_COL32(210, 180, 120, 255), title);

		const ImU32 flourish = IM_COL32(196, 162, 110, 175);
		DrawFlourish(dl, { tc.x, tc.y - titleSize * 1.40f }, bookW * 0.27f, flourish);
		DrawFlourish(dl, { tc.x, tc.y + titleSize * 1.40f }, bookW * 0.27f, flourish);

		const char* year = book.config.year.c_str();
		ImVec2 ys = font->CalcTextSizeA(bookH * 0.040f, FLT_MAX, 0.f, year);
		dl->AddText(font, bookH * 0.040f, { tc.x - ys.x * 0.5f, tc.y + titleSize * 2.20f }, IM_COL32(206, 178, 136, 225), year);

		const char* author = book.config.author.c_str();
		ImVec2 as = font->CalcTextSizeA(bookH * 0.034f, FLT_MAX, 0.f, author);
		dl->AddText(font, bookH * 0.034f, { tc.x - as.x * 0.5f, by + bookH * 0.905f }, IM_COL32(206, 178, 136, 220), author);
	}

	void RenderInventory()
	{
		if (!s_inventoryOpen) return;
		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		ImVec2 ds = io.DisplaySize;
		ImFont* f = io.Fonts->Fonts[1] ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];

		dl->AddRectFilled({ 0, 0 }, ds, IM_COL32(10, 8, 5, 230));

		// Search bar at top
		{
			const float searchBarW = 400.f;
			const float searchBarH = 35.f;
			const float searchBarX = ds.x * 0.5f - searchBarW * 0.5f;
			const float searchBarY = 30.f;

			// Check if clicking on search bar
			bool clickOnSearch = io.MousePos.x >= searchBarX && io.MousePos.x <= searchBarX + searchBarW &&
			                     io.MousePos.y >= searchBarY && io.MousePos.y <= searchBarY + searchBarH;
			if (clickOnSearch && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				s_searchFocused = true;
			}
			else if (!clickOnSearch && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				s_searchFocused = false;
			}

			dl->AddRectFilled({ searchBarX, searchBarY }, { searchBarX + searchBarW, searchBarY + searchBarH }, IM_COL32(30, 25, 20, 220), 4.f);
			dl->AddRect({ searchBarX, searchBarY }, { searchBarX + searchBarW, searchBarY + searchBarH }, IM_COL32(180, 140, 90, 200), 4.f, 0, 2.f);

			const char* searchHint = WJConfig::CB_SearchHint.c_str();
			if (s_searchBuffer[0] == 0)
			{
				if (s_searchFocused)
				{
					float blink = (float)ImGui::GetTime();
					if (fmod(blink, 1.0f) < 0.5f)
					{
						ImVec2 hs = f->CalcTextSizeA(f->FontSize * 0.9f, FLT_MAX, 0.f, "|");
						dl->AddText(f, f->FontSize * 0.9f, { searchBarX + 10.f, searchBarY + 10.f }, IM_COL32(234, 223, 197, 255), "|");
					}
				}
				else
				{
					ImVec2 hs = f->CalcTextSizeA(f->FontSize * 0.9f, FLT_MAX, 0.f, searchHint);
					dl->AddText(f, f->FontSize * 0.9f, { searchBarX + 10.f, searchBarY + 10.f }, IM_COL32(120, 100, 80, 180), searchHint);
				}
			}
			else
			{
				dl->AddText(f, f->FontSize * 0.9f, { searchBarX + 10.f, searchBarY + 10.f }, IM_COL32(234, 223, 197, 255), s_searchBuffer);
				if (s_searchFocused)
				{
					float blink = (float)ImGui::GetTime();
					if (fmod(blink, 1.0f) < 0.5f)
					{
						ImVec2 textSz = f->CalcTextSizeA(f->FontSize * 0.9f, FLT_MAX, 0.f, s_searchBuffer);
						dl->AddText(f, f->FontSize * 0.9f, { searchBarX + 10.f + textSz.x, searchBarY + 10.f }, IM_COL32(234, 223, 197, 255), "|");
					}
				}
			}

			// Handle typing in search bar ONLY if focused
			if (s_searchFocused)
			{
				for (int vk = 'A'; vk <= 'Z'; ++vk)
				{
					if (ImGui::IsKeyPressed((ImGuiKey)vk, false))
					{
						int len = (int)strlen(s_searchBuffer);
						if (len < (int)sizeof(s_searchBuffer) - 1)
						{
							char c = (char)vk;
							if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
								c = (char)std::tolower((unsigned char)c);
							s_searchBuffer[len] = c;
							s_searchBuffer[len + 1] = 0;
							s_selectedBookIdx = 0;
						}
					}
				}
				if (ImGui::IsKeyPressed(ImGuiKey_Space, false))
				{
					int len = (int)strlen(s_searchBuffer);
					if (len < (int)sizeof(s_searchBuffer) - 1)
					{
						s_searchBuffer[len] = ' ';
						s_searchBuffer[len + 1] = 0;
						s_selectedBookIdx = 0;
					}
				}
				if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false))
				{
					int len = (int)strlen(s_searchBuffer);
					if (len > 0)
					{
						s_searchBuffer[len - 1] = 0;
						s_selectedBookIdx = 0;
					}
				}
			}
		}

		// Apply filter
		ApplySearchFilter();

		if (s_filteredBooks.empty())
		{
			const char* empty = s_searchBuffer[0] ? WJConfig::CB_NoMatch.c_str() : WJConfig::CB_NoBooks.c_str();
			ImVec2 es = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, empty);
			dl->AddText(f, f->FontSize, { ds.x * 0.5f - es.x * 0.5f, ds.y * 0.5f }, IM_COL32(160, 140, 100, 200), empty);
		}
		else
		{
			if (s_selectedBookIdx >= (int)s_filteredBooks.size())
				s_selectedBookIdx = 0;
			if (s_selectedBookIdx < 0)
				s_selectedBookIdx = 0;

			LoadBook(s_filteredBooks[s_selectedBookIdx]);
			auto& book = s_loadedBooks[s_filteredBooks[s_selectedBookIdx]];

			float bookW = ds.x * 0.35f;
			float bookH = ds.y * 0.70f;
			float bx = ds.x * 0.5f - bookW * 0.5f;
			float by = ds.y * 0.5f - bookH * 0.5f;

			DrawBookCover(dl, book, bx, by, bookW, bookH, 1.0f);

			float arrowSize = 40.f;
			float arrowX_left = bx - arrowSize - 20.f;
			float arrowX_right = bx + bookW + 20.f;
			float arrowY = ds.y * 0.5f;

			bool hoverLeft = io.MousePos.x >= arrowX_left && io.MousePos.x <= arrowX_left + arrowSize &&
			                 io.MousePos.y >= arrowY - arrowSize * 0.5f && io.MousePos.y <= arrowY + arrowSize * 0.5f;
			bool hoverRight = io.MousePos.x >= arrowX_right && io.MousePos.x <= arrowX_right + arrowSize &&
			                  io.MousePos.y >= arrowY - arrowSize * 0.5f && io.MousePos.y <= arrowY + arrowSize * 0.5f;

			ImU32 arrowColLeft = hoverLeft ? IM_COL32(255, 215, 0, 200) : IM_COL32(200, 180, 140, 100);
			ImU32 arrowColRight = hoverRight ? IM_COL32(255, 215, 0, 200) : IM_COL32(200, 180, 140, 100);

			dl->AddTriangleFilled(
				{ arrowX_left + arrowSize * 0.7f, arrowY - arrowSize * 0.4f },
				{ arrowX_left + arrowSize * 0.7f, arrowY + arrowSize * 0.4f },
				{ arrowX_left + arrowSize * 0.2f, arrowY },
				arrowColLeft);

			dl->AddTriangleFilled(
				{ arrowX_right + arrowSize * 0.3f, arrowY - arrowSize * 0.4f },
				{ arrowX_right + arrowSize * 0.3f, arrowY + arrowSize * 0.4f },
				{ arrowX_right + arrowSize * 0.8f, arrowY },
				arrowColRight);

			if (hoverLeft && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				if (s_selectedBookIdx > 0)
					s_selectedBookIdx--;
				else
					s_selectedBookIdx = (int)s_filteredBooks.size() - 1;
			}

			if (hoverRight && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				if (s_selectedBookIdx < (int)s_filteredBooks.size() - 1)
					s_selectedBookIdx++;
				else
					s_selectedBookIdx = 0;
			}

			float metaY = by + bookH + 20.f;
			const char* dtitle = book.config.displayTitle.c_str();
			ImVec2 dts = f->CalcTextSizeA(f->FontSize * 1.3f, FLT_MAX, 0.f, dtitle);
			dl->AddText(f, f->FontSize * 1.3f, { ds.x * 0.5f - dts.x * 0.5f, metaY }, IM_COL32(230, 205, 160, 255), dtitle);

			char meta[256];
			snprintf(meta, sizeof(meta), "by %s  |  %s",
				book.config.author.c_str(),
				book.config.category.c_str());
			ImVec2 ms = f->CalcTextSizeA(f->FontSize * 0.9f, FLT_MAX, 0.f, meta);
			dl->AddText(f, f->FontSize * 0.9f, { ds.x * 0.5f - ms.x * 0.5f, metaY + f->FontSize * 1.5f }, IM_COL32(180, 160, 120, 220), meta);

			std::string navHint = WJConfig::CB_NavHintRandom;
			if (book.config.hasIndex && !book.chapters.empty())
				navHint += "   |   I: Index";
			ImVec2 ns = f->CalcTextSizeA(f->FontSize * 0.85f, FLT_MAX, 0.f, navHint.c_str());
			dl->AddText(f, f->FontSize * 0.85f, { ds.x * 0.5f - ns.x * 0.5f, ds.y - 40.f }, IM_COL32(160, 140, 100, 200), navHint.c_str());
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
		{
			s_searchFocused = false;
			CloseInventory();
		}

		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
		{
			if (s_selectedBookIdx > 0)
				s_selectedBookIdx--;
			else if (!s_filteredBooks.empty())
				s_selectedBookIdx = (int)s_filteredBooks.size() - 1;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false))
		{
			if (s_selectedBookIdx < (int)s_filteredBooks.size() - 1)
				s_selectedBookIdx++;
			else if (!s_filteredBooks.empty())
				s_selectedBookIdx = 0;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
		{
			if (!s_filteredBooks.empty() && s_selectedBookIdx >= 0 && s_selectedBookIdx < (int)s_filteredBooks.size())
			{
				s_currentPage = 0; // Open from beginning
				OpenBook(s_filteredBooks[s_selectedBookIdx]);
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_K, false))
		{
			if (!s_filteredBooks.empty() && s_selectedBookIdx >= 0 && s_selectedBookIdx < (int)s_filteredBooks.size())
			{
				const std::string& bookName = s_filteredBooks[s_selectedBookIdx];
				auto bmIt = s_bookmarks.find(bookName);
				if (bmIt != s_bookmarks.end() && bmIt->second >= 0)
				{
					s_currentPage = bmIt->second;
				}
				else
				{
					s_currentPage = 0;
				}
				OpenBook(bookName);
				s_ribbonAnim.active = true;
				s_ribbonAnim.progress = 0.f;
				s_ribbonAnim.placing = true;
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_R, false))
		{
			if (!s_filteredBooks.empty() && s_selectedBookIdx >= 0 && s_selectedBookIdx < (int)s_filteredBooks.size())
			{
				const std::string& bookName = s_filteredBooks[s_selectedBookIdx];
				LoadBook(bookName);
				auto& book = s_loadedBooks[bookName];
				std::string fullText;
				for (const auto& line : book.lines)
				{
					if (!fullText.empty()) fullText += "\n";
					fullText += line;
				}
				ImFont* font = io.Fonts->Fonts[1] ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];
				float fontSize = font->FontSize + book.config.fontSizeOverride;
				if (fontSize < 10.f) fontSize = 10.f;
				float bookW = ds.x * 0.6f;
				float margin = 30.f;
				float pageW = bookW * 0.5f - margin * 1.5f;
				auto wrapped = WrapText(fullText, pageW, font, fontSize);
				int linesPerPage = (int)((ds.y * 0.75f - margin * 2.f) / (fontSize * 1.6f));
				if (linesPerPage < 1) linesPerPage = 1;
				int totalPages = (int)wrapped.size() / linesPerPage + 1;
				s_currentPage = rand() % totalPages;
				OpenBook(bookName);
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_I, false))
		{
			if (!s_filteredBooks.empty() && s_selectedBookIdx >= 0 && s_selectedBookIdx < (int)s_filteredBooks.size())
			{
				const std::string& bookName = s_filteredBooks[s_selectedBookIdx];
				LoadBook(bookName);
				auto& book = s_loadedBooks[bookName];
				if (book.config.hasIndex && !book.chapters.empty())
				{
					s_currentBook = bookName;
					CloseInventory();
					OpenIndex();
				}
			}
		}
	}

	static std::vector<std::string> WrapText(const std::string& text, float maxWidth, ImFont* f, float fontSize)
	{
		std::vector<std::string> wrappedLines;
		std::istringstream stream(text);
		std::string line;
		while (std::getline(stream, line))
		{
			if (line.empty())
			{
				wrappedLines.push_back("");
				continue;
			}

			std::istringstream wordStream(line);
			std::string word;
			std::string currentLine;
			while (wordStream >> word)
			{
				std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
				ImVec2 testSize = f->CalcTextSizeA(fontSize, FLT_MAX, 0.f, testLine.c_str());
				if (testSize.x > maxWidth && !currentLine.empty())
				{
					wrappedLines.push_back(currentLine);
					currentLine = word;
				}
				else
				{
					currentLine = testLine;
				}
			}
			if (!currentLine.empty())
			{
				wrappedLines.push_back(currentLine);
			}
		}
		return wrappedLines;
	}

	void RenderBook()
	{
		if (!s_bookOpen) return;
		auto it = s_loadedBooks.find(s_currentBook);
		if (it == s_loadedBooks.end()) return;
		auto& book = it->second;

		if (book.edits.empty())
			LoadEdits(s_currentBook);

		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		ImVec2 ds = io.DisplaySize;

		dl->AddRectFilled({ 0, 0 }, ds, IM_COL32(0, 0, 0, 200));

		float bookW = ds.x * 0.6f;
		float bookH = ds.y * 0.75f;
		float bx = ds.x * 0.5f - bookW * 0.5f;
		float by = ds.y * 0.5f - bookH * 0.5f;

		ImU32 coverCol = IM_COL32(book.config.coverColorRGB[0], book.config.coverColorRGB[1], book.config.coverColorRGB[2], 255);
		ImU32 pageCol = IM_COL32(245, 235, 220, 255);

		dl->AddRectFilled({ bx - 4, by - 4 }, { bx + bookW + 4, by + bookH + 4 }, coverCol, 6.f);
		dl->AddRectFilled({ bx, by }, { bx + bookW, by + bookH }, pageCol, 3.f);

		float spineX = bx + bookW * 0.5f - 2.f;
		dl->AddRectFilled({ spineX, by }, { spineX + 4, by + bookH }, IM_COL32(180, 170, 155, 255));

		float margin = 30.f;
		float pageW = bookW * 0.5f - margin * 1.5f;
		float pageTextH = bookH - margin * 2.f;

		ImFont* f = io.Fonts->Fonts[1] ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];
		float fontSize = f->FontSize + book.config.fontSizeOverride;
		if (fontSize < 10.f) fontSize = 10.f;

		ImU32 inkCol = GetInkColor(book.config.inkColor);
		float lineH = fontSize * 1.6f;
		int linesPerPage = (int)(pageTextH / lineH);
		if (linesPerPage < 1) linesPerPage = 1;

		if (book.lazyTotalChars > LAZY_CHAR_THRESHOLD)
		{
			int targetLine = s_currentPage * linesPerPage * 2;
			if (targetLine < book.lazyStartLine || targetLine >= book.lazyStartLine + book.lazyLoadedCount)
			{
				LoadChunk(book, targetLine);
			}
		}

		std::string fullText;
		for (const auto& line : book.lines)
		{
			if (!fullText.empty()) fullText += "\n";
			fullText += line;
		}

		std::vector<std::string> wrappedLines = WrapText(fullText, pageW, f, fontSize);

		int leftPage = s_currentPage * 2;
		int rightPage = leftPage + 1;

		int totalWrappedPages = (int)wrappedLines.size() / linesPerPage + 1;
		if (book.lazyTotalChars > LAZY_CHAR_THRESHOLD)
		{
			int totalLinesEstimate = book.lazyTotalLines;
			totalWrappedPages = (totalLinesEstimate + linesPerPage - 1) / linesPerPage;
		}

		bool leftRipped = IsPageRipped(s_currentBook, leftPage);
		bool rightRipped = IsPageRipped(s_currentBook, rightPage);

		float leftX = bx + margin;
		float rightX = bx + bookW * 0.5f + margin * 0.5f;

		ImVec2 leftMin{ leftX, by + margin };
		ImVec2 leftMax{ leftX + pageW, by + bookH - margin };
		ImVec2 rightMin{ rightX, by + margin };
		ImVec2 rightMax{ rightX + pageW, by + bookH - margin };

		auto drawPage = [&](int pageIdx, float px, float py, float pw, bool ripped)
		{
			if (ripped)
			{
				bool isLeft = (pageIdx % 2 == 0);
				ImVec2 pgMin{ px, py };
				ImVec2 pgMax{ px + pw, py + bookH - margin * 2.f };
				DrawRippedPageSlot(dl, pgMin, pgMax, isLeft);

				const char* msg = WJConfig::Sheet_PageRipped.c_str();
				ImVec2 msz = f->CalcTextSizeA(fontSize * 1.1f, FLT_MAX, 0.f, msg);
				dl->AddText(f, fontSize * 1.1f,
					{ px + pw * 0.5f - msz.x * 0.5f, py + (bookH - margin * 2.f) * 0.5f - msz.y * 0.5f },
					IM_COL32(76, 62, 48, 200), msg);
				return;
			}

			int startLine = pageIdx * linesPerPage - book.lazyStartLine;
			if (startLine < 0 || startLine >= (int)wrappedLines.size()) return;
			float ty = py;

			int absLineOffset = book.lazyStartLine;

			for (int l = 0; l < linesPerPage && (startLine + l) < (int)wrappedLines.size(); ++l)
			{
				const std::string& line = wrappedLines[startLine + l];
				if (line.empty()) { ty += lineH; continue; }
				float tx = px;
				if (book.config.textAlignment == 1)
				{
					ImVec2 ls = f->CalcTextSizeA(fontSize, pw, 0.f, line.c_str());
					tx = px + pw * 0.5f - ls.x * 0.5f;
				}

				int actualLineIdx = absLineOffset + startLine + l;
				bool hasEdit = false;
				for (const auto& edit : book.edits)
				{
					if (edit.lineIndex == actualLineIdx)
					{
						hasEdit = true;

						std::string before = line.substr(0, edit.startChar);
						std::string target = line.substr(edit.startChar, edit.endChar - edit.startChar);
						std::string after = line.substr(edit.endChar);

						float beforeW = 0.f;
						if (!before.empty())
						{
							ImVec2 bsz = f->CalcTextSizeA(fontSize, FLT_MAX, 0.f, before.c_str());
							beforeW = bsz.x;
							dl->AddText(f, fontSize, { tx, ty }, inkCol, before.c_str());
						}

						float targetW = 0.f;
						if (!target.empty())
						{
							ImVec2 tsz = f->CalcTextSizeA(fontSize, FLT_MAX, 0.f, target.c_str());
							targetW = tsz.x;
							float strikeY = ty + fontSize * 0.5f;
							dl->AddText(f, fontSize, { tx + beforeW, ty }, IM_COL32(120, 100, 80, 180), target.c_str());
							dl->AddLine({ tx + beforeW, strikeY }, { tx + beforeW + targetW, strikeY }, IM_COL32(120, 50, 30, 220), 1.5f);
						}

						if (!edit.replacementText.empty())
						{
							float smallSize = fontSize * 0.6f;
							float repY = ty - smallSize * 0.8f;
							dl->AddText(f, smallSize, { tx + beforeW, repY }, IM_COL32(48, 38, 30, 230), edit.replacementText.c_str());
						}

						float afterX = tx + beforeW + targetW;
						if (!after.empty())
						{
							dl->AddText(f, fontSize, { afterX, ty }, inkCol, after.c_str());
						}
						break;
					}
				}

				if (!hasEdit)
				{
					dl->AddText(f, fontSize, { tx, ty }, inkCol, line.c_str(), nullptr, pw);
				}

				ty += lineH;
			}
			char pnum[16];
			snprintf(pnum, sizeof(pnum), "- %d -", pageIdx + 1);
			ImVec2 pns = f->CalcTextSizeA(fontSize * 0.8f, FLT_MAX, 0.f, pnum);
			dl->AddText(f, fontSize * 0.8f, { px + pw * 0.5f - pns.x * 0.5f, py + bookH - margin * 2.f - fontSize }, IM_COL32(150, 140, 130, 200), pnum);
		};

		drawPage(leftPage, leftX, by + margin, pageW, leftRipped);
		if (rightPage < totalWrappedPages)
			drawPage(rightPage, rightX, by + margin, pageW, rightRipped);

		if (s_cbSelectedPage >= 0)
		{
			const float pulse = 0.5f + 0.5f * std::sin((float)ImGui::GetTime() * 4.0f);
			if (s_cbSelectedPage == leftPage && !leftRipped)
				DrawPageGlow(dl, { leftX, by + margin }, { leftX + pageW, by + bookH - margin }, pulse);
			else if (s_cbSelectedPage == rightPage && !rightRipped)
				DrawPageGlow(dl, { rightX, by + margin }, { rightX + pageW, by + bookH - margin }, pulse);
		}

		const char* btitle = book.config.displayTitle.c_str();
		ImVec2 bts = f->CalcTextSizeA(f->FontSize * 1.2f, FLT_MAX, 0.f, btitle);
		dl->AddText(f, f->FontSize * 1.2f, { ds.x * 0.5f - bts.x * 0.5f, by - f->FontSize * 2.f }, IM_COL32(234, 223, 197, 255), btitle);

		if (s_ribbonAnim.active)
		{
			float t = s_ribbonAnim.progress;
			float ribbonT = (t < 0.25f) ? (t / 0.25f) : 1.f;
			ribbonT = 1.f - (1.f - ribbonT) * (1.f - ribbonT);
			float ribbonH = bookH * ribbonT;
			float ribbonW = 12.f;
			float ribbonX = bx + bookW * 0.5f - ribbonW * 0.5f;
			float ribbonY = by + (bookH - ribbonH) * 0.5f;

			const ImVec2 rbPts[5] = {
				{ ribbonX, by - 5.f },
				{ ribbonX + ribbonW, by - 5.f },
				{ ribbonX + ribbonW, ribbonY + ribbonH },
				{ ribbonX + ribbonW * 0.5f, ribbonY + ribbonH - ribbonW * 0.85f },
				{ ribbonX, ribbonY + ribbonH }
			};
			dl->AddConvexPolyFilled(rbPts, 5, IM_COL32(116, 28, 24, 240));
			dl->AddPolyline(rbPts, 5, IM_COL32(84, 18, 15, 240), ImDrawFlags_None, 1.2f);

			if (t >= 0.15f && t <= 0.85f)
			{
				float textAlpha = 1.f;
				if (t < 0.3f) textAlpha = (t - 0.15f) / 0.15f;
				else if (t > 0.7f) textAlpha = (0.85f - t) / 0.15f;
				const char* bmMsg = s_ribbonAnim.placing ? WJConfig::BookmarkSaved.c_str() : WJConfig::BookmarkRemoved.c_str();
				ImVec2 msgSz = f->CalcTextSizeA(f->FontSize * 1.4f, FLT_MAX, 0.f, bmMsg);
				dl->AddText(f, f->FontSize * 1.4f, { ds.x * 0.5f - msgSz.x * 0.5f, by - f->FontSize * 3.f },
					IM_COL32(234, 223, 197, (int)(255 * textAlpha)), bmMsg);
			}
		}

		std::string hintStr = WJConfig::CB_BookNavHint;
		if (s_cbSelectedPage >= 0)
		{
			hintStr += "   |   P: Rip Page   |   E: Edit   |   ESC: Deselect";
		}
		ImVec2 hs = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, hintStr.c_str());
		dl->AddText(f, f->FontSize, { ds.x * 0.5f - hs.x * 0.5f, ds.y * 0.93f }, IM_COL32(200, 200, 200, 200), hintStr.c_str());

		if (s_cbEditMode)
		{
			const float editW = 300.f;
			const float editH = 30.f;
			ImVec2 editPos{ ds.x * 0.5f - editW * 0.5f, ds.y * 0.15f };
			dl->AddRectFilled(editPos, { editPos.x + editW, editPos.y + editH }, IM_COL32(30, 25, 20, 240), 4.f);
			dl->AddRect(editPos, { editPos.x + editW, editPos.y + editH }, IM_COL32(180, 140, 90, 200), 4.f, 0, 2.f);
			dl->AddText(f, fontSize * 0.8f, { editPos.x + 8.f, editPos.y + 8.f }, IM_COL32(234, 223, 197, 255), s_cbEditBuffer);

			const char* editHint = "Type replacement   |   ENTER: Confirm   |   ESC: Cancel";
			ImVec2 ehs = f->CalcTextSizeA(f->FontSize * 0.8f, FLT_MAX, 0.f, editHint);
			dl->AddText(f, f->FontSize * 0.8f, { ds.x * 0.5f - ehs.x * 0.5f, editPos.y + editH + 8.f }, IM_COL32(200, 180, 140, 200), editHint);
		}

		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false))
		{
			if (s_cbSelectedPage >= 0)
			{
				if (s_cbSelectedPage == leftPage && !rightRipped)
					s_cbSelectedPage = rightPage;
				else if (s_cbSelectedPage == rightPage)
				{
					int maxPage = totalWrappedPages / 2;
					if (s_currentPage < maxPage)
					{
						s_currentPage++;
						s_cbSelectedPage = s_currentPage * 2;
					}
				}
			}
			else
			{
				int maxPage = totalWrappedPages / 2;
				if (s_currentPage < maxPage)
					s_currentPage++;
			}
		}
		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
		{
			if (s_cbSelectedPage >= 0)
			{
				if (s_cbSelectedPage == rightPage && !leftRipped)
					s_cbSelectedPage = leftPage;
				else if (s_cbSelectedPage == leftPage)
				{
					if (s_currentPage > 0)
					{
						s_currentPage--;
						s_cbSelectedPage = s_currentPage * 2 + 1;
					}
				}
			}
			else
			{
				if (s_currentPage > 0) s_currentPage--;
			}
		}

		if (s_cbSelectedPage < 0)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				ImVec2 mp = io.MousePos;
				bool clickLeft = mp.x >= leftX && mp.x <= leftX + pageW && mp.y >= by + margin && mp.y <= by + bookH - margin;
				bool clickRight = mp.x >= rightX && mp.x <= rightX + pageW && mp.y >= by + margin && mp.y <= by + bookH - margin;

				if (clickLeft && !leftRipped)
					s_cbSelectedPage = leftPage;
				else if (clickRight && !rightRipped)
					s_cbSelectedPage = rightPage;
			}
		}
		else
		{
			if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			{
				s_cbSelectedPage = -1;
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_P, false))
			{
				if (!IsPageRipped(s_currentBook, s_cbSelectedPage))
				{
					int partner = Sheets::GetPartnerPage(s_cbSelectedPage);
					std::string pageText;
					int startLine = s_cbSelectedPage * linesPerPage;
					for (int l = 0; l < linesPerPage && (startLine + l) < (int)wrappedLines.size(); ++l)
					{
						if (!pageText.empty()) pageText += "\n";
						pageText += wrappedLines[startLine + l];
					}

					std::string backText;
					if (partner > 0 && !IsPageRipped(s_currentBook, partner))
					{
						int partnerStartLine = partner * linesPerPage;
						for (int l = 0; l < linesPerPage && (partnerStartLine + l) < (int)wrappedLines.size(); ++l)
						{
							if (!backText.empty()) backText += "\n";
							backText += wrappedLines[partnerStartLine + l];
						}
					}

					Sheets::StartRipPage(pageText, SheetDrawing(), s_cbSelectedPage, false, s_currentBook, 1, backText, SheetDrawing());
				}
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_E, false))
			{
				s_cbEditMode = true;
				s_cbEditBuffer[0] = '\0';
			}
		}

		if (s_cbEditMode)
		{
			for (int vk = 'A'; vk <= 'Z'; ++vk)
			{
				if (ImGui::IsKeyPressed((ImGuiKey)vk, false))
				{
					int len = (int)strlen(s_cbEditBuffer);
					if (len < (int)sizeof(s_cbEditBuffer) - 1)
					{
						char c = (char)vk;
						if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
							c = (char)std::tolower((unsigned char)c);
						s_cbEditBuffer[len] = c;
						s_cbEditBuffer[len + 1] = 0;
					}
				}
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false))
			{
				int len = (int)strlen(s_cbEditBuffer);
				if (len > 0)
					s_cbEditBuffer[len - 1] = 0;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Space, false))
			{
				int len = (int)strlen(s_cbEditBuffer);
				if (len < (int)sizeof(s_cbEditBuffer) - 1)
				{
					s_cbEditBuffer[len] = ' ';
					s_cbEditBuffer[len + 1] = 0;
				}
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
			{
				if (strlen(s_cbEditBuffer) > 0 && s_cbSelectedPage >= 0)
				{
					TextEdit edit;
					int startLine = s_cbSelectedPage * linesPerPage;
					edit.lineIndex = book.lazyStartLine + startLine;
					edit.startChar = 0;
					edit.endChar = 10;
					edit.originalText = "text";
					edit.replacementText = s_cbEditBuffer;
					AddEdit(s_currentBook, edit);
				}
				s_cbEditMode = false;
				s_cbEditBuffer[0] = '\0';
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			{
				s_cbEditMode = false;
				s_cbEditBuffer[0] = '\0';
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_K, false) && s_cbSelectedPage < 0)
		{
			auto bmIt = s_bookmarks.find(s_currentBook);
			if (bmIt != s_bookmarks.end() && bmIt->second == s_currentPage)
			{
				s_bookmarks.erase(s_currentBook);
				s_ribbonAnim.active = true;
				s_ribbonAnim.progress = 0.f;
				s_ribbonAnim.placing = false;
			}
			else
			{
				s_bookmarks[s_currentBook] = s_currentPage;
				s_ribbonAnim.active = true;
				s_ribbonAnim.progress = 0.f;
				s_ribbonAnim.placing = true;
			}
		}
		if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && s_cbSelectedPage < 0 && !s_cbEditMode)
		{
			CloseBook();
			OpenInventory();
		}

		if (s_ribbonAnim.active)
		{
			s_ribbonAnim.progress += io.DeltaTime / 2.5f;
			if (s_ribbonAnim.progress >= 1.f)
				s_ribbonAnim.active = false;
		}
	}

	void SetBookOwned(const std::string& internalName, bool owned)
	{
		fs::path configPath = GetBooksDir() / internalName / "config.ini";
		if (!fs::exists(configPath)) return;

		std::ifstream in(configPath);
		if (!in) return;
		std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		in.close();

		size_t pos = content.find("isOwned=");
		if (pos != std::string::npos)
		{
			size_t endLine = content.find('\n', pos);
			std::string newVal = owned ? "isOwned=1" : "isOwned=0";
			if (endLine != std::string::npos)
				content.replace(pos, endLine - pos, newVal);
			else
				content.replace(pos, content.size() - pos, newVal);

			std::ofstream out(configPath, std::ios::trunc);
			if (out) out << content;
		}

		if (owned)
		{
			if (std::find(s_availableBooks.begin(), s_availableBooks.end(), internalName) == s_availableBooks.end())
			{
				s_availableBooks.push_back(internalName);
			}
		}
		else
		{
			auto it = std::find(s_availableBooks.begin(), s_availableBooks.end(), internalName);
			if (it != s_availableBooks.end())
			{
				s_availableBooks.erase(it);
				if (s_selectedBookIdx >= (int)s_availableBooks.size())
					s_selectedBookIdx = std::max(0, (int)s_availableBooks.size() - 1);
			}
		}
		s_ownedBooks[internalName] = owned;
	}

	bool IsBookOwned(const std::string& internalName)
	{
		auto it = s_ownedBooks.find(internalName);
		if (it != s_ownedBooks.end())
			return it->second;

		fs::path configPath = GetBooksDir() / internalName / "config.ini";
		if (!fs::exists(configPath)) return false;

		BookConfig cfg;
		ParseConfig(configPath, cfg);
		s_ownedBooks[internalName] = cfg.isOwned;
		return cfg.isOwned;
	}

	void UpdatePickupPrompt(float playerX, float playerY, float playerZ)
	{
		s_nearbyBook.clear();
		s_showPickupPrompt = false;

		for (const auto& bookName : s_availableBooks)
		{
			LoadBook(bookName);
			auto& book = s_loadedBooks[bookName];
			if (!book.config.findable) continue;
			if (IsBookOwned(bookName)) continue;

			float dx = playerX - book.config.locationX;
			float dy = playerY - book.config.locationY;
			float dz = playerZ - book.config.locationZ;
			float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

			if (dist <= book.config.pickupRadius)
			{
				s_nearbyBook = bookName;
				s_showPickupPrompt = true;
				break;
			}
		}
	}

	bool TryPickupBook(float playerX, float playerY, float playerZ)
	{
		if (!s_showPickupPrompt || s_nearbyBook.empty()) return false;

		LoadBook(s_nearbyBook);
		auto& book = s_loadedBooks[s_nearbyBook];

		float dx = playerX - book.config.locationX;
		float dy = playerY - book.config.locationY;
		float dz = playerZ - book.config.locationZ;
		float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

		if (dist <= book.config.pickupRadius)
		{
			SetBookOwned(s_nearbyBook, true);
			s_showPickupPrompt = false;
			s_nearbyBook.clear();
			return true;
		}
		return false;
	}

	bool IsNearPickup()
	{
		return s_showPickupPrompt;
	}

	const std::string& GetPickupMessage()
	{
		if (!s_nearbyBook.empty())
		{
			LoadBook(s_nearbyBook);
			return s_loadedBooks[s_nearbyBook].config.pickupMessage;
		}
		static std::string empty;
		return empty;
	}

	void RenderPickupPrompt()
	{
		if (!s_showPickupPrompt || s_nearbyBook.empty()) return;

		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		ImVec2 ds = io.DisplaySize;
		ImFont* f = io.Fonts->Fonts[1] ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];

		const char* msg = GetPickupMessage().c_str();
		ImVec2 msgSz = f->CalcTextSizeA(f->FontSize * 1.2f, FLT_MAX, 0.f, msg);
		float x = ds.x * 0.5f - msgSz.x * 0.5f;
		float y = ds.y * 0.75f;

		dl->AddText(f, f->FontSize * 1.2f, { x, y }, IM_COL32(234, 223, 197, 255), msg);
	}

	void OpenIndex()
	{
		s_indexOpen = true;
		s_selectedChapterIdx = 0;
	}

	void CloseIndex()
	{
		s_indexOpen = false;
	}

	bool IsIndexOpen()
	{
		return s_indexOpen;
	}

	void RenderIndex()
	{
		if (!s_indexOpen || s_currentBook.empty()) return;

		auto it = s_loadedBooks.find(s_currentBook);
		if (it == s_loadedBooks.end()) return;
		auto& book = it->second;

		if (book.chapters.empty()) return;

		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		ImVec2 ds = io.DisplaySize;
		ImFont* f = io.Fonts->Fonts[1] ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];

		dl->AddRectFilled({ 0, 0 }, ds, IM_COL32(0, 0, 0, 200));

		float bookW = ds.x * 0.6f;
		float bookH = ds.y * 0.75f;
		float bx = ds.x * 0.5f - bookW * 0.5f;
		float by = ds.y * 0.5f - bookH * 0.5f;

		ImU32 coverCol = IM_COL32(book.config.coverColorRGB[0], book.config.coverColorRGB[1], book.config.coverColorRGB[2], 255);
		ImU32 pageCol = IM_COL32(245, 235, 220, 255);

		dl->AddRectFilled({ bx - 4, by - 4 }, { bx + bookW + 4, by + bookH + 4 }, coverCol, 6.f);
		dl->AddRectFilled({ bx, by }, { bx + bookW, by + bookH }, pageCol, 3.f);

		float spineX = bx + bookW * 0.5f - 2.f;
		dl->AddRectFilled({ spineX, by }, { spineX + 4, by + bookH }, IM_COL32(180, 170, 155, 255));

		float margin = 30.f;
		float pageW = bookW * 0.5f - margin * 1.5f;

		const char* title = "Index";
		ImVec2 ts = f->CalcTextSizeA(f->FontSize * 1.8f, FLT_MAX, 0.f, title);
		dl->AddText(f, f->FontSize * 1.8f, { bx + margin + pageW * 0.5f - ts.x * 0.5f, by + margin }, IM_COL32(48, 38, 30, 255), title);

		float startY = by + margin + f->FontSize * 2.5f;
		float lineHeight = f->FontSize * 2.2f;
		float maxVisible = (bookH - margin * 2.f - f->FontSize * 3.f) / lineHeight;
		int visibleCount = std::min((int)book.chapters.size(), (int)maxVisible);

		int startIdx = std::max(0, s_selectedChapterIdx - (int)(maxVisible / 2));
		if (startIdx + visibleCount > (int)book.chapters.size())
			startIdx = std::max(0, (int)book.chapters.size() - visibleCount);

		float leftX = bx + margin;

		for (int i = 0; i < visibleCount; ++i)
		{
			int chapterIdx = startIdx + i;
			if (chapterIdx >= (int)book.chapters.size()) break;

			const auto& chapter = book.chapters[chapterIdx];
			float y = startY + i * lineHeight;
			bool isSelected = (chapterIdx == s_selectedChapterIdx);

			if (isSelected)
			{
				dl->AddRectFilled({ leftX, y - 2.f }, { leftX + pageW * 2.f, y + lineHeight - 4.f }, IM_COL32(210, 200, 175, 100), 3.f);
			}

			ImU32 textCol = isSelected ? IM_COL32(48, 38, 30, 255) : IM_COL32(76, 62, 48, 200);
			float fontSize = isSelected ? f->FontSize * 1.2f : f->FontSize * 1.0f;

			dl->AddText(f, fontSize, { leftX + 10.f, y }, textCol, chapter.title.c_str());
		}

		const char* hint = "Arrows: Navigate   |   Enter: Go   |   ESC: Close";
		ImVec2 hs = f->CalcTextSizeA(f->FontSize * 0.9f, FLT_MAX, 0.f, hint);
		dl->AddText(f, f->FontSize * 0.9f, { ds.x * 0.5f - hs.x * 0.5f, by + bookH - margin - f->FontSize }, IM_COL32(150, 140, 130, 200), hint);

		if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			CloseIndex();

		if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, false))
		{
			if (s_selectedChapterIdx > 0)
				s_selectedChapterIdx--;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, false))
		{
			if (s_selectedChapterIdx < (int)book.chapters.size() - 1)
				s_selectedChapterIdx++;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
		{
			if (s_selectedChapterIdx >= 0 && s_selectedChapterIdx < (int)book.chapters.size())
			{
				int targetLine = book.chapters[s_selectedChapterIdx].lineIndex;
				int linesPerPage = 12;
				s_currentPage = targetLine / (linesPerPage * 2);

				if (book.lazyTotalChars > LAZY_CHAR_THRESHOLD)
				{
					LoadChunk(book, targetLine);
				}

				CloseIndex();
			}
		}
	}

	void MarkChapterAsRipped(const std::string& bookName, int lineIndex)
	{
		auto it = s_loadedBooks.find(bookName);
		if (it == s_loadedBooks.end()) return;
		auto& book = it->second;

		for (auto& chapter : book.chapters)
		{
			if (chapter.lineIndex == lineIndex)
			{
				chapter.title = WJConfig::Sheet_RippedChapter + " (" + chapter.title + ")";
				break;
			}
		}

		fs::path indexPath = GetBooksDir() / bookName / "index.json";
		std::ofstream out(indexPath, std::ios::trunc);
		if (out)
		{
			out << "{\n  \"chapters\": [\n";
			for (size_t i = 0; i < book.chapters.size(); ++i)
			{
				out << "    {\"title\": \"" << book.chapters[i].title << "\", \"line\": " << book.chapters[i].lineIndex << "}";
				if (i < book.chapters.size() - 1) out << ",";
				out << "\n";
			}
			out << "  ]\n}";
		}
	}

	int GetNextValidLineIndex(const std::string& bookName, int lineIndex)
	{
		auto it = s_loadedBooks.find(bookName);
		if (it == s_loadedBooks.end()) return lineIndex;
		auto& book = it->second;

		for (size_t i = 0; i < book.chapters.size(); ++i)
		{
			if (book.chapters[i].lineIndex == lineIndex)
			{
				if (i + 1 < book.chapters.size())
					return book.chapters[i + 1].lineIndex;
				break;
			}
		}
		return lineIndex;
	}

	bool IsPageRipped(const std::string& bookName, int page)
	{
		auto it = s_rippedCustomBookPages.find(bookName);
		if (it == s_rippedCustomBookPages.end()) return false;
		return it->second.count(page) > 0;
	}

	void RipPage(const std::string& bookName, int page)
	{
		s_rippedCustomBookPages[bookName].insert(page);
		int partner = Sheets::GetPartnerPage(page);
		if (partner > 0)
			s_rippedCustomBookPages[bookName].insert(partner);
	}

	void RestorePage(const std::string& bookName, int page)
	{
		auto it = s_rippedCustomBookPages.find(bookName);
		if (it == s_rippedCustomBookPages.end()) return;
		it->second.erase(page);
		int partner = Sheets::GetPartnerPage(page);
		if (partner > 0)
			it->second.erase(partner);
	}

	void LoadEdits(const std::string& bookName)
	{
		auto it = s_loadedBooks.find(bookName);
		if (it == s_loadedBooks.end()) return;
		auto& book = it->second;
		book.edits.clear();

		fs::path editsPath = GetBooksDir() / bookName / "edits.txt";
		std::ifstream f(editsPath);
		if (!f) return;

		std::string line;
		while (std::getline(f, line))
		{
			line = Trim(line);
			if (line.empty() || line[0] == ';') continue;

			TextEdit edit;
			size_t pos1 = line.find('|');
			if (pos1 == std::string::npos) continue;
			edit.lineIndex = std::stoi(Trim(line.substr(0, pos1)));

			size_t pos2 = line.find('|', pos1 + 1);
			if (pos2 == std::string::npos) continue;
			edit.startChar = std::stoi(Trim(line.substr(pos1 + 1, pos2 - pos1 - 1)));

			size_t pos3 = line.find('|', pos2 + 1);
			if (pos3 == std::string::npos) continue;
			edit.endChar = std::stoi(Trim(line.substr(pos2 + 1, pos3 - pos2 - 1)));

			size_t pos4 = line.find('|', pos3 + 1);
			if (pos4 == std::string::npos) continue;
			edit.originalText = Trim(line.substr(pos3 + 1, pos4 - pos3 - 1));
			edit.replacementText = Trim(line.substr(pos4 + 1));

			book.edits.push_back(edit);
		}
	}

	void SaveEdits(const std::string& bookName)
	{
		auto it = s_loadedBooks.find(bookName);
		if (it == s_loadedBooks.end()) return;
		auto& book = it->second;

		fs::path editsPath = GetBooksDir() / bookName / "edits.txt";
		std::ofstream f(editsPath, std::ios::trunc);
		if (!f) return;

		for (const auto& edit : book.edits)
		{
			f << edit.lineIndex << "|" << edit.startChar << "|" << edit.endChar
			  << "|" << edit.originalText << "|" << edit.replacementText << "\n";
		}
	}

	void AddEdit(const std::string& bookName, const TextEdit& edit)
	{
		auto it = s_loadedBooks.find(bookName);
		if (it == s_loadedBooks.end()) return;
		it->second.edits.push_back(edit);
		SaveEdits(bookName);
	}

	static void DrawPageGlow(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float pulse)
	{
		const int alpha = (int)(130.f + 60.f * pulse);
		const ImU32 glow = IM_COL32(0, 180, 255, std::clamp(alpha, 0, 255));
		dl->AddRect({ mn.x - 3.f, mn.y - 3.f }, { mx.x + 3.f, mx.y + 3.f }, glow, 4.f, 0, 3.f);
		dl->AddRect({ mn.x - 7.f, mn.y - 7.f }, { mx.x + 7.f, mx.y + 7.f },
		            IM_COL32(0, 180, 255, (int)((130.f + 60.f * pulse) * 0.4f)), 5.f, 0, 1.5f);
	}

	static void DrawRippedPageSlot(ImDrawList* dl, ImVec2 pgMin, ImVec2 pgMax, bool isLeft)
	{
		unsigned seed = isLeft ? 1234u : 5678u;
		float jagSize = 6.f;
		float step = 10.f;

		auto rng = [](unsigned& s) -> float {
			s = s * 1664525u + 1013904223u;
			return (float)((s >> 8) & 0xFFFFFF) / (float)0xFFFFFF;
		};

		dl->AddRectFilled(pgMin, pgMax, IM_COL32(180, 165, 135, 120));

		std::vector<ImVec2> tornEdge;
		if (isLeft)
		{
			tornEdge.push_back(pgMax);
			tornEdge.push_back({ pgMax.x, pgMin.y });
			for (float y = pgMin.y; y <= pgMax.y; y += step)
			{
				float jx = pgMax.x - (rng(seed) * jagSize);
				tornEdge.push_back({ jx, y });
			}
		}
		else
		{
			tornEdge.push_back(pgMin);
			tornEdge.push_back({ pgMin.x, pgMax.y });
			for (float y = pgMax.y; y >= pgMin.y; y -= step)
			{
				float jx = pgMin.x + (rng(seed) * jagSize);
				tornEdge.push_back({ jx, y });
			}
		}

		if (tornEdge.size() >= 3)
			dl->AddConvexPolyFilled(tornEdge.data(), (int)tornEdge.size(), IM_COL32(210, 200, 175, 200));
	}
}
