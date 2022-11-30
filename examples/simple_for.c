int simple_for(int count) {
    int x = 0;
    char y = x * 6;

    for (int i = 0; i < count; i += 2) {
        x = x + 3;

        if (x == 7)
            for (int j = 8; j < count; j++)
                x = x * 3;
        else {
            if (x < 56) {
                x = x * 2;
                y -= 9;
                x += 90;
            } else {
                return x + y;
            }
        }
    }

    x = 78;
    x = 90;

    for (int i = 0; i < count; i++)
        x = 12 * 3;

    return x;
}