# terminalBible
search the Scriptures from a terminal

# Usage directions:
## To use terminalBible:
  To start run "./terminalBible {directory}" (directory containing resources, provided is "KJV") <br>
  Then (flag) Bookname Chapter(:verse{-{chapter:}verse}) <br>
  Flag can be "/r" (default) to output full verses <br>
  or "/m" to output only first letter of each verse (for memorisation) <br>
  or "/s" to perform a word search (will be REGEX later) <br>
  later will add "/c" for properly formatted to copy-paste <br>
  Flag is optional, the program remembers the previously used one, and it defaults on "/r" <br>

  John 3:36 returns 
  <blockquote>John 3:36 He that believeth on the Son hath everlasting life: and he that believeth not the Son shall not see life; but the wrath of God abideth on him.</blockquote>
  Bookname can be anything as long as it starts with one of the key values in the .table file (which is just plain text) <br>
  Chapter is an integer <br>
  Verse is an optional integer, as is the "-verse2" for printing a passage. If verse is ommited the whole chapter is printed<br>
  The '-' continuation on it's own prints the rest of the chapter (e.g. John 3:27- will return John 3:27-36)<br>
  If the continuation is "-chapter:verse", it'll go until the specified verse in the same book. If "-verse", until the verse in the same chapter <br>
<h2> Bible instances provided: (<em>release containing</em>)</h2>
<ol><li><em>KJV (1.0)</em></li>
  <li><em>Synodal (1.1)</em></li></ol>
<h2> To create a Bible instance to read: </h2>
  Run "./updateSQL t {input.txt} {output_directory}"  to create a database based on a .txt of the same format as "kjv.txt" present in release 1.0 <br>
  Run "./updateSQL s {directory}" to create a word search database based on the provided database (currently bugged) <br>
  Or ./updateSQL st {source.txt} {directory} will do all the setup in one <br>
  NOTE: unzip the one provided in release 1.0 because the search database is 40MB, takes a while to generate <br>
