#include "kPlatform.hpp"
#include "../kversion/kVersion.h"
#include <vector>
#include <time.h>
#include <set>
#include <algorithm>
#ifdef __linux__
#include <iconv.h>
#include <dirent.h>
#include <sys/sendfile.h>  // sendfile
#include <fcntl.h>         // open
#else
#include <tchar.h>
#include <psapi.h>
#endif

bool kFileExist(const std::wstring &name)
{
	bool success = false;
#ifndef __linux__
	WIN32_FIND_DATAW searchResult;
	HANDLE fh = FindFirstFileW(name.c_str(), &searchResult);
	success = (fh != INVALID_HANDLE_VALUE);
	FindClose(fh);
#else
	std::string sname = kToUtf8(name);
	char *nameChar = sname.data();
	struct stat status;
	success = (stat(nameChar, &status) == 0);
	success = success || (lstat(nameChar, &status) == 0);
#endif
	return success;
}

bool kFileDelete(const std::wstring &name)
{
#ifndef __linux__
	return DeleteFileW(name.c_str()) == TRUE;
#else
	std::string sname = kToUtf8(name);
    char *nameChar = sname.data();
	return (remove(nameChar) == 0);
#endif
}

bool kFileCopy(const wchar_t *src, const wchar_t *dest, bool overwrite)
{
#ifdef __linux__
    std::string _src = kToUtf8(src);
    std::string _dest = kToUtf8(dest);
    char command[MAX_PATH];
    if (overwrite) {
        sprintf(command, "cp %s %s", _src.data(), _dest.data());
    } else {
        sprintf(command, "cp -n %s %s", _src.data(), _dest.data());
    }
    return (system(command) != -1);
#else
    int failIfExists = 1;
    if (overwrite) failIfExists = 0;
    int result = CopyFileW(src, dest, failIfExists);
    // return value is non-zero for success
    bool success = (result != 0);
    if (failIfExists && !success) {
        success = true;
    }
    return success;
#endif
}

bool kFileCopyAll(const std::wstring &dest_dir, const std::wstring &fullPattern, wchar_t *drive, wchar_t *dir, bool overwrite, bool withFolder)
{
	bool success = true;
#ifdef __linux__
	/*
	copy file
	http://stackoverflow.com/questions/10195343/copy-a-file-in-a-sane-safe-and-efficient-way
	LINUX - WAY // requires kernel >= 2.6.33
	*/
	std::string dest_folder = kToUtf8(dest_dir);
	std::vector<std::wstring> files = kFindSomeFiles(fullPattern, L"", 0);
	for (int i = 0; i < files.size(); i++) {
        std::string full_file_name = kToUtf8(files[i]);
        int source = open(full_file_name.data(), O_RDONLY, 0);

		std::string destination_file = dest_folder + "/" + full_file_name;
        int dest = open(destination_file.data(), O_WRONLY | O_CREAT /*| O_TRUNC/**/, 0644);

		// struct required, rationale: function stat() exists also
		struct stat stat_source;
		fstat(source, &stat_source);

		success = success && (sendfile(dest, source, 0, stat_source.st_size) != -1);
		close(source);
		close(dest);
	}
#else
	WIN32_FIND_DATAW searchResult;
	HANDLE fh = FindFirstFileW(fullPattern.c_str(), &searchResult);
	if (fh != INVALID_HANDLE_VALUE)
	{
		if ((searchResult.cFileName[0] != '.' && withFolder) ||
			(searchResult.cFileName[0] != '.' && !(int(searchResult.dwFileAttributes) == int(FILE_ATTRIBUTE_DIRECTORY) + int(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)))) {
			std::wstring file;
			file += drive;
			file += kNativeSeparatorW();
			file += dir;
			file += kNativeSeparatorW();
			file += searchResult.cFileName;
			success = success && CopyFileW(file.c_str(), (dest_dir + searchResult.cFileName).c_str(), !overwrite);
		}
		while (FindNextFileW(fh, &searchResult)) {
			if (searchResult.cFileName[0] != '.' && withFolder ||
				(searchResult.cFileName[0] != '.' && !(int(searchResult.dwFileAttributes) == int(FILE_ATTRIBUTE_DIRECTORY) + int(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)))) {
				std::wstring file;
				file += drive;
				file += kNativeSeparatorW();
				file += dir;
				file += kNativeSeparatorW();
				file += searchResult.cFileName;
				success = success && CopyFileW(file.c_str(), (dest_dir + searchResult.cFileName).c_str(), !overwrite);
			}
		}
	}
	FindClose(fh);
#endif
	return success;
}

#ifdef __linux__
bool is_directory(const std::string &fullname, bool &error)
{
    error = false;
    struct stat st;
    if (stat(fullname.c_str(), &st) == -1) {
        error = true;
        return false;
    }
    return ((st.st_mode & S_IFDIR) != 0);
}
#endif

