// sorting.mc — bubble sort on a fixed array

void bubble_sort(int arr[10], int n) {
    int i = 0;
    while (i < n - 1) {
        int j = 0;
        while (j < n - i - 1) {
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
            j++;
        }
        i++;
    }
}

void main() {
    int arr[10];
    arr[0] = 64;
    arr[1] = 34;
    arr[2] = 25;
    arr[3] = 12;
    arr[4] = 22;
    arr[5] = 11;
    arr[6] = 90;
    arr[7] = 45;
    arr[8] = 3;
    arr[9] = 77;

    bubble_sort(arr, 10);

    print("Sorted array:");
    int i = 0;
    while (i < 10) {
        print(arr[i]);
        i++;
    }
}
