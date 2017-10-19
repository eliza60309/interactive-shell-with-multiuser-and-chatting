#include <bits/stdc++.h>

using namespace std;

int main()
{
	string s("i dont know");
	stringstream ss;
	ss << "aaaaa\nbbbbb\nccccc";
	ss >> s;
	ss.ignore(s.size());
	char c[10000] = {};
	ss.getline(c, 10000);
	cout << "\"" << s << "\"" << endl;
	cout << ss.str();
}
