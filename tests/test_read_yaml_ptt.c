#include <stdlib.h>
#include <stdio.h>

#include <bstrlib.h>
#include <bstrlib_helper.h>
#include <error.h>

#include <read_yaml_ptt.h>
#include <test_types.h>

int global_verbosity = DEBUGLEV_DEVELOP;

int main(int argc, char* argv[])
{
    TestConfig_t config = NULL;
    char* filename = "load.yaml";

    int ret = read_yaml_ptt(filename, &config);
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



