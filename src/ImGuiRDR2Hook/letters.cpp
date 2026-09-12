#define NOMINMAX
#include "letters.h"
#include "config.h"
#include "imgui/imgui.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <cstring>
#include <ctime>

namespace fs = std::filesystem;

namespace Letters
{
	static constexpr uint32_t LETTER_DRAWING_MAGIC = 0x574A4C01;
	static constexpr float POSTBOX_RADIUS = 3.0f;

	struct PostboxLocation
	{
		float x, y, z;
	};

	static PostboxLocation s_postboxLocations[] = {
		{ -5227.425293f, -3470.578369f, -20.569660f },
		{ -874.949463f, -1328.759766f, 43.958019f },
		{ -875.149841f, -1325.163330f, 43.978577f },
		{ 1231.434082f, -1299.668701f, 76.903465f },
		{ 2747.336914f, -1394.895264f, 46.183086f },
		{ 2749.542480f, -1399.684082f, 46.192284f },
		{ 2939.454346f, 1288.619019f, 44.652821f },
		{ 2931.539551f, 1282.908081f, 44.652855f },
		{ 2985.994873f, 568.672974f, 44.592808f },
		{ 2987.332275f, 576.226929f, 44.584068f },
		{ 1521.989014f, 439.494812f, 90.680717f },
		{ 1525.194702f, 442.655975f, 90.680717f },
		{ -174.514267f, 633.262634f, 114.089630f },
		{ -178.988113f, 626.750549f, 114.089600f },
		{ -1299.311401f, 401.964996f, 95.389923f },
		{ -1767.399658f, -381.454102f, 157.731873f },
		{ -1765.095825f, -384.142242f, 157.741516f },
		{ -1095.652222f, -576.684326f, 82.407570f },
		{ -1094.327393f, -574.917847f, 82.413368f }
	};
	static constexpr int NUM_POSTBOXES = 19;

	static std::atomic<float> s_playerX{ 0.f };
	static std::atomic<float> s_playerY{ 0.f };
	static std::atomic<float> s_playerZ{ 0.f };

	static std::vector<Letter> s_sentLetters;
	static std::vector<Letter> s_receivedLetters;
	static int s_nextLetterId = 1;
	static bool s_scanned = false;

	static bool s_inboxOpen = false;
	static int s_inboxIndex = 0;
	static bool s_showingSent = true;

	static bool s_readingLetter = false;
	static int s_readingLetterId = -1;
	static float s_readOpenAnimT = 0.f;
	static constexpr float READ_OPEN_DURATION = 0.6f;

	static bool s_nearPostbox = false;
	static int s_nearPostboxIndex = -1;

	static std::mutex s_lettersMutex;

	static fs::path GetLettersDir()
	{
		return fs::path(WJConfig::GetModuleDir()) / "myjourney" / "Letters";
	}

	static fs::path GetSentDir()
	{
		return GetLettersDir() / "Sent";
	}

	static fs::path GetReceivedDir()
	{
		return GetLettersDir() / "Received";
	}

	static std::string Trim(const std::string& s)
	{
		size_t a = s.find_first_not_of(" \t\r\n");
		if (a == std::string::npos) return "";
		size_t b = s.find_last_not_of(" \t\r\n");
		return s.substr(a, b - a + 1);
	}

