/* gui_main.c - GTK3 graphical interface for Smart Dictionary & Autocomplete Engine
 *
 * Shares all core logic (BST/AVL/TBT/loader/autocomplete/benchmark) with
 * the CLI version.  Only the presentation layer is different.
 *
 * Build:  make gui          (see Makefile)
 * Output: smart_dict_gui.exe
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>          /* _dup, _dup2, _fileno — MinGW/Windows */

#include "config.h"
#include "dictionary.h"
#include "utils.h"
#include "bst.h"
#include "avl.h"
#include "tbt.h"
#include "loader.h"
#include "autocomplete.h"
#include "benchmark.h"

/* ── Forward declarations ────────────────────────────────────── */
static void on_search_changed(GtkSearchEntry *entry, gpointer data);
static void update_stats(void);
static void show_status(const gchar *msg);
static void clear_word_detail(void);

/* ── Global tree state ───────────────────────────────────────── */
static BSTNode *g_bst_root    = NULL;
static AVLNode *g_avl_root    = NULL;
static TBTNode *g_tbt_header  = NULL;
static int      g_active_tree = 2;    /* 1=BST  2=AVL  3=TBT */
static int      g_word_count  = 0;

/* ── Widget references (set during UI construction) ──────────── */
static GtkWidget *g_window         = NULL;
static GtkWidget *g_search_entry   = NULL;
static GtkWidget *g_result_listbox = NULL;
static GtkWidget *g_lbl_word       = NULL;
static GtkWidget *g_lbl_pos        = NULL;
static GtkWidget *g_lbl_meaning    = NULL;
static GtkWidget *g_lbl_freq       = NULL;
static GtkWidget *g_lbl_picks      = NULL;
static GtkWidget *g_lbl_stats      = NULL;
static GtkWidget *g_lbl_status     = NULL;
static GtkWidget *g_combo_tree     = NULL;

/* Last word the user interacted with */
static char g_selected_word[MAX_WORD_LEN] = "";

/* ── Tiny helpers ────────────────────────────────────────────── */

static const char *active_tree_name(void) {
    if (g_active_tree == 2) return "AVL";
    if (g_active_tree == 3) return "TBT";
    return "BST";
}

static void show_status(const gchar *msg) {
    if (g_lbl_status)
        gtk_label_set_text(GTK_LABEL(g_lbl_status), msg);
}

static void update_stats(void) {
    gchar buf[256];
    if (!g_lbl_stats) return;
    g_snprintf(buf, sizeof(buf),
               "Words: %d  |  BST h=%d  |  AVL h=%d  |  Active: %s",
               g_word_count,
               bst_height(g_bst_root),
               avl_height(g_avl_root),
               active_tree_name());
    gtk_label_set_text(GTK_LABEL(g_lbl_stats), buf);
}

/* ── Detail panel ────────────────────────────────────────────── */

static void show_word_detail(const WordRecord *rec) {
    gchar buf[32];
    gtk_label_set_text(GTK_LABEL(g_lbl_word),
                       rec->word[0] ? rec->word : "—");
    gtk_label_set_text(GTK_LABEL(g_lbl_pos),
                       rec->part_of_speech[0] ? rec->part_of_speech : "—");
    gtk_label_set_text(GTK_LABEL(g_lbl_meaning),
                       rec->meaning[0] ? rec->meaning : "—");
    g_snprintf(buf, sizeof(buf), "%d", rec->frequency_score);
    gtk_label_set_text(GTK_LABEL(g_lbl_freq), buf);
    g_snprintf(buf, sizeof(buf), "%d", rec->user_select_count);
    gtk_label_set_text(GTK_LABEL(g_lbl_picks), buf);
}

static void clear_word_detail(void) {
    gtk_label_set_text(GTK_LABEL(g_lbl_word),    "—");
    gtk_label_set_text(GTK_LABEL(g_lbl_pos),     "");
    gtk_label_set_text(GTK_LABEL(g_lbl_meaning), "Type a prefix and select a word.");
    gtk_label_set_text(GTK_LABEL(g_lbl_freq),    "—");
    gtk_label_set_text(GTK_LABEL(g_lbl_picks),   "—");
}

/* ── Result list ─────────────────────────────────────────────── */

