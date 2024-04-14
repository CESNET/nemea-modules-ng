#include <ncurses/ncurses.h>
#include <string>

class CursesPrint {
public:
	CursesPrint();

	void print(std::string string);

	~CursesPrint();
};
