/*
 * My_Alarm.c
 *
 * This is an enhancement to the alarm_mutex.c program. This new
 * version uses an alarm thread, which retreives the next
 * entry in a list, and assigns a display thread to process the
 * alarm. The main thread places new requests onto the
 * list, in order of absolute expiration time. The list is
 * protected by a mutex, and the alarm thread sleeps for at
 * least 1 second, each iteration, to ensure that the main
 * thread can lock the mutex to add new work to the list.
 */
#include <pthread.h>
#include <time.h>
#include "errors.h"

/*
 * The "alarm" structure now contains the time_t (time since the
 * Epoch, in seconds) for each alarm, so that they can be
 * sorted. Storing the requested number of seconds would not be
 * enough, since the "alarm thread" cannot tell how long it has
 * been on the list.
 */
typedef struct alarm_tag {
    struct alarm_tag    *link;
    int                 seconds;
    time_t              time;   /* seconds from EPOCH */
    char                message[64];
} alarm_t;

pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t d1_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t d2_cond = PTHREAD_COND_INITIALIZER;
alarm_t *alarm_list = NULL;
alarm_t *current_alarm = NULL;  /* current alarm to process */

/*
 * The alarm thread's start routine.
 */
void *alarm_thread (void *arg)
{
    alarm_t *alarm;
    time_t now;
    int status;
    int sleep_time;

    /*
     * Loop forever, retreiving alarms. The alarm thread will
     * be disintegrated when the process exits.
     */
    while (1) 
    {
        status = pthread_mutex_lock (&alarm_mutex);
        if (status != 0)
            err_abort (status, "Lock mutex");
        alarm = alarm_list;

        /*
         * If the alarm list is empty, wait for one second. This
         * allows the main thread to run, and read another
         * command. If the list is not empty, remove the first
         * item. Get the expiry time, and assign display thread 1
	 * to process the alarm if the expiry time is odd, or display
         * thread 2 if the expiry time is even.
         */
        if (alarm == NULL)
	   sleep_time = 1;
        else
	{ 	    
            alarm_list = alarm->link;
	    current_alarm = alarm;
	    if (alarm->time % 2 == 1)
            {
		/* 
		 * Message to indicate that the current alarm has been passed to
		 * display thread 1
		 */
	   	printf("Alarm Thread Passed on Alarm Request to Display Thread 1 "
			"at %d: %d %s\n", time (NULL), alarm->seconds, alarm->message);
		/* Wake up display thread 1 to process the current alarm */
		status = pthread_cond_signal(&d1_cond);
		if (status != 0)
	    		err_abort(status, "Signal cond");
	    }
	    else
	    {
		/* 
		 * Message to indicate that the current alarm has been passed to
		 * display thread 2
		 */
		printf("Alarm Thread Passed on Alarm Request to Display Thread 2 "
			"at %d: %d %s\n", time (NULL), alarm->seconds, alarm->message);
		/* Wake up display thread 2 to process the current alarm */
		status = pthread_cond_signal(&d2_cond);
		if (status != 0)
	    		err_abort(status, "Signal cond");
	    }
	}

        /*
         * Unlock the mutex before waiting, so that the main
         * thread can lock it to insert a new alarm request,
	 * or have one of the display thread process the current alarm.
         */
        status = pthread_mutex_unlock (&alarm_mutex);
        if (status != 0)
            err_abort (status, "Unlock mutex");
	sleep(sleep_time);
    }
}

/* Display 1 start routine */
void *display_thread_1(void *arg)
{
    int status;
    alarm_t *alarm;
    time_t now;

    /*
     * Loop forever, processing alarms. The display thread will
     * be disintegrated when the process exits.
     */
    while(1)
    {
        status = pthread_mutex_lock(&alarm_mutex);
    	if (status != 0)
	    err_abort(status, "Lock mutex");
	/* 
	 * Wait for the alarm thread to signal when an alarm is ready to be
      	 * procced.
	 */
	status = pthread_cond_wait(&d1_cond, &alarm_mutex);
	if (status != 0)
	    err_abort(status, "Wait on cond");
        alarm = current_alarm;
	/* Message to indicate that display thread 1 has received the alarm */
	printf("Display Thread 1: Received Alarm Request at %d: %d %s,"
		" ExpiryTime is %d \n", time (NULL), alarm->seconds,
		alarm->message, alarm->time);
	now = time (NULL);
	/* While the alarm has yet to expiry, print a message every 2 seconds */
	while(alarm->time > time (NULL))
	{
	    printf("Display Thread 1: Number of Seconds Left %d: Time: %d: "
			"%d %s\n",alarm->time - time (NULL), now
				, alarm->seconds, alarm->message);
	    sleep(2);
	}
	/* Prints a message saying that the current alarm has expired */
	printf("Display Thread 1: Alarm Expired at %d: "
			"%d %s\n", time (NULL), alarm->seconds, alarm->message);
	status = pthread_mutex_unlock(&alarm_mutex);
    	if (status != 0)
	    err_abort(status, "unlock mutex");
	free(alarm);
     }	
}