/* Build one row widget for the search-results listbox. */
static GtkWidget *build_result_row(const WordRecord *rec) {
    GtkWidget *row, *box, *lbl_word, *lbl_score;
    gchar score_buf[32];
    gchar *word_copy;

    row  = gtk_list_box_row_new();
    box  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 6);

    lbl_word = gtk_label_new(rec->word);
    gtk_label_set_xalign(GTK_LABEL(lbl_word), 0.0f);
    gtk_widget_set_hexpand(lbl_word, TRUE);

    g_snprintf(score_buf, sizeof(score_buf), "%d",
               rec->frequency_score + 10 * rec->user_select_count);
    lbl_score = gtk_label_new(score_buf);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_score),
                                "score-label");

    gtk_box_pack_start(GTK_BOX(box), lbl_word,  TRUE,  TRUE,  0);
    gtk_box_pack_end  (GTK_BOX(box), lbl_score, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(row), box);

    /* Attach word string so on_row_activated can retrieve it */
    word_copy = g_strdup(rec->word);
    g_object_set_data_full(G_OBJECT(row), "word", word_copy, g_free);

    gtk_widget_show_all(row);
    return row;
}

/* Clear and repopulate the results listbox. */
static void populate_results(const WordRecord *records, int n) {
    GList *children, *c;
    int i;

    /* Remove existing rows */
    children = gtk_container_get_children(GTK_CONTAINER(g_result_listbox));
    for (c = children; c; c = c->next)
        gtk_container_remove(GTK_CONTAINER(g_result_listbox),
                             GTK_WIDGET(c->data));
    g_list_free(children);

    for (i = 0; i < n; i++)
        gtk_container_add(GTK_CONTAINER(g_result_listbox),
                          build_result_row(&records[i]));

    gtk_widget_show_all(g_result_listbox);
}

/* ── Signal callbacks ────────────────────────────────────────── */

/* Called as user types in the search box (after GTK's 150 ms debounce). */
static void on_search_changed(GtkSearchEntry *entry, gpointer data) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    WordRecord   results[TOP_K_DEFAULT];
    int          n;
    gchar        msg[64];

    (void)data;

    if (str_is_empty(text)) {
        populate_results(NULL, 0);
        clear_word_detail();
        show_status("Type a prefix to search.");
        return;
    }

    if (g_active_tree == 2)
        n = autocomplete_avl(g_avl_root,   text, results, TOP_K_DEFAULT);
    else if (g_active_tree == 3)
        n = autocomplete_tbt(g_tbt_header, text, results, TOP_K_DEFAULT);
    else
        n = autocomplete_bst(g_bst_root,   text, results, TOP_K_DEFAULT);

    populate_results(results, n);

    if (n == 0)
        show_status("No matches found.");
    else {
        g_snprintf(msg, sizeof(msg), "%d match%s for \"%s\"",
                   n, n == 1 ? "" : "es", text);
        show_status(msg);
    }
}

/* Called when the user presses Enter in the search box (exact lookup). */
static void on_search_activate(GtkEntry *entry, gpointer data) {
    const gchar *text = gtk_entry_get_text(entry);
    BSTNode     *node;
    gchar        msg[128];

    (void)data;

    if (str_is_empty(text)) return;

    node = bst_search(g_bst_root, text);
    if (node) {
        show_word_detail(&node->data);
        str_safe_copy(g_selected_word, text, sizeof(g_selected_word));
        autocomplete_record_selection(text,
                                      g_bst_root, g_avl_root, g_tbt_header);
        g_snprintf(msg, sizeof(msg), "Found \"%s\".", text);
    } else {
        g_snprintf(msg, sizeof(msg), "\"%s\" not found.", text);
    }
    show_status(msg);
}

/* Called when the user clicks / activates a row in the results list. */
static void on_row_activated(GtkListBox    *listbox,
                              GtkListBoxRow *row,
                              gpointer       data) {
    const gchar *word;
    BSTNode     *bst_n;
    AVLNode     *avl_n;
    TBTNode     *tbt_n;
    gchar        msg[128];

    (void)listbox; (void)data;
    if (!row) return;

    word = (const gchar *)g_object_get_data(G_OBJECT(row), "word");
    if (!word) return;
    str_safe_copy(g_selected_word, word, sizeof(g_selected_word));

    /* Look up full record in whichever tree is active */
    if (g_active_tree == 2) {
        avl_n = avl_search(g_avl_root, word);
        if (avl_n) { show_word_detail(&avl_n->data); goto done; }
    } else if (g_active_tree == 3) {
        tbt_n = tbt_search(g_tbt_header, word);
        if (tbt_n) { show_word_detail(&tbt_n->data); goto done; }
    }
    bst_n = bst_search(g_bst_root, word);
    if (bst_n) { show_word_detail(&bst_n->data); }

done:
    /* Record user selection for personalised autocomplete scoring */
    autocomplete_record_selection(word,
                                  g_bst_root, g_avl_root, g_tbt_header);
    g_snprintf(msg, sizeof(msg), "Selected \"%s\".", word);
    show_status(msg);
}

