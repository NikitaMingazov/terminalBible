# terminalBible
search the Scriptures from a terminal

# Usage directions:
## To use terminalBible:
  To start run "./terminalBible {directory}" (directory containing resources, provided is "KJV") <br>
  Then (flag) Bookname Chapter(:verse{-verse}) <br>
  Flag can be "r" (default) to output full verses <br>
  or "m" to output only first letter of each verse (for memorisation) <br>
  or "s" to perform a word search (will be REGEX later) <br>
  later will add "c" for properly formatted to copy-paste <br>
  Flag is optional, the program remembers the previously used one (unless using "s"), and it defaults on "r" <br>

  John 3:36 returns 
  <blockquote>36: He that believeth on the Son hath everlasting life: and he that believeth not the Son shall not see life; but the wrath of God abideth on him.</blockquote>
  Bookname can be anything as long as it starts with one of the key values in the .table file (it's plain text) <br>
  Chapter is an integer <br>
  Verse is optional, as is the "-verse2" for printing a passage (inter-chapter/book continuation will be added) <br>

## To create a Bible instance to read: <em>(Default KJV provided)</em>
  Run "./updateSQL t {input.txt} {output_directory}"  to create a database based on a .txt of the same format as "kjv.txt" present in release 1.0 <br>
  Run "./updateSQL s {directory}" to create a word search database based on the provided database (currently bugged) <br>
  Or ./updateSQL st {source.txt} {filename} will do all the setup in one <br>
  NOTE: unzip the one provided in release 1.0 because the search table is 40MB, takes a while to generate <br>
