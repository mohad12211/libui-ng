// 11 june 2015
#include "uipriv_unix.h"

static int created = 0;

struct uiWindow {
	uiUnixControl c;

	GtkWidget *widget;
	GtkContainer *container;
	GtkWindow *window;

	GtkWidget *vboxWidget;
	GtkContainer *vboxContainer;
	GtkBox *vbox;

	GtkWidget *childHolderWidget;
	GtkContainer *childHolderContainer;

	GtkWidget *menubar;

	uiControl *child;
	int margined;
	int resizeable;
	int focused;

	int (*onClosing)(uiWindow *, void *);
	void *onClosingData;
	void (*onContentSizeChanged)(uiWindow *, void *);
	void *onContentSizeChangedData;
	void (*onFocusChanged)(uiWindow *, void *);
	void *onFocusChangedData;
	gboolean fullscreen;
};

static gboolean onClosing(GtkWidget *win, GdkEvent *e, gpointer data)
{
	uiWindow *w = uiWindow(data);

	// manually destroy the window ourselves; don't let the delete-event handler do it
	if ((*(w->onClosing))(w, w->onClosingData))
		uiControlDestroy(uiControl(w));
	// don't continue to the default delete-event handler; we destroyed the window by now
	return TRUE;
}

static void onSizeAllocate(GtkWidget *widget, GdkRectangle *allocation, gpointer data)
{
	uiWindow *w = uiWindow(data);

	// TODO deal with spurious size-allocates
	(*(w->onContentSizeChanged))(w, w->onContentSizeChangedData);
}

static gboolean onGetFocus(GtkWidget *win, GdkEvent *e, gpointer data)
{
	uiWindow *w = uiWindow(data);
	w->focused = 1;
	w->onFocusChanged(w, w->onFocusChangedData);
	return FALSE;
}

static gboolean onLoseFocus(GtkWidget *win, GdkEvent *e, gpointer data)
{
	uiWindow *w = uiWindow(data);
	w->focused = 0;
	w->onFocusChanged(w, w->onFocusChangedData);
	return FALSE;
}

static int defaultOnClosing(uiWindow *w, void *data)
{
	return 0;
}

static void defaultOnPositionContentSizeChanged(uiWindow *w, void *data)
{
	// do nothing
}

static void defaultOnFocusChanged(uiWindow *w, void *data)
{
	// do nothing
}

static void uiWindowDestroy(uiControl *c)
{
	uiWindow *w = uiWindow(c);

	// first hide ourselves
	gtk_widget_hide(w->widget);
	// now destroy the child
	if (w->child != NULL) {
		uiControlSetParent(w->child, NULL);
		uiUnixControlSetContainer(uiUnixControl(w->child), w->childHolderContainer, TRUE);
		uiControlDestroy(w->child);
	}
	// now destroy the menus, if any
	if (w->menubar != NULL)
		uiprivFreeMenubar(w->menubar);
	gtk_widget_destroy(w->childHolderWidget);
	gtk_widget_destroy(w->vboxWidget);
	// and finally free ourselves
	// use gtk_widget_destroy() instead of g_object_unref() because GTK+ has internal references (see #165)
	gtk_widget_destroy(w->widget);
	uiFreeControl(uiControl(w));
}

uiUnixControlDefaultHandle(uiWindow)

uiControl *uiWindowParent(uiControl *c)
{
	return NULL;
}

void uiWindowSetParent(uiControl *c, uiControl *parent)
{
	uiUserBugCannotSetParentOnToplevel("uiWindow");
}

static int uiWindowToplevel(uiControl *c)
{
	return 1;
}

uiUnixControlDefaultVisible(uiWindow)

static void uiWindowShow(uiControl *c)
{
	uiWindow *w = uiWindow(c);

	// don't use gtk_widget_show_all() as that will show all children, regardless of user settings
	// don't use gtk_widget_show(); that doesn't bring to front or give keyboard focus
	// (gtk_window_present() does call gtk_widget_show() though)
	gtk_window_present(w->window);
}

uiUnixControlDefaultHide(uiWindow)
uiUnixControlDefaultEnabled(uiWindow)
uiUnixControlDefaultEnable(uiWindow)
uiUnixControlDefaultDisable(uiWindow)
// TODO?
uiUnixControlDefaultSetContainer(uiWindow)

char *uiWindowTitle(uiWindow *w)
{
	return uiUnixStrdupText(gtk_window_get_title(w->window));
}

void uiWindowSetTitle(uiWindow *w, const char *title)
{
	gtk_window_set_title(w->window, title);
}

void uiWindowContentSize(uiWindow *w, int *width, int *height)
{
	GtkAllocation allocation;

	gtk_widget_get_allocation(w->childHolderWidget, &allocation);
	*width = allocation.width;
	*height = allocation.height;
}

void uiWindowSetContentSize(uiWindow *w, int width, int height)
{
	GtkAllocation childAlloc;
	gint winWidth, winHeight;

	// we need to resize the child holder widget to the given size
	// we can't resize that without running the event loop
	// but we can do gtk_window_set_size()
	// so how do we deal with the differences in sizes?
	// simple arithmetic, of course!

	// from what I can tell, the return from gtk_widget_get_allocation(w->window) and gtk_window_get_size(w->window) will be the same
	// this is not affected by Wayland and not affected by GTK+ builtin CSD
	// so we can safely juse use them to get the real window size!
	// since we're using gtk_window_resize(), use the latter
	gtk_window_get_size(w->window, &winWidth, &winHeight);

	// now get the child holder widget's current allocation
	gtk_widget_get_allocation(w->childHolderWidget, &childAlloc);
	// and punch that out of the window size
	winWidth -= childAlloc.width;
	winHeight -= childAlloc.height;

	// now we just need to add the new size back in
	winWidth += width;
	winHeight += height;
	// and set it
	// this will not move the window in my tests, so we're good
	gtk_window_resize(w->window, winWidth, winHeight);
}

