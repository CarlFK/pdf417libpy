#define OUTPUT_GIF  1
#define OUTPUT_PBM  2
#define OUTPUT_PS   3
#define OUTPUT_RAW  4
#define OUTPUT_PS_BITS 5

extern void pdf417_init_arrays ();
extern void pd417_set_raw_line (int linenum, char *instr);
extern void pdf417_set_raw_count (int countin);
extern void pdf417_set_input_filename (char *instr);
extern void pdf417_set_output_filename (char *instr);
extern void pdf417_en (int in_rval, int in_cval, int in_ec_level,
                       int output_type, int use_default, int raw_in_mode);
extern void pdf417_en_dim (int in_rval, int in_cval, int in_ec_level,
                       int output_type, int use_default, int raw_in_mode,
                       int X, int Y, int QZ );
extern void pdf417_en_new (int in_rval, int in_cval, int in_ec_level,
                       int output_type, int use_default, int raw_in_mode,
                       int X, int Y, int QZ );
//extern void pdf417_prep_to_raw(const char* buffer, int buffer_len);
