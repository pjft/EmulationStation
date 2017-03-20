#include "FileFilterIndex.h"

FileFilterIndex::FileFilterIndex() 
	: filterByGenre(false), filterByPlayers(false), filterByPubDev(false), filterByRatings(false)
{

}

FileFilterIndex::~FileFilterIndex()
{
	clearIndex(genreIndex);
	clearIndex(multiplayerIndex);
	clearIndex(pubDevIndex);
	clearIndex(ratingsIndex);
}

void FileFilterIndex::addToIndex(FileData* game)
{
	// add to indexes, fully capitalized
	std::string genre = strToUpper(game->metadata.get("genre"));

	std::string developer = strToUpper(game->metadata.get("developer"));
	std::string publisher = strToUpper(game->metadata.get("publisher"));

	int players = 0;
	if (game->metadata.get("players") != "") {
		try {
			players = std::stoi(game->metadata.get("players"));
		} 
		catch (int e) 
		{
			LOG(LogError) << "Error parsing Players (invalid value, expected integer): " << game->metadata.get("players");
		}
	}

	int rating = 0;
	if (game->metadata.get("rating") != "") {
		try {
			boost::math::iround(std::stod(game->metadata.get("rating")));
		} 
		catch (int e) 
		{
			LOG(LogError) << "Error parsing Rating (invalid value, expected decimal): " << game->metadata.get("rating");
		}	
	}

	addGenreEntryToIndex(game, genre);
	addPlayerEntryToIndex(game, players);
	addPubDevEntryToIndex(game, publisher, developer);
	addRatingsEntryToIndex(game, rating);
}

bool FileFilterIndex::setFilter(FilterIndexType type, const std::vector<std::string> values) 
{
	// test if it exists before setting
	switch(type) 
	{
		case NONE:
			clearAllFilters();
			break;
		case GENRE_FILTER:
			filterByGenre = true;
			filterByGenreKeys.clear();
			for (std::vector<std::string>::iterator it = values.begin(); it != values.end(); ++it ) {
		        // check if exists
		        if (genreIndex.find(it) != genreIndex.end()) {
		        	filterByGenreKeys.push_back(std::string(it));
		        }
		    }	
		    // debug
		    for (auto& x: filterByGenreKeys) {
			    LOG(LogInfo) << "Genre Index Keys: " << x;
			}
			break;
		case PLAYER_FILTER:
			filterByPlayers = true;
			filterByPlayerKeys.clear();
			break;
		case PUBDEV_FILTER:
			filterByPubDev = true;
			filterByPubDevKeys.clear();
			break;
		case RATINGS_FILTER:
			filterByRatings = true;
			filterByRatingsKeys.clear();
			break;
	}

	return true;
}

FileData* FileFilterIndex::getFilteredFolder()
{
	// check if filter value exists in category, fully capitalized

	// set filter type and value

	// log time
	// iterate folder and filter

	// log end time
	// return list
}

void FileFilterIndex::clearAllFilters() 
{
	curFilterType = NONE;
	curFilterValue = "";
}

void FileFilterIndex::rebuildIndex()
{
	// log time
	// iterate entire list and rebuild indexes

	// log end time
}

void FileFilterIndex::debugPrintIndexes() 
{
	LOG(LogInfo) << "Printing Indexes...";
	for (auto& x: multiplayerIndex) {
	    LOG(LogInfo) << "Multiplayer Index: " << x.first << ": " << x.second->getCount();
	}
	for (auto& x: genreIndex) {
	    LOG(LogInfo) << "Genre Index: " << x.first << ": " << x.second->getCount();
	}
}

bool resetFilter(FilterIndexType type) {

}

bool FileFilterIndex::showFile(FileData* game)
{
	// check if file matches filter

	// if folder, needs further inspection - i.e. see if folder contains at least one element
	// that should be shown

	// wonder about starts with performance, for MAME
	/*
		if (s.compare(0, t.length(), t) == 0)
		{
			// ok
		}
	*/

}

void FileFilterIndex::addGenreEntryToIndex(FileData* game, std::string key)
{
	// flag for including unknowns
	bool includeUnknown = false;
	
	boost::trim(key);

	if (key == "") {
		key = "UNKNOWN";
	}

	// only add unknown in pubdev IF both dev and pub are empty
	if (!includeUnknown && (key == "UNKNOWN" || key == "BIOS")) {
		// no valid player info found
			return;
	} 

	if (genreIndex.find(key) == genreIndex.end())
	{
		genreIndex[key] = new FileIndexEntry();
	} 
	else
	{
		genreIndex.at(key)->addEntry(NULL);
	}

	// separate add for MAME categories with "/"
	
    std::istringstream f(key);
    std::string newKey = "";    
    getline(f, newKey, '/');
	boost::trim(newKey);
    if (newKey != "" && newKey != key) {
		if (genreIndex.find(newKey) == genreIndex.end())
		{
			genreIndex[newKey] = new FileIndexEntry();
		} 
		else
		{
			genreIndex.at(newKey)->addEntry(NULL);
		}
    }
}

void FileFilterIndex::addPlayerEntryToIndex(FileData* game, int players)
{
	// flag for including unknowns
	bool includeUnknown = false;
	
	// only add unknown in pubdev IF both dev and pub are empty
	if (!includeUnknown && players == 0) {
		// no valid player info found
			return;
	} 

	std::string key = "";
	if (players == 0) {
		key = "UNKNOWN";
	}
	else 
	{
		key = (players == 1 ? "SINGLE PLAYER" : "MULTIPLAYER");
	}

	if (multiplayerIndex.find(key) == multiplayerIndex.end())
	{
		multiplayerIndex[key] = new FileIndexEntry();
	} 
	else
	{
		multiplayerIndex.at(key)->addEntry(NULL);
	}	
}

void FileFilterIndex::addPubDevEntryToIndex(FileData* game, std::string pubKey, std::string devKey)
{
	// flag for including unknowns
	bool includeUnknown = false;
	
	// only add unknown in pubdev IF both dev and pub are empty
	if (includeUnknown) {

	}
}

void FileFilterIndex::addRatingsEntryToIndex(FileData* game, int rating)
{
	// flag for including unknowns
	bool includeUnknown = false;
	
	// only add unknown in pubdev IF both dev and pub are empty
	if (includeUnknown) {

	}
}

void FileFilterIndex::removeEntryFromIndex(FileData* game, FilterIndexType type)
{

}

void FileFilterIndex::clearIndex(std::map<std::string, FileIndexEntry*> indexMap)
{
	for (std::map<std::string, FileIndexEntry*>::iterator it = indexMap.begin(); it != indexMap.end(); ++it ) {
        delete it->second;
    }
    indexMap.clear();
}