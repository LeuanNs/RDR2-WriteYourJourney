#pragma once
#include <string>
#include <vector>
#include "sheets.h"

struct Letter
{
	int id = 0;
	std::string from;
	std::string to;
	std::string text;
	SheetDrawing drawing;
	SheetDrawing envelopeDrawing;
	std::string date;
	int originalPage = 0;
	std::string bookName;
	bool sent = false;
	bool received = false;
};

namespace Letters
{
	void Init();
	void ScanLetters();

	void SetPlayerCoords(float x, float y, float z);

	bool IsNearPostbox();
	void UpdatePostboxPrompt(float px, float py, float pz);
	void RenderPostboxPrompt();

	bool IsInboxOpen();
	void OpenInbox();
	void CloseInbox();
	void HandleInput();
	void RenderInbox();

	bool IsReadingLetter();
	void RenderLetterRead();

	bool TrySaveLetterFromOverlay(const std::string& from, const std::string& to, const SheetDrawing& envelopeDrawing);

	int GetNextLetterId();

	const std::vector<Letter>& GetSentLetters();
	const std::vector<Letter>& GetReceivedLetters();
}
