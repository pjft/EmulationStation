#pragma once

#include "GuiComponent.h"
#include "SystemData.h"
#include "components/MenuComponent.h"
#include "CollectionSystemManager.h"
#include "Log.h"


template<typename T>
class OptionListComponent;


class GuiCollectionSystemsOptions : public GuiComponent
{
public:
	GuiCollectionSystemsOptions(Window* window);
	~GuiCollectionSystemsOptions();
	bool input(InputConfig* config, Input input) override;

	virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
	void initializeMenu();
	void applySettings();
	void addSystemsToMenu();
	void updateSettings(std::string newAutoSettings, std::string newCustomSettings);
	std::shared_ptr< OptionListComponent<std::string> > autoOptionList;
	std::shared_ptr< OptionListComponent<std::string> > customOptionList;
	MenuComponent mMenu;
	SystemData* mSystem;
};
