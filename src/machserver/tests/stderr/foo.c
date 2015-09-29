/* 
 * Test program to try reading from stderr.  Yeah, it's gross, but tset 
 * depends on it, and it seems to work with native Sprite and Mach.
 */

main()
{
    int i;
    char buf[1024];

    while((i = read(2, buf, sizeof(buf))) > 0) {
	write(1, buf, i);
    }
    if (i < 0) {
	perror("read");
    }
}
