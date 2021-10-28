#pragma once

#include <string>

enum CASES{ 
	PRINTENV = 0,
	SETENV,
	EXIT,
	FILEOUTPUTREDIRECT,
	ORDINARYPIPEORNOPIPE,
	STDOUTNUMBERPIPE,
	STDOUTSTDERRNUMBERPIPE
};

class LineInputInfo{
public:
    CASES cases;
    int number;
    std::string fileName;

    LineInputInfo(CASES, int, std::string);
};
