// mkbup.c - модуль дочернего процесса, который бекапит часть исходного источника.

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

unsigned f(unsigned count, unsigned divnum);
const unsigned PATHNAME_LENGTH = 255u;
const char *help = "Usage: proc [options]\n\
				   Options:\n\
				   \t-c [image pathname]  (Image pathname to backup)\n\
				   \t-s [start byte]      (Start byte of image to backup)\n\
				   \t-S [end byte]        (End byte of image to backup)\n\
				   \t-r [image pathname]  (Image pathname to restore from backup)\n\
				   \t-b [bitmap pathname] (Bitmap pathname, that needed to restore image)\n\
				   \t-B [backup pathname] (Backup pathname, that needed to restore image)\n\
				   \t-P [file prefix]     (Prefix apended to end of files)\n\n\
				   See README for details.\nSorry me for my english <:)";

int main(int argc, char *argv[])
{
	if (argc!= 12)
	{
		printf("%s", help);
		return 0;
	}

	int fsource;							// исходник
	int fresolv;							// востановленный исходник
	int fbitmap;							// битовая карта
	int fbackup;							// бэкап
	int todo;								// todo == 0 (востановление), todo == 1 (создание)
	char ext[PATHNAME_LENGTH];
	char bitmap_pathname[PATHNAME_LENGTH];
	char backup_pathname[PATHNAME_LENGTH];
	char resolv_pathname[PATHNAME_LENGTH];
	unsigned long start_byte;
	unsigned long end_byte;
	unsigned long R;
	int readed;
	unsigned int i, j, k;
	const unsigned BLOCK_SIZE = 512;
	const unsigned BYTE_SIZE = 8;
	unsigned char buf[BLOCK_SIZE * BYTE_SIZE];
	unsigned char block[BLOCK_SIZE];
	unsigned char card;

	if(strcmp(argv[0], "-c") == 0)
	{
		if((fsource = open(argv[1], O_RDONLY)) == -1)
		{
			printf("File \"%s\" not found.", argv[1]);
			return 1;
		}
		for(i = 2; i < 12; i+=2)
		{
			switch (argv[i][1])			
			{
			case 'b': strcpy(bitmap_pathname, argv[i+1]);
				break;
			case 'B': strcpy(backup_pathname, argv[i+1]);
				break;			
			case 's': start_byte = atoi(argv[i+1]);
				break;
			case 'S': end_byte = atoi(argv[i+1]);
				break;
			case 'P': sprintf(ext, ".%s", argv[i+1]);
				break;
			}
		}
		todo = 1;
	}
	else if(strcmp(argv[0], "-r") == 0)
	{
		strcpy(resolv_pathname, argv[1]);
		for(i = 2; i < 12; i+=2)
		{
			switch (argv[i][1])			
			{
			case 'b': strcpy(bitmap_pathname, argv[i+1]);
				break;
			case 'B': strcpy(backup_pathname, argv[i+1]);
				break;			
			case 's': start_byte = atoi(argv[i+1]);
				break;
			case 'S': end_byte = atoi(argv[i+1]);
				break;
			case 'P': sprintf(ext, ".%s", argv[i+1]);
				strcat(resolv_pathname, ext);
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

	strcat(bitmap_pathname, ext);
	strcat(backup_pathname, ext);
	lseek(fsource, start_byte, SEEK_SET);

	if(todo == 1)
	{
		if((fbitmap = open(bitmap_pathname, O_WRONLY | O_CREAT)) == -1)
		{
			printf("File \"%s\" can't create.", bitmap_pathname);
			return 1;
		}	
		if((fbackup = open(backup_pathname, O_WRONLY | O_CREAT)) == -1)
		{
			printf("File \"%s\" can't create.", backup_pathname);
			return 1;
		}

		R = start_byte;
		while(R <= end_byte)
		{
			readed = read(fsource, buf, sizeof buf);
			if(readed > 0)
			{
				// чтобы не жадничал...
				if(R + readed > end_byte)
					readed = end_byte - R +1;
				R+= readed;

				const unsigned char c = 1;      
				for(i = 0, card = 0; i < readed; i++)
				{
					j = 1;
					k = i+1;
					if((int)buf[i])									
						card|= c << f(i, BYTE_SIZE);

					if( (k % BYTE_SIZE) == 0 )			
					{
						write(fbitmap, (char *)&card, sizeof card);
						card = 0;
						j = 0;
					}
				}
			}
			else if(readed < 0)
			{
				printf("%s\n","[failed]");
				puts("Read file error when creating bitmap.");
				return 1;
			}
			else if(readed == 0)
				break;
		}
		if(j)
			write(fbitmap, (char *)&card, sizeof card);

		close(fbitmap);
		close(fsource);
		fsource = open(argv[1], O_RDONLY);
		lseek(fsource, start_byte, SEEK_SET);
		fbitmap = open(bitmap_pathname, O_RDONLY);

		while((readed = read(fbitmap, block, sizeof block))!= 0)
		{
			if(readed > 0)
			{
				read(fsource, buf, sizeof buf);
				unsigned char c;
				char *pbuf;

				pbuf = buf;
				for(i = 0; i < readed; i++)
				{
					if((int)block[i])
					{
						for(j = 0, c = 1; j < BYTE_SIZE; j++, c = 1)
						{
							c = c << j;
							if((block[i] & c) == c)
								write(fbackup, (char *)&pbuf[j], 1);
						}
					}
					pbuf = &pbuf[BYTE_SIZE];
				} 
			}
			else if(readed < 0)
			{
				printf("%s\n","[failed]");
				puts("Read bitmap file error when creating backup.");
				return 1;
			}
		}
		close(fbackup);
		close(fbitmap);
		close(fsource);
	}
	else	// восстановление
	{
		if((fbitmap = open(bitmap_pathname, O_RDONLY)) == -1)
		{
			printf("File \"%s\" can't open.", bitmap_pathname);
			return 1;
		}	
		if((fbackup = open(backup_pathname, O_RDONLY)) == -1)
		{
			printf("File \"%s\" can't open.", backup_pathname);
			return 1;
		}
		if((fresolv = open(resolv_pathname, O_WRONLY | O_CREAT)) == -1)
		{
			printf("File \"%s\" can't create.", resolv_pathname);
			return 1;
		}

		unsigned char byte, res, c, z = 0;
		unsigned char s[BYTE_SIZE];
		while((readed = read(fbitmap, (char *)&byte, 1))!= 0)
		{
			for(j = 0, c = 1; j < BYTE_SIZE; j++, c = 1)
			{
				c = c << j;
				if((byte & c) == c)
				{
					read(fbackup, (char *)&res, 1);
					write(fresolv, (char *)&res, 1);
				}
				else
					write(fresolv, (char *)&z, 1);
			}
		}

		close(fbitmap);              
		if(unlink(bitmap_pathname))
			printf("Can't delete file %s.\n", bitmap_pathname);

		close(fbackup);
		if(unlink(backup_pathname))
			printf("Can't delete file %s.\n", backup_pathname);    
		close(fresolv);
	}

	return 0;
}

unsigned f(unsigned count, unsigned divnum)
{
	while(count >= divnum)
		count-=divnum;
	return count;
}
