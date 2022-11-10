int gcd(int a, int b) {
    int r;

    while ((a % b) > 0) {
        r = a % b;
        a = b;
        b = r;
    }

    return b;
}
