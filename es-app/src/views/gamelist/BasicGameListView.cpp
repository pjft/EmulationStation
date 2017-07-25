#include "views/gamelist/BasicGameListView.h"
#include "views/ViewController.h"
#include "Renderer.h"
#include "Window.h"
#include "Log.h"
#include "ThemeData.h"
#include "SystemData.h"
#include "Settings.h"
#include "FileFilterIndex.h"

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
		//mHeaderText.setText(files.at(0)->getSystem()->getFullName());

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
	return mList.getSelected();
}

void BasicGameListView::setCursor(FileData* cursor)
{
	LOG(LogError) << "Setting Cursor " << cursor->getName();
	if(!mList.setCursor(cursor) && (!cursor->isPlaceHolder()))
	{
		LOG(LogError) << "Into if statement.";
		LOG(LogError) << "cursor get parent is null? " << (cursor->getParent() == NULL ? "It's NULL!" : "Not NULL");
		LOG(LogError) << "Parent Name: " << cursor->getParent()->getName();
		populateList(cursor->getParent()->getChildrenListToDisplay());
		LOG(LogError) << "After populating list";
		mList.setCursor(cursor);
		LOG(LogError) << "After setting cursor in mList";
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
	LOG(LogError) << "Finished setting cursor";
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
	LOG(LogError) << "Removing game";
	LOG(LogError) << "Name: " << game->getName();
	LOG(LogError) << "Parent? " << (game->getParent() == NULL ? " IS NULL! " : game->getParent()->getName());
	if (deleteFile)
		boost::filesystem::remove(game->getPath());  // actually delete the file on the filesystem
	FileData* parent = game->getParent();
	if (getCursor() == game)                     // Select next element in list, or prev if none
	{
		LOG(LogError) << "Cursor is game";
		std::vector<FileData*> siblings = parent->getChildrenListToDisplay();
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
	LOG(LogError) << "Going to remove from mList";
	mList.remove(game);
	LOG(LogError) << "Removed";
	if(mList.size() == 0)
	{
		LOG(LogError) << "Adding Placeholder";
		addPlaceholder();
	}
	LOG(LogError) << "Going to delete";
	delete game;                                 // remove before repopulating (removes from parent)
	LOG(LogError) << "Deleted game";
	onFileChanged(parent, FILE_REMOVED);           // update the view, with game removed
	LOG(LogError) << "Finished";
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
	if(mRoot->getSystem()->isGameSystem())
	{
		const char* prompt = CollectionSystemManager::get()->getEditingCollection().c_str();
		prompts.push_back(HelpPrompt("y", prompt));
	}
	return prompts;
}