std::vector<std::wstring> kFindSomeFiles(const std::wstring &filename1, const std::wstring &filename2, int maxNumber)
{
	std::vector<std::wstring> result;
	int filesFound = 0;
	std::set<std::wstring> set;
#ifdef __linux__
	/*
	find all files in folder
	http://stackoverflow.com/questions/306533/how-do-i-get-a-list-of-files-in-a-directory-in-c
	*/

	DIR *dir;
    dirent *ent;
    std::string folder = kToUtf8(filename1);
	dir = opendir(folder.c_str());
    std::wstring filePattern;
    if (dir == 0) {
        wchar_t drive[MAX_PATH], directory[MAX_PATH], file[MAX_PATH], ext[MAX_PATH];
        _ksplitpathcomponents(filename2.c_str(), drive, directory, file, ext);
        filePattern = std::wstring(ext);
        folder = kToUtf8(directory);
        dir = opendir(folder.c_str());
    }
    if (dir) {
        while ((ent = readdir(dir)) != NULL) {
            const std::string file_name = ent->d_name;
            const std::string full_file_name = folder + file_name;
            if (file_name[0] == '.')
                continue;

            bool error;
            bool is_dir = is_directory(full_file_name, error);
            if (error || is_dir)
                continue;

            std::wstring wfull_file_name = kFromUtf8(full_file_name);
            bool processFile = true;
            if (!filePattern.empty()) {
                processFile = (wfull_file_name.find(filePattern.c_str()) != std::wstring::npos);
            }
            if (processFile) {
                filesFound++;
                set.insert(wfull_file_name);
            }
        }
        closedir(dir);
    }
#else
	WIN32_FIND_DATAW searchResult;
	HANDLE fh = FindFirstFileW(filename1.c_str(), &searchResult);
	if (fh != INVALID_HANDLE_VALUE) {
		filesFound++;
		set.insert(searchResult.cFileName);
		if (maxNumber != 1) {
			while (FindNextFileW(fh, &searchResult) && ((maxNumber == 0) || (filesFound<maxNumber))) {
				filesFound++;
				set.insert(searchResult.cFileName);
			}
		}
	}
	FindClose(fh);

	fh = FindFirstFileW(filename2.c_str(), &searchResult);
	if (fh != INVALID_HANDLE_VALUE) {
		filesFound++;
		set.insert(searchResult.cFileName);
		if (maxNumber != 1) {
			while (FindNextFileW(fh, &searchResult) && ((maxNumber == 0) || (filesFound<maxNumber))) {
				filesFound++;
				set.insert(searchResult.cFileName);
			}
		}
	}
	FindClose(fh);
#endif
	std::set<std::wstring>::iterator it;
	for (it = set.begin(); it != set.end(); ++it) {
		result.push_back(it->data());
	}
	return result;
}

FILE *  k_wfopen(const wchar_t *filename, const wchar_t *mode)
{
#ifdef __linux__
	std::string sfilename = kToUtf8(filename);
	char *cfilename = sfilename.data();

	std::string smode = kToUtf8(mode);
	char *cmode = smode.data();

    return fopen(cfilename, cmode);
#else
	return _wfopen(filename, mode);
#endif
}

FILE *  k_wfsopen(const wchar_t *filename, const wchar_t *mode, int shflag)
{
#ifdef __linux__
	std::string sfilename = kToUtf8(filename);
	char *cfilename = sfilename.data();

	std::string smode = kToUtf8(mode);
	char *cmode = smode.data();

    return fopen(cfilename, cmode);
#else
	return _wfsopen(filename, mode, shflag);
#endif
}

void kFstreamOpen(std::fstream &fStreamData, const wchar_t *name, std::ios_base::openmode mode)
{
#ifdef __linux__
	std::wstring wstr(name);
	std::string _name = kToUtf8(wstr);
	fStreamData.open(_name.c_str(), mode);
#else
	fStreamData.open(name, mode);
#endif
}

void kFstreamWriteBytes(std::fstream &fStreamData, const wchar_t *s)
{
#ifdef __linux__
	for (size_t i = 0; i < wcslen(s); i++) {
		fStreamData.put(LOBYTE(s[i]));
		fStreamData.put(HIBYTE(s[i]));
	}
#else
	fStreamData.write((const char*)(s), 2 * wcslen(s));
#endif
}

char * kNewline()
{
#ifdef __linux__
	return "\n";
#else
	return "\r\n";
#endif
}

