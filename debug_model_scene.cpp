#pragma execution_character_set("utf-8")
/*==============================================================================
   デバッグモデルビューアシーン [debug_model_scene.cpp]
   asset\model フォルダ内の .fbx を自動列挙し、グリッド状に配置して表示する。
   Ghost と家具憑依機能のみを残し、モデルに近づくとモデル名と
   block_definitions.json への登録状況を描画する。
==============================================================================*/

#include <d3d11.h>
#include <DirectXMath.h>
#include <windows.h>
#include <string>
#include <vector>
#include <cmath>
#include <fstream>
#include <map>
using namespace DirectX;

#include "debug_model_scene.h"
#include "direct3d.h"
#include "shader.h"
#include "camera.h"
#include "ghost.h"
#include "furniture.h"
#include "model.h"
#include "sprite3d.h"
#include "sprite.h"
#include "font.h"
#include "keyboard.h"
#include "mouse.h"
#include "light.h"
#include "define.h"
#include "UI_PauseMenu.h"
#include "field.h"

// ======================================================
// 内部構造体
// ======================================================

// 展示モデル1体分の情報
struct DebugModelEntry
{
	std::string fileName;       // "bathtub.fbx" など
	std::string filePath;       // "asset\\model\\bathtub.fbx"
	XMFLOAT3    worldPos;       // 配置ワールド座標
	bool        isInBlockDef;   // block_definitions.json に登録されているか
	std::string blockDefNameEn; // 登録されていれば英語名 (name)
	std::string blockDefName;   // 登録されていれば日本語名 (names_ja)
	std::string blockDefAction; // 登録されていればアクション (action)
	int         blockDefID;     // 登録されていればブロックID（-1 = 未登録）
};

// ======================================================
// 静的変数
// ======================================================
static std::vector<DebugModelEntry> g_Entries;
static AmbientLight* g_pAmbientLight = nullptr;
static PointLight* g_pFloorLight = nullptr;

// 近接表示用フォント
static FontRenderer* g_pModelNameFont = nullptr;
static FontRenderer* g_pSubInfoFont = nullptr;
static FontRenderer* g_pControlHintFont = nullptr;

// 配置パラメータ
static const float MODEL_SPACING = 5.0f;  // モデル同士の間隔
static const float LABEL_RANGE = 8.0f;  // この距離以内で名前を表示

// block_definitions.json のブロック情報
struct BlockDefInfo
{
	std::string nameEn;   // name
	std::string nameJa;   // names_ja
	std::string action;   // action
	int         id;       // default_id
};
static std::map<std::string, BlockDefInfo> g_BlockDefModelMap;

