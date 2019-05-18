﻿#include "Music.h"
#include "SusAnalyzer.h"
#include "SettingManager.h"


using namespace std;
using namespace std::filesystem;

namespace ScoreDifficulty {
	bool RegisterTypes(asIScriptEngine* engine)
	{
		bool result = true;
		result = (result && 0 <= engine->RegisterEnum(SU_IF_SCORE_DIFFICULTY));
		result = (result && 0 <= engine->RegisterEnumValue(SU_IF_SCORE_DIFFICULTY, "Basic", int(Enum::Basic)));
		result = (result && 0 <= engine->RegisterEnumValue(SU_IF_SCORE_DIFFICULTY, "Advanced", int(Enum::Advanced)));
		result = (result && 0 <= engine->RegisterEnumValue(SU_IF_SCORE_DIFFICULTY, "Expert", int(Enum::Expert)));
		result = (result && 0 <= engine->RegisterEnumValue(SU_IF_SCORE_DIFFICULTY, "Master", int(Enum::Master)));
		result = (result && 0 <= engine->RegisterEnumValue(SU_IF_SCORE_DIFFICULTY, "WorldsEnd", int(Enum::WorldsEnd)));
		return result;
	}
}


ScoreParameter::ScoreParameter()
	: refcount(1)
{}

ScoreParameter::~ScoreParameter()
{
#ifdef _DEBUG
	SU_ASSERT(GetRefcount() == 0);
#endif
}

ScoreParameter* ScoreParameter::Create(SusAnalyzer* analyzer, const path& cpath)
{
	SU_ASSERT(analyzer);

	if (!exists(cpath) || !is_regular_file(cpath)) return nullptr;

	analyzer->Reset();
	if (!analyzer->LoadFromFile(cpath, true)) return nullptr;

	const auto& meta = analyzer->SharedMetaData;
	const auto& root = cpath.parent_path();

	auto ptr = new ScoreParameter();

	ptr->ScorePath = ConvertUnicodeToUTF8(cpath);
	ptr->SongId = meta.USongId;

	ptr->Title = meta.UTitle;
	ptr->Artist = meta.UArtist;
	ptr->Designer = meta.UDesigner;
	ptr->Difficulty = ScoreDifficulty::Enum(meta.DifficultyType);
	ptr->Level = meta.Level;
	ptr->DifficultyName = meta.UExtraDifficulty;
	ptr->Bpm = meta.ShowBpm;

	ptr->JacketPath = ConvertUnicodeToUTF8(root / ConvertUTF8ToUnicode(meta.UJacketFileName));
	ptr->WavePath = ConvertUnicodeToUTF8(root / ConvertUTF8ToUnicode(meta.UWaveFileName));
	ptr->WaveOffset = meta.WaveOffset;
	ptr->BackgroundPath = ConvertUnicodeToUTF8(root / ConvertUTF8ToUnicode(meta.UBackgroundFileName));
	ptr->BackgroundOffset = meta.MovieOffset;

	return ptr;
}

bool ScoreParameter::RegisterTypes(asIScriptEngine* engine)
{
	// TODO: プロパティではなくゲッタ―にして、スキン側では読み出し専用にしたい
	bool result = true;
	result = (result && 0 <= engine->RegisterObjectType(SU_IF_SCORE, 0, asOBJ_REF));
	result = (result && 0 <= engine->RegisterObjectBehaviour(SU_IF_SCORE, asBEHAVE_ADDREF, "void f()", asMETHOD(ScoreParameter, AddRef), asCALL_THISCALL));
	result = (result && 0 <= engine->RegisterObjectBehaviour(SU_IF_SCORE, asBEHAVE_RELEASE, "void f()", asMETHOD(ScoreParameter, Release), asCALL_THISCALL));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, "string ID", asOFFSET(ScoreParameter, SongId)));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, "string Path", asOFFSET(ScoreParameter, ScorePath)));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, "string Title", asOFFSET(ScoreParameter, Title)));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, "string Artist", asOFFSET(ScoreParameter, Artist)));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, "string Designer", asOFFSET(ScoreParameter, Designer)));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, SU_IF_SCORE_DIFFICULTY " Difficulty", asOFFSET(ScoreParameter, Difficulty)));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, "uint Level", asOFFSET(ScoreParameter, Level)));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, "string DifficultyName", asOFFSET(ScoreParameter, DifficultyName)));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, "double Bpm", asOFFSET(ScoreParameter, Bpm)));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, "string JacketPath", asOFFSET(ScoreParameter, JacketPath)));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, "string WavePath", asOFFSET(ScoreParameter, WavePath)));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, "double WaveOffset", asOFFSET(ScoreParameter, WaveOffset)));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, "string BackgroundPath", asOFFSET(ScoreParameter, BackgroundPath)));
	result = (result && 0 <= engine->RegisterObjectProperty(SU_IF_SCORE, "double BackgroundOffset", asOFFSET(ScoreParameter, BackgroundOffset)));
	return result;
}



CategoryParameter::CategoryParameter()
	: refcount(1)
{}

CategoryParameter::~CategoryParameter()
{
#ifdef _DEBUG
	SU_ASSERT(GetRefcount() == 0);
#endif

	for (auto& p : music) p->Release();
}

bool CategoryParameter::AddMusic(MusicParameter* pMusic)
{
	if (!pMusic) return false;

	music.push_back(pMusic);

	return true;
}

