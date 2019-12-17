/* File: better.cpp
 * Author: Cole Blakley
 * Purpose: This program acts as a very simple preprocessor. It is based on the
 *  preprocessor ginevra as given in Arthur Pyster's _Compiler Design and Construction_
 *  (1988 edition). It takes in a .cpp or .h text file, tokenizes it, and recognizes
 *  any #define statements, which it then uses to replace any defined macros, just
 *  like `#define APPLE 8` in C will replace all occurrences of the `APPLE` token
 *  with `8`
 *
 * This is a rewrite of ginevra++.cpp. It greatly simplifies the state machine
 * and has a more procedural style, with the Scanner object containing very
 * very little state. Overall, this is the more straightforward and readable
 * version of the preprocessor, and as an added bonus, it is the shorter version.
 */
#include <iostream>
#include <fstream>
#include <string>
#include <utility> //for std::pair
#include <map>

// The different types of tokens
enum class State : char {
    Start, Identifier, InIdentifier, InComment, String, InSingleQuote,
    InDoubleQuote, EoF, Bad, Other
};

class Scanner {
private:
    std::ifstream m_file;
public:
    Scanner(const std::string path);
    bool hasNext() const { return !m_file.fail(); }
    const std::string nextLine();
    const std::pair<State,std::string> nextToken();
};

Scanner::Scanner(const std::string path) : m_file(path)
{
    if(!m_file) {
	std::cerr << "Error: file " << path << " can't be found\n";
	exit(1);
    } else if(m_file.peek() == EOF) {
	exit(1);
    }
}

const std::string Scanner::nextLine()
{
    std::string line;
    std::getline(m_file, line);
    return line;
}

// Extract the next token from the text stream, additionally determining
// the type (state) of that token, returning both as a pair.
const std::pair<State,std::string> Scanner::nextToken()
{
    bool done = false;
    char currChar = m_file.get();
    State currState = State::Start;
    std::string tokenText;
    while(!done) {
	switch(currState) {
	case State::Start: {
	    // Skip whitespace; not significant
	    if(currChar == ' ' || currChar == '\t') {
		break;
	    // Start of `#define` or another `identifier`
	    } else if(currChar == '#' || std::isalpha(currChar)) {
		currState = State::InIdentifier;
		tokenText += currChar;
	    // Opening of single-quote string
	    } else if(currChar == '\'') {
		currState = State::InSingleQuote;
		tokenText += currChar;
	    // Opening of double-quote string
	    } else if(currChar == '"') {
		currState = State::InDoubleQuote;
		tokenText += currChar;
	    // Opening of multi-line comment
	    } else if(currChar == '/' && m_file.peek() == '*') {
		m_file.ignore(1);
		currState = State::InComment;
	    // File stream is empty; stop extracting 
	    } else if(currChar == EOF) {
		currState = State::EoF;
		done = true;
	    // Ignore newlines
	    } else if(currChar == '\n') {
		tokenText += currChar;
	    // "Other" possibilities include things like parentheses, brackets,
	    // and other non-string/identifier/comment things
	    } else {
		currState = State::Other;
		tokenText += currChar;
	    }
	    break;
	}
	case State::InIdentifier: {
	    // Identifiers are words (with optional method calls on them)
	    if(std::isalpha(currChar) || currChar == '.') {
		tokenText += currChar;
		break;
	    }
	    // If reached char that isn't part of identifier, put it back, stop
	    m_file.putback(currChar);
	    currState = State::Identifier;
	    done = true;
	    break;
	}
	case State::InSingleQuote: {
	    // Closing of string
	    if(currChar == '\'') {
		currState = State::String;
		tokenText += currChar;
		done = true;
	    // Escape backslashes within the string
	    } else if(currChar == '\\' && m_file.peek() == '\'') {
		tokenText += currChar + m_file.get();
	    // Can't have newline in middle of string
	    } else if(currChar == '\n') {
		currState = State::Bad;
		std::cerr << "\nError: Malformed string\n";
		currChar = m_file.get(); //Move onto next line
		done = true;
	    // Can't have string cut-off by end of file
	    } else if(currChar == EOF) {
		currState = State::Bad;
		std::cerr << "\nError: Unexpected end of file\n";
		done = true;
	    // Otherwise, just collect the contents of the string
	    } else {
		tokenText += currChar;
	    }
	    break;
	}
	// Double quoted strings are treated essentially the same as single-quotes 
	case State::InDoubleQuote: {
	    if(currChar == '"') {
		currState = State::String;
		tokenText += currChar;
		done = true;
	    } else if(currChar == '\\' && m_file.peek() == '\"') {
		m_file.get();
	    } else if(currChar == '\n') {
		currState = State::Bad;
		std::cerr << "\nError: Malformed string\n";
		tokenText += currChar;
		done = true;
	    } else if(currChar == EOF) {
		currState = State::Bad;
		std::cerr << "\nFatal error: Unexpected end of file\n";
		exit(1);
	    } else {
		tokenText += currChar;
	    }
	    break;
	}
	// Ignore all characters in comments until closing `*/`, then continue
	case State::InComment: {
	    if(currChar == '*' || m_file.peek() == '/') {
		m_file.ignore(1);
		if(m_file.peek() == '\n') m_file.ignore(1);
		currState = State::Start;
	    } else if(currChar == EOF) {
		currState = State::Bad;
		std::cerr << "\nFatal error: Unexpected end of file\n";
		exit(1);
	    }
	    break;
	}
	// Try to process `Other` characters, ignoring comments as necessary
	case State::Other: {
	    if(currChar == ' ' || currChar == '\n') {
		done = true;
	    } else if(currChar == '/' && m_file.peek() == '*') {
		m_file.ignore(1);
		currState = State::InComment;
		break;
	    }
	    tokenText += currChar;
	    break;
	}
	default:
	    // Have no clue how to deal with the current character, stop
	    currState = State::Bad;
	    done = true;
	}
	// Every iteration until token is complete, pull a character from the
	// stream
	if(!done) {
	    currChar = m_file.get();
	}
    }
    // Tell caller what type (state) the token is, what the token's content is
    return {currState, tokenText};
}

