#include "SystemData.h"
#include "Gamelist.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "Util.h"
#include <algorithm>
#include <fstream>
#include <stdlib.h>
#include <SDL_joystick.h>
#include "Renderer.h"
#include "Log.h"
#include "InputManager.h"
#include <iostream>
#include "Settings.h"
#include "pugixml/src/pugixml.hpp"
#include "guis/GuiInfoPopup.h"

namespace fs = boost::filesystem;

/* Handling the getting, initialization, deinitialization, saving and deletion of
 * a CollectionSystemManager Instance */
CollectionSystemManager* CollectionSystemManager::sInstance = NULL;

CollectionSystemManager::CollectionSystemManager(Window* window) : mWindow(window)
{
	CollectionSystemDecl systemDecls[] = {
		//type                  name            long name       //default sort              // theme folder            // isCustom
		{ AUTO_ALL_GAMES,       "all",          "all games",    "filename, ascending",      "auto-allgames",           false },
		{ AUTO_LAST_PLAYED,     "recent",       "last played",  "last played, descending",  "auto-lastplayed",         false },
		{ AUTO_FAVORITES,       "favorites",    "favorites",    "filename, ascending",      "auto-favorites",          false },
		{ CUSTOM_COLLECTION,    "custom",   	"my collections",   "filename, ascending",  "custom-collections",      true }
	};

	// create a map
	std::vector<CollectionSystemDecl> tempSystemDecl = std::vector<CollectionSystemDecl>(systemDecls, systemDecls + sizeof(systemDecls) / sizeof(systemDecls[0]));

	for (std::vector<CollectionSystemDecl>::iterator it = tempSystemDecl.begin(); it != tempSystemDecl.end(); ++it )
	{
		mCollectionSystemDeclsIndex[(*it).name] = (*it);
		//mCollectionSystemDeclsIndex.insert(std::make_pair((*it).name,(*it)));
	}

	// creating standard environment data
	mCollectionEnvData = new SystemEnvironmentData;
	mCollectionEnvData->mStartPath = "";
	std::vector<std::string> exts;
	mCollectionEnvData->mSearchExtensions = exts;
	mCollectionEnvData->mLaunchCommand = "";
	std::vector<PlatformIds::PlatformId> allPlatformIds;
	allPlatformIds.push_back(PlatformIds::PLATFORM_IGNORE);
	mCollectionEnvData->mPlatformIds = allPlatformIds;

	std::string path = getCollectionsFolder();
	if(!fs::exists(path))
		fs::create_directory(path);

	// TO DO: Create custom editing help style
}

CollectionSystemManager::~CollectionSystemManager()
{
	assert(sInstance == this);
	LOG(LogError) << "Removing CollectionSystemManager";
	removeCollectionsFromDisplayedSystems();
	sInstance = NULL;
}

CollectionSystemManager* CollectionSystemManager::get()
{
	assert(sInstance);
	return sInstance;
}

void CollectionSystemManager::init(Window* window)
{
	assert(!sInstance);
	sInstance = new CollectionSystemManager(window);
}

void CollectionSystemManager::deinit()
{
	if (sInstance)
	{
		delete sInstance;
	}
}

void CollectionSystemManager::saveCustomCollection(SystemData* sys)
{
	std::string name = sys->getName();
	std::unordered_map<std::string, FileData*> games = sys->getRootFolder()->getChildrenByFilename();
	if (games.size() == 0)
	{
		LOG(LogError) << "No files to save for: " << name;
		return;
	}
	bool found = mCustomCollectionSystemsData.find(name) != mCustomCollectionSystemsData.end();
	if (found) {
		CollectionSystemData sysData = mCustomCollectionSystemsData.at(name);
		if ((sysData.needsSave || true))
		{
			LOG(LogError) << "Need to save: " << name;
			std::ofstream configFile;
			configFile.open(getCustomCollectionConfigPath(name) + ".bak");
			for(std::unordered_map<std::string, FileData*>::iterator iter = games.begin(); iter != games.end(); ++iter)
			{
				std::string path =  iter->first;
				configFile << path << std::endl;
			}
			configFile.close();
		}
		else
		{
			LOG(LogError) << "Don't need to save: " << name;
		}
	}
}

