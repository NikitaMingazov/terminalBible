/*
A program to access God's Word from the terminal

*/
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
/*
g++ -o terminalBible terminalBible.cpp -lsqlite3
*/
sqlite3_stmt* stmt; // these are declared in every function, I'm sick of them
const char* tail;
std::string sql;
int rc;

std::string getTime() {
	auto currentTimePoint = std::chrono::system_clock::now();

    // Convert the time point to a time_t (Unix timestamp)
    std::time_t currentTime = std::chrono::system_clock::to_time_t(currentTimePoint);

    // Convert the time_t to a struct tm (time structure)
    std::tm* timeInfo = std::localtime(&currentTime);

    // Format the time structure into a human-readable string
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeInfo);

    // Print the human-readable timestamp
    return buffer;
}
void logError(const std::string& message) {
	std::ofstream logfile("error.log", std::ios_base::app);
	logfile << message << " (" << getTime() << ")" << std::endl;
	logfile.close();
}
// provided one int input parameter with int output expected, runs provided sql query on provided database
int oneIntInputOneIntOutput(sqlite3* db, const char* sql, int input) {
	int output = -1;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);
	if (rc != SQLITE_OK) { // debugging
		logError("SQL err: " + std::string(sql) + " Par: " + std::to_string(input));
		return output;
	}
	sqlite3_bind_int(stmt, 1, input);
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		output = sqlite3_column_int(stmt, 0);
	}
	else if (rc != SQLITE_DONE) { // debugging
		logError("SQL exec err: " + std::string(sql) + " Par: " + std::to_string(input));
	}
	sqlite3_finalize(stmt);

	return output;
}
// provided two int input parameter with int output expected, runs provided sql query on provided database
int twoIntInputOneIntOutput(sqlite3* db, const char* sql, int input1, int input2) {
	int output = -1;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);
	if (rc != SQLITE_OK) { // debugging
		logError("SQL err: " + std::string(sql) + " Par1: " + std::to_string(input1) + " Par2: " + std::to_string(input2));
		return output;
	}
	sqlite3_bind_int(stmt, 1, input1);
	sqlite3_bind_int(stmt, 2, input2);
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		output = sqlite3_column_int(stmt, 0);
	}
	else if (rc != SQLITE_DONE) { // debugging
		logError("SQL exec err: " + std::string(sql) + " Par1: " + std::to_string(input1) + " Par2: " + std::to_string(input2));
	}
	sqlite3_finalize(stmt);

	return output;
}
// provided one int input parameter with string output expected, runs provided sql query on provided database
std::string oneIntInputOneStringOutput(sqlite3* db, const char* sql, int input) {
	std::string body;

	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);
	if (rc != SQLITE_OK) { // debugging
		logError("SQL error, Query: " + std::string(sql) + " Parameter: " + std::to_string(input));
		return "NULL";
	}
	sqlite3_bind_int(stmt, 1, input);
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		body = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)); // const unsigned char*
	}
	else if (rc != SQLITE_DONE) { // debugging
		logError("SQL execution error, Query: " + std::string(sql) + " Parameter: " + std::to_string(input));
	}
	sqlite3_finalize(stmt);

	return body;
}
// populates the given Book->ID map according to the given filename
void populateMap(std::unordered_map<std::string, int>& ident, std::string filename) {
	std::ifstream inputFile(filename); // "kjv.txt" for KJV, etc.
	if (inputFile.is_open()) {
		std::string line;
		std::string index;
		while (getline(inputFile, line)) {
			int state = 0;
			std::string cur; // part of the line
			for (int i = 0; i < line.length(); i++) {
				char c = line[i];
				if (c == '#' || c == '[') { // comment detected, finish this line
					break;
				}
				switch(state) {
				case 0: // book
					if (c != ' ') {
						cur += c;
					}
					else {
						state++;
						index = cur;
						cur = "";
					}
					break;
				case 1: // ID
					if (!isdigit(c)) { // end of ID
						break; // this break statement doesn't do what I want it to, but it only matters on faulty input data (e.g. I use "BOOK ID #com" and this extra space will be properly disregarded)
					}
					cur += c;
					break;
				}
			}
			if (cur.length() > 0) {
				ident[index] = stoi(cur); // the last thing cur was set to was ID
			}
		}
		inputFile.close();
	}
	 else {
		std::cerr << "Failed to open the file." << std::endl;
	}
}
// populates the given ID->Book map according to the given filename
void populateNameTable(std::string* ident, std::string filename) {
	std::ifstream inputFile(filename); // "kjv.txt" for KJV, etc.
	if (inputFile.is_open()) {
		std::string line;
		int i = 0;
		while (getline(inputFile, line) && i < 66) {
			std::string cur;
			bool begun = false;
			for (int i = 0; i < line.length(); i++) {
				char c = line[i];
				if (c == '#') { // comment found, skip line
					cur = "";
					break;
				}
				if (c == '[') {
					begun = true;
					continue;
				}
				if (c == ']') { // finish
					break;
				}
				if (begun) {
					cur += c;
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
}
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
// finds whether the given book name matches an entry in the map
int getBookID(std::unordered_map<std::string, int>& table, std::string book) {
	std::string search;
	bool found = false;
	for (int i = 0; i < book.length(); i++) {
		if (book[i] != ' ') {
			search += book[i];
			if (table.count(search) > 0) {
				found = true;
				break;
			}
		}
	}
	if (found) {
		return table[search];
	}
	else {
		return -1;
	}
}
// parse the index section of the reference for the requested chapter and verse(-range)
void parseIndex(const char* index, int* results) {
	int chapter = -1;
	int verse = -1;
	int limit = 1;
	
	int state = 0;
	std::string cur;
	for (int i = 0; i < strlen(index); i++) {
		char c = index[i];
		switch (state) {
		case 0: // identify chapter
			if (c == ':') {
				state++;
				chapter = std::stoi(cur);
				cur = "";
			} else {
				cur += c;
				if (i == strlen(index) - 1) {
					chapter = std::stoi(cur);
				}
			}
			break;
		case 1: // identify verse
			if (c == '-') {
				state++;
				verse = std::stoi(cur);
				cur = "";
				if (i == strlen(index) - 1) { // e.g. Job 1:15- prints until end of chapter
					limit = -1;
				}
			} else {
				cur += c;
				if (i == strlen(index) - 1) {
					verse = std::stoi(cur);
				}
			}
			break;
		case 2:  // if there is x-y, then find how many extra verses to reach y
			cur += c;
			if (i == strlen(index) - 1) {
				limit += std::stoi(cur) - verse;
			}
			break;
		}
	}
	//result = new int[3] {chapter, verse, limit};
	results[0] = chapter;
	results[1] = verse;
	results[2] = limit;
}
// prints the given line, but skips over the second+ letter in each Word
void printFirstLetter(const std::string& line) {
	bool newWord = true;

	for (char c : line) {
		if (c == '[' || c == ']') {
			continue;
		}
		if (std::isalpha(c) && newWord) {
			std::cout << c;
			newWord = false;
		} else if (!std::isalpha(c)) {
			std::cout << c;
			if (c == ' ' || c == '\t') {
				newWord = true;
			}
		}
	}
	std::cout << std::endl;
}
// turns BookID and Chapter number into equivalent ChapterID
int chapterIDFromBookIDandOffset (sqlite3* textdb, int BookID, int offset) {
	sql = "SELECT ChapterID FROM Chapters WHERE BookID = ? LIMIT 1 OFFSET ?";

	return twoIntInputOneIntOutput(textdb, sql.c_str(), BookID, offset - 1);
}
// turns ChapterID and Verse number into equivalent VerseID
int verseIDFromChapterIDandOffset (sqlite3* textdb, int ChapterID, int offset) {
	sql = "SELECT VerseID FROM Verses WHERE ChapterID = ? LIMIT 1 OFFSET ?";

	return twoIntInputOneIntOutput(textdb, sql.c_str(), ChapterID, offset - 1);
}
std::string fetchBodyFromVerseID(sqlite3* textdb, int VerseID) {
	sql = "SELECT body FROM Verses WHERE VerseID = ?";
	
	return oneIntInputOneStringOutput(textdb, sql.c_str(), VerseID);
}
int chapterIDFromVerseID(sqlite3* textdb, int VerseID) {
	sql = "SELECT ChapterID FROM Verses WHERE VerseID = ?";

	return oneIntInputOneIntOutput(textdb, sql.c_str(), VerseID);
}
int bookIDFromChapterID(sqlite3* textdb, int ChapterID) {
	sql = "SELECT BookID FROM Chapters WHERE ChapterID = ?";

	return oneIntInputOneIntOutput(textdb, sql.c_str(), ChapterID);
}
// compare starting chapter of book to current chapter
int chapterOffsetFromBookAndChapterID(sqlite3* textdb, int BookID, int ChapterID) {
	sql = "SELECT ChapterID FROM Chapters WHERE BookID = ? LIMIT 1";

	int startID = oneIntInputOneIntOutput(textdb, sql.c_str(), BookID);
	int offset = ChapterID - startID;
	return offset + 1;
}
int verseOffsetFromChapterAndVerseID(sqlite3* textdb, int ChapterID, int VerseID) {
	sql = "SELECT VerseID FROM Verses WHERE ChapterID = ? LIMIT 1";

	int startID = oneIntInputOneIntOutput(textdb, sql.c_str(), ChapterID);
	int offset = VerseID - startID;
	return offset + 1;
}
int chapterEndVerseIDFromVerseID(sqlite3* textdb, int VerseID) {
	sql = "SELECT VerseID FROM Verses WHERE ChapterID = (SELECT ChapterID FROM Verses WHERE VerseID = ?) ORDER BY VerseID DESC LIMIT 1";

	return oneIntInputOneIntOutput(textdb, sql.c_str(), VerseID);
}
int verseIDFromReference(sqlite3* textdb, std::unordered_map<std::string, int> mappings, std::tuple<std::string, int, int> ref) {
	// identify requested book
	int BookID = getBookID(mappings, std::get<0>(ref));
	if (BookID == -1) {
		std::cout << "Your book name returns no matches" << std::endl;
	}

	int ChapterID = chapterIDFromBookIDandOffset(textdb, BookID, std::get<1>(ref));

	int verseNum = 1;
	if (std::get<2>(ref) != -1) { // chapter but not verse
		verseNum = std::get<2>(ref);
	}
	int VerseID = verseIDFromChapterIDandOffset(textdb, ChapterID, verseNum);
	return VerseID;
}
// prints to the console verses according to the criteria given
int query(std::string directory, sqlite3* db, std::queue<int>& queryResults, std::tuple<std::string, int, int> ref, std::string end) {
	// load Book mapping table
	std::unordered_map<std::string, int> table;
	populateMap(table, directory+"Bible.table");

	int VerseID = verseIDFromReference(db, table, ref);
	
	if (end.length() == 1 || std::get<2>(ref) == -1) { // end == "-" or Genesis 1
		int EndID = chapterEndVerseIDFromVerseID(db, VerseID);
		for (int i = VerseID; i <= EndID; i++) {
			queryResults.push(i);
		}
	}
	else if (end.length() == 0) { // single verse
		queryResults.push(VerseID);
	}
	else {
		std::string cur;
		int chapter = -1;
		int verse = -1;
		int state = 0;
		for (int i = 1; i < end.length(); i++) {
			char c = end[i];
			switch (state) {
			case 0:
				if (c == ':') {
					chapter = stoi(cur);
					cur = "";
					state++;
				}
				else {
					cur += c;
					if (i == end.length() - 1) { // no chapter portion
						verse = stoi(cur);
					}
				}
				break;
			case 1:
				cur += c;
				if (i == end.length() - 1) { // no chapter portion
					verse = stoi(cur);
				}
				break;
			}
		}
		int EndID;
		if (chapter == -1) { // Genesis 3:1-15
			EndID = verseIDFromReference(db, table, std::make_tuple(std::get<0>(ref), std::get<1>(ref), verse));
		}
		else {
			EndID = verseIDFromReference(db, table, std::make_tuple(std::get<0>(ref), chapter, verse));
		}
		for (int i = VerseID; i <= EndID; i++) {
			queryResults.push(i);
		}
	}
	return 0;
}
// given an input string identifies it's book, chapter and verse
// return a pair of VerseID ints? potential improvement
void parseReferenceIntoTuple(std::string line, std::tuple<std::string, int, int>& reference) {
	std::string cur;
	std::string book = "";
	int chapter = -1;
	int verse = -1;
	int state = 0;
	for (int i = 0; i < line.length(); i++) {
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
// output query results to console
void printResults(std::list<std::pair<int, std::string>>& results, int queryMode) {
	std::list<std::pair<int, std::string>>::iterator it = results.begin();
	for (int i = 0; i < results.size(); i++) { // iterate through results and print each entry
		std::pair<int, std::string> verse = *it;
		int verseNum = verse.first;
		std::string body = verse.second;
		std::cout << verseNum << ": ";
		if (queryMode == 0) { // temporarily put in 2 here, later it will have it's own output function
			std::cout << body << std::endl;
		} else if (queryMode == 1) {
			printFirstLetter(body);
		}
		it++;
	}
}
// turns a word search into a SQL statement and the parameters to fill it
std::string parseSearchIntoSqlStatement(std::string search, std::queue<std::string>& parameters) {
	// splitting up the search into it's elements
	std::queue<std::string> unparsed;
	std::string substring;
	for (int i = 0; i < search.length(); i++) {
		char c = search[i];
		if (c == '(' || c == ')') {
			if (substring.length() > 0) {
				unparsed.push(substring);
				substring = "";
			}
			unparsed.push(std::string(1, c));
			continue;
		}
		if (c == '/' || c == ' ') { // new word
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
	std::string WordSelection = "VerseID IN (SELECT VerseID FROM WordVerse WHERE WordID = (SELECT WordID FROM Words WHERE Word = ?))";
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
// turns a VerseID into the reference that would return it
// this function is very slow, potential improvement is a database that has an integer for the first chapterID/verseID in each book/chapter
void fetchReferenceFromVerseID(sqlite3* textdb, int VerseID, std::string* names, std::tuple<std::string, int, int>& reference) {

	int ChapterID = chapterIDFromVerseID(textdb, VerseID);

	int BookID = bookIDFromChapterID(textdb, ChapterID);

	int chapterOffset = chapterOffsetFromBookAndChapterID(textdb, BookID, ChapterID);

	int verseOffset = verseOffsetFromChapterAndVerseID(textdb, ChapterID, VerseID);

	reference = make_tuple(names[BookID-1], chapterOffset, verseOffset);
}
// print VerseIDs to console as their respective reference and body
void printSearchResults(std::string directory, sqlite3* textdb, std::queue<int>& searchResults) {
	std::list<std::pair<std::tuple<std::string, int, int>, std::string>> toPrint;

	std::string* names = new std::string[66];
	populateNameTable(names, directory+"Bible.table");

	while (!searchResults.empty()) {
		int VerseID = searchResults.front();

		std::tuple<std::string, int, int> reference;
		fetchReferenceFromVerseID(textdb, VerseID, names, reference);

		std::string body = fetchBodyFromVerseID(textdb, VerseID);

		toPrint.push_back(std::make_pair(reference, body));
		searchResults.pop();
	}

	delete[] names;

	std::list<std::pair<std::tuple<std::string, int, int>, std::string>>::iterator it = toPrint.begin();
	for (int i = 0; i < toPrint.size(); i++) { // iterate through results and print each entry
		std::tuple<std::string, int, int> ref = (*it).first;
		std::string body = (*it).second;
		std::cout << std::get<0>(ref) << " " << std::get<1>(ref) << ":" << std::get<2>(ref) << " " << body << std::endl;
		it++;
	}
}
// provided a word search, returns the list of matching IDs
int verseIDsFromWordSearch(std::string directory, sqlite3* searchdb, std::string search, std::queue<int>& queryResults) {
	
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
void searchAndOutputToConsole(const char* constDirectory, std::string search) {
	std::string directory = std::string(constDirectory)+"/"; // to clean up filename code

	sqlite3* textdb;
	rc = sqlite3_open_v2((directory+"Bible.db").c_str(), &textdb, SQLITE_OPEN_READONLY, nullptr);
	sqlite3* searchdb;
	rc = sqlite3_open_v2((directory+"BibleSearch.db").c_str(), &searchdb, SQLITE_OPEN_READONLY, nullptr);

	std::queue<int> searchResults;
	verseIDsFromWordSearch(directory, searchdb, search, searchResults);

	printSearchResults(directory, textdb, searchResults);

	sqlite3_close(textdb);
	sqlite3_close(searchdb);
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
			int prevQueryMode = queryMode; // there has to be a more elegant solution that doesn't punish using the same flag twice in a row
			if (line[0] == 'r') { // identify flag if present
				queryMode = 0;
			}
			else if (line[0] == 'm') {
				queryMode = 1;
			}
			else if (line[0] == 's') {
				queryMode = 2;
			}
			if (queryMode == 0 || queryMode == 1) { // reading or memory

				std::string first = "";
				std::string second = "";
				std::tuple<std::string, int, int> ref; // parse the reference from the input line
				bool skipFirstChar = queryMode != prevQueryMode;
				int state = 0;
				for (int i = 0 + skipFirstChar; i < line.length(); i++) {
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
				std::string directory1 = std::string(directory)+"/";

				sqlite3* db;
				rc = sqlite3_open_v2((directory1+"Bible.db").c_str(), &db, SQLITE_OPEN_READONLY, nullptr);
				if (rc) {
					std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
					return 1;
				}

				if (query(directory1, db, results, ref, second) == 0) {
					printSearchResults(directory1, db, results);	
				}
				else {
					std::cout << "An error occured" << std::endl;
				}
				sqlite3_close(db);
			}
			else if (queryMode == 2) { // search
				std::string search;
				for(int i = 1; i < line.length(); i++) { // skip first character, because I can't figure how to remember search flag in a user-friendly manner
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