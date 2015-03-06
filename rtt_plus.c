/* include rtt1 */
#include	"unprtt_plus.h"

// int		rtt_d_flag = 0;		/* debug flag; can be set by caller */

/*
 * Calculate the RTO value based on current estimators:
 *		smoothed RTT plus four times the deviation
 */
// #define	RTT_RTOCALC(ptr) ((ptr)->rtt_srtt + (4.0 * (ptr)->rtt_rttvar))
#define	RTT_RTOCALC(ptr) ((ptr)->rtt_srtt + 4 * ((ptr)->rtt_rttvar))

/*
static float
rtt_minmax(float rto)
{
	if (rto < RTT_RXTMIN)
		rto = RTT_RXTMIN;
	else if (rto > RTT_RXTMAX)
		rto = RTT_RXTMAX;
	return(rto);
}
 */

static int rtt_minmax(int rto) {
	if (rto < RTT_RXTMIN)
		rto = RTT_RXTMIN;
	else if (rto > RTT_RXTMAX)
		rto = RTT_RXTMAX;
	return(rto);
}

/*
void
rtt_init(struct rtt_info *ptr)
{
	struct timeval	tv;

	Gettimeofday(&tv, NULL);
	ptr->rtt_base = tv.tv_sec;

	ptr->rtt_rtt    = 0;
	ptr->rtt_srtt   = 0;
	ptr->rtt_rttvar = 0.75;
	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
}
*/

void
rtt_init(struct rtt_info_plus *ptr)
{
	struct timeval	tv;

	Gettimeofday(&tv, NULL);
	ptr->rtt_base = tv.tv_sec;

	ptr->rtt_rtt    = 0;
	ptr->rtt_srtt   = 0;
	ptr->rtt_rttvar = 750;
  ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
}

/*
 * Return the current timestamp.
 * Our timestamps are 32-bit integers that count milliseconds since
 * rtt_init() was called.
 */

/* include rtt_ts */
/*
uint32_t
rtt_ts(struct rtt_info *ptr)
{
	uint32_t		ts;
	struct timeval	tv;

	Gettimeofday(&tv, NULL);
	ts = ((tv.tv_sec - ptr->rtt_base) * 1000) + (tv.tv_usec / 1000);
	return(ts);
}
 */
uint32_t
rtt_ts(struct rtt_info_plus *ptr)
{
	uint32_t		ts;
	struct timeval	tv;

	Gettimeofday(&tv, NULL);
	ts = ((tv.tv_sec - ptr->rtt_base) * 1000) + (tv.tv_usec / 1000);
	return(ts);
}

/*
void
rtt_newpack(struct rtt_info *ptr)
{
	ptr->rtt_nrexmt = 0;
}
 */
void
rtt_newpack(struct rtt_info_plus *ptr)
{
	ptr->rtt_nrexmt = 0;
}

/*
int
rtt_start(struct rtt_info_plus *ptr)
{
	return((int) (ptr->rtt_rto + 0.5));
}
 */

int
rtt_start(struct rtt_info_plus *ptr)
{
	return ptr->rtt_rto;
}
/* end rtt_ts */

/*
 * A response was received.
 * Stop the timer and update the appropriate values in the structure
 * based on this packet's RTT.  We calculate the RTT, then update the
 * estimators of the RTT and its mean deviation.
 * This function should be called right after turning off the
 * timer with alarm(0), or right after a timeout occurs.
 */

/* include rtt_stop 
void
rtt_stop(struct rtt_info *ptr, uint32_t ms)
{
	double		delta;

	ptr->rtt_rtt = ms / 1000.0;

	delta = ptr->rtt_rtt - ptr->rtt_srtt;
	ptr->rtt_srtt += delta / 8;

	if (delta < 0.0)
		delta = -delta;

	ptr->rtt_rttvar += (delta - ptr->rtt_rttvar) / 4;

	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
}
 end rtt_stop */

void
rtt_stop(struct rtt_info_plus *ptr, uint32_t ms)
{
	int delta;

	// ptr->rtt_rtt = ms / 1000;
  ptr->rtt_rtt = ms;

	delta = ptr->rtt_rtt - ptr->rtt_srtt;
	ptr->rtt_srtt += delta / 8;

	if (delta < 0)
		delta = -delta;
  
	ptr->rtt_rttvar += (delta - ptr->rtt_rttvar) / 4;

	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
}

/*
 * A timeout has occurred.
 * Return -1 if it's time to give up, else return 0.
 */

/* include rtt_timeout
int
rtt_timeout(struct rtt_info *ptr)
{
	ptr->rtt_rto *= 2;

	if (++ptr->rtt_nrexmt > RTT_MAXNREXMT)
		return(-1);
	return(0);
}
 end rtt_timeout */

int
rtt_timeout(struct rtt_info_plus *ptr)
{
	ptr->rtt_rto *= 2;

  ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));

  printf("rtt_nrexmt = %d\n", ptr->rtt_nrexmt);
	if (++ptr->rtt_nrexmt > RTT_MAXNREXMT)
    return(-1);

	return(0);
}

/*
 * Print debugging information on stderr, if the "rtt_d_flag" is nonzero.
 */

/*
void
rtt_debug(struct rtt_info_plus *ptr)
{
	if (rtt_d_flag == 0)
		return;

	fprintf(stderr, "rtt = %.3f, srtt = %.3f, rttvar = %.3f, rto = %.3f\n",
			ptr->rtt_rtt, ptr->rtt_srtt, ptr->rtt_rttvar, ptr->rtt_rto);
	fflush(stderr);
}
 */
