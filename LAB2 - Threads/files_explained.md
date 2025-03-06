# Reading and Writing Matrix Files in C

This guide explains how to read and write matrix data from/to a file in the following format:
```
row=10 col=5
1	2	3	4	5
6	7	8	9	10
... (remaining rows)
```

---

## Matrix File Format
- **First Line:** Header with `row=<ROW_COUNT> col=<COL_COUNT>`
- **Subsequent Lines:** Matrix values separated by **tabs** (`\t`)
- Example:
  ```
  row=3 col=2
  1	2
  3	4
  5	6
  ```

---

## Reading a Matrix from a File

### Steps:
1. **Open the File**
2. **Read the Header** to get row/column counts
3. **Allocate Memory** for the matrix
4. **Read Data** into the matrix

### Code Example
```c
#include <stdio.h>
#include <stdlib.h>

int** read_matrix(const char* filename, int* rows, int* cols) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    // Read header
    if (fscanf(file, "row=%d col=%d\n", rows, cols) != 2) {
        fprintf(stderr, "Invalid header format\n");
        fclose(file);
        return NULL;
    }

    // Allocate memory
    int** matrix = (int**)malloc(*rows * sizeof(int*));
    for (int i = 0; i < *rows; i++) {
        matrix[i] = (int*)malloc(*cols * sizeof(int));
    }

    // Read data
    for (int i = 0; i < *rows; i++) {
        for (int j = 0; j < *cols; j++) {
            if (fscanf(file, "%d", &matrix[i][j]) != 1) {
                fprintf(stderr, "Error reading data at row=%d, col=%d\n", i, j);
                fclose(file);
                return NULL;
            }
        }
    }

    fclose(file);
    return matrix;
}
```

---

## Writing a Matrix to a File

### Steps:
1. **Open the File**
2. **Write the Header** with row/column counts
3. **Write Data** with tab-separated values

### Code Example
```c
void write_matrix(const char* filename, int** matrix, int rows, int cols) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Failed to create file");
        return;
    }

    // Write header
    fprintf(file, "row=%d col=%d\n", rows, cols);

    // Write data
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(file, "%d", matrix[i][j]);
            if (j < cols - 1) {
                fprintf(file, "\t");  // Add tab between values
            }
        }
        fprintf(file, "\n");  // Newline after each row
    }

    fclose(file);
}
```

---

## Usage Example

```c
int main() {
    // Read matrix
    int rows, cols;
    int** matrix = read_matrix("input.txt", &rows, &cols);
    
    // Process matrix (example: print)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }

    // Write matrix
    write_matrix("output.txt", matrix, rows, cols);

    // Free memory
    for (int i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);

    return 0;
}
```

---

## Key Considerations
1. **Error Handling:** 
   - Check for file open/read/write failures.
   - Validate the number of values per row matches the column count.

2. **Memory Management:**
   - Always free allocated memory after use.
   - Use `calloc` instead of `malloc` if zero-initialization is needed.

3. **Formatting:**
   - Ensure tabs (`\t`) are used as delimiters (not spaces).
   - Avoid trailing tabs/newlines in the file.

4. **Edge Cases:**
   - Handle empty files or files with incomplete data.
   - Validate row/column counts are positive integers.

---

## File Format Notes
- The first line **must** follow `row=<N> col=<M>` exactly.
- Values can be positive/negative integers.
- Extra whitespace characters are allowed but not recommended.