/* Called when the tree selector combo changes. */
static void on_tree_changed(GtkComboBox *combo, gpointer data) {
    gint idx = gtk_combo_box_get_active(combo);
    (void)data;
    g_active_tree = idx + 1;   /* combo indices 0/1/2 → trees 1/2/3 */
    update_stats();
    /* Re-run the current search so results come from the new tree */
    on_search_changed(GTK_SEARCH_ENTRY(g_search_entry), NULL);
}

/* ── Button callbacks ────────────────────────────────────────── */

static void on_insert_clicked(GtkButton *btn, gpointer data) {
    GtkWidget  *dialog, *content, *grid;
    GtkWidget  *lbl, *entry_word, *entry_pos, *entry_meaning;
    gint        resp;

    (void)btn; (void)data;

    dialog = gtk_dialog_new_with_buttons(
                 "Insert Word", GTK_WINDOW(g_window),
                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                 "Cancel", GTK_RESPONSE_CANCEL,
                 "Insert", GTK_RESPONSE_OK,
                 NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 380, -1);

    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    grid    = gtk_grid_new();
    gtk_grid_set_row_spacing   (GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 18);

    /* Row 0 – Word */
    lbl = gtk_label_new("Word:");
    gtk_label_set_xalign(GTK_LABEL(lbl), 1.0f);
    entry_word = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_word), "Required");
    gtk_widget_set_hexpand(entry_word, TRUE);
    gtk_grid_attach(GTK_GRID(grid), lbl,        0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_word, 1, 0, 1, 1);

    /* Row 1 – Part of speech */
    lbl = gtk_label_new("Part of speech:");
    gtk_label_set_xalign(GTK_LABEL(lbl), 1.0f);
    entry_pos = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_pos), "noun, verb, adj…");
    gtk_grid_attach(GTK_GRID(grid), lbl,       0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_pos, 1, 1, 1, 1);

    /* Row 2 – Meaning */
    lbl = gtk_label_new("Meaning:");
    gtk_label_set_xalign(GTK_LABEL(lbl), 1.0f);
    entry_meaning = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_meaning), "Optional definition");
    gtk_widget_set_size_request(entry_meaning, 260, -1);
    gtk_grid_attach(GTK_GRID(grid), lbl,           0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_meaning, 1, 2, 1, 1);

    gtk_box_pack_start(GTK_BOX(content), grid, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);
    resp = gtk_dialog_run(GTK_DIALOG(dialog));

    if (resp == GTK_RESPONSE_OK) {
        const gchar *w = gtk_entry_get_text(GTK_ENTRY(entry_word));
        const gchar *p = gtk_entry_get_text(GTK_ENTRY(entry_pos));
        const gchar *m = gtk_entry_get_text(GTK_ENTRY(entry_meaning));

        if (!str_is_empty(w)) {
            WordRecord rec;
            int prev = g_word_count;

            word_record_init(&rec);
            str_safe_copy(rec.word,           w, sizeof(rec.word));
            str_safe_copy(rec.part_of_speech, p, sizeof(rec.part_of_speech));
            str_safe_copy(rec.meaning,        m, sizeof(rec.meaning));
            rec.frequency_score = FREQ_SCORE_DEFAULT;

            bst_insert(&g_bst_root, &rec);
            g_avl_root = avl_insert(g_avl_root, &rec);
            tbt_insert(g_tbt_header, &rec);
            g_word_count = bst_count(g_bst_root);

            if (g_word_count > prev) {
                gchar msg[128];
                g_snprintf(msg, sizeof(msg), "Inserted \"%s\".", w);
                show_status(msg);
                update_stats();
            } else {
                show_status("Word already exists — skipped.");
            }
        } else {
            show_status("Insert cancelled: word field was empty.");
        }
    }

    gtk_widget_destroy(dialog);
}