MusicParameter* CategoryParameter::GetMusic(uint32_t index) const
{
	const auto size = GetMusicCount();
	if (size == 0) return nullptr;

	auto ptr = music[index % size];
	if (ptr) ptr->AddRef();
	return ptr;
}

MusicParameter* CategoryParameter::GetMusic(const std::string& songId) const
{
	// TODO: musicをMapか何かにして検索時間を減らす
	for (const auto p : music) {
		if (p && p->songId == songId) {
			p->AddRef();
			return p;
		}
	}

	return nullptr;
}


CategoryParameter* CategoryParameter::Create(SusAnalyzer* analyzer, const std::filesystem::path& cpath)
{
	if (!is_directory(cpath) || !exists(cpath)) return nullptr;

	auto category = new CategoryParameter();
	for (const auto& dir : directory_iterator(cpath)) {
		if (!is_directory(dir)) continue;
		for (const auto& file : directory_iterator(dir)) {
			if (is_directory(file)) continue;
			if (file.path().extension() != ".sus") continue;     //これ大文字どうすんの

			auto ptr = ScoreParameter::Create(analyzer, file);
			if (!ptr) continue;

			auto music = category->GetMusic(ptr->SongId);
			if (!music) {
				music = MusicParameter::Create(ptr->SongId);
				if (!music) {
					ptr->Release();
					continue;
				}

				music->AddRef();
				category->AddMusic(music);
			}

			ptr->AddRef();
			music->AddScore(ptr);

			music->Release();
			ptr->Release();
		}
	}

	if (category->GetMusicCount() > 0) {
		category->name = ConvertUnicodeToUTF8(cpath.filename());
	}
	else {
		category->Release();
		category = nullptr;
	}

	return category;
}

bool CategoryParameter::RegisterTypes(asIScriptEngine* engine)
{
	bool result = true;
	result = (result && 0 <= engine->RegisterObjectType(SU_IF_CATEGORY, 0, asOBJ_REF));
	result = (result && 0 <= engine->RegisterObjectBehaviour(SU_IF_CATEGORY, asBEHAVE_ADDREF, "void f()", asMETHOD(CategoryParameter, AddRef), asCALL_THISCALL));
	result = (result && 0 <= engine->RegisterObjectBehaviour(SU_IF_CATEGORY, asBEHAVE_RELEASE, "void f()", asMETHOD(CategoryParameter, Release), asCALL_THISCALL));
	result = (result && 0 <= engine->RegisterObjectMethod(SU_IF_CATEGORY, "string get_Name()", asMETHOD(CategoryParameter, GetName), asCALL_THISCALL));
	result = (result && 0 <= engine->RegisterObjectMethod(SU_IF_CATEGORY, "uint get_MusicCount()", asMETHOD(CategoryParameter, GetMusicCount), asCALL_THISCALL));
	result = (result && 0 <= engine->RegisterObjectMethod(SU_IF_CATEGORY, SU_IF_MUSIC "@ GetMusic(uint)", asMETHODPR(CategoryParameter, GetMusic, (uint32_t) const, MusicParameter*), asCALL_THISCALL));
	result = (result && 0 <= engine->RegisterObjectMethod(SU_IF_CATEGORY, SU_IF_MUSIC "@ GetMusic(string)", asMETHODPR(CategoryParameter, GetMusic, (const string&) const, MusicParameter*), asCALL_THISCALL));
	return result;
}


MusicParameter::MusicParameter()
	:refcount(1)
{}

MusicParameter::~MusicParameter()
{
#ifdef _DEBUG
	SU_ASSERT(GetRefcount() == 0);
#endif

	for (auto& p : scores) p->Release();
}

bool MusicParameter::AddScore(ScoreParameter* pScore)
{
	if (!pScore) return false;

	scores.push_back(pScore);

	return true;
}

ScoreParameter* MusicParameter::GetScore(uint32_t index) const
{
	const auto size = GetScoreCount();
	if (size == 0) return nullptr;

	auto ptr = scores[index % size];
	if (ptr) ptr->AddRef();
	return ptr;
}

MusicParameter* MusicParameter::Create(const string& songID)
{
	auto ptr = new MusicParameter();
	if (!ptr) return nullptr;

	ptr->songId = songID;

	return ptr;
}

bool MusicParameter::RegisterTypes(asIScriptEngine * engine)
{
	bool result = true;
	result = (result && 0 <= engine->RegisterObjectType(SU_IF_MUSIC, 0, asOBJ_REF));
	result = (result && 0 <= engine->RegisterObjectBehaviour(SU_IF_MUSIC, asBEHAVE_ADDREF, "void f()", asMETHOD(MusicParameter, AddRef), asCALL_THISCALL));
	result = (result && 0 <= engine->RegisterObjectBehaviour(SU_IF_MUSIC, asBEHAVE_RELEASE, "void f()", asMETHOD(MusicParameter, Release), asCALL_THISCALL));
	result = (result && 0 <= engine->RegisterObjectMethod(SU_IF_MUSIC, "string get_Id()", asMETHOD(MusicParameter, GetSongId), asCALL_THISCALL));
	result = (result && 0 <= engine->RegisterObjectMethod(SU_IF_MUSIC, "uint get_ScoreCount()", asMETHOD(MusicParameter, GetScoreCount), asCALL_THISCALL));
	result = (result && 0 <= engine->RegisterObjectMethod(SU_IF_MUSIC, SU_IF_SCORE "@ GetScore(uint)", asMETHOD(MusicParameter, GetScore), asCALL_THISCALL));
	return result;
}
