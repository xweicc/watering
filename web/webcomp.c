
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <zlib.h>

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

int compress_data(unsigned char *source_data, size_t source_size, unsigned char **dest_data, size_t *dest_size) {
    // 初始化压缩流
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    int ret = deflateInit(&strm, Z_BEST_COMPRESSION);
    if (ret != Z_OK) {
        fprintf(stderr, "deflateInit failed\n");
        return -1;
    }

    // 分配足够大的空间来存储压缩后的数据
    *dest_data = (unsigned char *)malloc(source_size);
    if (!*dest_data) {
        fprintf(stderr, "Failed to allocate memory for destination data\n");
        deflateEnd(&strm);
        return -1;
    }

    // 开始压缩数据
    strm.avail_in = source_size;
    strm.next_in = source_data;
    strm.avail_out = *dest_size;
    strm.next_out = *dest_data;

    ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        fprintf(stderr, "deflate failed\n");
        free(*dest_data);
        deflateEnd(&strm);
        return -1;
    }

    // 压缩后的数据大小
    *dest_size = strm.total_out;

    // 结束压缩
    deflateEnd(&strm);

    return 0;
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

        unsigned char *dest_data;
        size_t dest_size = datalen * 2;
        if (compress_data(data, datalen, &dest_data, &dest_size)) {
            fprintf(stderr, "Failed to compress data.\n");
            goto out;
        }
		
		fprintf(webfile, "static const unsigned char file%d_data[]={", file_num);
		for(i=0;i<dest_size;i++){
			fprintf(webfile, "%3u,", dest_data[i]);
            if(i && i%50==0){
                fprintf(webfile, "\n");
            }
		}
		fprintf(webfile, "0};\n");
        free(dest_data);

        printf("dest_size:%d\n",(int)dest_size);
        fprintf(webfile, "static http_file_data_t http_file%d={\n",file_num);
		fprintf(webfile, "\t.uri=\"/%s\",\n",file);
        fprintf(webfile, "\t.datalen=%d,\n",(int)dest_size);
		fprintf(webfile, "\t.data=file%d_data\n",file_num);
        fprintf(webfile, "};\n");
        
		fprintf(webfile, "static httpd_uri_t uri%d={\n",file_num);
		fprintf(webfile, "\t.uri=\"/%s\",\n",file);
        fprintf(webfile, "\t.method=HTTP_GET,\n");
        fprintf(webfile, "\t.handler=html_handler,\n");
		fprintf(webfile, "\t.user_ctx=&http_file%d\n",file_num);
		fprintf(webfile, "};\n");
		
		file_num++;
	}
	fprintf(webfile, "int http_file_num = %d;\n",file_num);
	fprintf(webfile, "httpd_uri_t *http_files[] = {\n");
	for(i=0;i<file_num;i++){
		fprintf(webfile, "\t%s&uri%d\n", i?",":"", i);
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


