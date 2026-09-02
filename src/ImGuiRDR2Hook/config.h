#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>

namespace WJConfig
{
	inline char OpenJournalKey = 'J';
	inline int  InstantOpen = 0;
	inline bool BlessingValid = false;
	inline bool EnableDeveloperLog = false;
	inline std::string MyName;
	inline int  ReloadButton = VK_F5;
	inline bool CustomBooksEnabled = true;
	inline char CustomBooksKey = 'B';

	inline constexpr const char* FB_Help_Cover    = "ENTER: Open journal   |   Hold ESC 5s: Save and close";
	inline constexpr const char* FB_Help_Overview = "Arrows: Focus page (2nd time: turn page)   |   ENTER: Select   |   Hold ESC 5s: Close";
	inline constexpr const char* FB_Help_Zoom     = "R: Exit zoom   |   F: Change font   |   ESC: Deselect";
	inline constexpr const char* FB_Help_Draw     = "D: Exit drawing   |   E: Eraser   |   Z/X: Eraser size   |   V: Appreciate view   |   SHIFT: Tools   |   ESC: Deselect";
	inline constexpr const char* FB_Help_Write    = "W: Write   |   D: Draw   |   R: Zoom   |   ESC: Deselect   |   Hold ESC 5s: Close";
	inline constexpr const char* FB_Empty_Page    = "The pages are blank... (press W to write)";
	inline constexpr const char* FB_Format_Title  = "Text Format";
	inline constexpr const char* FB_Draw_Tools    = "Drawing Tools";
	inline constexpr const char* FB_CB_NavHint         = "<- -> : Browse   |   K: Bookmark   |   ESC : Close";
	inline constexpr const char* FB_CB_NavHintRandom   = "<- -> : Browse   |   K: Bookmark   |   R: Random Page   |   ESC : Close";
	inline constexpr const char* FB_CB_SearchHint      = "Search: title, author, category...";
	inline constexpr const char* FB_CB_NoBooks         = "No books found in MyJourney/Books/";
	inline constexpr const char* FB_CB_NoMatch         = "No books match your search";
	inline constexpr const char* FB_CB_OpenLabel       = "Open:";
	inline constexpr const char* FB_CB_BookmarkPage    = "K: Bookmark (Page %d)";
	inline constexpr const char* FB_CB_SetBookmarkFirst= "K: Set Bookmark First";
	inline constexpr const char* FB_CB_OpenBeginning   = "Enter: From Beginning";
	inline constexpr const char* FB_CB_OpenRandom      = "R: Random Page";
	inline constexpr const char* FB_CB_BookNavHint     = "Arrows: Turn page   |   K: Bookmark   |   ESC: Close book";
	inline constexpr const char* FB_CB_OpenBookLabel   = "Open Book:";
	inline constexpr const char* FB_CB_OpenAtBookmark  = "K: Open at Bookmark (Page %d)";
	inline constexpr const char* FB_CB_OpenFromStart   = "Enter: Open from Beginning";
	inline constexpr const char* FB_CB_SatchelOpening  = "Opening Satchel...";
	inline constexpr const char* FB_BookmarkSaved      = "Bookmark Saved";
	inline constexpr const char* FB_BookmarkRemoved    = "Bookmark Removed";
	inline constexpr const char* FB_Sheet_RipHint      = "P: Rip Page";
	inline constexpr const char* FB_Sheet_RippingProgress = "Ripping page...";
	inline constexpr const char* FB_Sheet_LeaveHint    = "L: Leave Page here";
	inline constexpr const char* FB_Sheet_ReadHint     = "R: Read";
	inline constexpr const char* FB_Sheet_RestoreHint  = "Add page back";
	inline constexpr const char* FB_Sheet_CloseHint    = "Close";
	inline constexpr const char* FB_Sheet_Nearby       = "There is a ripped page nearby";
	inline constexpr const char* FB_Sheet_PressE       = "Press E to pick it up";

