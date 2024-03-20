/* GStreamer command line playback testing utility
 *
 * Copyright (C) 2013 Tim-Philipp Müller <tim centricular net>
 * Copyright (C) 2013 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/*//for suzuki
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
*/

#include <gst/gst.h>
#include <gst/tag/tag.h>
/*//for suzuki
#include <gst/gst-i18n-app.h>
*/
#include <gst/pbutils/pbutils.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <termios.h>

GST_DEBUG_CATEGORY (play_debug);
#define GST_CAT_DEFAULT play_debug

//#define TEST_SEEKZERO		//https://tims.telechips.com:8443/browse/IS004A-7420
#define SIMPLE_STREAMCHANGE_TEST

GstElement *gPlaybin;

typedef struct
{
    gchar **uris;
    guint num_uris;
    gint cur_idx;

    GstElement *playbin;

    GMainLoop *loop;
    guint bus_watch;
    guint timeout;

    /* missing plugin messages */
    GList *missing;

    gboolean buffering;
    gboolean is_live;

    /* configuration */
    gboolean gapless;

    GstState desired_state;
} GstPlay;

void usage(void);
void _seek(GstElement* playpipe,guint position, gdouble rate);
int fast_seek(GstElement* playpipe,gint interval, gdouble rate);
static gboolean play_bus_msg (GstBus * bus, GstMessage * msg, gpointer data);
static gboolean play_next (GstPlay * play);
static gboolean play_timeout (gpointer user_data);
static void play_about_to_finish (GstElement * playbin, gpointer user_data);
static void keyboard_cb (const gchar * key_input, gpointer user_data);
static void play_reset (GstPlay * play);
int TextTrackChange(GstElement* playpipe, unsigned int track);
int TrackChange(GstElement* playpipe, unsigned int track);

static GstElement *audio_sink_element;
static GstElement *video_sink_element;
static GstElement *text_sink_element;
static gboolean need_seek = 0;
static gboolean f_aging_test = 0;
static gboolean f_unit_test = 0;
static gboolean f_set_play_state = 1;
static gboolean f_set_nullstate = 0;
static gboolean f_get_discoveryinfo = 0;
static gboolean f_trackchange_test = 0;
static gboolean f_repeatplay = 1;
static int f_mute = 0;
static int user_set_seekposition = 136000;
static int set_playtime_for_aging = 5;	//5sec
static int set_playtrack_num = 2;
static int set_texttrack_num = 3;
static int force_seek_flags = 2; //(1:accurate, 2:keyunit, other:SANP)
static int set_fastseek_interval = 3; //3sec

static double set_volume = 1.0;

gdouble rate = 1.0;      /* Current playback rate (can be negative) */
static int f_filesink = 0;

typedef void (*GstPlayKbFunc) (const gchar * kb_input, gpointer user_data);
gboolean gst_play_kb_set_key_handler (GstPlayKbFunc kb_func, gpointer user_data);
static struct termios term_settings;
static gboolean term_settings_saved = FALSE;
static GstPlayKbFunc kb_callback;
static gpointer kb_callback_data;
static gulong io_watch_id;

static gboolean
gst_play_kb_io_cb (GIOChannel * ioc, GIOCondition cond, gpointer user_data)
{
  GIOStatus status;

  if (cond & G_IO_IN) {
    gchar buf[16] = { 0, };
    gsize read;

    status = g_io_channel_read_chars (ioc, buf, sizeof (buf) - 1, &read, NULL);
    if (status == G_IO_STATUS_ERROR)
      return FALSE;
    if (status == G_IO_STATUS_NORMAL) {
      if (kb_callback)
        kb_callback (buf, kb_callback_data);
    }
  }

  return TRUE;                  /* call us again */
}

gboolean
gst_play_kb_set_key_handler (GstPlayKbFunc kb_func, gpointer user_data)
{
  GIOChannel *ioc;

  if (!isatty (STDIN_FILENO)) {
    GST_INFO ("stdin is not connected to a terminal");
    return FALSE;
  }

  if (io_watch_id > 0) {
    g_source_remove (io_watch_id);
    io_watch_id = 0;
  }

  if (kb_func == NULL && term_settings_saved) {
    /* restore terminal settings */
    if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &term_settings) == 0)
      term_settings_saved = FALSE;
    else
      g_warning ("could not restore terminal attributes");

    setvbuf (stdin, NULL, _IOLBF, 0);
  }

  if (kb_func != NULL) {
    struct termios new_settings;

    if (!term_settings_saved) {
      if (tcgetattr (STDIN_FILENO, &term_settings) != 0) {
        g_warning ("could not save terminal attributes");
        return FALSE;
      }
      term_settings_saved = TRUE;

      /* Echo off, canonical mode off, extended input processing off  */
      new_settings = term_settings;
      new_settings.c_lflag &= ~(ECHO | ICANON | IEXTEN);
      new_settings.c_cc[VMIN] = 0;
      new_settings.c_cc[VTIME] = 0;

      if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &new_settings) != 0) {
        g_warning ("Could not set terminal state");
        return FALSE;
      }
      setvbuf (stdin, NULL, _IONBF, 0);
    }
  }

  ioc = g_io_channel_unix_new (STDIN_FILENO);

  io_watch_id = g_io_add_watch_full (ioc, G_PRIORITY_DEFAULT, G_IO_IN,
      (GIOFunc) gst_play_kb_io_cb, user_data, NULL);
  g_io_channel_unref (ioc);

  kb_callback = kb_func;
  kb_callback_data = user_data;

  return TRUE;
}

static void
restore_terminal_test (void)
{
	gst_play_kb_set_key_handler (NULL,NULL);
}

static void
toggle_paused (GstPlay * play)
{
  if (play->desired_state == GST_STATE_PLAYING)
    play->desired_state = GST_STATE_PAUSED;
  else
    play->desired_state = GST_STATE_PLAYING;

  if (!play->buffering) {
    gst_element_set_state (play->playbin, play->desired_state);
  } else if (play->desired_state == GST_STATE_PLAYING) {
    g_print ("\nWill play as soon as buffering finishes)\n");
  }
}

