#ifndef VIBE_CONFIG_H
#define VIBE_CONFIG_H

#define VIBE_CONFIG_FILENAME    ".vibe-req.yaml"
#define CONFIG_IGNORE_DIRS_MAX  32
#define CONFIG_IGNORE_DIR_LEN   256
#define CONFIG_PATH_BUF_LEN     512

/*
 * Project-level configuration read from .vibe-req.yaml.
 */
typedef struct {
    char ignore_dirs[CONFIG_IGNORE_DIRS_MAX][CONFIG_IGNORE_DIR_LEN];
    int  ignore_dirs_count;
} VibeConfig;

/*
 * Load <root_dir>/.vibe-req.yaml and populate *cfg.
 *
 * Returns  0 on success (file exists and was parsed).
 * Returns -1 if the file cannot be opened or parsed; *cfg is zeroed so
 *            the caller can safely treat it as "no ignored directories".
 */
#ifdef __cplusplus
extern "C" {
#endif

int config_load(const char *root_dir, VibeConfig *cfg);

/*
 * Returns 1 if dir_name matches any entry in cfg->ignore_dirs,
 * 0 otherwise.  Safe to call with cfg == NULL (always returns 0).
 */
int config_is_ignored_dir(const VibeConfig *cfg, const char *dir_name);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VIBE_CONFIG_H */