void _ksplitpathcomponents(const wchar_t *filename, wchar_t *drive, wchar_t *dir, wchar_t *datei, wchar_t *ext)
{
    std::wstring name(filename);
    bool isUNCPath = (name.find(L"\\\\") == 0);
    if (isUNCPath) {
        /*
        eg. "\\kissgear.ch\dfs\Applikationen\Kisssoft\Prog-2013-03\license\serverLicense500.lic"
        we want to find the 3rd \ (".ch\dfs")
        */
        unsigned counter = 0;
        size_t pos = name.find(L"\\");
        size_t offset = 0;
        if (pos != std::wstring::npos) {
            offset += pos + 1;
            pos = name.substr(offset).find(L"\\");
            offset += pos + 1;
            pos = name.substr(offset).find(L"\\");
            offset += pos + 1;
            pos = name.substr(offset).find(L"\\");
            offset += pos;

            std::wstring t1 = name.substr(0, offset);
            std::wstring t2 = name.substr(offset);

            size_t lastSlash = t2.find_last_of(L"\\") + 1;
            std::wstring file = t2.substr(lastSlash);
            size_t dotPosition = file.find(L".");
            if (dotPosition != std::wstring::npos) {
                wcscpy(datei, file.substr(0, dotPosition).c_str());
                wcscpy(ext, file.substr(dotPosition).c_str());
            } else {
                wcscpy(datei, L"");
                wcscpy(ext, L"");
            }
            t2 = t2.substr(0, lastSlash);

            wcscpy(drive, t1.c_str());
            wcscpy(dir, t2.c_str());
        }
        return;
    }
#ifdef __linux__
    // no drive letter for *nix variants
    wcscpy(drive, L"");

    std::string _filename = kToUtf8(filename);
    char resolved_path[PATH_MAX];
    strcpy(resolved_path, _filename.c_str());
    //realpath(_filename.c_str(), resolved_path);

    char path1[PATH_MAX], path2[PATH_MAX];
    strcpy(path1, resolved_path); // we need copies because dirname and basename may change the original path
    strcpy(path2, resolved_path);

    char *_dir = dirname(path1);
    char *_file = basename(path2);

    std::string sub = _filename.substr(_filename.length() - 1, 1);
    if (strcmp(sub.data(), "/") == 0) {
        // if the last character is a slash, we are looking at a folder
        strcpy(_dir, resolved_path);
        strcpy(_file, "");
    }

    std::wstring _wdir = kFromUtf8(_dir);
    if (_wdir == L".") {
        // we are dealing with a local file ("license165.lic"), and KISSsoft doesn't want the "." in the drive name
        _wdir = L"";
    } else if (!_wdir.empty()) {
        _wdir += kNativeSeparatorW();
    }
    wcscpy(dir, _wdir.c_str());

    std::wstring _wfile = kFromUtf8(_file);
    size_t dotPosition =_wfile.find(L'.');
    if (dotPosition == std::wstring::npos) {
        // no extension found
        wcscpy(datei, _wfile.c_str());
        wcscpy(ext, L"");
    } else {
        std::wstring _wfilename = _wfile.substr(0, dotPosition);
        std::wstring _wextension = _wfile.substr(dotPosition, _wfile.length() - dotPosition);
        wcscpy(datei, _wfilename.c_str());
        wcscpy(ext, _wextension.c_str());
    }
#else
    _wsplitpath(filename, drive, dir, datei, ext);
    std::wstring wdrive(drive);
    std::wstring wdir(dir);
    std::replace(wdrive.begin(), wdrive.end(), kNativeSeparatorWChar(), L'/');
    std::replace(wdir.begin(), wdir.end(), kNativeSeparatorWChar(), L'/');
    wcscpy(drive, wdrive.data());
    wcscpy(dir, wdir.data());
#endif
}

bool kIsUnicode(std::fstream &fStreamData)
{
	bool isUnicode = false;
	std::streamoff pos = fStreamData.tellg();
	fStreamData.seekg(0);
#ifdef __linux__
	char test[2];	
	fStreamData.read(test, 2);
	fStreamData.seekg(pos);
	int n1 = test[0];
	int n2 = test[1];
	isUnicode = (!((n1 ^ 0xFF) ^ (n2 ^ 0xFE)));
#else
	wchar_t test;	
	fStreamData.read((char*)&test, 2);
	fStreamData.seekg(pos);
	isUnicode = (test == wchar_t(0xFEFF));
#endif
	return isUnicode;
}

void kGetTempPath(wchar_t *path, size_t maxlen)
{
#ifdef __linux__
	char const *tmpFolder = getenv("TMPDIR");
	if (tmpFolder == 0) {
		tmpFolder = "/tmp";
	}
    std::wstring wpath = kFromUtf8(tmpFolder);
    wcsncpy(path, wpath.data(), wpath.size());
#else
	GetTempPathW((int)maxlen, path);
#endif
}

bool kGetFolderPath(wchar_t *path, size_t length, KPlatform::Folder folder)
{
#ifdef __linux__
	// such folders do not exist in linux
	return false;
#else
	int csidl = 0;
	switch (folder) {
	case KPlatform::APPDATA:		csidl = CSIDL_APPDATA; break;
	case KPlatform::PERSONAL:		csidl = CSIDL_PERSONAL; break;
	case KPlatform::LOCAL_APPDATA:	csidl = CSIDL_LOCAL_APPDATA; break;
	case KPlatform::COMMON_APPDATA:	csidl = CSIDL_COMMON_APPDATA; break;
	}
	return SUCCEEDED(SHGetFolderPathW(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, path));
#endif
}

