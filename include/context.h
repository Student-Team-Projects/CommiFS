#ifndef COMICONTEXT_H
#define COMICONTEXT_H

struct comi_state {
    FILE *logfile;
    char *rootdir;
};

#define COMI_CONTEXT ((struct comi_state *) fuse_get_context()->private_data)


#endif
