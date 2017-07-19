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
	bool isPopulated;
	bool needsSave;
};

class CollectionSystemManager
{
public:
	CollectionSystemManager(Window* window);
	~CollectionSystemManager();

	static CollectionSystemManager* get();
	static void init(Window* window);
	static void deinit();
	void saveCustomCollection(SystemData* sys);

	void loadCollectionSystems();
	void loadEnabledListFromSettings();
	void updateSystemsList();

	void updateCollectionSystems(FileData* file);
	void deleteCollectionFiles(FileData* file);

	inline std::map<std::string, CollectionSystemData> getAutoCollectionSystems() { return mAutoCollectionSystemsData; };
	inline std::map<std::string, CollectionSystemData> getCustomCollectionSystems() { return mCustomCollectionSystemsData; };
	std::vector<std::string> getUnusedSystemsFromTheme();
	void addNewCustomCollection(std::string name);

	bool isThemeCollectionCompatible(bool customCollections);

	bool toggleGameInCollection(FileData* file, std::string collection);

private:
	static CollectionSystemManager* sInstance;
	SystemEnvironmentData* mCollectionEnvData;
	std::map<std::string, CollectionSystemDecl> mCollectionSystemDeclsIndex;
	std::map<std::string, CollectionSystemData> mAutoCollectionSystemsData;
	std::map<std::string, CollectionSystemData> mCustomCollectionSystemsData;
	Window* mWindow;

	void initAutoCollectionSystems();
	void initCustomCollectionSystems();
	SystemData* getAllGamesCollection();
	SystemData* createNewCollectionEntry(std::string name, CollectionSystemDecl sysDecl);
	void populateAutoCollection(SystemData* newSys, CollectionSystemDecl sysDecl);
	void populateCustomCollection(SystemData* newSys, CollectionSystemDecl sysDecl);

	void removeCollectionsFromDisplayedSystems();
	void addEnabledCollectionsToDisplayedSystems(std::map<std::string, CollectionSystemData>* colSystemData);

	std::vector<std::string> getSystemsFromConfig();
	std::vector<std::string> getSystemsFromTheme();
	std::vector<std::string> getCollectionsFromConfigFolder();
	std::vector<std::string> getCollectionThemeFolders(bool custom);

	bool themeFolderExists(std::string folder);

	bool includeFileInAutoCollections(FileData* file);
};

std::string getCustomCollectionConfigPath(std::string collectionName);
std::string getCollectionsFolder();
