#include <pthread.h>
#include "caltrain.h"

/*
to complie the code: 
to run the code: ./repeat.sh

*/

void station_init(struct station *station)
{
	// Init counters
	station->waiting = 0;
	station->seats_available = 0;
	station->people_to_sit = 0;
	station->boarding = 0;

	// Init mutex and conds
	pthread_mutex_init(&station->lock, NULL);
	pthread_cond_init(&station->train_arrived, NULL);
	pthread_cond_init(&station->all_boarded, NULL);
}

void station_load_train(struct station *station, int count)
{
	pthread_mutex_lock(&station->lock);

	// Save the number of available seat
	station->seats_available = count;

	/*
	If the number of pe0ple waiting less than the number of available seats then everybody waiting can sit
	else then it's the number of all available seats
	 */
	station->people_to_sit = (station->waiting < count) ? station->waiting : count;

	// Let the people waiting know that a train arrived
	pthread_cond_broadcast(&station->train_arrived);

	// Wait untill everybody who can sit boards
	while (station->boarding < station->people_to_sit)
	{
		pthread_cond_wait(&station->all_boarded, &station->lock);
	}

	// Reset for the next train
	station->boarding = 0;
	station->seats_available = 0;
	station->people_to_sit = 0;

	pthread_mutex_unlock(&station->lock);
}

void station_wait_for_train(struct station *station)
{
	pthread_mutex_lock(&station->lock);

	// Inc the number of waiting passengers
	station->waiting++;

	// Loop until there is an available seat
	while (station->seats_available == 0)
	{
		// Waits on the condition variable until it ges a signal that there is an available seat
		pthread_cond_wait(&station->train_arrived, &station->lock);

		// When a seat is available it exists the loop
	}

	// When the seat is available dec this seat and the number of waiting passengers
	// as one of them has taken the seat
	station->seats_available--;
	station->waiting--;

	pthread_mutex_unlock(&station->lock);
}

void station_on_board(struct station *station)
{
	pthread_mutex_lock(&station->lock);

	// Inc boarding
	station->boarding++;

	// If the number of people boarding equal to the number of people who will sit 
	if (station->boarding == station->people_to_sit)
	{
		// Signal that the train is fully boarded
		pthread_cond_broadcast(&station->all_boarded);
	}

	pthread_mutex_unlock(&station->lock);
}