void get_discoverer_info(const char *file_path)
{
	GError *err = NULL;
	GList *tmp_stream, *get_streams;
	GstDiscovererResult result;

	GstDiscovererInfo *discovererinfo = NULL;
	GstDiscoverer *discoverer = NULL;

	gint video_width = 0;
	gint video_height = 0;
	gint audio_channel = 0;
	gint audio_samplerate = 0;
	const gchar *subtitle_language;
	
	if (file_path == NULL)
	{
		return;
	}

	discoverer = gst_discoverer_new(10 * GST_SECOND, &err);

	if (discoverer == NULL)
	{
		printf("[%d] Error : %s\n", __LINE__,err->message);
		g_clear_error (&err);
		return;
	}

	discovererinfo = gst_discoverer_discover_uri(discoverer, file_path, &err);

	if (discovererinfo == NULL)
	{
		printf("[%d] Error : %s\n", __LINE__,err->message);
		g_clear_error (&err);
		return;
	}

	result = gst_discoverer_info_get_result(discovererinfo);

	switch (result)
	{
		case GST_DISCOVERER_OK:
		{
			break;
		}

		case GST_DISCOVERER_URI_INVALID:
		{
			printf("URI is not valid\n");
			return;
		}

		case GST_DISCOVERER_ERROR:
		{
			printf( "An error was encountered while discovering the file\n");
			return;
		}

		case GST_DISCOVERER_TIMEOUT:
		{
			printf("Analyzing URI timed out\n");
			return;
		}

		case GST_DISCOVERER_BUSY:
		{
			printf( "Discoverer was busy\n");
			return;
		}

		case GST_DISCOVERER_MISSING_PLUGINS:
		{
			printf("Missing plugins\n");
			break;
		}
	}

	get_streams = gst_discoverer_info_get_stream_list(discovererinfo);

	for (tmp_stream = get_streams; tmp_stream; tmp_stream = tmp_stream->next)
	{
		GstDiscovererStreamInfo *tmp_stream_info = (GstDiscovererStreamInfo *) tmp_stream->data;

		GstDiscovererAudioInfo *audio_info = NULL;
		GstDiscovererVideoInfo *video_info = NULL;
		GstDiscovererSubtitleInfo *subtitle_info = NULL;

		printf( "nick  : %s \n", gst_discoverer_stream_info_get_stream_type_nick(tmp_stream_info));

		if (GST_IS_DISCOVERER_AUDIO_INFO(tmp_stream_info))
		{
			audio_info = (GstDiscovererAudioInfo *) tmp_stream_info;
			audio_channel = gst_discoverer_audio_info_get_channels(audio_info);
			audio_samplerate = gst_discoverer_audio_info_get_sample_rate(audio_info);
			printf( "AUDIO : channel %d\n",audio_channel);
			printf( "AUDIO : samperate %d\n",audio_samplerate);
		}
		else if (GST_IS_DISCOVERER_VIDEO_INFO(tmp_stream_info))
		{
			video_info = (GstDiscovererVideoInfo *) tmp_stream_info;

			video_width = gst_discoverer_video_info_get_width(video_info);
			video_height = gst_discoverer_video_info_get_height(video_info);
			printf("VIDEO : width(%d) x height(%d)\n",video_width,video_height);
		}
		else if (GST_IS_DISCOVERER_SUBTITLE_INFO(tmp_stream_info))
		{
			subtitle_info = (GstDiscovererSubtitleInfo *) tmp_stream_info;
			subtitle_language = gst_discoverer_subtitle_info_get_language(subtitle_info);

			printf("SUBTITLE : language - %s\n",subtitle_language ? subtitle_language : "<unknown>");
		}
	}


	printf( "====================================================== \n");
	printf( " Media Info. \n");
	printf( "====================================================== \n");
	printf( " Width  : %d \n", video_width);
	printf( " Height  : %d \n", video_height);
	printf( " Audio Ch.  : %d \n", audio_channel);
	printf( " Audio Sf.  : %d \n", audio_samplerate);
	printf( " Subtitle language : %s \n", subtitle_language ? subtitle_language : "<unknown>");
	printf( " Duration  : %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(gst_discoverer_info_get_duration(discovererinfo)));
	printf( "====================================================== \n");

	gst_discoverer_stream_info_list_free(get_streams);
	g_object_unref(discoverer);
	gst_discoverer_info_unref(discovererinfo);

	if (err)
	{
    	g_error_free(err);
	}
}




static GstPlay *
play_new (gchar ** uris, const gchar * audio_sink_name, const gchar * video_sink_name,
          const gchar * text_sink_name, gboolean gapless)
{
    GstPlay *play;

    play = g_new0 (GstPlay, 1);

    play->uris = uris;
    play->num_uris = g_strv_length (uris);
    play->cur_idx = -1;

    gPlaybin = play->playbin = gst_element_factory_make ("playbin", "playbin");

    if (audio_sink_name != NULL)
    {
		g_print ("audio_sink_name is %s\n",audio_sink_name);
        audio_sink_element = gst_element_factory_make (audio_sink_name, NULL);
        if (audio_sink_element != NULL){
            g_object_set (play->playbin, "audio-sink", audio_sink_element, NULL);
			if (!strncmp(audio_sink_name,"filesink",8))
			{
				f_filesink = 1;
				g_print ("audio sink is filesink, output file is /home/root/dump_audio.pcm \n");
				g_object_set (audio_sink_element,"location","/home/root/dump_audio.pcm",NULL);
			}
		}
        else
            g_warning ("Couldn't create specified audio sink '%s'\n", audio_sink_name);
    }
    if (video_sink_name != NULL)
    {
        video_sink_element = gst_element_factory_make (video_sink_name, NULL);
        if (video_sink_element != NULL){
            g_object_set (play->playbin, "video-sink", video_sink_element, NULL);
#if 1
            if (!strncmp(GST_ELEMENT_NAME(video_sink_element),"v4l2sink",8)) {
                char *v4l2_device = getenv("V4L2_DEVICE");
                if (v4l2_device != NULL)
                {
                    g_object_set(video_sink_element, "device", v4l2_device, NULL);
                }
            }
#endif
        }
        else
            g_warning ("Couldn't create specified video sink '%s'\n", video_sink_name);
    }
    if (text_sink_name != NULL)
    {
		g_print ("text_sink_name is %s\n",text_sink_name);
        text_sink_element = gst_element_factory_make (text_sink_name, NULL);
        if (text_sink_element != NULL){
            g_object_set (play->playbin, "text-sink", text_sink_element, NULL);
			if (!strncmp(text_sink_name,"filesink",8))
			{
				g_print ("text sink is filesink, output file is /home/root/dump_subtitle.txt \n");
				g_object_set (text_sink_element,"location","/home/root/dump_subtitle.txt",NULL);
			}
		}
        else
            g_warning ("Couldn't create specified text sink '%s'\n", text_sink_name);
    }

    play->loop = g_main_loop_new (NULL, FALSE);

    play->bus_watch = gst_bus_add_watch (GST_ELEMENT_BUS (play->playbin),
                                         play_bus_msg, play);

    /* FIXME: make configurable incl. 0 for disable */
    play->timeout = g_timeout_add (100, play_timeout, play);

    play->missing = NULL;
    play->buffering = FALSE;
    play->is_live = FALSE;

    play->desired_state = GST_STATE_PLAYING;

    play->gapless = gapless;
    if (gapless) {
      g_signal_connect (play->playbin, "about-to-finish",
        G_CALLBACK (play_about_to_finish), play);
    }

    return play;
}

static void
play_free (GstPlay * play)
{
    play_reset (play);

    gst_element_set_state (play->playbin, GST_STATE_NULL);

    gst_object_unref (play->playbin);

    g_source_remove (play->bus_watch);
    g_source_remove (play->timeout);
    g_main_loop_unref (play->loop);


    g_strfreev (play->uris);
    g_free (play);
}

/* reset for new file/stream */
static void
play_reset (GstPlay * play)
{
    g_list_foreach (play->missing, (GFunc) gst_message_unref, NULL);
    play->missing = NULL;

    play->buffering = FALSE;
    play->is_live = FALSE;
	if (f_unit_test)
		need_seek = 1;
}

/* returns TRUE if something was installed and we should restart playback */
static gboolean
play_install_missing_plugins (GstPlay * play)
{
    /* FIXME: implement: try to install any missing plugins we haven't
     * tried to install before */
    return FALSE;
}
#if 0
void get_samplerate(GstPlay *play)
{
	GstPad *audio_pad = NULL;
	GstCaps *caps = NULL;
	g_signal_emit_by_name(G_OBJECT(play->playbin),"get-audio-pad",0, &audio_pad);
	if (audio_pad) {
		gint samplerate = 0;
		GstStructure *structure = NULL;
		caps = gst_pad_get_current_caps(audio_pad);
		structure = gst_caps_get_structure(caps, 0);
		gst_structure_get_int(structure,"rate",&samplerate);
		printf("%s samplerate = %d\n",__func__,samplerate);
		gst_object_unref(audio_pad);
	} else printf("%s audio_pad is NULL\n",__func__);
}
#endif
static gboolean
play_bus_msg (GstBus * bus, GstMessage * msg, gpointer user_data)
{
    GstPlay *play = user_data;

    switch (GST_MESSAGE_TYPE (msg))
    {
    case GST_MESSAGE_ASYNC_DONE:
        g_print ("Prerolled.\r");
        if (play->missing != NULL && play_install_missing_plugins (play))
        {
            g_print ("New plugins installed, trying again...\n");
            --play->cur_idx;
            play_next (play);
        }
        g_print("video_sink is %s\n",GST_ELEMENT_NAME(video_sink_element));
#if 0
	    if (!strncmp(GST_ELEMENT_NAME(video_sink_element),"waylandsink",8)){
    	  g_print("video_sink is waylandsink\n");
//	      g_object_set(G_OBJECT( video_sink ),"fullscreen", 1, NULL);
    	}
		else if (!strncmp(GST_ELEMENT_NAME(video_sink_element),"v4l2sink",8)) {
    	  g_print("video_sink is v4l2sink, set overlay width: 1280, height: 720\n");
		  g_object_set(G_OBJECT( video_sink_element ),"overlay-set-width", 1280, NULL );
		  g_object_set(G_OBJECT( video_sink_element ),"overlay-set-height", 720, NULL );
		  g_object_set(G_OBJECT( video_sink_element ),"overlay-set-update", 1, NULL );
        }
#endif
        break;
    case GST_MESSAGE_BUFFERING:
    {
        gint percent;

		if (f_set_play_state){

        if (!play->buffering)
            g_print ("\n");

        gst_message_parse_buffering (msg, &percent);
        g_print ("%s %d%%  \r", "Buffering...", percent);

        /* no state management needed for live pipelines */
        if (play->is_live)
            break;

        if (percent == 100)
        {
            /* a 100% message means buffering is done */
            if (play->buffering)
            {
                play->buffering = FALSE;
                gst_element_set_state (play->playbin, GST_STATE_PLAYING);
            }
        }
        else
        {
            /* buffering... */
            if (!play->buffering)
            {
                gst_element_set_state (play->playbin, GST_STATE_PAUSED);
                play->buffering = TRUE;
            }
        }

		}

        break;
    }
    case GST_MESSAGE_LATENCY:
        g_print ("Redistribute latency...\n");
        gst_bin_recalculate_latency (GST_BIN (play->playbin));
        break;
    case GST_MESSAGE_REQUEST_STATE:
    {
        GstState state;
        gchar *name;

        name = gst_object_get_path_string (GST_MESSAGE_SRC (msg));

        gst_message_parse_request_state (msg, &state);

        g_print ("Setting state to %s as requested by %s...\n",
                 gst_element_state_get_name (state), name);

        gst_element_set_state (play->playbin, state);
        g_free (name);
        break;
    }
    case GST_MESSAGE_EOS:
        /* print final position at end */
        play_timeout (play);
        g_print ("\n");
        /* and switch to next item in list */
        if (!play_next (play))
        {
            g_print ("Reached end of play list.\n");
            g_main_loop_quit (play->loop);
        }
        break;
    case GST_MESSAGE_WARNING:
    {
        GError *err;
        gchar *dbg = NULL;

        gst_message_parse_warning (msg, &err, &dbg);
        g_printerr ("WARNING %s\n", err->message);
        if (dbg != NULL)
            g_printerr ("WARNING debug information: %s\n", dbg);
        g_error_free (err);
        g_free (dbg);
        break;
    }
    case GST_MESSAGE_ERROR:
    {
        GError *err;
        gchar *dbg;

        gst_message_parse_error (msg, &err, &dbg);
        g_printerr ("ERROR %s for %s\n", err->message, play->uris[play->cur_idx]);
        if (dbg != NULL)
            g_printerr ("ERROR debug information: %s\n", dbg);
        g_error_free (err);
        g_free (dbg);

        /* flush any other error messages from the bus and clean up */
        gst_element_set_state (play->playbin, GST_STATE_NULL);
		
#ifndef AGING_TEST
		if (!play_next(play))
		{
			g_print("Reached end of play list.\n");
			g_main_loop_quit(play->loop);
		}
#endif
        break;
    }
	case GST_MESSAGE_STATE_CHANGED:
	{
	    GstState old_state, new_state;
        gst_message_parse_state_changed (msg, &old_state, &new_state, NULL);
		if( strncmp(GST_OBJECT_NAME(msg->src),"playbin",7) == 0 )
			g_print ("Element %s changed state from %s to %s.\n",GST_OBJECT_NAME (msg->src),gst_element_state_get_name (old_state),gst_element_state_get_name (new_state));
        break;	
	}
    default:
        /*//for suzuki
             if (gst_is_missing_plugin_message (msg)) {
               gchar *desc;

               desc = gst_missing_plugin_message_get_description (msg);
               g_print ("Missing plugin: %s\n", desc);
               g_free (desc);
               play->missing = g_list_append (play->missing, gst_message_ref (msg));
             }
        */
        break;
    }

    return TRUE;
}

static gboolean
play_timeout (gpointer user_data)
{
    GstPlay *play = user_data;
    gint64 pos = -1, dur = -1;

    if (play->buffering)
        return TRUE;

    gst_element_query_position (play->playbin, GST_FORMAT_TIME, &pos);
    gst_element_query_duration (play->playbin, GST_FORMAT_TIME, &dur);

    if (pos >= 0 && dur > 0)
    {
		if (f_unit_test && need_seek) {
		  // media duration = over 2 min. -> playback 2 min.
		  if (dur > 120 * GST_SECOND && pos >= 120 * GST_SECOND)
		  {
			// seek to end - 10sec.
			_seek(play->playbin, (dur/GST_SECOND - 10)*GST_USECOND, rate);
			need_seek = 0;
		  }
		  // media duration = 50 sec. ~ 2 min. -> playback 50 sec.
		  else if (((dur <= 120 * GST_SECOND) && (dur > 50 * GST_SECOND)) && pos >= 50 * GST_SECOND)
		  {
			// seek to end - 10sec.
			_seek(play->playbin, (dur/GST_SECOND - 10)*GST_USECOND, rate);
			need_seek = 0;
		  }
		  // media duration = 30 sec. ~ 50 sec -> playback 30sec
		  else if (((dur <= 50 * GST_SECOND) && (dur > 30 * GST_SECOND)) && pos >= 30 * GST_SECOND)
		  {
			// seek to end - 10sec.
			_seek(play->playbin, (dur/GST_SECOND - 10)*GST_USECOND, rate);
			need_seek = 0;
		  }
		}

        gchar dstr[32], pstr[32];

        /* FIXME: pretty print in nicer format */
        g_snprintf (pstr, 32, "%" GST_TIME_FORMAT, GST_TIME_ARGS (pos));
        pstr[9] = '\0';
        g_snprintf (dstr, 32, "%" GST_TIME_FORMAT, GST_TIME_ARGS (dur));
        dstr[9] = '\0';
        //for suzuki
		g_print ("%s / %s\r", pstr, dstr);

		if ( f_aging_test && ((dur >= set_playtime_for_aging*GST_SECOND) && (pos >= set_playtime_for_aging*GST_SECOND)))
		{
    	    gst_element_set_state (play->playbin, GST_STATE_READY);

			if (!play_next(play))
			{
				g_print("Reached end of play list.\n");
				g_main_loop_quit(play->loop);
			}
		}
#ifdef TEST_SEEKZERO
		if ((pos >= 5 * GST_SECOND) || f_filesink){
			_seek(play->playbin, 0, rate);
		}
#endif
    }

    return TRUE;
}

static gchar *
play_uri_get_display_name (GstPlay * play, const gchar * uri)
{
    gchar *loc;

    if (gst_uri_has_protocol (uri, "file"))
    {
        loc = g_filename_from_uri (uri, NULL, NULL);
    }
    else if (gst_uri_has_protocol (uri, "pushfile"))
    {
        loc = g_filename_from_uri (uri + 4, NULL, NULL);
    }
    else
    {
        loc = g_strdup (uri);
    }

    /* Maybe additionally use glib's filename to display name function */
    return loc;
}

/* returns FALSE if we have reached the end of the playlist */
static gboolean
play_next (GstPlay * play)
{
    GstStateChangeReturn sret;
    const gchar *next_uri;
    gchar *loc;

    if (++play->cur_idx >= play->num_uris)
    {
        if (f_repeatplay) play->cur_idx = 0;
        else
            return FALSE;
	}

    if (f_set_nullstate)
		gst_element_set_state (play->playbin, GST_STATE_NULL);
	else
        gst_element_set_state (play->playbin, GST_STATE_READY);

	g_print ("Set to %s state\n",(f_set_nullstate ? "NULL" : "READY"));
    play_reset (play);

    next_uri = play->uris[play->cur_idx];
    loc = play_uri_get_display_name (play, next_uri);
    g_print ("Now playing %s\n", loc);
    g_free (loc);

    g_object_set (play->playbin, "uri", next_uri, NULL);

    if (f_get_discoveryinfo) {
	    g_print("\n $$$ Start Discoverer...\n");
  	  get_discoverer_info(next_uri);
	    g_print("\n $$$ End Discoverer...\n");
	  }

    if (f_set_play_state){
		sret = gst_element_set_state (play->playbin, GST_STATE_PLAYING);
		g_print("set Playing, ret %d\n",sret);
	}
	else{
        sret = gst_element_set_state (play->playbin, GST_STATE_PAUSED);
		g_print("set Paused, ret %d\n",sret);
	}
//    gst_element_set_state (play->playbin, GST_STATE_PAUSED);
//	GstState cur_state;
//	gst_element_get_state (play->playbin, &cur_state, NULL, GST_CLOCK_TIME_NONE);
//	sret = gst_element_set_state (play->playbin, GST_STATE_PLAYING);

	if (f_set_play_state) {
    switch (sret)
    {
    case GST_STATE_CHANGE_FAILURE:
        /* ignore, we should get an error message posted on the bus */
        break;
    case GST_STATE_CHANGE_NO_PREROLL:
        g_print ("Pipeline is live.\n");
        play->is_live = TRUE;
        break;
    case GST_STATE_CHANGE_ASYNC:
        g_print ("Prerolling...\r");
        break;
    default:
        break;
    }
	}

    return TRUE;
}

static void
play_about_to_finish (GstElement * playbin, gpointer user_data)
{
  GstPlay *play = user_data;
  const gchar *next_uri;
  gchar *loc;
  guint next_idx;

  if (!play->gapless)
    return;

  next_idx = play->cur_idx + 1;
  if (next_idx >= play->num_uris)
    return;

  next_uri = play->uris[next_idx];
  loc = play_uri_get_display_name (play, next_uri);
  g_print ("About to finish, preparing next title: %s\n", loc);
  g_free (loc);

  g_object_set (play->playbin, "uri", next_uri, NULL);
  play->cur_idx = next_idx;
}



static void
do_play (GstPlay * play)
{
    gint i;

    /* dump playlist */
    for (i = 0; i < play->num_uris; ++i)
        GST_INFO ("%4u : %s", i, play->uris[i]);

    if (!play_next (play))
        return;

    g_main_loop_run (play->loop);
}

static void
add_to_playlist (GPtrArray * playlist, const gchar * filename)
{
    GDir *dir;
    gchar *uri;

    if (gst_uri_is_valid (filename))
    {
        g_ptr_array_add (playlist, g_strdup (filename));
        return;
    }

    if ((dir = g_dir_open (filename, 0, NULL)))
    {
        const gchar *entry;

        /* FIXME: sort entries for each directory? */
        while ((entry = g_dir_read_name (dir)))
        {
            gchar *path;

            path = g_strconcat (filename, G_DIR_SEPARATOR_S, entry, NULL);
            add_to_playlist (playlist, path);
            g_free (path);
        }

        g_dir_close (dir);
        return;
    }

    uri = gst_filename_to_uri (filename, NULL);
    if (uri != NULL)
        g_ptr_array_add (playlist, uri);
    else
        g_warning ("Could not make URI out of filename '%s'", filename);
}


int TextTrackChange(GstElement* playpipe, unsigned int track)
{
	if (playpipe != NULL)
	{
		int currTrack = 0;
		int totalTrackcount;

		g_object_get(playpipe, "current-text", &currTrack, NULL);
		g_object_get(playpipe, "n-text", &totalTrackcount, NULL);

		if ((currTrack != track) && (totalTrackcount > track))
		{
			GstState state;
			gint64 pos;

			if (gst_element_get_state(playpipe, &state, NULL, GST_CLOCK_TIME_NONE) == 0)
			{
				g_print("Get state failed");
				return -1;
			}

			if (gst_element_query_position(video_sink_element, GST_FORMAT_TIME, &pos) == FALSE)
			{
				g_print("Get current player position failed");
				return -1;
			}

			g_print("Track Change %d -> %d (total: %d)\n", currTrack, track, totalTrackcount);

      //seek before track change when the state is paused
			if (state == GST_STATE_PAUSED)
			{
				g_print("seek to %lld, current state = %d\n",pos, state);

			  if (!gst_element_seek_simple(playpipe, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE), pos))
				  g_print("[%s][%d]gst_element_seek_simple error\n", __func__, __LINE__);
			}
			g_object_set(playpipe, "current-text", track, NULL);   // track change

			g_print("[%s:%d] force seek position %lld start\n",__func__,__LINE__,pos);
			// force seek for resolve mute bug when set "current-audio" property to playbin
			if ( !gst_element_seek_simple(playpipe, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE), pos)) // Requirement1. force seek for resolve mute bug when set "current-audio" property to playbin
				g_print("[%s][%d]gst_element_seek_simple error\n", __func__, __LINE__);

			g_print("Track[%d] Change Complete\n", track);
		}
		else {
			g_print("[TEXT] target track: %d, current track: %d, total track: %d\n", track, currTrack, totalTrackcount);
			g_print("Current Track is Tarket Track. No Change.\n");
		}
		return 0;
	}
	else
	{
		g_print("[%s:%d] Err! pipeline is NULL\n", __func__, __LINE__);
		return -1;
	}
}