/* Methods to load all Collections into memory, and handle enabling the active ones */
// loads all Collection Systems
void CollectionSystemManager::loadCollectionSystems()
{
	loadAutoCollectionSystems();
	// we will also load custom systems here in the future
	loadCustomCollectionSystems();
	// Now see which ones are enabled
	loadEnabledListFromSettings();
	// add to the main System Vector, and create Views as needed
	updateSystemsList();
}

// loads settings
void CollectionSystemManager::loadEnabledListFromSettings()
{
	// we parse the auto collection settings list
	std::vector<std::string> autoSelected = commaStringToVector(Settings::getInstance()->getString("CollectionSystemsAuto"));

	// iterate the map
	for(std::map<std::string, CollectionSystemData>::iterator it = mAutoCollectionSystemsData.begin() ; it != mAutoCollectionSystemsData.end() ; it++ )
	{
		it->second.isEnabled = (std::find(autoSelected.begin(), autoSelected.end(), it->first) != autoSelected.end());
	}

	// we parse the custom collection settings list
	std::vector<std::string> customSelected = commaStringToVector(Settings::getInstance()->getString("CollectionSystemsCustom"));

	// iterate the map
	for(std::map<std::string, CollectionSystemData>::iterator it = mCustomCollectionSystemsData.begin() ; it != mCustomCollectionSystemsData.end() ; it++ )
	{
		it->second.isEnabled = (std::find(customSelected.begin(), customSelected.end(), it->first) != customSelected.end());
	}
}

// updates enabled system list in System View
void CollectionSystemManager::updateSystemsList()
{
	// remove all Collection Systems
	removeCollectionsFromDisplayedSystems();

	// add auto enabled ones
	addEnabledCollectionsToDisplayedSystems(&mAutoCollectionSystemsData);

	// add custom enabled ones
	addEnabledCollectionsToDisplayedSystems(&mCustomCollectionSystemsData);
}

/* Methods to manage collection files related to a source FileData */
// updates all collection files related to the source file
void CollectionSystemManager::updateCollectionSystems(FileData* file)
{
	// collection files use the full path as key, to avoid clashes
	std::string key = file->getFullPath();
	// find games in collection systems
	for(auto sysIt = SystemData::sSystemVector.begin(); sysIt != SystemData::sSystemVector.end(); sysIt++)
	{
		if ((*sysIt)->isCollection()) {
			const std::unordered_map<std::string, FileData*>& children = (*sysIt)->getRootFolder()->getChildrenByFilename();
			bool found = children.find(key) != children.end();
			FileData* rootFolder = (*sysIt)->getRootFolder();
			FileFilterIndex* fileIndex = (*sysIt)->getIndex();
			std::string name = (*sysIt)->getName();
			if (found) {
				// if we found it, we need to update it
				FileData* collectionEntry = children.at(key);
				// remove from index, so we can re-index metadata after refreshing
				fileIndex->removeFromIndex(collectionEntry);
				collectionEntry->refreshMetadata();
				if (name == "favorites" && file->metadata.get("favorite") == "false") {
					// need to check if still marked as favorite, if not remove
					ViewController::get()->getGameListView((*sysIt)).get()->remove(collectionEntry, false);
					ViewController::get()->onFileChanged((*sysIt)->getRootFolder(), FILE_REMOVED);
				}
				else
				{
					// re-index with new metadata
					fileIndex->addToIndex(collectionEntry);
					ViewController::get()->onFileChanged(collectionEntry, FILE_METADATA_CHANGED);
				}
			}
			else
			{
				// we didn't find it here - we need to check if we should add it
				if (name == "recent" && file->metadata.get("playcount") > "0" && includeFileInAutoCollections(file) ||
					name == "favorites" && file->metadata.get("favorite") == "true") {
					CollectionFileData* newGame = new CollectionFileData(file, (*sysIt));
					rootFolder->addChild(newGame);
					fileIndex->addToIndex(newGame);
					ViewController::get()->onFileChanged(file, FILE_METADATA_CHANGED);
					ViewController::get()->getGameListView((*sysIt))->onFileChanged(newGame, FILE_METADATA_CHANGED);
				}
			}
			rootFolder->sort(getSortTypeFromString(mCollectionSystemDeclsIndex[name].defaultSort));
			ViewController::get()->onFileChanged(rootFolder, FILE_SORTED);
		}
	}
}

