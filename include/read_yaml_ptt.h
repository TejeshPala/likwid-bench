#ifndef READ_YAML_PTT_H
#define READ_YAML_PTT_H

#include <test_types.h>



int read_yaml_ptt(char* filename, TestConfig_t* config);
void close_yaml_ptt(TestConfig_t config);

#endif
