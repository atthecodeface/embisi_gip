#ifndef WAVE_GTK12COMPAT_H
#define WAVE_GTK12COMPAT_H

#if WAVE_USE_GTK2

#define WAVE_GTKIFE(a,b,c,d,e) a,b,c,d,e,NULL
#define WAVE_GDK_GET_POINTER(a,b,c,bi,ci,d)  gdk_window_get_pointer(a,bi,ci,d)
#define WAVE_GDK_GET_POINTER_COPY x=xi; y=yi;
#define WAVE_GTK_SIGFONT gdk_font_load("-*-courier-*-r-*-*-14-*-*-*-*-*-*-*"); \
	if(!signalfont) { fprintf(stderr, "Could not load signalfont courier 14, exiting!\n"); exit(255); }
#define WAVE_GTK_WAVEFONT gdk_font_load("-*-courier-*-r-*-*-10-*-*-*-*-*-*-*"); \
	if(!wavefont) { fprintf(stderr, "Could not load wavefont courier 10, exiting!\n"); exit(255); }
#define WAVE_GTK_SFUNCAST(x) ((void (*)(GtkWidget *, gpointer))(x))

#else

#define WAVE_GTKIFE(a,b,c,d,e) a,b,c,d,e
#define WAVE_GDK_GET_POINTER(a,b,c,bi,ci,d)  gdk_input_window_get_pointer(a, event->deviceid, b, c, NULL, NULL, NULL, d)
#define WAVE_GDK_GET_POINTER_COPY
#define WAVE_GTK_SIGFONT wavearea->style->font
#define WAVE_GTK_WAVEFONT wavearea->style->font
#define WAVE_GTK_SFUNCAST(x) ((GtkSignalFunc)(x))

#endif

#endif