// deletes all collection files from collection systems related to the source file
void CollectionSystemManager::deleteCollectionFiles(FileData* file)
{
	// collection files use the full path as key, to avoid clashes
	std::string key = file->getFullPath();
	// find games in collection systems
	for(auto sysIt = SystemData::sSystemVector.begin(); sysIt != SystemData::sSystemVector.end(); sysIt++)
	{
		if ((*sysIt)->isCollection()) {
			const std::unordered_map<std::string, FileData*>& children = (*sysIt)->getRootFolder()->getChildrenByFilename();

			bool found = children.find(key) != children.end();
			if (found) {
				FileData* collectionEntry = children.at(key);
				ViewController::get()->getGameListView((*sysIt)).get()->remove(collectionEntry, false);
			}
		}
	}
}

// returns whether the current theme is compatible with Automatic or Custom Collections
bool CollectionSystemManager::isThemeCollectionCompatible(bool customCollections)
{
	std::vector<std::string> cfgSys = getCollectionThemeFolders(customCollections);
	for(auto sysIt = cfgSys.begin(); sysIt != cfgSys.end(); sysIt++)
	{
		if(!themeFolderExists(*sysIt))
			return false;
	}
	return true;
}

// adds or removes a game from a specific collection
bool CollectionSystemManager::toggleGameInCollection(FileData* file, std::string collection)
{
	if (file->getType() == GAME)
	{
		GuiInfoPopup* s;

		MetaDataList* md = &file->getSourceFileData()->metadata;
		std::string value = md->get("favorite");
		if (value == "false")
		{
			md->set("favorite", "true");
			s = new GuiInfoPopup(mWindow, "Added '" + removeParenthesis(file->getName()) + "' to 'Favorites'", 4000);
		}else
		{
			md->set("favorite", "false");
			s = new GuiInfoPopup(mWindow, "Removed '" + removeParenthesis(file->getName()) + "' from 'Favorites'", 4000);
		}
		mWindow->setInfoPopup(s);
		updateCollectionSystems(file->getSourceFileData());
		return true;
	}
	return false;
}

/* Handles loading a collection system, creating an empty one, and populating on demand */
// loads Automatic Collection systems (All, Favorites, Last Played)
void CollectionSystemManager::loadAutoCollectionSystems()
{
	for(std::map<std::string, CollectionSystemDecl>::iterator it = mCollectionSystemDeclsIndex.begin() ; it != mCollectionSystemDeclsIndex.end() ; it++ )
	{
		CollectionSystemDecl sysDecl = it->second;
		if (!sysDecl.isCustom && !findCollectionSystem(sysDecl.name))
		{
			loadCollectionSystem(sysDecl.name, sysDecl, &mAutoCollectionSystems);
		}
	}
}

void CollectionSystemManager::loadCustomCollectionSystems()
{
	std::vector<std::string> systems = getCollectionsFromConfigFolder();
	std::vector<std::string> themeSystems = getUnusedSystemsFromTheme();
	// append both
	systems.insert(systems.end(), themeSystems.begin(), themeSystems.end());

	for (auto nameIt = systems.begin(); nameIt != systems.end(); nameIt++)
	{
		CollectionSystemDecl decl = mCollectionSystemDeclsIndex["custom"];
		decl.themeFolder = *nameIt;
		SystemData* newSys = createNewCollectionEntry(*nameIt, decl, &mCustomCollectionSystems);
		// we should be able to delay populating until it's actually enabled
		//populateCustomCollection(newSys, decl);
	}
}