std::wstring kGetShortPathName(const wchar_t *longPath)
{
#ifdef __linux__
	return std::wstring(longPath);
#else
	//https://msdn.microsoft.com/en-us/library/windows/desktop/aa364989(v=vs.85).aspx
	long     length = 0;
	WCHAR*   buffer = NULL;
	length = GetShortPathNameW(longPath, NULL, 0);
	if (length == 0) {
		return std::wstring();
	}
	buffer = new WCHAR[length];
	length = GetShortPathNameW(longPath, buffer, length);
	if (length == 0) {
		return std::wstring();
	}
	std::wstring shortPath(buffer);
	delete[] buffer;
	return shortPath;
#endif
}

bool kDirExist(const std::wstring &name)
{
	bool exist = false;
#ifndef __linux__
	wchar_t buffer[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, buffer);
	exist = SetCurrentDirectoryW(name.c_str()) == TRUE;
	if (buffer) {
		SetCurrentDirectoryW(buffer);
	}
#else
    std::string utf8name = kToUtf8(name);
    char *dirname = utf8name.data();
    exist = (access(dirname, 0) == 0);
#endif
	return exist;
}

bool kDirReadOnly(const std::wstring &dirname)
{
#ifndef __linux__
	wchar_t filename[MAX_PATH];
	if (GetTempFileNameW(dirname.c_str(), L"KS", 0, filename)) {
		DeleteFileW(filename);
		return false;
	}
	return true;
#else
	std::string sdirname = kToUtf8(dirname);
    const char *dname = sdirname.data();

	struct stat status;
	stat(dname, &status);
	bool isDirectory = (status.st_mode & S_IFDIR) != 0;
	if (isDirectory) {
        uid_t userID = getuid();
        gid_t userGroupID = getgid();

        uid_t ownerID = status.st_uid;
        gid_t groupID = status.st_gid;

        bool has_read_only_permission = true;
        if (userID == ownerID) {
                                                        // write by owner
            has_read_only_permission = ((status.st_mode & S_IWUSR) == 0);
        } else if (userGroupID == groupID) {
                                                        // write by group
            has_read_only_permission = ((status.st_mode & S_IWGRP) == 0);
        } else {
                                                        // write by others
            has_read_only_permission = ((status.st_mode & S_IWOTH) == 0);
        }
        return has_read_only_permission;
	}
	return false;
#endif
}

