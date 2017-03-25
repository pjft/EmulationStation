#pragma once

#include "GuiComponent.h"
#include "SystemData.h"
#include "components/MenuComponent.h"


template<typename T>
class OptionListComponent;

class SwitchComponent;

//The starting point for a multi-game scrape.
//Allows the user to set various parameters (to set filters, to set which systems to scrape, to enable manual mode).
//Generates a list of "searches" that will be carried out by GuiScraperLog.
class GuiGamelistFilter : public GuiComponent
{
public:
	GuiGamelistFilter(Window* window);

	bool input(InputConfig* config, Input input) override;

	virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
	void pressedStart();
	void start();

	std::shared_ptr< OptionListComponent<GameFilterFunc> > mFilters;
	std::shared_ptr< OptionListComponent<SystemData*> > mSystems;
	std::shared_ptr<SwitchComponent> mApproveResults;

	MenuComponent mMenu;
};
