
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

int  webcomp_fsize(const char *path)
{
	struct stat st;
	if (stat(path, &st) == 0) return st.st_size;
	return -1;
}

int webcomp_fread(const char *path, void *buffer, int max)
{
	int f;
	int n;
	
	if ((f = open(path, O_RDONLY)) < 0) return -1;
	n = read(f, buffer, max);
	close(f);
	return n;
}

int web_compile(char *fileList, char *dest)
{
	int ret=-1;
	FILE *list=NULL,*webfile=NULL;
	char buf[1024],*file=buf;
	char *p;
	int datalen;
	char *data;
	int file_num=0;
	int i=0;

	list=fopen(fileList, "r");
	if(!list){
		perror("Error: fopen");
		return -1;
	}
	webfile=fopen(dest, "w");
	if(!webfile){
		perror("Error: fopen");
		fclose(list);
		return -1;
	}

	fprintf(webfile, "#include \"webfile.h\"\n");
	while(1){
		memset(buf,0,sizeof(buf));
		if(NULL==fgets(buf,sizeof(buf),list)){
			break;
		}
		if(strlen(buf)<3){
			continue;
		}
		if(buf[0] == '#'){
			continue;
		}
		if((p = strchr(buf, '\n'))){
			*p = '\0';
		}
		if((p = strchr(buf, '\r'))){
			*p = '\0';
		}
		if(buf[0] == '\0'){
			continue;
		}
		file=buf;
		
		datalen=webcomp_fsize(file);
		if(datalen<=0){
			fprintf(stderr, "Error: Can't open %s\n", file);
			continue;
		}
		data=malloc(datalen);
		if(!data){
			perror("malloc");
			goto out;
		}
		if(webcomp_fread(file, data, datalen)!=datalen){
			fprintf(stderr, "Error: Can't read %s\n", file);
			goto out;
		}
		
		fprintf(webfile, "static unsigned char file%d_data[]={", file_num);
		for(i=0;i<datalen;i++){
			fprintf(webfile, "%3u,", (unsigned char)data[i]);
            if(i && i%50==0){
                fprintf(webfile, "\n");
            }
		}
		fprintf(webfile, "0};\n");
        
		fprintf(webfile, "static http_file_data_t http_file%d={\n",file_num);
		fprintf(webfile, "\t.name=\"%s\",\n",file);
		fprintf(webfile, "\t.datalen=%d,\n",datalen);
		fprintf(webfile, "\t.data=file%d_data\n",file_num);
		fprintf(webfile, "};\n");
		
		file_num++;
	}
	fprintf(webfile, "int http_file_num = %d;\n",file_num);
	fprintf(webfile, "http_file_data_t *http_files[] = {\n");
	for(i=0;i<file_num;i++){
		fprintf(webfile, "\t%s&http_file%d\n", i?",":"", i);
	}
	fprintf(webfile, "};\n");
	fflush(webfile);

	ret=0;
out:
	fclose(list);
	fclose(webfile);
	return ret;
}

int main(int argc, char **argv)
{
	char *fileList,*dest;

	if (argc != 3) {
		printf("Usage: %s [filelist] [dest]\n",argv[0]);
		return -1;
	}

	fileList = argv[1];
	dest = argv[2];

	if(web_compile(fileList,dest) < 0){
		return -1;
	}
	return 0;
}


