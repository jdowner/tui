#include <vte/vte.h>
#include <pango/pango.h>
#include <stdlib.h>


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
  PangoFontDescription *font;
} tui_t;

tui_t *tui_create() {
  tui_t *tui;

  tui = (tui_t*)malloc(sizeof(tui_t));
  tui->window = NULL;
  tui->cmdline = NULL;
  tui->terminal = NULL;
  tui->overlay = NULL;
  tui->cmdline_visible = FALSE;

  tui->font = pango_font_description_from_string("DejaVu Sans Mono,monospace 14");

  return tui;
}

void tui_destroy(tui_t **ref) {
  tui_t *tui = *ref;

  pango_font_description_free(tui->font);
  tui->window = NULL;
  tui->cmdline = NULL;
  tui->terminal = NULL;
  tui->overlay = NULL;
  tui->cmdline_visible = FALSE;

  free(tui);

  *ref = NULL;
}

void
tui_cmdline_show(tui_t *tui){
  if(!tui->cmdline_visible) {
    tui->cmdline_visible = TRUE;
    gtk_overlay_reorder_overlay(GTK_OVERLAY(tui->overlay), tui->terminal, 0);
    gtk_overlay_reorder_overlay(GTK_OVERLAY(tui->overlay), tui->cmdline, 1);
    gtk_window_set_focus(GTK_WINDOW(tui->window), tui->cmdline);
  }
}

void
tui_cmdline_hide(tui_t *tui){
  if(tui->cmdline_visible) {
    tui->cmdline_visible = FALSE;
    gtk_entry_set_text(GTK_ENTRY(tui->cmdline), "");
    gtk_overlay_reorder_overlay(GTK_OVERLAY(tui->overlay), tui->terminal, 1);
    gtk_overlay_reorder_overlay(GTK_OVERLAY(tui->overlay), tui->cmdline, 0);
    gtk_window_set_focus(GTK_WINDOW(tui->window), tui->terminal);
  }
}

void
tui_increase_font_size(tui_t *tui){
  pango_font_description_set_size(tui->font,
      pango_font_description_get_size(tui->font) + PANGO_SCALE);

  vte_terminal_set_font(VTE_TERMINAL(tui->terminal), tui->font);
}

void
tui_decrease_font_size(tui_t *tui){
  gint size = pango_font_description_get_size(tui->font);

  if(size > 5 * PANGO_SCALE){
    pango_font_description_set_size(tui->font, size - PANGO_SCALE);
    vte_terminal_set_font(VTE_TERMINAL(tui->terminal), tui->font);
  }
}

static gboolean
on_entry_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer userdata) {
  tui_cmdline_hide((tui_t*)userdata);
  return FALSE;
}

static gboolean
on_window_key_press(GtkWidget *widget, GdkEventKey *event, gpointer userdata) {
  if(event->type != GDK_KEY_PRESS) {
    return FALSE;
  }

  tui_t *tui = (tui_t*)userdata;

  if((event->state & MOD_KEY) == MOD_KEY){
    if(event->keyval == GDK_KEY_colon) {
      tui_cmdline_show(tui);
      return TRUE;
    }
  }

  if(event->keyval == GDK_KEY_Escape) {
    if(((tui_t*)userdata)->cmdline_visible) {
      tui_cmdline_hide(tui);
      return TRUE;
    }
  }

  if((event->state & MOD_KEY) == MOD_KEY){
    if(event->keyval == GDK_KEY_P) {
      vte_terminal_paste_clipboard((VteTerminal*)tui->terminal);
      return TRUE;
    }
  }

  if((event->state & MOD_KEY) == MOD_KEY){
    if(event->keyval == GDK_KEY_Y) {
      vte_terminal_copy_clipboard((VteTerminal*)tui->terminal);
      return TRUE;
    }
  }

  if((event->state & MOD_KEY) == MOD_KEY){
    if(event->keyval == GDK_KEY_Up) {
      tui_increase_font_size(tui);
      return TRUE;
    }
  }

  if((event->state & MOD_KEY) == MOD_KEY){
    if(event->keyval == GDK_KEY_Down) {
      tui_decrease_font_size(tui);
      return TRUE;
    }
  }

  return FALSE;
}

int
main(int argc, char *argv[])
{
  GtkWidget *window, *terminal, *overlay, *cmdline;
  tui_t *tui;

  tui = tui_create();

  /* Initialise GTK, the window and the terminal */
  gtk_init(&argc, &argv);

  overlay = gtk_overlay_new();
  cmdline = gtk_entry_new();

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "tui");
  gtk_widget_set_valign(cmdline, GTK_ALIGN_END);

  /* Create the terminal widget */
  terminal = vte_terminal_new();

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

  /* Initialize the tui structure */
  tui->window = window;
  tui->overlay = overlay;
  tui->cmdline = cmdline;
  tui->terminal = terminal;
  tui->cmdline_visible = FALSE;

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
  g_signal_connect(window, "key-press-event", G_CALLBACK(on_window_key_press), tui);
  g_signal_connect(cmdline, "focus-out-event", G_CALLBACK(on_entry_focus_out), tui);

  /* Put widgets together and run the main loop */
  gtk_container_add(GTK_CONTAINER(window), overlay);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), cmdline);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), terminal);
  gtk_window_set_focus(GTK_WINDOW(window), terminal);

  gtk_widget_show_all(window);
  gtk_main();

  tui_destroy(&tui);
}
