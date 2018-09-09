#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include "tokenizer.h"

char *prog;  // Программа 

int variables[26]= {    // 2для переменных 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0
};

struct commands { // Команды
  char command[10];
  char tok;
} table[] = { 
  "print", PRINT, 
  "input", INPUT,
  "if", IF,
  "then", THEN,
  "goto", GOTO,
  "gosub", GOSUB,
  "return", RETURN,
  "end", END,
  "", END  //означает конец table
};

char token[80];
char token_type, tok;

struct label {
  char name[LABEL_LEN];
  char *p;  // иказатель на место в исходном файле
};
struct label label_table[NUM_LABEL];

char *gstack[SUB_NEST];	//для gosub

int gindex;  //вверхний индекс gosub 


int main(int argc, char *args[])
{
  
  char *prog_buf;

  //выделение памяти пол программу
  if(!(prog_buf=(char *) malloc(PROG_SIZE))) {
    printf("Нельзя выделить пямать");
    exit(1);
  }

  if(!args[1] || strcmp(args[1],"test")){
    printf("Неправильный формат командной строки \nФормат командной строки : ./<название скомпилированного файла> <название файла>\n Правильный пример командной строки:./result test\n");
  }else if(!load_program(prog_buf,args[1])) { //загрузка программы из файла test
    printf("невозможно загрузить файл");
    exit(1);
  }

  
 

  prog = prog_buf;
  scan_labels(); // поиск меток в программе

  gindex = 0; 
  do {
    token_type = get_token();
    
    if(token_type==VARIABLE) {
      putback(); //возвращает программу во входной поток
      assignment(); //запись переменной в массив variables
    }
    else // если это команда
      switch(tok) {
        case PRINT:
	        print();
  	        break;
        case GOTO:
	        goto_statement();
	        break;
	    case IF:
	        if_statement();
	        break;
  	    case INPUT:
	        input();
	        break;
        case GOSUB:
	        gosub();
	        break;
	    case RETURN:
	        return_statement();
	        break;
        case END:
	        exit(0);
      }
  } while (tok != FINISHED);
}


int load_program(char *p, char *fname)
{
  FILE *fn;
  int i=0;
  
  if(!(fn=fopen(fname, "rb"))) return 0;

  i = 0;
  do {
    *p = getc(fn);//получение символов из fp
    p++;
    i++;
  } while(!feof(fn) && i<PROG_SIZE);
  *(p-2) = '\0'; //null перкратит выполнение программы
  fclose(fn);
  return 1;
}

//запись переменной в массив variables
void assignment()
{
  int var, value;

  //получаем имя переменной
  get_token();
  if(!isalpha(*token)) {//проверка является она символом или нет
    printf("неправильное название переменной");
  }

  var = toupper(*token)-'A';
  

  //получаем следующий символ после переменной , он должен быть оператором присвоения
  get_token();
  
  if(*token!='=') {
    printf("нехватает = при инициализации");
  }

  //получяем символ после равно
  start_recursive(&value);

  // запись значения 
  variables[var] = value;
}

//печать на экран 
void print()
{
  int answer;
  int len=0, spaces;
  char last_delim;

  do {
    get_token(); //получаем символы после PRINT
    if(tok==EOL || tok==FINISHED) break;
    if(token_type==QUOTE) { //если это из кавычек ,то печатаем на экран
      printf("%s",token);
    }
    else { // если это число или выражение
      putback();
      start_recursive(&answer);
      get_token();
      len += printf("%d", answer);
    }
    last_delim = *token;
  } while (*token==';' || *token==',');
}

//находим все метки
void scan_labels()
{
    int addr;
    char *tmp;

    register int t;
    //обнуляем все метки
    for(t=0; t<NUM_LABEL; ++t) label_table[t].name[0]='\0';
    
    tmp = prog;   //делаем временную переменную сс нашей программой

    //если первый токен это метка
    get_token();
    
    
    if(token_type==MARK) {
        strcpy(label_table[0].name,token);
        label_table[0].p=prog;
    }


    find_eol();//переход на новую строку
    do {
        get_token();
        
        if(token_type == MARK) {
          addr = get_next_label(token);
        if(addr==-1 || addr==-2) {
          printf("%s","Ошибка в массиве меток");
        }
        strcpy(label_table[addr].name, token);
        label_table[addr].p = prog;  //текущая точка в нашей программе
    }
    //если не на пустой строке , надо найти новую
    if(tok!=EOL) find_eol();
  } while(tok!=FINISHED);
  prog = tmp;  // после прочтения и записи всех меток нужно вернуть изначальную программу, которую мы ранее записали в tmp
}

