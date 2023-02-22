#ifndef LOG_H
#define LOG_H

#define LOG_MAX     4096
#define LOG_FILE    "comiFS.log"

FILE *log_open()
{
    FILE *logfile;
    
    logfile = fopen(LOG_FILE, "w");
    if (logfile == NULL) {
        perror("logfile");
        exit(EXIT_FAILURE);
    }
    
    setvbuf(logfile, NULL, _IOLBF, 0);
    return logfile;
}

void log_syscall(char *syscall, const char *path)
{
    fprintf(COMI_CONTEXT->logfile, "Path: %s    Syscall: %s\n", path, syscall);   
}

void custom_log(const char *text) 
{
    fprintf(COMI_CONTEXT->logfile, "%s\n", text); 
}


#endif
