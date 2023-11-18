#include <sqlite3.h>
#include <unordered_map>
#include <string>
#include <queue>
#include <tuple>

#ifndef BibleText_H
#define BibleText_H

class BibleText
{
public:
	BibleText(const char* directory);
	~BibleText();
	// into the queue provided puts the verses returned by the verse reference given
	int query(std::queue<int>& queryResults, std::tuple<std::string, int, int> ref, std::string end);
	// turns VerseID into reference and body in one tuple
	void retrieveVerseFromID(int VerseID, std::tuple<std::string, int, int, std::string>& verse);
private:
	sqlite3* textdb;
	std::unordered_map<std::string, int> mappings;
	std::string* names;

	sqlite3_stmt* stmt; // these are declared in every function, I'm sick of them
	const char* tail;
	std::string sql;
	int rc;
	// debugging (should these be static?)
	std::string getTime();
	void logError(const std::string& message);
	// database access
	int oneIntInputOneIntOutput(const char* sql, int input);
	int twoIntInputOneIntOutput(const char* sql, int input1, int input2);
	std::string oneIntInputOneStringOutput(const char* sql, int input);
	// .txt access
	void populateMap(std::string filename);
	void populateNameTable(std::string filename);
	// .txt usage
	int getBookID(std::string book);
	// database usage
	int chapterIDFromVerseID(int VerseID);
	int bookIDFromChapterID(int ChapterID) ;
	int chapterIDFromBookIDandOffset (int BookID, int offset);
	int verseIDFromChapterIDandOffset (int ChapterID, int offset);
	int chapterOffsetFromBookAndChapterID(int BookID, int ChapterID);
	int verseOffsetFromChapterAndVerseID(int ChapterID, int VerseID);
	int chapterEndVerseIDFromVerseID(int VerseID);
	// main database functions
	int verseIDFromReference(std::tuple<std::string, int, int> ref);
	void fetchReferenceFromVerseID(int VerseID, std::tuple<std::string, int, int>& reference);
	std::string fetchBodyFromVerseID(int VerseID);

};

#endif