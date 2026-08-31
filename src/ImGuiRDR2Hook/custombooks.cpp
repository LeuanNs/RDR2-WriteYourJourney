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
	static std::vector<std::string> s_availableBooks;
	static std::unordered_map<std::string, CustomBook> s_loadedBooks;
	static bool s_inventoryOpen = false;
	static bool s_bookOpen = false;
	static std::string s_currentBook;
	static int s_currentPage = 0;
	static int s_totalPages = 0;
	static float s_bHoldTime = 0.f;
	static bool s_bHeld = false;

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
				s_availableBooks.push_back(entry.path().filename().string());
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
		bool bDown = (GetAsyncKeyState('B') & 0x8000) != 0;
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

	void RenderInventory()
	{
		if (!s_inventoryOpen) return;
		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		ImVec2 ds = io.DisplaySize;
		ImFont* f = io.Fonts->Fonts[1] ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];

		dl->AddRectFilled({ 0, 0 }, ds, IM_COL32(15, 12, 8, 220));

		float panelW = ds.x * 0.7f;
		float panelH = ds.y * 0.8f;
		float px = ds.x * 0.5f - panelW * 0.5f;
		float py = ds.y * 0.5f - panelH * 0.5f;

		dl->AddRectFilled({ px, py }, { px + panelW, py + panelH }, IM_COL32(35, 28, 20, 240), 8.f);
		dl->AddRect({ px, py }, { px + panelW, py + panelH }, IM_COL32(120, 90, 50, 200), 8.f, 0, 2.5f);

		float innerPad = 15.f;
		dl->AddRectFilled({ px + innerPad, py + innerPad }, { px + panelW - innerPad, py + panelH - innerPad }, IM_COL32(25, 20, 15, 200), 4.f);

		const char* title = "Satchel";
		ImVec2 ts = f->CalcTextSizeA(f->FontSize * 1.6f, FLT_MAX, 0.f, title);
		dl->AddText(f, f->FontSize * 1.6f, { ds.x * 0.5f - ts.x * 0.5f, py + 25.f }, IM_COL32(210, 185, 140, 255), title);

		float lineY = py + 25.f + f->FontSize * 2.2f;
		dl->AddLine({ px + 40.f, lineY }, { px + panelW - 40.f, lineY }, IM_COL32(100, 80, 50, 150), 1.f);

		float gridTop = lineY + 20.f;
		float gridBottom = py + panelH - 130.f;
		float gridH = gridBottom - gridTop;

		int cols = 4;
		int rows = 4;
		float cellW = 110.f;
		float cellH = 150.f;
		float gapX = (panelW - 80.f - cols * cellW) / (cols - 1);
		float gapY = (gridH - rows * cellH) / (rows > 1 ? rows - 1 : 1);
		if (gapX < 10.f) gapX = 10.f;
		if (gapY < 10.f) gapY = 10.f;

		int hoveredIdx = -1;

		for (size_t i = 0; i < s_availableBooks.size() && i < (size_t)(cols * rows); ++i)
		{
			int col = i % cols;
			int row = i / cols;
			float cx = px + 40.f + col * (cellW + gapX);
			float cy = gridTop + row * (cellH + gapY);

			LoadBook(s_availableBooks[i]);
			auto& book = s_loadedBooks[s_availableBooks[i]];

			bool hovered = io.MousePos.x >= cx && io.MousePos.x <= cx + cellW &&
			               io.MousePos.y >= cy && io.MousePos.y <= cy + cellH;

			ImU32 slotBg = hovered ? IM_COL32(55, 45, 30, 220) : IM_COL32(40, 32, 22, 200);
			dl->AddRectFilled({ cx, cy }, { cx + cellW, cy + cellH }, slotBg, 4.f);
			dl->AddRect({ cx, cy }, { cx + cellW, cy + cellH }, hovered ? IM_COL32(200, 170, 100, 220) : IM_COL32(80, 65, 40, 150), 4.f, 0, 1.5f);

			float bookPadX = 12.f, bookPadY = 10.f;
			float bkX = cx + bookPadX, bkY = cy + bookPadY;
			float bkW = cellW - bookPadX * 2.f, bkH = cellH - bookPadY * 2.f - f->FontSize * 1.5f;

			ImU32 coverCol = IM_COL32(book.config.coverColorRGB[0], book.config.coverColorRGB[1], book.config.coverColorRGB[2], 255);
			dl->AddRectFilled({ bkX, bkY }, { bkX + bkW, bkY + bkH }, coverCol, 3.f);
			dl->AddRect({ bkX, bkY }, { bkX + bkW, bkY + bkH }, IM_COL32(50, 40, 30, 200), 3.f, 0, 1.f);

			float spineW = 6.f;
			dl->AddRectFilled({ bkX, bkY }, { bkX + spineW, bkY + bkH }, IM_COL32(
				(int)(book.config.coverColorRGB[0] * 0.6f),
				(int)(book.config.coverColorRGB[1] * 0.6f),
				(int)(book.config.coverColorRGB[2] * 0.6f), 255));

			const char* btitle = book.config.displayTitle.c_str();
			ImVec2 bts = f->CalcTextSizeA(f->FontSize * 0.9f, bkW - 10.f, 0.f, btitle);
			float textX = bkX + bkW * 0.5f - bts.x * 0.5f;
			float textY = bkY + bkH * 0.35f;
			dl->AddText(f, f->FontSize * 0.9f, { textX, textY }, IM_COL32(234, 223, 197, 240), btitle, nullptr, bkW - 10.f);

			if (hovered)
			{
				hoveredIdx = (int)i;
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					OpenBook(s_availableBooks[i]);
				}
			}
		}

		float descY = gridBottom + 15.f;
		dl->AddLine({ px + 40.f, descY }, { px + panelW - 40.f, descY }, IM_COL32(100, 80, 50, 150), 1.f);

		float descTextY = descY + 15.f;
		if (hoveredIdx >= 0 && hoveredIdx < (int)s_availableBooks.size())
		{
			auto& book = s_loadedBooks[s_availableBooks[hoveredIdx]];
			const char* dtitle = book.config.displayTitle.c_str();
			dl->AddText(f, f->FontSize * 1.3f, { px + 50.f, descTextY }, IM_COL32(220, 195, 150, 255), dtitle);

			char meta[256];
			snprintf(meta, sizeof(meta), "by %s  |  %s  |  %s",
				book.config.author.c_str(),
				book.config.category.c_str(),
				book.config.year.c_str());
			dl->AddText(f, f->FontSize, { px + 50.f, descTextY + f->FontSize * 1.8f }, IM_COL32(180, 160, 120, 200), meta);
		}
		else
		{
			const char* hint = "Hover over a book to see details";
			dl->AddText(f, f->FontSize, { px + 50.f, descTextY }, IM_COL32(140, 120, 90, 180), hint);
		}

		const char* escHint = "ESC: Close";
		ImVec2 hs = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, escHint);
		dl->AddText(f, f->FontSize, { px + panelW - 50.f - hs.x, descTextY + f->FontSize * 1.8f }, IM_COL32(160, 140, 100, 180), escHint);

		if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			CloseInventory();
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
		int linesPerPage = (int)(pageTextH / (fontSize * 1.6f));
		if (linesPerPage < 1) linesPerPage = 1;

		int leftPage = s_currentPage * 2;
		int rightPage = leftPage + 1;

		auto drawPage = [&](int pageIdx, float px, float py, float pw)
		{
			int startLine = pageIdx * linesPerPage;
			float ty = py + margin;
			for (int l = 0; l < linesPerPage && (startLine + l) < (int)book.lines.size(); ++l)
			{
				const std::string& line = book.lines[startLine + l];
				if (line.empty()) { ty += fontSize * 1.6f; continue; }
				float tx = px;
				if (book.config.textAlignment == 1)
				{
					ImVec2 ls = f->CalcTextSizeA(fontSize, pw, 0.f, line.c_str());
					tx = px + pw * 0.5f - ls.x * 0.5f;
				}
				dl->AddText(f, fontSize, { tx, ty }, inkCol, line.c_str(), nullptr, pw);
				ty += fontSize * 1.6f;
			}
			char pnum[16];
			snprintf(pnum, sizeof(pnum), "- %d -", pageIdx + 1);
			ImVec2 pns = f->CalcTextSizeA(fontSize * 0.8f, FLT_MAX, 0.f, pnum);
			dl->AddText(f, fontSize * 0.8f, { px + pw * 0.5f - pns.x * 0.5f, py + bookH - margin - fontSize }, IM_COL32(150, 140, 130, 200), pnum);
		};

		float leftX = bx + margin;
		float rightX = bx + bookW * 0.5f + margin * 0.5f;
		drawPage(leftPage, leftX, by, pageW);
		if (rightPage < (int)book.lines.size() / linesPerPage + 1)
			drawPage(rightPage, rightX, by, pageW);

		const char* btitle = book.config.displayTitle.c_str();
		ImVec2 bts = f->CalcTextSizeA(f->FontSize * 1.2f, FLT_MAX, 0.f, btitle);
		dl->AddText(f, f->FontSize * 1.2f, { ds.x * 0.5f - bts.x * 0.5f, by - f->FontSize * 2.f }, IM_COL32(234, 223, 197, 255), btitle);

		const char* hint = "Arrows: Turn page   |   ESC: Close book";
		ImVec2 hs = f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, hint);
		dl->AddText(f, f->FontSize, { ds.x * 0.5f - hs.x * 0.5f, ds.y * 0.93f }, IM_COL32(200, 200, 200, 200), hint);

		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false))
		{
			if ((s_currentPage + 1) * 2 < (int)book.lines.size() / linesPerPage + 1)
				s_currentPage++;
		}
		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
		{
			if (s_currentPage > 0) s_currentPage--;
		}
		if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
		{
			CloseBook();
			OpenInventory();
		}
	}
}
