#define NOMINMAX
#include "custombooks.h"
#include "config.h"
#include "imgui/imgui.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;

namespace CustomBooks
{
	static std::vector<std::string> WrapText(const std::string& text, float maxWidth, ImFont* f, float fontSize);

	static std::vector<std::string> s_availableBooks;
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
					if (tempCfg.isOwned)
					{
						s_availableBooks.push_back(name);
						s_ownedBooks[name] = true;
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
		auto& book = s_loadedBooks[internalName];
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
			const char* label = "Opening Satchel...";
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
		ImU32 coverCol = IM_COL32(book.config.coverColorRGB[0], book.config.coverColorRGB[1], book.config.coverColorRGB[2], 255);
		ImU32 darkCover = IM_COL32(
			(int)(book.config.coverColorRGB[0] * 0.6f),
			(int)(book.config.coverColorRGB[1] * 0.6f),
			(int)(book.config.coverColorRGB[2] * 0.6f), 255);

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

		if (s_availableBooks.empty())
		{
			const char* empty = "No books found in MyJourney/Books/";
			ImVec2 es = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, empty);
			dl->AddText(f, f->FontSize, { ds.x * 0.5f - es.x * 0.5f, ds.y * 0.5f }, IM_COL32(160, 140, 100, 200), empty);
		}
		else
		{
			if (s_selectedBookIdx >= (int)s_availableBooks.size())
				s_selectedBookIdx = 0;
			if (s_selectedBookIdx < 0)
				s_selectedBookIdx = 0;

			LoadBook(s_availableBooks[s_selectedBookIdx]);
			auto& book = s_loadedBooks[s_availableBooks[s_selectedBookIdx]];

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
					s_selectedBookIdx = (int)s_availableBooks.size() - 1;
			}