static void on_delete_clicked(GtkButton *btn, gpointer data) {
    GtkListBoxRow *row;
    const gchar   *word = NULL;
    GtkWidget     *confirm;
    gint           resp;

    (void)btn; (void)data;

    row = gtk_list_box_get_selected_row(GTK_LIST_BOX(g_result_listbox));
    if (row)
        word = (const gchar *)g_object_get_data(G_OBJECT(row), "word");
    if (!word || !word[0])
        word = g_selected_word[0] ? g_selected_word : NULL;

    if (!word || !word[0]) {
        show_status("Select a word from the list to delete.");
        return;
    }

    confirm = gtk_message_dialog_new(
                  GTK_WINDOW(g_window), GTK_DIALOG_MODAL,
                  GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                  "Delete \"%s\" from all three trees?", word);
    resp = gtk_dialog_run(GTK_DIALOG(confirm));
    gtk_widget_destroy(confirm);

    if (resp == GTK_RESPONSE_YES) {
        int prev = g_word_count;
        bst_delete  (&g_bst_root, word);
        g_avl_root = avl_delete(g_avl_root, word);
        tbt_delete  (g_tbt_header, word);
        g_word_count = bst_count(g_bst_root);

        if (g_word_count < prev) {
            gchar msg[128];
            g_snprintf(msg, sizeof(msg), "Deleted \"%s\".", word);
            show_status(msg);
            g_selected_word[0] = '\0';
            clear_word_detail();
            update_stats();
            on_search_changed(GTK_SEARCH_ENTRY(g_search_entry), NULL);
        } else {
            show_status("Word not found.");
        }
    }
}

static void on_load_clicked(GtkButton *btn, gpointer data) {
    GtkWidget     *fc;
    GtkFileFilter *filter;
    gint           resp;

    (void)btn; (void)data;

    fc = gtk_file_chooser_dialog_new(
             "Load Dictionary File", GTK_WINDOW(g_window),
             GTK_FILE_CHOOSER_ACTION_OPEN,
             "Cancel", GTK_RESPONSE_CANCEL,
             "Load",   GTK_RESPONSE_ACCEPT,
             NULL);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Text files (*.txt)");
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fc), filter);

    resp = gtk_dialog_run(GTK_DIALOG(fc));
    if (resp == GTK_RESPONSE_ACCEPT) {
        gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
        int n;

        bst_free  (&g_bst_root);
        avl_free  (&g_avl_root);
        tbt_free  (&g_tbt_header);
        g_tbt_header = tbt_create_header();

        n = load_words(path, &g_bst_root, &g_avl_root, g_tbt_header);
        if (n > 0) {
            load_frequencies(FILE_WORD_FREQ,
                             g_bst_root, g_avl_root, g_tbt_header);
            g_word_count = bst_count(g_bst_root);
            gchar msg[128];
            g_snprintf(msg, sizeof(msg), "Loaded %d words.", n);
            show_status(msg);
            clear_word_detail();
            populate_results(NULL, 0);
            update_stats();
        } else {
            show_status("Failed to load file (empty or not found).");
        }
        g_free(path);
    }
    gtk_widget_destroy(fc);
}

static void on_save_clicked(GtkButton *btn, gpointer data) {
    gchar msg[128];
    (void)btn; (void)data;

    if (!g_bst_root) { show_status("Nothing to save."); return; }

    if (save_custom_words(FILE_CUSTOM_WORDS, g_bst_root) == 0) {
        g_snprintf(msg, sizeof(msg),
                   "Saved %d words to %s.", g_word_count, FILE_CUSTOM_WORDS);
        show_status(msg);
    } else {
        show_status("Error: could not write session file.");
    }
}

/* Run benchmark_run_all(), capture its printf output via stdout redirect,
   then display the captured text in a scrollable dialog. */
