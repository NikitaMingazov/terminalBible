/*
A program to access God's Word from the terminal

*/
#include "BibleText.h"
#include "BibleSearch.h"

#include <cstring>
#include <string>
#include <iostream>
#include <list>
#include <utility>
#include <queue>
#include <tuple>

/* Abbreviation version
void populateNameTable(std::string* ident, std::string filename) {
	std::ifstream inputFile(filename); // "kjv.txt" for KJV, etc.
	if (inputFile.is_open()) {
		std::string line;
		int i = 0;
		while (getline(inputFile, line) && i < 66) {
			std::string cur;
			for (int i = 0; i < line.length(); i++) {
				char c = line[i];
				if (c == '#' ||) { // comment detected, finish this line
					break;
				}
				if (c != ' ') {
					cur += c;
				}
				else {
					break;
				}
			}
			if (cur.length() > 0) {
				ident[i++] = cur; // the last thing cur was set to was ID
			}
		}
		inputFile.close();
	}
	 else {
		std::cerr << "Failed to open the file." << std::endl;
	}
}*/

inline size_t getCodePointSize(const std::string& utf8String, size_t index) {
	unsigned char firstByte = static_cast<unsigned char>(utf8String[index]);

	if (firstByte < 0b10000000) {
		// Single-byte character (0*)
		return 1;
	} else if (firstByte < 0b11100000) {
		// Two-byte character (110*)
		return 2;
	} else if (firstByte < 0b11110000) {
		// Three-byte character (1110*)
		return 3;
	} else {
		// Four-byte character (11110*)
		return 4;
	}
}
inline std::string readUtf8Character(const std::string& utf8String, size_t& index) {
	std::string character;

	size_t length = getCodePointSize(utf8String, index);
	for (size_t i = 0; i < length; i++) {
		character += utf8String[index];
		index++;
	}
	return character;
}

// in the given line, skips over the second+ letter in each Word
std::string firstLetterOfEachWord(const std::string& line) {
	std::string output;
	bool newWord = true;

	for (char c : line) {
		if (c == '[' || c == ']') {
			continue;
		}
		if (std::isalpha(c) && newWord) {
			output += c;
			newWord = false;
		} else if (!std::isalpha(c)) {
			output += c;
			if (c == ' ' || c == '\t') {
				newWord = true;
			}
		}
	}
	return output;
}
// print the given VerseIDs out of the given Bible
void printVerses(std::queue<int>& toPrint, BibleText& Bible, int queryMode) {
	std::tuple<std::string, int, int, std::string> verse;
	
	while (!toPrint.empty()) {
		int VerseID = toPrint.front();
		toPrint.pop();

		std::tuple<std::string, int, int, std::string> verse;
		Bible.retrieveVerseFromID(VerseID, verse);
		std::string body = std::get<3>(verse);
		if (queryMode == 1) { // memorisation
			body = firstLetterOfEachWord(body);
		}
		std::cout << std::get<0>(verse) << " " << std::get<1>(verse) << ":" << std::get<2>(verse) << " " << body << std::endl;
	}
}
int main(int argc, char **argv) { // notes: create a copy mode for referencing, and remember previous book/mode queried, x- until end of chapter
	int queryMode = 0;
	const char* directory;
	if (argc > 1) {
		directory = argv[1];

		while(true) {
			std::string line;
			std::getline(std::cin, line); // ask for input
			if (line.length() == 0) {
				std::cout << "No input, program ended" << std::endl;
				break;
			}
			bool skipFlag = false;
			if (line[0] == '/') { // identify flag if present
				skipFlag = true;
				if (line[1] == 'r') {
					queryMode = 0;
				}
				else if (line[1] == 'm') {
					queryMode = 1;
				}
				else if (line[1] == 's') {
					queryMode = 2;
				}
				else {
					std::cout << "unknown flag attempted" << std::endl;
					continue;
				}
			}
			if (skipFlag) {
				line = line.substr(2); // prune flag from line
			}
			if (queryMode == 0 || queryMode == 1) { // reading or memory

				BibleText Bible = BibleText(directory);

				std::queue<int> results; // results of query stored here
				if (Bible.query(results, line) == 0) {
					printVerses(results, Bible, queryMode);
				}
				else {
					std::cout << "An error occured" << std::endl;
				}
			}
			else if (queryMode == 2) { // search
				BibleSearch WordSearch = BibleSearch(directory);

				std::queue<int> searchResults;
				WordSearch.verseIDsFromWordSearch(line, searchResults);

				BibleText Bible = BibleText(directory);
				if (searchResults.size() != 0) {
					printVerses(searchResults, Bible, queryMode);	
				}
				else {
					std::cout << "No search results" << std::endl;
				}
			}
		}
	}
	else {
		std::cout << "Missing filename. Enter your primary \".db\" but without the file extension" << std::endl;
	}
}