#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#define delimiter " "

char* readLine()
{
  char* line = (char*) malloc(1024*sizeof(char));
  gets(line);
  return line;
}

void quit()
{
  exit(0);
}

void currentDirectory()
{
  system("pwd");
}

void cd(char* path)
{
  if(path == NULL || strlen(path) == 0)
  {
    chdir(getenv("HOME"));
    free(path);
  }
  else if(chdir(path) != 0)
  {
    printf("cd: %s: No such file or directory\n", path);
    free(path);
  }
}

char** breakLine(char* line)
{
  int part = 0, size = 32;
  char **args = (char**) malloc(size * sizeof(char*));
  char *token;

  token = strtok(line, delimiter);
  while(token != NULL)
  {
    args[part] = token;
    part++;
    if(part >= size)
    {
      size += 32;
      args = realloc(args, size * sizeof(char*));
    }
    token = strtok(NULL, delimiter);
  }
  args[part] = NULL;
  return args;
}

void run(char** arguments, int background)
{
  pid_t processID;
  int status;
  processID = fork();
  if(processID == 0)
  {
    //child process

    if(execvp(arguments[1], arguments) == -1)
    {
      printf("Running %s failed\n", arguments[1]);
    }
    exit(1);
  }
  else if (processID < 0) {
    //fork failed
    printf("Process failed to fork\n");
  }
  else if(background == 0)
    {
        //parent process waiting for child to end
        waitpid(processID, &status, WUNTRACED);
    }
}

//Function to tell where the piping/redirection starts
int pipeRdrChk(char** arguments)
{
  int i = 0;

  while (arguments[i] != NULL)
  {
    if(strcmp(arguments[i], "|") == 0)
    {
      return i;
    }
    else if(strcmp(arguments[i], ">") == 0)
    {
      return i;
    }
    i++;
  }

  return -1;
}

void piping(char** arguments)
{
  int pipefd[2];
  pid_t pid;
  int status;
  int pipeIndex = pipeRdrChk(arguments);
  char **cmd1, **cmd2;
  arguments[pipeIndex] = NULL;
  cmd2 = &arguments[pipeIndex+1];
  cmd1 = arguments;

  if(pipe(pipefd) != 0) //pipefd[0] is for reading, pipefd[1] is for writing
  {
    printf("Failed to pipe\n");
  }

  pid = fork();

  if(pid == 0)
  {
    //child process
    //reading output from first program
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);
    execvp(cmd1[1], cmd1);
  }
  else
  {
    //parent process
    //writing input to second program
    wait(NULL);
    pid = fork();
    if(pid == 0)
    {
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);
    execvp(cmd2[1], cmd2);
    }
    else{
      wait(NULL);
    }
  }
}

void redirecting(char** arguments)
{
  pid_t pid;
  int duplicate;
  int j = pipeRdrChk(arguments);
  j++;
  arguments[j-1] = NULL;
  pid = fork();
  if(pid == 0)
  {
    //Child process in charge of opening the output file
    duplicate = open(arguments[j], O_CREAT | O_WRONLY, 0600);
    dup2(duplicate, STDOUT_FILENO);
    close(duplicate);

    //checking if the first process is a basic run command or something else
    if(strcmp(arguments[0], "ex") == 0)
    {

      if(execvp(arguments[1], arguments) == -1)
      {
        printf("running %s failed\n", arguments[1]);
        system("setterm -bold off");
        exit(1);
      }
    }
    else
    {
      if(execvp(arguments[0], arguments) == -1)
      {
        printf("running %s failed\n", arguments[0]);
        exit(1);
      }
    }
  }
  else if(pid < 0)
  {
    printf("Failed to fork\n");
  }
  else
  {
    //parent process waiting for child to finish
    wait(NULL);
  }
}

int main (int argc, char **argv)
{
  char* line;
  char** arguments;
  do
  {
    printf("$ ");
    line = readLine();
    system("setterm -bold on");
    arguments = breakLine(line);

    //case of emty line
    if (strcmp(line, "\0") == 0)
    {
      continue;
    }
    else if(pipeRdrChk(arguments) != -1)
    {
      int index = pipeRdrChk(arguments);
      if (strcmp(arguments[index], "|") == 0)
      {
        piping(arguments);
        //printf("\npiping done!\n");
      }
      else if (strcmp(arguments[index], ">") == 0 )
      {
        redirecting(arguments);
      }
    }
    else if(strcmp(arguments[0], "exit") == 0)
    {

      system("setterm -bold off");
      quit();
    }
    else if(strcmp(arguments[0], "pwd") == 0)
    {
      currentDirectory();
    }
    else if(strcmp(arguments[0], "info") == 0)
    {
      printf("COMP2211 Simplified Shell by SC16AMEG\n");
    }
    else if(strcmp(arguments[0], "cd") == 0)
    {
      cd(arguments[1]);
    }
    else if(strcmp(arguments[0], "ex") == 0)
    {
      run(arguments, 0);
    }
    else if(strcmp(arguments[0], "exb") == 0)
    {
      run(arguments, 1);
    }
    else
    {
      printf("%s: command not found...\n", arguments[0]);
    }
      system("setterm -bold off");

    free(line);
    free(arguments);
  }
  while(1);
}
