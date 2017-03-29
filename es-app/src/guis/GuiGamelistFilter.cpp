#include "guis/GuiGamelistFilter.h"
#include "guis/GuiMsgBox.h"
#include "views/ViewController.h"

#include "components/TextComponent.h"
#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"

GuiGamelistFilter::GuiGamelistFilter(Window* window, SystemData* system) : GuiComponent(window),
	mMenu(window, "SCRAPE NOW"), mSystem(system)
{
	addChild(&mMenu);

	// get filters from system
	FileFilterIndex* filterIndex = system->getIndex();
	genreIndexAllKeys = filterIndex->getGenreAllIndexedKeys();
	genreIndexFilteredKeys = filterIndex->getGenreFilteredKeys();

	// add filters (with first one selected

	// add genres
	mGenres = std::make_shared< OptionListComponent<std::string> >(mWindow, "FILTER BY GENRES", true);
	for(auto it: *genreIndexAllKeys)
	{
		mGenres->add(it.first, it.first, filterIndex->isKeyBeingFilteredBy(it.first, GENRE_FILTER));
	}
	mMenu.addWithLabel("GENRES", mGenres);

	/*mApproveResults = std::make_shared<SwitchComponent>(mWindow);
	mApproveResults->setState(true);
	mMenu.addWithLabel("INCLUDE UNKNOWN ENTRIES", mApproveResults);*/

	mMenu.addButton("START", "start", std::bind(&GuiGamelistFilter::pressedStart, this));
	mMenu.addButton("BACK", "back", [&] { delete this; });

	mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

void GuiGamelistFilter::pressedStart()
{
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
