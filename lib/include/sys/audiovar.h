/*	"@(#)audiovar.h 1.17 89/04/20 SMI"
*/

#ifndef LOCORE

/*  This is the default size for the ring buffer.  Size must be a power of
 *  2, so that we can make a bit mask for efficient operation.  Eventually,
 *  the user will be able to specify the size of the buffer that he
 *  desires.
 */
#define AUDIO_DEFAULT_RING_SIZE		16384
#define AUDIO_DEFAULT_RING_MASK		0x003fff

#define AUDIO_MIN_RING_SIZE		1024
#define AUDIO_MAX_RING_SIZE		1048576


#if 0
struct	audio_ring {
	char	*data;
	int	head;
	int	tail;
	int	wakeup;
	int	size;
	int	mask;
};

struct	audio {
	int	opened;
	int	read_active;
	int	write_active;
	int	pause;
	int	drain;
	int	interrupt_reason;
	struct	audio_chip	*chip;
	struct	audio_ring	read_ring;
	struct	audio_ring	write_ring;
	struct	buf	read_buf;
	struct	buf	write_buf;
};

#endif

#endif

/*  These are the reasons we can schedule a level 4 interrupt.
 */

#define AUDIO_INTERRUPT_NONE		0
#define AUDIO_INTERRUPT_WAKEUP_READ	0x01
#define AUDIO_INTERRUPT_WAKEUP_WRITE	0x02
#define AUDIO_INTERRUPT_STOP_READ	0x04
#define AUDIO_INTERRUPT_STOP_WRITE	0x08
#define AUDIO_INTERRUPT_UNEXPECTED	0x10