int TrackChange(GstElement* playpipe, unsigned int track)
{
	if (playpipe != NULL)
	{
		int currTrack = 0;
		int totalTrack;

		g_object_get(playpipe, "current-audio", &currTrack, NULL);
		g_object_get(playpipe, "n-audio", &totalTrack, NULL);

#ifdef SIMPLE_STREAMCHANGE_TEST
		track = currTrack + 1;
		if (track >= totalTrack)
		{
			track = 0;
		}
#endif
		if ((currTrack != track) && (totalTrack > track))
		{
			GstState state;
			gint64 pos;

			if (gst_element_get_state(playpipe, &state, NULL, GST_CLOCK_TIME_NONE) == 0)
			{
				g_print("Get state failed");
				return -1;
			}

			if (gst_element_query_position(video_sink_element, GST_FORMAT_TIME, &pos) == FALSE)
			{
				g_print("Get current player position failed");
				return -1;
			}

			g_print("Track Change %d -> %d (total: %d)\n", currTrack, track, totalTrack);

#if 1
      //seek before track change when the state is paused
			if (state == GST_STATE_PAUSED)  //Requirement 2.
			{
				g_print("seek to %lld, current state = %d\n",pos, state);

			  if (!gst_element_seek_simple(playpipe, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE), pos))
				  g_print("[%s][%d]gst_element_seek_simple error\n", __func__, __LINE__);
			}
#endif
			g_object_set(playpipe, "current-audio", track, NULL);   // track change

#if 1 //Requirement 1.
			if (state == GST_STATE_PAUSED)
			{
			g_print("[%s:%d] force seek position %lld start\n",__func__,__LINE__,pos);
			// force seek for resolve mute bug when set "current-audio" property to playbin
			if ( !gst_element_seek_simple(playpipe, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE), pos)) // Requirement1. force seek for resolve mute bug when set "current-audio" property to playbin
				g_print("[%s][%d]gst_element_seek_simple error\n", __func__, __LINE__);
			}
#endif
			g_print("Track[%d] Change Complete\n", track);

			/*
			//g_printf("[%s:%d] force seek position %lld start\n",__func__,__LINE__,pos);
			// force seek for resolve mute bug when set "current-audio" property to playbin
			if (gst_element_seek_simple(playpipe, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE), pos))
				g_print("[%s][%d]gst_element_seek_simple error\n", __func__, __LINE__);
			//g_printf("[%s:%d] force seek position %lld done\n", __func__, __LINE__, pos);*/
		}
		else {
			g_print("[AUDIO] target track: %d, current track: %d, total track: %d\n", track, currTrack, totalTrack);
			g_print("Current Track is Tarket Track. No Change.\n");
		}
		return 0;
	}
	else
	{
		g_print("[%s:%d] Err! pipeline is NULL\n", __func__, __LINE__);
		return -1;
	}
}

