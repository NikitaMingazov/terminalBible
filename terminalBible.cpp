/*
A program to access God's Word from the terminal

*/
#include <unordered_map>
#include <cstring>
#include <string>
#include <sqlite3.h>
#include <iostream>
#include <fstream>
/*
g++ -o terminalBible terminalBible.cpp -lsqlite3
*/
// populates the given Book->ID map according to the given filename
void populate(std::unordered_map<std::string, int>& ident, std::string filename) {
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
int getBookID(std::unordered_map<std::string, int>& ident, const char* book) {
	std::string search;
	bool found = false;
	for (int i = 0; i < strlen(book); i++) {
		if (book[i] != ' ') {
			search += book[i];
			if (ident.count(search) > 0) {
				found = true;
				break;
			}
		}
	}
	if (found) {
		return ident[search];
	}
	else {
		return -1;
	}
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
int query(const char* filename, const char* book, const char* index, int readMode) {
	sqlite3* db;
	int rc = sqlite3_open((std::string(filename)+std::string(".db")).c_str(), &db);
	if (rc) {
		std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
		return 1;
	}

	std::unordered_map<std::string, int> ident;
	populate(ident, (std::string(filename)+std::string(".table")).c_str());

	int BookID = getBookID(ident, book);
	if (BookID == -1) {
		std::cout << "Your book name returns no matches" << std::endl;
		sqlite3_close(db);
		return 1;
	}

	int chapter = -1;
	int verse = -1;
	int limit = 1;

	int state = 0;
	std::string cur; // parse the index section of the query for the requested chapter and verse(-range)
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
	} else {
		selectQuery = "SELECT body FROM Verses WHERE ChapterID = ? LIMIT ? OFFSET ?";
	}

	rc = sqlite3_prepare_v2(db, selectQuery.c_str(), -1, &stmt, &tail);
	rc = sqlite3_bind_int(stmt, 1, ChapterID);
	//the following should be put in an if statement? but no errors are occuring at present
	rc = sqlite3_bind_int(stmt, 2, limit);
	rc = sqlite3_bind_int(stmt, 3, verse - 1);

	int increment = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		std::cout << verse + increment++ << ": "; // print verse number
		const unsigned char* line = sqlite3_column_text(stmt, 0); // print verse body
		if (readMode == 0) {
			std::cout << line << std::endl;
		} else if (readMode == 1) {
			printFirstLetter(reinterpret_cast<const char*>(line));
		}
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return 0;
}

int main(int argc, char **argv) { // notes: create a copy mode for referencing, and remember previous book/mode queried, x- until end of chapter
	int readMode = 0;
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
			int state = 0;
			std::string cur;
			std::string book; // parsing input
			for (int i = 0; i < line.length(); i++) {
				char c = line[i];
				if (c == ' ') { // ignore spaces
					continue;
				}
				if (i == 0) { // identify flag if present
					if (c == 'r') {
						readMode = 0;
						continue;
					}
					else if (c == 'm') {
						readMode = 1;
						continue;
					}
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
			if (query(filename, book.c_str(), cur.c_str(),readMode) != 0) {
				std::cout << "An error occured" << std::endl;
			}
		}
	}
	else {
		std::cout << "Missing filename. Enter your primary \".db\" but without the file extension" << std::endl;
	}
}