#include "FileFilterIndex.h"


FileFilterIndex::FileFilterIndex() 
	: curFilterType(NONE), curFilterValue("")
{

}

FileFilterIndex::~FileFilterIndex()
{
	genreIndex.clear();
	multiplayerIndex.clear();
	pubDevIndex.clear();
	ratingsIndex.clear();
}

void FileFilterIndex::index(FileData game)
{
	// add to indexes, fully capitalized
}

FileData* FileFilterIndex::setFilter(FileData* rootFolder, FilterType type, std::string value)
{
	// check if filter value exists in category, fully capitalized

	// set filter type and value

	// log time
	// iterate folder and filter

	// log end time
	// return list
}


void FileFilterIndex::rebuildIndexFromFolder(FileData* rootFolder)
{
	// log time
	// iterate entire list and rebuild indexes

	// log end time
}

void FileFilterIndex::debugPrintIndexes() 
{
	LOG(LogInfo) << "Printing Indexes...";
}