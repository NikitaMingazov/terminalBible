# terminalBible
search the Scriptures from a terminal

#Usage directions:
##To use terminalBible:
  To start just do "./terminalBible"
  Then (flag) Bookname Chapter(:verse{-verse})
  Flag can be "r" (default) to output full verses
  or "m" to output only first letter of each verse (for memorisation)
  later will add "c" for properly formatted to copy-paste

  The program remembers the previous flag
  Bookname can be anything as long as it starts with one of the key values in the .table file (it's just plain text)
  so "Isaiah", "I", "I&& %" are all valid
  Chapter is an integer, Exodus 4 returns what you would expect
  Verse is optional, as is the "-verse2" for printing a passage
  inter-chapter/book continuation will be added

##To create a Bible instance to read:
  Run "./updateSQL t" (currently uses hardcoded filenames) to create a database based on a .txt of the same format as "kjv.txt" present in folder
  (FUTURE FUNCTIONALITY) Run "./updateSQL s" to create a word search database based on the provided database
  (later ./updateSQL st {source.txt} {filename} will do all the setup in one)
  NOTE: do not use ".db" extension, it will be automatically appended by the program
