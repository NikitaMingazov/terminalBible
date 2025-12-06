# terminalBible
Most approaches I've seen for terminal bible queries I've seen use awk on a plaintext file, resulting in a ~50ms overhead for scanning the whole bible. This C++ and SQL approach has very low latency, although it's not yet optimised so performance slows down to be equivalent for very large outputs, as the per-verse cost is a bit higher.

# Usage directions:
## Build:
Run the Makefile to build the executable, and use the database in releases, or use the provided scripts to make your own. <br>
Then to use the executable, it is advised to make a wrapper shell script like this one: <br>
script like so <br>
```
#!/bin/bash
directory="/home/username/.local/share/biblesetup"
executable="kjv"
data="KJV"
additional_args=("$@")
"$directory/$executable" "$directory/$data" "${additional_args[@]}"
```
and so "kjv s potter" will work properly <br>
## Usage:
Format: ./kjv </data/path> <?flag> <query>
### Flags:
'r' is read mode, the default, making flag optional. an example query is "gen 1:32-2:3", where "gen" uniquely identifies "Genesis", and an inter-chapter range is used. chapter-chapter and <ref>;<ref> are currently broken. <br>
's' uses search mode, it does queries on the database. It supports AND, OR and parenthesis, and spaces are AND so "unto us is" is equivalent to "unto/ANDus/ANDis" (I know the syntax sucks, I was new to linux when I made this). <br>
"(emerods/ANDgaza)/OR(knoweth/ANDsin)" is an example query.
'm' is memorisation mode, it is read mode except all words are replaced by their first letter (only works on ASCII).

# Improvements I won't make because I'm done with this:
Have the makefile store data and executable into .local/share. <br>
Use "-s" for flags. <br>
Expose a C interface for interop. <br>
Use an automatically generated FSM instead of my manual one. <br>
Fix the bug for <book0> <subref> ; <book1> <subref> queries, they once worked but broke due to the unmaintainable manual FSM.
