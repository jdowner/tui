#include <vte/vte.h>


#define CLR_R(x)   (((x) & 0xff0000) >> 16)
#define CLR_G(x)   (((x) & 0x00ff00) >>  8)
#define CLR_B(x)   (((x) & 0x0000ff) >>  0)
#define CLR_16(x)  ((double)(x) / 0xff)
#define CLR_GDK(x) (const GdkRGBA){ .red = CLR_16(CLR_R(x)), \
                                    .green = CLR_16(CLR_G(x)), \
                                    .blue = CLR_16(CLR_B(x)), \
                                    .alpha = 1 }

#define PALETTE_SIZE 16

#define MOD_KEY (GDK_CONTROL_MASK | GDK_SHIFT_MASK)

static const GdkRGBA GRUVBOX[PALETTE_SIZE] = {
  CLR_GDK(0x1e1e1e),
  CLR_GDK(0xbe0f17),
  CLR_GDK(0x868715),
  CLR_GDK(0xcc881a),
  CLR_GDK(0x377375),
  CLR_GDK(0xa04b73),
  CLR_GDK(0x578e57),
  CLR_GDK(0x978771),
  CLR_GDK(0x7f7061),
  CLR_GDK(0xf73028),
  CLR_GDK(0xaab01e),
  CLR_GDK(0xf7b125),
  CLR_GDK(0x719586),
  CLR_GDK(0xc77089),
  CLR_GDK(0x7db669),
  CLR_GDK(0xe6d4a3),
};

typedef struct {
  GtkWidget *window;
  GtkWidget *cmdline;
  GtkWidget *terminal;
  GtkWidget *overlay;
  gboolean cmdline_visible;
} tui_t;

int
main(int argc, char *argv[])
{
    GtkWidget *window, *terminal;

    /* Initialise GTK, the window and the terminal */
    gtk_init(&argc, &argv);
    terminal = vte_terminal_new();
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "tui");

    vte_terminal_set_colors(
        VTE_TERMINAL(terminal),
        NULL,
        NULL,
        GRUVBOX,
        PALETTE_SIZE);

    vte_terminal_set_scrollback_lines(VTE_TERMINAL(terminal), 8192);
    vte_terminal_set_scroll_on_output(VTE_TERMINAL(terminal), FALSE);
    vte_terminal_set_scroll_on_keystroke(VTE_TERMINAL(terminal), TRUE);
    vte_terminal_set_rewrap_on_resize(VTE_TERMINAL(terminal), TRUE);
    vte_terminal_set_mouse_autohide(VTE_TERMINAL(terminal), TRUE);
    vte_terminal_set_allow_bold(VTE_TERMINAL(terminal), FALSE);

    /* Start a new shell */
    gchar **envp = g_get_environ();
    gchar **command = (gchar *[]){g_strdup(g_environ_getenv(envp, "SHELL")), NULL };
    g_strfreev(envp);
    vte_terminal_spawn_sync(VTE_TERMINAL(terminal),
        VTE_PTY_DEFAULT,
        NULL,       /* working directory  */
        command,    /* command */
        NULL,       /* environment */
        0,          /* spawn flags */
        NULL, NULL, /* child setup */
        NULL,       /* child pid */
        NULL, NULL);

    /* Connect some signals */
    g_signal_connect(window, "delete-event", gtk_main_quit, NULL);
    g_signal_connect(terminal, "child-exited", gtk_main_quit, NULL);

    /* Put widgets together and run the main loop */
    gtk_container_add(GTK_CONTAINER(window), terminal);
    gtk_widget_show_all(window);
    gtk_main();
}
