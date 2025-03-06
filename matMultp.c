/*
 * Compilation:
 *      gcc -pthread -o matMultp matMultp.c
 *
 * Execution:
 *      ./matMultp a b c
 */

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

// Timer functions (used #define for simplicity and reuse)
struct timeval start, stop;
#define START_TIMER gettimeofday(&start, NULL)
#define STOP_TIMER gettimeofday(&stop, NULL)

// Global variables
// Directories and files names (change these if files are in different dir)
char file_A[128] = "a.txt";
char file_B[128] = "b.txt";
char file_C_method_1[128] = "c_per_matrix.txt";
char file_C_method_2[128] = "c_per_row.txt";
char file_C_method_3[128] = "c_per_element.txt";

// Global pointers to matrices and thier sizes
int *matA = NULL;
int *matB = NULL;
int *matC_method_1 = NULL; // Result matrix from method 1 (per matrix)
int *matC_method_2 = NULL; // Result matrix from method 2 (per row)
int *matC_method_3 = NULL; // Result matrix from method 3 (per element)

int A_rows, A_cols;
int B_rows, B_cols;
int C_rows, C_cols; // For valid matrix multiplication, C_rows = A_rows and C_cols = B_cols

/*
int* is used instead of int** because:
    1) it simplifies memory management since only one call to malloc is needed per matrix.
    2) it works well with threading and avoids additional pointer indirections.
*/

// A struct to pass the parameters of the element in method 3
typedef struct
{
    int row;
    int col;
} Element_Params;

// Functions
int *read_mat_file(const char *file_name, int *rows, int *cols);
void write_mat_file(const char *file_name, int *matrix, int rows, int cols);
void *mult_matrix();           // Method 1  (per matrix)
void *mult_row(void *arg);     // Method 2 (per row)
void *mult_element(void *arg); // Method 3 (per element)

////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////============== MAIN ==============///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

