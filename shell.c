#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>


typedef struct
{
	char** tokens;
	int numTokens;
} instruction;

char *aliasName[10];
char *actualCommand[10];

int counter = 0;

// flags
int redirection = 0;
int piper = 0;
int background = 0;
char * iofile;

void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);

void searchForEnvironmentalVariables(instruction* instr_ptr);
char* FindShortcut(instruction* instr_ptr, char* path);
void pathResolution(instruction * instr_ptr, char* tempPath);
char* searchCommands(instruction* instr_ptr, int numOfExecCommands, char* path);
int fileCheck(char * path);
void my_execute(char ** cmd);

void createAlias(instruction* instr_ptr);
void removeAlias(instruction * instr_ptr);
int checkAlias(instruction * instr_ptr);
//void my_execute(char ** cmd);
int errorCheckingIORedirection(instruction * instr_ptr);

int main() {
	char* token = NULL;
	char* temp = NULL;

	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;

	int executing_commands = 0;

	char* path = getenv("PWD");

	counter = 0;

	while (1) {

		//printf("Please enter an instruction: ");

		//part 2
		printf("%s@%s : %s> ", getenv("USER"), getenv("MACHINE"), getenv("PWD"));

		// loop reads character sequences separated by whitespace
		do {
			//scans for next token and allocates token var to size of scanned token
			scanf("%ms", &token);
			temp = (char*)malloc((strlen(token) + 1) * sizeof(char));

			int i;
			int start = 0;
			for (i = 0; i < strlen(token); i++) {
				//pull out special characters and make them into a separate token in the instruction
				if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') {

					if (token[i] == '>') redirection = 1;
          				if (token[i] == '<') redirection = 2;
          				if (token[i] == '|') piper = 1;

					if(i-start > 0) {
						memcpy(temp, token + start, i - start);
						temp[i-start] = '\0';
						addToken(&instr, temp);
					}

					char specialChar[2];
					specialChar[0] = token[i];
					specialChar[1] = '\0';

					addToken(&instr,specialChar);

					start = i + 1;
				}

			}

			if (start < strlen(token)) {
				memcpy(temp, token + start, strlen(token) - start);
				temp[i-start] = '\0';
				addToken(&instr, temp);
			}


			//free and reset variables
			free(token);
			free(temp);

			token = NULL;
			temp = NULL;


		} while ('\n' != getchar());    //until end of line is reached

		//executing_commands++;
		//printf("%d\n", instr.numTokens);
		//printf("number of tokens in main: %d\n", instr.numTokens);
		
		int num = errorCheckingIORedirection(&instr);		
		if(num == 1)
			executing_commands++;
	
		iofile = instr.tokens[instr.numTokens-1];

		//path = FindShortcut(&instr,path);
		addNull(&instr);
		//printTokens(&instr);
		//searchForEnvironmentalVariables(&instr);
		//strcpy(path, FindShortcut(&instr, path));
		path = searchCommands(&instr, executing_commands, path);

		// reset flags
		redirection = 0;
    		piper= 0;
    		background = 0;

		clearInstruction(&instr);
		//printf("%d\n", counter);


	}

	return 0;
}

//reallocates instruction array to hold another token
//allocates for new token within instruction array
void addToken(instruction* instr_ptr, char* tok)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**) malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**) realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	//allocate char array for new token in new slot
	instr_ptr->tokens[instr_ptr->numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
	strcpy(instr_ptr->tokens[instr_ptr->numTokens], tok);

	instr_ptr->numTokens++;
}

void addNull(instruction* instr_ptr)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**)malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	instr_ptr->tokens[instr_ptr->numTokens] = (char*) NULL;
	instr_ptr->numTokens++;
}

void printTokens(instruction* instr_ptr)
{
	int i;
	printf("Tokens:\n");
	for (i = 0; i < instr_ptr->numTokens; i++) {
		if ((instr_ptr->tokens)[i] != NULL)
			printf("%s\n", (instr_ptr->tokens)[i]);
	}
}

