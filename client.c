#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <errno.h>

typedef struct thread_data{
	int thread_num;
	char * address;
	char * port;
	char * cmd;
}thread_data;

int fileFlag;
FILE * errorFile;

void * socket_func(void * args_arr);
void getLineCount(int num_hosts);
thread_data * t_data;

int main(int argc, char **argv)
{
	if(argc < 2){
		printf("\n\n\tUSAGE: ./client FILE_FLAG COMMAND ARGS\n");
		return 0;
	}
	
	errorFile = fopen("error_file","w");
	char cmd[2000];
	int i = 0;

	fileFlag = atoi(argv[1]);
	
	if(fileFlag != 1 && fileFlag != 0){
		printf("FILE FLAG should be 1 for output to files or 0 for output to stdout");
		return 0;
	}
	
	// make a command string 
	strcpy(cmd,argv[2]);
	
	for(i=3;i<argc;i++){
		strcat(cmd," ");
		strcat(cmd,argv[i]);
	}

	// get list of hosts
	FILE * host_fp = fopen("host_list","r");
	int num_hosts = 0;
	char ch;
	while(!feof(host_fp)){
	  ch = fgetc(host_fp);
	  if(ch == '\n')
	  {
		num_hosts++;
	  }
	}
	
	char * host_list[num_hosts]; 
	
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int host_num = 0;
	rewind(host_fp);
	while ((read = getline(&line, &len, host_fp)) != -1) {
		line[read-1] = '\0';
        host_list[host_num] = line;
        host_num++;
        line = NULL;
    }
	
	fclose(host_fp);
	
	pthread_t threads[num_hosts];
	
	t_data = malloc(sizeof(thread_data) * num_hosts);
	int * threadNums = malloc(sizeof(int) * num_hosts);
	
	for(host_num = 0;host_num < num_hosts;host_num++){
		threadNums[host_num] = host_num;
		
		t_data[host_num].thread_num = host_num;
		t_data[host_num].address = host_list[host_num];;
		t_data[host_num].port = strdup("10000");
		t_data[host_num].cmd = cmd;
		
		
		pthread_create(&threads[host_num],NULL,socket_func,&threadNums[host_num]);

	}
	
	for(i=0;i<num_hosts;i++){
  		pthread_join(threads[i],NULL);  	
  	}
	
	// free
	for(host_num= 0;host_num<num_hosts;host_num++){
		free(host_list[host_num]);
		free(t_data[host_num].port);
	} 

	if(fileFlag)
		getLineCount(num_hosts);
	
	free(t_data);
	free(threadNums);
	fclose(errorFile);
	
	return 0;
	
}


void * socket_func(void * args_struct)
{
	int t_id = *(int*)args_struct;
	thread_data t = t_data[t_id];
	char * address = t.address;
	char * port = t.port;
	char * cmd = t.cmd;
	
	// return if address was NULL
	if(address == NULL){
		fprintf(errorFile,"%d : address was NULL",t_id);
		return 0;
	}
	
	// return if port was NULL
	if(port == NULL){
		fprintf(errorFile,"%d : port was NULL",t_id);
		return 0;
	}
	
    // create a socket
	int s;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(sock_fd < 0 ){
		fprintf(errorFile,"%d : error opening socket\n",t_id);
		fprintf(errorFile,"%s\n\n",strerror(errno));
    	return NULL;
    }	
    
    // get the address info using getaddrinfo
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; /* IPv4 only */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    s = getaddrinfo(address, port, &hints, &result);
    if (s != 0) {
			fprintf(errorFile, "%d : getaddrinfo: %s\n", t_id, gai_strerror(s));
			fprintf(errorFile,"%s\n\n",strerror(errno));
            return NULL;
    }
    
    // connect to the server
    int con = connect(sock_fd, result->ai_addr, result->ai_addrlen);
    
    if(con < 0){
		fprintf(errorFile,"%d : error connecting to %s\n",t_id,address);
		fprintf(errorFile,"%s\n\n",strerror(errno));
		return NULL;
    }
    
    // write to the socket
    size_t write_len = 0;
    write_len = write(sock_fd,cmd,strlen(cmd));
    write(sock_fd,"\n",1);
    if(write_len < 0){
    	char errorBuf[150];
		fprintf(errorFile,"%d : error connecting to %s\n",t_id,address);
    }
    
    int bufSize = 10;
    char buffer[bufSize];
    char * bufferMal = malloc(bufSize);
    int bytesRead = 0;

	// read output from the server
	char c = 176;
    char cc = 179;
    while(1){
        char tmp[1];
        int n = read(sock_fd, &tmp, 1);
        if(n < 0){
        	fprintf(errorFile,"%d : error reading",t_id);
        }
        else{
           /* if(tmp[0] == c){
                bufferMal[bytesRead] = '\0';
                break;
            }*/

            if(tmp[0] == cc && (bytesRead > 0) && bufferMal[bytesRead - 1] == c){
                bytesRead -= 1;
                bufferMal[bytesRead] = '\0';
                break;
            }

            bufferMal[bytesRead] = tmp[0];
            bytesRead += n;
        }
        if(bytesRead == (bufSize-2)){    	
			bufSize = bufSize * 2;
        	bufferMal = realloc(bufferMal, bufSize);
        }
    }
	// write output to file if file flag set to be true
	if(fileFlag){
		char fname[8];
		sprintf(fname,"VM%d.out",t_id);
		FILE * output = fopen(fname,"w");
		write(fileno(output),bufferMal,bytesRead);
		fclose(output);	
	}
	else{
    	printf("\nOUTPUT FROM VM%d:\n%s\n\n",t_id + 1,bufferMal);
    }
    
    free(bufferMal);
	freeaddrinfo(result);
	close(sock_fd);

    return 0;
}

/*
gets number of lines in output files
*/
void getLineCount(int num_hosts){
	int i = 0; 
	FILE * line_num_file = fopen("line_no_file","w");
	int total_count = 0;
	for(i = 0;i<num_hosts;i++){
		int count = 0;
		char fname[8];
		sprintf(fname,"VM%d.out",i);
		FILE * fp = fopen(fname,"r");
		if(fp){
			char c;
			for (c = getc(fp); c != EOF; c = getc(fp))
		    	if (c == '\n') // Increment count if this character is newline
		        	count = count + 1;
			fprintf(line_num_file,"VM%d : %d\n",i+1,count);
			total_count += count;
		}
	}
    fprintf(line_num_file,"------------\ntotal count : %d",total_count);
	fclose(line_num_file);
	
}

