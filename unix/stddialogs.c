// 26 june 2015
#include "uipriv_unix.h"

// LONGTERM figure out why, and describe, that this is the desired behavior
// LONGTERM also point out that font and color buttons also work like this

#define windowWindow(w) (GTK_WINDOW(uiControlHandle(uiControl(w))))

static char *filedialog(GtkWindow *parent, GtkFileChooserAction mode, const gchar *confirm,
			const char *defaultPath, const char *defaultName, const char *filter)
{
	GtkWidget *fcd;
	GtkFileChooser *fc;
	gint response;
	char *filename;

	gchar _filter[256];
	gchar *fp = &_filter[0];
	gchar *fname;
	gchar *j;
	int i;
	int s;

	fcd = gtk_file_chooser_dialog_new(NULL, parent, mode,
		"_Cancel", GTK_RESPONSE_CANCEL,
		confirm, GTK_RESPONSE_ACCEPT,
		NULL);
	fc = GTK_FILE_CHOOSER(fcd);

	for (i = s = 0; i < 255; i++) {
		if (filter[i] == '|' || filter[i] == '\0') {
			_filter[i] = '\0';
			if (s & 1) {
				GtkFileFilter *filter = gtk_file_filter_new();
				gtk_file_filter_set_name(filter, fname);
				for (j = fp; ; j++) {
					if (*j == ';') {
						*j = '\0';
						gtk_file_filter_add_pattern(filter, fp);
						fp = j + 1;
					} else if (*j == '\0') {
						gtk_file_filter_add_pattern(filter, fp);
						break;
					}
				}
				gtk_file_chooser_add_filter(fc, filter);
			} else {
				fname = fp;
			}
			fp = &_filter[i + 1];
			s++;
			if (s >= 8) break;
			if (filter[i] == '\0') break;
		} else {
			_filter[i] = filter[i];
		}
	}

	gtk_file_chooser_set_local_only(fc, FALSE);
	gtk_file_chooser_set_select_multiple(fc, FALSE);
	gtk_file_chooser_set_show_hidden(fc, TRUE);
	gtk_file_chooser_set_do_overwrite_confirmation(fc, TRUE);
	gtk_file_chooser_set_create_folders(fc, TRUE);
	if (defaultPath != NULL && strlen(defaultPath) > 0)
		gtk_file_chooser_set_current_folder(fc, defaultPath);
	if (defaultName != NULL && strlen(defaultName) > 0)
		gtk_file_chooser_set_current_name(fc, defaultName);

	response = gtk_dialog_run(GTK_DIALOG(fcd));
	if (response != GTK_RESPONSE_ACCEPT) {
		gtk_widget_destroy(fcd);
		return NULL;
	}
	filename = gtk_file_chooser_get_filename(fc);
	gtk_widget_destroy(fcd);
	return filename;
}

char *uiOpenFile(uiWindow *parent, const char *defaultPath, const char *filter)
{
	return filedialog(windowWindow(parent), GTK_FILE_CHOOSER_ACTION_OPEN, "_Open",
			  defaultPath, NULL, filter);
}

char *uiOpenFolder(uiWindow *parent, const char *defaultPath)
{
	return filedialog(windowWindow(parent), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "_Open",
			  defaultPath, NULL, "");
}

char *uiSaveFile(uiWindow *parent, const char *defaultPath, const char *defaultName, const char *filter)
{
	return filedialog(windowWindow(parent), GTK_FILE_CHOOSER_ACTION_SAVE, "_Save",
			  defaultPath, defaultName, filter);
}

static void msgbox(GtkWindow *parent, const char *title, const char *description, GtkMessageType type, GtkButtonsType buttons)
{
	GtkWidget *md;

	md = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL,
		type, buttons,
		"%s", title);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(md), "%s", description);
	gtk_dialog_run(GTK_DIALOG(md));
	gtk_widget_destroy(md);
}

void uiMsgBox(uiWindow *parent, const char *title, const char *description)
{
	msgbox(windowWindow(parent), title, description, GTK_MESSAGE_OTHER, GTK_BUTTONS_OK);
}

void uiMsgBoxError(uiWindow *parent, const char *title, const char *description)
{
	msgbox(windowWindow(parent), title, description, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK);
}
