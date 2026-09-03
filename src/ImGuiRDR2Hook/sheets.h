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
	std::string backText;
	SheetDrawing backDrawing;
	int backPage = 0;
};

struct DiscoverableSheet
{
	int id = 0;
	float x = 0.f, y = 0.f, z = 0.f;
	float radius = 5.f;
	std::string pickupMessage;
	std::string text;
	SheetDrawing drawing;
	std::string author;
	std::string source;
	bool collected = false;
	int originalPage = 0;
	bool fromJournal = true;
	std::string bookName;
	int chapter = 1;
	std::string backText;
	SheetDrawing backDrawing;
};

namespace Sheets
{
	void Init();
	void ScanSheets();

	void SetPlayerCoords(float x, float y, float z);

	void HandleInput();
	void Render();
	void RenderPickupPrompt();
	void RenderEHoldPrompt();

	void UpdatePickupPrompt(float px, float py, float pz);
	bool TryPickupSheet();
	bool IsNearPickup();

	bool IsShowingOverlay();
	bool IsRipping();
	bool IsAnimating();

	bool IsPageRipped(int page, bool isJournal, const std::string& bookName = "");
	int GetNextVisiblePage(int startPage);
	int GetPartnerPage(int page);

	void StartRipPage(const std::string& text, const SheetDrawing& drawing, int page, bool fromJournal, const std::string& bookName = "", int chapter = 1, const std::string& backText = "", const SheetDrawing& backDrawing = SheetDrawing());
	void ConfirmRip();
	void CancelRip();

	void LeaveSheetAtPlayer();
	void RestorePage();

	bool IsWalkingToSheet();
	void StartWalkToSheet();
	void CancelWalk();
	bool UpdateWalk(float px, float py, float pz);
	void GetNearbySheetCoords(float& x, float& y, float& z);

	float GetRipProgress();
	void SetRipProgress(float p);
	void DismissOverlay();

	int GetRipSourcePage();
	bool GetRipFromJournal();
	bool IsViewingDiscoverable();
	const std::string& GetRipBookName();

	float GetEHoldProgress();
	bool IsEHoldActive();
	void StartEHold();
	void CancelEHold();
	bool IsEHoldComplete();

	bool IsCrouching();
	void StartCrouch();
	bool UpdateCrouch();

	bool IsFlipAnimating();
	float GetFlipAnimT();
	void StartFlipAnim();
	bool IsShowingBack();
	void ToggleBackSide();

	void KeepSheet();
	bool IsSheetKept();

	void MarkSheetCollected(int sheetId);
	bool IsSheetCollected(int sheetId);

	bool WasSheetViewed();
	void SetSheetViewed(bool v);
}
