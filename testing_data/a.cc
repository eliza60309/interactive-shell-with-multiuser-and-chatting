#include <bits/stdc++.h>

using namespace std;

int main()
{
	char c;
	int cnt = 0;
	while(1)
	{
		cin >> c;
		if(c == '|')
		{
			cnt++;
			cout << cnt << endl;
		}
	}
}
