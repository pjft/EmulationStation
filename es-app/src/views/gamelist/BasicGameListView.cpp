#include "views/gamelist/BasicGameListView.h"
#include "views/ViewController.h"
#include "Renderer.h"
#include "Window.h"
#include "ThemeData.h"
#include "SystemData.h"
#include "Settings.h"
#include "FileFilterIndex.h"

BasicGameListView::BasicGameListView(Window* window, FileData* root)
	: ISimpleGameListView(window, root), mList(window)
{
	mList.setSize(mSize.x(), mSize.y() * 0.8f);
	mList.setPosition(0, mSize.y() * 0.2f);
	addChild(&mList);

	populateList(root->getChildren());
}

void BasicGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	ISimpleGameListView::onThemeChanged(theme);
	using namespace ThemeFlags;
	mList.applyTheme(theme, getName(), "gamelist", ALL);
}

// pjft - this is where we'll change the UI!
void BasicGameListView::onFileChanged(FileData* file, FileChangeType change)
{
	if(change == FILE_METADATA_CHANGED)
	{
		// might switch to a detailed view
		ViewController::get()->reloadGameListView(this);
		return;
	}

	ISimpleGameListView::onFileChanged(file, change);
}

void BasicGameListView::populateList(const std::vector<FileData*>& files)
{
	mList.clear();
	mHeaderText.setText(files.at(0)->getSystem()->getFullName());
	float startTime = SDL_GetTicks();

	LOG(LogInfo) << "Populating List for " << files.at(0)->getSystem()->getFullName();
	// this looks ugly, but I believe may be more performant
	FileFilterIndex* idx = this->mRoot->getSystem()->getIndex();
	if (idx->isFiltered()) {
		for(auto it = files.begin(); it != files.end(); it++)
		{
			// pjft - we can try to filter on render, rather than creating and managing fake FileData
			if (idx->showFile((*it))) {
				mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
			}
		}
	}
	else 
	{
		for(auto it = files.begin(); it != files.end(); it++)
		{
			mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
		}
	}
	LOG(LogInfo) << "Populating List took " << (SDL_GetTicks()-startTime)/1000 << "secs.";
	// pjft - need to check if list is empty, and if so add a placeholder node
	if (mList.size() == 0) 
	{
		// add a placeholder
		// for now, just create a new FileData
		FileData* placeholder = new FileData(FOLDER, "", files.at(0)->getSystem());
		mList.add("<No Results Found for Current Filter Criteria>", placeholder, true);
	} 
}

FileData* BasicGameListView::getCursor()
{
	return mList.getSelected();
}

void BasicGameListView::resetCursor() {
	mList.resetCursor();
}

void BasicGameListView::setCursor(FileData* cursor)
{
	if(!mList.setCursor(cursor))
	{
		populateList(cursor->getParent()->getChildren());
		mList.setCursor(cursor);

		// update our cursor stack in case our cursor just got set to some folder we weren't in before
		if(mCursorStack.empty() || mCursorStack.top() != cursor->getParent())
		{
			std::stack<FileData*> tmp;
			FileData* ptr = cursor->getParent();
			while(ptr && ptr != mRoot)
			{
				tmp.push(ptr);
				ptr = ptr->getParent();
			}
			
			// flip the stack and put it in mCursorStack
			mCursorStack = std::stack<FileData*>();
			while(!tmp.empty())
			{
				mCursorStack.push(tmp.top());
				tmp.pop();
			}
		}
	}
}

void BasicGameListView::launch(FileData* game)
{
	ViewController::get()->launch(game);
}

void BasicGameListView::remove(FileData *game)
{
	boost::filesystem::remove(game->getPath());  // actually delete the file on the filesystem
	if (getCursor() == game)                     // Select next element in list, or prev if none
	{
		std::vector<FileData*> siblings = game->getParent()->getChildren();
		auto gameIter = std::find(siblings.begin(), siblings.end(), game);
		auto gamePos = std::distance(siblings.begin(), gameIter);
		if (gameIter != siblings.end())
		{
			if ((gamePos + 1) < siblings.size())
			{
				setCursor(siblings.at(gamePos + 1));
			} else if ((gamePos - 1) > 0) {
				setCursor(siblings.at(gamePos - 1));
			}
		}
	}
	delete game;                                 // remove before repopulating (removes from parent)
	onFileChanged(game, FILE_REMOVED);           // update the view, with game removed
}

std::vector<HelpPrompt> BasicGameListView::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;

	if(Settings::getInstance()->getBool("QuickSystemSelect"))
		prompts.push_back(HelpPrompt("left/right", "system"));
	prompts.push_back(HelpPrompt("up/down", "choose"));
	prompts.push_back(HelpPrompt("a", "launch"));
	prompts.push_back(HelpPrompt("b", "back"));
	prompts.push_back(HelpPrompt("select", "options"));
	return prompts;
}
