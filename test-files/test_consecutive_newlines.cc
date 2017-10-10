#include <iostream>
using namespace std;





float foo() {
  return 5.0f;
}

int x(float f) {
  return (int)f;
}

int main() {
	float a, b;
    int c;
    cin >> a >> b;
    if (13.4*a == 10*b - 350) {
        return 1;
    } else {
        return 0;
    }
}
