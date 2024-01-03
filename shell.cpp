#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>
#include <string>
#include <ctime>
#include<time.h>
#include "Tokenizer.h"
#include <cstring>
#include <fstream>
#include <time.h>
#include <fcntl.h>

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;


int main () {
    //create copies of stdin/stdout; dup()
    int stdin = dup(0); //stdin
    int stout = dup(1); //stdout
    vector<pid_t> pids;
    char curdir[100];
    char prevdir[100];
    int status = 0;
    for (;;) {
        //impletement iteration over 
        //implement date/time with TODO
        //implement username with getlogin()
        //implement curdir with getcwd()
        // need date/time, username, and absolute path to current dir

        getcwd(curdir,100);
        time_t rawtime;

        time (&rawtime);
        string time = ctime(&rawtime);
        time.replace(20,5,"");
        time.replace(0,4,"");   
        time.replace(3,1,"");
        cout << YELLOW << time <<getenv("USER") << ":" <<  curdir << "$" << NC << " ";
        
        // get user inputted command
        string input;
        getline(cin, input);
        
        for(size_t j = 0; j < pids.size(); j++){
            pid_t bg_pid = waitpid(pids[j], &status, WNOHANG);
            if(bg_pid != 0){
                pids.erase(pids.begin()+j);
                j--;
            }
        }
        
        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }
        
            // get tokenized commands from user input
            Tokenizer tknr(input);
            if (tknr.hasError()) {  // continue to next prompt if input had an error
                continue;
            }


            if (tknr.commands[0]->args[0] == "cd"){
                int ch;
                char* arr;
                if(tknr.commands[0]->args[1] == "-"){
                    getcwd(curdir,100);
                    cerr << "curr directory: " << curdir << endl;
                    cerr << "prev directory: " << prevdir << endl;
                    ch = chdir(prevdir);
                }
                else{
                    arr = new char[tknr.commands[0]->args[1].size() + 1];
                    strcpy(arr, tknr.commands[0]->args[1].c_str()); 
                    ch = chdir(arr);
                    delete[] arr;
                }
                if(ch<0) cerr << ("chdir change of directory not successful\n") << endl;
                else cerr << ("chdir change of directory successful") << endl;
                memcpy(&prevdir, &curdir, sizeof(curdir));
            }
            else{
                    
                // // print out every command token-by-token on individual lines
                // // prints to cerr to avoid influencing autograder
                for (auto cmd : tknr.commands) {
                    for (auto str : cmd->args) {
                        cerr << "|" << str << "| ";
                    }
                    if (cmd->hasInput()) {
                        cerr << "in< " << cmd->in_file << " ";
                    }
                    if (cmd->hasOutput()) {
                        cerr << "out> " << cmd->out_file << " ";
                    }
                    cerr << endl;
                }

                //for piping
                //for cmd : commands
                //      call pipe() to make pipe
                //      fork() - in child, redirect stdout; in par, redirect stdin
                //         ^ is already written
                //add checks for first/last command
                
                int fd[2];
                //save original stdin and stdout
                for (long unsigned int i = 0; i < tknr.commands.size(); i++){
           
                    // Create pipe
                    pipe(fd);
                    // Create child to run first command
                    char ** args = nullptr;

                    pid_t pid = fork();

                    if (pid < 0) {  // error check
                        perror("fork");
                        exit(2);
                    }
                    // add check for bg process - add pid to vector if bg and don't waitpid() in parent
                    if(pid == 0){
                        // In child, redirect output to write end of pipe
                        if(i < tknr.commands.size()-1){
                            dup2(fd[1],1);
                        }
                        // Close the read end of the pipe on the child side.
                        close(fd[0]);
                        // In child, execute the command
                        int arg_size = tknr.commands[i]->args.size();
                        char ** args = new char* [arg_size+1];
                        // run single commands with no arguments
                        // implement multiple arguments - iterate over args of current command to make 
                        //      char * array
                        for (int j = 0; j < arg_size; j++){
                            args[j]  = (char*)tknr.commands[i]->args[j].c_str();
                        }
                        args[(arg_size)]=nullptr;
                        // if current command is redirected, then open file and dup2 std(in/out) that's being redirected
                        //implement is safely for both at same time
                        if(tknr.commands[i]->hasOutput()){
                            int file_desc = open((tknr.commands[i]->out_file).c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
                            dup2(file_desc, 1);
                        }
                        if (tknr.commands[i]->hasInput()){
                            //need to save contents of file into buffer 
                            int file_desc = open((tknr.commands[i]->in_file).c_str(), O_RDONLY, 0600);
                            //int file_desc = open((tknr.commands[i]->in_file).c_str(), O_CREAT | O_TRUNC | O_WRONLY);
                            dup2(file_desc, 0);
                        }
                    
                        if (execvp(args[0], args) < 0) {  // error check
                            perror("execvp");
                            exit(2);
                        }

            
                    }
                    else{
                        //signal(SIGCHLD,SIG_IGN);
                        //  redirect the SHELL(PARENT)'s input to the read end of the pipe.
                        dup2(fd[0],0);
                        // Close the write end of the pipe
                        close(fd[1]);
                        delete[] args;
                        args = nullptr;
                

                        if(tknr.commands[i]->isBackground()){
                            pids.push_back(pid);
                        }
                        else{
                            waitpid(pid, &status, 0);
                        }
                    

                        if (status > 1) {  // exit if child didn't exec properly
                            exit(status);
                        }
                    }
                    close(fd[0]);
                    close(fd[1]);
                }
            }

    dup2(stdin,0);
    dup2(stout,1);
    }
    
}
