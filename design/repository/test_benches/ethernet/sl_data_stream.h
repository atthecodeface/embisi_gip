typedef struct t_sl_data_stream;
extern void sl_data_stream_start_packet( t_sl_data_stream *ds );
extern void sl_data_stream_next_data( t_sl_data_stream *ds );
extern t_sl_data_stream *sl_data_stream_create( char *option_string );
extern int sl_data_stream_packet_length( t_sl_data_stream *ds );
extern unsigned int sl_data_stream_packet_header( t_sl_data_stream *ds );
extern unsigned int sl_data_stream_packet_data( t_sl_data_stream *ds );