// ======================================================
// block_definitions.json を解析してモデルパスをキーにしたマップを作る
// ======================================================
static void LoadBlockDefMap()
{
	g_BlockDefModelMap.clear();

	std::ifstream file("SchemToArray\\block_definitions.json");
	if (!file.is_open()) return;

	std::string content;
	file.seekg(0, std::ios::end);
	content.reserve(static_cast<size_t>(file.tellg()));
	file.seekg(0, std::ios::beg);
	content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	// "blocks" 配列の開始位置を探す
	size_t blocksKey = content.find("\"blocks\"");
	if (blocksKey == std::string::npos) return;

	size_t arrayStart = content.find('[', blocksKey);
	if (arrayStart == std::string::npos) return;

	// 配列の終了位置を探す
	size_t arrayEnd = arrayStart + 1;
	int bracketLevel = 1;
	while (bracketLevel > 0 && arrayEnd < content.length())
	{
		char c = content[arrayEnd];
		if (c == '"')
		{
			// 文字列リテラルをスキップ
			arrayEnd++;
			while (arrayEnd < content.length())
			{
				if (content[arrayEnd] == '\\') { arrayEnd += 2; continue; }
				if (content[arrayEnd] == '"') break;
				arrayEnd++;
			}
		}
		else if (c == '[') bracketLevel++;
		else if (c == ']') bracketLevel--;
		arrayEnd++;
	}

	std::string arrayContent = content.substr(arrayStart, arrayEnd - arrayStart);

	// 文字列内の値を取得するヘルパー（最初に見つかった "key": "value" を返す）
	// JSONエスケープをデコードして返す
	auto findStr = [](const std::string& obj, const char* key) -> std::string {
		std::string sk = std::string("\"") + key + "\"";
		size_t p = obj.find(sk);
		if (p == std::string::npos) return "";
		size_t c = obj.find(':', p + sk.length());
		if (c == std::string::npos) return "";
		size_t s = obj.find('"', c + 1);
		if (s == std::string::npos) return "";
		// エスケープを考慮しつつ終了位置を探し、デコードした文字列を構築
		std::string result;
		size_t e = s + 1;
		while (e < obj.length())
		{
			if (obj[e] == '\\' && e + 1 < obj.length())
			{
				// JSONエスケープをデコード
				char next = obj[e + 1];
				switch (next)
				{
				case '"':  result += '"'; break;
				case '\\': result += '\\'; break;
				case '/':  result += '/'; break;
				case 'n':  result += '\n'; break;
				case 'r':  result += '\r'; break;
				case 't':  result += '\t'; break;
				default:   result += next; break;
				}
				e += 2;
				continue;
			}
			if (obj[e] == '"') break;
			result += obj[e];
			e++;
		}
		return result;
	};

	auto findInt = [](const std::string& obj, const char* key) -> int {
		std::string sk = std::string("\"") + key + "\"";
		size_t p = obj.find(sk);
		if (p == std::string::npos) return -1;
		size_t c = obj.find(':', p + sk.length());
		if (c == std::string::npos) return -1;
		size_t s = obj.find_first_of("0123456789", c + 1);
		if (s == std::string::npos) return -1;
		size_t e = obj.find_first_not_of("0123456789", s);
		std::string v = (e == std::string::npos) ? obj.substr(s) : obj.substr(s, e - s);
		return v.empty() ? -1 : std::stoi(v);
	};

	// blocks 配列内の第一階層の { } だけを取り出す
	size_t pos = 1; // '[' の次から
	while (pos < arrayContent.length())
	{
		size_t objStart = arrayContent.find('{', pos);
		if (objStart == std::string::npos) break;

		// 対応する '}' を探す（ネストを考慮）
		size_t objEnd = objStart + 1;
		int level = 1;
		while (level > 0 && objEnd < arrayContent.length())
		{
			char c = arrayContent[objEnd];
			if (c == '"')
			{
				objEnd++;
				while (objEnd < arrayContent.length())
				{
					if (arrayContent[objEnd] == '\\') { objEnd += 2; continue; }
					if (arrayContent[objEnd] == '"') break;
					objEnd++;
				}
			}
			else if (c == '{') level++;
			else if (c == '}') level--;
			objEnd++;
		}

		std::string obj = arrayContent.substr(objStart, objEnd - objStart);

		std::string mdl = findStr(obj, "model");
		if (!mdl.empty())
		{
			BlockDefInfo info;
			info.nameEn = findStr(obj, "name");
			info.nameJa = findStr(obj, "names_ja");
			info.action = findStr(obj, "action");
			info.id = findInt(obj, "default_id");
			g_BlockDefModelMap[mdl] = info;
		}

		// 次のオブジェクトへ（ネストされた中身はスキップ済み）
		pos = objEnd;
	}
}

// ======================================================
// asset\model フォルダを列挙して .fbx を集める
// ======================================================
static void EnumerateModels()
{
	WIN32_FIND_DATAA fd;
	HANDLE hFind = FindFirstFileA("asset\\model\\*.fbx", &fd);
	if (hFind == INVALID_HANDLE_VALUE) return;

	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

		DebugModelEntry entry;
		entry.fileName = fd.cFileName;
		entry.filePath = "asset\\model\\" + entry.fileName;
		entry.worldPos = { 0, 0, 0 };
		entry.isInBlockDef = false;
		entry.blockDefNameEn = "";
		entry.blockDefName = "";
		entry.blockDefAction = "";
		entry.blockDefID = -1;

		// block_definitions.json にあるか確認
		if (g_BlockDefModelMap.count(entry.filePath) > 0)
		{
			const BlockDefInfo& info = g_BlockDefModelMap[entry.filePath];
			entry.isInBlockDef = true;
			entry.blockDefNameEn = info.nameEn;
			entry.blockDefName = info.nameJa;
			entry.blockDefAction = info.action;
			entry.blockDefID = info.id;
		}

		g_Entries.push_back(entry);
	} while (FindNextFileA(hFind, &fd));

	FindClose(hFind);
}

// ======================================================
// Initialize
// ======================================================
void DebugModelScene_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバッグシーンではマップの壁判定を無効化する
	Field_SetWallCheckEnabled(false);

	// ライト
	g_pAmbientLight = new AmbientLight(XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));

	// block_definitions.json 解析
	LoadBlockDefMap();

	// モデル列挙
	EnumerateModels();

	// グリッド配置: 1列あたりのモデル数
	int cols = 6;
	for (int i = 0; i < (int)g_Entries.size(); i++)
	{
		int row = i / cols;
		int col = i % cols;
		float x = (col - cols / 2.0f + 0.5f) * MODEL_SPACING;
		float z = (float)row * MODEL_SPACING;
		float y = 0.0f;

		g_Entries[i].worldPos = { x, y, z };

		// 家具（憑依用）を生成: ACTION_SCARE で統一（描画もこれで行う）
		CreateFurniture(
			{ x, y, z },
			{ 1.0f, 1.0f, 1.0f },
			{ 0.0f, 0.0f, 0.0f },
			g_Entries[i].filePath.c_str(),
			ACTION_SCARE,
			g_Entries[i].blockDefID >= 0 ? g_Entries[i].blockDefID : 0
		);
	}

	// カメラ初期化
	Camera_Initialize();

	// Ghost 初期化（開始位置をグリッド手前に設定）
	Ghost_Initialize(pDevice, pContext);
	Ghost* ghost = GetGhost();
	if (ghost)
	{
		ghost->SetPos({ 0.0f, GHOST_POS_Y, -3.0f });
		Camera_SetTargetPos(ghost->GetPos());
	}

	// フォント
	g_pModelNameFont = new FontRenderer({ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 140.0f }, 36.0f, 0.0f, { 1,1,1,1 }, "");
	g_pSubInfoFont = new FontRenderer({ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 90.0f }, 28.0f, 0.0f, { 0.8f,0.8f,0.2f,1 }, "");
	g_pControlHintFont = new FontRenderer({ SCREEN_WIDTH / 2.0f,                  30.0f }, 22.0f, 0.0f, { 0.6f,0.6f,0.6f,1 }, "WASD:Move  Mouse:Look  SPACE:Possess  E:Release");
	g_pControlHintFont->PreCacheGlyphs();

	// ポーズメニュー初期化
	UI_PauseMenu_Initialize(pDevice, pContext);
}

