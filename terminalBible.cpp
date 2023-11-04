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
/*
g++ -o terminalBible terminalBible.cpp -lsqlite3
*/
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
				if (c == '#') { // comment detected, finish this line
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
// finds whether the given book name matches an entry in the map
int getBookID(std::unordered_map<std::string, int>& table, const char* book) {
	std::string search;
	bool found = false;
	for (int i = 0; i < strlen(book); i++) {
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
// prints to the console verses according to the criteria given
int query(const char* constFilename, std::list<std::pair<int, std::string>>& queryResults, const char* book, const char* index) {
	sqlite3* db;
	std::string filename = std::string(constFilename); // to clean up filename code
	int rc = sqlite3_open_v2((filename+".db").c_str(), &db, SQLITE_OPEN_READONLY, nullptr);
	if (rc) {
		std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
		return 1;
	}
	// load Book mapping table
	std::unordered_map<std::string, int> table;
	populateMap(table, (filename+".table").c_str());
	// identify requested book
	int BookID = getBookID(table, book);
	if (BookID == -1) {
		std::cout << "Your book name returns no matches" << std::endl;
		sqlite3_close(db);
		return 1;
	}
	// parse the given index into usable values
	int* results = new int[3];
	parseIndex(index, results);
	int chapter = results[0];
	int verse = results[1];
	int limit = results[2];
	delete results;

	sqlite3_stmt* stmt;
	const char* tail;

	std::string queryStr;
	queryStr = "SELECT ChapterID FROM Chapters WHERE BookID = ? LIMIT 1 OFFSET ?";
	rc = sqlite3_prepare_v2(db, queryStr.c_str(), -1, &stmt, &tail);

	rc = sqlite3_bind_int(stmt, 1, BookID);
	rc = sqlite3_bind_int(stmt, 2, chapter - 1);

	int ChapterID = -1;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		ChapterID = sqlite3_column_int(stmt, 0);
	}
	sqlite3_finalize(stmt);

	std::string selectQuery;
	if (verse == -1) { // if Book Chapter but no verse, then whole chapter is returned
		verse = 1;
		selectQuery = "SELECT body FROM Verses WHERE ChapterID = ?";
	} 
	else if (limit == -1) { // "verse-"
		selectQuery = "SELECT body FROM Verses WHERE ChapterID = ? LIMIT 999999 OFFSET ?";
	}
	else { // "verse-verse2"
		selectQuery = "SELECT body FROM Verses WHERE ChapterID = ? LIMIT ? OFFSET ?";
	}

	rc = sqlite3_prepare_v2(db, selectQuery.c_str(), -1, &stmt, &tail);
	rc = sqlite3_bind_int(stmt, 1, ChapterID);
	//the following should be put in an if statement for verse == -1's sake? but no errors are occuring at present
	if (limit == -1) {
		rc = sqlite3_bind_int(stmt, 2, verse - 1);
	}
	else {
		rc = sqlite3_bind_int(stmt, 2, limit);
		rc = sqlite3_bind_int(stmt, 3, verse - 1);
	}
	// saving query results into the passed list pointer to use in the function that called this
	int increment = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const unsigned char* body = sqlite3_column_text(stmt, 0);
		queryResults.push_back(std::pair<int, std::string>(verse+increment++, reinterpret_cast<const char*>(body)));
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	if (increment == 0) {
		std::cout << "Book identified but no results from your query" << std::endl;	
		return 1;
	}
	return 0;
}
// given an input string determines where the book and the index are
void parseReference(std::string line, std::pair<std::string, std::string>& reference, bool flagChange) {
	std::string cur;
	std::string book;
	int state = 0;
	for (int i = 0 + flagChange; i < line.length(); i++) {
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
			}
			cur += c;
			break;
		case 1:
			cur += c; // chapter onwards 
			break;
		}
	}
	reference = std::make_pair(book, cur);
}
// output query results to console
void printResults(std::list<std::pair<int, std::string>>& results, int queryMode) {
	std::list<std::pair<int, std::string>>::iterator it = results.begin();
	for (int i = 0; i < results.size(); i++) { // iterate through results and print each entry
		std::pair<int, std::string> verse = *it;
		int verseNum = verse.first;
		std::string body = verse.second;
		std::cout << verseNum << ": ";
		if (queryMode == 0 || 2) { // temporarily put in 2 here
			std::cout << body << std::endl;
		} else if (queryMode == 1) {
			printFirstLetter(body);
		}
		it++;
	}
}
int searchBible(const char* constFilename, std::list<std::pair<int, std::string>>& queryResults, std::string search) {
	sqlite3* textdb;
	std::string filename = std::string(constFilename); // to clean up filename code
	int rc = sqlite3_open_v2((filename+".db").c_str(), &textdb, SQLITE_OPEN_READONLY, nullptr);

	sqlite3* searchdb;
	rc = sqlite3_open_v2((filename+"Search.db").c_str(), &searchdb, SQLITE_OPEN_READONLY, nullptr);

	sqlite3_stmt* stmt;
	const char* tail;
	std::string sql;
	int WordID = -1;
	sql = "SELECT WordID FROM Words WHERE Word = ?";
	rc = sqlite3_prepare_v2(searchdb, sql.c_str(), -1, &stmt, &tail);
	rc = sqlite3_bind_text(stmt, 1, search.c_str(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		WordID = sqlite3_column_int(stmt, 0);
	}
	sqlite3_finalize(stmt);

	std::cout << WordID << std::endl;
	std::queue<int> verseIDs;
	sql = "SELECT VerseID FROM WordVerse WHERE WordID = ?";
	rc = sqlite3_prepare_v2(searchdb, sql.c_str(), -1, &stmt, &tail);
	sqlite3_bind_int(stmt, 1, WordID);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		verseIDs.push(sqlite3_column_int(stmt, 0));
	}
	sqlite3_finalize(stmt);

	while (!verseIDs.empty()) {
		int VerseID = verseIDs.front();
		verseIDs.pop();
		sql = "SELECT body FROM Verses WHERE VerseID = ?";
		rc = sqlite3_prepare_v2(textdb, sql.c_str(), -1, &stmt, &tail);
		sqlite3_bind_int(stmt, 1, VerseID);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			const unsigned char* body = sqlite3_column_text(stmt, 0);
			queryResults.push_back(std::pair<int, std::string>(VerseID, reinterpret_cast<const char*>(body)));
			//queryResults.push_back(std::pair<int, std::string>(VerseID, reinterpret_cast<const char*>(body)));
		}
		sqlite3_finalize(stmt);
	}


	sqlite3_close(textdb);
	sqlite3_close(searchdb);
	return 0;
}
int main(int argc, char **argv) { // notes: create a copy mode for referencing, and remember previous book/mode queried, x- until end of chapter
	int queryMode = 0;
	const char* filename;
	if (argc > 1) {
		filename = argv[1];

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
				std::pair<std::string, std::string> ref; // parse the reference from the input line
				parseReference(line, ref, queryMode != prevQueryMode);

				std::list<std::pair<int, std::string>> results; // results of query stored here
				if (query(filename, results, ref.first.c_str(), ref.second.c_str()) == 0) {
					printResults(results, queryMode);	
				}
				else {
					std::cout << "An error occured" << std::endl;
				}
			}
			else if (queryMode == 2) { // search
				std::string search;
				for(int i = 1; i < line.length(); i++) {
					search += line[i];
				}
				std::list<std::pair<int, std::string>> results; // results of query stored here
				searchBible(filename, results, search);
				printResults(results, queryMode);
			}
		}
	}
	else {
		std::cout << "Missing filename. Enter your primary \".db\" but without the file extension" << std::endl;
	}
}