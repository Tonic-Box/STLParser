//
//  main.cpp
//  D3Parser
//

#include <iostream>
#include <fstream>
#include <stdio.h>
#include "time.h"
#include "types.h"
#include <map>
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <direct.h>
#include <algorithm>
#include <filesystem>

#ifdef WIN32
#include <io.h>
#else
#include <dirent.h>
#endif

#include <unordered_set>
#include "tinyxml.h"
#include <regex>

#define MAX_DATE 12

class Range {
public:
	Range operator+(Range& other) {
		Range result = *this;
		result.min += other.min;
		result.max += other.max;
		return result;
	}

	Range operator-(Range& other) {
		Range result = *this;
		result.min -= other.min;
		result.max -= other.max;
		return result;
	}

	Range operator*(Range& other) {
		Range result = *this;
		result.min *= other.min;
		result.max *= other.max;
		return result;
	}

	Range operator/(Range& other) {
		Range result = *this;
		result.min /= other.min;
		result.max /= other.max;
		return result;
	}

	float min;
	float max;

};

typedef int FileID;
std::map<FileID, std::string> filesMap;

typedef std::map<Hash, std::vector<std::string> > StringsHashMap;
//std::map<Hash, std::vector<std::string> > stringsHashMap;
std::map<int, StringsHashMap > stringsHashMap;

std::string get_date(void)
{
	time_t now;
	char the_date[MAX_DATE];
	the_date[0] = '\0';
	now = time(NULL);
	if (now != -1)
	{
		strftime(the_date, MAX_DATE, "%d_%m_%Y", gmtime(&now));
	}
	return std::string(the_date);
}

Hash hash(const char* string) {
	Hash hash = 0;
	size_t n = strlen(string);
	for (size_t i = 0; i < n; i++)
		hash = (hash * 0x21) + tolower(string[i]);
	return hash;
}

bool isValidHash(Hash h) {
	return h != 0 && h != -1;
}

Byte* fileContents(const char* fileName, size_t* fileSize) {
	FILE* f = fopen(fileName, "rb");
	if (!f)
		return NULL;

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (size == 0)
		return NULL;

	Byte* memory = new Byte[size];
	fread(memory, size, 1, f);
	fclose(f);

	if (fileSize)
		*fileSize = size;
	return memory;
}

void processStlFile(MPQStlHeader* stlHeader) {
	int count = stlHeader->entriesSize / sizeof(MPQStlEntry);
	MPQStlEntry* pStlEntry = reinterpret_cast<MPQStlEntry*>(stlHeader + 1);
	for (int i = 0; i < count; i++) {
		if (i == 48) {
			i = i;
		}
		std::vector<std::string> strings;
		strings.reserve(4);
		int32_t offsets[] = { pStlEntry->string1offset, pStlEntry->string2offset };
		int32_t sizes[] = { pStlEntry->string1size, pStlEntry->string2size };
		std::string key;
		for (int i = 0; i < 2; i++) {
			if (offsets[i] > 0 && sizes[i] > 1) {
				const char* s = reinterpret_cast<char*>(stlHeader) + offsets[i];
				strings.push_back(s);
				if (key.length() == 0)
					key = s;
			}
			else
				strings.push_back("");
		}
		Hash h = hash(key.c_str());
		stringsHashMap[stlHeader->stlFileId][h] = strings;
		pStlEntry += 1;
	}
}

void processFile(const std::string& fileName) {
	size_t size = 0;
	Byte* contents = fileContents(fileName.c_str(), &size);
	if (!contents)
		return;

	MPQHeader* mpqHeader = (MPQHeader*)contents;
	std::string extension = fileName.substr(fileName.find_last_of(".") + 1);
	if (extension == "stl") {
		MPQStlHeader* stlHeader = reinterpret_cast<MPQStlHeader*>(mpqHeader + 1);
		processStlFile(stlHeader);
		filesMap[stlHeader->stlFileId] = fileName;
	}
	delete[] contents;
}

std::vector<std::string> stlFiles() {
	std::string dirPath = "StringList/";

#ifdef WIN32
	struct _finddata_t c_file;
	intptr_t hFile;

	std::vector<std::string> files;
	if ((hFile = _findfirst("./StringList/*.stl", &c_file)) != -1) {
		do {
			std::string fileName = c_file.name;
			files.push_back(dirPath + fileName);
		} while (_findnext(hFile, &c_file) == 0);
		_findclose(hFile);
	}

#else
	DIR* dir = opendir(dirPath.c_str());
	std::vector<std::string> files;
	struct dirent* ent;
	if (dir) {
		while ((ent = readdir(dir)) != NULL) {
			std::string fileName = ent->d_name;
			std::string extension = fileName.substr(fileName.find_last_of(".") + 1);
			if (extension == "stl")
				files.push_back(dirPath + fileName);
		}
	}
#endif
	return files;
}