//находит начало следующей строки
void find_eol()
{
  while(*prog!='\n'  && *prog!='\0') ++prog;
  if(*prog) prog++;
}

//функция возвращает позицию в масиве меток 
int get_next_label(char *s)
{
  register int t;
  for(t=0;t<NUM_LABEL;++t) {
    if(label_table[t].name[0]==0) return t;
    if(!strcmp(label_table[t].name,s)) return -2; //-2 если такая метка уже существует
  }

  return -1;//-1 если массив с метками полон
}

//находит положение метки в массиве меток 
char *find_label(s)
char *s;
{
  register int t;

  for(t=0; t<NUM_LABEL; ++t){
    if(!strcmp(label_table[t].name,s)) return label_table[t].p;
  }
  return '\0'; //ткой метки не существует
}

//GOTO 
void goto_statement()
{

  char *location;

  get_token(); //получаем позицию метки(позиция после goto)

  //после получения метки ищем её в массиве меток
  location = find_label(token);
  if(location=='\0')
    printf("такой метки не существует");

  else prog=location;  //стартуем с location
}

//if
void if_statement()
{
  int left , right, status;
  char oper;

  start_recursive(&left); // получаем первое число

  get_token(); // получаем оператор (=><)
  if(!strchr("=<>", *token)) {
    printf("неправильный оператор");
    return;
  }
  oper=*token; // в oper записываем оператор

  start_recursive(&right); //получаем второе число

  /* determine the outcome */
  status = 0;
  switch(oper) {
    case '<':
      if(left<right) status=1;
      break;
    case '>':
      if(left>right) status=1;
      break;
    case '=':
      if(left==right) status=1;
      break;
  }
  if(status) { //если IF выполнится
    get_token();//получаем следующий токен THEN
    if(tok!=THEN) {
      printf("не хватает оператора THEN");
      return;
    }
  }
  else find_eol(); //находим начало след строки
}


//input
void input()
{
  char var;
  int i;

  get_token(); //получаем все, что находится после INPUT
  if(token_type==QUOTE) {
    printf("%s",token); 
    get_token();
  }
  else printf("? ");
  var = toupper(*token)-'A'; // получаем переменную для INPUT

  scanf("%d", &i); //читаем ввод с консоли

  variables[var] = i; //записываем в массив переменных число
}

//gosub
void gosub()
{
  char *location;

  get_token();
  //получаем метку после GOSUB
  location = find_label(token); //ищем метку которуб получили
  if(location=='\0')
    printf("метка не найдена");
  else {
   
     //сохраняем программу с меткой в массив gstack
    prog = location;  //стартуем программу с метки которую нашли ранее
    gpush(prog);
  }
}

// return 
void return_statement()
{
  prog = gstack[gindex--];
}

//фнкция которая записывает в стек 
void gpush(char *str)
{
  gindex++;

  if(gindex==SUB_NEST) {
    printf("gpush выход за границы");
    return;
  }

  gstack[gindex]=str;//сохраняем проограмму в массив 

}

//получаем токен
int get_token()
{

  char *tmp;

  token_type=0; 
  tok=0;
  tmp=token;

  if(*prog=='\0') { //конец файла 
    *token=0;
    tok = FINISHED;
    return(token_type=DELIMITER);
  }

  while(iswhite(*prog)) ++prog;  //пропуск пробелов

  if(strchr("+-*^/%=;(),><", *prog)){ //сравнение операторов
    *tmp=*prog;
    prog++; 
    tmp++;
    *tmp=0;
    return (token_type=DELIMITER);
  }

  if(*prog=='"') { //сравниваем с " 
    prog++;
    while(*prog!='"' && *prog!='\r') *tmp++=*prog++;//считываем все , что находится в "
    prog++;*tmp=0;
    return(token_type=QUOTE);
  }

  if(isdigit(*prog)) { //проверка на число
    while(!isdelim(*prog)) *tmp++=*prog++;
    *tmp = '\0';
    return(token_type = NUMBER);
  }

  if(strchr(":",*prog)){
    prog++;
    if(isalpha(*prog)){
      while(!isdelim(*prog)) *tmp++=*prog++;
      *tmp = '\0';
      return(token_type = MARK);
    }
  }
  

  if(isalpha(*prog)) { //проверка на буквы
    while(!isdelim(*prog)) *tmp++=*prog++;
    token_type=STRING;
  }

  *tmp = '\0';

  
  if(token_type==STRING) {
    tok=look_up(token); //записываем в tok 0 или 1 (либо команда ,  либо переменная)
    if(!tok) token_type = VARIABLE;
    else token_type = COMMAND;
  }
  return token_type;
}



