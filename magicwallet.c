#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

#define WALLET_CAPACITY 10
#define STONE_COLLECTION_TIME 2 // Sekunden
#define SIMULATION_TIME 300   // 5 Minuten in Sekunden
#define HUNTERS 5

pthread_mutex_t wallet_lock;
int coins = 0; // Zufällige Anzahl der Münzen zu Beginn
bool running = true;

pthread_t hunters[HUNTERS];
pthread_t collector;

// Signal-Handler Funktion für SIGINT
void handle_sigint(int sig) {
    printf("\n[Signal] SIGINT empfangen, beende die Simulation...\n");
    running = false; // Setze das Flag auf false, um alle Threads zu beenden
}

// Funktion zum Sammeln von Kieselsteinen und Umwandeln in Münzen
void* collect_stones(void* arg) {
    while (running) {
        sleep(STONE_COLLECTION_TIME); // Sammle Kieselsteine für eine gewisse Zeit
        pthread_mutex_lock(&wallet_lock); // Sperre die Geldbörse
        if (coins == 0 && running) { // Nur Münzen erzeugen, wenn der Händler läuft
            coins = rand() % (WALLET_CAPACITY + 1); // Zufällige Anzahl Münzen bis zur Kapazität
            printf("[Händler] Verwandelt Kieselsteine in %d Münzen und wirft sie aus!\n", coins);
        }
        pthread_mutex_unlock(&wallet_lock); // Entsperre die Geldbörse
    }
    return NULL;
}

// Thread-Funktion für jeden Jäger, um Münzen zu sammeln
void* hunter_thread(void* arg) {
    int hunter_id = *((int*)arg);
    int collected_coins = 0;
    free(arg);

    while (running) {
        bool collected = false;
        pthread_mutex_lock(&wallet_lock);
        if (coins > 0) {
            coins--; // Nimm eine Münze aus der Börse
            collected = true;
            printf("[Jäger %d] Eine Münze gesammelt. Verbleibende Münzen: %d\n", hunter_id, coins);
        }
        pthread_mutex_unlock(&wallet_lock);
        if (collected) {
            collected_coins++;
        }
        usleep((rand() % 400 + 100) * 1000); // Zufällige Verzögerung
    }

    // Rückgabe der gesammelten Münzen an den Hauptthread
    int* result = malloc(sizeof(int));
    if (result == NULL) {
        perror("Fehler bei der Speicherzuweisung");
        pthread_exit(NULL);
    }
    *result = collected_coins;
    pthread_exit(result);
}

// Funktion zum Stoppen aller Threads bei SIGINT
void graceful_shutdown() {
    printf("[Main] Beende die Simulation und warte auf alle Threads...\n");
    running = false;

    // Warten auf das Ende der Jäger-Threads und Ausgabe der gesammelten Münzen
    for (int i = 0; i < HUNTERS; i++) {
        void* retval;
        pthread_join(hunters[i], &retval);
        if (retval != NULL) {
            int collected_coins = *((int*)retval);
            free(retval);
            printf("Jäger %d hat %d Münzen gesammelt.\n", i, collected_coins);
        } else {
            printf("Jäger %d hat keine Münzen gesammelt.\n", i);
        }
    }

    pthread_join(collector, NULL); // Warten auf das Ende des Sammel-Threads
    pthread_mutex_destroy(&wallet_lock);
    printf("[Main] Programm beendet.\n");
}

int main() {
    srand(time(NULL)); // Initialisiere den Zufallszahlengenerator
    pthread_mutex_init(&wallet_lock, NULL);

    // Zufällige Initialisierung der Münzen in der Geldbörse
    coins = rand() % (WALLET_CAPACITY + 1);
    printf("[Main] Anfangsanzahl der Münzen in der Geldbörse: %d\n", coins);

    signal(SIGINT, handle_sigint);

    // Starte den Sammel-Thread (Händler), um Kieselsteine zu sammeln und Münzen zu werfen
    pthread_create(&collector, NULL, collect_stones, NULL);

    // Starte die Jäger-Threads
    for (int i = 0; i < HUNTERS; i++) {
        int* hunter_id = malloc(sizeof(int));
        if (hunter_id == NULL) {
            perror("Fehler bei der Speicherzuweisung");
            exit(EXIT_FAILURE);
        }
        *hunter_id = i;
        pthread_create(&hunters[i], NULL, hunter_thread, hunter_id);
    }

    sleep(SIMULATION_TIME); // Simulation läuft für 5 Minuten
    graceful_shutdown(); // Beende die Simulation sauber und warte auf alle Threads

    return 0;
}