int uiWindowFullscreen(uiWindow *w)
{
	return w->fullscreen;
}

// TODO use window-state-event to track
// TODO does this send an extra size changed?
// TODO what behavior do we want?
void uiWindowSetFullscreen(uiWindow *w, int fullscreen)
{
	w->fullscreen = fullscreen;
	if (w->fullscreen)
		gtk_window_fullscreen(w->window);
	else
		gtk_window_unfullscreen(w->window);
}

void uiWindowOnContentSizeChanged(uiWindow *w, void (*f)(uiWindow *, void *), void *data)
{
	w->onContentSizeChanged = f;
	w->onContentSizeChangedData = data;
}

void uiWindowOnClosing(uiWindow *w, int (*f)(uiWindow *, void *), void *data)
{
	w->onClosing = f;
	w->onClosingData = data;
}

int uiWindowFocused(uiWindow *w)
{
	return w->focused;
}

void uiWindowOnFocusChanged(uiWindow *w, void (*f)(uiWindow *, void *), void *data)
{
	w->onFocusChanged = f;
	w->onFocusChangedData = data;
}

int uiWindowBorderless(uiWindow *w)
{
	return gtk_window_get_decorated(w->window) == FALSE;
}

void uiWindowSetBorderless(uiWindow *w, int borderless)
{
	gtk_window_set_decorated(w->window, borderless == 0);
}

// TODO save and restore expands and aligns
void uiWindowSetChild(uiWindow *w, uiControl *child)
{
	if (w->child != NULL) {
		uiControlSetParent(w->child, NULL);
		uiUnixControlSetContainer(uiUnixControl(w->child), w->childHolderContainer, TRUE);
	}
	w->child = child;
	if (w->child != NULL) {
		uiControlSetParent(w->child, uiControl(w));
		uiUnixControlSetContainer(uiUnixControl(w->child), w->childHolderContainer, FALSE);
	}
}

int uiWindowMargined(uiWindow *w)
{
	return w->margined;
}

void uiWindowSetMargined(uiWindow *w, int margined)
{
	w->margined = margined;
	uiprivSetMargined(w->childHolderContainer, w->margined);
}

int uiWindowResizeable(uiWindow *w)
{
	return w->resizeable;
}

void uiWindowSetResizeable(uiWindow *w, int resizeable)
{
	// workaround for https://gitlab.gnome.org/GNOME/gtk/-/issues/4945
	// calling gtk_window_set_resizable(w->window, 0) will cause the window to resize to default size
	// (default is smallest size here because we're using gtk_window_resize() when creating the window)
	// to prevent this we call gtk_window_set_default_size() on the current window size so that it doesn't resize
	if (resizeable == 0) {
		gint width, height;
		gtk_window_get_size(w->window, &width, &height);
		gtk_window_set_default_size(w->window, width, height);
	}

	w->resizeable = resizeable;
	gtk_window_set_resizable(w->window, resizeable);
}

uiWindow *uiNewWindow(const char *title, int width, int height, int hasMenubar)
{
	uiWindow *w;

	uiUnixNewControl(uiWindow, w);

	w->resizeable = TRUE;
	w->widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	w->container = GTK_CONTAINER(w->widget);
	w->window = GTK_WINDOW(w->widget);

	gtk_window_set_title(w->window, title);
	gtk_window_resize(w->window, width, height);
	if (created == 0) {
		created =  1;
	} else {
		gtk_window_set_modal(w->window, 1);
	}

	w->vboxWidget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	w->vboxContainer = GTK_CONTAINER(w->vboxWidget);
	w->vbox = GTK_BOX(w->vboxWidget);

	// set the vbox as the GtkWindow child
	gtk_container_add(w->container, w->vboxWidget);

	if (hasMenubar) {
		w->menubar = uiprivMakeMenubar(uiWindow(w));
		gtk_container_add(w->vboxContainer, w->menubar);
	}

	w->childHolderWidget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	w->childHolderContainer = GTK_CONTAINER(w->childHolderWidget);
	gtk_widget_set_hexpand(w->childHolderWidget, TRUE);
	gtk_widget_set_halign(w->childHolderWidget, GTK_ALIGN_FILL);
	gtk_widget_set_vexpand(w->childHolderWidget, TRUE);
	gtk_widget_set_valign(w->childHolderWidget, GTK_ALIGN_FILL);
	gtk_box_set_homogeneous(GTK_BOX(w->childHolderWidget), TRUE);
	gtk_container_add(w->vboxContainer, w->childHolderWidget);

	// show everything in the vbox, but not the GtkWindow itself
	gtk_widget_show_all(w->vboxWidget);

	// and connect our events
	g_signal_connect(w->widget, "delete-event", G_CALLBACK(onClosing), w);
	g_signal_connect(w->childHolderWidget, "size-allocate", G_CALLBACK(onSizeAllocate), w);
	g_signal_connect(w->widget, "focus-in-event", G_CALLBACK(onGetFocus), w);
	g_signal_connect(w->widget, "focus-out-event", G_CALLBACK(onLoseFocus), w);

	uiWindowOnClosing(w, defaultOnClosing, NULL);
	uiWindowOnContentSizeChanged(w, defaultOnPositionContentSizeChanged, NULL);

	uiWindowOnFocusChanged(w, defaultOnFocusChanged, NULL);

	// normally it's SetParent() that does this, but we can't call SetParent() on a uiWindow
	// TODO we really need to clean this up, especially since see uiWindowDestroy() above
	g_object_ref(w->widget);

	return w;
}
