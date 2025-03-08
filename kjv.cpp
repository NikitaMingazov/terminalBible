/*
Generate Bible references from a script
*/
#include "BibleText.h"
#include "BibleSearch.h"

#include <unistd.h>

#include <cstring>
#include <string>
#include <iostream>
#include <list>
#include <utility>
#include <queue>
#include <tuple>
#include <cctype> // for uppercase conversion
//#include <sstream>
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
		/*if (queryMode == 2) { // memorisation (depreciated)
			body = firstLetterOfEachWord(body);
		}*/
		std::cout << std::get<0>(verse) << " " << std::get<1>(verse) << ":" << std::get<2>(verse) << " " << body << std::endl;
	}
}
int main(int argc, char **argv) { // notes: create a copy mode for referencing, and remember previous book/mode queried, x- until end of chapter
	int queryMode = 0;
	const char* directory = argv[1];
	if (argc >= 3 && std::strlen(argv[2]) > 1 ) { // no flag, so reading mode
		std::string line = std::string(argv[2]);
		// capitalising book title to enable one-handed use
		if (std::islower(line[0])) {
			line[0] = std::toupper(line[0]);
		}
		else if (!std::isupper(line[0])) {
			// first character was a number
			line[1] = std::toupper(line[1]);
		}
		BibleText Bible = BibleText(directory);

		std::queue<int> results; // results of query stored here
		if (Bible.query(results, line) == 0) {
			if (results.size() == 0) {
				std::cout << "No results" << std::endl;
			}
			else {
				printVerses(results, Bible, queryMode);
			}
		}
		else {
			std::cout << "An error occured" << std::endl;
		}
	}
	if (argc > 3) { // other usage modes (search, memorisation)
		int mode = 0;
		char flag = argv[2][0];
		std::string line = std::string(argv[3]);
		if (flag == 's') {
			mode = 1;
		}
		else if (flag == 'm') {
			mode = 2;
		}
		else if (flag == 'c') {
			mode = 3;
		}
		if (mode == 0 || mode == 2) {

			BibleText Bible = BibleText(directory);

			std::queue<int> results; // results of query stored here
			if (Bible.query(results, line) == 0) {
				if (results.size() > 0) {
					printVerses(results, Bible, mode);
				}
				else {
					std::cout << "No match found" << std::endl;
				}
			}
			else {
				std::cout << "An error occured" << std::endl;
			}
		}
		else if (mode == 1) { // search
			BibleSearch WordSearch = BibleSearch(directory);

			std::queue<int> searchResults;
			WordSearch.verseIDsFromWordSearch(line, searchResults);

			BibleText Bible = BibleText(directory);
			if (searchResults.size() != 0) {
				printVerses(searchResults, Bible, mode);
			}
			else {
				std::cout << "No search results" << std::endl;
			}
		}
		else if (mode == 3) { // copying/referencing
			// don't print Book/Chapter on each new of same kind
		}
		else if (mode == 4) {
			// todo: body only, no reference
		}
	}
}
