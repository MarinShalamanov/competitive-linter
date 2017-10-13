#include <iostream>
#include <algorithm>
using namespace std;

int NOD(int a, int b)
{
    if (b > a)
        swap(a, b);

    while (a % b != 0) {
        int c = b;
        b = a % b;
        a = c;
    }
    return b;
}

int main()
{
    int n;
    cin >> n;
    int x, y, X, Y;
    pair<int, int> all[n];
    bool vertical = false;
    bool horizontal = false;
    int sum  = 0;

    for (int i = 0; i < n; i++) {
        cin >> x >> y >> X >> Y;
        if (x < X) {
            swap(x, X);
            swap(y, Y);
        }
        if ((x - X) == 0)
            all[i] = make_pair(0,1);
        if ((y - Y) == 0)
            all[i] = make_pair(1,0);
        if ((x - X) != 0 && (y - Y) != 0) {
            int nod = NOD(x - X, y - Y);
            all[i] = make_pair((x - X) / nod, (y - Y) / nod);
            //cout << "Pair is:" << all[i].first << "/" << all[i].second<< endl;
        }
    }
    /*if(horizontal == true)
        sum++;
    if(vertical == true)
        sum++;
        */
    sort(all+0, all+n);
    for(int i = 1; i < n; i++){
            //cout << "Pair is:" << all[i].first << "/" << all[i].second<< endl;
        if(all[i] != all[i-1]){
            sum++;
        }
        //cout << "Sum is " << sum << endl;
    }
    cout << sum + 1 << endl;
}