#include <stdio.h>

#define LENGTH 6

#define swap(a, b) do {\
    a = a + b;\
    b = a - b;\
    a = a - b;\
} while(0);

/**
 * 每次求得i位置上的应该是多少, 其他不考虑
 */
void bubble_a(int *arr)
{
    int i, j, tmp;
    int counter = 0;

    for (i = 0; i < LENGTH - 1; i++) {
        for (j = i + 1; j < LENGTH; j++) {
            counter++;
            if (arr[i] > arr[j]) {
                swap(arr[i], arr[j]);
            }
        }
    }
    for (tmp = 0; tmp < LENGTH; tmp++) {
        printf("%d", arr[tmp]);
    }
    printf("\nMETHDO A DONE: %d\n", counter);
}

void bubble_b(int *arr)
{
    int i, j, tmp;
    int counter = 0;

    for (i = 0; i < LENGTH; i++) {
        for (tmp = 0; tmp < LENGTH; tmp++) {
            printf("%d", arr[tmp]);
        }
        puts("");
        for (j = 0; j < LENGTH - i - 1; j++) {
            counter++;
            if (arr[j] > arr[j + 1]) {
                swap(arr[j], arr[j + 1]);
            }
        }
    }
    for (tmp = 0; tmp < LENGTH; tmp++) {
        printf("%d", arr[tmp]);
    }
    printf("\nMETHDO B DONE: %d\n", counter);
}

int main()
{
    int arra[6] = {6, 2, 5, 3, 1, 4};
    int arrb[6] = {6, 2, 5, 3, 1, 4};

    bubble_a(arra);
    bubble_b(arrb);

    return 0;
}