std::string replaceQuotes(const std::string& s) {
	std::string ss;
	for (auto c : s) {
		if (c == '"')
			ss += '"';
		ss += c;
	}
	return ss;
}

void processStrResults() {
	std::ofstream stringsCSV("RAW_STRINGS\\" + get_date() + "-RAW_STRINGS.txt", std::ios::out | std::ios::trunc);

	for (auto i : stringsHashMap) {
		const auto resourceFile = filesMap[i.first].c_str();
		printf_s("processing %s \n", resourceFile);
		for (auto j : i.second) {
			const std::vector<std::string>& strings = j.second;
			std::regex e("([^\n]|^)\n([^\n]|$)");
			std::string desc = std::regex_replace(strings[1], e, "$1 $2");
			if (desc == "") {
				desc = strings[0];
			}
			stringsCSV << strings[0] << std::endl <<
				desc << std::endl;
		}
	}
}

void CleanUpRawDump(const std::string& FilePath)
{
	std::ifstream in(FilePath);
	std::string line, text;
	std::string bullets = "  {icon:bullet}";
	std::string bullet = "{icon:bullet}";
	std::string c_garbage = "{s1}";
	//!(line.empty() || line.find_first_not_of(' ') == std::string::npos) && 
	while (std::getline(in, line)) {
		if (line.substr(0, bullets.size()) != bullets && line.substr(0, c_garbage.size()) != c_garbage && line.substr(0, bullet.size()) != bullet) {
			text += line + "\n";
		}
	}
	in.close();
	std::ofstream out(FilePath);
	out << text;
}

int HashLowerCaseToInt(std::string input, std::string test)
{
	std::transform(input.begin(), input.end(), input.begin(), ::tolower);
	int num = 0;
	int count = input.length();
	for (int i = 0; i < count; i++)
	{
		num = (num << 5) + num + (int)input[i];
	}
	return num;
}

std::vector<std::string> HashToIntValue(std::vector<std::string> output, std::string sDate, std::vector<std::string> name, int i, std::string gbids, std::string text)
{
	for (int j = 0; j < name.size(); j++)
	{
		if (name[j].size() && j % 2 == 0)
		{
			int gbid = HashLowerCaseToInt(name[j], name[j + 1]);
			char buffer[100] = { 0 };
			int number_base = 10;
			std::string o = itoa(gbid, buffer, number_base);
			std::string i_name = name[j + 1];
			if (!(i_name.size())) {
				i_name = name[j];
			}
			text = o + "\t=\t" + i_name + "\t=\tNew_Item\t=\tConsole\t=\t\n";
			std::cout << text;
			output.push_back(text);
		}
	}
	return output;
}

int main(int argc, const char* argv[])
{
	_mkdir("RAW_STRINGS");
	std::vector<std::string> stls = stlFiles();
	for (auto path : stls) {
		processFile(path.c_str());
	}
	processStrResults();
	//CleanUpRawDump("RAW_STRINGS\\" + get_date() + "-RAW_STRINGS.txt");
	std::cout << "Processing RAW_STRINGS\\" << get_date() << "-RAW_STRINGS.txt\n72546\n";
	try {
		std::cout << "Hashing raw dump...\n";
		std::vector<std::string> output;
		std::string path = "RAW_STRINGS\\" + get_date() + "-RAW_STRINGS.txt";
		int num = 0;
		std::vector<std::string> name;
		std::string line;
		std::ifstream myfile(path);
		while (std::getline(myfile, line))
		{
			name.push_back(line);
		}
		myfile.close();
		std::vector<std::string> contents = HashToIntValue(output, "", name, num, "", "");
		_mkdir("GBIDs");
		std::ofstream outfile("GBIDs\\" + get_date() + "-GBID_Dump.txt");
		for (int j = 0; j < contents.size(); j++) {
			outfile << contents[j];
		}
		std::cout << "Done!\n";
	}
	catch (...) {
		std::cout << "No hashable data found in stl... Only raw strings dumped.\n";
	}
}

