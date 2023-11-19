#include "BibleSearch.h"

#include <iostream>

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
BibleSearch::BibleSearch(const char* inputDir) {
	std::string directory = std::string(inputDir)+"/";

	rc = sqlite3_open_v2((directory+"BibleSearch.db").c_str(), &searchdb, SQLITE_OPEN_READONLY, nullptr);
	if (rc) {
		std::cerr << "Can't open database: " << sqlite3_errmsg(searchdb) << std::endl;
		return;
	}
}
BibleSearch::~BibleSearch() {
	sqlite3_close(searchdb);
}
// turns a word search into a SQL statement and the parameters to fill it
std::string BibleSearch::parseSearchIntoSqlStatement(std::string search, std::queue<std::string>& parameters) {
	// splitting up the search into it's elements
	std::queue<std::string> unparsed;
	std::string substring;
	for (size_t i = 0; i < search.length();) {
		std::string c = readUtf8Character(search, i);
		if (c == "(" || c == ")") {
			if (substring.length() > 0) {
				unparsed.push(substring);
				substring = "";
			}
			unparsed.push(c);
			continue;
		}
		if (c == "/" || c == " ") { // new word
			if (substring.length() > 0) {
				unparsed.push(substring);
			}
			substring = "";
		}
		substring += c;
		if (substring == "/OR") {
			unparsed.push("/OR");
			substring = "";
		}
		else if (substring == "/AND" || substring == " ") {
			unparsed.push("/AND");
			substring = "";
		}
	}
	if (substring.length() > 0) { // a word to search, not syntax
		unparsed.push(substring);
		substring = "";
	}
	// parsing the list of inputs into SQL, and keeping a log of parameters to later insert
	// COLLATE NOCASE is Sqlite3 specific to make search case insensitive for ASCII. For non-ASCII I'm not looking forward to, if this is even good to do
	std::string WordSelection = "VerseID IN (SELECT VerseID FROM WordVerse WHERE WordID = (SELECT WordID FROM Words WHERE Word COLLATE NOCASE = ?))";
	// I need DISTINCT here because my SQL code is cobbled together and really bad, creates tons of duplicates
	std::string searchQuery = "SELECT DISTINCT VerseID FROM WordVerse WHERE ";
	while (!unparsed.empty()) {
		if (unparsed.front() == "(") {
			searchQuery += "(";
		}
		else if (unparsed.front() == ")") {
			searchQuery += ")";
		}
		else if (unparsed.front() == "/AND") {
			searchQuery += " AND ";
		}
		else if (unparsed.front() == "/OR") {
			searchQuery += " OR ";
		}
		else {
			searchQuery += WordSelection;
			parameters.push(unparsed.front());
		}
		unparsed.pop();
	}
	searchQuery += " ORDER BY VerseID";
	//std::cout << searchQuery << std::endl;
	return searchQuery;
}
// provided a word search, returns the list of matching IDs
int BibleSearch::verseIDsFromWordSearch(std::string search, std::queue<int>& queryResults) {
	
	std::queue<std::string> parameters;
	std::string searchQuery = parseSearchIntoSqlStatement(search, parameters);

	// preparing statement with logged parameters
	rc = sqlite3_prepare_v2(searchdb, searchQuery.c_str(), -1, &stmt, &tail);
	int counter = 0;
	while (!parameters.empty()) {
		rc = sqlite3_bind_text(stmt, ++counter, parameters.front().c_str(), -1, SQLITE_STATIC);
		parameters.pop();
	}
	//executing query
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		queryResults.push(sqlite3_column_int(stmt, 0));
	}
	sqlite3_finalize(stmt);

	return 0;
}