static void on_benchmark_clicked(GtkButton *btn, gpointer data) {
    char   tmppath[512];
    int    saved_fd = -1;
    FILE  *rf;
    long   sz;
    gchar *output = NULL;
    GtkWidget *dialog, *content, *scroll, *tv;
    GtkTextBuffer *tbuf;

    (void)btn; (void)data;

    /* ── Capture stdout to a temp file ── */
    g_snprintf(tmppath, sizeof(tmppath),
               "%s\\sdict_bench.txt", g_get_tmp_dir());

    if (_fileno(stdout) >= 0)
        saved_fd = _dup(_fileno(stdout));

    freopen(tmppath, "w", stdout);
    benchmark_run_all();
    fflush(stdout);

    if (saved_fd >= 0) {
        _dup2(saved_fd, _fileno(stdout));
        _close(saved_fd);
    } else {
        freopen("NUL", "w", stdout);
    }

    /* ── Read captured output ── */
    rf = fopen(tmppath, "r");
    if (rf) {
        fseek(rf, 0, SEEK_END);
        sz = ftell(rf);
        rewind(rf);
        output = (gchar *)g_malloc((gsize)(sz + 1));
        if (output) {
            fread(output, 1, (size_t)sz, rf);
            output[sz] = '\0';
        }
        fclose(rf);
    }
    remove(tmppath);

    if (!output)
        output = g_strdup("(Could not read benchmark output.)");

    /* ── Show in a resizable dialog with a monospace text view ── */
    dialog = gtk_dialog_new_with_buttons(
                 "Benchmark Results", GTK_WINDOW(g_window),
                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                 "Close", GTK_RESPONSE_CLOSE,
                 NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 640, 420);

    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    scroll  = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(scroll), 8);

    tv   = gtk_text_view_new();
    tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_text_buffer_set_text(tbuf, output, -1);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(tv), 8);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(tv), 8);

    gtk_container_add(GTK_CONTAINER(scroll), tv);
    gtk_box_pack_start(GTK_BOX(content), scroll, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free(output);
}

/* Auto-save on window close. */
static void on_window_destroy(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    if (g_bst_root)
        save_custom_words(FILE_CUSTOM_WORDS, g_bst_root);
    bst_free  (&g_bst_root);
    avl_free  (&g_avl_root);
    tbt_free  (&g_tbt_header);
}

/* ── CSS ─────────────────────────────────────────────────────── */

static void apply_css(void) {
    static const gchar *CSS =
        /* overall window background */
        "window { background-color: #f2f2f2; }"

        /* left sidebar */
        ".sidebar { background-color: #ffffff;"
        "           border-right: 1px solid #d0d0d0; }"

        /* right detail area */
        ".detail { background-color: #ffffff;"
        "          padding: 16px; }"

        /* large word heading */
        ".word-title { font-size: 22px; font-weight: bold;"
        "              color: #1a1a1a; }"

        /* POS tag beneath word */
        ".pos-tag { color: #555555; font-style: italic; }"

        /* section header labels (Meaning, Frequency…) */
        ".section-hdr { font-weight: bold; color: #333333; }"

        /* small score number in result rows */
        ".score-label { color: #1565c0; font-size: 11px; }"

        /* result row hover / selected */
        "row:hover   { background-color: #e8f0fe; }"
        "row:selected { background-color: #1976d2; }"
        "row:selected label { color: #ffffff; }"

        /* toolbar */
        ".toolbar { background-color: #e8e8e8;"
        "           border-bottom: 1px solid #c8c8c8; }"

        /* status bar */
        ".statusbar { color: #444444; font-size: 11px; }";

    GtkCssProvider *prov;
    GdkDisplay     *disp;
    GdkScreen      *scr;
    GError         *err = NULL;

    prov = gtk_css_provider_new();
    gtk_css_provider_load_from_data(prov, CSS, -1, &err);
    if (err) {
        g_warning("CSS error: %s", err->message);
        g_error_free(err);
    }
    disp = gdk_display_get_default();
    scr  = gdk_display_get_default_screen(disp);
    gtk_style_context_add_provider_for_screen(
        scr, GTK_STYLE_PROVIDER(prov),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(prov);
}

/* ── UI construction ─────────────────────────────────────────── */

static GtkWidget *build_toolbar(void) {
    GtkWidget *bar, *btn, *sep, *lbl;

    bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(bar), 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(bar), "toolbar");

    btn = gtk_button_new_with_label("Insert");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_insert_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(bar), btn, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label("Delete");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_delete_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(bar), btn, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label("Load File");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_load_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(bar), btn, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label("Save Session");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_save_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(bar), btn, FALSE, FALSE, 0);

    btn = gtk_button_new_with_label("Benchmark");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_benchmark_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(bar), btn, FALSE, FALSE, 0);

    /* Vertical separator */
    sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(bar), sep, FALSE, FALSE, 6);

    /* Tree selector */
    lbl = gtk_label_new("Active tree:");
    gtk_box_pack_start(GTK_BOX(bar), lbl, FALSE, FALSE, 0);

    g_combo_tree = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(g_combo_tree),
                              "bst", "BST  (Binary Search Tree)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(g_combo_tree),
                              "avl", "AVL  (Self-Balancing)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(g_combo_tree),
                              "tbt", "TBT  (Threaded)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(g_combo_tree), 1); /* default AVL */
    g_signal_connect(g_combo_tree, "changed",
                     G_CALLBACK(on_tree_changed), NULL);
    gtk_box_pack_start(GTK_BOX(bar), g_combo_tree, FALSE, FALSE, 0);

    return bar;
}

