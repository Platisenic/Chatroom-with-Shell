# Online Chatroom with Shell

### Shell

The `npshell` support  the following features:

1. Execution of commands
2. Ordinary Pipe
3. Numbered Pipe
4. File Redirection

### Chatroom

There are 3 kinds of servers:

1. A concurrent connection-oriented server. This server allows one client connect to it and execute customize shell commands.
2. In this system, users can communicate with other users. The server is designed using single-process concurrent paradigm.
3. In this system, users can communicate with other users. The server is designed using the concurrent connection-oriented paradigm with shared memory and FIFO.

In server1:

- Support all npshell command.

In server 2 and 3:

- Pipe between different users.  Broadcast message whenever a user pipe is used.
- Broadcast message of login/logout information.
- New built-in commands:
  - who:  show information of all users.
  - tell:  send a message to another user.
  - yell:  send a message to all users.
  - name:  change your name.
- Support all npshell command.



## How to use

1. Build from source

   ```shell
   cd online-chatroom-with-shell
   make
   ```

2. Start the server you want.

   A. Start the first simple server (concurrent connection-oriented server)
   ```shell
   ./np_simple [port]
   ```

   B. Start the second  server (single-process concurrent paradigm)
      ```shell
      ./np_single_proc [port]
      ```

   C. Start the third simple server (concurrent connection-oriented paradigm)

     ```shell
     ./np_multi_proc [port]
     ```

3. Connect to server by `nc`
   ```shell
   nc [ip] [port]
   ```

## Implementation detail

### NPShell

1. Behavior
   - Use "%" as the command line prompt.  Notice that there is one space character after "%".
   - The npshell parses the inputs and executes commands.
   - The npshell terminates after receiving theexitcommand or EOF.
   - There will NOT exist the test case that commands need to read from STDIN.
2. Built-in Commands
   - `setenv [var] [value]`
     - Change or add an environment variable.
   - `printenv [var]`
     - Print the value of an environment variable.
   - `exit`
     - Terminate npshell.
3. If there is an unknown command, print error message to STDERR with the following format: **Unknown command:  [command]**.
4.  Ordinary Pipe and Numbered Pipe
   - Implement pipe (cmd1 | cmd2), which means the STDOUT of the left hand sidecommand will be piped to the right hand side command.
   - Implement a special piping mechanism, called numbered pipe.  There are two types of numbered pipe (cmd|N and cmd !N).
     - `|N` means the STDOUT of the left hand side command will be piped to the first command of the next N-th line.
     - `!N` means both STDOUT and STDERR of the left hand side command will be piped to the first command of the next N-th line.
   - When the output of one command is piped (no matter ordinary pipe or numbered pipe), it is likely that the output of this command exceeds the capacity of the pipe.  In this case, we still need to guarantee that all the output of this command is piped correctly.
   - The number of piped commands may exceed the maximum number of processes that one user can run.  In this case, we still need to guarantee that all commands are executed properly.
5. File Redirection
   -  Implement standard output redirection (cmd > file), which means the output of the command will be written to files.

###  Chatroom

1. Built-in Commands

   - `who`
     - Show information of all users.
   - `tell [userid] [message]`
     - Tell the user some message
   - `yell [message]`
     - Broadcast the message
   - `name [new_name]`
     - Change username and broadcast the message.

2. User Pipe

   - The formats of using user pipe are (command > n) and (command < n). 

     - \>n pipes result (ONLY stdout) of command into the pipe.
     - \<n reads contents from the pipe.

## Scenario

```shell
bash$ nc nplinux1.nctu.edu.tw 7001
***************************************
** Welcome to the information server **
***************************************                     # Welcome message
*** User ’(no name)’ entered from 140.113.215.62:1201. ***  # Broadcast message of user login
% who
<ID>    <nickname>  <IP:port>           <indicate me>
1       (no name)   140.113.215.62:1201 <- me
% name Pikachu
*** User from 140.113.215.62:1201 is named ’Pikachu’. ***
% ls
bin test.html
% *** User ’(no name)’ entered from 140.113.215.63:1013. *** # User 2 logins
who
<ID>    <nickname>  <IP:port>           <indicate me>
1       Pikachu       140.113.215.62:1201 <- me
2       (no name)   140.113.215.63:1013
% *** User from 140.113.215.63:1013 is named ’Snorlax’. ***    # User 2 inputs ’name Snorlax’
who
<ID>    <nickname>  <IP:port>           <indicate me>
1       Pikachu       140.113.215.62:1201 <- me
2       Snorlax       140.113.215.63:1013
% *** User ’(no name)’ entered from 140.113.215.64:1302. *** # User 3 logins
who
<ID>    <nickname>  <IP:port>           <indicate me>
1       Pikachu       140.113.215.62:1201 <- me
2       Snorlax       140.113.215.63:10133       (no name)   140.113.215.64:1302
% yell Who knows how to do project 2? help me plz!
*** Pikachu yelled ***: Who knows how to do project 2? help me plz!
% *** (no name) yelled ***: Sorry, I don’t know. :-(         # User 3 yells
*** Snorlax yelled ***: I know! It’s too easy!                 # User 2 yells
% tell 2 Plz help me, my friends!
% *** Snorlax told you ***: Yeah! Let me show you how to send files to you!   # User 2 tells to User 1
*** Snorlax (#2) just piped ’cat test.html >1’ to Pikachu (#1) *** # Broadcast message of user pipe
*** Snorlax told you ***: You can use ’cat <2’ to show it!
cat <5                                                       # mistyping
*** Error: user #5 does not exist yet. ***
% cat <2                                                     # receive from the user pipe
*** Pikachu (#1) just received from Snorlax (#2) by ’cat <2’ ***
<!test.html>
<TITLE>Test<TITLE>
<BODY>This is a <b>test</b> program for rwg.</BODY>
% tell 2 It’s works! Great!
% *** Snorlax (#2) just piped ’number test.html >1’ to Pikachu (#1) ***
*** Snorlax told you ***: You can receive by your program! Try ’number <2’!
number <2
*** Pikachu (#1) just received from Snorlax (#2) by ’number <2’ ***
1 1 <!test.html>
2 2 <TITLE>Test<TITLE>
3 3 <BODY>This is a <b>test</b> program for rwg.</BODY>
% tell 2 Cool! You’re genius! Thank you!
% *** Snorlax told you ***: You’re welcome!
*** User ’Snorlax’ left. ***
exit
bash$
```


