/*
This is a script that creates the files used in my program
It takes a .txt and creates a "filename".db, a "filename".table(just plaintext) and "filename"Search.db
*/
#include <iostream>
#include <fstream>
#include <cmath>
#include <sqlite3.h>
#include <cstring>
#include <unordered_map>
#include <list>
#include <stack>
#include <utility>
#include <filesystem>

/*
g++ -o updateSQL.exe updateSQL.cpp -lsqlite3
*/
using namespace std;
namespace fs = std::filesystem;  // Define an alias for the namespace

// alter depending on input format and/or language
// discerns whether the input character is part of a Word
bool isACharacter(string c) {
	if (isalpha(c[0])) { // (KJV)
		return true;
	}
	if (isdigit(c[0])) { // Song of Solomon chapter 7 has issues in Synodal
		return false;
	}
	string nonChars[] = {" ","(",")",",",".",":",";","-","?","!","„","\""}; // (Synodal)
	for (int i = 0; i < size(nonChars); ++i)
	{
		if (c == nonChars[i]) {
			return false;
		}
	}
	return true;
}
// checks if the character is one to exclude from the BibleText database
bool isUndesiredCharacter(string c) {
	if (c == u8"¶") { // pilcrow
		return true;
	}
	if (c == u8"‹") { // red letter "quotations"
		return true;
	}
	if (c == u8"›") {
		return true;
	}
	return false;
}
int getCodePointSize(const std::string& utf8String, size_t index) {
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
std::string readUtf8Character(const std::string& utf8String, size_t& index) {
	std::string character;

	int length = getCodePointSize(utf8String, index);
	for (size_t i = 0; i < length; i++) {
		character += utf8String[index];
		index++;
	}
	return character;
}
int updateSearch(const char* constDirectory) {
	sqlite3* tdb; // input Bible text database
	sqlite3* sdb; // output search index database
	sqlite3_stmt* stmt;
	const char* sql;

	if (!fs::exists(fs::path(".") / constDirectory)) {
		cout << "Folder does not exist" << endl;
		return 1;
	}
	string directory = string(constDirectory)+"/"; // to clean up filename code

	// create the search db
	int rc = sqlite3_open((directory+"BibleSearch.db").c_str(), &sdb);

	sql = "DROP TABLE IF EXISTS Words";
	rc = sqlite3_exec(sdb, sql, 0, 0, 0);
	sql = "CREATE TABLE Words (WordID INT, Word VARCHAR(40), PRIMARY KEY (WordID))";
	rc = sqlite3_exec(sdb, sql, 0, 0, 0);
	sql = "CREATE INDEX Words_Word_Index ON Words (Word)"; // creating indexes for efficiency
	rc = sqlite3_exec(sdb, sql, 0, 0, 0);

	sql = "DROP TABLE IF EXISTS WordVerse";
	rc = sqlite3_exec(sdb, sql, 0, 0, 0);
	sql = "CREATE TABLE WordVerse (WordID INT, VerseID INT, PRIMARY KEY (WordID, VerseID), FOREIGN KEY (WordID) REFERENCES Words(WordID))";
	rc = sqlite3_exec(sdb, sql, 0, 0, 0);
	sql = "CREATE INDEX WordVerse_WordID_Index ON WordVerse (WordID)"; // creating indexes for efficiency
	rc = sqlite3_exec(sdb, sql, 0, 0, 0);
	sql = "CREATE INDEX WordVerse_VerseID_Index ON WordVerse (VerseID)"; // creating indexes for efficiency
	rc = sqlite3_exec(sdb, sql, 0, 0, 0);

	// open the Bible text as read-only
	rc = sqlite3_open_v2((directory+"Bible.db").c_str(), &tdb, SQLITE_OPEN_READONLY, nullptr);
	sql = "SELECT VerseID, body FROM Verses";
	stack<pair<int, string>> verses;
	if (sqlite3_prepare_v2(tdb, sql, -1, &stmt, 0) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			int verseID = sqlite3_column_int(stmt, 0);
			const char* body = (const char*)sqlite3_column_text(stmt, 1);

			verses.push(make_pair(verseID,body));
		}
	}
	sqlite3_close(tdb);

	int wordID = 0;  // Initialize wordID

	while (!verses.empty()) {
	    pair<int, string> verse = verses.top();
	    verses.pop();

	    int verseID = verse.first;
	    string body = verse.second;

	    string word = "";
	    for (size_t i = 0; i < body.length();) {
	        string c = readUtf8Character(body, i);
	        if (isACharacter(c)) { // Building a word
	            word += c;
	            if (i == body.length() - 1) { // If it's the end of the string, process the word
	                goto insert;
	            }
	        } else if (word.length() > 0) { // Word identified
	            insert:
	            // Check if the word exists in the Words table
	            sqlite3_stmt* targetStmt;
	            const char* targetSql = "SELECT WordID FROM Words WHERE Word = ?";
	            if (sqlite3_prepare_v2(sdb, targetSql, -1, &targetStmt, 0) == SQLITE_OK) {
	                sqlite3_bind_text(targetStmt, 1, word.c_str(), -1, SQLITE_STATIC);
	                int wordForeignKey = -1; // Initialize the foreign key

	                if (sqlite3_step(targetStmt) == SQLITE_ROW) {
	                    // The word exists in the Words table
	                    wordForeignKey = sqlite3_column_int(targetStmt, 0);
	                } else {
	                    // The word doesn't exist, so insert it into the Words table
	                    const char* insertWordSql = "INSERT INTO Words (WordID, Word) VALUES (?, ?)";
	                    sqlite3_stmt* insertWordStmt;
	                    if (sqlite3_prepare_v2(sdb, insertWordSql, -1, &insertWordStmt, 0) == SQLITE_OK) {
	                    	sqlite3_bind_int(insertWordStmt, 1, ++wordID);
	                        sqlite3_bind_text(insertWordStmt, 2, word.c_str(), -1, SQLITE_STATIC);
	                        if (sqlite3_step(insertWordStmt) == SQLITE_DONE) {
	                            wordForeignKey = sqlite3_last_insert_rowid(sdb);
	                            if (wordForeignKey == 0) {
	                            	std::cout << "Word: " << word << " GlobalID:" << wordID << std::endl;
	                            }
	                        }
	                        sqlite3_finalize(insertWordStmt);
	                    }
	                }
	                sqlite3_finalize(targetStmt);

	                // Insert the Word-Verse pairing with the correct foreign key
	                const char* insertWordVerseSql = "INSERT INTO WordVerse (WordID, VerseID) VALUES (?, ?)";
	                sqlite3_stmt* insertWordVerseStmt;
	                if (sqlite3_prepare_v2(sdb, insertWordVerseSql, -1, &insertWordVerseStmt, 0) == SQLITE_OK) {
	                    sqlite3_bind_int(insertWordVerseStmt, 1, wordForeignKey);
	                    sqlite3_bind_int(insertWordVerseStmt, 2, verseID);
	                    sqlite3_step(insertWordVerseStmt);
	                    sqlite3_finalize(insertWordVerseStmt);
	                }
	                //cout << "Word: " << word << " WordID: " << wordForeignKey << " VerseID: " << verseID << " Verse body: " << body << endl;
	                word = ""; // Clearing once finished
	            }
	        }
	    }
	}

	sqlite3_close(sdb);
	return 0;
}
// find minimal unique starting substring for the set of book names given
// algorithm explained:
// each Book title goes into a bucket with it's respective character which is added to a recursive stack
// look at the top of the stack, if the bucket is unique it is done. if not, progressively add characters to each item in the bucket, and place it respectively
// sometimes they'll get placed into the same bucket e.g. {Phil, Philippians, Philemon} occurs (of course, the two booknames are indexes stored in int for typing reasons
// this logic is implemented in a map pointing to lists
int abbreviate(string books[], string abbreviations[]) {
	stack<list<string>*> open; // open buckets
	unordered_map<string, list<string>*> map; // used to map buckets to their substring

	for (int i = 0; i < 66; i++) { // setting up initial buckets
		size_t zero = 0;
		string firstLetter = readUtf8Character(books[i], zero);
		if (map.count(firstLetter) == 0) {
			list<string> bucket;
			bucket.push_back(firstLetter);
			map[firstLetter] = new list<string>(bucket); // content of bucket e.g. "M" for Malachi
			open.push(map[firstLetter]);
		}
		map[firstLetter]->push_back(to_string(i)); // add index of book to it's bucket e.g. "39" for Malachi
	}
	while (!open.empty()) { // splitting buckets until each item inside is unique
		list<string> bucket = *(open.top());
		open.pop();

		list<string>::iterator it = bucket.begin();
		if (bucket.size() == 2) { // unique substring
			string abbrev = *it;
			it++;
			abbreviations[stoi(*it)] = abbrev; // buckets are e.g. {"Mal","39"} for the final result for Malachi
			delete map[abbrev];
		}
		else { // more splits required
			string base = *it; // current substring and it's length
			int n = base.length();

			string temp;
			for (int i = 1; i < bucket.size(); i++) { // start from position 1
				it++; // next()
				string index = *it;
				int k = 0;
				string book = books[stoi(index)];
				for (size_t j = 0; j < book.length();) { // iterate through the book referenced in the bucket
					string c = readUtf8Character(book, j);
					if (c == " ") {
						k++;
						continue; // skip over spaces
					}
					if (j - k > n) { // has reached the next character to build upon the substring
						temp = base + c; // create new substring
						break;
					}
				}
				if (map.count(temp) == 0) { // substring isn't mapped yet, create a new bucket for it
					list<string> bucket;
					bucket.push_back(temp);
					map[temp] = new list<string>(bucket);
					open.push(map[temp]);
				}
				map[temp]->push_back(index); // add Book identifier to respective bucket
			}
		}
	}
	return 0;
}
// save book mappings to a file
int writeTable(string filename, string abbreviations[], string books[]) {
	ofstream outputFile(filename);

	if (outputFile.is_open()) {
		outputFile << "# This is a comment" << std::endl;
		outputFile << "# Line stops being parsed after \"#\" has been seen" << std::endl;
		outputFile << "# [bookname] is what will be printed out for word searches" << std::endl;
		for (int i = 0; i < 66; i++) {
			outputFile << abbreviations[i] << " " << (i+1) << " [" << books[i] << "]" << std::endl;
		}
		outputFile << "# Add your own mappings below" << std::endl;
		outputFile << "# Php 50 #Philippians" << std::endl;
		outputFile << "# Php 57 #Philemon" << std::endl;
		outputFile << "# Jdg 7 #Judges" << std::endl;

		outputFile.close();
	} else {
		std::cerr << "Unable to open the file for writing." << std::endl;
	}
	return 0;
}
// create a database and bookID table based off an input Bible
int updateText(const char* source, const char* constDirectory) {
	sqlite3* db;
	sqlite3_stmt* stmt;
	const char* sql;

	if (!fs::exists(fs::path(".") / constDirectory)) {
		//fs::remove_all(fs::path(".") / constDirectory);
		fs::create_directory(fs::path(".") / constDirectory);
	}

	string directory = string(constDirectory) + "/"; // to clean up filename code

	int rc = sqlite3_open((directory+"Bible.db").c_str(), &db); // Open or create a SQLite database file named "Bible.db"

	// Create the Chapters table
	sql = "DROP TABLE IF EXISTS Chapters";
	rc = sqlite3_exec(db, sql, 0, 0, 0);
	sql = "CREATE TABLE Chapters (BookID INT, ChapterID INT, PRIMARY KEY (ChapterID))";
	rc = sqlite3_exec(db, sql, 0, 0, 0);
	sql = "CREATE INDEX IF NOT EXISTS C_Chapter_Index ON Chapters (ChapterID)"; // creating indexes for efficiency
	rc = sqlite3_exec(db, sql, 0, 0, 0);
	sql = "CREATE INDEX IF NOT EXISTS C_Book_Index ON Chapters (BookID)";
	rc = sqlite3_exec(db, sql, 0, 0, 0);
	// Create the Verses table
	sql = "DROP TABLE IF EXISTS Verses";
	rc = sqlite3_exec(db, sql, 0, 0, 0);
	sql = "CREATE TABLE Verses (VerseID INT, ChapterID INT, Body TEXT, PRIMARY KEY (VerseID), FOREIGN KEY (ChapterID) REFERENCES Chapters(ChapterID))";
	rc = sqlite3_exec(db, sql, 0, 0, 0);
	sql = "CREATE INDEX V_Verse_Index ON Verses (VerseID)";
	rc = sqlite3_exec(db, sql, 0, 0, 0);
	sql = "CREATE INDEX V_Chapter_Index ON Verses (ChapterID)";
	rc = sqlite3_exec(db, sql, 0, 0, 0);

	ifstream inputFile(source); // "kjv.txt" for KJV, etc.
	if (inputFile.is_open()) {
		string line;

		int bookID = 0;
		int chapterID = 0;
		int verseID = 0;

		string prevBook = "";
		int prevChapter = -1;
		string books[66];

		while (getline(inputFile, line)) {
			string body = "";

			string book = "";
			string chapter = "";

			int state = 0;
			for (size_t i = 0; i < line.length();) {
				string c = readUtf8Character(line, i);
				switch(state) {
					case 0: // book
						if (isdigit(c[0]) && i != 1) { // book identified
							if (!(prevBook == book)) {
								prevBook = book;
								prevChapter = -1; // forcing a new chapter for single-chapter books
								book.pop_back(); // pruning " " from the end of Book name
								books[bookID] = book;
								bookID++;
							}
							chapter += c;
							state++;
						}
						else { // identifying the book
							book += c;
						}
					break;
					case 1: // identifying chapter
						if (c == ":") {
							state++;
						}
						else {
							chapter += c;
						}
					break;
					case 2: // identified chapter
						if (prevChapter != stoi(chapter)) {
							prevChapter = stoi(chapter);
							// new chapter
							chapterID++;

							sql = "INSERT INTO Chapters (BookID, ChapterID) VALUES (?, ?)";
							rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

							sqlite3_bind_int(stmt, 1, bookID);
							sqlite3_bind_int(stmt, 2, chapterID);
							rc = sqlite3_step(stmt);
							sqlite3_finalize(stmt);
						}
						state++;
					break;
					case 3: // finding verse start
						// checking if an undesired unicode (e.g. ¶) has appeared
						if (isUndesiredCharacter(c)) {
							continue;
						}
						if (c != " " && !isdigit(c[0])) { // verse start found
							body += c;
							state++;
						}
					break;
					case 4: // verse
						if (isUndesiredCharacter(c)) {
							continue;
						}
						body += c;
					break;
				}
			}
			verseID++;

			sql = "INSERT INTO Verses (VerseID, ChapterID, Body) VALUES (?, ?, ?)";
			rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
			if (rc != SQLITE_OK) {
				cerr << "SQL error: " << sqlite3_errmsg(db) << endl;
				sqlite3_close(db);
				return 1;
			}
			sqlite3_bind_int(stmt, 1, verseID);
			sqlite3_bind_int(stmt, 2, chapterID);
			sqlite3_bind_text(stmt, 3, body.c_str(), -1, SQLITE_STATIC);
			rc = sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
		inputFile.close();
		// generating optimal unique substring matches for books
		string abbreviations[66];
		abbreviate(books, abbreviations);

		writeTable((directory+"Bible.table").c_str(), abbreviations, books);

	} else {
		cerr << "Failed to open the file." << endl;
		sqlite3_close(db);
		return 1;
	}

	sqlite3_close(db);
	return 0;
}

int main(int argc, char **argv) { // add filename inputs
	if (argc < 2) {
		cout << "missing function, \"t\" for text, \"s\" for search, \"ts\" for both" << endl;
	}
	else {
		if (string(argv[1]) == "t") { // update Bible text
			if (argc < 4) {
				cout << "Missing folder to output text to" << endl;
			}
			else { // presently hardcoded input, update later
				int result = updateText(argv[2], argv[3]);
				if (result == 0) {
					cout << "Text update success" << endl;
				}
				else {
					cout << "Text update failure" << endl;
				}
			}
		}
		else if (string(argv[1]) == "s") { // update word search index
			if (argc < 3) {
				cout << "Missing folder to insert search DB into" << endl;
			}
			else {
				int result = updateSearch(argv[2]);
				if (result == 0) {
					cout << "Search update success" << endl;
				}
				else {
					cout << "Search update failure" << endl;
				}
			}
		}
		else if (string(argv[1]) == "ts") {
			// do both
			if (argc < 4) {
				cout << "Missing inputs";
			}
			else {
				int result = updateText(argv[2],argv[3]); // you can have these filenames be input
				if (result == 0) {
					cout << "Text update success" << endl;
					result = updateSearch(argv[3]);
					if (result == 0) {
						cout << "Search update success" << endl;
					}
					else {
						cout << "Search update failure" << endl;
					}
				}
				else {
					cout << "Text update failure" << endl;
				}
			}
		}
		else {
			cout << "missing function, \"t\" for text, \"s\" for search, \"ts\" for both" << endl;
		}
	}
	return 0;
}
