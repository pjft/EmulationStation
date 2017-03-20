#include "views/gamelist/IGameListView.h"
#include "Window.h"
#include "guis/GuiMetaDataEd.h"
#include "guis/GuiMenu.h"
#include "guis/GuiGamelistOptions.h"
#include "views/ViewController.h"
#include "Settings.h"
#include "Log.h"
#include "Sound.h"

// pjft - This is where I'll change the filtering as well
bool IGameListView::input(InputConfig* config, Input input)
{
	// select to open GuiGamelistOptions
	if(config->isMappedTo("select", input) && input.value)
	{
		Sound::getFromTheme(mTheme, getName(), "menuOpen")->play();
		mWindow->pushGui(new GuiGamelistOptions(mWindow, this->mRoot->getSystem()));
		return true;

	// Ctrl-R to reload a view when debugging
	}else if(Settings::getInstance()->getBool("Debug") && config->getDeviceId() == DEVICE_KEYBOARD && 
		(SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL)) && input.id == SDLK_r && input.value != 0)
	{
		LOG(LogDebug) << "reloading view";
		ViewController::get()->reloadGameListView(this, true);
		return true;
	}
	// pjft test filter
	if(config->isMappedTo("y", input) && input.value) {
		FileFilterIndex* idx = this->mRoot->getSystem()->getIndex();
		std::vector<std::string> filters ("ACTION", "ADVENTURE", "BREAKOUT", "CLIMBING");
		idx->setFilter(GENRE_FILTER, filters);
	}

	return GuiComponent::input(config, input);
}

void IGameListView::setTheme(const std::shared_ptr<ThemeData>& theme)
{
	mTheme = theme;
	onThemeChanged(theme);
}

HelpStyle IGameListView::getHelpStyle()
{
	HelpStyle style;
	style.applyTheme(mTheme, getName());
	return style;
}