static GtkWidget *build_left_panel(void) {
    GtkWidget *box, *scroll;

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(box, 230, -1);
    gtk_style_context_add_class(gtk_widget_get_style_context(box), "sidebar");

    /* Search entry */
    g_search_entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_search_entry),
                                   "Type a prefix…");
    gtk_widget_set_margin_start  (g_search_entry, 8);
    gtk_widget_set_margin_end    (g_search_entry, 8);
    gtk_widget_set_margin_top    (g_search_entry, 8);
    gtk_widget_set_margin_bottom (g_search_entry, 8);
    g_signal_connect(g_search_entry, "search-changed",
                     G_CALLBACK(on_search_changed), NULL);
    g_signal_connect(g_search_entry, "activate",
                     G_CALLBACK(on_search_activate), NULL);
    gtk_box_pack_start(GTK_BOX(box), g_search_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 0);

    /* Scrollable results list */
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);

    g_result_listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(g_result_listbox),
                                    GTK_SELECTION_SINGLE);
    g_signal_connect(g_result_listbox, "row-activated",
                     G_CALLBACK(on_row_activated), NULL);
    gtk_container_add(GTK_CONTAINER(scroll), g_result_listbox);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

    /* Stats label at bottom of sidebar */
    gtk_box_pack_end(GTK_BOX(box),
                     gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                     FALSE, FALSE, 0);
    g_lbl_stats = gtk_label_new("Words: 0");
    gtk_label_set_xalign(GTK_LABEL(g_lbl_stats), 0.0f);
    gtk_widget_set_margin_start  (g_lbl_stats, 8);
    gtk_widget_set_margin_end    (g_lbl_stats, 8);
    gtk_widget_set_margin_top    (g_lbl_stats, 4);
    gtk_widget_set_margin_bottom (g_lbl_stats, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(g_lbl_stats),
                                "statusbar");
    gtk_box_pack_end(GTK_BOX(box), g_lbl_stats, FALSE, FALSE, 0);

    return box;
}

static GtkWidget *build_right_panel(void) {
    GtkWidget *box, *grid, *lbl;

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(box), "detail");
    gtk_widget_set_margin_start  (box, 20);
    gtk_widget_set_margin_end    (box, 20);
    gtk_widget_set_margin_top    (box, 16);
    gtk_widget_set_margin_bottom (box, 16);

    /* Word (large heading) */
    g_lbl_word = gtk_label_new("—");
    gtk_label_set_xalign(GTK_LABEL(g_lbl_word), 0.0f);
    gtk_style_context_add_class(gtk_widget_get_style_context(g_lbl_word),
                                "word-title");
    gtk_widget_set_margin_bottom(g_lbl_word, 2);
    gtk_box_pack_start(GTK_BOX(box), g_lbl_word, FALSE, FALSE, 0);

    /* Part of speech */
    g_lbl_pos = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(g_lbl_pos), 0.0f);
    gtk_style_context_add_class(gtk_widget_get_style_context(g_lbl_pos),
                                "pos-tag");
    gtk_widget_set_margin_bottom(g_lbl_pos, 12);
    gtk_box_pack_start(GTK_BOX(box), g_lbl_pos, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 0);

    /* Meaning section */
    lbl = gtk_label_new("Meaning");
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl),
                                "section-hdr");
    gtk_widget_set_margin_top(lbl, 12);
    gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);

    g_lbl_meaning = gtk_label_new("Type a prefix and select a word.");
    gtk_label_set_xalign     (GTK_LABEL(g_lbl_meaning), 0.0f);
    gtk_label_set_line_wrap  (GTK_LABEL(g_lbl_meaning), TRUE);
    gtk_label_set_line_wrap_mode(GTK_LABEL(g_lbl_meaning),
                                 PANGO_WRAP_WORD_CHAR);
    gtk_widget_set_hexpand   (g_lbl_meaning, TRUE);
    gtk_widget_set_margin_top   (g_lbl_meaning, 4);
    gtk_widget_set_margin_bottom(g_lbl_meaning, 16);
    gtk_box_pack_start(GTK_BOX(box), g_lbl_meaning, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 0);

    /* Frequency / Picks grid */
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing   (GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 40);
    gtk_widget_set_margin_top  (grid, 12);

    lbl = gtk_label_new("Frequency Score");
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl),
                                "section-hdr");
    gtk_grid_attach(GTK_GRID(grid), lbl, 0, 0, 1, 1);

    g_lbl_freq = gtk_label_new("—");
    gtk_label_set_xalign(GTK_LABEL(g_lbl_freq), 0.0f);
    gtk_grid_attach(GTK_GRID(grid), g_lbl_freq, 0, 1, 1, 1);

    lbl = gtk_label_new("User Picks");
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl),
                                "section-hdr");
    gtk_grid_attach(GTK_GRID(grid), lbl, 1, 0, 1, 1);

    g_lbl_picks = gtk_label_new("—");
    gtk_label_set_xalign(GTK_LABEL(g_lbl_picks), 0.0f);
    gtk_grid_attach(GTK_GRID(grid), g_lbl_picks, 1, 1, 1, 1);

    gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 0);

    return box;
}

