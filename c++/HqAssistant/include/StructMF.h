#pragma once

#include <Windows.h>

struct fltype
{
	WORD astk : 1;
	WORD bstk : 1;
	WORD jj : 1;
	WORD gz : 1;
	WORD qiz : 1;
	WORD qz : 1;
	WORD qtstk : 1;
	WORD cystk : 1;
	WORD sb : 1;
	WORD addcode : 1;
	WORD cyb : 1;
	WORD unused : 5;
};

union flunion
{
	struct fltype ftype;
	WORD fshort;
};
