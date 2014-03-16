#define _BSD_SOURCE
#define BATT_NOW    "/sys/class/power_supply/BAT0/charge_now"
#define BATT_FULL    "/sys/class/power_supply/BAT0/charge_full"
#define BATT_STATUS    "/sys/class/power_supply/BAT0/status"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
    va_list fmtargs;
    char *ret;
    int len;

    va_start(fmtargs, fmt);
    len = vsnprintf(NULL, 0, fmt, fmtargs);
    va_end(fmtargs);

    ret = malloc(++len);
    if (ret == NULL) {
        perror("malloc");
        exit(1);
    }

    va_start(fmtargs, fmt);
    vsnprintf(ret, len, fmt, fmtargs);
    va_end(fmtargs);

    return ret;
}

void
settz(char *tzname)
{
    setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
    char buf[129];
    time_t tim;
    struct tm *timtm;

    memset(buf, 0, sizeof(buf));

    if (tzname != NULL) {
        settz(tzname);
    }
    tim = time(NULL);
    timtm = localtime(&tim);
    if (timtm == NULL) {
        perror("localtime");
        exit(1);
    }

    if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
        fprintf(stderr, "strftime == 0\n");
        exit(1);
    }

    return smprintf("%s", buf);
}

long
read_long(const char *path)
{
    long i = 0;
    FILE *fd;

    if (!(fd = fopen(path, "r")))
        return -1;

    fscanf(fd, "%ld", &i);
    fclose(fd);
    return i;
}

char
get_status()
{
    FILE *fp;
    char st;

    if ((fp = fopen(BATT_STATUS, "r")) == NULL)
        return '?';

    st = fgetc(fp);
    fclose(fp);

    switch(st) {
        case 'C': return '+';
        case 'D': return '-';
        case 'F': return '=';
        default : return '?';
    }
}

char *
getbattery()
{
    long lnum1, lnum2, percent;
    const char *color;
    char s;

    lnum1 = read_long(BATT_NOW);
    lnum2 = read_long(BATT_FULL);
    s = get_status();

    percent = (lnum1/(lnum2/100));

    if (percent <= 20)
        color = "#ff0000";
    else
        color = "#00ff00";

    return smprintf("<span color=\"%s\">%c%ld%%</span>", color, s, percent);
}

void
setstatus(char *str)
{
    XStoreName(dpy, DefaultRootWindow(dpy), str);
    XSync(dpy, False);
}

int
main(void)
{
    char *status;
    char *timeinfo;
    char *battery;

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "dwmstatus: cannot open display.\n");
        return 1;
    }

    for (;;sleep(30)) {
        timeinfo = mktimes("%a %d %b %H:%M %Z %Y", NULL);
        battery = getbattery();

        status = smprintf("%s / %s",
                battery, timeinfo);
        setstatus(status);
        free(battery);
        free(timeinfo);
        free(status);
    }

    XCloseDisplay(dpy);

    return 0;
}
