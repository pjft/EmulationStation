#include "guis/GuiCollectionSystemsOptions.h"
#include "guis/GuiMsgBox.h"
#include "Settings.h"
#include "views/ViewController.h"

#include "components/TextComponent.h"
#include "components/OptionListComponent.h"

GuiCollectionSystemsOptions::GuiCollectionSystemsOptions(Window* window) : GuiComponent(window), mMenu(window, "GAME COLLECTION SETTINGS")
{
	initializeMenu();
}

void GuiCollectionSystemsOptions::initializeMenu()
{
	addChild(&mMenu);

	// get virtual systems

	addSystemsToMenu();

	mMenu.addButton("BACK", "back", std::bind(&GuiCollectionSystemsOptions::applySettings, this));

	mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

GuiCollectionSystemsOptions::~GuiCollectionSystemsOptions()
{
	//mSystemOptions.clear();
}

void GuiCollectionSystemsOptions::addSystemsToMenu()
{

	std::map<std::string, CollectionSystemData> autoSystems = CollectionSystemManager::get()->getAutoCollectionSystems();

	autoOptionList = std::make_shared< OptionListComponent<std::string> >(mWindow, "SELECT COLLECTIONS", true);

	// add Auto Systems
	for(std::map<std::string, CollectionSystemData>::iterator it = autoSystems.begin() ; it != autoSystems.end() ; it++ )
	{
		autoOptionList->add(it->second.decl.longName, it->second.decl.name, it->second.isEnabled);
	}
	mMenu.addWithLabel("AUTOMATIC GAME COLLECTIONS", autoOptionList);

	std::map<std::string, CollectionSystemData> customSystems = CollectionSystemManager::get()->getCustomCollectionSystems();

	customOptionList = std::make_shared< OptionListComponent<std::string> >(mWindow, "SELECT COLLECTIONS", true);

	// add Custom Systems
	for(std::map<std::string, CollectionSystemData>::iterator it = customSystems.begin() ; it != customSystems.end() ; it++ )
	{
		customOptionList->add(it->second.decl.longName, it->second.decl.name, it->second.isEnabled);
	}
	mMenu.addWithLabel("CUSTOM GAME COLLECTIONS", customOptionList);
}

void GuiCollectionSystemsOptions::applySettings()
{
	std::string outAuto = commaStringToVector(autoOptionList->getSelectedObjects());
	std::string prevAuto = Settings::getInstance()->getString("CollectionSystemsAuto");
	std::string outCustom = commaStringToVector(customOptionList->getSelectedObjects());
	std::string prevCustom = Settings::getInstance()->getString("CollectionSystemsCustom");
	if ((outAuto != "" && !CollectionSystemManager::get()->isThemeCollectionCompatible(false)) ||
		(outCustom != "" && !CollectionSystemManager::get()->isThemeCollectionCompatible(true)))
	{
		mWindow->pushGui(new GuiMsgBox(mWindow,
			"Your theme does not support game collections. Please update your theme, or ensure that you use a theme that contains the folders:\n\n• auto-favorites\n• auto-lastplayed\n• auto-allgames\n• custom-collections\n\nDo you still want to enable collections?",
				"YES", [this, outAuto, prevAuto, outCustom, prevCustom] {
					if (prevAuto != outAuto || prevCustom != outCustom)
					{
						updateSettings(outAuto, outCustom);
					}
					delete this; },
				"NO", [this] { delete this; }));
	}
	else
	{
		if (prevAuto != outAuto || prevCustom != outCustom)
		{
			updateSettings(outAuto, outCustom);
		}
		delete this;
	}
}

void GuiCollectionSystemsOptions::updateSettings(std::string newAutoSettings, std::string newCustomSettings)
{
	Settings::getInstance()->setString("CollectionSystemsAuto", newAutoSettings);
	Settings::getInstance()->setString("CollectionSystemsCustom", newCustomSettings);
	Settings::getInstance()->saveFile();
	CollectionSystemManager::get()->loadEnabledListFromSettings();
	CollectionSystemManager::get()->updateSystemsList();
	ViewController::get()->goToStart();
	ViewController::get()->reloadAll();
}

bool GuiCollectionSystemsOptions::input(InputConfig* config, Input input)
{
	bool consumed = GuiComponent::input(config, input);
	if(consumed)
		return true;

	if(config->isMappedTo("b", input) && input.value != 0)
	{
		applySettings();
	}


	return false;
}

std::vector<HelpPrompt> GuiCollectionSystemsOptions::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("b", "back"));
	return prompts;
}
