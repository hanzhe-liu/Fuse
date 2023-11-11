#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

char* dir_list[256];
int curr_dir_idx = -1;

char* files_list[256];
int curr_file_idx = -1;

char* files_content[256];
int curr_file_content_idx = -1;

char* files_backup_content[256];

void add_dir(const char* dir_name)
{
    curr_dir_idx++;
    dir_list[curr_dir_idx] = (char*)malloc(strlen(dir_name) + 1);
    strcpy(dir_list[curr_dir_idx], dir_name);
}

int is_dir(const char* path)
{
    path++; 

    for (int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++)
        if (strcmp(path, dir_list[curr_idx]) == 0)
            return 1;

    return 0;
}

void add_file(const char* filename)
{
    curr_file_idx++;
    files_list[curr_file_idx] = (char*)malloc(strlen(filename) + 1);
    strcpy(files_list[curr_file_idx], filename);

    curr_file_content_idx++;
    files_content[curr_file_content_idx] = (char*)malloc(256);
    strcpy(files_content[curr_file_content_idx], "");
}

int is_file(const char* path)
{
    path++; 

    for (int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++)
        if (strcmp(path, files_list[curr_idx]) == 0)
            return 1;

    return 0;
}

int get_file_index( const char *path )
{
    path++; 

    for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
        if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
            return curr_idx;
    
    return -1;
}

void backup_file_content(const char* path)
{
    int file_idx = get_file_index(path);
    if (file_idx != -1)
    {
        if (files_backup_content[file_idx] != NULL)
            free(files_backup_content[file_idx]);
        files_backup_content[file_idx] = (char*)malloc(strlen(files_content[file_idx]) + 1);
        strcpy(files_backup_content[file_idx], files_content[file_idx]);
    }
}

void write_to_file(const char* path, const char* new_content)
{
    int file_idx = get_file_index(path);

    if (file_idx == -1){
        return;
    }

    if (files_content[file_idx] == NULL){
        files_content[file_idx] = (char*)malloc(strlen(new_content) + 1);
    } else {
        files_content[file_idx] = (char*)realloc(files_content[file_idx], strlen(new_content) + 1);
    }
    strcpy(files_content[file_idx], new_content);
}



static int do_getattr( const char *path, struct stat *st )
{
    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_atime = time( NULL );
    st->st_mtime = time( NULL );
    
    if ( strstr( path, ".revert" ) ) 
    {
        char original_path[ 256 ];
        strncpy( original_path, path, strlen(path) - 7 );
        original_path[ strlen(path) - 7 ] = '\0';

        if ( is_file( original_path ) )
        {
            st->st_mode = S_IFREG | 0644;
            st->st_nlink = 1;
            st->st_size = 1024; 
            return 0;
        }
    }

    if ( strcmp( path, "/" ) == 0 || is_dir( path ) == 1 )
    {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    }
    else if ( is_file( path ) == 1 )
    {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = 1024;
    }
    else
    {
        return -ENOENT;
    }
    
    return 0;
}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
    filler( buffer, ".", NULL, 0 ); 
    filler( buffer, "..", NULL, 0 ); 
    
    if ( strcmp( path, "/" ) == 0 )
    {
        for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
            filler( buffer, dir_list[ curr_idx ], NULL, 0 );
    
        for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
            filler( buffer, files_list[ curr_idx ], NULL, 0 );
    }
    
    return 0;
}



void revert_to_backup( const char *path )
{
    int file_idx = get_file_index( path );
    if ( file_idx != -1 ) 
    {
        strcpy( files_content[ file_idx ], files_backup_content[ file_idx ] );
    }
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
    if ( strstr( path, ".revert" ) )
    {
        char original_path[ 256 ];
        strncpy( original_path, path, strlen(path) - 7 );
        original_path[ strlen(path) - 7 ] = '\0';
        
        char current[1024];
        int file_idx = get_file_index( original_path );
        strcpy( current, files_backup_content[ file_idx ] );
        backup_file_content( original_path );   
        strcpy( files_content[ file_idx ], current );
        char *content = files_content[ file_idx ];
        memcpy( buffer, content + offset, size );

        return strlen( content ) - offset;
    }

    int file_idx = get_file_index( path );
    if ( file_idx == -1 )
        return -1;

    char *content = files_content[ file_idx ];
    memcpy( buffer, content + offset, size );

    return strlen( content ) - offset;
}


static int do_write( const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info )
{
    backup_file_content( path );   
    write_to_file( path, buffer );
    
    return size;
}

static int do_mkdir( const char *path, mode_t mode )
{
    path++;
    add_dir( path );
    
    return 0;
}

static int do_mknod( const char *path, mode_t mode, dev_t rdev )
{
    path++;
    add_file( path );

    return 0;
}


static struct fuse_operations operations = {
    .getattr    = do_getattr,
    .readdir    = do_readdir,
    .read       = do_read,
    .mkdir      = do_mkdir,
    .mknod      = do_mknod,
    .write      = do_write,
};

int main(int argc, char* argv[])
{
    int result = fuse_main(argc, argv, &operations, NULL);

    // Free the allocated memory before exit
    for (int i = 0; i <= curr_dir_idx; i++)
        free(dir_list[i]);
    for (int i = 0; i <= curr_file_idx; i++)
    {
        free(files_list[i]);
        free(files_content[i]);
        if (files_backup_content[i] != NULL)
            free(files_backup_content[i]);
    }

    return result;
}