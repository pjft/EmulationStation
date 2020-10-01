#include "Gamelist.h"

#include <chrono>

#include "utils/FileSystemUtil.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include <pugixml/src/pugixml.hpp>

FileData* findOrCreateFile(SystemData* system, const std::string& path, FileType type)
{
	// first, verify that path is within the system's root folder
	FileData* root = system->getRootFolder();
	bool contains = false;
	std::string relative = Utils::FileSystem::removeCommonPath(path, root->getPath(), contains);

	if(!contains)
	{
		LOG(LogError) << "File path \"" << path << "\" is outside system path \"" << system->getStartPath() << "\"";
		return NULL;
	}

	Utils::FileSystem::stringList pathList = Utils::FileSystem::getPathList(relative);
	auto path_it = pathList.begin();
	FileData* treeNode = root;
	bool found = false;
	while(path_it != pathList.end())
	{
		const std::unordered_map<std::string, FileData*>& children = treeNode->getChildrenByFilename();

		std::string key = *path_it;
		found = children.find(key) != children.cend();
		if (found) {
			treeNode = children.at(key);
		}

		// this is the end
		if(path_it == --pathList.end())
		{
			if(found)
				return treeNode;

			if(type == FOLDER)
			{
				LOG(LogWarning) << "gameList: folder doesn't already exist, won't create";
				return NULL;
			}

			FileData* file = new FileData(type, path, system->getSystemEnvData(), system);

			// skipping arcade assets from gamelist
			if(!file->isArcadeAsset())
			{
				treeNode->addChild(file);
			}
			return file;
		}

		if(!found)
		{
			// don't create folders unless it's leading up to a game
			// if type is a folder it's gonna be empty, so don't bother
			if(type == FOLDER)
			{
				LOG(LogWarning) << "gameList: folder doesn't already exist, won't create";
				return NULL;
			}

			// create missing folder
			FileData* folder = new FileData(FOLDER, Utils::FileSystem::getStem(treeNode->getPath()) + "/" + *path_it, system->getSystemEnvData(), system);
			treeNode->addChild(folder);
			treeNode = folder;
		}

		path_it++;
	}

	return NULL;
}

void parseGamelist(SystemData* system)
{
	bool trustGamelist = Settings::getInstance()->getBool("ParseGamelistOnly");
	std::string xmlpath = system->getGamelistPath(false);

	if(!Utils::FileSystem::exists(xmlpath))
		return;

	LOG(LogInfo) << "Parsing XML file \"" << xmlpath << "\"...";

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(xmlpath.c_str());

	if(!result)
	{
		LOG(LogError) << "Error parsing XML file \"" << xmlpath << "\"!\n	" << result.description();
		return;
	}

	pugi::xml_node root = doc.child("gameList");
	if(!root)
	{
		LOG(LogError) << "Could not find <gameList> node in gamelist \"" << xmlpath << "\"!";
		return;
	}

	std::string relativeTo = system->getStartPath();

	const char* tagList[2] = { "game", "folder" };
	FileType typeList[2] = { GAME, FOLDER };
	for(int i = 0; i < 2; i++)
	{
		const char* tag = tagList[i];
		FileType type = typeList[i];
		for(pugi::xml_node fileNode = root.child(tag); fileNode; fileNode = fileNode.next_sibling(tag))
		{
			const std::string path = Utils::FileSystem::resolveRelativePath(fileNode.child("path").text().get(), relativeTo, false);

			if(!trustGamelist && !Utils::FileSystem::exists(path))
			{
				LOG(LogWarning) << "File \"" << path << "\" does not exist! Ignoring.";
				continue;
			}

			FileData* file = findOrCreateFile(system, path, type);
			if(!file)
			{
				LOG(LogError) << "Error finding/creating FileData for \"" << path << "\", skipping.";
				continue;
			}
			else if(!file->isArcadeAsset())
			{
				std::string defaultName = file->metadata.get("name");
				file->metadata = MetaDataList::createFromXML(GAME_METADATA, fileNode, relativeTo);

				//make sure name gets set if one didn't exist
				if(file->metadata.get("name").empty())
					file->metadata.set("name", defaultName);

				file->metadata.resetChangedFlag();
			}
		}
	}
}

