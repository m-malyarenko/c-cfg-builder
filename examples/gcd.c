int gcd(int a, int b) {
    int r = 0;

    while ((a % b) > 0) {
        r = a % b;
        a = b;
        b = r;
    }

    return b;
}
