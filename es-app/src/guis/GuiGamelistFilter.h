#pragma once

#include "GuiComponent.h"
#include "SystemData.h"
#include "components/MenuComponent.h"
#include "FileFilterIndex.h"
#include "Log.h"


template<typename T>
class OptionListComponent;

class SwitchComponent;

//The starting point for a multi-game scrape.
//Allows the user to set various parameters (to set filters, to set which systems to scrape, to enable manual mode).
//Generates a list of "searches" that will be carried out by GuiScraperLog.
class GuiGamelistFilter : public GuiComponent
{
public:
	GuiGamelistFilter(Window* window, SystemData* system);
	~GuiGamelistFilter();
	bool input(InputConfig* config, Input input) override;

	virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
	void pressedStart();
	void start();
	void resetAllFilters() { mFilterIndex->clearAllFilters(); };
	void addFiltersToMenu();
	void debugPrint();

	std::map<FilterIndexType, std::shared_ptr< OptionListComponent<std::string> >> mFilterOptions;

	// define the 4 filter types
	/*std::shared_ptr< OptionListComponent<std::string> > mGenres;
	std::shared_ptr< OptionListComponent<std::string> > mPlayers;
	std::shared_ptr< OptionListComponent<std::string> > mPubDev;
	std::shared_ptr< OptionListComponent<std::string> > mRatings;

	std::map<std::string, FileIndexEntry*>* genreIndexAllKeys;
	std::map<std::string, FileIndexEntry*>* multiplayerIndexAllKeys;
	std::map<std::string, FileIndexEntry*>* pubDevIndexAllKeys;
	std::map<std::string, FileIndexEntry*>* ratingsIndexAllKeys;

	std::vector<std::string>* genreIndexFilteredKeys;
	std::vector<std::string>* playerIndexFilteredKeys;
	std::vector<std::string>* pubDevIndexFilteredKeys;
	std::vector<std::string>* ratingsIndexFilteredKeys;*/

	MenuComponent mMenu;
	SystemData* mSystem;
	FileFilterIndex* mFilterIndex;
};