/* ── Application entry point ─────────────────────────────────── */

static void activate(GtkApplication *app, gpointer data) {
    GtkWidget *main_box, *paned, *left, *right;
    gchar      msg[128];
    int        n;

    (void)data;

    /* Window */
    g_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(g_window),
                         APP_NAME "  v" APP_VERSION);
    gtk_window_set_default_size(GTK_WINDOW(g_window), 920, 600);
    gtk_window_set_position(GTK_WINDOW(g_window), GTK_WIN_POS_CENTER);
    g_signal_connect(g_window, "destroy",
                     G_CALLBACK(on_window_destroy), NULL);

    apply_css();

    /* Outer vertical box */
    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Toolbar */
    gtk_box_pack_start(GTK_BOX(main_box), build_toolbar(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box),
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                       FALSE, FALSE, 0);

    /* Paned: left sidebar | right detail */
    paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    left  = build_left_panel();
    right = build_right_panel();
    gtk_paned_pack1(GTK_PANED(paned), left,  FALSE, FALSE);
    gtk_paned_pack2(GTK_PANED(paned), right, TRUE,  FALSE);
    gtk_paned_set_position(GTK_PANED(paned), 250);
    gtk_box_pack_start(GTK_BOX(main_box), paned, TRUE, TRUE, 0);

    /* Status bar */
    gtk_box_pack_end(GTK_BOX(main_box),
                     gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                     FALSE, FALSE, 0);
    g_lbl_status = gtk_label_new("Loading…");
    gtk_label_set_xalign(GTK_LABEL(g_lbl_status), 0.05f);
    gtk_widget_set_margin_top   (g_lbl_status, 3);
    gtk_widget_set_margin_bottom(g_lbl_status, 3);
    gtk_style_context_add_class(gtk_widget_get_style_context(g_lbl_status),
                                "statusbar");
    gtk_box_pack_end(GTK_BOX(main_box), g_lbl_status, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(g_window), main_box);
    gtk_widget_show_all(g_window);

    /* Auto-load: try custom session file first, then canonical words.txt */
    n = load_words(FILE_CUSTOM_WORDS, &g_bst_root, &g_avl_root, g_tbt_header);
    if (n <= 0)
        n = load_words(FILE_WORDS, &g_bst_root, &g_avl_root, g_tbt_header);

    if (n > 0) {
        load_frequencies(FILE_WORD_FREQ, g_bst_root, g_avl_root, g_tbt_header);
        g_word_count = bst_count(g_bst_root);
    }

    update_stats();
    g_snprintf(msg, sizeof(msg),
               "Ready — %d words loaded. Type a prefix to search.",
               g_word_count);
    show_status(msg);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int             status;

    /* Initialise TBT header before anything touches the trees */
    g_tbt_header = tbt_create_header();

    app = gtk_application_new("com.smartdict.gui",
                              G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