bool kDirMake(const std::wstring &name)
{
	if (!kDirExist(name)) {
		bool success;
#ifndef __linux__
		success = (CreateDirectoryW(name.c_str(), NULL) == TRUE);
#else
		// http://linux.die.net/man/3/mkdir
		std::string str = kToUtf8(name);
		// read/write/search permissions for owner and group, and with read/search permissions for others
		success = (mkdir(str.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0);
#endif
		return success;
	}
	return true;
}

bool kDirDelete(const std::wstring &name)
{
#ifdef __linux__
	std::string folder = kToUtf8(name);
	return (rmdir(folder.c_str()) == 0);
#else
	std::wstring copy = name;
	std::wstring directory = name;
	size_t length = copy.length();
	size_t lastPos = length - 1;
	if (length > 0 && copy[lastPos] != '\\' && copy[lastPos] != '/') {
		copy += L"/*.*";
		directory += L"/";
	} else {
		copy += L"*.*";
	}
	wchar_t dir[MAX_PATH];
	wcscpy(dir, directory.c_str());
	kDirDeleteAllFiles(copy, L"", dir);
	return (RemoveDirectoryW(name.c_str()) == 1);
#endif
}

bool kDirDeleteAllFiles(const std::wstring &fullPattern, wchar_t *drive, wchar_t *dir)
{
	bool success = true;
#ifdef __linux__
    std::string folder = kToUtf8(dir);
	DIR *theFolder = opendir(folder.c_str());
	struct dirent *next_file;
    char filepath[MAX_PATH];
    std::string extension;
    bool deleteAll = false;
    {
        wchar_t drive[MAX_PATH], directory[MAX_PATH], file[MAX_PATH], ext[MAX_PATH];
        _ksplitpathcomponents(fullPattern.c_str(), drive, directory, file, ext);
        extension = kToUtf8(ext);
        deleteAll = (extension == ".*");
    }

	while ((next_file = readdir(theFolder)) != NULL) {
		// build the path for each file in the folder
        if (next_file->d_name[0] == '.') {
            continue;
        }
        bool processEntry = deleteAll;
        if (!processEntry) {
            processEntry = (std::string(next_file->d_name).find(extension) != std::string::npos);
        }
        if (processEntry) {
            sprintf(filepath, "%s%s", folder.c_str(), next_file->d_name);

            bool error;
            bool is_dir = is_directory(filepath, error);
            if (error)
                continue;

            if (is_dir) {
                // clean up all files in folder first
                std::string contained_folder(filepath);
                contained_folder += "/";
                std::string contained_folder_pattern = contained_folder + "*.*";

                std::wstring _folder = kFromUtf8(contained_folder);
                std::wstring _pattern = kFromUtf8(contained_folder_pattern);
                success = success && kDirDeleteAllFiles(_pattern, L"", _folder.data());
            }
            success = success && (remove(filepath) == 0);
        }
	}
	closedir(theFolder);
#else
	WIN32_FIND_DATAW searchResult;
	HANDLE fh = FindFirstFileW(fullPattern.c_str(), &searchResult);
	if (fh != INVALID_HANDLE_VALUE) {
		if (searchResult.cFileName[0] != '.') {
			if (searchResult.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				std::wstring folder;
				folder += drive;
				folder += dir;
				folder += searchResult.cFileName;
				success = success && kDirDelete(folder);
			}
			else {
				std::wstring file;
				file += drive;
				file += dir;
				file += searchResult.cFileName;
				success = success && kFileDelete(file);
			}
		}
		while (FindNextFileW(fh, &searchResult)) {
			if (searchResult.cFileName[0] != '.') {
				if (searchResult.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					std::wstring folder;
					folder += drive;
					folder += dir;
					folder += searchResult.cFileName;
					success = success && kDirDelete(folder);
				}
				else {
					std::wstring file;
					file += drive;
					file += dir;
					file += searchResult.cFileName;
					success = success && kFileDelete(file);
				}
			}
		}
	}
	FindClose(fh);
#endif
	return success;
}

std::wstring getRelativeFolderForUnitTests(const wchar_t * path)
{
#ifdef __linux__
    std::wstring folder(L"//home//ksoft//");
    folder += KSettingFolder;
#else
    std::wstring folder(L"..//..//");
#endif
    folder += path;
    return folder;
}

#ifndef __linux__
/*
https://msdn.microsoft.com/en-us/library/windows/desktop/dd374130(v=vs.85).aspx
https://msdn.microsoft.com/en-us/library/windows/desktop/dd317756(v=vs.85).aspx
*/
const UINT LATIN_CODEPAGE = 1252;
#endif

std::wstring kFromLatin1(const std::string& latin1string)
{
#ifdef __linux__
	std::wstring wstr;
	for (size_t i = 0; i < latin1string.size(); i++) {
		int c = latin1string[i];
		if (c < 0) {
			c += 256;
		}
		if (c < 255) {
			wstr += c;
		}
		else {
			wstr += L'?';
		}
	}
	return wstr;
#else
	int widesize = ::MultiByteToWideChar(LATIN_CODEPAGE, 0, latin1string.c_str(), -1, NULL, 0);
	if (widesize == 0) {
		std::string str = "kFromLatin1 conversion error: " + latin1string;
		throw std::exception(str.c_str());
	}
	std::vector<wchar_t> resultstring(widesize);
	int convresult = ::MultiByteToWideChar(LATIN_CODEPAGE, 0, latin1string.c_str(), -1, &resultstring[0], widesize);
	if (convresult != widesize) {
		throw std::exception("kFromLatin1: widesize error");
	}
	return std::wstring(&resultstring[0]);
#endif
}

#pragma warning (push)
#pragma warning( disable:4305)

std::string kToLatin1(const std::wstring& widestring)
{
#ifdef __linux__
	std::string str;
	for (size_t i = 0; i < widestring.size(); i++) {
		wchar_t wc = widestring[i];
		int wv = wc;
		if (wv < 255 && wv > 0) {
			str += wv;
		}
		else {
			str += '?';
		}
	}
	return str;
#else
	int latin1size = ::WideCharToMultiByte(LATIN_CODEPAGE, 0, widestring.c_str(), -1, NULL, 0, NULL, NULL);
	if (latin1size == 0) {
		throw std::exception("kToLatin1: Error in conversion.");
	}
	std::vector<char> resultstring(latin1size);
	int convresult = ::WideCharToMultiByte(LATIN_CODEPAGE, 0, widestring.c_str(), -1, &resultstring[0], latin1size, NULL, NULL);
	if (convresult != latin1size) {
		throw std::exception("La falla!");
	}
	for (size_t i = 0; i < resultstring.size(); i++) {
		if ((int)resultstring[i] < 0) {
			resultstring[i] += 256;
		}
	}
	return std::string(&resultstring[0]);
#endif
}
#pragma warning (pop)

#ifndef __linux
const UINT UTF8_CODEPAGE = CP_UTF8;
#endif

std::wstring kFromUtf8(const std::string& utf8string)
{
#ifdef __linux__
	setlocale(LC_ALL, "");
	const char *str = utf8string.c_str();
	wchar_t buffer[1024];
	int ret;
	ret = mbstowcs(buffer, str, 1024);
	if (ret == 1023) {
		// error in conversion
		buffer[1023] = L'\0';
	}
	std::wstring result(buffer);
	return result;
#else
	int widesize = ::MultiByteToWideChar(UTF8_CODEPAGE, 0, utf8string.c_str(), -1, NULL, 0);
	if (widesize == 0) {
		int err = GetLastError();
		std::string str = ("Error " + std::to_string(err) + " in conversion of UTF8 string: \n") + utf8string;
		throw std::exception(str.c_str());
	}
	std::vector<wchar_t> resultstring(widesize);
	int convresult = ::MultiByteToWideChar(UTF8_CODEPAGE, 0, utf8string.c_str(), -1, &resultstring[0], widesize);
	if (convresult != widesize) {
		throw std::exception("kFromUtf8: widesize error");
	}
	return std::wstring(&resultstring[0]);
#endif
}

std::string kToUtf8(const std::wstring& widestring)
{
#ifdef __linux__
	setlocale(LC_ALL, "");
	const wchar_t *str = widestring.c_str();
	char buffer[1024];
	int ret;
	ret = wcstombs(buffer, str, 1024);
	if (ret == 1023) {
		// error in conversion
		buffer[0] = '\0';
	}
	std::string result(buffer);
	return result;
#else
	int utf8size = ::WideCharToMultiByte(UTF8_CODEPAGE, 0, widestring.c_str(), -1, NULL, 0, NULL, NULL);
	if (utf8size == 0) {
		throw std::exception("kToUtf8: Error in conversion.");
	}
	std::vector<char> resultstring(utf8size);
	int convresult = ::WideCharToMultiByte(UTF8_CODEPAGE, 0, widestring.c_str(), -1, &resultstring[0], utf8size, NULL, NULL);
	if (convresult != utf8size) {
		throw std::exception("La falla!");
	}
	return std::string(&resultstring[0]);
#endif
}

std::wstring kFromkWchar(const kWchar *s)
{
	std::wstring wStringData;
#ifdef __linux__
	wStringData = L"";
	unsigned int i = 0;
	wchar_t c;
	while (s[i]) {
		c = s[i];
		wStringData += c;
		i++;
	}
#else
	wStringData = (wchar_t*)s;
#endif
	return wStringData;
}

std::wstring ktoRTFW(const std::wstring &data)
{
	std::wstring result = L"";
#ifdef __linux__
#else
	size_t len = data.length();
	for (size_t i = 0; i < len; i++) {
		wchar_t wc = data[i];
		if (wc>127) {
			if (wc != 0xFEFF) // Kennung für Unicode am Dateianfang unterdrücken
			{
				wchar_t buffer[16];
				_itow(wc, buffer, 10);
				result += L"\\u" + std::wstring((wchar_t*)buffer) + L"?";
			}
		}
		else {
			result += wc;
		}
	}
#endif
	return result;
}

void kAssign(std::wstring &wStringData, const kWchar *r)
{
#ifdef __linux__
	wStringData = L"";
	unsigned int i = 0;
	wchar_t c;
	while (r[i]) {
		c = r[i];
		wStringData += c;
		i++;
	}
#else
	wStringData = (wchar_t*)r;
#endif
}

char *kitoa(int32_t i, char* s, size_t maxLen, int dummy_radix) {
#ifdef __linux__
	snprintf(s, maxLen, "%" PRIi32, i);
#else
	if (_itoa_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = '\0';
	}
#endif
	return s;
}

wchar_t *kitow(int32_t i, wchar_t* s, size_t maxLen, int dummy_radix) {
#ifdef __linux__
	swprintf(s, maxLen, L"%" PRIi32, i);
#else
	if (_itow_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = L'\0';
	}
#endif
	return s;
}

char *ki64toa(int64_t i, char* s, size_t maxLen, int dummy_radix)
{
#ifdef __linux__
	snprintf(s, maxLen, "%" PRIi64, i);
#else
	if (_i64toa_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = '\0';
	}
#endif
	return s;
}

wchar_t *ki64tow(int64_t i, wchar_t* s, size_t maxLen, int dummy_radix)
{
#ifdef __linux__
	swprintf(s, maxLen, L"%" PRIi64, i);
#else
	if (_i64tow_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = L'\0';
	}
#endif
	return s;
}

char *kutoa(uint32_t i, char* s, size_t maxLen, int dummy_radix) {
#ifdef __linux__
	snprintf(s, maxLen, "%" PRIu32, i);
#else
	if (_ultoa_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = '\0';
	}
#endif
	return s;
}

wchar_t *kutow(uint32_t i, wchar_t* s, size_t maxLen, int dummy_radix) {
#ifdef __linux__
	swprintf(s, maxLen, L"%" PRIu32, i);
#else
	if (_ultow_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = L'\0';
	}
#endif
	return s;
}

wchar_t *kltow(long i, wchar_t* s, size_t maxLen, int radix) {
#ifdef __linux__
	swprintf(s, maxLen, L"%" PRIu32, i);
#else
	if (_ltow_s(i, s, maxLen, radix) != 0) {
		s[0] = L'\0';
	}
#endif
	return s;
}

char *ku64toa(uint64_t i, char* s, size_t maxLen, int dummy_radix) {
#ifdef __linux__
	snprintf(s, maxLen, "%" PRIu64, i);
#else
	if (_ui64toa_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = '\0';
	}
#endif
	return s;
}

wchar_t *ku64tow(uint64_t i, wchar_t* s, size_t maxLen, int dummy_radix) {
#ifdef __linux__
	swprintf(s, maxLen, L"%" PRIu64, i);
#else
	if (_ui64tow_s(i, s, maxLen, dummy_radix) != 0) {
		s[0] = L'\0';
	}
#endif
	return s;
}

int kwtoi(const wchar_t *str)
{
#ifdef __linux__
	std::string sstr = kToUtf8(str);
    char *tmp = sstr.data();
	return atoi(tmp);
#else
	return _wtoi(str);
#endif
}

int64_t kwtoi64(const wchar_t *str)
{
#ifdef __linux__
    wchar_t *ptr;
    int64_t result = wcstoll(str, &ptr, 10);
	return result;
#else
	return _wtoi64(str);
#endif
}

double kwtof(const wchar_t *str)
{
#ifdef __linux__
	std::string sstr = kToUtf8(str);
	char *tmp = sstr.data();
	return atof(tmp);
#else
	return _wtof(str);
#endif
}

long kwtol(const wchar_t *str)
{
#ifdef __linux__
	std::string sstr = kToUtf8(str);
	char *tmp = sstr.data();
	return atof(tmp);
#else
	return _wtol(str);
#endif
}

void k_wcsupr_s(wchar_t *string, unsigned size)
{
#ifdef __linux__
	for (int i = 0; i < size; i++)
	{
		string[i] = towupper(string[i]);
	}
#else
	_wcsupr_s(string, size);
#endif
}

char * k_strdup(const char *strSource)
{
#ifdef __linux__
	return strdup(strSource);
#else
	return _strdup(strSource);
#endif
}

int kstrcasecmp(const char *string1, const char *string2)
{
#ifdef __linux__
	return strcasecmp(string1, string2);
#else
	return _stricmp(string1, string2);
#endif
}

double kwcstod(const wchar_t *str, wchar_t **endptr)
{
#ifdef __linux__
	std::string sstr = kToUtf8(str);
	char *tmp = sstr.data();
	return strtof(tmp, (char**)endptr);
#else
	return wcstod(str, endptr);
#endif
}

char *kstrlwr(char *text)
{
#ifdef __linux__
	char *origtext = text;
	while (*text != '\0')
	{
		if ((*text > 64) && (*text < 91)) *text += 32;
		text++;
	}
	return (origtext);
#else
	return _strlwr(text);
#endif
}

errno_t kstrncpy_s(char *strDestination, size_t numberOfElements, const char *strSource)
{
#ifdef __linux__
	strncpy(strDestination, strSource, numberOfElements);
	return 0;
#else
	return strcpy_s(strDestination, numberOfElements, strSource);
#endif
}

errno_t kstrncat_s(char *strDestination, size_t numberOfElements, const char *strSource)
{
#ifdef __linux__
	strncat(strDestination, strSource, numberOfElements);
	return 0;
#else
	return strcat_s(strDestination, numberOfElements, strSource);
#endif
}

errno_t k_wcscpy_s(wchar_t *strDestination, size_t numberOfElements, const wchar_t *strSource)
{
#ifdef __linux__
	wcscpy(strDestination, strSource);
	return 0;
#else
	return wcscpy_s(strDestination, numberOfElements, strSource);
#endif
}

char *kstrtok(char *str, const char *delim, char **saveptr)
{
#ifdef __linux__
	return strtok_r(str, delim, saveptr);
#else
	return strtok_s(str, delim, saveptr);
#endif
}

wchar_t *kwcstok(wchar_t *wstr, const wchar_t *delim)
{
#ifdef __linux__
	wchar_t *state;
	return wcstok(wstr, delim, &state);
#else
	return wcstok(wstr, delim);
#endif
}

std::vector<std::wstring> kToWStringList(const std::wstring &wStringData, wchar_t sep, bool keepEmptyParts)
{
	size_t sepPos;
	size_t oldPos = 0;
	std::vector<std::wstring> result;
	while ((sepPos = wStringData.find(sep, oldPos)) != std::wstring::npos) {
		std::wstring elem = wStringData.substr(oldPos, sepPos - oldPos);
		if (keepEmptyParts || !elem.empty()) {
			result.push_back(elem);
		}
		oldPos = sepPos + 1;
	}
	std::wstring elem = wStringData.substr(oldPos);
	if (keepEmptyParts || !elem.empty()) {
		result.push_back(elem);
	}
	return result;
}

std::vector<std::string> kToStringList(const char *_buf, char seperator, bool keepEmptyParts)
{
	std::vector<std::string> list;
	const char *start = _buf;
	while (start && *start) {
		const char *end = start;
		while (*end && *end != seperator) {
			end++;
		}
		if (*end) {
			std::string neu(start, end - start);
			if (neu.length() > 0 || keepEmptyParts) {
				list.push_back(neu);
			}
			start = end + 1;
		} else {
			std::string neu = start;
			if (neu.length() > 0 || keepEmptyParts) {
				list.push_back(neu);
			}
			start = end;
		}
	}
	return list;
}

#define INFO_BUFFER_SIZE 32767

bool kGetUserName(wchar_t *username, unsigned long &length)
{
	length = sizeof(username);
	bool ok = true;
#ifdef __linux__
	char *name = getlogin();
	ok = (name != 0);
	if (ok) {
		std::string str(name);
		std::wstring wname = kFromUtf8(str);
		wcsncpy(username, wname.c_str(), wname.size());
	}
#else
	TCHAR infoBuf[INFO_BUFFER_SIZE];
	DWORD bufCharCount = INFO_BUFFER_SIZE;
	ok = (GetUserName(infoBuf, &bufCharCount) == 1);
	if (ok) {
		length = bufCharCount;
		wcsncpy(username, (wchar_t *)infoBuf, length);
	}
#endif
	if (!ok) {
		length = 0;
		wcsncpy(username, L"", 0);
	}
	return ok;
}

int kSystemCommand(const wchar_t *command, bool interactive)
{
#ifdef __linux__
	std::string scommand = kToUtf8(command);
	char *comm = scommand.data();
    return system(comm);
#else
	if (!interactive) {
		STARTUPINFOW si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		if (CreateProcessW(command, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return 0;
		}
		return 1;
	}
	return _wsystem(command);
#endif
}

unsigned kNumberOfCPUCores()
{
	// returns the number of CPU cores the computer has
	int numCPU = 1;
#ifdef __linux__
	numCPU = sysconf(_SC_NPROCESSORS_ONLN);
#else
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	numCPU = sysinfo.dwNumberOfProcessors;
#endif
	return numCPU;
}

size_t kGetCacheLineSize()
{
#ifdef __linux__
	FILE * p = 0;
	p = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
	int lineSize = 0;
	if (p) {
		fscanf(p, "%d", &lineSize);
		fclose(p);
	}
	return lineSize;
#else
	size_t lineSize = 0;
	DWORD bufferSize = 0;
	DWORD i = 0;
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION * buffer = 0;
	GetLogicalProcessorInformation(0, &bufferSize);
	buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(bufferSize);
	GetLogicalProcessorInformation(&buffer[0], &bufferSize);
	for (i = 0; i != bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) {
		if (buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == 1) {
			lineSize = buffer[i].Cache.LineSize;
			break;
		}
	}
	free(buffer);
	return lineSize;
#endif
}

size_t kCurrentMemoryUsage()
{
	// from http://nadeausoftware.com/articles/2012/07/c_c_tip_how_get_process_resident_set_size_physical_memory_use
#ifdef __linux__
	/* Linux ---------------------------------------------------- */
	long rss = 0L;
	FILE* fp = NULL;
	if ((fp = fopen("/proc/self/statm", "r")) == NULL)
		return (size_t)0L;		/* Can't open? */
	if (fscanf(fp, "%*s%ld", &rss) != 1) {
		fclose(fp);
		return (size_t)0L;		/* Can't read? */
	}
	fclose(fp);
	return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);
#else
	/* Windows -------------------------------------------------- */
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
	return (size_t)info.WorkingSetSize;
#endif
}

int kGetLastError()
{
#ifdef __linux__
	return errno;
#else
	return GetLastError();
#endif
}

void kCopyCurrentTimeToBuffer(char *buffer, size_t len)
{
#ifdef __linux__
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, len, "%I:%M%p.", timeinfo);
#else
	_strtime_s(buffer, len);
#endif
}

bool kStartDetachedProcess(const wchar_t *program, const wchar_t *arguments, const wchar_t *binDir, int64_t *clientId)
{
	// todo
	return false;
}

unsigned kGetMSB(uint64_t number)
{
#ifdef __linux__
	return __builtin_clzll(number);
#else
	unsigned long index;
	_BitScanReverse(&index, (unsigned long)number);
	return index;
#endif
}

void kzset()
{
#ifdef __linux__
    return tzset();
#else
    return _tzset();
#endif
}
