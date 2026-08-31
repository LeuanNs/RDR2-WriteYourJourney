#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct BookConfig
{
	std::string displayTitle;
	std::string author;
	std::string year;
	std::string category;
	int coverColorRGB[3] = { 139, 69, 19 };
	int fontType = 1;
	int fontSizeOverride = 0;
	std::string inkColor = "Black";
	int textAlignment = 0;
	bool preserveLineBreaks = true;
	bool allowsOpenRandomPage = true;
	bool hasIndex = true;
	bool autoOrderPages = true;
	bool isOwned = false;
};

struct BookChapter
{
	std::string title;
	int lineIndex = 0;
};

struct CustomBook
{
	std::string internalName;
	BookConfig config;
	std::vector<std::string> lines;
	std::vector<BookChapter> chapters;
	bool loaded = false;
};

namespace CustomBooks
{
	void Init();
	void ScanBooks();
	void LoadBook(const std::string& internalName);
	void UnloadBook(const std::string& internalName);

	void OpenInventory();
	void CloseInventory();
	bool IsInventoryOpen();
	void RenderInventory();

	void OpenBook(const std::string& internalName);
	void CloseBook();
	bool IsBookOpen();
	void RenderBook();

	void HandleInput();

	const std::vector<std::string>& GetAvailableBooks();

	void SetBookOwned(const std::string& internalName, bool owned);
	bool IsBookOwned(const std::string& internalName);
}
