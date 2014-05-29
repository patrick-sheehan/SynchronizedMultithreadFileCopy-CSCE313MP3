/*  MP3/mcopyfile.c
	 	Patrick Sheehan
	 	CSCE 313 MP3
	 	26 Mar 2014
	 	Resources: stackoverflow, computing.llnl.gov,
	 							pubs.opengroup.org, "Unix Systems Programming"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>


#define BUFFER_SIZE 20
#define PATH_MAX 256
#define SRC_DIR "testfiles/"
#define DEST_DIR "copiedfiles/"

typedef struct{
	char* fp_src;
	char* fp_dest;
	int src_fd;
	int dest_fd;
	char file_name[PATH_MAX];
} FilePair;

int NUM_THREADS;
int count;
FilePair pairs[BUFFER_SIZE];
pthread_cond_t buffer_empty;
pthread_cond_t buffer_full;
pthread_mutex_t lock;


void producerBufferWrite();
void consumerBufferRead();


int main(int argc, char* argv[])
{	
	if (argc != 2)
	{ // error handling
		printf("Please provide number of filecopy consumer threads as an argument\n");
		return 0;
	}

	// variable declarations
	char* files[2];
	int i;
	NUM_THREADS = atoi(argv[1]);
	pthread_t producer_thread;
	pthread_t comsumer_threads[NUM_THREADS];
	
	count = 0;
	pthread_mutex_init(&lock, NULL);

	producerBufferWrite();

	for (i = 0; i < NUM_THREADS; i++)
	{
		pthread_create(&comsumer_threads[i], NULL, (void*)consumerBufferRead, NULL);
	}
	

	for (i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(comsumer_threads[i], NULL);
	}
	

	printf("Reached end of main\n");

	return 0;
}



void consumerBufferRead()
{
	int newCount = -1;
	char *src, *dest;	
	int src_fd, dest_fd;
	FILE *srcfile, *destfile;
  int ch;

	pthread_mutex_lock(&lock);
	while (count <= 0)
	{
		pthread_cond_wait(&buffer_empty, &lock);
	}
	
	count--;
	FilePair fp = pairs[count];
	
	src_fd = fp.src_fd;
	dest_fd = fp.dest_fd;
	newCount = count;
	pthread_cond_signal(&buffer_full);

	pthread_mutex_unlock(&lock);
  
  off_t fsize;
  fsize = lseek(src_fd, 0, SEEK_END);

  char temp_buffer[fsize];

  read(src_fd, temp_buffer, fsize);
  printf("read %d\n", src_fd);
  write(dest_fd, temp_buffer, fsize);
  printf("wrote %d\n", dest_fd);

  int src_closed = close(src_fd);
  printf("close(src - %d) = %d\n", src_fd, src_closed);
  int dest_closed = close(dest_fd);  
  printf("close(dest - %d) = %d\n", dest_fd, dest_closed);
  //printf("\nCONSUMER:\nCompleted copying the following following file:\n(%s -> %s)\nnew count = %d\n\n", src, dest, newCount);
  pthread_exit(NULL);
}


void producerBufferWrite()
{
	// function for producer to add file pair to shared buffer
	char src_file[256];
	char dest_file[256];
	DIR *dp;
	struct dirent *ep;
	FilePair fp;	

	dp = opendir(SRC_DIR);
	if (dp != NULL)
	{
		while ((ep = readdir(dp)))
		{	// get each file pairs ready to individually place on the shared buffer

			if (ep->d_type != DT_REG) continue;
			
			pthread_mutex_lock(&lock);
			while (count == BUFFER_SIZE) 
			{
				pthread_cond_wait(&buffer_full, &lock);
			}

			char fileName[256] = "";
			strcpy(fileName, (char*)ep->d_name);
			strcpy(src_file, SRC_DIR);
			strcpy(dest_file, DEST_DIR);
			strcat(src_file, fileName);
			strcat(dest_file, fileName);

			int srcFD = open(src_file, O_RDONLY);
			int destFD = open(dest_file, O_RDWR | O_CREAT);
			fp.src_fd = srcFD;
			fp.dest_fd = destFD;

			

			pairs[count++] = fp;
			//printf("\nPRODUCER:\nCompleted putting the following into buffer:\n(%s, %s)\nnew count = %d\n\n", src_file, dest_file, count);
			printf("srcFD  = %d\ndestFD = %d\n\n", srcFD, destFD);
			pthread_cond_signal(&buffer_empty);
			pthread_mutex_unlock(&lock);
		}

		(void) closedir(dp);		
	}
	else
	{
		perror("Error opening source directory\n");
	}
};
