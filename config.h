#ifndef SGS_CONFIG
#define SGS_CONFIG

// The settings of the config
typedef struct Config {
    char* repo_path;
    int port;
} Config;

/**
 * Loads in the config (specified by path) and puts its settings in the Config struct
 */
Config loadConfig(const char* path);

/**
 * frees the buffers allocated with malloc() of the Config struct
 */
void destroyConfig(Config config);

#endif