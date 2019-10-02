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
extern int cathelpfile();
extern void clear_the_screen ();