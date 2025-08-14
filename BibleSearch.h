#include <sqlite3.h>
#include <queue>
#include <string>
#include <optional>

#ifndef BibleSearch_H
#define BibleSearch_H

class BibleSearch
{
public:
	BibleSearch(const char* directory);
	~BibleSearch();
	std::optional<std::queue<int>> verseIDsFromWordSearch(std::string search);
private:
	sqlite3* searchdb;

	sqlite3_stmt* stmt; // these are declared in every function, I'm sick of them
	const char* tail;
	std::string sql;
	int rc;

	// turns a word search into a SQL statement and the parameters to fill it
	std::string parseSearchIntoSqlStatement(std::string search, std::queue<std::string>& parameters);

};


#endif