	inline std::string Help_Cover;
	inline std::string Help_Overview;
	inline std::string Help_Zoom;
	inline std::string Help_Draw;
	inline std::string Help_Write;
	inline std::string Empty_Page;
	inline std::string Format_Title;
	inline std::string Draw_Tools;
	inline std::string CB_NavHint;
	inline std::string CB_NavHintRandom;
	inline std::string CB_SearchHint;
	inline std::string CB_NoBooks;
	inline std::string CB_NoMatch;
	inline std::string CB_OpenLabel;
	inline std::string CB_BookmarkPage;
	inline std::string CB_SetBookmarkFirst;
	inline std::string CB_OpenBeginning;
	inline std::string CB_OpenRandom;
	inline std::string CB_BookNavHint;
	inline std::string CB_OpenBookLabel;
	inline std::string CB_OpenAtBookmark;
	inline std::string CB_OpenFromStart;
	inline std::string CB_SatchelOpening;
	inline std::string BookmarkSaved;
	inline std::string BookmarkRemoved;
	inline std::string Sheet_RipHint;
	inline std::string RippingProgress;
	inline std::string Sheet_LeaveHint;
	inline std::string Sheet_ReadHint;
	inline std::string Sheet_RestoreHint;
	inline std::string Sheet_CloseHint;
	inline std::string Sheet_Nearby;
	inline std::string Sheet_PressE;

	inline std::string GetModuleDir()
	{
		static std::string s_dir;
		if (s_dir.empty())
		{
			HMODULE hModule = GetModuleHandleA("WriteYourJourney.asi");
			if (hModule)
			{
				char path[MAX_PATH];
				GetModuleFileNameA(hModule, path, MAX_PATH);
				s_dir = std::filesystem::path(path).parent_path().string();
			}
			else
			{
				s_dir = ".";
			}
		}
		return s_dir;
	}

	inline std::string GetIniPath()
	{
		return GetModuleDir() + "\\WriteYourJourney.ini";
	}

	inline bool ValidateBlessing()
	{
		std::ifstream file(GetIniPath(), std::ios::in);
		if (!file) return false;

		std::ostringstream ss;
		ss << file.rdbuf();
		const std::string content = ss.str();
		return content.find("- Made with love By Leuan... May god bless you all") != std::string::npos;
	}