void _seek(GstElement* playpipe,guint position, gdouble rate)
{
	GstEvent *_seek_event = NULL;
	guint64 _seekpos,target_position;
	GstSeekFlags _seek_flags;

	gboolean _ret;

	_seekpos = ( guint64 )position * GST_MSECOND;

	printf( "Seek to %d ms\n",position);

    /* Obtain the current position, needed for the seek event */
    if (!gst_element_query_position (playpipe, GST_FORMAT_TIME, &target_position)) {
      printf ("Unable to retrieve current position.\n");
      return;
    }
    if (rate != 1.0 && !position)
      _seekpos = target_position;

	if (force_seek_flags == 1)
		_seek_flags = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE;
	else if (force_seek_flags == 2)
		_seek_flags = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT;
	else {
		if (_seekpos > target_position)
			_seek_flags = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SNAP_AFTER;
		else
			_seek_flags = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SNAP_BEFORE;
	}


//	printf("gst_event_new_seek start\n");
  /* Create the seek event */
  if (rate > 0) {
    _seek_event = gst_event_new_seek (rate, GST_FORMAT_TIME, _seek_flags,
        GST_SEEK_TYPE_SET, _seekpos, GST_SEEK_TYPE_NONE, 0);
  }
  else {
    _seek_event = gst_event_new_seek (rate, GST_FORMAT_TIME, _seek_flags,
        GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, position);
  }
//  printf("gst_event_new_seek end\n");

	
	if( gst_element_send_event( playpipe, _seek_event ) == FALSE )
	{
		printf( " Seek Failer: send seek event failed!\n" );
		return;
	}
	else
	{
	    printf( "Seek Success (seek_flags = 0x%x\n",_seek_flags );
	}
 
	return;
}      /* -----  end of static function _seek  ----- */

