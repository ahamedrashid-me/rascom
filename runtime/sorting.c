// RasCode Sort & Search Library
// Fast sorting and binary search implementations

#include <stdlib.h>
#include <string.h>

// Comparison function for qsort (long values)
static int compare_longs(const void *a, const void *b) {
    long x = *(const long *)a;
    long y = *(const long *)b;
    return (x > y) - (x < y);  // Branchless comparison
}

// Quick sort (in-place)
// Usage: @qsort[arr_ptr, count, element_size] -> sorted_array
long sc_qsort(long arr_ptr, long count, long elem_size) {
    if (!arr_ptr || count <= 1) return arr_ptr;
    
    qsort((void *)arr_ptr, (size_t)count, (size_t)elem_size, compare_longs);
    
    return arr_ptr;
}

// Binary search
// Usage: @bsearch[arr_ptr, count, elem_size, search_val] -> index or -1
long sc_bsearch(long arr_ptr, long count, long elem_size, long search_val) {
    if (!arr_ptr || count <= 0) return -1;
    
    char *arr = (char *)arr_ptr;
    long left = 0;
    long right = count - 1;
    
    while (left <= right) {
        long mid = (left + right) / 2;
        long *mid_val = (long *)(arr + mid * elem_size);
        
        if (*mid_val == search_val) {
            return mid;
        } else if (*mid_val < search_val) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    return -1;
}

// Linear search (for small arrays or unsorted)
// Usage: @search[arr_ptr, count, value] -> index or -1
long sc_search(long arr_ptr, long count, long value) {
    if (!arr_ptr || count <= 0) return -1;
    
    long *arr = (long *)arr_ptr;
    for (long i = 0; i < count; i++) {
        if (arr[i] == value) {
            return i;
        }
    }
    
    return -1;
}

// Fisher-Yates shuffle (in-place)
// Usage: @shuffle[arr_ptr, count] -> shuffled_array
long sc_shuffle(long arr_ptr, long count) {
    if (!arr_ptr || count <= 1) return arr_ptr;
    
    long *arr = (long *)arr_ptr;
    
    for (long i = count - 1; i > 0; i--) {
        // Generate random index 0 to i
        long j = rand() % (i + 1);
        
        // Swap
        long tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
    
    return arr_ptr;
}

// Bubble sort (simple, stable)
// Usage: @bubble_sort[arr_ptr, count] -> sorted_array
long sc_bubble_sort(long arr_ptr, long count) {
    if (!arr_ptr || count <= 1) return arr_ptr;
    
    long *arr = (long *)arr_ptr;
    
    for (long i = 0; i < count - 1; i++) {
        for (long j = 0; j < count - 1 - i; j++) {
            if (arr[j] > arr[j + 1]) {
                long tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
    
    return arr_ptr;
}

// Selection sort (simple)
// Usage: @selection_sort[arr_ptr, count] -> sorted_array
long sc_selection_sort(long arr_ptr, long count) {
    if (!arr_ptr || count <= 1) return arr_ptr;
    
    long *arr = (long *)arr_ptr;
    
    for (long i = 0; i < count - 1; i++) {
        long min_idx = i;
        for (long j = i + 1; j < count; j++) {
            if (arr[j] < arr[min_idx]) {
                min_idx = j;
            }
        }
        
        long tmp = arr[i];
        arr[i] = arr[min_idx];
        arr[min_idx] = tmp;
    }
    
    return arr_ptr;
}

// Insertion sort (stable, good for small arrays)
// Usage: @insertion_sort[arr_ptr, count] -> sorted_array
long sc_insertion_sort(long arr_ptr, long count) {
    if (!arr_ptr || count <= 1) return arr_ptr;
    
    long *arr = (long *)arr_ptr;
    
    for (long i = 1; i < count; i++) {
        long key = arr[i];
        long j = i - 1;
        
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        
        arr[j + 1] = key;
    }
    
    return arr_ptr;
}

// Find minimum value
// Usage: @find_min[arr_ptr, count] -> minimum_value
long sc_find_min(long arr_ptr, long count) {
    if (!arr_ptr || count <= 0) return 0;
    
    long *arr = (long *)arr_ptr;
    long min = arr[0];
    
    for (long i = 1; i < count; i++) {
        if (arr[i] < min) {
            min = arr[i];
        }
    }
    
    return min;
}

// Find maximum value
// Usage: @find_max[arr_ptr, count] -> maximum_value
long sc_find_max(long arr_ptr, long count) {
    if (!arr_ptr || count <= 0) return 0;
    
    long *arr = (long *)arr_ptr;
    long max = arr[0];
    
    for (long i = 1; i < count; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    
    return max;
}

// Find min index
// Usage: @find_min_idx[arr_ptr, count] -> index_of_min
long sc_find_min_idx(long arr_ptr, long count) {
    if (!arr_ptr || count <= 0) return -1;
    
    long *arr = (long *)arr_ptr;
    long min_idx = 0;
    
    for (long i = 1; i < count; i++) {
        if (arr[i] < arr[min_idx]) {
            min_idx = i;
        }
    }
    
    return min_idx;
}

// Find max index
// Usage: @find_max_idx[arr_ptr, count] -> index_of_max
long sc_find_max_idx(long arr_ptr, long count) {
    if (!arr_ptr || count <= 0) return -1;
    
    long *arr = (long *)arr_ptr;
    long max_idx = 0;
    
    for (long i = 1; i < count; i++) {
        if (arr[i] > arr[max_idx]) {
            max_idx = i;
        }
    }
    
    return max_idx;
}

// Count occurrences
// Usage: @count_val[arr_ptr, count, value] -> count
long sc_count_val(long arr_ptr, long count, long value) {
    if (!arr_ptr || count <= 0) return 0;
    
    long *arr = (long *)arr_ptr;
    long cnt = 0;
    
    for (long i = 0; i < count; i++) {
        if (arr[i] == value) cnt++;
    }
    
    return cnt;
}

// Sum array elements
// Usage: @sum[arr_ptr, count] -> sum_of_all_elements
long sc_sum(long arr_ptr, long count) {
    if (!arr_ptr || count <= 0) return 0;
    
    long *arr = (long *)arr_ptr;
    long sum = 0;
    
    for (long i = 0; i < count; i++) {
        sum += arr[i];
    }
    
    return sum;
}

// Average of array elements
// Usage: @average[arr_ptr, count] -> average (integer division)
long sc_average(long arr_ptr, long count) {
    if (!arr_ptr || count <= 0) return 0;
    
    return sc_sum(arr_ptr, count) / count;
}