// ======================================================
// Update
// ======================================================
void DebugModelScene_Update(void)
{
	// ポーズメニュー更新（ESCキー判定含む）
	UI_PauseMenu_Update();
	if (UI_PauseMenu_IsPaused())
	{
		return;
	}

	Camera_Update();
	Shader_SetCameraPos(GetCamera()->GetPos());

	Ghost_Update();
	Furniture_Update();

	// 近接モデルの判定とフォントテキスト更新
	Ghost* ghost = GetGhost();
	if (!ghost) return;

	XMFLOAT3 gp = ghost->GetPos();
	float nearestDist = FLT_MAX;
	int nearestIdx = -1;

	for (int i = 0; i < (int)g_Entries.size(); i++)
	{
		float dx = g_Entries[i].worldPos.x - gp.x;
		float dz = g_Entries[i].worldPos.z - gp.z;
		float dist = sqrtf(dx * dx + dz * dz);
		if (dist < nearestDist)
		{
			nearestDist = dist;
			nearestIdx = i;
		}
	}

	if (nearestIdx >= 0 && nearestDist < LABEL_RANGE)
	{
		const DebugModelEntry& e = g_Entries[nearestIdx];

		// モデル名（ファイル名）
		g_pModelNameFont->SetText(e.fileName);
		g_pModelNameFont->PreCacheGlyphs();

		// 補助情報
		std::string sub;
		if (e.isInBlockDef)
		{
			sub = e.blockDefName + " [" + e.blockDefNameEn + "]  action:" + e.blockDefAction + "  ID:" + std::to_string(e.blockDefID);
		}
		else
		{
			sub = "JSON定義なし（block_definitions.json に未登録）";
		}
		g_pSubInfoFont->SetText(sub);
		g_pSubInfoFont->PreCacheGlyphs();
	}
	else
	{
		g_pModelNameFont->SetText("");
		g_pSubInfoFont->SetText("");
	}
}

// ======================================================
// Draw
// ======================================================
void DebugModelScene_Draw(void)
{
	// 3D 描画
	SetDepthTest(true);

	Shader_SetAmbientLight(g_pAmbientLight);
	Shader_ClearPointLights();
	Ghost_SetLight();

	// 家具描画（展示モデル＋ビルボードアイコン含む）
	Furniture_Draw();

	Ghost_Draw();

	// 2D 描画
	SetDepthTest(false);
	Sprite_BeginDraw2D();

	if (g_pModelNameFont)   g_pModelNameFont->Draw();
	if (g_pSubInfoFont)     g_pSubInfoFont->Draw();
	if (g_pControlHintFont) g_pControlHintFont->Draw();

	// ポーズメニュー描画（2D描画内で最後に描く）
	UI_PauseMenu_Draw();

	Sprite_EndDraw2D();
}

// ======================================================
// Finalize
// ======================================================
void DebugModelScene_Finalize(void)
{
	g_Entries.clear();

	Furniture_Finalize();
	Ghost_Finalize();
	Camera_Finalize();

	if (g_pAmbientLight) { delete g_pAmbientLight; g_pAmbientLight = nullptr; }
	if (g_pFloorLight) { delete g_pFloorLight;   g_pFloorLight = nullptr; }

	if (g_pModelNameFont) { delete g_pModelNameFont;   g_pModelNameFont = nullptr; }
	if (g_pSubInfoFont) { delete g_pSubInfoFont;     g_pSubInfoFont = nullptr; }
	if (g_pControlHintFont) { delete g_pControlHintFont; g_pControlHintFont = nullptr; }

	// ポーズメニュー終了処理
	UI_PauseMenu_Finalize();

	g_BlockDefModelMap.clear();

	// 壁判定を元に戻す
	Field_SetWallCheckEnabled(true);
}