int fast_seek(GstElement* playpipe,gint interval, gdouble rate)
{
	GstEvent *_seek_event = NULL;
	gint64 _seekpos,target_position,dur;
	GstSeekFlags _seek_flags;
	gboolean _ret;

  gst_element_query_duration (playpipe, GST_FORMAT_TIME, &dur);

  /* Obtain the current position, needed for the seek event */
  if (!gst_element_query_position (playpipe, GST_FORMAT_TIME, &target_position)) {
    printf ("Unable to retrieve current position.\n");
    return -1;
  }

  _seekpos = target_position + interval*GST_SECOND;

  if (_seekpos < 0)
	  _seekpos = 0;

  if (_seekpos > dur)
  	return -1;

	if (_seekpos > target_position)
		_seek_flags = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_SNAP_AFTER;
	else
		_seek_flags = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_SNAP_BEFORE;

	printf("current pos[%lld] tarket pos[%lld] (dur:%lld) seekflag(%x)\n",(long long int)target_position, (long long int)_seekpos, (long long int)dur, _seek_flags );
//	printf("gst_event_new_seek start\n");
  /* Create the seek event */
  if (rate > 0) {
    _seek_event = gst_event_new_seek (rate, GST_FORMAT_TIME, _seek_flags,
        GST_SEEK_TYPE_SET, _seekpos, GST_SEEK_TYPE_NONE, 0);
  }
  else {
    _seek_event = gst_event_new_seek (rate, GST_FORMAT_TIME, _seek_flags,
        GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, _seekpos);
  }
//  printf("gst_event_new_seek end\n");

	
	if( gst_element_send_event( playpipe, _seek_event ) == FALSE )
	{
		printf( " Seek Failer: send seek event failed!\n" );
		return -1;
	}
	else
	{
	    printf( "Seek Success\n" );	
	}
 
	return 0;
}      /* -----  end of static function _seek  ----- */


