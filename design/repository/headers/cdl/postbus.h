constant integer c_postbus_width=32;
typedef enum [2]
{
    postbus_word_type_start = 0,
    postbus_word_type_idle = 1,
    postbus_word_type_hold = 1,
    postbus_word_type_data = 2,
    postbus_word_type_last = 3,
} t_postbus_type;
typedef bit[c_postbus_width] t_postbus_data;
typedef bit[2] t_postbus_ack;

