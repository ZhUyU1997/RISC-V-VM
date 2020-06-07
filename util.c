#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/select.h>

static struct termios orig_termios; /* In order to restore at exit.*/

static int rawmode = 0;
void disableRawMode(int fd)
{
    /* Don't even check the return value as it's too late. */
    if (rawmode)
    {
        tcsetattr(fd, TCSAFLUSH, &orig_termios);
        rawmode = 0;
    }
}

/* Called at exit to avoid remaining in raw mode. */
void disableRawModeAtExit(void)
{
    disableRawMode(STDIN_FILENO);
}

/* Raw mode: 1960 magic shit. */
int enableRawMode(int fd)
{
    struct termios raw;

    if (!isatty(fd))
        goto fatal;
    atexit(disableRawModeAtExit);
    if (tcgetattr(fd, &orig_termios) == -1)
        goto fatal;

    raw = orig_termios; /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer. */
    raw.c_cc[VMIN] = 0;  /* Return each byte, or zero for timeout. */
    raw.c_cc[VTIME] = 1; /* 100 ms timeout (unit is tens of second). */

    rawmode = 1;
    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd, TCSAFLUSH, &raw) < 0)
        goto fatal;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}

int probchar()
{
    fd_set rfds;
    struct timeval tv;
    int retval;

    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);

    /* Wait up to five seconds. */

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    retval = select(1, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */

    return retval;
}

int peekchar()
{
    int retval = probchar();
    unsigned char c;

    if (retval == -1)
    {
        perror("select()");
        return -1;
    }
    else if (retval)
    {
        read(STDIN_FILENO, &c, 1);
        return c;
    }
    else
        return -1;
}