// loads a Collection system, based on the name and declaration
void CollectionSystemManager::loadCollectionSystem(std::string name, CollectionSystemDecl sysDecl, std::vector<SystemData*>* collectionVector)
{
	SystemData* newSys = createNewCollectionEntry(name, sysDecl, collectionVector);
	if (name == "all")
	{
		// we need to always populate the "all" collection as custom collections are based on that
		populateAutoCollection(newSys, sysDecl);
		mAutoCollectionSystemsData["all"].isPopulated = true;
	}
}

// creates a new, empty Collection system, based on the name and declaration
SystemData* CollectionSystemManager::createNewCollectionEntry(std::string name, CollectionSystemDecl sysDecl, std::vector<SystemData*>* collectionVector)
{
	SystemData* newSys = new SystemData(name, sysDecl.longName, mCollectionEnvData, sysDecl.themeFolder, true);
	collectionVector->push_back(newSys);

	CollectionSystemData newCollectionData;
	newCollectionData.system = newSys;
	newCollectionData.decl = sysDecl;
	newCollectionData.isEnabled = false;
	newCollectionData.isPopulated = false;
	newCollectionData.needsSave = false;
	if (!sysDecl.isCustom)
	{
		mAutoCollectionSystemsData[name] = newCollectionData;
	}
	else
	{
		newCollectionData.decl.name = name;
		newCollectionData.decl.longName = name;
		mCustomCollectionSystemsData[name] = newCollectionData;
	}

	return newSys;
}

// populates an Automatic Collection System
void CollectionSystemManager::populateAutoCollection(SystemData* newSys, CollectionSystemDecl sysDecl)
{
	FileData* rootFolder = newSys->getRootFolder();
	FileFilterIndex* index = newSys->getIndex();
	for(auto sysIt = SystemData::sSystemVector.begin(); sysIt != SystemData::sSystemVector.end(); sysIt++)
	{
		// we won't iterate all collections
		if ((*sysIt)->isGameSystem() && !(*sysIt)->isCollection()) {
			std::vector<FileData*> files = (*sysIt)->getRootFolder()->getFilesRecursive(GAME);
			for(auto gameIt = files.begin(); gameIt != files.end(); gameIt++)
			{
				bool include = includeFileInAutoCollections((*gameIt));
				switch(sysDecl.type) {
					case AUTO_LAST_PLAYED:
						include = include && (*gameIt)->metadata.get("playcount") > "0";
						break;
					case AUTO_FAVORITES:
						// we may still want to add files we don't want in auto collections in "favorites"
						include = (*gameIt)->metadata.get("favorite") == "true";
						break;
				}

				if (include) {
					CollectionFileData* newGame = new CollectionFileData(*gameIt, newSys);
					rootFolder->addChild(newGame);
					index->addToIndex(newGame);
				}
			}
		}
	}
	rootFolder->sort(getSortTypeFromString(sysDecl.defaultSort));

	if (sysDecl.name == "all")
	{
		allGamesCollection = newSys;
	}
}

// populates a Custom Collection System
void CollectionSystemManager::populateCustomCollection(SystemData* newSys, CollectionSystemDecl sysDecl)
{
	std::string path = getCustomCollectionConfigPath(newSys->getName());

	if(!fs::exists(path))
	{
		LOG(LogError) << "Couldn't find custom collection config file at " << path;
		return;
	}
	LOG(LogError) << "Loading custom collection config file at " << path;

	FileData* rootFolder = newSys->getRootFolder();
	FileFilterIndex* index = newSys->getIndex();

	// get Configuration for this Custom System
	std::ifstream input(path);

	// get all files map
	std::unordered_map<std::string,FileData*> allFilesMap = allGamesCollection->getRootFolder()->getChildrenByFilename();

	// iterate list of files in config file

	for(std::string gameKey; getline(input, gameKey); )
	{
		std::unordered_map<std::string,FileData*>::iterator it = allFilesMap.find(gameKey);
		if (it != allFilesMap.end()) {
			CollectionFileData* newGame = new CollectionFileData(it->second, newSys);
			rootFolder->addChild(newGame);
			index->addToIndex(newGame);
		}
		else
		{
			LOG(LogError) << "Couldn't find game referenced at '" << gameKey << "' for system config '" << path << "'";
		}
	}
	rootFolder->sort(getSortTypeFromString(sysDecl.defaultSort));
}

