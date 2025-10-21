typedef enum grbl_action {
    MOVE_X,
    MOVE_Y,
    MOVE_Z,
    ZERO_Z,
    ZERO_XY,
    HOME,
    PROBE_Z,
    PROBE_XY,
    INVALID, /* leave at end */
} grbl_action;

typedef struct command {
    grbl_action action;
    int16_t arg1;
} grbl_cmd;

/*
 * Init functions which starts a handler thread on core-1
 */
int grbl_init(void);


/* Queue a command */
int grbl_queue_command(grbl_cmd *command);

/* Convienence command for ISR to directly queue a commmand when it doesn't know the queue */
int grbl_quick_command(grbl_action action, int16_t arg1);

/* Return string for given command enum */
const char *action_name(grbl_action action);
