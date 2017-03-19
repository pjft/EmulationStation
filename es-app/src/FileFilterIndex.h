#include <map>
#include "FileData.h"
#include "Log.h"

enum FilterType
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
	void index(FileData game);
	FileData* setFilter(FileData* rootFolder, FilterType type, std::string value);
	void rebuildIndexFromFolder(FileData* rootFolder);
	void debugPrintIndexes();
	std::string getCurrentFilterValue();
	FilterType getCurrentFilterType();

private:
	std::map<std::string, int> genreIndex;
	std::map<std::string, int> multiplayerIndex;
	std::map<std::string, int> pubDevIndex;
	std::map<std::string, int> ratingsIndex;

	FilterType curFilterType;
	std::string curFilterValue;

};