void addFileDataNode(pugi::xml_node& parent, const FileData* file, const char* tag, SystemData* system)
{
	//create game and add to parent node
	pugi::xml_node newNode = parent.append_child(tag);

	//write metadata
	file->metadata.appendToXML(newNode, true, system->getStartPath());

	if(newNode.children().begin() == newNode.child("name") //first element is name
		&& ++newNode.children().begin() == newNode.children().end() //theres only one element
		&& newNode.child("name").text().get() == file->getDisplayName()) //the name is the default
	{
		//if the only info is the default name, don't bother with this node
		//delete it and ultimately do nothing
		parent.remove_child(newNode);
	}else{
		//there's something useful in there so we'll keep the node, add the path

		// try and make the path relative if we can so things still work if we change the rom folder location in the future
		newNode.prepend_child("path").text().set(Utils::FileSystem::createRelativePath(file->getPath(), system->getStartPath(), false).c_str());
	}
}

void updateGamelist(SystemData* system)
{
	//We do this by reading the XML again, adding changes and then writing it back,
	//because there might be information missing in our systemdata which would then miss in the new XML.
	//We have the complete information for every game though, so we can simply remove a game
	//we already have in the system from the XML, and then add it back from its GameData information...

	if(Settings::getInstance()->getBool("IgnoreGamelist"))
		return;

	pugi::xml_document doc;
	pugi::xml_node root;
	std::string xmlReadPath = system->getGamelistPath(false);

	std::string relativeTo = system->getStartPath();

	LOG(LogInfo) << "Starting to update Gamelist";
	if(Utils::FileSystem::exists(xmlReadPath))
	{
		LOG(LogInfo) << "Going to read";
		//parse an existing file first
		pugi::xml_parse_result result = doc.load_file(xmlReadPath.c_str());
		LOG(LogInfo) << "Finish Reading";
		if(!result)
		{
			LOG(LogError) << "Error parsing XML file \"" << xmlReadPath << "\"!\n	" << result.description();
			return;
		}

		root = doc.child("gameList");
		LOG(LogInfo) << "Got gamelist";
		if(!root)
		{
			LOG(LogError) << "Could not find <gameList> node in gamelist \"" << xmlReadPath << "\"!";
			return;
		}
	}else{
		//set up an empty gamelist to append to
		root = doc.append_child("gameList");
	}

	std::vector<FileData*> changedGames;
	std::vector<FileData*> changedFolders;

	/*

	for(auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
	{
		if((*it)->getType() & typeMask)
		{
			if (!displayedOnly || !idx->isFiltered() || idx->showFile(*it))
				out.push_back(*it);
		}

		if((*it)->getChildren().size() > 0)
		{
			std::vector<FileData*> subchildren = (*it)->getFilesRecursive(typeMask, displayedOnly);
			out.insert(out.cend(), subchildren.cbegin(), subchildren.cend());
		}
	}

	*/

	bool newpjt = true;

	LOG(LogInfo) << "Getting root folder";
	//now we have all the information from the XML. now iterate through all our games and add information from there
	FileData* rootFolder = system->getRootFolder();
	if (rootFolder != nullptr)
	{
		int numUpdated = 0;
		LOG(LogInfo) << "Getting files";
		//get only files, no folders
		std::vector<FileData*> files = rootFolder->getFilesRecursive(GAME | FOLDER);
		//iterate through all files, checking if they're already in the XML
		LOG(LogInfo) << "Iterating FileData for system";
		auto startTs = std::chrono::system_clock::now();
			
		auto endTs = std::chrono::system_clock::now();

		for(std::vector<FileData*>::const_iterator fit = files.cbegin(); fit != files.cend(); ++fit)
		{

			// do not touch if it wasn't changed anyway
			if (!(*fit)->metadata.wasChanged()) {
				//LOG(LogInfo) << "Game wasn't changed: " << (*fit)->getName();
				continue;
			}

			const char* tag = ((*fit)->getType() == GAME) ? "game" : "folder";
			
			// adding game to changed list

			
			if ((*fit)->getType() == GAME) {
				changedGames.push_back((*fit));	
			}
			else {
				changedFolders.push_back((*fit));
			}
			
			//// PJT --- CUT FROM HERE
			//// OLD MODEL
			if (!newpjt) {

				LOG(LogInfo) << "Game was changed! " << (*fit)->getName() << " at " << (*fit)->getPath() << " Checking XML now";
				std::string gameSrcPath = (*fit)->getPath();
				startTs = std::chrono::system_clock::now();
				std::string gamePath = Utils::FileSystem::getCanonicalPath((*fit)->getPath());

				endTs = std::chrono::system_clock::now();
				LOG(LogInfo) << "Got gamepath: " << gamePath << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs).count();
				
				// check if the file already exists in the XML
				// if it does, remove it before adding
				for(pugi::xml_node fileNode = root.child(tag); fileNode; fileNode = fileNode.next_sibling(tag))
				{
					startTs = std::chrono::system_clock::now();
					auto vstartTs = std::chrono::system_clock::now();
					
					pugi::xml_node pathNode = fileNode.child("path");


					if(!pathNode)
					{
						LOG(LogError) << "<" << tag << "> node contains no <path> child!";
						continue;
					}



					std::string xmlpath = Utils::FileSystem::resolveRelativePath(pathNode.text().get(), relativeTo, false);
					startTs = endTs;
					endTs = std::chrono::system_clock::now();
					LOG(LogInfo) << "Got xmlpath " << xmlpath << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs).count();

					endTs = std::chrono::system_clock::now();
					


					// pathnode
					std::string pathnode = pathNode.text().get();
					startTs = endTs;
					endTs = std::chrono::system_clock::now();
					LOG(LogInfo) << "Got pathnode " << pathnode << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs).count();

					// systempath
					std::string startpath = system->getStartPath();
					startTs = endTs;
					endTs = std::chrono::system_clock::now();
					LOG(LogInfo) << "Got systempath " << startpath << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs).count();

					// resolverelative
					std::string relativepath = Utils::FileSystem::resolveRelativePath(pathnode, startpath, true);
					
					startTs = endTs;
					endTs = std::chrono::system_clock::now();
					LOG(LogInfo) << "Got relativepath " << relativepath << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs).count();


					// getcanonicalpath


					std::string nodePath = Utils::FileSystem::getCanonicalPath(relativepath);
					startTs = endTs;
					endTs = std::chrono::system_clock::now();
					LOG(LogInfo) << "Got canonical path: " << nodePath << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs).count();
					
					if(nodePath == gamePath)
					{
						LOG(LogInfo) << "Deleting entry from xml";
						// found it
						root.remove_child(fileNode);
						LOG(LogInfo) << "Deleted from xml!";
						LOG(LogInfo) << "XXXXXXXXXXXXXXXXXXXXXXXxXXXXXXXXX. ";	
						LOG(LogInfo) << "XXXXXXXXXXXXXXXXXXXXXXXxXXXXXXXXX. ";	
						LOG(LogInfo) << "XXXXXXXXXXXXXXXXXXXXXXXxXXXXXXXXX. ";	
						LOG(LogInfo) << "XML Path - " << xmlpath;	
						LOG(LogInfo) << "Game source path - " << gameSrcPath;	
						LOG(LogInfo) << "Paths Same? " << (xmlpath == gameSrcPath);	
						LOG(LogInfo) << "XXXXXXXXXXXXXXXXXXXXXXXxXXXXXXXXX. ";	
						LOG(LogInfo) << "XXXXXXXXXXXXXXXXXXXXXXXxXXXXXXXXX. ";	
						LOG(LogInfo) << "XXXXXXXXXXXXXXXXXXXXXXXxXXXXXXXXX. ";	
						
						break;
					}
					startTs = endTs;
					endTs = std::chrono::system_clock::now();
					LOG(LogInfo) << "Not the same. Continue. - total " << std::chrono::duration_cast<std::chrono::milliseconds>(endTs - vstartTs).count();
					;
				}

				// it was either removed or never existed to begin with; either way, we can add it now
				LOG(LogInfo) << "Adding game to XML";
				addFileDataNode(root, *fit, tag, system);
				LOG(LogInfo) << "Added to XML";
				++numUpdated;
			}
		}

		// PJT check changed files
		LOG(LogInfo) << "Changed Files in " << system->getName();
		for(std::vector<FileData*>::const_iterator fit = changedGames.cbegin(); fit != changedGames.cend(); ++fit)
		{
			LOG(LogInfo) << "Game: " << (*fit)->getName() << " at path: " << (*fit)->getPath();
		}
		for(std::vector<FileData*>::const_iterator fit = changedFolders.cbegin(); fit != changedFolders.cend(); ++fit)
		{
			LOG(LogInfo) << "Game: " << (*fit)->getName() << " at path: " << (*fit)->getPath();
		}
		//now write the file

		if (newpjt) // NEW PJT
		{

			const char* tagList[2] = { "game", "folder" };
			FileType typeList[2] = { GAME, FOLDER };
			std::vector<FileData*> changedList[2] = { changedGames, changedFolders };
			
			for(int i = 0; i < 2; i++)
			{
				const char* tag = tagList[i];
				std::vector<FileData*> changes = changedList[i];

				if (changes.size() > 0) {
					// check if the file already exists in the XML
					// if it does, remove it before adding
					for(pugi::xml_node fileNode = root.child(tag); fileNode; fileNode = fileNode.next_sibling(tag))
					{
						startTs = std::chrono::system_clock::now();
						auto vstartTs = std::chrono::system_clock::now();
						
						pugi::xml_node pathNode = fileNode.child("path");


						if(!pathNode)
						{
							LOG(LogError) << "<" << tag << "> node contains no <path> child!";
							continue;
						}

						// apply the same transformation as in Gamelist::parseGamelist
						std::string xmlpath = Utils::FileSystem::resolveRelativePath(pathNode.text().get(), relativeTo, false);
						startTs = endTs;
						endTs = std::chrono::system_clock::now();
						LOG(LogInfo) << "Got xmlpath " << xmlpath << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs).count();

						for(std::vector<FileData*>::const_iterator cfit = changes.cbegin(); cfit != changes.cend(); ++cfit)
						{
							LOG(LogInfo) << "Game: " << (*cfit)->getName() << " at path: " << (*cfit)->getPath();
							if(xmlpath == (*cfit)->getPath())
							{
								LOG(LogInfo) << "Deleting entry from xml: " << xmlpath;
								// found it
								root.remove_child(fileNode);
								//break;
							}
						}
						startTs = endTs;
						endTs = std::chrono::system_clock::now();
						LOG(LogInfo) << "Finish iterating for " << xmlpath << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs).count();

					}
					for(std::vector<FileData*>::const_iterator cfit = changes.cbegin(); cfit != changes.cend(); ++cfit)
					{
						// it was either removed or never existed to begin with; either way, we can add it now
						LOG(LogInfo) << "Adding game to XML";
						addFileDataNode(root, *cfit, tag, system);
						LOG(LogInfo) << "Added to XML";	
						++numUpdated;
					}
				}
			}
		}

		LOG(LogInfo) << "Writing the file";
		if (numUpdated > 0) {
			const auto startTs = std::chrono::system_clock::now();

			//make sure the folders leading up to this path exist (or the write will fail)
			std::string xmlWritePath(system->getGamelistPath(true));
			Utils::FileSystem::createDirectory(Utils::FileSystem::getParent(xmlWritePath));

			LOG(LogInfo) << "Added/Updated " << numUpdated << " entities in '" << xmlReadPath << "'";

			if (!doc.save_file(xmlWritePath.c_str())) {
				LOG(LogError) << "Error saving gamelist.xml to \"" << xmlWritePath << "\" (for system " << system->getName() << ")!";
			}

			const auto endTs = std::chrono::system_clock::now();
			LOG(LogInfo) << "Saved gamelist.xml for system \"" << system->getName() << "\" in " << std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs).count() << " ms";
		}
	}else{
		LOG(LogError) << "Found no root folder for system \"" << system->getName() << "\"!";
	}
}
