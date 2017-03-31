#include "FileFilterIndex.h"

FileFilterIndex::FileFilterIndex() 
	: filterByGenre(false), filterByPlayers(false), filterByPubDev(false), filterByRatings(false)
{
	FilterDataDecl filterDecls[] = {
		//type 				//allKeys 				//filteredBy 		//filteredKeys 				//primaryKey 	//hasSecondaryKey 	//secondaryKey 	//menuLabel
		{ GENRE_FILTER, 	&genreIndexAllKeys, 	&filterByGenre,		&genreIndexFilteredKeys, 	"genre",		true,				"genre",		"GENRE"	},
		{ PLAYER_FILTER, 	&playersIndexAllKeys, 	&filterByPlayers,	&playersIndexFilteredKeys, 	"players",		false,				"",				"PLAYERS"	},
		{ PUBDEV_FILTER, 	&pubDevIndexAllKeys, 	&filterByPubDev,	&pubDevIndexFilteredKeys, 	"developer",	true,				"publisher",	"PUBLISHER / DEVELOPER"	},
		{ RATINGS_FILTER, 	&ratingsIndexAllKeys, 	&filterByRatings,	&ratingsIndexFilteredKeys, 	"rating",		false,				"",				"RATING"	}
	};

	filterDataDecl = std::vector<FilterDataDecl>(filterDecls, filterDecls + sizeof(filterDecls) / sizeof(filterDecls[0]));
}

FileFilterIndex::~FileFilterIndex()
{
	clearIndex(genreIndexAllKeys);
	clearIndex(playersIndexAllKeys);
	clearIndex(pubDevIndexAllKeys);
	clearIndex(ratingsIndexAllKeys);

}

std::vector<FilterDataDecl>& FileFilterIndex::getFilterDataDecls() 
{
	return filterDataDecl;
}

std::string FileFilterIndex::getIndexableKey(FileData* game, FilterIndexType type, bool getSecondary)
{
	std::string key = "";
	switch(type) 
	{
		case GENRE_FILTER:
		{
			key = strToUpper(game->metadata.get("genre"));
			
			if (getSecondary && !key.empty()) {
				std::istringstream f(key);
			    std::string newKey;    
			    getline(f, newKey, '/');
		    	if (!newKey.empty() && newKey != key) 
		    	{
		    		key = newKey;
		    	}
		    	else
		    	{
		    		key = std::string();
		    	}
		    }
			break;
		}
		case PLAYER_FILTER:
		{
			if (getSecondary)
				break;
			
			key = game->metadata.get("players");
			break;
			// this is likely deprecated - let's test first
			/*int players = 0;
			if (!playersString.empty()) {
				try {
					players = std::stoi(playersString);
				} 
				catch (int e) 
				{
					LOG(LogError) << "Error parsing Players (invalid value, expected integer): " << playersString;
				}
			}
			return std::string("" + players);*/
		}
		case PUBDEV_FILTER:
		{
			if (getSecondary)
				key = strToUpper(game->metadata.get("developer"));
			else
				key = strToUpper(game->metadata.get("publisher"));
			break;
		}
		case RATINGS_FILTER:
		{
			int ratingNumber = 0;
			if (!getSecondary) 
			{	
				std::string ratingString = game->metadata.get("rating");
				if (!ratingString.empty()) {
					try {
						ratingNumber = boost::math::iround(std::stod(ratingString)*5);
						if (ratingNumber < 0)
							ratingNumber = 0;

						key = std::to_string(ratingNumber);
					} 
					catch (int e) 
					{
						LOG(LogError) << "Error parsing Rating (invalid value, expected decimal): " << ratingString;
					}	
				}
			}
			break;
		}
	}
	boost::trim(key);
	if (key.empty() || (type == RATINGS_FILTER && key == "0")) {
		key = "UNKNOWN";
	}
	return key;
}

void FileFilterIndex::addToIndex(FileData* game)
{
	manageGenreEntryInIndex(game);
	managePlayerEntryInIndex(game);
	managePubDevEntryInIndex(game);
	manageRatingsEntryInIndex(game);
}

void FileFilterIndex::removeFromIndex(FileData* game)
{
	manageGenreEntryInIndex(game, true);
	managePlayerEntryInIndex(game, true);
	managePubDevEntryInIndex(game, true);
	manageRatingsEntryInIndex(game, true);
}

