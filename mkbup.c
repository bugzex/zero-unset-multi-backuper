// mkbup.c - основной модуль, который делит источник для бэкапа между процессами.

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

unsigned div8(unsigned num);
unsigned fsize(const char *file_pathname);
const unsigned PATHNAME_LENGTH = 255u;
const char *help = "Usage: mkbup [options]\n\
				   Options:\n\
				   \t-c [image pathname]   (Image pathname to backup)\n\
				   \t-r [image pathname]   (Image pathname to restore from backup)\n\
				   \t-b [bitmap pathname]  (Bitmap pathname, that needed to restore image)\n\
				   \t-B [backup pathname]  (Backup pathname, that needed to restore image)\n\
				   \t-p [processes number]\n\n\
				   See README for details.\nSorry me for my english <:)";

int main(int argc, char *argv[])
{
	if (argc!= 9)
	{
		printf("%s", help);
		return 0;
	}

	unsigned i, j, k;
	unsigned less;
	int fsource;										// исходник
	int fresolv;										// востановленный исходник
	unsigned long file_len;								// размер исходника в байтах
	unsigned long piece;								// размер куска для процесса
	unsigned long P1, P2;
	unsigned procnum;									// количество процессов
	int todo;											// todo == 0 (востановление), todo == 1 (создание)
	int status;						
	char param[255];
	char arg[20][20];
	char source_pathname[PATHNAME_LENGTH];
	char bitmap_pathname[PATHNAME_LENGTH];
	char backup_pathname[PATHNAME_LENGTH];
	char resolv_pathname[PATHNAME_LENGTH];
	char prefix[4];
	char buf[PATHNAME_LENGTH];
	int file;
	int readed;
	char block[512];

	if(strcmp(argv[1], "-c") == 0)
	{
		strcpy(source_pathname, argv[2]);
		if ((fsource = open(source_pathname, O_RDONLY)) == -1)
		{
			printf("File \"%s\" not found.", source_pathname);
			return 1;
		}
		close(fsource);

		for(i = 3; i < 9; i+=2)
		{
			switch (argv[i][1])			
			{
			case 'b': strcpy(bitmap_pathname, argv[i+1]);
				break;
			case 'B': strcpy(backup_pathname, argv[i+1]);
				break;			
			case 'p': procnum = atoi(argv[i+1]);
				break;
			}
		}
		todo = 1;
	}
	else if(strcmp(argv[1], "-r") == 0)
	{
		strcpy(resolv_pathname, argv[2]);
		if((fresolv = open(resolv_pathname, O_WRONLY | O_CREAT)) == -1)
		{
			printf("File \"%s\" can't create.", resolv_pathname);
			return 1;
		}
		for(i = 3; i < 9; i+=2)
		{
			switch (argv[i][1])			
			{
			case 'b': strcpy(bitmap_pathname, argv[i+1]);
				break;
			case 'B': strcpy(backup_pathname, argv[i+1]);
				break;
			case 'p': procnum = atoi(argv[i+1]);
				break;
			}
		}
		todo = 0;
	}
	else
	{
		printf("%s", help);
		return 0;		
	}
	
	file_len = fsize(source_pathname);
	piece = div8(file_len/procnum);
	less = file_len-piece*procnum;
	unsigned part;

	printf("Processing");
	fflush(stdout);
	if(todo == 1)
	{
		for(i = 0; i < procnum; i++) {
			if(fork() == 0)
			{
				printf(".");
				fflush(stdout);

				if(i == procnum-1)
					part = file_len;
				else
					part = (i+1)*piece-1;

				sprintf(arg[0], "%u", i*piece);
				sprintf(arg[1], "%u", part);
				sprintf(arg[2], "%d", i);
				execlp("proc", "-c", source_pathname, "-s", arg[0], "-S", arg[1], "-b", bitmap_pathname, "-B", backup_pathname, "-P", arg[2], NULL);
			}
		}

		for(i = 0; i < procnum; i++) {
			printf(".");
			fflush(stdout);
			wait(&status);
		}
		puts("");
	}
	else
	{
		for(i = 0; i < procnum; i++)
			if(fork() == 0)
			{
				printf(".");
				fflush(stdout);
				sprintf(arg[0], "%u", 0);
				sprintf(arg[1], "%u", 0);
				sprintf(arg[2], "%d", i);
				execlp("proc", "-r", resolv_pathname, "-s", arg[0], "-S", arg[1], "-b", bitmap_pathname, "-B", backup_pathname, "-P", arg[2], NULL);
			}

			for(i = 0; i < procnum; i++) {
				printf(".");
				fflush(stdout);
				wait(&status);
			}

			for(i = 0; i < procnum; i++)
			{
				printf(".");
				fflush(stdout);
				strcpy(buf, resolv_pathname);
				sprintf(prefix, ".%d", i);
				strcat(buf, prefix);
				file = open(buf, O_RDONLY);
				lseek(file, 0L, SEEK_SET);

				while(readed = read(file, block, sizeof block))
					write(fresolv, block, readed);

				close(file);
				if(unlink(buf))
					printf("Can't delete file %s.\n", buf);
			}
			close(fresolv);
			puts("");
	}
	return 0;
}

// :(
unsigned fsize(const char *file_pathname)
{
	int file;
	int readed;
	unsigned R;
	char buf[1024];

	if( (file = open(file_pathname, O_RDONLY)) == -1 )
		return 1;

	while(readed = read(file, buf, sizeof buf))
		R+= readed;

	close(file);
	return R;
}

unsigned div8(unsigned num)
{
	while(num % 8)
		num--;
	return num;
}