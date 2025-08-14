#include <sqlite3.h>
#include <unordered_map>
#include <string>
#include <queue>
#include <tuple>
#include <optional>

#ifndef BibleText_H
#define BibleText_H

class BibleText
{
public:
	BibleText(const char* directory);
	~BibleText();
	// into the queue provided puts the verses returned by the verse reference given
	std::optional<std::queue<int>> query(std::string line);
	// turns VerseID into reference and body in one tuple
	std::tuple<std::string, int, int, std::string> retrieveVerseFromID(int VerseID);
private:
	std::string directory; // where the files are stored, for logging purposes
	sqlite3* textdb;
	std::unordered_map<std::string, int> mappings;
	std::string* names;

	sqlite3_stmt* stmt; // these are declared in every function, I'm sick of them
	const char* tail;
	std::string sql;
	int rc;

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
	int bookEndVerseIDFromBookID(int BookID);
	// main database functions
	void handleFSMOutput(std::queue<int>& queryResults, std::tuple<int, int, int> reference, int &rangeStart);
	std::optional<std::queue<int>> FiniteStateMachine(std::string line);
	int verseIDFromReference(std::tuple<int, int, int> ref);
	std::tuple<int, int, int> fetchReferenceFromVerseID(int VerseID);
	std::string fetchBodyFromVerseID(int VerseID);

};

#endif

