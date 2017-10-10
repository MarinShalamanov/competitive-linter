#include <iostream>
using namespace std;

int main() {
	int a;
	cin >> a;
    hell:
	if (a%2 == 0) {
		cout << "even";	
	} else {
		goto end;
		cout << "odd";
        goto hell;
	}
	end: ;
}
