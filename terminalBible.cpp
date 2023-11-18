/*
A program to access God's Word from the terminal

*/
#include "BibleText.h"
#include "BibleSearch.h"

#include <unordered_map>
#include <cstring>
#include <string>
#include <sqlite3.h>
#include <iostream>
#include <fstream>
#include <list>
#include <utility>
#include <queue>
#include <tuple>
#include <chrono>
#include <ctime>

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
void searchAndOutputToConsole(const char* directory, std::string search) {
	
	BibleSearch WordSearch = BibleSearch(directory);

	std::queue<int> searchResults;
	WordSearch.verseIDsFromWordSearch(search, searchResults);

	BibleText Bible = BibleText(directory);
	printVerses(searchResults, Bible, 2);
}
// given an input string identifies it's book, chapter and verse
// return a pair of VerseID ints? potential improvement
void parseReferenceIntoTuple(std::string line, std::tuple<std::string, int, int>& reference) {
	std::string cur;
	std::string book = "";
	int chapter = -1;
	int verse = -1;
	int state = 0;
	for (size_t i = 0; i < line.length(); i++) {
		char c = line[i];
		if (c == ' ') { // ignore spaces
			continue;
		}
		switch(state) {
		case 0:
			if (isdigit(c) && cur.length() > 0) { // book name (can start with a digit)
				book = cur;
				cur = "";
				state++;

				if (i == line.length() - 1) { // if end of string, finish here and leave verse as -1
					chapter = c - '0'; // convert recent char to int
				}
			}
			cur += c;
			break;
		case 1:
			if (c != ':') {
				cur += c; // chapter
				if (i == line.length() - 1) { // if end of string, finish here and leave verse as -1
					chapter = stoi(cur);
				}
			}
			else {
				chapter = stoi(cur);
				cur = "";
				state++;
			}
			break;
		case 2:
			cur += c; // verse (":" onwards)
			if (i == line.length() - 1) {
				verse = stoi(cur);
			}
		}
	}
	reference = std::make_tuple(book, chapter, verse);
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
			int skipFlag = 0;
			if (line[0] == '/') { // identify flag if present
				skipFlag += 2;
				if (line[1] == 'r') {
					queryMode = 0;
				}
				else if (line[1] == 'm') {
					queryMode = 1;
				}
				else if (line[1] == 's') {
					queryMode = 2;
				}
			}
			if (queryMode == 0 || queryMode == 1) { // reading or memory

				BibleText Bible = BibleText(directory);

				std::string first = "";
				std::string second = "";
				std::tuple<std::string, int, int> ref; // parse the reference from the input line
				int state = 0;
				for (unsigned int i = skipFlag; i < line.length(); i++) {
					char c = line[i];
					if (c == ' ') {
						continue;
					}
					switch(state) {
					case 0:
						if (c == '-') {
							state++;
							second += c;
							break;
						}
						first += c;
						break;
					case 1:
						second += c;
						break;
					}
				}
				parseReferenceIntoTuple(first, ref);

				std::queue<int> results; // results of query stored here
				if (Bible.query(results, ref, second) == 0) {
					printVerses(results, Bible, queryMode);
				}
				else {
					std::cout << "An error occured" << std::endl;
				}
			}
			else if (queryMode == 2) { // search
				std::string search;
				for(unsigned int i = skipFlag; i < line.length(); i++) { // skip first character, because I can't figure how to remember search flag in a user-friendly manner
					search += line[i];
				}
				searchAndOutputToConsole(directory, search);
			}
		}
	}
	else {
		std::cout << "Missing filename. Enter your primary \".db\" but without the file extension" << std::endl;
	}
}