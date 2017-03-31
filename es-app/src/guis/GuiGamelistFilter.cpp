#include "guis/GuiGamelistFilter.h"
#include "guis/GuiMsgBox.h"
#include "views/ViewController.h"

#include "components/TextComponent.h"
#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"

GuiGamelistFilter::GuiGamelistFilter(Window* window, SystemData* system) : GuiComponent(window),
	mMenu(window, "FILTER GAMELIST BY"), mSystem(system)
{
	addChild(&mMenu);

	// get filters from system	

	mFilterIndex = system->getIndex();

	ComponentListRow row;

	// show filtered menu
	row.elements.clear();
	row.addElement(std::make_shared<TextComponent>(mWindow, "RESET ALL FILTERS", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
	row.makeAcceptInputHandler(std::bind(&GuiGamelistFilter::resetAllFilters, this));
	mMenu.addRow(row);
	row.elements.clear();

	addFiltersToMenu();

	/*mApproveResults = std::make_shared<SwitchComponent>(mWindow);
	mApproveResults->setState(true);
	mMenu.addWithLabel("INCLUDE UNKNOWN ENTRIES", mApproveResults);*/

	mMenu.addButton("APPLY", "start", std::bind(&GuiGamelistFilter::pressedStart, this));
	mMenu.addButton("BACK", "back", [&] { delete this; });

	// resize + center
	mMenu.setSize(Renderer::getScreenWidth() * 0.5f, Renderer::getScreenHeight() * 0.82f);
	mMenu.setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2);

	//mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

void GuiGamelistFilter::debugPrint()
{
	// get decls
	std::vector<FilterDataDecl> decls = mFilterIndex->getFilterDataDecls();
	for (std::vector<FilterDataDecl>::iterator it = decls.begin(); it != decls.end(); ++it ) {
    	if ((*it).type == GENRE_FILTER)
		{
			LOG(LogInfo) << "Menu Label: " << (*it).menuLabel;
			LOG(LogInfo) << "Filtered by? " << ((*((*it).filteredByRef)) ? "TRUE" : "FALSE");			
			LOG(LogInfo) << "All Index Key size: " << (*it).allIndexKeys->size(); // all possible filters for this type
			LOG(LogInfo) << "Current Filtered Key Size: " << (*it).currentFilteredKeys->size(); // current keys being filtered for
			LOG(LogInfo) << "Primary Key: " << (*it).primaryKey; // primary key in metadata
			LOG(LogInfo) << "Has Secondary Key? " << ((*it).hasSecondaryKey ? "TRUE" : "FALSE"); // has secondary key for comparison
			LOG(LogInfo) << "Secondary Key: " << (*it).secondaryKey; // what's the secondary key
			// ok
			return;
		}
    }

	LOG(LogInfo) << "Couldn't find GENRE filter";
}

GuiGamelistFilter::~GuiGamelistFilter()
{
    mFilterOptions.clear();
}

void GuiGamelistFilter::addFiltersToMenu() 
{
	std::vector<FilterDataDecl> decls = mFilterIndex->getFilterDataDecls();
	for (std::vector<FilterDataDecl>::iterator it = decls.begin(); it != decls.end(); ++it ) {

		FilterIndexType type = (*it).type; // type of filter
		std::map<std::string, FileIndexEntry*>* allKeys = (*it).allIndexKeys; // all possible filters for this type
		std::vector<std::string>* allFilteredKeys = (*it).currentFilteredKeys; // current keys being filtered for
		std::string menuLabel = (*it).menuLabel; // text to show in menu
		std::shared_ptr< OptionListComponent<std::string> > optionList;

		
		// add filters (with first one selected)
		ComponentListRow row;	

		// add genres
		// Should have a "select all/remove all" option, somehow	
		optionList = std::make_shared< OptionListComponent<std::string> >(mWindow, "FILTER BY " + menuLabel, true);
		for(auto it: *allKeys)
		{
			optionList->add(it.first, it.first, mFilterIndex->isKeyBeingFilteredBy(it.first, type));
		}
		if (allKeys->size() > 0)
			mMenu.addWithLabel(menuLabel, optionList);

		mFilterOptions[type] = optionList;	
	}
}

void GuiGamelistFilter::pressedStart()
{
	LOG(LogInfo) << "Filter Options Size: " << mFilterOptions.size();
	std::vector<FilterDataDecl> decls = mFilterIndex->getFilterDataDecls();
	for (std::map<FilterIndexType, std::shared_ptr< OptionListComponent<std::string> >>::iterator it = mFilterOptions.begin(); it != mFilterOptions.end(); ++it ) {
		std::shared_ptr< OptionListComponent<std::string> > optionList = it->second;
		std::vector<std::string> filters = optionList->getSelectedObjects();
		mFilterIndex->setFilter(it->first, &filters);
	}
	//ViewController::get()->reloadGameListView(mSystem);
	delete this;
	/*std::vector<SystemData*> sys = mGenres->getSelectedObjects();
	for(auto it = sys.begin(); it != sys.end(); it++)
	{
		if((*it)->getPlatformIds().empty())
		{
			mWindow->pushGui(new GuiMsgBox(mWindow, 
				strToUpper("Warning: some of your selected systems do not have a platform set. Results may be even more inaccurate than usual!\nContinue anyway?"), 
				"YES", std::bind(&GuiGamelistFilter::start, this), 
				"NO", nullptr));
			return;
		}
	}

	start();*/
}

void GuiGamelistFilter::start()
{
	/*std::queue<ScraperSearchParams> searches = getSearches(mSystems->getSelectedObjects(), mFilters->getSelected());

	if(searches.empty())
	{
		mWindow->pushGui(new GuiMsgBox(mWindow,
			"NO GAMES FIT THAT CRITERIA."));
	}else{
		// do filtering
		delete this;
	}*/
}

bool GuiGamelistFilter::input(InputConfig* config, Input input)
{
	bool consumed = GuiComponent::input(config, input);
	if(consumed)
		return true;
	
	if(input.value != 0 && config->isMappedTo("b", input))
	{
		delete this;
		return true;
	}

	if(config->isMappedTo("start", input) && input.value != 0)
	{
		// close everything
		Window* window = mWindow;
		while(window->peekGui() && window->peekGui() != ViewController::get())
			delete window->peekGui();
	}


	return false;
}

std::vector<HelpPrompt> GuiGamelistFilter::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("b", "back"));
	prompts.push_back(HelpPrompt("start", "close"));
	return prompts;
}
