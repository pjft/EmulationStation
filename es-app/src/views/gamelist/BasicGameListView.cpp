#include "views/gamelist/BasicGameListView.h"

#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Settings.h"
#include "SystemData.h"
#include "Log.h"
#include <boost/filesystem/operations.hpp>

BasicGameListView::BasicGameListView(Window* window, FileData* root)
	: ISimpleGameListView(window, root), mList(window)
{
	mList.setSize(mSize.x(), mSize.y() * 0.8f);
	mList.setPosition(0, mSize.y() * 0.2f);
	mList.setDefaultZIndex(20);
	addChild(&mList);

	populateList(root->getChildrenListToDisplay());
}

void BasicGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	ISimpleGameListView::onThemeChanged(theme);
	using namespace ThemeFlags;
	mList.applyTheme(theme, getName(), "gamelist", ALL);

	sortChildren();
}

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
	mHeaderText.setText(mRoot->getSystem()->getFullName());
	if (files.size() > 0)
	{
		for(auto it = files.begin(); it != files.end(); it++)
		{
			mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
		}
	}
	else
	{
		addPlaceholder();
	}
}

FileData* BasicGameListView::getCursor()
{
	//LOG(LogError) << "Getting Cursor";
	//LOG(LogError) << "Is NULL? " << (mList.getSelected() == NULL);
	//LOG(LogError) << "Selected Name: " << mList.getSelectedName();
	return mList.getSelected();
}

void BasicGameListView::setCursor(FileData* cursor)
{
	LOG(LogError) << "Setting cursor.";
	LOG(LogError) << "Cursor Name: " << cursor->getName();
	if(!mList.setCursor(cursor) && (!cursor->isPlaceHolder()))
	{
		LOG(LogError) << "Inside IF.";
		LOG(LogError) << "Parent: " << cursor->getParent()->getName();
		populateList(cursor->getParent()->getChildrenListToDisplay());
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
	LOG(LogError) << "FINISHED Setting cursor.";
}

void BasicGameListView::addPlaceholder()
{
	// empty list - add a placeholder
	FileData* placeholder = new FileData(PLACEHOLDER, "<No Entries Found>", this->mRoot->getSystem()->getSystemEnvData(), this->mRoot->getSystem());
	mList.add(placeholder->getName(), placeholder, (placeholder->getType() == PLACEHOLDER));
}

void BasicGameListView::launch(FileData* game)
{
	ViewController::get()->launch(game);
}

void BasicGameListView::remove(FileData *game, bool deleteFile)
{
	if (deleteFile)
		boost::filesystem::remove(game->getPath());  // actually delete the file on the filesystem
	LOG(LogError) << "Deleting file? " << deleteFile;
	FileData* parent = game->getParent();
	LOG(LogError) << "Parent: " << parent->getName();
	LOG(LogError) << "Cursor: " << getCursor()->getName() << " -- " << getCursor()->getPath();
	LOG(LogError) << "Game: " << game->getName() << " -- " << game->getPath();
	LOG(LogError) << "Game == cursor? " << (getCursor() == game);
	if (getCursor() == game)                     // Select next element in list, or prev if none
	{
		LOG(LogError) << "We're deleting the cursor";
		std::vector<FileData*> siblings = parent->getChildrenListToDisplay();
		auto gameIter = std::find(siblings.begin(), siblings.end(), game);
		unsigned int gamePos = std::distance(siblings.begin(), gameIter);
		if (gameIter != siblings.end())
		{
			if ((gamePos + 1) < siblings.size())
			{
				setCursor(siblings.at(gamePos + 1));
			} else if ((gamePos - 1) > 0) {
				setCursor(siblings.at(gamePos - 1));
			}
		}
		LOG(LogError) << "New cursor: " << getCursor()->getName();
	}
	mList.remove(game);
	if(mList.size() == 0)
	{
		addPlaceholder();
	}
	delete game;                                 // remove before repopulating (removes from parent)
	LOG(LogError) << "Updating parent";
	onFileChanged(parent, FILE_REMOVED);           // update the view, with game removed
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
	prompts.push_back(HelpPrompt("x", "random"));
	if(mRoot->getSystem()->isGameSystem() && !ViewController::get()->isUIModeKid())
	{
		std::string prompt = CollectionSystemManager::get()->getEditingCollection();
		prompts.push_back(HelpPrompt("y", prompt));
	}
	return prompts;
}
