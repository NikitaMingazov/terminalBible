
#include "BibleText.h"

#include <fstream>
#include <string>

std::string reformat(const std::string& input) {
	std::string result;

	for (size_t i = 0; i < input.size(); ++i) {
		if (input[i] == '[') {
			result += "\\textit{";
		}
		else if (input[i] == ']') { 
			result += "}";
		}
		else {
			result += input[i];
        	}
	}

	return result;
}

int main() {
	std::ofstream outputFile("Eng-Rus_Polyglot.tex");
	
	if (outputFile.is_open()) {
		
		outputFile << "\\documentclass[12pt]{book}" << std::endl;
		outputFile << "\\usepackage{paracol}" << std::endl;
		outputFile << "\\usepackage{fontspec}" << std::endl;
		outputFile << "\\setmainfont{Times New Roman}" << std::endl;
		
		outputFile << "\\usepackage{geometry}" << std::endl;
		outputFile << "\\geometry{left=1.5cm,right=1.5cm,top=1.5cm,bottom=1.5cm}" << std::endl;
		outputFile << "\\linespread{1.3}" << std::endl;

		outputFile << "\\begin{document}" << std::endl;

		outputFile << "\\tableofcontents" << std::endl;

		outputFile << "\\begin{paracol}{2}" << std::endl;

		BibleText eng = BibleText("KJV");
		BibleText rus = BibleText("SYNODAL");
	
		std::string prevBook = "";
		int prevChapter = 0;
		for (int i = 1; i < 31012+91; i++) {
			// formatting
			outputFile << "\\switchcolumn[0]*[\\vspace*{\\fill}]" << std::endl;

			std::tuple<std::string, int, int, std::string> engVerse;
			eng.retrieveVerseFromID(i, engVerse);

			std::tuple<std::string, int, int, std::string> rusVerse;
			rus.retrieveVerseFromID(i, rusVerse);

			if (std::get<0>(engVerse) != prevBook) {
				outputFile << "\\chapter{" << std::get<0>(engVerse) << "}" << std::endl;
				// new Book
				outputFile << "\\textbf{\\LARGE " << std::get<0>(engVerse) << "} \\switchcolumn" << std::endl;
				
				outputFile << "\\textbf{\\LARGE " << std::get<0>(rusVerse) << "} \\switchcolumn" << std::endl;
				
				outputFile << "\\textbf{\\LARGE " << std::get<1>(engVerse) << "}  " << reformat(std::get<3>(engVerse)) << " \\switchcolumn" << std::endl;
		
				outputFile << "\\textbf{\\LARGE " << std::get<1>(rusVerse) << "}  " << std::get<3>(rusVerse) << " \\switchcolumn" << std::endl;
			}
			else if (std::get<1>(engVerse) != prevChapter) {
				// new chapter
				outputFile << "\\textbf{\\LARGE " << std::get<1>(engVerse) << "}  " << reformat(std::get<3>(engVerse)) << " \\switchcolumn" << std::endl;

				outputFile << "\\textbf{\\LARGE " << std::get<1>(rusVerse) << "}  " << std::get<3>(rusVerse) << " \\switchcolumn" << std::endl;
			}
			else {
				outputFile << std::get<2>(engVerse) << ": " << reformat(std::get<3>(engVerse)) << " \\switchcolumn" << std::endl;
		
				outputFile << std::get<2>(rusVerse) << ": " << std::get<3>(rusVerse) << " \\switchcolumn" << std::endl;
			}
			prevBook = std::get<0>(engVerse);
			prevChapter = std::get<1>(engVerse);
		}

		outputFile << "\\end{paracol}" << std::endl;
		outputFile << "\\end{document}" << std::endl;

		outputFile.close();
	}
	return 0;
}