/* Process keyboard input */
static void keyboard_cb (const gchar * key_input, gpointer user_data)
{
	int inputnum;
	char inputchar[8];
	int temp = 0;

	GstPlay *play = (GstPlay *) user_data;
	gchar key = '\0';

	/* only want to switch/case on single char, not first char of string */
	if (key_input[0] != '\0' && key_input[1] == '\0')
		key = g_ascii_tolower (key_input[0]);

		switch (key)
		{  
			case 'a':
				toggle_paused (play);
				break;
			case 'n':
				if (!play_next(play))
				{
					g_print("Reached end of play list.\n");
					g_main_loop_quit(play->loop);
				}
				break;
			case 'm':
#ifdef SIMPLE_STREAMCHANGE_TEST
				TrackChange(play->playbin,0);
#else
				printf("select which stream will be change (a:audio, t:text)\n");
				scanf("%s",inputchar);
				{
					printf("Type target stream number \n");
				    scanf("%d", &inputnum);
					if (inputchar[0] == 'a')
					{
				    	set_playtrack_num = inputnum;
						printf("Play %d track \n",set_playtrack_num);
						TrackChange(play->playbin,set_playtrack_num);
					}
					if (inputchar[0] == 't')
					{
				    	set_texttrack_num = inputnum;
						printf("Play %d track \n",set_texttrack_num);
						TextTrackChange(play->playbin,set_texttrack_num);
					}
				}
#endif
				break;
			case 'b':
			    f_set_nullstate = (f_set_nullstate == 1) ? 0 : 1;
			    printf("%s set null_state\n",(f_set_nullstate ? "Set" : "Unset"));
				break;
			case 'd':
			    f_get_discoveryinfo = (f_get_discoveryinfo == 1) ? 0 : 1;
			    printf("%s get discoveryinfo\n",(f_get_discoveryinfo ? "Set" : "Unset"));
				break;
			case 'r':
			    f_repeatplay = (f_repeatplay == 1) ? 0 : 1;
			    printf("%s repeatplay\n",(f_repeatplay ? "Set" : "Unset"));
				break;
			case 'e':
			    printf("Type target time(ms)\n");
				scanf("%d", &inputnum);
				_seek(play->playbin, inputnum, rate);

			break;
			case 'l':
			{
				int temp = 0;
				printf("FastForward Seek test, interval = %d sec\n",set_fastseek_interval);
			    for (temp = 0; temp < 100; temp++){
					g_usleep(1000*1000);
					if(fast_seek(play->playbin, set_fastseek_interval, rate) == -1){
						printf("file end!\n");
						temp = 100;
					}
				}
			}

			break;
			case 'c':
			    printf("Type constant target time(ms)\n");
				scanf("%d", &user_set_seekposition);
				break;
			case 't':
				printf("seek to %d(ms)\n",user_set_seekposition);
			    _seek(play->playbin,user_set_seekposition, rate);
				break;
			case 's':
			    printf("Type speed rate example 11=1.1\n");
				  scanf("%d", &inputnum);
				  rate = (double)inputnum/10;
		      _seek(play->playbin, 0, rate);
        		break;
			case 'f':
			    _seek(play->playbin,24000, rate);
				break;
			case 'w':
			    _seek(play->playbin,54000, rate);
				break;
			case 'h':
			    usage();
				break;
			case 'j':
				f_set_play_state = (f_set_play_state == 1) ? 0 : 1;
			    printf("use %s state.\n",(f_set_play_state ? "Play" : "Pause"));
				break;
			case 'u':
				f_unit_test = (f_unit_test == 1) ? 0 : 1;
			    printf("%s unit test\n",(f_unit_test ? "Set" : "Unset"));
				break;
			case 'g':
				f_aging_test = (f_aging_test == 1) ? 0 : 1;
			    printf("%s aging test\n",(f_aging_test ? "Set" : "Unset"));
				if (f_aging_test) {
					printf("Type constant target time (s)\n");
 				    scanf("%d", &inputnum);
				    if (inputnum)
					   set_playtime_for_aging = inputnum;
					printf("Play during %d secounds and skip file\n",set_playtime_for_aging);
				}
				break;
			case 'z':
				_seek(play->playbin,0, rate);
				break;
			case '0':
				f_mute = (f_mute == 1) ? 0 : 1;
			    printf("mute %s\n",(f_mute ? "On" : "Off"));
		    	g_object_set(play->playbin,"mute", f_mute,NULL);
				break;
			case 'p':
				scanf("%d", &inputnum);
				force_seek_flags = inputnum;
			    printf("force_seek_flags %s\n",((force_seek_flags == 1) ? "Accurate" : (force_seek_flags == 2) ? "Key-Unit" : "SNAP"));
				break;
			case '1'://standard
				g_object_set(G_OBJECT( video_sink_element ),"aspectratio", 1, NULL );
				break;
			case '2'://full
				g_object_set(G_OBJECT( video_sink_element ),"aspectratio", 2, NULL );
				break;
			case '3'://original
				g_object_set(G_OBJECT( video_sink_element ),"aspectratio", 3, NULL );
				break;
			case '4'://16-9
				g_object_set(G_OBJECT( video_sink_element ),"aspectratio", 4, NULL );
				break;
			case '5'://4-3
				g_object_set(G_OBJECT( video_sink_element ),"aspectratio", 5, NULL );
				break;
			case '6'://zoom
				g_object_set(G_OBJECT( video_sink_element ),"aspectratio", 6, NULL );
				break;
			case '7':
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-top", 0, NULL );
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-left", 0, NULL );
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-height", 111, NULL );
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-width", 111, NULL );
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-update", 1, NULL );
				break;
			case '8':
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-top", 561, NULL );
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-left", 123, NULL );
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-height", 111, NULL );
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-width", 111, NULL );
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-update", 1, NULL );
				break;
			case '9'://1280X720
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-top", 0, NULL );
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-left", 0, NULL );
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-height", 720, NULL );
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-width", 1280, NULL );
				g_object_set(G_OBJECT( video_sink_element ),"overlay-set-update", 1, NULL );
				break;
			case 'q'://quit
				g_print("Quit\n");
				g_main_loop_quit(play->loop);
			    break;
			default:  
			break;  
		}
}
void usage()
{
	/*************************************************/
	g_print("*******************************************\n");
	g_print("gst-play for unit test\n");
	g_print("---------------------------\n");
	g_print("b : set to null/ready state\n");
	g_print("d : set to get discovery info\n");
	g_print("g : set/unset aging test\n");
	g_print("u : set/unset unit test\n");
	g_print("r : set/unset repeat play\n");
	g_print("m : track change test\n");
	g_print("  -> select 'a' or 't' -> tarket track num.\n");
	g_print("p : set seek flag\n");
	g_print("  -> 0 : SNAP, 1 : accurate, 2 : key-unit\n");
	g_print("j : set to pause/play state\n");
	g_print("0 : set/unset mute\n");
	g_print("e 'number' : seek to 'number' ms\n");
	g_print("l : do fastforward seektest, seek to %d sec\n",set_fastseek_interval);
	g_print("f : seek to 24 sec\n");
	g_print("w : seek to 54 sec\n");
	g_print("c : set seek position - taget (ms)\n");
	g_print("t : seek to 'target' msec, current target is %d\n",user_set_seekposition);
	g_print("z : seek to zero\n");
	g_print("s 'number' : change speed rate to 'number'\n");
	g_print("h : print usage()\n");
	g_print("a : pause/unpause\n");
	g_print("n : next\n");
	g_print("q : quit\n");
	g_print("1 : ASPECT_MODE_STANDARD\n");
	g_print("2 : ASPECT_MODE_FULL\n");
	g_print("3 : ASPECT_MODE_ORIGINAL\n");
	g_print("4 : ASPECT_MODE_16_9\n");
	g_print("5 : ASPECT_MODE_4_3\n");
	g_print("6 : ASPECT_MODE_ZOOM (if support)\n");
	g_print("7 : overlay set - 0-0-111x111\n");
	g_print("8 : overlay set - 123-561-111x111\n");
	g_print("9 : overlay set - 0-0-1280x720\n");
	g_print("---------------------------\n");
	g_print("*******************************************\n");
	/*************************************************/
}
int
main (int argc, char **argv)
{
    GstPlay *play;
    GPtrArray *playlist;
    gboolean print_version = FALSE;
    gboolean gapless = FALSE;
    gchar **filenames = NULL;
    gchar *audio_sink = NULL;
    gchar *video_sink = NULL;
    gchar *text_sink = NULL;
    gchar **uris;
    guint num, i;
    GError *err = NULL;
    GOptionContext *ctx;

    GOptionEntry options[] =
    {
        {
            "version", 0, 0, G_OPTION_ARG_NONE, &print_version,
            "Print version information and exit", NULL
        },
        {
            "videosink", 0, 0, G_OPTION_ARG_STRING, &video_sink,
            "Video sink to use (default is autovideosink)", NULL
        },
        {
            "audiosink", 0, 0, G_OPTION_ARG_STRING, &audio_sink,
            "Audio sink to use (default is autoaudiosink)", NULL
        },
        {
            "textsink", 0, 0, G_OPTION_ARG_STRING, &text_sink,
            "Text sink to use (default is appsink)", NULL
        },
        {
            "gapless", 0, 0, G_OPTION_ARG_NONE, &gapless,
            "Enable gapless playback", NULL
        },
        {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL},
        {NULL}
    };

#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    /*//for suzuki
      g_set_prgname ("gst-play-" GST_API_VERSION);
    */
    g_set_prgname ("gst-play-1.0");

    ctx = g_option_context_new ("FILE1|URI1 [FILE2|URI2] [FILE3|URI3] ...");
    /*//for suzuki
      g_option_context_add_main_entries (ctx, options, GETTEXT_PACKAGE);
    */
    g_option_context_add_main_entries (ctx, options, NULL);
    g_option_context_add_group (ctx, gst_init_get_option_group ());
    if (!g_option_context_parse (ctx, &argc, &argv, &err))
    {
        g_print ("Error initializing: %s\n", GST_STR_NULL (err->message));
        g_option_context_free (ctx);
        g_clear_error (&err);
        return 1;
    }
    g_option_context_free (ctx);

    GST_DEBUG_CATEGORY_INIT (play_debug, "play", 0, "gst-play");

    /*//for suzuki
      if (print_version) {
        gchar *version_str;

        version_str = gst_version_string ();
        g_print ("%s version %s\n", g_get_prgname (), PACKAGE_VERSION);
        g_print ("%s\n", version_str);
        g_print ("%s\n", GST_PACKAGE_ORIGIN);
        g_free (version_str);
        return 0;
      }
    */

    if (filenames == NULL || *filenames == NULL)
    {
        /*//for suzuki
            g_printerr (_("Usage: %s FILE1|URI1 [FILE2|URI2] [FILE3|URI3] ..."),
                "gst-play-" GST_API_VERSION);
        */
        g_printerr ("Usage: %s FILE1|URI1 [FILE2|URI2] [FILE3|URI3] ...","gst-play-1.0");
        g_printerr ("\n\n"),g_printerr ("%s\n\n","You must provide at least one filename or URI to play.");
        return 1;
    }

    playlist = g_ptr_array_new ();

    /* fill playlist */
    num = g_strv_length (filenames);
    for (i = 0; i < num; ++i)
    {
        GST_LOG ("command line argument: %s", filenames[i]);
        add_to_playlist (playlist, filenames[i]);
    }
    g_strfreev (filenames);

    g_ptr_array_add (playlist, NULL);
	
	usage();

    /* play */
    uris = (gchar **) g_ptr_array_free (playlist, FALSE);
    play = play_new (uris, audio_sink, video_sink, text_sink, gapless);

    if (gst_play_kb_set_key_handler(keyboard_cb, play)){
        g_print ("Press 'h' to see a list of keyboard shortcuts.\n");
        atexit(restore_terminal_test);
    } else {
      g_print ("Interactive keyboard handling in terminal not available.\n");
    }

    do_play (play);

    /* clean up */
    play_free (play);

	g_free (audio_sink);
	g_free (video_sink);
	g_free (text_sink);

	gst_deinit ();

    return 0;
}

