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
	bool findable = false;
	float locationX = 0.f;
	float locationY = 0.f;
	float locationZ = 0.f;
	float pickupRadius = 10.f;
	std::string pickupMessage = "Press E to pick up";
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
		int lazyStartLine = 0;
		int lazyLoadedCount = 0;
		int lazyTotalLines = 0;
		int lazyTotalChars = 0;
	};

struct RibbonAnim
{
	float progress = 0.f;
	bool active = false;
	bool placing = true;
	float textFade = 0.f;
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

	void UpdatePickupPrompt(float playerX, float playerY, float playerZ);
	bool TryPickupBook(float playerX, float playerY, float playerZ);
	bool IsNearPickup();
	const std::string& GetPickupMessage();
	void RenderPickupPrompt();

	void OpenIndex();
	void CloseIndex();
	bool IsIndexOpen();
	void RenderIndex();

	void MarkChapterAsRipped(const std::string& bookName, int lineIndex);
	int GetNextValidLineIndex(const std::string& bookName, int lineIndex);
}
