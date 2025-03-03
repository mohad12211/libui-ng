// 11 june 2015
#include "uipriv_unix.h"

// on GTK+ a uiRadioButtons is a GtkBox with each of the GtkRadioButtons as children

struct uiRadioButtons {
	uiUnixControl c;
	GtkWidget *widget;
	GtkContainer *container;
	GtkBox *box;
	GPtrArray *buttons;
	void (*onSelected)(uiRadioButtons *, void *);
	void *onSelectedData;
	gboolean changing;
};

uiUnixControlAllDefaultsExceptDestroy(uiRadioButtons)

static void defaultOnSelected(uiRadioButtons *r, void *data)
{
	// do nothing
}

static void onToggled(GtkToggleButton *tb, gpointer data)
{
	uiRadioButtons *r = uiRadioButtons(data);

	// only care if a button is selected
	if (!gtk_toggle_button_get_active(tb))
		return;
	// ignore programmatic changes
	if (r->changing)
		return;
	(*(r->onSelected))(r, r->onSelectedData);
}

static void uiRadioButtonsDestroy(uiControl *c)
{
	uiRadioButtons *r = uiRadioButtons(c);
	GtkWidget *b;

	while (r->buttons->len != 0) {
		b = GTK_WIDGET(g_ptr_array_remove_index(r->buttons, 0));
		gtk_widget_destroy(b);
	}
	g_ptr_array_free(r->buttons, TRUE);
	// and free ourselves
	g_object_unref(r->widget);
	uiFreeControl(uiControl(r));
}

void uiRadioButtonsAppend(uiRadioButtons *r, const char *text)
{
	GtkWidget *rb;
	GtkRadioButton *previous;

	previous = GTK_RADIO_BUTTON(g_ptr_array_index(r->buttons, 0));
	rb = gtk_radio_button_new_with_label_from_widget(previous, text);
	g_signal_connect(rb, "toggled", G_CALLBACK(onToggled), r);
	gtk_container_add(r->container, rb);
	g_ptr_array_add(r->buttons, rb);
	gtk_widget_show(rb);
}

int uiRadioButtonsSelected(uiRadioButtons *r)
{
	GtkToggleButton *tb;
	guint i;

	for (i = 0; i < r->buttons->len; i++) {
		tb = GTK_TOGGLE_BUTTON(g_ptr_array_index(r->buttons, i));
		if (gtk_toggle_button_get_active(tb))
			return i - 1;
	}
	return -1;
}

void uiRadioButtonsSetSelected(uiRadioButtons *r, int n)
{
	GtkToggleButton *tb = GTK_TOGGLE_BUTTON(g_ptr_array_index(r->buttons, n + 1));
	// this is easier than remembering all the signals
	r->changing = TRUE;
	gtk_toggle_button_set_active(tb, TRUE);
	r->changing = FALSE;
}

void uiRadioButtonsOnSelected(uiRadioButtons *r, void (*f)(uiRadioButtons *, void *), void *data)
{
	r->onSelected = f;
	r->onSelectedData = data;
}

uiRadioButtons *uiNewRadioButtons(int orientation)
{
	uiRadioButtons *r;
	// Add a hidden button to indicate an empty selection (-1) as GTK does not support this natively
	GtkWidget *hiddenBtn;

	uiUnixNewControl(uiRadioButtons, r);

	r->widget = gtk_box_new(orientation, 0);
	r->container = GTK_CONTAINER(r->widget);
	r->box = GTK_BOX(r->widget);

	r->buttons = g_ptr_array_new();

	hiddenBtn = gtk_radio_button_new(NULL);
	gtk_container_add(r->container, hiddenBtn);
	g_ptr_array_add(r->buttons, hiddenBtn);
	gtk_widget_hide(hiddenBtn);

	uiRadioButtonsOnSelected(r, defaultOnSelected, NULL);

	return r;
}
