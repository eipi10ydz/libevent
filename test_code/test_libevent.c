#include <event.h>

struct event ev;
struct timeval tv;

void time_cb(int fd, short event, void *argc) {
    printf("timer wake up\n");
    // event_add(&ev, &tv);
}

int main() {
    struct event_base *base = event_init();

    tv.tv_sec = 1;
    tv.tv_usec = 0;
    // evtimer_set(&ev, time_cb, NULL);
    event_set(&ev, -1, EV_PERSIST, time_cb, NULL);
    event_add(&ev, &tv);
    event_base_set(base, &ev);
    event_base_dispatch(base);
}