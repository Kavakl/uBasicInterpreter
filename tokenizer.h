#define NUM_LABEL 50
#define LABEL_LEN 20
#define SUB_NEST 25
#define PROG_SIZE 1000

#define DELIMITER  1
#define VARIABLE  2
#define NUMBER    3
#define COMMAND   4
#define STRING	  5
#define QUOTE	  6
#define MARK      7

#define PRINT 1
#define INPUT 2
#define IF    3
#define THEN  4
#define GOTO  5
#define EOL   6
#define FINISHED  7
#define GOSUB 8
#define RETURN 9
#define END 10


void assignment();
void print(), scan_labels(), find_eol(), goto_statement();
void if_statement(), input();
void gosub(), return_statement(), gpush(char *str);
void start_recursive(), putback();
void level2(), level3(), level4(), level5(), level6(), primitive();
void unary(), arith();
int load_program(char *p, char *fname), look_up(char *s);
int get_next_label(char *s), iswhite(char c), isdelim(char c);
int find_var(char *s), get_token();
char *find_label();