bool FileFilterIndex::setFilter(FilterIndexType type, std::vector<std::string>* values) 
{
	// test if it exists before setting
	switch(type) 
	{
		case NONE:
			clearAllFilters();
			break;
		case GENRE_FILTER:
			filterByGenre = values->size() > 0;
			genreIndexFilteredKeys.clear();
			for (std::vector<std::string>::iterator it = values->begin(); it != values->end(); ++it ) {
		        // check if exists
		        if (genreIndexAllKeys.find(*it) != genreIndexAllKeys.end()) {
		        	genreIndexFilteredKeys.push_back(std::string(*it));
		        }
		    }	
		    // debug
		    for (auto x: genreIndexFilteredKeys) {
			    LOG(LogInfo) << "Genre Index Keys: " << x;
			}
			break;
		case PLAYER_FILTER:
			filterByPlayers = values->size() > 0;
			playersIndexFilteredKeys.clear();
			for (std::vector<std::string>::iterator it = values->begin(); it != values->end(); ++it ) {
		        // check if exists
		        if (playersIndexAllKeys.find(*it) != playersIndexAllKeys.end()) {
		        	playersIndexFilteredKeys.push_back(std::string(*it));
		        }
		    }	
			break;
		case PUBDEV_FILTER:
			filterByPubDev = values->size() > 0;
			pubDevIndexFilteredKeys.clear();
			for (std::vector<std::string>::iterator it = values->begin(); it != values->end(); ++it ) {
		        // check if exists
		        if (pubDevIndexAllKeys.find(*it) != pubDevIndexAllKeys.end()) {
		        	pubDevIndexFilteredKeys.push_back(std::string(*it));
		        }
		    }	
			break;
		case RATINGS_FILTER:
			filterByRatings = values->size() > 0;
			ratingsIndexFilteredKeys.clear();
			for (std::vector<std::string>::iterator it = values->begin(); it != values->end(); ++it ) {
		        // check if exists
		        if (ratingsIndexAllKeys.find(*it) != ratingsIndexAllKeys.end()) {
		        	ratingsIndexFilteredKeys.push_back(std::string(*it));
		        }
		    }	
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
	filterByGenre = false;
	genreIndexFilteredKeys.clear();
	filterByPlayers = false;
	playersIndexFilteredKeys.clear();
	filterByPubDev = false;
	pubDevIndexFilteredKeys.clear();
	filterByRatings = false;
	ratingsIndexFilteredKeys.clear();
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
	for (auto x: playersIndexAllKeys) {
	    LOG(LogInfo) << "Multiplayer Index: " << x.first << ": " << x.second->getCount();
	}
	for (auto x: genreIndexAllKeys) {
	    LOG(LogInfo) << "Genre Index: " << x.first << ": " << x.second->getCount();
	}
	for (auto x: ratingsIndexAllKeys) {
	    LOG(LogInfo) << "Ratings Index: " << x.first << ": " << x.second->getCount();
	}
	for (auto x: pubDevIndexAllKeys) {
	    LOG(LogInfo) << "PubDev Index: " << x.first << ": " << x.second->getCount();
	}
}

bool resetFilter(FilterIndexType type) {

}

bool FileFilterIndex::showFile(FileData* game)
{
	LOG(LogInfo) << "Is Filtered? " << (isFiltered() ? "TRUE" : "FALSE");
	// this shouldn't happen, but just in case let's get it out of the way
	if (!isFiltered())
		return true;

	// if folder, needs further inspection - i.e. see if folder contains at least one element
	// that should be shown
	if (game->getType() == FOLDER) {
		bool found = false;
		// iterate through all of the children, until there's a match
		// TO DO
		return false;
	}

	// sort these by expense of processing - handle 
	const bool filteredByList[4] = { filterByPlayers, filterByRatings, filterByGenre, filterByPubDev };
	const bool secondaryTagList[4] = { false, false, true, true };
	const FilterIndexType filterTypes[4] = { PLAYER_FILTER, RATINGS_FILTER, GENRE_FILTER, PUBDEV_FILTER };
	std::vector<std::string> filterKeysList[4] = { playersIndexFilteredKeys, ratingsIndexFilteredKeys, genreIndexFilteredKeys, pubDevIndexFilteredKeys };
	
	bool keepGoing = false;

	for(int i = 0; i < 4; i++) {
		if (filteredByList[i]) {
			// try to find a match
			std::string key = getIndexableKey(game, filterTypes[i], false);
			LOG(LogInfo) << "Checking for Key: " << key;
			for (std::vector<std::string>::iterator it = filterKeysList[i].begin(); it != filterKeysList[i].end(); ++it ) {
				LOG(LogInfo) << "Compare with: " << (*it);
		    	if (key == (*it))
				{
					// ok
					keepGoing = true;
					break;
				}   
		    }
		    // if we didn't find a match, try for secondary keys - i.e. publisher and dev, or first genre
		    if (!keepGoing) {
		    	if (!secondaryTagList[i]) 
		    	{
		    		return false;
		    	}
		    	key = getIndexableKey(game, filterTypes[i], true);
				for (std::vector<std::string>::iterator it = filterKeysList[i].begin(); it != filterKeysList[i].end(); ++it ) {
			    	if (key == (*it))
					{
						// ok
						keepGoing = true;
						break;
					}   
			    }
		    }
		    // if still nothing, then it's not a match
		    if (!keepGoing)
				return false;
		}
	}
	
	return keepGoing;
}

bool FileFilterIndex::isKeyBeingFilteredBy(std::string key, FilterIndexType type) {
	const FilterIndexType filterTypes[4] = { PLAYER_FILTER, RATINGS_FILTER, GENRE_FILTER, PUBDEV_FILTER };
	std::vector<std::string> filterKeysList[4] = { playersIndexFilteredKeys, ratingsIndexFilteredKeys, genreIndexFilteredKeys, pubDevIndexFilteredKeys };

	for (int i = 0; i < 4; i++) {
		if (filterTypes[i] == type) {
			for (std::vector<std::string>::iterator it = filterKeysList[i].begin(); it != filterKeysList[i].end(); ++it ) {
		    	if (key == (*it))
				{
					// ok
					return true;
				}
		    }
		    return false;
		}

	}

	return false;

}

void FileFilterIndex::manageGenreEntryInIndex(FileData* game, bool remove)
{

	std::string key = getIndexableKey(game, GENRE_FILTER, false);

	// flag for including unknowns
	bool includeUnknown = false;

	// only add unknown in pubdev IF both dev and pub are empty
	if (!includeUnknown && (key == "UNKNOWN" || key == "BIOS")) {
		// no valid player info found
		return;
	} 

	manageIndexEntry(&genreIndexAllKeys, key, remove);

	key = getIndexableKey(game, GENRE_FILTER, true);
	if (!key.empty())
	{
		manageIndexEntry(&genreIndexAllKeys, key, remove);
	}
}

void FileFilterIndex::managePlayerEntryInIndex(FileData* game, bool remove)
{
	
	// flag for including unknowns
	bool includeUnknown = false;
	std::string key = getIndexableKey(game, PLAYER_FILTER, false);

	// only add unknown in pubdev IF both dev and pub are empty
	if (!includeUnknown && key == "UNKNOWN") {
		// no valid player info found
			return;
	} 

	//key = (key == "1" ? "SINGLE PLAYER" : "MULTIPLAYER");
	
	manageIndexEntry(&playersIndexAllKeys, key, remove);
}

void FileFilterIndex::managePubDevEntryInIndex(FileData* game, bool remove)
{
	std::string pub = getIndexableKey(game, PUBDEV_FILTER, false);
	std::string dev = getIndexableKey(game, PUBDEV_FILTER, true);

	// flag for including unknowns
	bool includeUnknown = false;
	bool unknownPub = false;
	bool unknownDev = false;

	if (pub == "UNKNOWN") {
		unknownPub = true;
	}
	if (dev == "UNKNOWN") {
		unknownDev = true;
	}

	if (!includeUnknown && unknownDev && unknownPub) {
		// no valid rating info found
		return;
	} 

	if (unknownDev && unknownPub) {
		// if no info at all
		manageIndexEntry(&pubDevIndexAllKeys, pub, remove);
	}
	else 
	{
		if (!unknownDev) {
			// if no info at all
			manageIndexEntry(&pubDevIndexAllKeys, dev, remove);
		}
		if (!unknownPub) {
			// if no info at all
			manageIndexEntry(&pubDevIndexAllKeys, pub, remove);
		}
	}
}

void FileFilterIndex::manageRatingsEntryInIndex(FileData* game, bool remove)
{
	std::string key = getIndexableKey(game, RATINGS_FILTER, false);

	// flag for including unknowns
	bool includeUnknown = false;

	if (!includeUnknown && key == "UNKNOWN") {
		// no valid rating info found
		return;
	} 

	manageIndexEntry(&ratingsIndexAllKeys, key, remove);
}

void FileFilterIndex::manageIndexEntry(std::map<std::string, FileIndexEntry*>* index, std::string key, bool remove) {
	if (remove) {
		// removing entry
		LOG(LogInfo) << "Removing Entry from Index: " << key;
		// adding entry
		if (index->find(key) == index->end())
		{
			// this shouldn't happen
			LOG(LogError) << "Couldn't find entry in index! " << key;
		} 
		else
		{
			index->at(key)->removeEntry(NULL);
			LOG(LogError) << "Remaining count of entries: " << index->at(key)->getCount();
			if(index->at(key)->getCount() <= 0) {
				index->erase(key);
				LOG(LogInfo) << "Validating removal of key: " << key << " - " << (index->find(key) == index->end() ? "Removed" : "Still exists");
			}
		}
	} 
	else 
	{
		//LOG(LogInfo) << "Inserting Entry in Index: " << key;
		// adding entry
		if (index->find(key) == index->end())
		{
			(*index)[key] = new FileIndexEntry();
		} 
		else
		{
			index->at(key)->addEntry(NULL);
		}
	}	
}

void FileFilterIndex::clearIndex(std::map<std::string, FileIndexEntry*> indexMap)
{
	for (std::map<std::string, FileIndexEntry*>::iterator it = indexMap.begin(); it != indexMap.end(); ++it ) {
        delete it->second;
    }
    indexMap.clear();
}