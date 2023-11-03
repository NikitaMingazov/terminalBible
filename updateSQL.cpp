/*
This is a script that creates the files used in my program
It takes a .txt and creates a "filename".db, a "filename".table(just plaintext) and "filename"Search.db
*/
#include <iostream>
#include <fstream>
#include <cmath>
#include <sqlite3.h>
#include <cstring>

/*
g++ -o updateSQL updateSQL.cpp -lsqlite3
*/
using namespace std;

int updateSearch(const char* filename) {
	sqlite3* tdb; // input Bible text database
	sqlite3* sdb; // output search index database
	sqlite3_stmt* stmt;
	const char* sql;
	// open the Bible text as read-only
	// this filename extension formatting needs to improve
	int rc = sqlite3_open_v2((string(filename)+string(".db")).c_str(), &tdb, SQLITE_OPEN_READONLY, nullptr);

	rc = sqlite3_open((string(filename)+string("Search.db")).c_str(), &sdb);

	sql = "CREATE TABLE IF NOT EXISTS Words (WordID INT, Word TEXT, PRIMARY KEY (WordID))";
	rc = sqlite3_exec(sdb, sql, 0, 0, 0);
	sql = "DELETE FROM Words";
	rc = sqlite3_exec(sdb, sql, 0, 0, 0);
	sql = "CREATE TABLE IF NOT EXISTS WordVerse (WordID INT, VerseID INT, PRIMARY KEY (WordID, VerseID))";
	rc = sqlite3_exec(sdb, sql, 0, 0, 0);
	sql = "DELETE FROM WordVerse";
	rc = sqlite3_exec(sdb, sql, 0, 0, 0);


	sqlite3_close(tdb);
	sqlite3_close(sdb);
	return 0;
}
// testing for undesired unicode and giving number of chars to skip (present in kjv.txt)
int charTest(const char* c) {
	if (!strncmp(c, u8"¶", 2)) { // pilcrows
		return 1; // length of pilcrow - 1
	}
	if (!strncmp(c, u8"‹", 2) || !strncmp(c, u8"›", 2)) { // red-letter 'quotations'
		return 2; // length of quotation - 1
	}
	return 0;
}
int updateText(const char* source, const char* filename) {
	sqlite3* db;
	sqlite3_stmt* stmt;
	const char* sql;
	
	int rc = sqlite3_open((string(filename)+string(".db")).c_str(), &db); // Open or create a SQLite database file named "Bible.db"
	
	// Create the Chapters table
	sql = "DROP TABLE IF EXISTS Chapters";
	rc = sqlite3_exec(db, sql, 0, 0, 0);
	sql = "CREATE TABLE Chapters (BookID INT, ChapterID INT, PRIMARY KEY (ChapterID))";
	rc = sqlite3_exec(db, sql, 0, 0, 0);
	sql = "CREATE INDEX IF NOT EXISTS Chapter_Index ON Chapters (ChapterID)"; // creating indexes for efficiency
	rc = sqlite3_exec(db, sql, 0, 0, 0);
	sql = "CREATE INDEX IF NOT EXISTS Book_Index ON Chapters (BookID)";
	rc = sqlite3_exec(db, sql, 0, 0, 0);
	// Create the Verses table
	sql = "DROP TABLE IF EXISTS Verses";
	rc = sqlite3_exec(db, sql, 0, 0, 0);
	sql = "CREATE TABLE Verses (VerseID INT, ChapterID INT, Body TEXT, PRIMARY KEY (VerseID), FOREIGN KEY (ChapterID) REFERENCES Chapters(ChapterID))";
	rc = sqlite3_exec(db, sql, 0, 0, 0);
	sql = "CREATE INDEX Verse_Index ON Verses (VerseID)";
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
		string bookTags[66];

		while (getline(inputFile, line)) {
			string body = "";

			string book = "";
			string booktag = ""; // temporarily using this until I properly implement my dictionary's parsing
			string chapter = "";

			int state = 0;
			for (int i = 0; i < line.length(); i++) {
				char c = line[i];
				int charSkip = 0;
				switch(state) {
					case 0: // book
						if (isdigit(c) && i != 0) { // book identified
							if (!(prevBook == book)) {
								prevBook = book;
								prevChapter = -1; // forcing a new chapter for single-chapter books
								books[bookID] = book;
								bookTags[bookID] = booktag; // temporarily here until properly solved
								bookID++;
							}
							chapter += c;
							state++;
						}
						else { // identifying the book
							if (c != ' ') {
								booktag += c; // temporarily using a separate space-free one until I strip it while parsing
							}
							book += c;
						}
					break;
					case 1: // identifying chapter
						if (c == ':') {
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
						charSkip = charTest(&line[i]); // checking if an undesired unicode (¶) has appeared
						if (charSkip > 0) {
							i += charSkip;
							continue;
						}						
						if (c != ' ' && !isdigit(c)) { // verse start found
							body += c;
							state++;
						}
					break;
					case 4: // verse
						charSkip = charTest(&line[i]); // checking if an undesired unicode (‹›) has appeared
						if (charSkip > 0) {
							i += charSkip;
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

		string map[66];
		int booksLen = sizeof(books) / sizeof(books[0]);
		for (int i = 0; i < booksLen; i++) {
			string cur = books[i];
			map[i] = bookTags[i];
			for (int j = 0; j < booksLen; j++) {
				//
			}
		}
		// create and save book mappings
		ofstream outputFile((string(filename)+string(".table")).c_str());

		if (outputFile.is_open()) {
			outputFile << "# This is a comment" << std::endl;
			outputFile << "# Each line should either start with \"#\" indicating a comment" << std::endl;
			outputFile << "# Or have BOOKNAME ID (#COMMENT)" << std::endl;
			for (int i = 0; i < booksLen; i++) {
				outputFile << map[i] << " " << (i+1) << " #" << books[i] << std::endl;
			}
			outputFile << "# Add your own mappings below" << std::endl;

			outputFile.close();
		} else {
			std::cerr << "Unable to open the file for writing." << std::endl;
		}
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
		cout << "Function missing, refer to \"README.md\"" << endl;
	}
	else {
		if (string(argv[1]) == "t") { // update Bible text
			int result = updateText("kjv.txt","Bible"); // you can have these filenames be input
			if (result == 0) {
				cout << "Text update success" << endl;
			}
			else {
				cout << "Text update failure" << endl;
			}
		}
		else if (string(argv[1]) == "s") { // update word search index
			int result = updateSearch("Bible");
			if (result == 0) {
				cout << "Search update success" << endl;
			}
			else {
				cout << "Search update failure" << endl;
			}
		}
		else if (string(argv[1]) == "ts") {
			// do both
		}
		else {
			cout << "Unknown function called, refer to \"README.md\"." << endl;
		}
	}
	return 0;
}
