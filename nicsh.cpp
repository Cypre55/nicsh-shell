#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/inotify.h>
#include <dirent.h>

#include <fcntl.h>
#include <unistd.h> 

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

using namespace std;

struct termios saved_attributes;

static volatile int SIGCONT_flag = 0;
static volatile int stopProcess = 0;

pid_t y = getpid();
int inotify_fd;


void resetInputMode(void) {
  saved_attributes.c_lflag |= (ICANON | ECHO);
  saved_attributes.c_cc[VMIN] = 0;
  saved_attributes.c_cc[VTIME] = 1;
  tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

void setInputMode(void) {
  struct termios tattr;
  tcgetattr(STDIN_FILENO, &saved_attributes);
  atexit(resetInputMode);
  tcgetattr(STDIN_FILENO, &tattr);
  tattr.c_lflag &= ~(ICANON | ECHO);
  tattr.c_cc[VMIN] = 1;
  tattr.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}

class History {
    
    int history_len = 10000;
    int return_len = 1000;

    int last_entered;
    bool exceeded = false;

    vector<string> list;

    int filename_len = 1000;
    string filename;

    // Read history file
    int read_history() {

        ifstream infile(filename);

        string line;
        int num_lines = 0;
        while(getline(infile, line)) 
        {
            list[num_lines%history_len] = line; 
            num_lines++;    
        }

        if (num_lines >= history_len)
            exceeded = true;

        return num_lines%history_len;
    }

    void write_history() {
        ofstream outfile(filename, ios::trunc);

        if (exceeded)
        {
            for (int i = 0; i < history_len; i++)
            {
                outfile << list[(last_entered+i)%history_len] << endl;
            }
        }
        else
        {
            for (int i = 0; i < last_entered; i++)
            {
                outfile << list[i] << endl;
            }
        }
    }

public:
    string longest_substring(string str1, string str2) {
        string result;
        int m = str1.length();
        int n = str2.length();
        int LCSuff[m + 1][n + 1];
    
        int len = 0;
        int row, col;

        for (int i = 0; i <= m; i++) {
            for (int j = 0; j <= n; j++) {
                if (i == 0 || j == 0)
                    LCSuff[i][j] = 0;
    
                else if (str1[i - 1] == str2[j - 1]) {
                    LCSuff[i][j] = LCSuff[i - 1][j - 1] + 1;
                    if (len < LCSuff[i][j]) {
                        len = LCSuff[i][j];
                        row = i;
                        col = j;
                    }
                }
                else
                    LCSuff[i][j] = 0;
            }
        }
    
        if (len == 0) {
            result = "";
            return result;
        }
    
        char* resultStr = (char*)malloc((len + 1) * sizeof(char));
        result.resize(len);

        while (LCSuff[row][col] != 0) {
            result[--len] = str1[row - 1]; 
            row--;
            col--;
        }

        return result;

    }
    

    ~History() {
        write_history();
    }

    History() : filename("/.nicsh_history") {
        char fn[filename_len];
        getcwd(fn, filename_len);
        string fn_str = string(fn);
        fn_str.append(filename);
        filename = fn_str;
        list.resize(history_len);
        last_entered = read_history();
    }

    // Add to history
    void addToHistory(string line) {
        list[(last_entered++)%history_len] = line;

        if (last_entered == history_len)
        {
            last_entered = 0;
            exceeded = true;
        }
    }

    // Get 1000 latest
    void getLatest() {
        cout << "History: " << endl;
        for (int i = 0; i < return_len; i++)
        {
            int ind = (last_entered-i+history_len)%history_len;
            string str = list[ind];
            if (str != "")
                cout << "\t" << str << endl;
        }
    }

    // Search for best answer
    set<string> search(string item) {
        set<string> result;

        int max_len = 0;

        for (int i = 0; i < return_len; i++)
        {
            int ind = (last_entered-i+history_len)%history_len;
            string str = list[ind];
            if (str != "")
            {
                string lc = longest_substring(str, item);
                if (lc.length() > max_len)
                {
                    result.clear();
                    max_len = lc.length();
                    result.insert(list[ind]);
                }
                else if (lc.length() == max_len)
                {
                    result.insert(list[ind]);
                }
            }
                
        }
        if (max_len >= 2)
            return result;
        else {
            result.clear();
            return result;
        }
    }

    void performSearch() {
        cout << "\nEnter the search term: ";
        string searchString;

        getline(cin, searchString);

        set<string> result = search(searchString);

        if (result.size() == 0)
            cout << "No match found in history" << endl;
        else {
            for (string s: result) {
                cout << s << endl;
            }
        }
    }
};

History history;

void autoComplete(string &line) {

    string printedString;
    ostringstream printedStream(printedString);

    istringstream ss(line);
  
    string last_token; // for storing last token
  
    while (ss >> last_token) 
        ;

    vector <string> fileList;
	struct dirent * file;
	DIR * dir = opendir(".");
	while ((file = readdir(dir)) != NULL)
	{
		fileList.push_back(string(file->d_name));
	}

    printedStream << "\n";

    vector <string> options;
	for (auto x : fileList)
	{
		// check if queryString is a prefix
		if (x.length() >= last_token.length() && x.substr(0, last_token.length()) == last_token)
		{
			options.push_back(x);
		}
	}

    string answer = "";
    if (options.size() == 0)
    {
        ;
    }
    else if (options.size() == 1)
	{
		answer = options[0]; 
	}
	else
	{
		int count = 0;
		for (auto x : options)
		{
			printedStream << ++count << ") " << x << "\n"; 
		}

        printedStream << "Choice: ";
        cout << printedStream.str();
        cout.flush();

		string choice_string;
		getline(cin, choice_string);

        printedStream << choice_string << "\n";

        
        int choice = stoi(choice_string);
		answer = options[choice - 1];
        if (choice > 0 && choice <= options.size())
		{
			answer = options[choice - 1];
		}
	
    
    }

    printedStream << line << "\t";

    int str_len = printedStream.str().length();

    if (answer != "")
        line = line.substr(0, line.length()-last_token.length());

    line.append(answer);

}

int convertVectorToArray (vector<string> &vect, char** &arr) {

    int size = vect.size();

    arr = new char*[size + 1];

    for (int i = 0; i < size; i++) {

        int stringSize = vect[i].length();
        arr[i] = new char[stringSize];
        strcpy(arr[i], vect[i].c_str());

    }
    arr[size] = nullptr;

    return size + 1;
}

void executeMultiWatch(char *cmd_arr[], int cmd_len) {
    // syntax is multiWatch ["cmd1", "cmd2", "cmd3"]

    vector<vector<string>> commands;

    for (int i = 1; i < cmd_len-1; i++)
    {
        int front = 0;
        int back = strlen(cmd_arr[i]);

        if (cmd_arr[i][front] == '[')
            front++;
        if (cmd_arr[i][front] == '"')
        {
            vector<string> n;
            n.clear();
            commands.push_back(n);
            front++;
        }

        if (cmd_arr[i][back-1] == ']')
            back--;  
        if (cmd_arr[i][back-1] == ',')
            back--;
        if (cmd_arr[i][back-1] == '"')
            back--;

        char *cur_word = new char[back-front+1];
        strncpy(cur_word, cmd_arr[i] + front, back-front);
        cur_word[back-front] = '\0';

        int cur_ind = commands.size()-1;
        commands[cur_ind].push_back(string(cur_word));       

    }

    int command_count = commands.size();

    inotify_fd = inotify_init();
    if (inotify_fd == -1)
    {
        perror("inotify_init");
    }

    vector <int> readFD (command_count), writeFD (command_count), inotify_WD (command_count);
    for (int i = 0; i < command_count; i++)
    {
        string filename = ".temp.PID" + to_string(i + 1) + ".txt";
        writeFD[i] = open(filename.c_str(), O_WRONLY | O_CREAT, 0666);
        readFD[i] = open(filename.c_str(), O_RDONLY, 0666);
        int inotify_wd = inotify_add_watch(inotify_fd, filename.c_str(), IN_MODIFY);
        inotify_WD[i] = inotify_wd;
    }

    vector <pid_t> process_ids (command_count);

    int pid1 = fork();

    if (pid1 == 0) {
        do {
            for (int i = 0; i < command_count; i++) {
                process_ids[i] = fork();

            if (process_ids[i] < 0) {
                fprintf(stderr, "Could not create process. Exitting\n");
                exit(0);
            } else if (process_ids[i] == 0) { 


                char** cmd_arr;
                convertVectorToArray(commands[i], cmd_arr);
                dup2(writeFD[i], STDOUT_FILENO);
                if (execvp(cmd_arr[0], cmd_arr) == -1)
                {
                    exit(1);
                }
            }
        }
        sleep(1);
        // if (stopProcess) exit(0);
        } while (false);

        exit(0);
    }
    
    while (true) {

        char buf[BUF_LEN]
            __attribute__((aligned(__alignof__(struct inotify_event))));
        int len = read(inotify_fd, buf, sizeof(buf));

        // cout << stopProcess << endl;
        if (len <= 0) break;

        int i = 0;

        while (i < len) {
            struct inotify_event *event = (struct inotify_event *)&buf[i];

            if (event->mask & IN_MODIFY) {
                int wd = event->wd;

                int fileIndex;
                for (int i = 0; i < command_count; ++i)
                {
                    if (inotify_WD[i] == wd) {
                        fileIndex = i;
                        break;
                    }
                }

                cout << "\"";
                for (string s: commands[fileIndex])
                {
                    cout << s << " ";
                }
                cout << "\"";

                cout << " , current_time: " << (unsigned long)time(0) << ".\n";
                cout << "<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-\n";
                
                char read_buf[BUF_LEN + 1];
                for (int i = 0; i <= BUF_LEN; i++) {
                    read_buf[i] = '\0';
                }

                while (read(readFD[fileIndex], read_buf, BUF_LEN) > 0) {
                    cout << read_buf << endl;
                    for (int i = 0; i <= BUF_LEN; i++) {
                        read_buf[i] = '\0';
                    }
                }

                cout << "->->->->->->->->->->->->->->->->->->->->\n\n";
                cout.flush();
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }

    cout << "Removing files" << endl;
    for (int i = 0; i < command_count; i++) {
        // kill(process_ids[i], SIGKILL);
        char *fileName = (char *)malloc(BUF_LEN * sizeof(char));

        asprintf(&fileName, ".temp.PID%d.txt", i + 1);
        remove(fileName);
    }
}

void splitCommand (string str, vector<string> &cmd) {

    // Used to split string around spaces.
    istringstream ss(str);
  
    string word; // for storing each word
  
    while (ss >> word) 
    {
        cmd.push_back(word);
    }

}

int getPipedCommand (vector<string> &cmd, vector<vector<string>> &pipedCommands) {

    int start = 0;
    string str;
    for (int i = 0; i < cmd.size(); i++) {

        str = cmd[i];
        if (str == "|") {
            // cout << cmd[i-1] << " ";
            vector<string> sub_vect (cmd.begin() + start, cmd.begin() + i);
            pipedCommands.push_back(sub_vect);
            start = i+1;
        }

    }

    vector<string> sub_vect (cmd.begin() + start, cmd.end());
    pipedCommands.push_back(sub_vect);

    return pipedCommands.size();

}



void executeCommand(char *cmd_arr[], int cmd_len, int infd = 0, int outfd = 1)
{
    // Handling Custom Commands
    string command_name = string(cmd_arr[0]);
    if (command_name == "cd") {
        if(chdir(cmd_arr[1])!=0)
        {
            cout << "Error: directory not found\n";
        }
        return;
    }
    else if (command_name == "history") {
        history.getLatest();
        return;
    }
    else if (command_name == "exit") {
        exit(0);
        return;
    }
    else if (command_name == "multiwatch") {
        executeMultiWatch(cmd_arr, cmd_len);
        return;
    }

    // check if input or output redirection or if it has "&"
    char *infile = nullptr, *outfile = nullptr;
    bool bg = false;
    int cmd_end = cmd_len - 1;
    for (int i = 0; i < cmd_len - 1; i++)
    {
        if (strcmp(cmd_arr[i], "<") == 0)
        {
            if (i == cmd_len - 1)
            {
                cout << "syntax error" << endl;
                return;
            }
            infile = cmd_arr[i + 1];
            cmd_end = min(cmd_end, i);
        }
        if (strcmp(cmd_arr[i], ">") == 0)
        {
            if (i == cmd_len - 1)
            {
                cout << "syntax error" << endl;
                return;
            }
            outfile = cmd_arr[i + 1];
            cmd_end = min(cmd_end, i);
        }
        if (strcmp(cmd_arr[i], "&") == 0)
        {
            bg = true;
            cmd_end = min(cmd_end, i);
        }
    }

    cmd_arr[cmd_end] = NULL;

    // int infd = 0, outfd = 1;

    pid_t x = fork();
    if (x == 0)
    {

        if (infd != 0) {

            // cout << "Taking input from pipe" << endl;
            dup2(infd, STDIN_FILENO);
            close(infd);
        }

        if (outfd != 1) {

            // cout << "Sending output to pipe" << endl;
            dup2(outfd, STDOUT_FILENO);
            close(outfd);
        }

        if (infile != nullptr)
        {
            // cout << "Taking input from " << infile << endl;
            infd = open(infile, O_RDONLY);
            if (infd == -1)
            {
                cout << "cannot open the input file" << endl;
                return;
            }
            dup2(infd, STDIN_FILENO);
        }
        if (outfile != nullptr)
        {
            // cout << "Writing output to " << outfile << endl;
            outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (outfd == -1)
            {
                cout << "cannot open the output file" << endl;
                return;
            }
            dup2(outfd, STDOUT_FILENO);
        }
        
        
        execvp(cmd_arr[0], cmd_arr);
        exit(0);
    }
    else if (x > 0)
    {
        int status = 0;
        int wpid;
        if (!bg)
        {
            do
            {
                // cout << "ExecuteCommand" << SIGCONT_flag << endl;
                if (SIGCONT_flag)
                {
                    kill(x, SIGCONT);
                    bg = true;
                    SIGCONT_flag = 0;
                    break;
                    // flag = 0;
                }
                wpid = waitpid(x, &status, WUNTRACED|WCONTINUED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }
    else
    {
        cout << "ded" << endl;
    }
}

string getLine () {
    string result = "";

    // 0: Canonical
    // 1: Non-Canonical
    int inputMode = 0;

    char c;

    while (true) {

        if (inputMode == 0) {
            inputMode = 1;
            setInputMode();
        }

        c = getchar();

        if ((int) c == 127) {

            if (result.length() == 0)
                continue;
            
            result.pop_back();
            cout << "\b \b";
            cout.flush();
        }
        else if ((int) c == 18) {

            int str_len = result.length();

            for (int i = 0; i < str_len; i++) {
                cout << "\b \b";
                cout.flush();
            }

            inputMode = 0;
            resetInputMode();
            // cout << "Ctrl + R Pressed" << endl;
            history.performSearch();

            return string("");
        }
        else if ((int) c == 9) {

            inputMode = 0;
            resetInputMode();
            // cout << "Tab Pressed" << endl;
            

            autoComplete(result);
            // Return appropitaly
            cout << "\n\nnicsh >>> ";
            cout << result;

        }
        else if ((int) c == 4) {
            result.clear();
            result.append("<<>>");
            resetInputMode();
            return result;
        }
        else if (c == '\n' || c == EOF) {
            break;
        }
        else {
            result.push_back(c);
            cout << c;
            cout.flush();
        }

    }

    resetInputMode();
    cout << "\n";
    // cout << result << endl;
    return result;

}

void ctrl_C_Handler(int dum) {
  stopProcess = 1;
  fprintf(stdout, " Ctrl C Detected.\n");
  close(inotify_fd);
  return;
}

void ctrl_Z_Handler(int dum) {
  fprintf(stdout, " Ctrl Z Detected.\n");
  SIGCONT_flag = 1;
  cout << SIGCONT_flag << endl;
//   cout << y << endl;
  kill(y, SIGCONT);
}

int main (void) {

    signal(SIGINT, ctrl_C_Handler);
    signal(SIGTSTP, ctrl_Z_Handler);

    // cout << getpid() << endl;

    string prompt = "\nnicsh >>> ";
    string line;
    int number_pipes;
    char ** cmd_arr;
    int cmd_len;

    int status;

    while (1) {
        
        cout << prompt;

        string line = getLine();

        if (line == "<<>>") {
            cout << "exit\n";
            break;
        }
        
        if (line == "") {
            continue;

        }

        vector<string> cmd;
        vector<vector<string>> pipedCommand;

        history.addToHistory(line);
        splitCommand(line, cmd);

        number_pipes = getPipedCommand(cmd, pipedCommand);

        if (number_pipes == 1) {

            cmd_len = convertVectorToArray(pipedCommand[0], cmd_arr);

            executeCommand(cmd_arr, cmd_len);
        }
        else if (number_pipes > 1) {

            int infd = 0;
            int FD[2];
            int pipeError = 0;

            for (int i = 0; i < number_pipes - 1; i++) {

                if (pipe(FD) == -1) {
                    pipeError = 1;
                    break;
                }

                cmd_len = convertVectorToArray(pipedCommand[i], cmd_arr);

                executeCommand(cmd_arr, cmd_len, infd, FD[1]);

                close(FD[1]);
                infd = FD[0];

            }
            if(!pipeError)
            {
                cmd_len = convertVectorToArray(pipedCommand[number_pipes-1], cmd_arr);

                executeCommand(cmd_arr, cmd_len, infd);
            }

        }


    }

}