/* Handle System View removal and insertion of Collections */
void CollectionSystemManager::removeCollectionsFromDisplayedSystems()
{
	// remove all Collection Systems
	for(auto sysIt = SystemData::sSystemVector.begin(); sysIt != SystemData::sSystemVector.end(); )
	{
		if ((*sysIt)->isCollection())
		{
			sysIt = SystemData::sSystemVector.erase(sysIt);
		}
		else
		{
			sysIt++;
		}
	}
}

void CollectionSystemManager::addEnabledCollectionsToDisplayedSystems(std::map<std::string, CollectionSystemData>* colSystemData)
{
	// add auto enabled ones
	for(std::map<std::string, CollectionSystemData>::iterator it = colSystemData->begin() ; it != colSystemData->end() ; it++ )
	{
		if(it->second.isEnabled)
		{
			// check if populated, otherwise populate
			if (!it->second.isPopulated)
			{
				LOG(LogError) << "System isn't populated: " << it->second.decl.name;
				if(it->second.decl.isCustom)
				{
					populateCustomCollection(it->second.system, it->second.decl);
				}
				else
				{
					populateAutoCollection(it->second.system, it->second.decl);
				}
				it->second.isPopulated = true;
			}
			SystemData::sSystemVector.push_back(it->second.system);
		}
	}
}

