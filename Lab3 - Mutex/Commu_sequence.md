# CalTrain Synchronization Communication Sequence

This document explains how the various functions in the CalTrain synchronization task work together to coordinate boarding between passenger threads and the train thread. The key functions involved are:

- **`station_init`**
- **`station_wait_for_train`**
- **`station_on_board`**
- **`station_load_train`**

The implementation uses Pthreads (mutex and condition variables) to safely coordinate the shared state among multiple threads.

---

## 1. Station Initialization (`station_init`)

When the station is initialized:
- **Counters are set to zero:**
  - `waiting`: Number of passengers currently waiting.
  - `seats_available`: Free seats available on an arriving train.
  - `people_to_sit`: The computed number of passengers that are allowed to board (minimum of waiting passengers and available seats).
  - `boarding`: Number of passengers who have already boarded.
- **Synchronization primitives are initialized:**
  - `lock`: A mutex used to protect access to the shared state.
  - `train_arrived`: A condition variable used to signal passengers that a train has arrived.
  - `all_boarded`: A condition variable used to signal the train that all expected passengers have boarded.

*This sets the starting point for all threads to safely modify and check the shared state.*

---

## 2. Passenger Arrival and Waiting (`station_wait_for_train`)

When a passenger arrives at the station, it calls `station_wait_for_train`:

1. **Increment the Waiting Counter:**
   - The passenger locks the mutex.
   - The `waiting` counter is incremented to indicate a new passenger is waiting.

2. **Wait for a Train:**
   - The passenger enters a loop that waits until there is at least one available seat (i.e., `seats_available > 0`).
   - Inside this loop, the passenger calls `pthread_cond_wait` on the `train_arrived` condition variable. This releases the mutex and suspends the thread until it is signaled.

3. **Reserve a Seat:**
   - When a train arrives and signals `train_arrived`, the waiting passenger wakes up.
   - The passenger then decrements `seats_available` (reserving the seat) and decrements `waiting` (since it is no longer waiting).

4. **Release the Lock:**
   - The mutex is unlocked and the function returns.  
   *At this point, the passenger is ready to board the train.*

---

## 3. Passenger Boarding (`station_on_board`)

After a passenger has been allowed to board (by leaving `station_wait_for_train`), it then calls `station_on_board`:

1. **Update the Boarding Count:**
   - The function locks the mutex and increments the `boarding` counter to indicate that a passenger has boarded.

2. **Signal Completion if Boarding is Complete:**
   - It checks if the `boarding` count has reached `people_to_sit` (the number of passengers that were allowed to board).
   - If yes, it broadcasts on the `all_boarded` condition variable to wake up the train thread waiting in `station_load_train`.

3. **Unlock the Mutex:**
   - The mutex is unlocked and the function returns.

*This informs the train that one more passenger has taken a seat and, if all expected passengers have boarded, that boarding is complete.*

---

## 4. Train Arrival and Loading (`station_load_train`)

When a train arrives, it calls `station_load_train` with the number of free seats (`count`):

1. **Set Available Seats:**
   - The train locks the mutex.
   - It sets `seats_available` to the provided count, representing the number of seats available on this train.

2. **Determine How Many Passengers Can Board:**
   - The function computes `people_to_sit` as the minimum of the `waiting` count and `count` (available seats).
   - This ensures that if fewer passengers are waiting than the available seats, only that many will board.

3. **Notify Waiting Passengers:**
   - It uses `pthread_cond_broadcast` on `train_arrived` to signal all waiting passengers that a train is available.
   - This wakes up all threads waiting in `station_wait_for_train` so they can check the condition and reserve a seat.

4. **Wait for All Expected Passengers to Board:**
   - The train then enters a loop waiting on the `all_boarded` condition variable until `boarding` equals `people_to_sit`.
   - This ensures the train does not depart until every passenger who can board has done so.

5. **Reset State for the Next Train:**
   - Once boarding is complete, the function resets `boarding`, `seats_available`, and `people_to_sit` to 0.
   - This prepares the station state for the next arriving train.

6. **Unlock the Mutex and Return:**
   - Finally, the mutex is unlocked and the function returns, allowing the train to depart.

---

## 5. Communication Flow Summary

Below is an overview of the sequence of events and the communication between threads:

1. **Passenger Arrival:**
   - A passenger thread calls `station_wait_for_train`:
     - Increments `waiting`.
     - Waits for `seats_available > 0`.

2. **Train Arrival:**
   - A train thread calls `station_load_train(count)`:
     - Sets `seats_available = count`.
     - Calculates `people_to_sit = min(waiting, count)`.
     - Broadcasts `train_arrived` to wake waiting passengers.

3. **Seat Reservation by Passengers:**
   - Woken passenger threads in `station_wait_for_train` reserve a seat:
     - Decrement `seats_available` and `waiting`.
     - Proceed to board the train.

4. **Passenger Boards and Signals:**
   - After boarding, each passenger calls `station_on_board`:
     - Increments `boarding`.
     - When `boarding == people_to_sit`, broadcasts `all_boarded` to notify the train.

5. **Train Waits and Departs:**
   - The train thread in `station_load_train` waits until `boarding` equals `people_to_sit`.
   - Once all expected passengers have boarded, it resets the counters and departs.

---

## 6. Key Synchronization Concepts

- **Mutex (`lock`):**  
  Ensures that shared state (counters) is accessed safely by multiple threads.

- **Condition Variables (`train_arrived` and `all_boarded`):**  
  Allow threads to sleep until a particular condition is met (e.g., a train arrives or all passengers have boarded), avoiding busy-waiting.

- **Counters:**  
  Keep track of:
  - **Waiting Passengers (`waiting`):** How many passengers are queued.
  - **Available Seats (`seats_available`):** Seats available on the train.
  - **Expected Boarders (`people_to_sit`):** How many passengers should board.
  - **Actual Boarders (`boarding`):** How many passengers have boarded so far.

This coordination guarantees that:
- Passengers only board when a train with available seats is present.
- The train waits until the correct number of passengers have boarded before leaving.
- The shared state is reset correctly for subsequent trains.

---

This detailed sequence provides a comprehensive overview of the inter-thread communication in the CalTrain synchronization task. Use this document as a reference when testing and debugging your code.
