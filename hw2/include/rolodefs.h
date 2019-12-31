#define VERSION "2.0"

#define CHUNKSIZE 100000        /* storage size for holding data file */

#define NOLOCKFLAG 'l'
#define SUMMARYFLAG 's'
#define OTHERUSERFLAG 'u'

#define LEGAL_OPTIONS "lsu"
#define USAGE "rolo [ person1 person2 ...] [ -l -s -u user ] "

extern void save_and_exit();
extern void user_eof ();
extern void roloexit();
extern void save_to_disk ();
/*extern void write_rolo();*/
extern void cathelpfile();
extern void clear_the_screen ();
extern int rolo_menu_yes_no();
extern void display_entry();
extern void rolo_insert();
extern void rolo_peruse_mode();
extern void rolo_update_mode();
extern void rolo_delete();
extern void any_char_to_continue ();