/* Auxiliary methods to get available custom collection possibilities */
std::vector<std::string> CollectionSystemManager::getSystemsFromConfig()
{
	std::vector<std::string> systems;
	std::string path = SystemData::getConfigPath(false);

	if(!fs::exists(path))
	{
		return systems;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(path.c_str());

	if(!res)
	{
		return systems;
	}

	//actually read the file
	pugi::xml_node systemList = doc.child("systemList");

	if(!systemList)
	{
		return systems;
	}

	for(pugi::xml_node system = systemList.child("system"); system; system = system.next_sibling("system"))
	{
		// theme folder
		std::string themeFolder = system.child("theme").text().get();
		systems.push_back(themeFolder);
	}
	std::sort(systems.begin(), systems.end());
	return systems;
}

// gets all folders from the current theme path
std::vector<std::string> CollectionSystemManager::getSystemsFromTheme()
{
	std::vector<std::string> systems;
	auto themeSets = ThemeData::getThemeSets();
	if(themeSets.empty())
	{
		// no theme sets available
		return systems;
	}

	auto set = themeSets.find(Settings::getInstance()->getString("ThemeSet"));
	if(set == themeSets.end())
	{
		// currently selected theme set is missing, so just pick the first available set
		set = themeSets.begin();
		Settings::getInstance()->setString("ThemeSet", set->first);
	}

	fs::path themePath = set->second.path;

	if (fs::exists(themePath))
	{
		fs::directory_iterator end_itr; // default construction yields past-the-end
		for (fs::directory_iterator itr(themePath); itr != end_itr; ++itr)
		{
			if (fs::is_directory(itr->status()))
			{
				//... here you have a directory
				std::string folder = itr->path().string();
				folder = folder.substr(themePath.string().size()+1);

				if(fs::exists(set->second.getThemePath(folder)))
				{
					systems.push_back(folder);
				}
			}
		}
	}
	std::sort(systems.begin(), systems.end());
	return systems;
}

// returns the unused folders from current theme path
std::vector<std::string> CollectionSystemManager::getUnusedSystemsFromTheme()
{
	// get used systems in es_systems.cfg
	std::vector<std::string> systemsInUse = getSystemsFromConfig();
	// get available folders in theme
	std::vector<std::string> themeSys = getSystemsFromTheme();
	// get folders assigned to custom collections
	std::vector<std::string> autoSys = getCollectionThemeFolders(false);
	// get folder assigned to custom collections
	std::vector<std::string> customSys = getCollectionThemeFolders(true);
	// add them all to the list of systems in use
	systemsInUse.insert(systemsInUse.end(), autoSys.begin(), autoSys.end());
	systemsInUse.insert(systemsInUse.end(), customSys.begin(), customSys.end());

	for(auto sysIt = themeSys.begin(); sysIt != themeSys.end(); )
	{
		if (std::find(systemsInUse.begin(), systemsInUse.end(), *sysIt) != systemsInUse.end())
		{
			sysIt = themeSys.erase(sysIt);
		}
		else
		{
			sysIt++;
		}
	}
	return themeSys;
}

// returns which collection config files exist in the user folder
std::vector<std::string> CollectionSystemManager::getCollectionsFromConfigFolder()
{
	std::vector<std::string> systems;
	fs::path configPath = getCollectionsFolder();

	if (fs::exists(configPath))
	{
		fs::directory_iterator end_itr; // default construction yields past-the-end
		for (fs::directory_iterator itr(configPath); itr != end_itr; ++itr)
		{
			if (fs::is_regular_file(itr->status()))
			{
				// it's a file
				std::string file = itr->path().string();
				std::string filename = file.substr(configPath.string().size());

				// need to confirm filename matches config format
				if (boost::algorithm::ends_with(filename, ".cfg") && boost::algorithm::starts_with(filename, "custom-") && filename != "custom-.cfg")
				{
					filename = filename.substr(7, filename.size()-11);
					LOG(LogError) << "Found config for : " << filename;
					systems.push_back(filename);
				}
				else
				{
					LOG(LogError) << "Found NON-config file: " << filename;
				}
			}
		}
	}
	return systems;
}

// returns the theme folders for Automatic Collections (All, Favorites, Last Played)
std::vector<std::string> CollectionSystemManager::getCollectionThemeFolders(bool custom)
{
	std::vector<std::string> systems;
	for(std::map<std::string, CollectionSystemDecl>::iterator it = mCollectionSystemDeclsIndex.begin() ; it != mCollectionSystemDeclsIndex.end() ; it++ )
	{
		CollectionSystemDecl sysDecl = it->second;
		if (sysDecl.isCustom == custom)
		{
			systems.push_back(sysDecl.themeFolder);
		}
	}
	return systems;
}

// returns whether a specific folder exists in the theme
bool CollectionSystemManager::themeFolderExists(std::string folder)
{
	std::vector<std::string> themeSys = getSystemsFromTheme();
	return std::find(themeSys.begin(), themeSys.end(), folder) != themeSys.end();
}

SystemData* CollectionSystemManager::findCollectionSystem(std::string name)
{
	for(auto sysIt = SystemData::sSystemVector.begin(); sysIt != SystemData::sSystemVector.end(); sysIt++)
	{
		if ((*sysIt)->getName() == name) {
			// found it!
			return (*sysIt);
		}
	}
	return NULL;
}

bool CollectionSystemManager::includeFileInAutoCollections(FileData* file)
{
	// we exclude non-game files from collections (i.e. "kodi", at least)
	// if/when there are more in the future, maybe this can be a more complex method, with a proper list
	// but for now a simple string comparison is more performant
	return file->getName() != "kodi";
}


std::string getCustomCollectionConfigPath(std::string collectionName)
{
	fs::path path = getCollectionsFolder() + "custom-" + collectionName + ".cfg";
	return path.generic_string();
}

std::string getCollectionsFolder()
{
	return getHomePath() + "/.emulationstation/collections/";
}