	inline void Load()
	{
		const std::string iniPath = GetIniPath();
		char buf[512];

		GetPrivateProfileStringA("Settings", "OpenJournalKey", "J", buf, sizeof(buf), iniPath.c_str());
		if (buf[0] >= 'a' && buf[0] <= 'z')
			OpenJournalKey = (char)(buf[0] - 32);
		else if (buf[0] >= 'A' && buf[0] <= 'Z')
			OpenJournalKey = buf[0];

		GetPrivateProfileStringA("Settings", "InstantOpen", "0", buf, sizeof(buf), iniPath.c_str());
		InstantOpen = std::atoi(buf);

		GetPrivateProfileStringA("Settings", "EnableDeveloperLog", "0", buf, sizeof(buf), iniPath.c_str());
		EnableDeveloperLog = (std::atoi(buf) != 0);

		GetPrivateProfileStringA("Settings", "MyName", "", buf, sizeof(buf), iniPath.c_str());
		MyName = buf;

		GetPrivateProfileStringA("Settings", "ReloadButton", "VK_F5", buf, sizeof(buf), iniPath.c_str());
		ReloadButton = std::atoi(buf);
		if (ReloadButton == 0)
		{
			// Parsear teclas F1-F12
			if ((buf[0] == 'F' || buf[0] == 'f') && buf[1] >= '0' && buf[1] <= '9')
			{
				int fKey = std::atoi(buf + 1);
				if (fKey >= 1 && fKey <= 12)
					ReloadButton = VK_F1 + (fKey - 1);
			}
			else if (buf[0] >= 'a' && buf[0] <= 'z')
				ReloadButton = (int)(unsigned char)(buf[0] - 32);
			else if (buf[0] >= 'A' && buf[0] <= 'Z')
				ReloadButton = (int)(unsigned char)buf[0];
			else
				ReloadButton = VK_F5; // Default fallback
		}

		GetPrivateProfileStringA("CustomBooks", "Enabled", "1", buf, sizeof(buf), iniPath.c_str());
		CustomBooksEnabled = (std::atoi(buf) != 0);

		GetPrivateProfileStringA("CustomBooks", "Key", "B", buf, sizeof(buf), iniPath.c_str());
		if (buf[0] >= 'a' && buf[0] <= 'z')
			CustomBooksKey = (char)(buf[0] - 32);
		else if (buf[0] >= 'A' && buf[0] <= 'Z')
			CustomBooksKey = buf[0];

		auto loadStr = [&](const char* key, const char* fallback, std::string& out) {
			GetPrivateProfileStringA("Localization", key, fallback, buf, sizeof(buf), iniPath.c_str());
			out = buf;
		};

		loadStr("Help_Cover",    FB_Help_Cover,    Help_Cover);
		loadStr("Help_Overview", FB_Help_Overview, Help_Overview);
		loadStr("Help_Zoom",     FB_Help_Zoom,     Help_Zoom);
		loadStr("Help_Draw",     FB_Help_Draw,     Help_Draw);
		loadStr("Help_Write",    FB_Help_Write,    Help_Write);
		loadStr("Empty_Page",    FB_Empty_Page,    Empty_Page);
		loadStr("Format_Title",  FB_Format_Title,  Format_Title);
		loadStr("Draw_Tools",    FB_Draw_Tools,    Draw_Tools);

		loadStr("CB_NavHint",          FB_CB_NavHint,          CB_NavHint);
		loadStr("CB_NavHintRandom",    FB_CB_NavHintRandom,    CB_NavHintRandom);
		loadStr("CB_SearchHint",       FB_CB_SearchHint,       CB_SearchHint);
		loadStr("CB_NoBooks",          FB_CB_NoBooks,          CB_NoBooks);
		loadStr("CB_NoMatch",          FB_CB_NoMatch,          CB_NoMatch);
		loadStr("CB_OpenLabel",        FB_CB_OpenLabel,        CB_OpenLabel);
		loadStr("CB_BookmarkPage",     FB_CB_BookmarkPage,     CB_BookmarkPage);
		loadStr("CB_SetBookmarkFirst", FB_CB_SetBookmarkFirst, CB_SetBookmarkFirst);
		loadStr("CB_OpenBeginning",    FB_CB_OpenBeginning,    CB_OpenBeginning);
		loadStr("CB_OpenRandom",       FB_CB_OpenRandom,       CB_OpenRandom);
		loadStr("CB_BookNavHint",      FB_CB_BookNavHint,      CB_BookNavHint);
		loadStr("CB_OpenBookLabel",    FB_CB_OpenBookLabel,    CB_OpenBookLabel);
		loadStr("CB_OpenAtBookmark",   FB_CB_OpenAtBookmark,   CB_OpenAtBookmark);
		loadStr("CB_OpenFromStart",    FB_CB_OpenFromStart,    CB_OpenFromStart);
		loadStr("CB_SatchelOpening",   FB_CB_SatchelOpening,   CB_SatchelOpening);
		loadStr("BookmarkSaved",       FB_BookmarkSaved,       BookmarkSaved);
		loadStr("BookmarkRemoved",     FB_BookmarkRemoved,     BookmarkRemoved);
		loadStr("Sheet_RipHint",       FB_Sheet_RipHint,       Sheet_RipHint);
		loadStr("RippingProgress",     FB_Sheet_RippingProgress, RippingProgress);
		loadStr("Sheet_LeaveHint",     FB_Sheet_LeaveHint,     Sheet_LeaveHint);
		loadStr("Sheet_ReadHint",      FB_Sheet_ReadHint,      Sheet_ReadHint);
		loadStr("Sheet_RestoreHint",   FB_Sheet_RestoreHint,   Sheet_RestoreHint);
		loadStr("Sheet_CloseHint",     FB_Sheet_CloseHint,     Sheet_CloseHint);
		loadStr("Sheet_Nearby",        FB_Sheet_Nearby,        Sheet_Nearby);
		loadStr("Sheet_PressE",        FB_Sheet_PressE,        Sheet_PressE);

		BlessingValid = ValidateBlessing();
	}

	inline int GetVKKey() { return (int)(unsigned char)OpenJournalKey; }
}
