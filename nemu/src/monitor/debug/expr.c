#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NEQ, TK_AND, TK_OR, TK_REG, TK_HEX, TK_NUM,TK_NEGATIVE,TK_DEREFERENCE,

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},       // equal
  {"-", '-'},         // minus
  {"\\*", '*'},         // multiply
  {"/", '/'},         // divide
  {"\\(", '('},         // left bracket
  {"\\)", ')'},         // right bracket
  {"!=", TK_NEQ},       // not equal
  {"&&", TK_AND},       // and
  {"\\|\\|", TK_OR},    // or
   {"\\$(\\$0|ra|[sgt]p|t[0-6]|a[0-7]|s([0-9]|1[0-1]))", TK_REG},//registers
  {"0[xX][0-9a-fA-F]+",TK_HEX},    //hex numbers
  {"[0-9]+", TK_NUM},   // numbers
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        
        switch (rules[i].token_type) {
          case TK_NOTYPE: break;
          case '+': tokens[nr_token].type = '+'; nr_token++; break;
          case '-': tokens[nr_token].type = '-'; nr_token++; break;
          case '*': tokens[nr_token].type = '*'; nr_token++; break;
          case '/': tokens[nr_token].type = '/'; nr_token++; break;
          case '(': tokens[nr_token].type = '('; nr_token++; break;
          case ')': tokens[nr_token].type = ')'; nr_token++; break;
          case TK_EQ: tokens[nr_token].type = TK_EQ; nr_token++; break;
          case TK_NEQ: tokens[nr_token].type = TK_NEQ; nr_token++; break;
          case TK_AND: tokens[nr_token].type = TK_AND; nr_token++; break;
          case TK_OR: tokens[nr_token].type = TK_OR; nr_token++; break;
          case TK_REG: tokens[nr_token].type = TK_REG;
                        strncpy(tokens[nr_token].str, substr_start, substr_len);
                        nr_token++; break;
          case TK_HEX: tokens[nr_token].type = TK_HEX;
                        if(substr_len > 32) assert(0);
                        strncpy(tokens[nr_token].str, substr_start, substr_len);
                        nr_token++; break;
          case TK_NUM: tokens[nr_token].type = TK_NUM;
                        if(substr_len > 32) assert(0);
                        strncpy(tokens[nr_token].str, substr_start, substr_len);
                        nr_token++; break;
          default:  //TODO();
                    assert(0);
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p, int q){
  if(tokens[p].type != '(' || tokens[q].type != ')') return false;
  int i, cnt = 0;
  for(i = p + 1; i < q; i++){
    if(tokens[i].type == '(') cnt++;
    else if(tokens[i].type == ')') cnt--;
    if(cnt < 0) return false;
  }
  if(cnt == 0) return true;
  else return false;
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  for(int i = 0; i < nr_token; i++){
    if(tokens[i].type == '*' && (i==0 || (tokens[i-1].type == '+')|| (tokens[i-1].type == '-')|| (tokens[i-1].type == '(') || (tokens[i-1].type == '*'))){
      tokens[i].type = TK_DEREFERENCE;
    }
    if(tokens[i].type == '-' && (i==0 || (tokens[i-1].type == '+')|| (tokens[i-1].type == '-')|| (tokens[i-1].type == '('))){
      tokens[i].type = TK_NEGATIVE;
    }
  }
  return eval(0, nr_token-1, success);
}

int eval(int i, int j, bool *success){
  if(i > j){
    *success = false;
    return 0;
  }
  else if(i == j){
    if(tokens[i].type != TK_NUM && tokens[i].type != TK_HEXNUM && tokens[i].type != TK_REG){
      *success = false;
      return 0;
    }
    int number;
    *success = true;

    if(tokens[i].type == TK_REG)
      number = isa_reg_str2val(&(tokens[i].str[1]), success);
    else if(tokens[i].type == TK_NUM)
      sscanf(tokens[i].str, "%d", &number);
    else
      sscanf(&(tokens[i].str[2]), "%x", &number); /
    return number;
  }
  else if(check_parentheses(i,j)){
    return eval(i+1, j-1, success);
  }
  else{
    int bracketNum = 0, op = -1; // op is the position of main opcode
    int flag = 6; // flag is 1 only when the main opcode is * or /
    bool success1, success2;
    int value1, value2;
    // &&  <   !=,==  <  *,/  <  +,-
    for(int k = i; k <=j; k++){
      if(tokens[k].type == '('){
        bracketNum++;
      }
      else if(tokens[k].type == ')'){
        bracketNum--;
      }
      else if(bracketNum == 0){
        if(tokens[k].type == '+' || tokens[k].type == '-'){
          op = k;
          flag = 1;
        }
        if(flag>1 && (tokens[k].type == '*' || tokens[k].type == '/')){
          op = k;
          flag = 2;
        }
        if(flag > 2 && (tokens[k].type == TK_EQ || tokens[k].type == TK_NEQ)){
          op = k;
          flag = 3;
        }
        if(flag >= 4 && (tokens[k].type == TK_AND)){
          op = k;
          flag = 4;
        }
        if(flag >= 5 &&(tokens[k].type == TK_DEREFERENCE)){
            op = k;
            flag = 5;
        }
        if(flag >= 6 &&(tokens[k].type == TK_NEGATIVE)){
            op = k;
            flag = 6;
        }
      }
    }

    if(op == -1){
      *success = false;
      return 0;
    }

    if(flag == 5){
        int addr = eval(i+1, j, success);
        if(!(*success)){
          return 0;
        }
        return paddr_read(addr, 4);
    }

    if(flag == 6){
        int val = eval(i+1, j, success);
        if(!(*success)){
          return 0;
        }
        return -val;
    }

    value1 = eval(i, op-1, &success1);
    value2 = eval(op+1, j, &success2);
    if(!success1 || !success2){
      *success = false;
      return 0;
    }
    *success = true;
    switch(tokens[op].type){
      case '+': return value1 + value2;
      case '-': return value1 - value2;
      case '*': return value1*value2;
      case '/':
        if(value2 == 0){
          printf("Divide 0!\n");
          assert(0);
        }
        return value1/value2;
      case TK_EQ: return (value1 == value2);
      case TK_NEQ: return (value1 != value2);
      case TK_AND: return (value1 && value2);
      default: assert(0);
    }
  }
}