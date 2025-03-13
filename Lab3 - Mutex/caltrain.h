#include <pthread.h>

struct station {
	int waiting;          // Number of waiting passengers
    int seats_available;  // Number of free seats on the arriving train
	int people_to_sit;    // Number of people that can actually sit
    int boarding;         // Number of passengers currently boarding

	pthread_mutex_t lock; // Mutex to lock shared resources
    pthread_cond_t train_arrived; // Cond variable to signal a train has arrived
    pthread_cond_t all_boarded;   // Cond variable to signal that boarding is complete  

};

void station_init(struct station *station);

void station_load_train(struct station *station, int count);

void station_wait_for_train(struct station *station);

void station_on_board(struct station *station);