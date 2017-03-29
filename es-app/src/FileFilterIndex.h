#pragma once

#include <map>
#include "FileData.h"
#include "Log.h"
#include "FileIndexEntry.h"
#include <boost/math/special_functions/round.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <iostream>

enum FilterIndexType
{
	NONE,
	GENRE_FILTER,
	PLAYER_FILTER,
	PUBDEV_FILTER,
	RATINGS_FILTER
};

struct FilterDataDecl
{
	FilterIndexType type; // type of filter
	std::map<std::string, FileIndexEntry*> indexKeys; // all possible filters for this type
	bool filteredBy; // is it filtered by this type
	std::vector<std::string> currentFilterKeys; // current keys being filtered for
	std::string primaryKey; // primary key in metadata
	bool hasSecondaryKey; // has secondary key for comparison
	std::string secondaryKey; // what's the secondary key
};

class FileFilterIndex
{
public:
	FileFilterIndex();
	~FileFilterIndex();
	void addToIndex(FileData* game);
	bool setFilter(FilterIndexType type, std::vector<std::string>* values);
	bool resetFilter(FilterIndexType type);
	void clearAllFilters();
	void setRootFolder(FileData* rootFolder) { mRootFolder = rootFolder; };
	FileData* getRootFolder() { return mRootFolder; };
	FileData* getFilteredFolder();
	void rebuildIndex();
	void debugPrintIndexes();
	std::string getCurrentFilterValue() { return ""; }; // TO DO
	FilterIndexType getCurrentFilterType() { return NONE; }; // TO DO
	bool showFile(FileData* game);
	bool isFiltered() { return (filterByGenre || filterByPlayers || filterByPubDev || filterByRatings); };
	bool isKeyBeingFilteredBy(std::string key, FilterIndexType type);
	std::map<std::string, FileIndexEntry*>* getGenreAllIndexedKeys() { return &genreIndexAllKeys; };
	std::vector<std::string>* getGenreFilteredKeys() { return &genreIndexFilteredKeys; };

private:
	std::string getIndexableKey(FileData* game, FilterIndexType type, bool getSecondary);
	void addGenreEntryToIndex(FileData* game);
	void addPlayerEntryToIndex(FileData* game);
	void addPubDevEntryToIndex(FileData* game);
	void addRatingsEntryToIndex(FileData* game);
	void removeEntryFromIndex(FileData* game, FilterIndexType type);
	void clearIndex(std::map<std::string, FileIndexEntry*> indexMap);

	bool filterByGenre;
	bool filterByPlayers;
	bool filterByPubDev;
	bool filterByRatings;

	std::map<std::string, FileIndexEntry*> genreIndexAllKeys;
	std::map<std::string, FileIndexEntry*> playersIndexAllKeys;
	std::map<std::string, FileIndexEntry*> pubDevIndexAllKeys;
	std::map<std::string, FileIndexEntry*> ratingsIndexAllKeys;

	std::vector<std::string> genreIndexFilteredKeys;
	std::vector<std::string> playersIndexFilteredKeys;
	std::vector<std::string> pubDevIndexFilteredKeys;
	std::vector<std::string> ratingsIndexFilteredKeys;

	FileData* mRootFolder;

};