#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include <bstrlib.h>
#include <bstrlib_helper.h>
#include <error.h>

#include <read_yaml_ptt.h>
#include <test_types.h>

int global_verbosity = DEBUGLEV_DEVELOP;

int get_architecture_dirs(bstring basefolder, struct bstrList** outlist)
{
    DIR *dp = NULL;
    struct dirent *ep = NULL;
    DIR *(*myopendir)(const char *name) = opendir;
    struct bstrList* out = bstrListCreate();
    dp = myopendir(bdata(basefolder));
    if (dp == NULL)
    {
        return -errno;
    }
    while ((ep = readdir(dp)) != NULL)
    {
        if (ep->d_name[0] != '.')
        {
            bstring abs = bformat("%s/%s", bdata(basefolder), ep->d_name);
            bstrListAdd(out, abs);
            bdestroy(abs);
        }
    }
    closedir(dp);
    *outlist = out;
    return 0;
}

int get_architecture_tests(bstring archfolder, struct bstrList** outlist)
{
    DIR *dp = NULL;
    struct dirent *ep = NULL;
    DIR *(*myopendir)(const char *name) = opendir;
    struct bstrList* out = bstrListCreate();
    dp = myopendir(bdata(archfolder));
    if (dp == NULL)
    {
        return -errno;
    }
    while ((ep = readdir(dp)) != NULL)
    {
        if ((ep->d_name[0] != '.') && (strncmp(&ep->d_name[strlen(ep->d_name)-4], "yaml", 4) == 0))
        {
            bstring abs = bformat("%s/%s", bdata(archfolder), ep->d_name);
            bstrListAdd(out, abs);
            bdestroy(abs);
        }
    }
    closedir(dp);
    *outlist = out;
    return 0;
}

int read_test(bstring filename)
{
    TestConfig_t config = NULL;
    printf("---- Filename %s ----\n", bdata(filename));
    int ret = read_yaml_ptt(bdata(filename), &config);
    if (ret)
    {
        return ret;
    }

    printf("Name: %s\n", bdata(config->name));
    printf("Description: %s\n", bdata(config->description));
    printf("Language: %s\n", bdata(config->language));

    int i = 0;
    printf("Variables:\n");
    for (i = 0; i < config->num_vars; i++)
    {
        printf("\t'%s' -> '%s'\n", bdata(config->vars[i].name), bdata(config->vars[i].value));
    }
    printf("Metrics:\n");
    for (i = 0; i < config->num_metrics; i++)
    {
        printf("\t'%s' -> '%s'\n", bdata(config->metrics[i].name), bdata(config->metrics[i].value));
    }
    printf("Streams:\n");
    for (i = 0; i < config->num_streams; i++)
    {
        printf("\t'%s' (Type '%s')\n\tDims: ", bdata(config->streams[i].name), bdata(config->streams[i].btype));
        bstrListPrint(config->streams[i].dims);
    }
    printf("Parameters:\n");
    for (i = 0; i < config->num_params; i++)
    {
        printf("\t'%s': %s\n\tOptions: ", bdata(config->params[i].name), bdata(config->params[i].description));
        bstrListPrint(config->params[i].options);
    }

    printf("Code:\n");
    printf("%s\n", bdata(config->code));

    close_yaml_ptt(config);
    return 0;
}

int main(int argc, char* argv[])
{
    int ret = 0;

    struct tagbstring basefolder = bsStatic("../kernels");
    struct bstrList* archs = NULL;
    ret = get_architecture_dirs(&basefolder, &archs);
    if (ret != 0)
    {
        return ret;
    }
    for (int i = 0; i < archs->qty; i++)
    {
        struct bstrList* archtests = NULL;
        ret = get_architecture_tests(archs->entry[i], &archtests);
        if (ret != 0)
        {
            continue;
        }
        for (int j = 0; j < archtests->qty; j++)
        {
            read_test(archtests->entry[j]);
        }
        bstrListDestroy(archtests);
    }
    bstrListDestroy(archs);
    return 0;
}



