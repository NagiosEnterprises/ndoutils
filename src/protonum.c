#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv){
	int fd=STDIN_FILENO;
	FILE *fp;
	int x=0;
	char temp_buffer[1024];
	char *ptr1=NULL;
	char *ptr2=NULL;

	
	if(argc<2)
		exit(1);

	x=atoi(argv[1]);

	fp=fdopen(fd,"r");
	while(fgets(temp_buffer,sizeof(temp_buffer),fp)){

		ptr1=strtok(temp_buffer," ");
		ptr2=strtok(NULL," ");

		printf("%s %-44s %d\n",(ptr1==NULL)?"NULL":ptr1,(ptr2==NULL)?"NULL":ptr2,x);

		x++;
	        }
	

	exit(1);
        }