int main(int argc, char **argv)
{
    if(argc != 2) {
	std::cout << "usage: ./better filename[.cpp,.h]\n";
	exit(1);
    }
    const std::string path(argv[1]);
    if(path.size() < 2 || (path.substr(path.size()-2) != ".h"
			   && path.substr(path.size()-4) != ".cpp")) {
	std::cerr << "Invalid file extension\n";
	exit(1);
    }
    Scanner scanner(path);
    std::map<std::string, std::string> symbolTable;
    while(scanner.hasNext()) {
	const auto [tokenState, tokenText] = scanner.nextToken();
	// Add symbol/value from all `#define SYMBOL value` statements
	if(tokenText == "#define") {
	    const auto [symbolState, symbol] = scanner.nextToken();
	    const std::string value(scanner.nextLine());
	    if(symbolState != State::Identifier) {
		std::cerr << "\nError: expected identifier after #define\n";
		std::cout << symbol << ' ' << value << '\n';
	    } else {
		if(symbolTable.count(symbol) > 0) {
		    std::cout << "\nWarning: symbol " << symbol << " redefined\n";
		}
		// Add the mapping
		symbolTable[symbol] = value;
	    }
	// Print out identifiers separated with 1 space; replace any known symbols
	// with their mapped values
	} else if(tokenState == State::Identifier) {
	    if(symbolTable.count(tokenText) < 1) {
		std::cout << tokenText << ' ';
	    } else {
		std::cout << symbolTable[tokenText] << ' ';
	    }
	} else if(tokenState != State::Bad) {
	    std::cout << tokenText;
	} else {
	    std::cerr << "Error: bad token: " << tokenText << '\n';
	}
    }

    return 0;
}
