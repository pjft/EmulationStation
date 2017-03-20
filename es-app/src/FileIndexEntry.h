#pragma once

#include <map>
#include "FileData.h"
#include "Log.h"
#include "Util.h"

class FileIndexEntry {
public:
	FileIndexEntry();
	~FileIndexEntry();
	int getCount() { return counter; };
	void resetEntry();
	void addEntry(FileData* game);
	void removeEntry(FileData* game);
private:
	int counter;
	FileData* filteredFolder;
};