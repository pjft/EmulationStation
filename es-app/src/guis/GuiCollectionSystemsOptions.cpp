#include "guis/GuiCollectionSystemsOptions.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiTextEditPopup.h"
#include "Settings.h"
#include "views/ViewController.h"
#include "guis/GuiSettings.h"
#include "Log.h"
#include "Util.h"
#include "components/TextComponent.h"
#include "components/OptionListComponent.h"

GuiCollectionSystemsOptions::GuiCollectionSystemsOptions(Window* window) : GuiComponent(window), mMenu(window, "GAME COLLECTION SETTINGS")
{
	initializeMenu();
}

void GuiCollectionSystemsOptions::initializeMenu()
{
	addChild(&mMenu);

	// get collections

	addSystemsToMenu();

	// add "Create New Custom Collection from Theme"

	std::vector<std::string> unusedFolders = CollectionSystemManager::get()->getUnusedSystemsFromTheme();
	if (unusedFolders.size() > 0)
	{
		addEntry("CREATE NEW CUSTOM COLLECTION FROM THEME", 0x777777FF, true,
		[this, unusedFolders] {
			auto s = new GuiSettings(mWindow, "SELECT THEME FOLDER");
			std::shared_ptr< OptionListComponent<std::string> > folderThemes = std::make_shared< OptionListComponent<std::string> >(mWindow, "SELECT THEME FOLDER", true);

			// add Custom Systems
			for(auto it = unusedFolders.begin() ; it != unusedFolders.end() ; it++ )
			{
				ComponentListRow row;
				std::string name = *it;
				/*std::function<void()> createCollection = [name, this, s] {
					LOG(LogError) << "Create new Collection: " << name;
					SystemData* newSys = CollectionSystemManager::get()->addNewCustomCollection(name);
					customOptionList->add(name, name, true);
					std::string outAuto = vectorToCommaString(autoOptionList->getSelectedObjects());
					std::string outCustom = vectorToCommaString(customOptionList->getSelectedObjects());
					updateSettings(outAuto, outCustom);

					ViewController::get()->goToSystemView(newSys);

					Window* window = mWindow;
					CollectionSystemManager::get()->setEditMode(name);
					while(window->peekGui() && window->peekGui() != ViewController::get())
						delete window->peekGui();
					return;
				};*/
				std::function<void()> createCollectionCall = [name, this, s] {
					createCollection(name);
				};
				row.makeAcceptInputHandler(createCollectionCall);

				auto themeFolder = std::make_shared<TextComponent>(mWindow, strToUpper(name), Font::get(FONT_SIZE_SMALL), 0x777777FF);
				row.addElement(themeFolder, true);
				s->addRow(row);
			}
			mWindow->pushGui(s);
		});
	}

	ComponentListRow row;
	row.addElement(std::make_shared<TextComponent>(mWindow, "CREATE NEW CUSTOM COLLECTION", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
	auto createCustomCollection = [this](const std::string& newVal) {
		std::string name = newVal;
		// we need to store the first Gui and remove it, as it'll be deleted by the actual Gui
		Window* window = mWindow;
		GuiComponent* topGui = window->peekGui();
		window->removeGui(topGui);
		createCollection(name);
	};
	row.makeAcceptInputHandler([this, createCustomCollection] {
		mWindow->pushGui(new GuiTextEditPopup(mWindow, "New Collection Name", "", createCustomCollection, false));
	});

	mMenu.addRow(row);

	sortAllSystemsSwitch = std::make_shared<SwitchComponent>(mWindow);
	sortAllSystemsSwitch->setState(Settings::getInstance()->getBool("SortAllSystems"));
	mMenu.addWithLabel("SORT CUSTOM SYSTEMS AND COLLECTIONS", sortAllSystemsSwitch);

	// add "Create New Custom Collection"

	mMenu.addButton("BACK", "back", std::bind(&GuiCollectionSystemsOptions::applySettings, this));

	mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

void GuiCollectionSystemsOptions::addEntry(const char* name, unsigned int color, bool add_arrow, const std::function<void()>& func)
{
	std::shared_ptr<Font> font = Font::get(FONT_SIZE_MEDIUM);

	// populate the list
	ComponentListRow row;
	row.addElement(std::make_shared<TextComponent>(mWindow, name, font, color), true);

	if(add_arrow)
	{
		std::shared_ptr<ImageComponent> bracket = makeArrow(mWindow);
		row.addElement(bracket, false);
	}

	row.makeAcceptInputHandler(func);

	mMenu.addRow(row);
}

void GuiCollectionSystemsOptions::createCollection(std::string inName) {
	std::string name = CollectionSystemManager::get()->getValidNewCollectionName(inName);
	LOG(LogError) << "Create new Collection: " << name;
	SystemData* newSys = CollectionSystemManager::get()->addNewCustomCollection(name);
	customOptionList->add(name, name, true);
	std::string outAuto = vectorToCommaString(autoOptionList->getSelectedObjects());
	std::string outCustom = vectorToCommaString(customOptionList->getSelectedObjects());
	updateSettings(outAuto, outCustom);
	LOG(LogError) << "Going to New Collection View: " << name;
	ViewController::get()->goToSystemView(newSys);

	Window* window = mWindow;
	LOG(LogError) << "Setting Edit Mode: " << name;
	CollectionSystemManager::get()->setEditMode(name);
	while(window->peekGui() && window->peekGui() != ViewController::get())
		delete window->peekGui();
	LOG(LogError) << "Finished!";
	return;
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
	std::string outAuto = vectorToCommaString(autoOptionList->getSelectedObjects());
	std::string prevAuto = Settings::getInstance()->getString("CollectionSystemsAuto");
	std::string outCustom = vectorToCommaString(customOptionList->getSelectedObjects());
	std::string prevCustom = Settings::getInstance()->getString("CollectionSystemsCustom");
	bool outSort = sortAllSystemsSwitch->getState();
	bool prevSort = Settings::getInstance()->getBool("SortAllSystems");
	bool needUpdateSettings = prevAuto != outAuto || prevCustom != outCustom || outSort != prevSort;
	if ((outAuto != "" && !CollectionSystemManager::get()->isThemeGenericCollectionCompatible(false)) ||
		(outCustom != "" && outCustom != prevCustom && !CollectionSystemManager::get()->isThemeCustomCollectionCompatible(customOptionList->getSelectedObjects())))
	{
		mWindow->pushGui(new GuiMsgBox(mWindow,
			"Your theme does not support game collections. Please update your theme, or ensure that you use a theme that contains the folders:\n\n• auto-favorites\n• auto-lastplayed\n• auto-allgames\n• custom-collections\n\nDo you still want to enable collections?",
				"YES", [this, needUpdateSettings, outAuto, outCustom] {
					if (needUpdateSettings)
					{
						updateSettings(outAuto, outCustom);
					}
					delete this; },
				"NO", [this] { delete this; }));
	}
	else
	{
		if (needUpdateSettings)
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
	Settings::getInstance()->setBool("SortAllSystems", sortAllSystemsSwitch->getState());
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
