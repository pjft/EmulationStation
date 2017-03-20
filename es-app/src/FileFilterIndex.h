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

class FileFilterIndex
{
public:
	FileFilterIndex();
	~FileFilterIndex();
	void addToIndex(FileData* game);
	bool setFilter(FilterIndexType type, const std::vector<std::string> values);
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
private:
	void addGenreEntryToIndex(FileData* game, std::string key);
	void addPlayerEntryToIndex(FileData* game, int players);
	void addPubDevEntryToIndex(FileData* game, std::string pubKey, std::string devKey);
	void addRatingsEntryToIndex(FileData* game, int rating);
	void removeEntryFromIndex(FileData* game, FilterIndexType type);
	void clearIndex(std::map<std::string, FileIndexEntry*> indexMap);

	std::map<std::string, FileIndexEntry*> genreIndex;
	std::map<std::string, FileIndexEntry*> multiplayerIndex;
	std::map<std::string, FileIndexEntry*> pubDevIndex;
	std::map<std::string, FileIndexEntry*> ratingsIndex;

	bool filterByGenre;
	bool filterByPlayers;
	bool filterByPubDev;
	bool filterByRatings;

	std::vector<std::string> filterByGenreKeys;
	std::vector<std::string> filterByPlayersKeys;
	std::vector<std::string> filterByPubDevKeys;
	std::vector<std::string> filterByRatingsKeys;

	FileData* mRootFolder;

};