void clearInstruction(instruction* instr_ptr)
{
	int i;
	for (i = 0; i < instr_ptr->numTokens; i++)
		free(instr_ptr->tokens[i]);

	free(instr_ptr->tokens);

	instr_ptr->tokens = NULL;
	instr_ptr->numTokens = 0;
}

//part 3
void searchForEnvironmentalVariables(instruction* instr_ptr){		//searches to see if it is an echo command and gets environment variable it wants to print
        int i;
        char * environvar;						//char array for second token
	
	char *temp;
        if(strcmp((instr_ptr->tokens)[0], "echo") == 0){		//makes sure that command is echo
                environvar = (instr_ptr->tokens)[1];			//assigns environvar to second token (what we want to print)
                //environvar[sizeof((instr_ptr->tokens)[1])] = '\0';
		//printf("%s\n", environvar);

		if(environvar[0] != '$'){				//prints token if it didn't have a $ in the beginning
                        printf("%s\n", environvar);
                        return;
                }		
                for(i = 1; i < strlen((instr_ptr->tokens)[1]); i++){	//loop to shift everything to the left by 1 to get rid of '$'
	                        environvar[i-1] = environvar[i];		
                }
                environvar[i-1] = '\0';					//makes sure that the last character in the string is the null character
		
		if(getenv(environvar))
		        printf("%s\n", getenv(environvar));			//prints the environvar value
		else
			fprintf(stderr, "%s does not exist.\n", environvar);		//if token is not a environment variable, prints error
        }
}

char* FindShortcut(instruction* instr_ptr, char* path)
{
	char* temp;


	if(strcmp((instr_ptr->tokens)[0], "~") == 0 || strcmp((instr_ptr->tokens)[1], "~") == 0)	//changes to home directory	
	{

		//home directory path
		chdir(getenv("HOME"));
		temp = getcwd(path, 1024);
		//printf("%s\n", temp);		
		return temp;
	}

	else if(strcmp((instr_ptr->tokens)[0], "/") == 0 || strcmp((instr_ptr->tokens)[1], "/") == 0)	//changes to root directory
        {
		chdir(getenv("HOME"));
		chdir("..");
		chdir("..");
		chdir("..");
		
		temp = getcwd(path, 1024);	
		//temp = getenv("PWD");
		//printf("%s\n", temp);
		return temp;
        }

	else if(strcmp((instr_ptr->tokens)[1], ".") == 0)					//does nothing here
	{
		//current path
		//printf("%s\n", path);
		return path;

	}

	else if(strcmp((instr_ptr->tokens)[1], "..") == 0)					//changes to parent directory
	{
		if(strcmp(temp, "/") == 0)							//can't go to parent directory if 
		{										//   it is at the root directory
			printf("Cannot execute parent path of root directory\n");
			return path;
		}

		else
		{
			//printf("i come here \n");
			//temp = path;	
		
			//printf("%s\n", path);	
			chdir("..");

			temp = getcwd(path, 1024);
		
			//temp = getcwd(getenv("PWD"), 1024);

			//printf("%s\n", temp);

			return temp;
		}
	}
	else											//else changes to the directory given
	{
		//temp = getenv("PWD");
		temp = (instr_ptr->tokens)[1];
		chdir(temp);
		temp = getcwd(path, 1024);
		return temp;
	}

	free(temp);

}									