			if (hoverRight && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				if (s_selectedBookIdx < (int)s_availableBooks.size() - 1)
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

			// Bookmark menu
			auto bmIt = s_bookmarks.find(s_currentBook);
			bool hasBookmark = (bmIt != s_bookmarks.end() && bmIt->second >= 0);
			if (hasBookmark || true) // Always show menu
			{
				const float bmMenuY = metaY + f->FontSize * 3.f;
				const float bmItemH = f->FontSize * 1.5f;
				const float bmMenuW = 280.f;
				const float bmMenuX = ds.x * 0.5f - bmMenuW * 0.5f;

				dl->AddRectFilled({ bmMenuX, bmMenuY }, { bmMenuX + bmMenuW, bmMenuY + bmItemH * 3.2f }, IM_COL32(20, 15, 10, 200), 6.f);
				dl->AddRect({ bmMenuX, bmMenuY }, { bmMenuX + bmMenuW, bmMenuY + bmItemH * 3.2f }, IM_COL32(180, 140, 90, 180), 6.f, 0, 2.f);

				dl->AddText(f, f->FontSize * 0.95f, { bmMenuX + 12.f, bmMenuY + 8.f }, IM_COL32(234, 223, 197, 255), "Open:");

				if (hasBookmark)
				{
					char bmText[64];
					snprintf(bmText, sizeof(bmText), "K: Bookmark (Page %d)", bmIt->second + 1);
					dl->AddText(f, f->FontSize * 0.85f, { bmMenuX + 12.f, bmMenuY + bmItemH + 8.f }, IM_COL32(255, 215, 0, 255), bmText);
				}
				else
				{
					dl->AddText(f, f->FontSize * 0.85f, { bmMenuX + 12.f, bmMenuY + bmItemH + 8.f }, IM_COL32(120, 100, 80, 150), "K: Set Bookmark First");
				}

				dl->AddText(f, f->FontSize * 0.85f, { bmMenuX + 12.f, bmMenuY + bmItemH * 2 + 8.f }, IM_COL32(200, 180, 140, 220), "Enter: From Beginning");
				dl->AddText(f, f->FontSize * 0.85f, { bmMenuX + 12.f, bmMenuY + bmItemH * 3 + 8.f }, IM_COL32(200, 180, 140, 220), "R: Random Page");
			}

			const char* navHint = "<- -> : Browse   |   K: Bookmark   |   ESC : Close";
			ImVec2 ns = f->CalcTextSizeA(f->FontSize * 0.85f, FLT_MAX, 0.f, navHint);
			dl->AddText(f, f->FontSize * 0.85f, { ds.x * 0.5f - ns.x * 0.5f, ds.y - 40.f }, IM_COL32(160, 140, 100, 200), navHint);
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
		{
			CloseInventory();
		}

		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
		{
			if (s_selectedBookIdx > 0)
				s_selectedBookIdx--;
			else if (!s_availableBooks.empty())
				s_selectedBookIdx = (int)s_availableBooks.size() - 1;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false))
		{
			if (s_selectedBookIdx < (int)s_availableBooks.size() - 1)
				s_selectedBookIdx++;
			else if (!s_availableBooks.empty())
				s_selectedBookIdx = 0;
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
		{
			if (!s_availableBooks.empty() && s_selectedBookIdx >= 0 && s_selectedBookIdx < (int)s_availableBooks.size())
			{
				s_currentPage = 0; // Open from beginning
				OpenBook(s_availableBooks[s_selectedBookIdx]);
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_K, false))
		{
			if (!s_availableBooks.empty() && s_selectedBookIdx >= 0 && s_selectedBookIdx < (int)s_availableBooks.size())
			{
				const std::string& bookName = s_availableBooks[s_selectedBookIdx];
				auto bmIt = s_bookmarks.find(bookName);
				if (bmIt != s_bookmarks.end() && bmIt->second >= 0)
				{
					s_currentPage = bmIt->second; // Open at bookmark
					OpenBook(bookName);
				}
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_R, false))
		{
			if (!s_availableBooks.empty() && s_selectedBookIdx >= 0 && s_selectedBookIdx < (int)s_availableBooks.size())
			{
				const std::string& bookName = s_availableBooks[s_selectedBookIdx];
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

		std::string fullText;
		for (const auto& line : book.lines)
		{
			if (!fullText.empty()) fullText += "\n";
			fullText += line;
		}

		std::vector<std::string> wrappedLines = WrapText(fullText, pageW, f, fontSize);

		int leftPage = s_currentPage * 2;
		int rightPage = leftPage + 1;

		auto drawPage = [&](int pageIdx, float px, float py, float pw)
		{
			int startLine = pageIdx * linesPerPage;
			float ty = py + margin;
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
				dl->AddText(f, fontSize, { tx, ty }, inkCol, line.c_str(), nullptr, pw);
				ty += lineH;
			}
			char pnum[16];
			snprintf(pnum, sizeof(pnum), "- %d -", pageIdx + 1);
			ImVec2 pns = f->CalcTextSizeA(fontSize * 0.8f, FLT_MAX, 0.f, pnum);
			dl->AddText(f, fontSize * 0.8f, { px + pw * 0.5f - pns.x * 0.5f, py + bookH - margin - fontSize }, IM_COL32(150, 140, 130, 200), pnum);
		};

		float leftX = bx + margin;
		float rightX = bx + bookW * 0.5f + margin * 0.5f;
		drawPage(leftPage, leftX, by, pageW);
		if (rightPage < (int)wrappedLines.size() / linesPerPage + 1)
			drawPage(rightPage, rightX, by, pageW);

		const char* btitle = book.config.displayTitle.c_str();
		ImVec2 bts = f->CalcTextSizeA(f->FontSize * 1.2f, FLT_MAX, 0.f, btitle);
		dl->AddText(f, f->FontSize * 1.2f, { ds.x * 0.5f - bts.x * 0.5f, by - f->FontSize * 2.f }, IM_COL32(234, 223, 197, 255), btitle);

		// Bookmark menu overlay
		{
			const float menuY = ds.y * 0.12f;
			const float itemH = f->FontSize * 1.8f;
			const float menuW = 320.f;
			const float menuX = ds.x * 0.5f - menuW * 0.5f;

			dl->AddRectFilled({ menuX, menuY }, { menuX + menuW, menuY + itemH * 3.5f }, IM_COL32(20, 15, 10, 220), 6.f);
			dl->AddRect({ menuX, menuY }, { menuX + menuW, menuY + itemH * 3.5f }, IM_COL32(180, 140, 90, 200), 6.f, 0, 2.f);

			dl->AddText(f, f->FontSize * 1.2f, { menuX + 15.f, menuY + 10.f }, IM_COL32(234, 223, 197, 255), "Open Book:");

			auto bmIt = s_bookmarks.find(s_currentBook);
			bool hasBookmark = (bmIt != s_bookmarks.end() && bmIt->second > 0);
			ImU32 kCol = hasBookmark ? IM_COL32(255, 215, 0, 255) : IM_COL32(120, 100, 80, 150);
			char kText[128];
			if (hasBookmark)
				snprintf(kText, sizeof(kText), "K: Open at Bookmark (Page %d)", bmIt->second + 1);
			else
				snprintf(kText, sizeof(kText), "K: Set Bookmark First");
			dl->AddText(f, f->FontSize, { menuX + 15.f, menuY + itemH + 15.f }, kCol, kText);

			dl->AddText(f, f->FontSize, { menuX + 15.f, menuY + itemH * 2 + 15.f }, IM_COL32(200, 180, 140, 220), "Enter: Open from Beginning");
			dl->AddText(f, f->FontSize, { menuX + 15.f, menuY + itemH * 3 + 15.f }, IM_COL32(200, 180, 140, 220), "R: Random Page");
			dl->AddText(f, f->FontSize * 0.8f, { menuX + 15.f, menuY + itemH * 3.5f - 15.f }, IM_COL32(150, 130, 100, 180), "ESC: Close");
		}

		const char* hint = "Arrows: Turn page   |   K: Bookmark   |   ESC: Close book";
		ImVec2 hs = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, hint);
		dl->AddText(f, f->FontSize, { ds.x * 0.5f - hs.x * 0.5f, ds.y * 0.93f }, IM_COL32(200, 200, 200, 200), hint);

		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false))
		{
			if ((s_currentPage + 1) * 2 < (int)wrappedLines.size() / linesPerPage + 1)
				s_currentPage++;
		}
		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
		{
			if (s_currentPage > 0) s_currentPage--;
		}
		if (ImGui::IsKeyPressed(ImGuiKey_K, false))
		{
			// K: Set bookmark on current page
			s_bookmarks[s_currentBook] = s_currentPage;
		}
		if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
		{
			CloseBook();
			OpenInventory();
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
}
