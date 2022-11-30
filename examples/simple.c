int simple() {
    int x = 0, y = 0;

    if ((x != 0) || (y != 0)) {
        return -1;
    }

    switch (x + 5) {
    case 67: {
        for (short i = 7; i >= 0; i--)
            printf("%i", i);
    } break;
    case 45:
    case 22:
        x = 90 * cos(x);
    default:
        break;
    }

    for (int i = 0; i < 10; i++) {
        int w = x * i;
        for (int j = 0; j < 10; j++) {
            if (w > x) {
                goto EXIT_LOOP;
            }

            w += y;            
        }

        if (i == 10) {
            i -= 10;
        } else {
            goto EXIT_LOOP;
        }

        w = x - 7;
    }

    EXIT_LOOP:

    while (x > 0) {
        y = y * 78;
        if (y > 90) {
            break;
        }
    }

    return x + y; 
}
