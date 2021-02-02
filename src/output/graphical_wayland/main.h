#define WAYLAND_DISPLAY_NUM 4
#define WAYLAND_DISPLAY_NAME "wayland"

void cleanup_graphical_wayland(void);
int init_window_wayland(void);
void clear_screen_wayland(void);
int apply_window_settings_wayland(void);
int get_window_input_wayland(void);
void draw_graphical_wayland(int bars, int rest, int f[200], int flastd[200]);
