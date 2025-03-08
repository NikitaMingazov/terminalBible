import sqlite3

conn = sqlite3.connect('BibleSearch.db')

cursor = conn.cursor()

cursor.execute('''DROP TABLE IF EXISTS Words''')
cursor.execute('''CREATE TABLE Words (WordID INT, Word VARCHAR(40), PRIMARY KEY (WordID))''')
cursor.execute('''CREATE INDEX Words_Word_Index ON Words (Word)''')

cursor.execute('''DROP TABLE IF EXISTS WordVerse''')
cursor.execute('''CREATE TABLE WordVerse (WordID INT, VerseID INT, PRIMARY KEY (WordID, VerseID), FOREIGN KEY (WordID) REFERENCES Words(WordID))''')
cursor.execute('''CREATE INDEX WordVerse_WordID_Index ON WordVerse (WordID)''')

VerseID = 1
wordIDCounter = 1

def handleWord(word):
    global wordIDCounter
    cursor.execute('''SELECT WordID FROM Words WHERE Word = ?''',(word,))
    WordID = cursor.fetchone()
    #print(word,WordID,wordIDCounter,VerseID)
    if not WordID:
        WordID = wordIDCounter
        wordIDCounter += 1
        cursor.execute('''INSERT INTO Words (WordID, Word) VALUES (?, ?)''',(WordID,word))
    else:
        WordID = WordID[0]
    try:
        cursor.execute('''INSERT INTO WordVerse (WordID, VerseID) VALUES (?, ?)''',(WordID,VerseID))
    except sqlite3.IntegrityError as e:
        nothing = 0

insertion = False;
try:
    with open("kjvformatted.txt", 'r', encoding='utf-8') as input_file:
        for line in input_file:
            temp = line.strip()
            if insertion == False:
                if temp == '{':
                    insertion = True
            else:
                if temp == '}':
                    insertion = False
                    VerseID += 1
                else:
                    handleWord(temp)
except Exception as e:
    print(f"An error occurred: {e}")

conn.commit()
print(wordIDCounter)
