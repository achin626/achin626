#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <ctime>
#include <chrono>
using namespace std;

string trim (string input) {
    int len = input.length();
    bool foundspace = false;
    int i = 0;
    while(i < len) {
        if(i == 0 && input[i] == ' ') {
            input.erase(0, 1);
            len--;
            i--;
        } else {
            if(input[i] == ' ') {
                if(input[i + 1] == ' ') {
                    input.erase(i, 1);
                    len--;
                    i--;
                }
            }
            if(i == len - 1) {
                if(input[i] == ' ') {
                    input.erase(i, 1);
                }
            }
            i++;
        }
    }
    return input;
}

char** vec_to_char_array (vector<string> parts) {
    char** args = new char*[parts.size() + 1];
    args[parts.size()] = NULL;
    for(int i = 0; i < parts.size(); i++) {
        args[i] = new char[parts[i].size() + 1];
        strcpy(args[i], parts[i].c_str());
    }
    return args;
}

vector<string> split(string line, string separator=" ") {
    vector<string> parts;
    int pos = 0;
    int quote = 0;
    string part;
    if(separator == "|" || separator == "<" || separator == ">") {
        pos = line.find(separator);
        while(pos != string::npos) {
            quote = 0;
            for(int i = 0; i < pos; i++) {
                if(line[i] == '\'' || line[i] == '\"') {
                    quote++;
                }
            }
            if(quote % 2) {
                pos = line.find(separator, pos + 1);
            } else {
                part = line.substr(0, pos);
                part = trim(part);
                parts.push_back(part);
                line.erase(0, pos + separator.length());
                pos = line.find(separator);
            }
        }
        line = trim(line);
        parts.push_back(line);
    } else {
        while((pos = line.find(separator)) != string::npos) {
            if(line[pos+1] == '\"' || line[pos+1] == '\'') {
                part = line.substr(0, pos);
                parts.push_back(part);
                line.erase(0, pos + separator.length() + 1);
                line.erase(line.length() - 1, 1);
                //cout << line << endl;
                parts.push_back(line);
                line.erase(0, line.length());
            } else {
                part = line.substr(0, pos);
                //cout << part << endl;
                parts.push_back(part);
                line.erase(0, pos + separator.length());
            }
        
        }
    //cout << line << endl;
    parts.push_back(line);
    }
    return parts;
}

int main() {
    vector<int> bgs;
    char *lastpaths;
    char *time;
    auto timenow = chrono::system_clock::to_time_t(chrono::system_clock::now());
    lastpaths = getcwd(lastpaths, 0);
    while (true){
        for(int i = 0; i < bgs.size(); i++) {
            if(waitpid(bgs[i], 0, WNOHANG) == bgs[i]) {
                //cout << "Process: " << bgs[i] << " ended" << endl;
                bgs.erase(bgs.begin() + i);
                i--;
            } 
        }
        time = ctime(&timenow);
        cout << getenv("USER") << "@";
        for(int i = 0; i < 24; i++) {
            cout << time[i];
        }
        cout << "$ ";
        string inputline;
        getline (cin, inputline);
        inputline = trim(inputline);
        if(inputline == string("exit") || inputline == string("quit")) {
            cout << "Bye!! End of shell" << endl;
            break;
        }
        bool bg = false;
        if(inputline[inputline.size() - 1] == '&') {
            bg = true;
            //cout << "background process detected" << endl;
            inputline = inputline.substr(0, inputline.size() - 1);
            inputline = trim(inputline);
        }
        vector<string> pipes = split(inputline, "|");
        if(pipes.size() > 1) {
            dup2(0, 9);
            for(int i = 0; i < pipes.size(); i++) {
                int fds[2];
                pipe(fds);
                int cid = fork();
                if(cid == 0) {
                    if(i < pipes.size() - 1) {
                        dup2(fds[1], 1);
                    }
                    vector<string> c = split(pipes[i], ">");
                    vector<string> c2 = split(c[0], "<");
                    vector<string> parts = split(c2[0]);
                    int fd = 0;
                    if(c.size() > 1) {
                        fd = open(c[1].c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                        dup2(fd, 1);
                    }
                    if(c2.size() > 1) {
                        fd = open(c2[1].c_str(), O_CREAT|O_RDONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                        dup2(fd, 0);
                    }
                    char** args = vec_to_char_array(parts);
                    //cout << args[0] << args[1] << endl;
                    execvp(args[0], args);
                } else {
                    if(!bg) {
                        waitpid(cid, 0, 0);
                    } else {
                        bgs.push_back(cid);
                    }
                    if(i == pipes.size() - 1) {
                        waitpid(cid, 0, 0);
                    }
                    dup2(fds[0], 0);
                    close(fds[1]);
                }
            }
            dup2(9, 0);
        } else {
            vector<string> c = split(inputline, ">");
            vector<string> c2 = split(c[0], "<");
            vector<string> parts = split(c2[0]);
            if(parts[0] == "cd") {
                if(parts[1] == "-") {
                    chdir(lastpaths);
                } else if(parts[1] != ".") {
                    lastpaths = getcwd(lastpaths, 0);
                    chdir(parts[1].c_str());
                }
            } else {
                int pid = fork();

                if(pid == 0) {
                    
                    int fd = 0;
                    if(c.size() > 1) {
                        fd = open(c[1].c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                        dup2(fd, 1);
                    }
                    if(c2.size() > 1) {
                        fd = open(c2[1].c_str(), O_CREAT|O_RDONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                        dup2(fd, 0);
                    }
                    char** args = vec_to_char_array(parts);
                    
                    execvp(args[0], args);
                } else {
                    if(!bg) {
                        waitpid(pid, 0, 0);
                    } else {
                        bgs.push_back(pid);
                    }
                }
            }
        }
    }
}