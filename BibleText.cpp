
#include "BibleText.h"

#include <iostream>   // for std::cerr, std::cout
#include <fstream>    // for file I/O
#include <chrono>     // for std::chrono::system_clock
#include <ctime>      // for std::time_t, std::tm
#include <cctype>     // for std::isalpha
#include <list>       // for std::list
#include <regex>
#include <tuple>

inline std::string getTime() {
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
inline void logError(const std::string& message) {
	std::ofstream logfile("error.log", std::ios_base::app);
	logfile << message << " (" << getTime() << ")" << std::endl;
	logfile.close();
}
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
int BibleText::verseIDFromReference(std::tuple<int, int, int> ref) {
	// identify requested book
	/*int BookID = getBookID(names[std::get<0>(ref)-1]);
	if (BookID == -1) {
		std::cout << "Your book name returns no matches" << std::endl;
	}*/
	int BookID = std::get<0>(ref);

	int ChapterID = chapterIDFromBookIDandOffset(BookID, std::get<1>(ref));

	int verseNum = 1;
	if (std::get<2>(ref) != -1) { // chapter but not verse
		verseNum = std::get<2>(ref);
	}
	int VerseID = verseIDFromChapterIDandOffset(ChapterID, verseNum);
	return VerseID;
}
// turns a VerseID into the reference that would return it
void BibleText::fetchReferenceFromVerseID(int VerseID, std::tuple<int, int, int>& reference) {

	int ChapterID = chapterIDFromVerseID(VerseID);

	int BookID = bookIDFromChapterID(ChapterID);

	int chapterOffset = chapterOffsetFromBookAndChapterID(BookID, ChapterID);

	int verseOffset = verseOffsetFromChapterAndVerseID(ChapterID, VerseID);

	reference = std::make_tuple(BookID, chapterOffset, verseOffset);
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
bool regexMatch(std::string input, std::string pattern) {
	return std::regex_match(input, std::regex(pattern));
}
void BibleText::handleFSMOutput(std::queue<int>& queryResults, std::tuple<int, int, int> ref, int &rangeStart) {
	int VerseID = verseIDFromReference(ref);
	if (VerseID == -1) {
		std::cout << "No match found for " << names[std::get<0>(ref)-1] << " " << std::get<1>(ref) << ":" << std::get<2>(ref) << std::endl;
		return;
	}
	if (rangeStart != -1) {
		for (int i = rangeStart; i < VerseID; i++) { // do while?
			queryResults.push(i);
		}
		rangeStart = -1;
	}
	queryResults.push(VerseID);
	if (std::get<2>(ref) == -1) { // whole chapter
		int EndID = chapterEndVerseIDFromVerseID(VerseID);
		for (int i = VerseID + 1; i < EndID; i++) {
			queryResults.push(i);
		}
	}
}
// prints to the console verses according to the criteria given
int BibleText::query(std::queue<int>& queryResults, std::string line) {

	int rangeStart;

	std::string output;
	int book = -1;
	int chapter = -1;
	int verse = -1;
	int state = 0;
	for (size_t i = 0; i < line.length();) {
		std::string input = readUtf8Character(line, i);
		if (input == " ") {
			continue;
		}
		//std::cout << "State: " << state << " Input: " << input << " Output: " << output << std::endl;
		switch(state) { // FSM
		case 0:
			output += input;
			state = 1;
			break;
		case 1:
			if (regexMatch(input, "[1-9]")) {
				state = 2;
				book = getBookID(output);
				output = input; //entry to new state
				chapter = -1;
				verse = -1;
			}
			else if (regexMatch(input, ":")) {
				state = 3;
				book = getBookID(output);
				output = "";
				chapter = 1;
			}
			else {
				output += input;
			}
			break;
		case 2:
			chapter:
			if (regexMatch(input, "[0-9]")) {
				output += input;
			}
			else if (input == ":") {
				state = 3;
				chapter = stoi(output);
				output = "";
			}
			else if (input == "-") {
				state = 8;
				chapter = stoi(output);
				output = "";

				rangeStart = verseIDFromReference(std::make_tuple(book,chapter,verse));
			}
			else if (input == ";") {
				state = 10;
				chapter = stoi(output);
				output = "";

				handleFSMOutput(queryResults, std::make_tuple(book,chapter,verse), rangeStart);
			}
			else {
				goto error;
			}
			break;
		case 3:
			if (regexMatch(input, "[1-9]")) {
				state = 4;
				output += input;
			}
			break;
		case 4:
			if (regexMatch(input, "[0-9]")) {
				output += input;
			}
			else if (regexMatch(input, "[-]")) {
				state = 5;
				verse = stoi(output);
				output = "";

				rangeStart = verseIDFromReference(std::make_tuple(book,chapter,verse));
			}
			else if (regexMatch(input, "[,]")) {
				state = 7;
				verse = stoi(output);
				output = "";

				handleFSMOutput(queryResults, std::make_tuple(book,chapter,verse), rangeStart);
				verse = -1;
			}
			else if (regexMatch(input, "[;]")) {
				state = 10;
				verse = stoi(output);
				output = "";

				handleFSMOutput(queryResults, std::make_tuple(book,chapter,verse), rangeStart);
				chapter = -1;
				verse = -1;
			}
			break;
		case 5:
			if (regexMatch(input, "[1-9]")) {
				state = 6;
				output += input;
			}
			break;
		case 6:
			if (regexMatch(input, "[0-9]")) {
				output += input;
			}
			else if (regexMatch(input, "[,]")) {
				state = 7;
				verse = stoi(output);
				output = "";
				
				handleFSMOutput(queryResults, std::make_tuple(book,chapter,verse), rangeStart);
				verse = -1;
			}
			else if (regexMatch(input, "[;]")) {
				state = 10;
				verse = stoi(output);
				output = "";

				handleFSMOutput(queryResults, std::make_tuple(book,chapter,verse), rangeStart);
				chapter = -1;
				verse = -1;
			}
			else if (regexMatch(input, "[:]")) {
				state = 11;
				chapter = stoi(output);
				output = "";
			}
			break;
		case 7:
			if (regexMatch(input, "[1-9]")) {
				state = 4;
				output += input;
			}
			break;
		case 8:
			if (regexMatch(input, "[1-9]")) {
				state = 9;
				output += input;
			}
			break;
		case 9:
			if (regexMatch(input, "[0-9]")) {
				output += input;
			}
			else if (regexMatch(input, "[:]")) {
				state = 3;
				chapter = stoi(output);
				output = "";
			}
			else if (regexMatch(input, "[;]")) {
				state = 10;
				chapter = stoi(output);
				output = "";

				handleFSMOutput(queryResults, std::make_tuple(book,chapter,verse), rangeStart);
				chapter = -1;
				verse = -1;
			}
			break;
		case 10:
			// temporarily disabled choosing another book
			chapter = -1;
			verse = -1;
			rangeStart = -1;
			state = 2;
			goto chapter;
			break;
		case 11:
			if (regexMatch(input, "[0-9]")) {
				output += input;
			}
			if (regexMatch(input, "[,]")) {
				state = 7;
				verse = stoi(output);
				output = "";

				handleFSMOutput(queryResults, std::make_tuple(book,chapter,verse), rangeStart);
				verse = -1;
			}
			else if (regexMatch(input, "[;]")) {
				state = 10;
				verse = stoi(output);
				output = "";

				handleFSMOutput(queryResults, std::make_tuple(book,chapter,verse), rangeStart);
				chapter = -1;
				verse = -1;
			}
			break;
		}
	}
	switch (state) {
	case 2:
	case 9:
		chapter = stoi(output);
		handleFSMOutput(queryResults, std::make_tuple(book,chapter,verse), rangeStart);
		break;
	case 4:
	case 11:
		verse = stoi(output);
		handleFSMOutput(queryResults, std::make_tuple(book,chapter,verse), rangeStart);
		break;
	case 6:
		if (chapter == -1) {
			chapter = stoi(output);
		}
		else {
			verse = stoi(output);
		}
		handleFSMOutput(queryResults, std::make_tuple(book,chapter,verse), rangeStart);
		break;
	}
	if (false) {
		error:
		std::cout << "Error in FSM execution" << std::endl;
		return 1;
	}
	return 0;
}
// turns VerseID into reference and body in one tuple
void BibleText::retrieveVerseFromID(int VerseID, std::tuple<std::string, int, int, std::string>& verse) {
	std::tuple<int, int, int> reference;
	fetchReferenceFromVerseID(VerseID, reference);

	std::string bookName = names[std::get<0>(reference)-1];

	std::string body = fetchBodyFromVerseID(VerseID);
	//verse = tuple_cat(reference, make_tuple(body));
	verse = std::make_tuple(bookName, std::get<1>(reference), std::get<2>(reference), body);
}