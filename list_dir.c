#include "list_dir.h"

#define PATH_MAX 4096

int g_maxlen;

char *list_file(char *name,char* dir) //Print the - l parameter in the corresponding format
{
    int base = 0;
    int buf_length = sizeof(char) * MAXLINE;
    char *buf = (char *)malloc(buf_length);
    bzero(buf, buf_length);

    struct stat stat_buf;

    char *path=(char*)malloc(strlen(name)+strlen(dir)+2);
    sprintf(path,"%s/%s",dir,name);
    
    lstat(path, &stat_buf);

    char stat_buff_time[32];
    struct passwd *psd; //Receive the user name of the file owner from this structure
    struct group *grp;  //Get group name
    if (S_ISLNK(stat_buf.st_mode))
        base += sprintf(buf + base, "l");
    else if (S_ISREG(stat_buf.st_mode))
        base += sprintf(buf + base, "-");
    else if (S_ISDIR(stat_buf.st_mode))
        base += sprintf(buf + base, "d");
    else if (S_ISCHR(stat_buf.st_mode))
        base += sprintf(buf + base, "c");
    else if (S_ISBLK(stat_buf.st_mode))
        base += sprintf(buf + base, "b");
    else if (S_ISFIFO(stat_buf.st_mode))
        base += sprintf(buf + base, "f");
    else if (S_ISSOCK(stat_buf.st_mode))
        base += sprintf(buf + base, "s");
    //Get Print File Owner Rights
    if (stat_buf.st_mode & S_IRUSR)
        base += sprintf(buf + base, "r");
    else
        base += sprintf(buf + base, "-");
    if (stat_buf.st_mode & S_IWUSR)
        base += sprintf(buf + base, "w");
    else
        base += sprintf(buf + base, "-");
    if (stat_buf.st_mode & S_IXUSR)
        base += sprintf(buf + base, "x");
    else
        base += sprintf(buf + base, "-");

    //All group permissions
    if (stat_buf.st_mode & S_IRGRP)
        base += sprintf(buf + base, "r");
    else
        base += sprintf(buf + base, "-");
    if (stat_buf.st_mode & S_IWGRP)
        base += sprintf(buf + base, "w");
    else
        base += sprintf(buf + base, "-");
    if (stat_buf.st_mode & S_IXGRP)
        base += sprintf(buf + base, "x");
    else
        base += sprintf(buf + base, "-");

    //Other people's rights
    if (stat_buf.st_mode & S_IROTH)
        base += sprintf(buf + base, "r");
    else
        base += sprintf(buf + base, "-");
    if (stat_buf.st_mode & S_IWOTH)
        base += sprintf(buf + base, "w");
    else
        base += sprintf(buf + base, "-");
    if (stat_buf.st_mode & S_IXOTH)
        base += sprintf(buf + base, "x");
    else
        base += sprintf(buf + base, "-");

    base += sprintf(buf + base, " ");
    //Get the user name and group name of the file owner based on uid and gid
    
    psd = getpwuid(stat_buf.st_uid);
    grp = getgrgid(stat_buf.st_gid);

    base += sprintf(buf + base, "%d ", (int)(stat_buf.st_nlink)); //Link number
    base += sprintf(buf + base, "%s ", psd->pw_name);
    base += sprintf(buf + base, "%s ", grp->gr_name);
    
    base += sprintf(buf + base, "%d ", (int)(stat_buf.st_size));
    strcpy(stat_buff_time, ctime(&stat_buf.st_mtime));
    stat_buff_time[strlen(stat_buff_time) - 1] = '\0'; //Buffe_time has its own newline, so you need to remove the following newline character
    base += sprintf(buf + base, "%s ", stat_buff_time);

    base += sprintf(buf + base, "%s", name);

    base += sprintf(buf + base, "\n");

    return buf;
}

char *list_dir(char *path) //This function is used to process directories
{
    DIR *dir;           //Accept the file descriptor returned by opendir
    struct dirent *ptr; //Structures that accept readdir returns
    int count = 0;
    //Get the number of files and the longest file name length
    printf("%s\n", path);
    dir = opendir(path);

    g_maxlen = 0;
    while ((ptr = readdir(dir)) != NULL)
    {
        if (g_maxlen < strlen(ptr->d_name))
            g_maxlen = strlen(ptr->d_name);
        count++;
    }
    closedir(dir);

    char **filenames = (char **)malloc(sizeof(char *) * count), temp[PATH_MAX + 1]; //Dynamic allocation of storage space on the heap is achieved by the number of files in the directory. First, an array of pointers is defined.

    for (int i = 0; i < count; i++) //Then let each pointer in the array point to the allocated space in turn. Here is the optimization, which is effective.
    {                               //It prevents stack overflow and allocates memory dynamically, which saves more space.
        filenames[i] = (char *)malloc(sizeof(char) * PATH_MAX + 1);
    }

    int i, j;
    //Get all file names in this directory
    dir = opendir(path);
    for (i = 0; i < count; i++)
    {
        ptr = readdir(dir);
        strcpy(filenames[i], ptr->d_name);
    }
    closedir(dir);
    for (i = 0; i < count - 1; i++)
    {
        for (j = 0; j < count - 1 - i; j++)
        {
            if (strcmp(filenames[j], filenames[j + 1]) > 0)
            {
                strcpy(temp, filenames[j]);
                strcpy(filenames[j], filenames[j + 1]);
                strcpy(filenames[j + 1], temp);
            }
        }
    }

    int base = 0;
    int buf_length = count * MAXLINE * sizeof(char);
    char *buf = (char *)malloc(buf_length);
    bzero(buf, buf_length);

    for (int i = 0; i < count; i++)
    {
        if (filenames[i][0] != '.')

        {

            char *tmp_buf = list_file(filenames[i],path);
            base += sprintf(buf + base, "%s", tmp_buf);
        }
    }
    return buf;
}

// int main()
// {
//     char *path = "/tmp";
//     printf("%s\n", list_dir(path));
// }