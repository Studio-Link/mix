#include <mix.h>

static struct tmr tmr_metrics;
static struct tmr tmr_jitter;
static uint64_t jitter_last = 0;
static int64_t max_jitter   = 0; /* Mainloop jitter */
enum { JITTER_INTERVAL = 10 };


static void jitter_stats(void *arg)
{
	(void)arg;

	if (jitter_last) {
		int64_t jitter =
			(tmr_jiffies() - jitter_last) - JITTER_INTERVAL;
		if (jitter > max_jitter)
			max_jitter = jitter;
	}

	jitter_last = tmr_jiffies();
	tmr_start(&tmr_jitter, JITTER_INTERVAL, jitter_stats, NULL);
}


static void slmix_metrics(void *arg)
{
	struct le *le;
	int err;
	char metric_url[URL_SZ] = {0};
	struct sl_httpconn *http_conn;
	const struct rtcp_stats *audio_stat;
	const struct rtcp_stats *video_stat;
	struct jbuf_stat audio_jstat;
	struct jbuf_stat video_jstat;
	bool types = true;
	(void)arg;
	struct mix *mix = slmix();

	struct mbuf *mb = mbuf_alloc(512);
	if (!mb)
		goto out;

	re_snprintf(metric_url, sizeof(metric_url), METRICS_URL "/instance/%s",
		    mix->room);

	mbuf_printf(mb, "# TYPE mix_jitter gauge\n");
	mbuf_printf(mb, "mix_jitter %lld\n", max_jitter);
	max_jitter = 0;

	LIST_FOREACH(&mix->sessl, le)
	{
		struct session *sess = le->data;
		char labels[64]	     = {0};

		if (!sess->user || !sess->connected || !sess->user->speaker)
			continue;

		re_snprintf(labels, sizeof(labels), "user=\"%s\"",
			    sess->user->id);

		if (types) {
			mbuf_printf(mb, "# TYPE mix_rtt gauge\n");
			mbuf_printf(mb, "# TYPE mix_tx_sent counter\n");
			mbuf_printf(mb, "# TYPE mix_tx_lost counter\n");
			mbuf_printf(mb, "# TYPE mix_tx_jit gauge\n");
			mbuf_printf(mb, "# TYPE mix_rx_sent counter\n");
			mbuf_printf(mb, "# TYPE mix_rx_lost counter\n");
			mbuf_printf(mb, "# TYPE mix_rx_jit gauge\n");
			mbuf_printf(mb, "# TYPE mix_jbuf_delay gauge\n");
			mbuf_printf(mb, "# TYPE mix_jbuf_skew gauge\n");
			mbuf_printf(mb, "# TYPE mix_jbuf_late counter\n");
			mbuf_printf(mb, "# TYPE mix_jbuf_late_lost counter\n");
			mbuf_printf(mb, "# TYPE mix_jbuf_lost counter\n");
			mbuf_printf(mb, "# TYPE mix_jbuf_gnacks counter\n");
			mbuf_printf(mb, "# TYPE mix_jbuf_jitter gauge\n");
			mbuf_printf(mb, "# TYPE mix_jbuf_packets gauge\n");
			types = false;
		}

		audio_stat = stream_rtcp_stats(media_get_stream(sess->maudio));
		if (audio_stat) {
			mbuf_printf(mb, "mix_rtt{%s,kind=\"audio\"} %u\n",
				    labels, audio_stat->rtt / 1000);
			mbuf_printf(mb, "mix_tx_sent{%s,kind=\"audio\"} %u\n",
				    labels, audio_stat->tx.sent);
			mbuf_printf(mb, "mix_tx_lost{%s,kind=\"audio\"} %d\n",
				    labels, audio_stat->tx.lost);
			mbuf_printf(mb, "mix_tx_jit{%s,kind=\"audio\"} %u\n",
				    labels, audio_stat->tx.jit);
			mbuf_printf(mb, "mix_rx_sent{%s,kind=\"audio\"} %u\n",
				    labels, audio_stat->rx.sent);
			mbuf_printf(mb, "mix_rx_lost{%s,kind =\"audio\"} %d\n",
				    labels, audio_stat->rx.lost);
			mbuf_printf(mb, "mix_rx_jit{%s,kind=\"audio\"} %u\n",
				    labels, audio_stat->rx.jit);
		}

		video_stat = stream_rtcp_stats(media_get_stream(sess->mvideo));
		if (video_stat) {
			mbuf_printf(mb, "mix_rtt{%s,kind=\"video\"} %u\n",
				    labels, video_stat->rtt / 1000);
			mbuf_printf(mb, "mix_tx_sent{%s,kind=\"video\"} %u\n",
				    labels, video_stat->tx.sent);
			mbuf_printf(mb, "mix_tx_lost{%s,kind=\"video\"} %d\n",
				    labels, video_stat->tx.lost);
			mbuf_printf(mb, "mix_tx_jit{%s,kind=\"video\"} %u\n",
				    labels, video_stat->tx.jit);
			mbuf_printf(mb, "mix_rx_sent{%s,kind=\"video\"} %u\n",
				    labels, video_stat->rx.sent);
			mbuf_printf(mb, "mix_rx_lost{%s,kind=\"video\"} %d\n",
				    labels, video_stat->rx.lost);
			mbuf_printf(mb, "mix_rx_jit{%s,kind=\"video\"} %u\n",
				    labels, video_stat->rx.jit);
		}

		err = stream_jbuf_stats(media_get_stream(sess->maudio),
					&audio_jstat);
		err |= stream_jbuf_stats(media_get_stream(sess->mvideo),
					 &video_jstat);
		if (err)
			continue;

		mbuf_printf(mb, "mix_jbuf_delay{%s,kind=\"audio\"} %u\n",
			    labels, audio_jstat.c_delay);
		mbuf_printf(mb, "mix_jbuf_skew{%s,kind=\"audio\"} %d\n",
			    labels, audio_jstat.c_skew);
		mbuf_printf(mb, "mix_jbuf_late{%s,kind=\"audio\"} %u\n",
			    labels, audio_jstat.n_late);
		mbuf_printf(mb, "mix_jbuf_late_lost{%s,kind=\"audio\"} %u\n",
			    labels, audio_jstat.n_late_lost);
		mbuf_printf(mb, "mix_jbuf_lost{%s,kind=\"audio\"} %u\n",
			    labels, audio_jstat.n_lost);
		mbuf_printf(mb, "mix_jbuf_jitter{%s,kind=\"audio\"} %u\n",
			    labels, audio_jstat.c_jitter);
		mbuf_printf(mb, "mix_jbuf_packets{%s,kind=\"audio\"} %u\n",
			    labels, audio_jstat.c_packets);
		mbuf_printf(mb, "mix_jbuf_gnacks{%s,kind=\"audio\"} %u\n",
			    labels, audio_jstat.n_gnacks);

		mbuf_printf(mb, "mix_jbuf_delay{%s,kind=\"video\"} %u\n",
			    labels, video_jstat.c_delay);
		mbuf_printf(mb, "mix_jbuf_skew{%s,kind=\"video\"} %d\n",
			    labels, video_jstat.c_skew);
		mbuf_printf(mb, "mix_jbuf_late{%s,kind=\"video\"} %u\n",
			    labels, video_jstat.n_late);
		mbuf_printf(mb, "mix_jbuf_late_lost{%s,kind=\"video\"} %u\n",
			    labels, video_jstat.n_late_lost);
		mbuf_printf(mb, "mix_jbuf_lost{%s,kind=\"video\"} %u\n",
			    labels, video_jstat.n_lost);
		mbuf_printf(mb, "mix_jbuf_jitter{%s,kind=\"video\"} %u\n",
			    labels, video_jstat.c_jitter);
		mbuf_printf(mb, "mix_jbuf_packets{%s,kind=\"video\"} %u\n",
			    labels, video_jstat.c_packets);
		mbuf_printf(mb, "mix_jbuf_gnacks{%s,kind=\"video\"} %u\n",
			    labels, video_jstat.n_gnacks);
	}

	if (mbuf_pos(mb) == 0)
		goto out;

	err = sl_httpc_alloc(&http_conn, NULL, NULL, NULL);
	if (err)
		goto out;

	sl_httpc_req(http_conn, SL_HTTP_POST, metric_url, mb);
	mem_deref(http_conn);

out:
	mem_deref(mb);
	tmr_start(&tmr_metrics, 2000, slmix_metrics, NULL);
}


int slmix_stats_init(void)
{
	tmr_init(&tmr_metrics);
	tmr_init(&tmr_jitter);
	tmr_start(&tmr_metrics, 2000, slmix_metrics, NULL);
	tmr_start(&tmr_jitter, JITTER_INTERVAL, jitter_stats, NULL);

	return 0;
}


void slmix_stats_close(void)
{
	tmr_cancel(&tmr_metrics);
}
