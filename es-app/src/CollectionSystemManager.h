#pragma once

#include <vector>
#include <string>
#include "FileData.h"
#include "Window.h"
#include "MetaData.h"
#include "PlatformId.h"
#include "ThemeData.h"
#include "FileFilterIndex.h"
#include "SystemData.h"
#include "views/ViewController.h"

enum CollectionSystemType
{
	AUTO_ALL_GAMES,
	AUTO_LAST_PLAYED,
	AUTO_FAVORITES,
	CUSTOM_COLLECTION
};

struct CollectionSystemDecl
{
	CollectionSystemType type; // type of system
	std::string name;
	std::string longName;
	std::string defaultSort;
	std::string themeFolder;
	bool isCustom;
};

struct CollectionSystemData
{
	SystemData* system;
	CollectionSystemDecl decl;
	bool isEnabled;
};

class CollectionSystemManager
{
public:
	CollectionSystemManager(Window* window);
	~CollectionSystemManager();
	static void init(Window* window);
	static CollectionSystemManager* get();
	void loadEnabledListFromSettings();
	void loadCollectionSystems();
	void updateCollectionSystems(FileData* file);
	void deleteCollectionFiles(FileData* file);
	inline std::map<std::string, CollectionSystemData> getCollectionSystems() { return mAllCollectionSystemsDecls; };
	void updateSystemsList();
	bool isThemeAutoCompatible();
	bool toggleGameInCollection(FileData* file, std::string collection);

private:
	static CollectionSystemManager* sInstance;
	std::map<std::string, CollectionSystemDecl> mCollectionSystemDecls;
	SystemEnvironmentData* mCollectionEnvData;
	static FileData::SortType getSortType(std::string desc);
	void initAvailableSystemsList();
	void loadCollectionSystem(std::string name, CollectionSystemDecl sysDecl, std::vector<SystemData*>* collectionVector);

	SystemData* createNewCollectionEntry(std::string name, CollectionSystemDecl sysDecl, std::vector<SystemData*>* collectionVector);
	void populateCollection(SystemData* newSys, CollectionSystemDecl sysDecl);

	std::vector<std::string> getSystemsFromConfig();
	std::vector<std::string> getSystemsFromTheme();
	std::vector<std::string> getUnusedSystemsFromTheme();
	std::vector<std::string> getAutoThemeFolders();
	bool themeFolderExists(std::string folder);
	void loadAutoCollectionSystems();
	void loadCustomCollectionSystems();
	void loadCustomCollectionConfigurations();
	SystemData* findCollectionSystem(std::string name);
	bool includeFileInAutoCollections(FileData* file);
	std::map<std::string, CollectionSystemData> mAllCollectionSystemsDecls;
	std::vector<SystemData*> mAutoCollectionSystems;
	std::vector<SystemData*> mCustomCollectionSystems;
	Window* mWindow;
};
