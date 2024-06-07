#include "cursesPrint.hpp"

CursesPrint::CursesPrint()
{
	initscr();
}

CursesPrint::~CursesPrint()
{
	endwin();
}

void CursesPrint::print(std::string string)
{
	clear();
	printw("%s\n", string.c_str());
	refresh();
}