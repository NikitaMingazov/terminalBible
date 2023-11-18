
#include "BibleText.h"

#include <iostream>   // for std::cerr, std::cout
#include <fstream>    // for file I/O
#include <chrono>     // for std::chrono::system_clock
#include <ctime>      // for std::time_t, std::tm
#include <cctype>     // for std::isalpha
#include <list>       // for std::list

std::string BibleText::getTime() {
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
void BibleText::logError(const std::string& message) {
	std::ofstream logfile("error.log", std::ios_base::app);
	logfile << message << " (" << getTime() << ")" << std::endl;
	logfile.close();
}
// provided one int input parameter with int output expected, runs provided sql query on provided database
int BibleText::oneIntInputOneIntOutput(const char* sql, int input) {
	int output = -1;

	rc = sqlite3_prepare_v2(textdb, sql, -1, &stmt, &tail);
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
int BibleText::twoIntInputOneIntOutput(const char* sql, int input1, int input2) {
	int output = -1;

	rc = sqlite3_prepare_v2(textdb, sql, -1, &stmt, &tail);
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
std::string BibleText::oneIntInputOneStringOutput(const char* sql, int input) {
	std::string body;

	int rc = sqlite3_prepare_v2(textdb, sql, -1, &stmt, &tail);
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
// finds whether the given book name matches an entry in the map
int BibleText::getBookID(std::string book) {
	std::string search;
	bool found = false;
	for (unsigned int i = 0; i < book.length(); i++) {
		if (book[i] != ' ') {
			search += book[i];
			if (mappings.count(search) > 0) {
				found = true;
				break;
			}
		}
	}
	if (found) {
		return mappings[search];
	}
	else {
		return -1;
	}
}
// turns BookID and Chapter number into equivalent ChapterID
int BibleText::chapterIDFromBookIDandOffset (int BookID, int offset) {
	sql = "SELECT ChapterID FROM Chapters WHERE BookID = ? LIMIT 1 OFFSET ?";

	return twoIntInputOneIntOutput(sql.c_str(), BookID, offset - 1);
}
// turns ChapterID and Verse number into equivalent VerseID
int BibleText::verseIDFromChapterIDandOffset (int ChapterID, int offset) {
	sql = "SELECT VerseID FROM Verses WHERE ChapterID = ? LIMIT 1 OFFSET ?";

	return twoIntInputOneIntOutput(sql.c_str(), ChapterID, offset - 1);
}
std::string BibleText::fetchBodyFromVerseID(int VerseID) {
	sql = "SELECT body FROM Verses WHERE VerseID = ?";
	
	return oneIntInputOneStringOutput(sql.c_str(), VerseID);
}
int BibleText::chapterIDFromVerseID(int VerseID) {
	sql = "SELECT ChapterID FROM Verses WHERE VerseID = ?";

	return oneIntInputOneIntOutput(sql.c_str(), VerseID);
}
int BibleText::bookIDFromChapterID(int ChapterID) {
	sql = "SELECT BookID FROM Chapters WHERE ChapterID = ?";

	return oneIntInputOneIntOutput(sql.c_str(), ChapterID);
}
// compare starting chapter of book to current chapter
int BibleText::chapterOffsetFromBookAndChapterID(int BookID, int ChapterID) {
	sql = "SELECT ChapterID FROM Chapters WHERE BookID = ? LIMIT 1";

	int startID = oneIntInputOneIntOutput(sql.c_str(), BookID);
	int offset = ChapterID - startID;
	return offset + 1;
}
int BibleText::verseOffsetFromChapterAndVerseID(int ChapterID, int VerseID) {
	sql = "SELECT VerseID FROM Verses WHERE ChapterID = ? LIMIT 1";

	int startID = oneIntInputOneIntOutput(sql.c_str(), ChapterID);
	int offset = VerseID - startID;
	return offset + 1;
}
int BibleText::chapterEndVerseIDFromVerseID(int VerseID) {
	sql = "SELECT VerseID FROM Verses WHERE ChapterID = (SELECT ChapterID FROM Verses WHERE VerseID = ?) ORDER BY VerseID DESC LIMIT 1";

	return oneIntInputOneIntOutput(sql.c_str(), VerseID);
}
int BibleText::verseIDFromReference(std::tuple<std::string, int, int> ref) {
	// identify requested book
	int BookID = getBookID(std::get<0>(ref));
	if (BookID == -1) {
		std::cout << "Your book name returns no matches" << std::endl;
	}

	int ChapterID = chapterIDFromBookIDandOffset(BookID, std::get<1>(ref));

	int verseNum = 1;
	if (std::get<2>(ref) != -1) { // chapter but not verse
		verseNum = std::get<2>(ref);
	}
	int VerseID = verseIDFromChapterIDandOffset(ChapterID, verseNum);
	return VerseID;
}
// turns a VerseID into the reference that would return it
void BibleText::fetchReferenceFromVerseID(int VerseID, std::tuple<std::string, int, int>& reference) {

	int ChapterID = chapterIDFromVerseID(VerseID);

	int BookID = bookIDFromChapterID(ChapterID);

	int chapterOffset = chapterOffsetFromBookAndChapterID(BookID, ChapterID);

	int verseOffset = verseOffsetFromChapterAndVerseID(ChapterID, VerseID);

	reference = make_tuple(names[BookID-1], chapterOffset, verseOffset);
}
// populates the given Book->ID map according to the given filename
void BibleText::populateMap(std::string filename) {
	std::ifstream inputFile(filename); // "kjv.txt" for KJV, etc.
	if (inputFile.is_open()) {
		std::string line;
		std::string index;
		while (getline(inputFile, line)) {
			int state = 0;
			std::string cur; // part of the line
			for (unsigned int i = 0; i < line.length(); i++) {
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
				mappings[index] = stoi(cur); // the last thing cur was set to was ID
			}
		}
		inputFile.close();
	}
	 else {
		std::cerr << "Failed to open the file." << std::endl;
	}
}
// populates the given ID->Book map according to the given filename
void BibleText::populateNameTable(std::string filename) {
	std::ifstream inputFile(filename); // "kjv.txt" for KJV, etc.
	if (inputFile.is_open()) {
		std::string line;
		int i = 0;
		while (getline(inputFile, line) && i < 66) {
			std::string cur;
			bool begun = false;
			for (unsigned int i = 0; i < line.length(); i++) {
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
				names[i++] = cur; // the last thing cur was set to was ID
			}
		}
		inputFile.close();
	}
	 else {
		std::cerr << "Failed to open the file." << std::endl;
	}
}

BibleText::BibleText(const char* inputDir) {
	std::string directory = std::string(inputDir)+"/";

	rc = sqlite3_open_v2((directory+"Bible.db").c_str(), &textdb, SQLITE_OPEN_READONLY, nullptr);
	if (rc) {
		std::cerr << "Can't open database: " << sqlite3_errmsg(textdb) << std::endl;
		return;
	}
	populateMap(directory+"Bible.table");
	names = new std::string[66];
	populateNameTable(directory+"Bible.table");
}

BibleText::~BibleText() {
	delete[] names;
	sqlite3_close(textdb);
}
// prints to the console verses according to the criteria given
int BibleText::query(std::queue<int>& queryResults, std::tuple<std::string, int, int> ref, std::string end) {

	int VerseID = verseIDFromReference(ref);
	
	if (end.length() == 1 || std::get<2>(ref) == -1) { // end == "-" or Genesis 1
		int EndID = chapterEndVerseIDFromVerseID(VerseID);
		for (int i = VerseID; i <= EndID; i++) {
			queryResults.push(i);
		}
	}
	else if (end.length() == 0) { // single verse
		queryResults.push(VerseID);
	} 
	else { // specified range of verses
		std::string cur;
		int chapter = -1;
		int verse = -1;
		int state = 0;
		for (unsigned int i = 1; i < end.length(); i++) {
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
						chapter = std::get<1>(ref);
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
		int EndID = verseIDFromReference(std::make_tuple(std::get<0>(ref), chapter, verse));
		for (int i = VerseID; i <= EndID; i++) {
			queryResults.push(i);
		}
	}
	return 0;
}
// turns VerseID into reference and body in one tuple
void BibleText::retrieveVerseFromID(int VerseID, std::tuple<std::string, int, int, std::string>& verse) {
	std::tuple<std::string, int, int> reference;
	fetchReferenceFromVerseID(VerseID, reference);

	std::string body = fetchBodyFromVerseID(VerseID);
	verse = tuple_cat(reference, make_tuple(body));
}