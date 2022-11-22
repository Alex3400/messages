#include <iostream>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include<tuple> 
#include <algorithm>
#include <cctype>
#include <fcntl.h>
#include <Windows.h> // For Win32 APIs
#include <locale>
#include <codecvt>//
#include <tuple>
#include <unordered_map>


using namespace std;

unordered_map<string, tuple<string, int>> emojis;
const string classification = u8"\U0000200D";


// code taken from Bertrand Cachet on github to be able to write UTF-16 charachters to text files.
std::wstring decode(const std::string& str, int codePage = GetACP())
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(codePage, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(codePage, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string encode(const std::wstring& wstr, int codePage = GetACP())
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(codePage, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(codePage, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
} 


unordered_map<string, tuple<string, int>> get_emojis() {
    unordered_map<string, tuple<string, int>> e;
    fstream infile;
    infile.open("emojis-out.txt", ios::in);
    string tp;
    while (getline(infile, tp)) {
        size_t space = tp.find(" ");
        string emoji = tp.substr(0, space);
        string name = tp.substr(space);
        e.insert({ emoji,  make_tuple(name + " ", 0) });
    }
    infile.close();
    e.insert({ u8"\U0000201c" , make_tuple("\"", 0) }); //iphones code start and end quotes, as well as apostrophes diffrently from how 
    e.insert({ u8"\U0000201c" , make_tuple("\"", 0) }); //they're typically ascii coded so these 3 lines standardizes it.
    e.insert({ u8"\U00002019",  make_tuple("'", 0) });
    return e;
}
class Time {
private:
    int minute;
    int hour;
    int day;
    int month;
    int year;
    int dow; //day of week, 0 = sunday, 1 = monday etc.
public:
    Time() {
        minute = 0;
        hour = 0;
        day = 0;
        month = 0;
        year = 0;
        dow = 0;
    }
    Time(int m, int h, int d, int mo, int y){
        minute = m;
        hour = h;
        day = d;
        month = mo;
        year = y;
        int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
        y -= m < 3;
        dow = (y + y / 4 - y / 100 + y / 400 + t[mo - 1] + d) % 7;
    }
    int getMinute() {
        return minute;
    }
    int getHour() {
        return hour;
    }
    int getDay() {
        return day;
    }
    int getMonth() {
        return month;
    }
    int getYear() {
        return year;
    }
    int getDow(){
        return dow;
    }
    string toString() {
        string s = "";
        int times[5] = {hour, minute, day, month, year};
        for (int i = 0; i < 5; i++)
        {
            if (times[i] < 10) //this stuff is neccesary because it stores 2pm and february as 2 instead of 02
                s = s + "0";
            s = s + to_string(times[i]);
            if (i == 0)
                s = s + ":";
            else if (i == 1)
                s = s + ", ";
            else if (i < 4)
                s = s + "-";
        }
        string dowS = "";
        switch (dow) {
            case(0):
                dowS = "Sunday";
                break;
            case(1):
                dowS = "Monday";
                break;
            case(2):
                dowS = "Tuesday";
                break;
            case(3):
                dowS = "Wednesday";
                break;
            case(4):
                dowS = "Thursday";
                break;
            case(5):
                dowS = "Friday";
                break;
            case(6):
                dowS = "Saturday";
                break;
        }
        s = s + " " + dowS;
        return s;
    }

};
class Message {
private: 
    float avgWordLength;
    string content;
    Time time;
    vector<string> words;
    int chars;
public:
    Message(){
        content = "";
        time = Time();
        chars = 0;
        avgWordLength = 0;
    }
    Message(string c, Time t){
        chars = 0;
        content = "";
        addContent(c);
        time = t;
    }
    void addContent(string c) {
        c = content + c;
        chars = 0;
        //code to change emojis to " :{emoji name}: " and changing the charchter count to be correct. each emoji counts as 1.
        for (int i = 0; i < c.length(); i++) { //code to change ascii values of emojis to :{emoji name}:, and count emoji
            if (c[i] < 0) { //emojis are denoted by negative charchters, so if a negative value is found search for emoji
                int j = 1;
                while (c[i + j] < 0) { //emojis can be of varying length so keep checking for negative chars
                    tuple<string,int> e = emojis[c.substr(i, j + 1)]; //see if the string of negative chars corresponds to an emoji
                    if (!get<0>(e).empty()) {                      //if it does:
                        string s = c.substr(i + j + 1, 3);
                        if (s == classification) {//see if the 3 charachters after the emoji match up with the classification emoji
                            j += 2;               // if it does then the whole emoji is bigger and we should continue looking
                            continue;
                        }
                        int k = 1;
                        bool end = true;
                        while (end && i + j + k < c.length() && c[i + j + k] < 0 && k < 6) {
                            tuple<string, int> e = emojis[c.substr(i, j + k + 1)]; // this all checks if the current emoji found + the next 3-6 charchters
                            if (!get<0>(e).empty()) {                           // also makes an emoji, if it does then the emoji might be bigger.
                                j += k - 1;
                                end = false;
                            }
                            k += 1;
                        }
                        if (end) { //if the emoji is done then we replace the ascii stuff with the name.
                            c.replace(i, j + 1, get<0>(e)); //replace the negative chars with ":{emoji name}:"
                            get<1>(e) += 1;
                            chars -= (get<0>(e).size() - 1); //accounts for the length of the newly inserted string, then adds 1 because the emoji has 1 length
                            i += get<0>(e).length() - 1; // moves the pointer to the end of fixed emoji
                            break;
                        }
                    }
                    j++;
                }
            }
        }
        content = c;
        chars += c.length();
        words.clear();
        int prev = 0;
        int space = c.find(' ');
        //c.erase(std::remove_if(c.begin(), c.end(), ispunct), c.end());
        while (space != string::npos) {
            string str = c.substr(prev, space - prev);
            if (str != " ")
                words.push_back(str);
            c = c.substr(space + 1, string::npos);
            space = c.find(' ');

        }
        words.push_back(c);

    }
    float getAvgWordLen() {
        if (avgWordLength == -1) {
            int tot = 0;
            for (int i = 0; i < words.size(); i++){
                tot += words[i].length();
            }
            avgWordLength = tot / words.size();
        }
        return avgWordLength;
    }
    string getContent() {
        return content;
    }
    Time getTime() {
        return time;
    }
    int getChars() {
        return chars;
    }
    vector<string> getWords() {
        return words;
    }


};
class User {
private:
    string name;
    vector<Message> messages;
public:
    User(string n) {
        name = n;
    }
    void addMessage(Message m) {
        messages.push_back(m);
    }
    string getName() {
        return name;
    }
    vector<Message> getMessages() {
        return messages;
    }
};
class Conversation{
private:
    vector<User> users;
    vector<Message> messages;
public:
    Conversation() {

    }
    Conversation(vector<User> u, vector<Message> m)
    {
        users = u;
        messages = m; 
    }
    vector<User> getUsers() {
        return users;
    }
    vector<Message> getMessages() {
        return messages;
    }
    vector<tuple<int, string>> getTopWords(){
        vector<tuple<int, string>> sol;
        for (int i = 0; i < messages.size(); i++) {
            Message m = messages[i];
            vector<string> w = m.getWords();
            for (int j = 0; j < w.size(); j++) {
                bool unique = false;
                string word = w[j];
                std::transform(word.begin(), word.end(), word.begin(), [](unsigned char c) { return std::tolower(c);});
                for (int k = 0; k < sol.size(); k++) { //maybe don't want to make everything lowercase idk
                    if (word.compare(get<1>(sol[k])) == 0) {
                        get<0>(sol[k]) += 1;
                        unique = true;
                        break;
                    }
                }
                if (!unique) {
                    sol.push_back(make_tuple(1, word));
                }   
            }
        }
        sort(sol.begin(), sol.end());
        cout << "sorted";
        return sol;
    }
 };

int main() {
    auto begin = std::chrono::high_resolution_clock::now();
    emojis = get_emojis();
    fstream newfile;
    newfile.open("tpoint.txt", ios::in); //open a file to perform read operation using file object
    if (newfile.is_open()) { //checking whether the file is open
        Conversation convo;
        vector<Message> messages;
        vector<User> users;
        string tp;
        int i = 0;
        string save = "";
        bool sectioned = false;
        Message m1 = Message(); // this is to hold onto the previous message so we can add content to it if there is multiple lines.

        while (getline(newfile, tp)) { //read data from file object and put it into string.
            int first = tp.find_first_of('/');
            int second = tp.find_first_of('/', first + 1);
            int colon = tp.find_first_of(':');
            int space = tp.find_first_of(' ');
            int dash = tp.find_first_of('-');
            int colon2 = tp.find_first_of(":", colon + 2);

            int mo; int d; int y; int h; int m;
            try {
                mo = stoi(tp.substr(0, first));
                d = stoi(tp.substr(first + 1, second - first - 1));
                y = stoi(tp.substr(second + 1, tp.find_first_of(',') - second - 1));
                h = stoi(tp.substr(space + 1, colon - space - 1));
                if (tp[colon + 4] == 'P')
                    h += 12;
                m = stoi(tp.substr(colon + 1, 2));
            }
            catch (...) { // if the message isn't in the expected form an exception will be thrown, this means that the message has a new line in it
                sectioned = true; // in this case we want to hold onto this string until there is a correct message
                save += "\n" + tp;
                continue;
            }
            if (sectioned) { //when there is a correct message we add the saved content to the last correct message.
                m1.addContent(save);
                sectioned = false;
                save = "";
            }
            i++;
            string name = tp.substr(dash + 2, colon2 - dash - 2);
            string message = tp.substr(colon2 + 2);
            Time t = Time(m, h, d, mo, y);
            Message m1 = Message(message, t);
            bool unique = true;
            for (User u : users) {
                if (name.compare(u.getName()) == 0) {
                    u.addMessage(m1);
                    unique = false;
                }
            }
            if (unique) {
                users.push_back(User(name));
                cout << name;
            }
            //cout << t.toString() + "\n"; 
            messages.push_back(m1);
        }
        //reads line and takes in time, content, and sender

        convo = Conversation(users, messages);
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
        cout << elapsed.count() * 1e-9;
       
        newfile.close();
        fstream outfile;
        /*outfile.open("dataOut.txt", ios::out);
        vector<tuple<int, string>> topwords = convo.getTopWords();
        for (int j = 0; j < topwords.size(); j++) {
            tuple<int, string> tup = topwords[j];
            outfile << get<1>(tup) + "  " + to_string(get<0>(tup)) + "\n";
        }
        outfile.close();*/
        outfile.open("dataOut2.txt", ios::out);
        for (int j = 0; j < messages.size(); j++) {
            outfile << messages[j].getContent() + "\n";
        }
        auto end2 = std::chrono::high_resolution_clock::now();
        auto elapsed2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - begin);
        cout << elapsed2.count() * 1e-9;
    }   
};