/*
Main function:
    1) Parses input or uses the default dir
    2) reads the text files and saves the data in our global variables
    3) checks for mult condition (A_cols == B_rows)
    4) performs each mult method with different number of threads and a timer
    5) compares the time taken by each method
    6) writes the output of each method to different files (according to method)

*/
int main(int argc, char *argv[])
{
    // argc: counts how many arguments were passed.
    // argv: stores the arguments as strings.

    // 1) Parses input or uses the default dir
    if (argc == 4) // if custom input added to terminal
    {
        /* snprintf: is a safe version of sprintf,
        used to format a string and store it in a character array
        while preventing buffer overflows. */

        snprintf(file_A, sizeof(file_A), "%s.txt", argv[1]);
        snprintf(file_B, sizeof(file_B), "%s.txt", argv[2]);

        snprintf(file_C_method_1, sizeof(file_C_method_1), "%s_per_matrix.txt", argv[3]);
        snprintf(file_C_method_2, sizeof(file_C_method_2), "%s_per_row.txt", argv[3]);
        snprintf(file_C_method_3, sizeof(file_C_method_3), "%s_per_element.txt", argv[3]);
    } // else use default files names

    // 2) reads the text files and saves the data in our global variables
    matA = read_mat_file(file_A, &A_rows, &A_cols);
    if (matA == NULL)
    {
        // Printing to stderr ensures error messages are immediately displayed and not buffered. (unlike stdout)
        fprintf(stderr, "Error reading matrix A from file %s\n", file_A);
        exit(EXIT_FAILURE); // Terminates the program immediately.
    }
    matB = read_mat_file(file_B, &B_rows, &B_cols);
    if (matB == NULL)
    {
        fprintf(stderr, "Error reading matrix B from file %s\n", file_B);
        exit(EXIT_FAILURE);
    }

    // 3) checks for mult condition (A_cols == B_rows)
    if (A_cols != B_rows)
    {
        fprintf(stderr, "Matrix multiplication not possible: A_cols (%d) != B_rows (%d)\n", A_cols, B_rows);
        free(matA);
        free(matB);
        exit(EXIT_FAILURE);
    }

    // Set dimensions for the resulting matrix C
    C_rows = A_rows;
    C_cols = B_cols;
    // Alloc memory for the resulting matrices
    matC_method_1 = (int *)malloc(C_rows * C_cols * sizeof(int));
    matC_method_2 = (int *)malloc(C_rows * C_cols * sizeof(int));
    matC_method_3 = (int *)malloc(C_rows * C_cols * sizeof(int));
    if (!matC_method_1 || !matC_method_2 || !matC_method_3)
    {
        fprintf(stderr, "Memory allocation failed for result matrices.\n");
        free(matA);
        free(matB);
        exit(EXIT_FAILURE);
    }

    // 4) performs each mult method with different number of threads and a timer
    // Struct to save the time of execution of each thread
    typedef struct
    {
        long seconds;
        long msecods;  // milliseconds
        long useconds; // microseconds
    } Timer;

    /********** 4.1) METHOD 1: A Thread Per Matrix **********/
    // Create an ID for the thread
    pthread_t tID;

    // Start timer for method 1
    START_TIMER;

    // Create a single thread for the whole matrix mult operation
    if (pthread_create(&tID, NULL, mult_matrix, NULL) != 0) // Returns 0 on success
    {
        fprintf(stderr, "Error creating thread for method 1.\n");
        exit(EXIT_FAILURE);
    }
    // Wait for the thread to finish
    if (pthread_join(tID, NULL) != 0) // Returns 0 on success
    {
        fprintf(stderr, "Error waiting for the termination of thread for method 1.\n");
        exit(EXIT_FAILURE);
    }

    // Stop the timer for method 1
    STOP_TIMER;

    // Get the time taken to execute the thread
    Timer timer_1;
    timer_1.seconds = stop.tv_sec - start.tv_sec;
    timer_1.useconds = stop.tv_usec - start.tv_usec;

    if (timer_1.useconds < 0)
    {
        timer_1.seconds--;
        timer_1.useconds += 1000000;
    }

    timer_1.msecods = timer_1.useconds / 1000;
    timer_1.useconds = timer_1.useconds % 1000;

    /********** 4.2) METHOD 2: A Thread Per Row **********/
    // Create an array of IDs, so that each thread has its own unique ID
    pthread_t *tID_rows = (pthread_t *)malloc(A_rows * sizeof(pthread_t));
    if (!tID_rows)
    {
        fprintf(stderr, "Memory allocation failed for row threads.\n");
        exit(EXIT_FAILURE);
    }

    // Start timer for method 2
    START_TIMER;

    // Loop through each row creating a thread for each
    for (int i = 0; i < A_rows; i++)
    {
        // Each thread gets a unique separate copy of i
        int *row_index = (int *)malloc(sizeof(int));
        if (!row_index)
        {
            fprintf(stderr, "Memory allocation failed for row parameter.\n");
            exit(EXIT_FAILURE);
        }
        *row_index = i;

        // Create a seperate thread for each row and send the "row_index" parameter to the function
        if (pthread_create(&tID_rows[i], NULL, mult_row, row_index) != 0) // Returns 0 on success
        {
            fprintf(stderr, "Error creating thread for row %d in method 2.\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to finish and join them
    for (int i = 0; i < A_rows; i++)
    {
        if (pthread_join(tID_rows[i], NULL) != 0) // Returns 0 on success
        {
            fprintf(stderr, "Error waiting for the termination of thread %d for method 2.\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Stop the timer for method 2
    STOP_TIMER;

    // Get the time taken to execute the thread
    Timer timer_2;
    timer_2.seconds = stop.tv_sec - start.tv_sec;
    timer_2.useconds = stop.tv_usec - start.tv_usec;

    if (timer_2.useconds < 0)
    {
        timer_2.seconds--;
        timer_2.useconds += 1000000;
    }

    timer_2.msecods = timer_2.useconds / 1000;
    timer_2.useconds = timer_2.useconds % 1000;

    // Free ID array
    free(tID_rows);

    /********** 4.3) METHOD 3: A Thread Per Element **********/
    int totalThreads = C_rows * C_cols;

    // Create an ID array
    pthread_t *tID_elements = (pthread_t *)malloc(totalThreads * sizeof(pthread_t));
    if (!tID_elements)
    {
        fprintf(stderr, "Memory allocation failed for element threads.\n");
        exit(EXIT_FAILURE);
    }

    // Start timer for method 3
    START_TIMER;

    // Create a seperate thread for each element
    int thread_index = 0;

    for (int i = 0; i < A_rows; i++)
    {
        for (int j = 0; j < B_cols; j++)
        {

            // Setup the parameters struct that will be passed to the method 3 function
            Element_Params *param = (Element_Params *)malloc(sizeof(Element_Params));
            if (!param)
            {
                fprintf(stderr, "Memory allocation failed for element parameter.\n");
                exit(EXIT_FAILURE);
            }

            param->row = i;
            param->col = j;

            // Create thread and pass params
            if (pthread_create(&tID_elements[thread_index++], NULL, mult_element, param) != 0)
            {
                fprintf(stderr, "Error creating thread for element (%d, %d).\n", i, j);
                exit(EXIT_FAILURE);
            }
        }
    }

    // Join all element threads
    for (int i = 0; i < totalThreads; i++)
    {
        if (pthread_join(tID_elements[i], NULL) != 0)
        {
            fprintf(stderr, "Error waiting for the termination of thread %d for method 3.\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Stop the timer for method 3
    STOP_TIMER;

    // Get the time taken to execute the thread
    Timer timer_3;
    timer_3.seconds = stop.tv_sec - start.tv_sec;
    timer_3.useconds = stop.tv_usec - start.tv_usec;

    if (timer_3.useconds < 0)
    {
        timer_3.seconds--;
        timer_3.useconds += 1000000;
    }

    timer_3.msecods = timer_3.useconds / 1000;
    timer_3.useconds = timer_3.useconds % 1000;

    // Free ID array
    free(tID_elements);

    // 5) compares the time taken by each method
    printf("=== Method 1: A Thread Per Matrix ===\n");
    printf("Method 1: Threads created: 1\n");
    printf("Method 1: Execution time: %ld seconds, %ld milliseconds, %ld microseconds\n\n", timer_1.seconds, timer_1.msecods, timer_1.useconds);

    printf("=== Method 2: A Thread Per Row ===\n");
    printf("Method 2: Threads created: %d\n", A_rows);
    printf("Method 2: Execution time: %ld seconds, %ld milliseconds, %ld microseconds\n\n", timer_2.seconds, timer_2.msecods, timer_2.useconds);

    printf("=== Method 3: A Thread Per Element ===\n");
    printf("Method 3: Threads created: %d\n", totalThreads);
    printf("Method 3: Execution time: %ld seconds, %ld milliseconds, %ld microseconds\n\n", timer_3.seconds, timer_3.msecods, timer_3.useconds);

    // 6) writes the output of each method to different files (according to method)
    write_mat_file(file_C_method_1, matC_method_1, C_rows, C_cols);
    write_mat_file(file_C_method_2, matC_method_2, C_rows, C_cols);
    write_mat_file(file_C_method_3, matC_method_3, C_rows, C_cols);

    // Free all dynamically allocated memory
    free(matA);
    free(matB);
    free(matC_method_1);
    free(matC_method_2);
    free(matC_method_3);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////============ FUNCTIONS ============//////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

/* Function: read_mat_file
 * ---------------------
 * Reads a matrix from a text file. The first line of the file should be of the format:
 *    row=<number> col=<number>
 * The other lines contain the matrix elements separated by spaces.
 *
 * Parameters:
 *   filename - the name of the input file.
 *   rows     - pointer to an integer to store the number of rows.
 *   cols     - pointer to an integer to store the number of columns.
 *
 * Returns:
 *   A pointer to the dynamically allocated array containing the matrix data or NULL if an error occurs.
 */
int *read_mat_file(const char *file_name, int *rows, int *cols)
{
    FILE *fp = fopen(file_name, "r");

    // Incase of error
    if (fp == NULL)
    {
        fprintf(stderr, "Cannot open file: %s\n", file_name);
        return NULL;
    }

    // Read the first line of the file as row=<number> col=<number>
    if (fscanf(fp, "row=%d col=%d", rows, cols) != 2)
    {
        fprintf(stderr, "Invalid marix dimensions or wrong format in file: %s\n", file_name);
        fclose(fp);
        return NULL;
    }

    // Allocate memory for the matrix
    int *matrix = (int *)malloc((*rows) * (*cols) * sizeof(int));
    if (!matrix)
    {
        fprintf(stderr, "Memory allocation failed for matrix in file: %s\n", file_name);
        fclose(fp);
        return NULL;
    }

    // Read each element from the file (don't forget that the matrix is not saved as 2D only int*)
    for (int i = 0; i < (*rows) * (*cols); i++)
    {
        if (fscanf(fp, "%d", &matrix[i]) != 1)
        {
            fprintf(stderr, "Error reading matrix data in file: %s\n", file_name);
            free(matrix);
            fclose(fp);
            return NULL;
        }
    }

    // If no return, then the matrix is saved successfully
    fclose(fp);
    return matrix;
}

/* Function: write_mat_file
 * ----------------------
 * Writes a matrix to a text file in the following format:
 *   row=<number> col=<number>
 *   <matrix elements with rows on separate lines and elements separated by spaces>
 *
 * Parameters:
 *   filename - the name of the output file.
 *   matrix   - pointer to the matrix data.
 *   rows     - number of rows in the matrix.
 *   cols     - number of columns in the matrix.
 */
void write_mat_file(const char *file_name, int *matrix, int rows, int cols)
{
    FILE *fp = fopen(file_name, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Cannot open file for writing: %s\n", file_name);
        return;
    }

    // Write the matrix dimensions as the first line
    fprintf(fp, "row=%d col=%d\n", rows, cols);

    // Write the matrix data row by row
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            // matrix[i * cols + j] : because it's stored as 1D array (int* not int**)
            fprintf(fp, "%d ", matrix[i * cols + j]);
        }
        // New line after each row
        fprintf(fp, "\n");
    }
    fclose(fp);
}

/* Function: mult_matrix (METHOD 1)
 * ----------------------
 * Computes the product of two matrices (matA and matB) and stores the result in matC_method_1.
 * Matrices are stored in a 1D array (row-major order).
 *
 * Global variables used:
 *   - A_rows: Number of rows in matrix A.
 *   - A_cols: Number of columns in matrix A (also rows in B).
 *   - B_cols: Number of columns in matrix B.
 *   - matA, matB: Input matrices.
 *   - matC_method_1: Output matrix storing the result.
 *
 * Returns:
 *   NULL (since it is meant to be used as a thread function, though it does not spawn threads itself).
 */
void *mult_matrix()
{
    for (int i = 0; i < A_rows; i++)
    {
        for (int j = 0; j < B_cols; j++)
        {

            int sum = 0;
            for (int k = 0; k < A_cols; k++) // beacuse it's stored as 1D
            {
                sum += matA[i * A_cols + k] * matB[j + k * B_cols];
            }

            matC_method_1[i * C_cols + j] = sum;
        }
    }
    return NULL;
}

/* Function: mult_row (METHOD 2)
 * ----------------------
 * Computes a single row of the matrix multiplication result.
 *
 * Parameters:
 *   - arg: A pointer to an integer containing the row index to compute.
 *
 * Behavior:
 *   - The function extracts the row index from the argument.
 *   - It computes the result for the given row and stores it in matC_method_2.
 *   - The dynamically allocated argument is freed before returning.
 *
 * Returns:
 *   NULL (indicates the thread has finished execution).
 */
void *mult_row(void *arg)
{
    // (int*) :to cast the void pointer to an int pointer
    // * :to derefernce the arg to get data that the pointer points to
    int row = *(int *)arg;

    // Free the data in arg
    free(arg);

    // Same code as method one minus the outter rows loop to be just the passed row
    for (int j = 0; j < B_cols; j++)
    {

        int sum = 0;
        for (int k = 0; k < A_cols; k++) // beacuse it's stored as 1D
        {
            sum += matA[row * A_cols + k] * matB[j + k * B_cols];
        }

        matC_method_2[row * C_cols + j] = sum;
    }

    return NULL;
}

/* Function: mult_element (METHOD 3)
 * ----------------------
 * Computes a single element of the matrix multiplication result.
 *
 * Parameters:
 *   - arg: A pointer to an Element_Params struct containing the row and column indices.
 *
 * Behavior:
 *   - The function extracts the row and column indices from the argument.
 *   - It computes the result for the given element and stores it in matC_method_3.
 *   - The dynamically allocated struct is freed before returning.
 *
 * Returns:
 *   NULL (indicates the thread has finished execution).
 */
void *mult_element(void *arg)
{
    // get the data (row & col) from the passed struct
    Element_Params *params = (Element_Params *)arg;
    int row = params->row;
    int col = params->col;
    // Then free this struct
    free(params);

    // Same code as method 2 minus the outter col loop to be just the row and col passed
    int sum = 0;
    for (int k = 0; k < A_cols; k++) // beacuse it's stored as 1D
    {
        sum += matA[row * A_cols + k] * matB[col + k * B_cols];
    }

    matC_method_3[row * C_cols + col] = sum;
    return NULL;
}