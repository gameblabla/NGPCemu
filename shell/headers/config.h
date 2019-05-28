#ifndef CONFIG_H__
#define CONFIG_H__

typedef struct {
	int_fast8_t fullscreen;
	/* For input remapping */
	uint_fast16_t config_buttons[7];
} t_config;
extern t_config option;

#endif
