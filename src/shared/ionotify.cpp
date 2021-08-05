#include <iostream>
#include <vector>
#include <map>

#include <efsw/efsw.hpp>
#include <efsw/System.hpp>
#include <efsw/FileSystem.hpp>

extern "C" {
	#include "../shared.h"
}

#define C_FUNC extern "C"

/**
 * Before you comment on incosiderate design, I initially thought that this lib creates watches on a per file basis,
 * as opposed to an per-directory basis. That means that I had to hack together a solution that will allow multiple files
 * to access their individual ionotify functions whilst being able to be located within the same directory as files with
 * differing ionotify functions. Please excuse the messy implementation.
 *
 * Signed: nikp123 (11:17 CEST Aug 05, 2021)
 **/

class UpdateListener;

typedef void(*xava_ionotify_func)(struct XAVA_HANDLE*, int id, XAVA_IONOTIFY_EVENT);

struct xava_ionotify {
	efsw::FileWatcher *fw;
	UpdateListener    *ul;
};

struct xava_ionotify_watch {
	int id;
	xava_ionotify_func func;
	std::string filepath;
	std::string directory;
};

struct xava_ionotify_watch_updatelistener {
	// searchable by filepaths (because dumb implementation)
	std::map<std::string, int> ids;

	// search by id because faster
	std::map<int, xava_ionotify_func> funcs;

	std::string directory;
}; 

class UpdateListener : public efsw::FileWatchListener
{
public:
	UpdateListener(struct XAVA_HANDLE *a) {
		xava = a;
	}

	void handleFileAction( efsw::WatchID watchid, const std::string& dir, 
		const std::string& filename, efsw::Action action, std::string oldFilename = "" ) {

		std::string fullpath = dir + filename;

		bool exists = watches[watchid].ids.find(fullpath) != watches[watchid].ids.end();
		if(!exists) {
			xavaSpam("Triggered on non-existant file - %s (Ignore this)",
					fullpath.c_str());
			return;
		}

		int id = watches[watchid].ids[fullpath];
		xava_ionotify_func func = watches[watchid].funcs[id];

		switch (action) {
			case efsw::Actions::Add:
			case efsw::Actions::Modified:
				func(xava, id, XAVA_IONOTIFY_CHANGED);
				break;
			case efsw::Actions::Delete:
			case efsw::Actions::Moved:
				func(xava, id, XAVA_IONOTIFY_DELETED);
				break;
			default:
				func(xava, id, XAVA_IONOTIFY_ERROR);
				break;
		}
	}

	efsw::WatchID getExistingID(std::string directory) {
		return ids[directory];
	}

	void addWatch(efsw::WatchID id, struct xava_ionotify_watch watch) {
		// these two IDs are completely different, DONT MIX 'EM UP

		// im too lazy to make a proper unique ID generator
		if(watches[id].funcs.find(watch.id) != watches[id].funcs.end()) 
			xavaBail("The dev is to blame for this mistake. This will crash XAVA!");

		watches[id].ids[watch.filepath] = watch.id;
		watches[id].funcs[watch.id]     = watch.func;
		ids[watch.directory]            = id; 
	}

	void removeWatch(efsw::WatchID id) {
		watches[id].ids.clear();
		watches[id].funcs.clear();
		ids.erase(watches[id].directory);
		watches.erase(id);
	} 

	// destructor
	~UpdateListener() {
		watches.clear(); // hopefully (hopefully) C++ is smart enough to handle this on its own
		ids.clear();
	}

private:
	struct XAVA_HANDLE *xava;
	std::map<std::string, efsw::WatchID> ids;
	std::map<efsw::WatchID, struct xava_ionotify_watch_updatelistener> watches;
};

C_FUNC EXP_FUNC XAVAIONOTIFY xavaIONotifySetup(struct XAVA_HANDLE *xava) {
	struct xava_ionotify *handle = new struct xava_ionotify;

	handle->ul = new UpdateListener(xava);
	handle->fw = new efsw::FileWatcher(0);

	// set some fun defaults
	handle->fw->followSymlinks(true);
	handle->fw->allowOutOfScopeLinks(false);

	return handle;
}

// returns the ID of the current watch process
C_FUNC EXP_FUNC XAVAIONOTIFYWATCH xavaIONotifyAddWatch(XAVAIONOTIFYWATCHSETUP setup) {
	struct xava_ionotify       *hand    = setup->ionotify;
	efsw::WatchID              *watchID = new efsw::WatchID; 

	std::string filepath = setup->filename;
	std::string directory;
	std::string filename;

	size_t filepath_len = filepath.length();
	for(int i=filepath_len-1; i>=0; i--) {
		if(setup->filename[i] == DIRBRK) {
			directory = filepath.substr(0, i);
			filename  = filepath.substr(i+1, filepath_len-i-1);
			break;
		}
	}

	*watchID = hand->fw->addWatch(directory, hand->ul, false); // last parameter is recursive
	switch(*watchID) {
		case efsw::Errors::FileNotFound:
			xavaError("%s - File not found", setup->filename);
			return nullptr;
		case efsw::Errors::FileRepeated: {
			xavaLog("%s - File repeated", setup->filename);

			*watchID = hand->ul->getExistingID(directory);
			break;
		}
		case efsw::Errors::FileOutOfScope:
			xavaError("%s - File out of scope", setup->filename);
			return nullptr;
		case efsw::Errors::FileRemote:
			xavaError("%s - File is remote", setup->filename);
			return nullptr;
		case efsw::Errors::Unspecified:
			xavaError("%s - Unknown error", setup->filename);
			return nullptr;
		default:
			xavaLog("Successfully created IONotifyWatch - %s", setup->filename);
			break;
	}

	struct xava_ionotify_watch watch;
	watch.func      = setup->xava_ionotify_func;
	watch.id        = setup->id;
	watch.filepath  = filepath;
	watch.directory = directory;

	hand->ul->addWatch(*watchID, watch);

	hand->fw->watch();

	return watchID;
}

C_FUNC EXP_FUNC void xavaIONotifyEndWatch(XAVAIONOTIFY handle, XAVAIONOTIFYWATCH watch) {
	// honestly i don't feel like adding a billion lines of code to detect this, let's just
	// avoid seg. faults instead
	if(watch == nullptr) {
		xavaSpam("Probably found one of those repeated ones, working around seg. fault");
		return;
	}

	efsw::WatchID *id = static_cast<efsw::WatchID*>(watch);
	handle->ul->removeWatch(*id);
	handle->fw->removeWatch(*id);
	delete id;
}

C_FUNC EXP_FUNC void xavaIONotifyKill(const XAVAIONOTIFY ionotify) {
	delete ionotify->fw;
	delete ionotify->ul;
	delete ionotify;
}

