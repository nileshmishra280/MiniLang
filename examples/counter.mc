// counter.mc — demonstrates global variables and multiple functions

int count = 0;
int total = 0;

void add(int n) {
    total = total + n;
    count = count + 1;
}

int average() {
    if (count == 0) { return 0; }
    return total / count;
}

void main() {
    add(10);
    add(20);
    add(30);
    add(40);
    add(50);

    print("Count:");
    print(count);
    print("Total:");
    print(total);
    print("Average:");
    print(average());
}