/* Display 2 start routine */
void *display_thread_2(void *arg)
{
    int status;
    alarm_t *alarm;
    time_t now;

    /*
     * Loop forever, processing alarms. The display thread will
     * be disintegrated when the process exits.
     */
    while(1)
    {
        status = pthread_mutex_lock(&alarm_mutex);
    	if (status != 0)
	    err_abort(status, "Lock mutex");
	/* 
	 * Wait for the alarm thread to signal when an alarm is ready to be
      	 * procced.
	 */
	status = pthread_cond_wait(&d2_cond, &alarm_mutex);
	if (status != 0)
	    err_abort(status, "Wait on cond");
        alarm = current_alarm;
	/* Message to indicate that display thread 2 has received the alarm */
	printf("Display Thread 2: Received Alarm Request at %d: %d %s,"
		" ExpiryTime is %d \n", time (NULL), alarm->seconds,
		alarm->message, alarm->time);
	now = time (NULL);
	/* While the alarm has yet to expiry, print a message every 2 seconds */
	while(alarm->time > time (NULL))
	{
	    printf("Display Thread 2: Number of Seconds Left %d: Time: %d: "
			"%d %s\n",alarm->time - time (NULL), now
				, alarm->seconds, alarm->message);
	    sleep(2);
	}
	/* Prints a message saying that the current alarm has expired */
	printf("Display Thread 2: Alarm Expired at %d: "
			"%d %s\n", time (NULL), alarm->seconds, alarm->message);
	status = pthread_mutex_unlock(&alarm_mutex);
    	if (status != 0)
	    err_abort(status, "unlock mutex");
	free(alarm);
     }	
}

int main (int argc, char *argv[])
{
    int status;
    char line[128];
    alarm_t *alarm, **last, *next;
    pthread_t a_thread; /* Alarm thread */
    pthread_t d1_thread; /* Display thread 1 */
    pthread_t d2_thread; /* Display thread 2 */

    status = pthread_create (
        &a_thread, NULL, alarm_thread, NULL);
    if (status != 0)
        err_abort (status, "Create alarm thread");
    status = pthread_create (
	&d1_thread, NULL, display_thread_1, NULL);
    if (status != 0)
	err_abort (status, "Create display thread 1");
    status = pthread_create (
	&d2_thread, NULL, display_thread_2, NULL);
    if (status != 0)
	err_abort (status, "Create display thread 2");
    while (1) {
        printf ("alarm> ");
        if (fgets (line, sizeof (line), stdin) == NULL) exit (0);
        if (strlen (line) <= 1) continue;
        alarm = (alarm_t*)malloc (sizeof (alarm_t));
        if (alarm == NULL)
            errno_abort ("Allocate alarm");

        /*
         * Parse input line into seconds (%d) and a message
         * (%64[^\n]), consisting of up to 64 characters
         * separated from the seconds by whitespace.
         */
        if (sscanf (line, "%d %64[^\n]", 
            &alarm->seconds, alarm->message) < 2) {
            fprintf (stderr, "Bad command\n");
            free (alarm);
        } else {
            status = pthread_mutex_lock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Lock mutex");

	    /* Alarm request received message */
	    printf("Main Thread Received Alarm Request at %d: %d %s\n",
			time(NULL), alarm->seconds, alarm->message);

            alarm->time = time (NULL) + alarm->seconds;

            /*
             * Insert the new alarm into the list of alarms,
             * sorted by expiration time.
             */
            last = &alarm_list;
            next = *last;
            while (next != NULL) {
                if (next->time >= alarm->time) {
                    alarm->link = next;
                    *last = alarm;
                    break;
                }
                last = &next->link;
                next = next->link;
            }
            /*
             * If we reached the end of the list, insert the new
             * alarm there. ("next" is NULL, and "last" points
             * to the link field of the last item, or to the
             * list header).
             */
            if (next == NULL) {
                *last = alarm;
                alarm->link = NULL;
            }
#ifdef DEBUG
            printf ("[list: ");
            for (next = alarm_list; next != NULL; next = next->link)
                printf ("%d(%d)[\"%s\"] ", next->time,
                    next->time - time (NULL), next->message);
            printf ("]\n");
#endif
            status = pthread_mutex_unlock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Unlock mutex");
        }
    }
}