void pathResolution(instruction * instr_ptr, char* tempPath){
	char * temp = (instr_ptr->tokens)[1];

}
char* searchCommands(instruction* instr_ptr, int numOfExecCommands, char* path)				//built in commands function
{
	//printf("number of tokens in searchCommands: %d\n", instr_ptr->numTokens);
	char* temp;
	
	//char alias[10][100];
	if(strcmp((instr_ptr->tokens)[0], "exit") == 0){						//exits program
		printf("Exiting...\nCommands executed: %d\n", numOfExecCommands);
		exit(0);
	}

	else if(strcmp((instr_ptr->tokens)[0], "echo") == 0){						//echo command
		searchForEnvironmentalVariables(instr_ptr);
		return path;
	}
	
	else if(strcmp((instr_ptr->tokens)[0], "cd") == 0)						//cd command
	{
		if((instr_ptr->tokens)[1] == NULL)							//if only token is cd, changes to home directory
		{
			temp = getenv("HOME");
			chdir(temp);
			temp = getcwd(path, 1024);
			return temp;
		}
		else if(strcmp((instr_ptr->tokens)[1], "~")  == 0){					
			temp = FindShortcut(instr_ptr, path);
			return temp;
		}
		else if (fileCheck((instr_ptr->tokens)[1]) == -1){				//checks if directory is valid or not 
      			printf("No such directory\n");
      			temp = getenv("PWD");
			chdir(temp);
			temp = path;
      			return temp;
			
    		}
   		else{
      			temp = FindShortcut(instr_ptr, path);
      			return temp;
		}
	}

        else if(strcmp((instr_ptr->tokens)[0], "alias") == 0)					//alias command
        {
                createAlias(instr_ptr);
		return path;
        }

	else if(strcmp((instr_ptr->tokens)[0], "unalias") == 0)					//unalias command
	{
		removeAlias(instr_ptr);
		return path;
	}


	else{											//execute simple commands

		//if(redirection == 1 || redirection == 2)
		//	if(errorCheckingIORedirection(instr_ptr) != 1)
		//		return;
		my_execute(instr_ptr->tokens);
		return path;

	}
}

int fileCheck(char * path){									//checks if file or directory exists or not
	char * fname = path;
	if (access(fname, F_OK) == 0)
		return 0;
	else{
		return -1;
	}
}

void my_execute(char ** cmd){									//execute simple commands
//void my_execute(instruction * instr_ptr){
	//char **cmd = instr_ptr->tokens;
	int status;
	pid_t pid = fork();
	
	if(pid == -1){				
		fprintf(stderr, "Fork failed \n");
		exit(1);
	}
	else if(pid == 0){
		//printf("%s\n", temp);
		char temp[] = "/usr/bin/";
		strcat(temp, cmd[0]);
	
		if(redirection == 2){								//input redirection
			int fd = open(iofile, O_RDONLY, 0666);
			if (fd == -1){
				fprintf(stderr, "File does not exist\n");
				exit(1);
			}
			else{
				close(0);
				dup(fd);
				close(fd);
				free(iofile);
			}

			char ** temp2 = cmd;							//makes token associated with '<' equal to NULL
			int i;	
			for (i=0; temp2[i] != NULL; i++){
				if(strcmp(temp2[i], "<")==0){
					temp2[i] = NULL;
				}
			}
			execv(temp, temp2);							//executes command
		}
		else if (redirection == 1){							//output redirection
			int fd = open(iofile, O_CREAT|O_WRONLY, 0666);

			close(1);
			dup(fd);
			close(fd);
			free(iofile);
			
			char ** temp2 = cmd;							//makes token associated with '>' equal to NULL
                        int i;
                        for (i=0; temp2[i] != NULL; i++){
                                if(strcmp(temp2[i], ">")==0){
                                        temp2[i] = NULL;
                                }
                        }
			execv(temp, temp2);							//executes command
			printf("Problem executing %s\n", cmd[0]);

		}
		//if not io redirection then just execute normally
		else if (redirection == 0){							//not IO redirection
			execv(temp, cmd);
			printf("Problem executing %s\n", cmd[0]);
		}
		exit(1);
	}

	else{
		waitpid(pid, &status, 0);
	}
}


int errorCheckingIORedirection(instruction * instr_ptr){					//error checking for syntax errors in IO Redirection
	if(instr_ptr->numTokens == 1){
		if(strcmp((instr_ptr->tokens)[0], "<") == 0){
			fprintf(stderr, "Syntax error.\n");
			return 0;
		}

		if(strcmp((instr_ptr->tokens)[0], ">") == 0){
			fprintf(stderr, "Syntax error.\n");
			return 0;
		}
	}
	else if(instr_ptr->numTokens == 2){
		if(strcmp((instr_ptr->tokens)[0], "<") == 0  || strcmp((instr_ptr->tokens)[0], ">") == 0 || strcmp((instr_ptr->tokens)[1], "<") == 0  || strcmp((instr_ptr->tokens)[1], ">") == 0){
			fprintf(stderr, "Syntax error.\n");
			return 0;
		}
	}

	return 1;
}