	static void SaveDrawingToFile(const fs::path& path, const SheetDrawing& sd)
	{
		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		if (!out) return;
		out.write((const char*)&LETTER_DRAWING_MAGIC, sizeof(LETTER_DRAWING_MAGIC));
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
		if (magic != LETTER_DRAWING_MAGIC) return sd;
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

	static void ParseEnvelopeIni(const fs::path& path, Letter& letter)
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
			if (key == "from") letter.from = val;
			else if (key == "to") letter.to = val;
			else if (key == "date") letter.date = val;
			else if (key == "originalPage") letter.originalPage = std::stoi(val);
			else if (key == "bookName") letter.bookName = val;
		}
	}

	void ScanLetters()
	{
		std::lock_guard<std::mutex> lock(s_lettersMutex);
		s_sentLetters.clear();
		s_receivedLetters.clear();
		s_nextLetterId = 1;

		auto scanDir = [&](const fs::path& dir, std::vector<Letter>& out, bool isSent) {
			if (!fs::exists(dir)) return;
			for (const auto& entry : fs::directory_iterator(dir))
			{
				if (!entry.is_directory()) continue;
				std::string name = entry.path().filename().string();
				if (name.substr(0, 6) != "LETTER") continue;

				int id = 0;
				try { id = std::stoi(name.substr(6)); }
				catch (...) { continue; }

				if (id >= s_nextLetterId) s_nextLetterId = id + 1;

				Letter letter;
				letter.id = id;
				letter.sent = isSent;
				letter.received = !isSent;

				fs::path envPath = entry.path() / "envelope.ini";
				if (fs::exists(envPath))
					ParseEnvelopeIni(envPath, letter);

				fs::path textPath = entry.path() / "letter.txt";
				if (fs::exists(textPath))
				{
					std::ifstream tf(textPath);
					if (tf)
					{
						std::ostringstream ss;
						ss << tf.rdbuf();
						letter.text = ss.str();
					}
				}

				fs::path drawPath = entry.path() / "letter_draw.dat";
				if (fs::exists(drawPath))
					letter.drawing = LoadDrawingFromFile(drawPath);

				fs::path envDrawPath = entry.path() / "envelope_draw.dat";
				if (fs::exists(envDrawPath))
					letter.envelopeDrawing = LoadDrawingFromFile(envDrawPath);

				out.push_back(std::move(letter));
			}
		};

		scanDir(GetSentDir(), s_sentLetters, true);
		scanDir(GetReceivedDir(), s_receivedLetters, false);

		s_scanned = true;
	}

	void Init()
	{
		if (!WJConfig::LettersEnabled) return;
		ScanLetters();
	}

	void SetPlayerCoords(float x, float y, float z)
	{
		s_playerX.store(x);
		s_playerY.store(y);
		s_playerZ.store(z);
	}

	void UpdatePostboxPrompt(float px, float py, float pz)
	{
		if (!WJConfig::LettersEnabled) return;

		s_nearPostbox = false;
		s_nearPostboxIndex = -1;

		for (int i = 0; i < NUM_POSTBOXES; ++i)
		{
			float dx = px - s_postboxLocations[i].x;
			float dy = py - s_postboxLocations[i].y;
			float dz = pz - s_postboxLocations[i].z;
			float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (dist <= POSTBOX_RADIUS)
			{
				s_nearPostbox = true;
				s_nearPostboxIndex = i;
				break;
			}
		}
	}

	bool IsNearPostbox() { return s_nearPostbox; }

	void RenderPostboxPrompt()
	{
		if (!WJConfig::LettersEnabled) return;
		if (!s_nearPostbox) return;

		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* dl = ImGui::GetForegroundDrawList();
		ImVec2 ds = io.DisplaySize;
		ImFont* f = io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];
		ImFont* df = ImGui::GetFont();

		const char* msg1 = WJConfig::Letters_NearPostbox.c_str();
		ImVec2 s1 = f->CalcTextSizeA(f->FontSize * 1.2f, FLT_MAX, 0.f, msg1);

		float y = ds.y * 0.75f;
		dl->AddText(f, f->FontSize * 1.2f, { ds.x * 0.5f - s1.x * 0.5f, y }, IM_COL32(234, 223, 197, 255), msg1);

		char keyStr[2] = { WJConfig::LettersInteractKey, '\0' };
		std::string msg2 = "Press ";
		msg2 += keyStr;
		msg2 += " to open inbox";
		ImVec2 s2 = df->CalcTextSizeA(df->FontSize, FLT_MAX, 0.f, msg2.c_str());
		dl->AddText(df, df->FontSize, { ds.x * 0.5f - s2.x * 0.5f, y + f->FontSize * 1.5f }, IM_COL32(220, 208, 180, 230), msg2.c_str());
	}

	bool IsInboxOpen() { return s_inboxOpen; }

	void OpenInbox()
	{
		s_inboxOpen = true;
		s_inboxIndex = 0;
		s_showingSent = true;
		ScanLetters();
	}

	void CloseInbox()
	{
		s_inboxOpen = false;
		s_readingLetter = false;
		s_readingLetterId = -1;
	}

	bool IsReadingLetter() { return s_readingLetter; }

	int GetNextLetterId()
	{
		return s_nextLetterId++;
	}

	const std::vector<Letter>& GetSentLetters() { return s_sentLetters; }
	const std::vector<Letter>& GetReceivedLetters() { return s_receivedLetters; }

	bool TrySaveLetterFromOverlay(const std::string& from, const std::string& to, const std::string& text, const SheetDrawing& drawing, const SheetDrawing& envelopeDrawing, int originalPage, bool fromJournal, const std::string& bookName)
	{
		fs::path dir = GetSentDir();
		fs::create_directories(dir);

		int newId = GetNextLetterId();
		std::string folderName = "LETTER" + std::to_string(newId);
		fs::path letterDir = dir / folderName;
		fs::create_directories(letterDir);

		{
			std::ofstream env(letterDir / "envelope.ini");
			if (!env) return false;
			env << "[Envelope]\n";
			env << "from=" << from << "\n";
			env << "to=" << to << "\n";

			time_t now = time(nullptr);
			struct tm lt;
			localtime_s(&lt, &now);
			char dateBuf[64];
			strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M", &lt);
			env << "date=" << dateBuf << "\n";
			env << "originalPage=" << originalPage << "\n";
			env << "bookName=" << bookName << "\n";
		}

		{
			std::ofstream txt(letterDir / "letter.txt");
			if (txt) txt << text;
		}

		if (!drawing.lines.empty())
		{
			SaveDrawingToFile(letterDir / "letter_draw.dat", drawing);
		}

		if (!envelopeDrawing.lines.empty())
		{
			SaveDrawingToFile(letterDir / "envelope_draw.dat", envelopeDrawing);
		}

		Letter letter;
		letter.id = newId;
		letter.from = from;
		letter.to = to;
		letter.text = text;
		letter.drawing = drawing;
		letter.envelopeDrawing = envelopeDrawing;
		letter.originalPage = originalPage;
		letter.bookName = bookName;
		letter.sent = true;

		{
			std::lock_guard<std::mutex> lock(s_lettersMutex);
			s_sentLetters.push_back(std::move(letter));
		}

		return true;
	}

	void HandleInput()
	{
		if (!WJConfig::LettersEnabled) return;

		if (s_inboxOpen)
		{
			if (s_readingLetter)
			{
				if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
				{
					s_readingLetter = false;
					s_readingLetterId = -1;
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
				{
					const auto& letters = s_showingSent ? s_sentLetters : s_receivedLetters;
					if (s_readingLetterId >= 0 && s_readingLetterId < (int)letters.size())
					{
						const Letter& letter = letters[s_readingLetterId];
						fs::path dir = (letter.sent ? GetSentDir() : GetReceivedDir()) / ("LETTER" + std::to_string(letter.id));
						if (fs::exists(dir) && fs::is_directory(dir))
						{
							fs::remove_all(dir);
						}
						ScanLetters();
					}
					s_readingLetter = false;
					s_readingLetterId = -1;
				}
			}
			else
			{
				if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
				{
					CloseInbox();
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_Tab, false))
				{
					s_showingSent = !s_showingSent;
					s_inboxIndex = 0;
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
				{
					const auto& letters = s_showingSent ? s_sentLetters : s_receivedLetters;
					if (!letters.empty())
					{
						s_inboxIndex--;
						if (s_inboxIndex < 0) s_inboxIndex = (int)letters.size() - 1;
					}
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false))
				{
					const auto& letters = s_showingSent ? s_sentLetters : s_receivedLetters;
					if (!letters.empty())
					{
						s_inboxIndex++;
						if (s_inboxIndex >= (int)letters.size()) s_inboxIndex = 0;
					}
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
				{
					const auto& letters = s_showingSent ? s_sentLetters : s_receivedLetters;
					if (!letters.empty() && s_inboxIndex >= 0 && s_inboxIndex < (int)letters.size())
					{
						s_readingLetter = true;
						s_readingLetterId = s_inboxIndex;
					}
				}
			}
		}
	}

	void RenderInbox()
	{
		if (!WJConfig::LettersEnabled) return;
		if (!s_inboxOpen || s_readingLetter) return;

		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		ImVec2 ds = io.DisplaySize;

		dl->AddRectFilled({ 0, 0 }, ds, IM_COL32(0, 0, 0, 180));

		ImFont* f = io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];
		ImFont* df = ImGui::GetFont();

		const char* title = WJConfig::Letters_InboxTitle.c_str();
		ImVec2 tsz = f->CalcTextSizeA(f->FontSize * 1.5f, FLT_MAX, 0.f, title);
		dl->AddText(f, f->FontSize * 1.5f, { ds.x * 0.5f - tsz.x * 0.5f, ds.y * 0.08f }, IM_COL32(234, 223, 197, 255), title);

		const auto& letters = s_showingSent ? s_sentLetters : s_receivedLetters;
		std::string tabLabel = s_showingSent ? "Sent" : "Received";
		tabLabel += " (" + std::to_string(letters.size()) + ")";
		ImVec2 tbsz = df->CalcTextSizeA(df->FontSize * 1.2f, FLT_MAX, 0.f, tabLabel.c_str());
		dl->AddText(df, df->FontSize * 1.2f, { ds.x * 0.5f - tbsz.x * 0.5f, ds.y * 0.15f }, IM_COL32(200, 180, 140, 230), tabLabel.c_str());

		if (letters.empty())
		{
			const char* emptyMsg = "No letters";
			ImVec2 esz = df->CalcTextSizeA(df->FontSize, FLT_MAX, 0.f, emptyMsg);
			dl->AddText(df, df->FontSize, { ds.x * 0.5f - esz.x * 0.5f, ds.y * 0.5f }, IM_COL32(180, 160, 130, 200), emptyMsg);
		}
		else
		{
			if (s_inboxIndex >= (int)letters.size()) s_inboxIndex = (int)letters.size() - 1;
			if (s_inboxIndex < 0) s_inboxIndex = 0;

			const Letter& letter = letters[s_inboxIndex];

			float envW = ds.x * 0.4f;
			float envH = ds.y * 0.25f;
			ImVec2 envMn{ ds.x * 0.5f - envW * 0.5f, ds.y * 0.35f };
			ImVec2 envMx{ envMn.x + envW, envMn.y + envH };

			dl->AddRectFilled(envMn, envMx, IM_COL32(220, 210, 180, 255), 4.f);
			dl->AddRect(envMn, envMx, IM_COL32(160, 140, 110, 230), 4.f, 0, 2.f);

			ImVec2 flapPts[3] = {
				envMn,
				{ ds.x * 0.5f, envMn.y + envH * 0.3f },
				{ envMx.x, envMn.y }
			};
			dl->AddTriangleFilled(flapPts[0], flapPts[1], flapPts[2], IM_COL32(200, 190, 160, 255));
			dl->AddTriangle(flapPts[0], flapPts[1], flapPts[2], IM_COL32(160, 140, 110, 200), 1.5f);

			float stampSize = envH * 0.25f;
			ImVec2 stampMn{ envMx.x - stampSize - 10.f, envMn.y + 10.f };
			ImVec2 stampMx{ stampMn.x + stampSize, stampMn.y + stampSize };
			dl->AddRectFilled(stampMn, stampMx, IM_COL32(180, 40, 30, 230), 2.f);

			float textX = envMn.x + 15.f;
			float textY = envMn.y + envH * 0.4f;
			std::string toStr = "To: " + letter.to;
			std::string fromStr = "From: " + letter.from;
			dl->AddText(df, df->FontSize * 1.1f, { textX, textY }, IM_COL32(60, 50, 40, 255), toStr.c_str());
			dl->AddText(df, df->FontSize * 1.1f, { textX, textY + df->FontSize * 1.4f }, IM_COL32(60, 50, 40, 255), fromStr.c_str());

			if (!letter.date.empty())
			{
				ImVec2 dsz = df->CalcTextSizeA(df->FontSize * 0.9f, FLT_MAX, 0.f, letter.date.c_str());
				dl->AddText(df, df->FontSize * 0.9f, { envMx.x - dsz.x - 10.f, envMx.y - df->FontSize * 1.2f }, IM_COL32(100, 90, 70, 200), letter.date.c_str());
			}

			if (letters.size() > 1)
			{
				float arrowSize = 40.f;
				float arrowY = envMn.y + envH * 0.5f;
				float leftArrowX = envMn.x - arrowSize - 20.f;
				float rightArrowX = envMx.x + 20.f;

				dl->AddText(df, df->FontSize * 2.f, { leftArrowX, arrowY - df->FontSize }, IM_COL32(220, 200, 160, 230), "<");
				dl->AddText(df, df->FontSize * 2.f, { rightArrowX, arrowY - df->FontSize }, IM_COL32(220, 200, 160, 230), ">");

				std::string counter = std::to_string(s_inboxIndex + 1) + " / " + std::to_string(letters.size());
				ImVec2 csz = df->CalcTextSizeA(df->FontSize, FLT_MAX, 0.f, counter.c_str());
				dl->AddText(df, df->FontSize, { ds.x * 0.5f - csz.x * 0.5f, envMx.y + 15.f }, IM_COL32(200, 180, 140, 200), counter.c_str());
			}
		}

		const char* navHint = WJConfig::Letters_NavHint.c_str();
		ImVec2 nhsz = df->CalcTextSizeA(df->FontSize, FLT_MAX, 0.f, navHint);
		dl->AddText(df, df->FontSize, { ds.x * 0.5f - nhsz.x * 0.5f, ds.y - df->FontSize * 2.5f }, IM_COL32(200, 188, 160, 200), navHint);
	}

	void RenderLetterRead()
	{
		if (!s_readingLetter) return;

		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		ImVec2 ds = io.DisplaySize;

		dl->AddRectFilled({ 0, 0 }, ds, IM_COL32(0, 0, 0, 200));

		ImFont* f = io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];
		ImFont* df = ImGui::GetFont();

		float letterW = ds.x * 0.5f;
		float letterH = ds.y * 0.6f;
		ImVec2 letterMn{ ds.x * 0.5f - letterW * 0.5f, ds.y * 0.2f };
		ImVec2 letterMx{ letterMn.x + letterW, letterMn.y + letterH };

		dl->AddRectFilled(letterMn, letterMx, IM_COL32(210, 200, 175, 255), 3.f);
		dl->AddRect(letterMn, letterMx, IM_COL32(160, 140, 110, 230), 3.f, 0, 1.5f);

		const auto& letters = s_showingSent ? s_sentLetters : s_receivedLetters;
		if (s_readingLetterId >= 0 && s_readingLetterId < (int)letters.size())
		{
			const Letter& letter = letters[s_readingLetterId];

			if (!letter.text.empty())
			{
				float pad = 20.f;
				ImVec2 tmin(letterMn.x + pad, letterMn.y + pad);
				ImVec2 tmax(letterMx.x - pad, letterMx.y - pad);
				float textW = tmax.x - tmin.x;
				float fontSize = f->FontSize * 0.85f;
				float lineH = fontSize * 1.45f;
				float curY = tmin.y;

				dl->PushClipRect(letterMn, letterMx, true);

				std::istringstream stream(letter.text);
				std::string rawLine;
				while (std::getline(stream, rawLine))
				{
					if (curY + lineH > tmax.y) break;
					if (rawLine.empty())
					{
						curY += lineH;
						continue;
					}
					dl->AddText(f, fontSize, { tmin.x, curY }, IM_COL32(48, 38, 30, 255), rawLine.c_str());
					curY += lineH;
				}

				dl->PopClipRect();
			}
		}

		const char* readHint = WJConfig::Letters_ReadHint.c_str();
		ImVec2 rhsz = df->CalcTextSizeA(df->FontSize, FLT_MAX, 0.f, readHint);
		dl->AddText(df, df->FontSize, { ds.x * 0.5f - rhsz.x * 0.5f, ds.y - df->FontSize * 2.5f }, IM_COL32(200, 188, 160, 200), readHint);
	}
}
