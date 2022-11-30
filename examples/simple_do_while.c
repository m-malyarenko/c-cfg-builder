int simple_do_while(int count) {
    int x = 0;
    int i = 0;

    do {
        x = x + 3;

        if (i == 35) {
            x = x + 7;
        } else {
            x = x * 3;
        }

        i++;
    } while (i < count);

    return x + i;
}