void createAlias(instruction* instr_ptr)						//creating an alias
{											//adds alias to array to store key and value
	char * token = (instr_ptr->tokens)[1];
	char temp[100];

	if(counter > 9)
	{
		printf("You already have 10 aliases, remove some before adding more.\n");
	}
	else
	{

		if(checkAlias(instr_ptr) == 0)		 					//0 = there is an alias with that name
		{
			printf("An alias with that name already exists.\n");
		}
		else										//1 = no found in struct, okay to create
		{
		        char delim = '=';
			char quote = '\'';
			int i = 0;	
			int j= 0;

			for (i = 0; i < strlen(token); i++)
        		{
				if(token[i] != '=')
				{
					temp[i] = token[i];
				}
                		else
					break;
	       		}
			temp[i] = '\0';
			printf("%d\n", counter);
			
			aliasName[counter] = temp;
			

			char quoteArray[100];	
			j = 0;
			for(i = i + 1; i < strlen(token); i++)
			{
				int quote_counter = 0;

				if(token[i] == quote)
				{
					if(quote_counter == 0)
						quote_counter = quote_counter + 1;
					else
						break;		
				} 
				else
				{
					quoteArray[j]  = token[i];
					j++;
				}	

			}
			quoteArray[j] = '\0';
			actualCommand[counter] = quoteArray;
			counter = counter + 1;
			
			
			//printf("%s\n", aliasName[0]);
			//printf("%s\n", actualCommand[0]);
			//printf("%s\n", aliasName[1]);
                        //printf("%s\n", actualCommand[1]);
			//printf("%s\n", aliasName[2]);
                        //printf("%s\n", actualCommand[2]);
		}
	}
	int i;
	for(i = 0; i < counter; i++)
	{	
		printf("%d\n",i);	
		printf("%s\n",aliasName[i]); 

	}
}

void removeAlias(instruction* instr_ptr)					//checks to see if the wanted alias exists in the array
{										//	if so, it removes it from the array creating 
	//printf("%s\n", aliasName[0]);						//	space for another alias
        //printf("%s\n", actualCommand[0]);

	char * token = (instr_ptr->tokens)[1];
	char temp[100];
	char *checkName;

	if(checkAlias(instr_ptr) == 1)
	{
		printf("No alias with that name exists.\n");

        }
	else
	{
        	char quote = '\'';
        	int i = 0;


		for (i = 0; i < strlen(token); i++)
		{
	                if(token[i] != '=')
                        temp[i] = token[i];
                }
  	}
	
	checkName = temp;
	int location = 0;
        int j = 0;

        for(j = 0; j < 10; j++)
        {
                if(checkName == aliasName[j])
                {
                        location = j;
                }
                else
             		continue;
        }

	int i = 0;
	for(i = location; i < 10; i++)
	{
		aliasName[location] = aliasName[location + 1];
		actualCommand[location] = actualCommand[location +1];
	}

	//counter = counter - 1;
	
	printf("%s\n", aliasName[0]);
	printf("%s\n", actualCommand[0]);
}

int checkAlias(instruction* instr_ptr)							//checks if the alias is already in the array 	
{
	char * token = (instr_ptr->tokens)[1];
	char temp[100];
	char *checkName;

	char delim = '=';
	char quote = '\'';
	int i = 0;


	for (i = 0; i < strlen(token); i++)
        {
	//printf("here\n");

		if(token[i] != '=')
			temp[i] = token[i];
		else
			break;
	}

        checkName = temp;

	int j = 0;
	for(j = 0; j < 10; j++)
	{
		if(checkName == aliasName[j])							//if alias is already in the array, return 0
		{
			return 0;
		}
		else
		{					
			return 1;								//if not in the array, return 1
		}				
	}	

}
