#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int id;
    char *payload;
} event_t;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    event_t *pending;
    int stopping;
} monitor_t;

static void monitor_init(monitor_t *m) {
    pthread_mutex_init(&m->mutex, NULL);
    pthread_cond_init(&m->cond, NULL);
    m->pending = NULL;
    m->stopping = 0;
}

static void monitor_destroy(monitor_t *m) {
    pthread_mutex_destroy(&m->mutex);
    pthread_cond_destroy(&m->cond);
}

static void monitor_provide(monitor_t *m, event_t *ev) {
    pthread_mutex_lock(&m->mutex);

    while (m->pending != NULL && !m->stopping)
        pthread_cond_wait(&m->cond, &m->mutex);

    if (m->stopping) {
        pthread_mutex_unlock(&m->mutex);
        
        free(ev->payload);
        free(ev);
        return;
    }

    m->pending = ev;
    printf("Событие %d отправлено\n", ev->id);
    fflush(stdout);

    pthread_cond_signal(&m->cond);
    pthread_mutex_unlock(&m->mutex);
}

static void monitor_stop(monitor_t *m) {
    pthread_mutex_lock(&m->mutex);
    m->stopping = 1;
    pthread_cond_broadcast(&m->cond);
    pthread_mutex_unlock(&m->mutex);
}

static event_t *monitor_consume(monitor_t *m) {
    pthread_mutex_lock(&m->mutex);

    while (m->pending == NULL && !m->stopping)
        pthread_cond_wait(&m->cond, &m->mutex);

    if (m->pending == NULL) {
        pthread_mutex_unlock(&m->mutex);
        return NULL;
    }

    event_t *ev = m->pending;
    m->pending = NULL;

    printf("Событие %d получено\n", ev->id);
    fflush(stdout);

    pthread_cond_signal(&m->cond);
    pthread_mutex_unlock(&m->mutex);
    
    return ev;
}

static monitor_t mon;

static void *provider_thread(void *arg) {
    (void)arg;
    for (int i = 1; i <= 5; ++i) {
        sleep(1);
        
        event_t *ev = malloc(sizeof(*ev));
        ev->id = i;
        
        ev->payload = malloc(64);
        snprintf(ev->payload, 64, "Данные для события %d", i);

        monitor_provide(&mon, ev);
    }

    monitor_stop(&mon);
    return NULL;
}

static void *consumer_thread(void *arg) {
    (void)arg;
    for (;;) {
        event_t *ev = monitor_consume(&mon);
        if (ev == NULL)
            break;
        
        printf("    %s\n\n", ev->payload);
        
        free(ev->payload);
        free(ev);
    }

    return NULL;
}

int main(void) {
    monitor_init(&mon);

    pthread_t prov, cons;
    pthread_create(&prov, NULL, provider_thread, NULL);
    pthread_create(&cons, NULL, consumer_thread, NULL);

    pthread_join(prov, NULL);
    pthread_join(cons, NULL);

    monitor_destroy(&mon);
    return 0;
}