#pragma once
#include <string>
#include <vector>
#include <atomic>
#include "imgui/imgui.h"

struct SheetDrawingLine
{
	std::vector<ImVec2> points;
	ImU32 color = IM_COL32(48, 38, 30, 255);
	float thickness = 2.0f;
	int brush = 0;
};

struct SheetDrawing
{
	std::vector<SheetDrawingLine> lines;
};

struct RippedSheetCache
{
	std::string text;
	SheetDrawing drawing;
	int sourcePage = 0;
	bool fromJournal = true;
	std::string bookName;
	int chapter = 1;
};

struct DiscoverableSheet
{
	int id = 0;
	float x = 0.f, y = 0.f, z = 0.f;
	float radius = 10.f;
	std::string pickupMessage;
	std::string text;
	SheetDrawing drawing;
	std::string author;
	std::string source;
	bool collected = false;
};

namespace Sheets
{
	void Init();
	void ScanSheets();

	void SetPlayerCoords(float x, float y, float z);

	void HandleInput();
	void Render();
	void RenderPickupPrompt();

	void UpdatePickupPrompt(float px, float py, float pz);
	bool TryPickupSheet();
	bool IsNearPickup();

	bool IsShowingOverlay();
	bool IsRipping();
	bool IsAnimating();

	bool IsPageRipped(int page, bool isJournal, const std::string& bookName = "");

	void StartRipPage(const std::string& text, const SheetDrawing& drawing, int page, bool fromJournal, const std::string& bookName = "", int chapter = 1);
	void ConfirmRip();
	void CancelRip();

	void LeaveSheetAtPlayer();
	void RestorePage();

	float GetRipProgress();
	void SetRipProgress(float p);

	int GetRipSourcePage();
	bool GetRipFromJournal();
	const std::string& GetRipBookName();
}
