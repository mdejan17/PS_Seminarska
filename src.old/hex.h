//  0 - frozen, 0 - by_frozen, 2 - free, 3 - edge 

typedef struct cell{
    double s;
    unsigned char flag;
} cell;

/**
 * used for transitioning new frozen cells
 * 
 * (in an interation one can only write frozen flags to send second (new_array)
 * but the frozen flag data must oslo get to the original array for next iteration)
*/
typedef struct freeze_transition{
    int *i;
    int *j;
    int len;
} freeze_transition;