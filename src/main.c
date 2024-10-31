#include <glib.h>
#include <gtk/gtk.h>
#include <vte/vte.h>

typedef struct {
    char* execute;
    GArray* colors;
    char* font_family;
} Config;

static Config config = {
    .execute = NULL,
    .colors = NULL,
    .font_family = NULL
};

static void parse_config(const char* config_path) {
    GError* error = NULL;
    char* content = NULL;
    gsize length;

    // Initialize colors array
    config.colors = g_array_new(FALSE, TRUE, sizeof(GdkRGBA));

    // Get config file path
    char* home = g_strdup(g_get_home_dir());
    char* default_config_path = g_build_filename(home, ".config", "lightty", "lightty.conf", NULL);
    const char* final_path = config_path ? config_path : default_config_path;

    if (!g_file_get_contents(final_path, &content, &length, &error)) {
        g_warning("Failed to read config file: %s", error->message);
        g_error_free(error);
        return;
    }

    GKeyFile* key_file = g_key_file_new();
    if (!g_key_file_load_from_data(key_file, content, length, G_KEY_FILE_NONE, &error)) {
        g_warning("Failed to parse config file: %s", error->message);
        g_error_free(error);
        return;
    }

    // Parse execute command
    config.execute = g_key_file_get_string(key_file, "General", "Execute", &error);
    if (error) {
        g_warning("Failed to read Execute: %s", error->message);
        g_clear_error(&error);
        config.execute = g_strdup("/usr/bin/bash");
    }

    // Parse font family
    config.font_family = g_key_file_get_string(key_file, "General", "FontFamily", &error);
    if (error) {
        g_warning("Failed to read FontFamily: %s", error->message);
        g_clear_error(&error);
        config.font_family = g_strdup("Monospace");
    }

    // Parse colors
    char** colors = g_key_file_get_string_list(key_file, "General", "Colors", NULL, &error);
    if (!error && colors) {
        for (int i = 0; colors[i] != NULL; i++) {
            GdkRGBA color;
            if (gdk_rgba_parse(&color, colors[i])) {
                g_array_append_val(config.colors, color);
            }
        }
        g_strfreev(colors);
    }

    g_key_file_free(key_file);
    g_free(content);
    g_free(home);
    g_free(default_config_path);
}

static void terminal_spawn_callback(VteTerminal *terminal, GPid pid, GError *error, gpointer user_data) {
    if (error) {
        g_warning("Failed to spawn terminal: %s", error->message);
    }
}

static void window_destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Parse config
    parse_config(NULL);

    // Create window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "LightTY");
    g_signal_connect(window, "destroy", G_CALLBACK(window_destroy), NULL);

    // Create terminal
    GtkWidget *terminal = vte_terminal_new();

    // Apply font
    PangoFontDescription *font_desc = pango_font_description_from_string(config.font_family);
    vte_terminal_set_font(VTE_TERMINAL(terminal), font_desc);
    pango_font_description_free(font_desc);

    // Apply colors
    if (config.colors->len >= 2) {
        GdkRGBA foreground = g_array_index(config.colors, GdkRGBA, 0);
        GdkRGBA background = g_array_index(config.colors, GdkRGBA, 1);
        vte_terminal_set_colors(VTE_TERMINAL(terminal), &foreground, &background,
                              (const GdkRGBA*)config.colors->data,
                              MIN(config.colors->len, 16));
    }

    // Set up terminal
    char *cmd_argv[] = { config.execute, NULL };  // Renamed from argv to cmd_argv
    char **envv = g_get_environ();
    vte_terminal_spawn_async(
        VTE_TERMINAL(terminal),
        VTE_PTY_DEFAULT,
        NULL,       // working directory
        cmd_argv,   // command (using renamed array)
        envv,       // environment
        G_SPAWN_SEARCH_PATH,
        NULL, NULL, // child setup
        NULL,       // child pid
        -1,        // timeout
        NULL,       // cancellable
        terminal_spawn_callback,
        NULL
    );
    g_strfreev(envv);

    // Set up scrolling
    gtk_container_add(GTK_CONTAINER(window), terminal);
    gtk_widget_show_all(window);

    // Main loop
    gtk_main();

    // Cleanup
    if (config.execute) g_free(config.execute);
    if (config.font_family) g_free(config.font_family);
    if (config.colors) g_array_unref(config.colors);

    return 0;
}