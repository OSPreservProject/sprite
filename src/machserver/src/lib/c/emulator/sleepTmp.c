/* Temporary hack to provide sleep support the clients. */

#include <sprite.h>
#include <mach.h>
#include <mach/message.h>

static mach_msg_header_t sleepMsg;
static mach_port_t sleepPort;
static void SleepInitialize();

void
msleep(milliseconds)
    int milliseconds;
{
    static Boolean initialized = 0;

    if (!initialized) {
	SleepInitialize();
	initialized = TRUE;
    }
    (void)mach_msg(&sleepMsg, MACH_RCV_MSG | MACH_RCV_TIMEOUT, 0,
		   sizeof(sleepMsg), sleepPort,
		   (mach_msg_timeout_t)milliseconds, MACH_PORT_NULL);
}

void
sleep(seconds)
    int seconds;
{
    msleep(seconds * 1000);
}

static void
SleepInitialize()
{
    mach_init();
    (void)mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
			     &sleepPort);
}
