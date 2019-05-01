﻿#pragma once

#define SU_IF_SKIN "Skin"
#define SU_IF_SIZE "Size"
#define SU_IF_VOID_PTR "Address"

class AngelScript;
class SImage;
class SFont;
class SSound;
class SAnimatedImage;

class SkinHolder final {
private:
	const std::shared_ptr<AngelScript> scriptInterface;
	const std::filesystem::path skinRoot;

	std::unordered_map<std::string, SImage*> images;
	std::unordered_map<std::string, SFont*> fonts;
	std::unordered_map<std::string, SSound*> sounds;
	std::unordered_map<std::string, SAnimatedImage*> animatedImages;

public:
	SkinHolder(const std::wstring& name, const std::shared_ptr<AngelScript>& script);
	~SkinHolder();

	bool Initialize();
	void Terminate();

	asIScriptObject* ExecuteSkinScript(const std::filesystem::path& file, bool forceReload = false);

	void LoadSkinImage(const std::string& key, const std::string& filename);
	void LoadSkinImageFromMem(const std::string& key, void* buffer, size_t size);
	void LoadSkinFont(const std::string& key, const std::string& filename);
	void LoadSkinFontFromMem(const std::string& key, void* buffer, size_t size);
	void LoadSkinSound(const std::string& key, const std::string& filename);
	void LoadSkinSoundFromMem(const std::string& key, const void* buffer, size_t size);
	void LoadSkinAnime(const std::string& key, const std::string& filename, int x, int y, int w, int h, int c, double time);
	void LoadSkinAnimeFromMem(const std::string& key, void* buffer, size_t size, int x, int y, int w, int h, int c, double time);
	SImage* GetSkinImage(const std::string& key);
	SFont* GetSkinFont(const std::string& key);
	SSound* GetSkinSound(const std::string& key);
	SAnimatedImage* GetSkinAnime(const std::string& key);

	static void RegisterType(asIScriptEngine* engine);
};

SkinHolder* GetSkinObject();
