#include <iostream>
using namespace std;
#define GREAN "\033[32m"
#define DEFALUT "\033[0m"
enum Color
{
	Zero = 0,
	Red = 31,
	Grean,
	Yellow,
	Blue,
	Purple,
	DarkBle,
	White
};
void CC(Color c) {
	cout << "\033[" << c << "m" << flush;
}
int main()
{	
//	cout << GREAN << "world" << DEFAULT << endl;
	CC(Grean);
	cout << "hello " << endl;
	CC(Zero);
	CC(Blue);
	cout << "world" << endl;
	CC(Red);
	cout << "world" << endl;
	CC(Yellow);
	cout << "world" << endl;
	CC(Zero);
	cout << "world" << endl;
	cout << "\033[32m hello \033[0m" << endl;
	return 0;
}
