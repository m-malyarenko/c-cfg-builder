void switch_func(int x) {
    switch (x) {
        case 34 + 67:
            x = 9;
            x -= 8;
            x = 89;
            break;
        default:
            return;
        case 8: {
            x = 9;
            x -= 8;
            x = 902;
        }
        case 89: {
            x = x + 8;
            x = 90;
            break;
        }
    }
}
