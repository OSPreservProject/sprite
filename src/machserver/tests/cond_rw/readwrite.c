/* 
 * Forks a child that simply echoes characters passed to it.  Can 
 * either use busy waiting or a condition variable to wait on the 
 * shared buffer.  Uses a condition variable if USE_CONDITION is 
 * defined.
 */

#include <cthreads.h>
#include <stdio.h>

#ifdef USE_CONDITION
struct condition cond;
#endif


struct mutex lock;
int buf = 0;

any_t ChildProc();
void pause();
void notify();

main()
{
	cthread_t child;
	int ch;

	cthread_init();
	mutex_init(&lock);
#ifdef USE_CONDITION
	condition_init(&cond);
#endif

	child = cthread_fork(ChildProc, (any_t)0);

	do {
		ch = getchar();
		if (ch == 0)
			continue;
		sendach(ch);
	} while (ch != EOF);

	cthread_join(child);
	exit(0);
}

any_t
ChildProc(arg)
	any_t arg;
{
	int ch;

	/* Wire our C thread to a kernel thread, for gdb's sake. */
	cthread_wire();

	while ((ch = getach()) != EOF) {
		if (ch == 'D') {
			abort();
		}
		putchar(ch);
		fflush(stdout);
	}
}

/* Wait until buf is empty, then put char in and return. */
sendach(ch)
	int ch;
{
	mutex_lock(&lock);

	while (buf != 0)
		pause();
	buf = ch;
	notify();

	mutex_unlock(&lock);
}

/* Wait until buf is not empty, then remove char and return it. */
int
getach()
{
	int ch;

	mutex_lock(&lock);

	while (buf == 0)
		pause();
	ch = buf;
	buf = 0;
	notify();

	mutex_unlock(&lock);
	return(ch);
}

void
pause()
{
#ifdef USE_CONDITION
	condition_wait(&cond, &lock);
#else /* USE_CONDITION */
	mutex_unlock(&lock);
	cthread_yield();
	mutex_lock(&lock);
#endif /* USE_CONDITION */
}

void
notify()
{
#ifdef USE_CONDITION
	condition_signal(&cond);
#endif
}
