#include "FileIndexEntry.h"

FileIndexEntry::FileIndexEntry()
	: counter(1), filteredFolder(NULL)
{

}

FileIndexEntry::~FileIndexEntry()
{
	delete filteredFolder;
}

void FileIndexEntry::resetEntry()
{

}

void FileIndexEntry::addEntry(FileData* game)
{
	counter++;
}

void FileIndexEntry::removeEntry(FileData* game)
{

}