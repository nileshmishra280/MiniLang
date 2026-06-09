// fibonacci.mc — recursive Fibonacci sequence
int fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

void main() {
    print("Fibonacci sequence:");
    int i = 0;
    while (i <= 10) {
        print(fib(i));
        i++;
    }
}
