#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <dirent.h>

void execute_cat(char * arg) {
	char buf[1024];
	memset(buf, '\0', 1024);
	int fd = open(arg, 0);
	if (fd > 0) {
		read(fd, buf, 1024);
        puts(buf);
		close(fd);
	}
}

void execute_ls(char * arg) {
    if (arg == NULL || *arg == '\0') {
		char buf[40];
		memset(buf, '\0', 40);
		getcwd(buf, 40);
		memcpy(arg, buf, strlen(buf));
	}
	dir_info* dir = opendir(arg);
	if (dir != NULL) {
		struct dirent *dir_entry =  readdir(dir);
		while (dir_entry != NULL) {
			//printf("\n%s", dir_entry->d_name);
			dir_entry =  readdir(dir);	
		}
		closedir(dir);
	}
}

void execute_cd(char * arg) {
	chdir(arg);
}

void execute_pwd() {
	char buf[40];
	memset(buf, '\0', 40);
	getcwd(buf, 40);
	if (buf != NULL) {
		printf("\n %s", buf);
	}
}

void extract_arg(char * start, char * arg) {
	char * trav = start;
    int no_of_arguments = 0;
	while(trav != NULL && *trav != '\0' && no_of_arguments <= 1) {
		if (*trav != ' ') {
			*arg++ = *trav++;
		} else {
			no_of_arguments += 1;
			trav++;
		}
	}
	*arg = '\0';
 }

void execute_clrscr() {
	clrscr();
}


int parse_command(char * buff, char * arg) {
	if (buff == NULL || *buff == '\0') {
		return -1;
	}
	if (strlen(buff) >= 2 && buff[0] == 'c' && buff[1] == 'd') {
		extract_arg(buff + 2, arg);
		return 1;
	}

	if (strlen(buff) >= 2 && buff[0] == 'l' && buff[1] == 's') {
		 extract_arg(buff + 2, arg);
         return 2;
    }

	if (strlen(buff) >= 3 && buff[0] == 'c' && buff[1] == 'a' && buff[2] == 't') {
		 extract_arg(buff + 3, arg);
         return 3;
	}

	if (strlen(buff) >= 3 && buff[0] == 'p' && buff[1] == 'w' && buff[2] == 'd') {
		return 4;
	}

	if (strlen(buff) >= 6 && buff[0] == 'c' && buff[1] == 'l' && buff[2] == 'r'
		&& buff[3] == 's' && buff[4] == 'c' && buff[5] == 'r') {
		return 5;
	}

	if (strlen(buff) >= 4 && buff[0] == 't' && buff[1] == 'e' && buff[2] == 's'
	    && buff[3] == 't') {
	       return 6;
    }


	return -1;
}


/*
This method is used for testing malloc/fork etc which where not integrated as launching
binaries was not completed.
*/
void test() {
	for (int i = 0; i < 4000000; i++) {
		char * ptr = (char *)malloc(4000);
		*ptr = 'm';
		printf("\n %d - %c - %p" , i, *ptr, ptr);
		free(ptr);
	}
}


int main(int argc, char *argv[], char *envp[]) {

	printf("\n**************************************************************");
	printf("\nLook at rootfs/etc/command.txt file for all supported commands\n");
	printf("**************************************************************\n");
    
    char buf[100];
	char arg[100];

    /*uint64_t pid = fork();
    if( pid == 0) {
        printf(" I am Child !!! RET = %d ", pid);
        ps();
        sleep(5);
        //execvpe("rootfs/bin/ls", NULL, NULL);
        exit(0);
        //while(1) { }
    }
    else {
        printf(" I am Parent !!! CPID = %d ", pid);
        //yield();
        //while(1) { }
    }*/

    while(1) {
      puts("\nsbush~>");
      gets(buf);
      switch(parse_command(buf, arg)) {
		case 1:
			// cd <>
			execute_cd(arg);
			break;
		case 2:
			// ls <>
			execute_ls(arg);
			break;
		case 3:
			// cat <>
			execute_cat(arg);
			break;
		case 4:
			// pwd
			execute_pwd();
			break;
		case 5:
			// clrscr
			execute_clrscr();
			break;

        case 6:
			// Test malloc() / fork() etc from here - write your code from libc here.
			test();
			break;

		default:
			printf("%s:Command Not Found", buf);
	  }
	  memset(arg, '\0', strlen(arg));
      memset(buf, '\0', strlen(buf));
    }

    return 0;
}

