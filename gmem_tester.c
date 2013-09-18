#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define MEMORY_BUFFER_SIZE 256

int main (int argc, char **argv)
{
	int fp;
	
	//opening the device driver for read and write
	fp = open("/dev/gmem", O_RDWR);
	
	char *argument;
	
	// print usage if number of arguments is not proper.
	if (argc < 2)
	{
		printf("usage: \n \
		./gmem_tester show - reads the gmem driver.\n \
		./gmem_tester write <your_string_in_double_quotes> - writes to the driver.\n");
	
		return 0;
	}
	
	strcpy(argument,argv[1]);
	
	if (!strcmp(argument,"show"))
	{
		int res=0;
		char *read_buffer;
		read_buffer = (char *) malloc(sizeof(char) * MEMORY_BUFFER_SIZE);
		res = read(fp,read_buffer,MEMORY_BUFFER_SIZE);
		if(res < 0)
		{
			printf("Error reading device /dev/gmem.\n");
		} else 
		{
			int i;
			for (i=0;i<res;i++)
			{
				printf("%c",read_buffer[i]);
			}
		}
	} else if (!strcmp(argument,"write"))
	{
		int res =0,buffer_len , write_bytes;
		char *write_buffer;
		if(argc < 3)
		{
			printf("Missing Argument.Specify what is to be written.\n");
			close(fp);
			return -1;
		}
		if (strlen(argv[2]) > MEMORY_BUFFER_SIZE)
		{
			printf("String too long. Only %d bytes will be copied.\n",MEMORY_BUFFER_SIZE);
		}
		write_buffer = (char*)malloc(sizeof(char) * strlen(argv[2]));
		strcpy(write_buffer,argv[2]);
		buffer_len = strlen(write_buffer);
		write_bytes = (buffer_len > MEMORY_BUFFER_SIZE) ? MEMORY_BUFFER_SIZE : buffer_len;
		res = write(fp,write_buffer,write_bytes);
		
		if(res < 0)
		{
			printf("Error writing to device /dev/gmem.\nERROR: %s\n",strerror(errno));
		} else 
		{
			printf("%d bytes written of %d bytes.\n",res,write_bytes);
		}
	} else 
	{
		printf("Invalid Argument.\n");
	}
	close(fp);
}