//возвращает токен во входной поток
void putback()
{

  char *t;

  t = token;
  for(; *t; t++) prog--;
}

//метод ищет команду в массиве table 
int look_up(char *s)
{
  register int i;
  char *p;

  
  p = s;
  while(*p){ 
    *p = tolower(*p); //нижний регистр
    p++;
  }

  for(i=0; *table[i].command; i++)
      if(!strcmp(table[i].command, s)) return table[i].tok;//проверяем на нахождении команды в массиве
  return 0; //неизвестная команда
}

//проверяет является ли c разделителем
int isdelim(char c)
{
  if(strchr(" ;,+-<>/*%^=()", c))
    return 1;
  return 0;
}

//пропуск пробелов 
int iswhite(char c)
{
  if(c==' ') return 1;
  else return 0;
}

void start_recursive(result)
int *result;
{
  get_token();
  level2(result);
  putback(); //возвращаемся назад во входной поток
}

void level2(result)
int *result;
{
  register char  op;
  int hold;

  level3(result);
  while((op = *token) == '+' || op == '-') {
    get_token();
    level3(&hold);
    arith(op, result, &hold);
  }
}

//умножение , деление и получение остатка
void level3(result)
int *result;
{
  register char  op;
  int hold;

  level4(result);
  while((op = *token) == '*' || op == '/' || op == '%') {
    get_token();
    level4(&hold);
    arith(op, result, &hold);
  }
}

// процес возведения в степень
void level4(result)
int *result;
{
  int hold;

  level5(result);
  if(*token== '^') {
    get_token();
    level4(&hold);
    arith('^', result, &hold);
  }
}

//унарная операция + или -
void level5(result)
int *result;
{
  register char  op;

  op = 0;
  if((token_type==DELIMITER) && *token=='+' || *token=='-') {
    op = *token;
    get_token();
  }
  level6(result);
  if(op)
    unary(op, result);
}

//процесс нахождения кавычек
void level6(result)
int *result;
{
  if((*token == '(') && (token_type == DELIMITER)) {
    get_token();
    level2(result);
    if(*token != ')')
      printf("нет закрывающих кавычек");
    get_token();
  }
  else
    primitive(result);
}

//result = либо число либо переменную
void primitive(result)
int *result;
{

  switch(token_type) {
  case VARIABLE:
    *result = find_var(token);
    get_token();
    return;
  case NUMBER:
    *result = atoi(token);
    get_token();
    return;
  default:
    printf("ошибка в синтаксисе");
  }
}

//выполняем определённые действия между двумя числами
void arith(oper, number1, number2)
char oper;
int *number1, *number2;
{
  register int t, ex;

  switch(oper) {
    case '-':
      *number1 = *number1-*number2;
      break;
    case '+':
      *number1 = *number1+*number2;
      break;
    case '*':
      *number1 = *number1 * *number2;
      break;
    case '/':
      *number1 = (*number1)/(*number2);
      break;
    case '%':
      t = (*number1)/(*number2);
      *number1 = *number1-(t*(*number2));
      break;
    case '^':
      ex = *number1;
      if(*number2==0) {
        *number1 = 1;
        break;
      }
      for(t=*number1-1; t>0; --t) *number1 = (*number1) * ex;
      break;
  }
}

//делаем число отрицательное
void unary(o, r)
char o;
int *r;
{
  if(o=='-') *r = -(*r);
}

//находит переменную
int find_var(char *s)
{
  if(!isalpha(*s)){
    printf("не переменная");
    return 0;
  }
  return variables[toupper(*token)-'A'];
}