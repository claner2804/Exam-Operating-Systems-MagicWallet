/*
 * Claudia Steiner
 * Martikelnummer: 52312304
 */


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

#define WALLET_CAPACITY 10
#define STONE_COLLECTION_TIME 2 //sekunden
#define SIMULATION_TIME 300   //5 min in sek
#define HUNTERS 5

pthread_mutex_t wallet_lock;
int coins = 0; //münzen zu beginn
bool running = true;

pthread_t hunters[HUNTERS];
pthread_t collector;

//das ist der signal handler für SIGINT für STRG+C
void handle_sigint(int sig) {
    printf("\n[Signal] Das SIGINT wurde empfangen! STRG+C ist eine tolle Tastenkombination! Die Aufgabe der Händler und Jäger aus Innsbruck beendet sich...\n");
    running = false; //setze das Flag auf false, um alle Threads zu beenden
}

//das ist die Funktion, die die Kieselsteine sammelt und in Münzen umwandelt
void* collect_stones(void* arg) {
    while (running) {
        sleep(STONE_COLLECTION_TIME); //sammle Kieselsteine für eine gewisse Zeit
        pthread_mutex_lock(&wallet_lock); //hier wird die Geldbörse gesperrt
        if (coins == 0 && running) { //wenn die Geldbörse leer ist und die Simulation noch läuft
            coins = rand() % (WALLET_CAPACITY + 1); //zufällige Anzahl an Münzen bis zur Kapazität
            printf("Der verirrte Händler sagt: Kieselsteine werden in %d Münzen verwandelt und ausgeworfen! \n", coins);
        }
        pthread_mutex_unlock(&wallet_lock); //hier wird der Geldbeutel entsperrt
    }
    return NULL;
}

//das ist die Thread-Funktion für jeden Jäger, um Münzen zu sammeln
void* hunter_thread(void* arg) {
    int hunter_id = *((int*)arg);
    int collected_coins = 0;
    free(arg);

    while (running) {
        bool collected = false;
        pthread_mutex_lock(&wallet_lock);
        if (coins > 0) {
            coins--; //nur eine Münze wird aus der Börse genommen
            collected = true;
            printf("armer Jäger %d hat schaut wie viele Münzen er noch hat.. Es sind %d Stück!\n", hunter_id, coins);
        }
        pthread_mutex_unlock(&wallet_lock);
        if (collected) {
            collected_coins++;
        }
        usleep((rand() % 400 + 100) * 1000); //zufällige delay
    }


    int* result = malloc(sizeof(int));
    if (result == NULL) {
        perror("Achtung!! Fehler bei der Speicherzuweisung");
        pthread_exit(NULL);
    }
    *result = collected_coins;
    pthread_exit(result);
}

//das ist die Funktion, die die Simulation sauber beendet "graceful_shutdown" und auf alle Threads wartet
void graceful_shutdown() {
    printf("Nun warten wir auf alle Threads der Jäger und der Händler und beenden die Aufgabe...\n");
    running = false;

    //warten auf alle Jäger-Threads
    for (int i = 0; i < HUNTERS; i++) {
        void* retval;
        pthread_join(hunters[i], &retval);
        if (retval != NULL) {
            int collected_coins = *((int*)retval);
            free(retval);
            printf("Der edle Jäger Nummer %d hat ganze %d Münzen gesammelt. Ein reicher Kerl!\n", i, collected_coins);
        } else {
            printf("Der arme Jäger %d hat gar keine Münzen gesammelt... Er ist nun traurig.\n", i);
        }
    }

    pthread_join(collector, NULL); //wenn der Händler fertig ist, warte auf ihn
    pthread_mutex_destroy(&wallet_lock);
    printf("Die Jäger und die Sammler sind müde und beeden ihre Aufgaben. \n");
}

int main() {
    srand(time(NULL)); //für zufallszahlen
    pthread_mutex_init(&wallet_lock, NULL);

    //für zufällige Anzahl an Münzen zu Beginn
    coins = rand() % (WALLET_CAPACITY + 1);
    printf("Wir starten motiviert in die Aufgabe und haben zu Anfang einige Münzen in der Geldbörse: %d\n", coins);

    signal(SIGINT, handle_sigint);

    //das starten des Sammel-Threads (Händler), um Kieselsteine zu sammeln und Münzen zu werfen
    pthread_create(&collector, NULL, collect_stones, NULL);

    //das starten der Jäger-Threads
    for (int i = 0; i < HUNTERS; i++) {
        int* hunter_id = malloc(sizeof(int));
        if (hunter_id == NULL) {
            perror("Achtung!! Fehler bei der Speicherzuweisung");
            exit(EXIT_FAILURE);
        }
        *hunter_id = i;
        pthread_create(&hunters[i], NULL, hunter_thread, hunter_id);
    }

    sleep(SIMULATION_TIME);
    graceful_shutdown(); //sauberes Beenden der Aufgabe und wartet auf alle Threads

    return 0;
}
