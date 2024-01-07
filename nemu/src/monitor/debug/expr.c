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
  {"\\-", '-'},         // minus
  {"\\*", '*'},         // multiply
  {"\\/", '/'},         // divide
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

int dominant_operator(int p, int q){
  int i, cnt = 0, op = -1, op_type = -1;
  for(i = p; i <= q; i++){
    if(tokens[i].type == '(') cnt++;
    else if(tokens[i].type == ')') cnt--;
    else if(cnt == 0){
      if(tokens[i].type == '+' || tokens[i].type == '-'){
        if(op_type < 2){
          op = i;
          op_type = 2;
        }
      }
      else if(tokens[i].type == '*' || tokens[i].type == '/'){
        if(op_type < 1){
          op = i;
          op_type = 1;
        }
      }
      else if(tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ){
        if(op_type < 3){
          op = i;
          op_type = 3;
        }
      }
      else if(tokens[i].type == TK_AND){
        if(op_type < 4){
          op = i;
          op_type = 4;
        }
      }
      else if(tokens[i].type == TK_OR){
        if(op_type < 5){
          op = i;
          op_type = 5;
        }
      }
    }
  }
  return op;
}

uint32_t eval(int p, int q){
  if(p > q){
    assert(0);
  }
  else if(p == q){

    if(tokens[p].type == TK_NUM){
      return atoi(tokens[p].str);
    }
    else if(tokens[p].type == TK_HEX){
      return strtol(tokens[p].str, NULL, 16);
    }
    else if(tokens[p].type == TK_REG){
      return isa_reg_str2val(tokens[p].str + 1);
    }
    else{
      assert(0);
    }
  }
  else if(check_parentheses(p, q) == true){
    return eval(p + 1, q - 1);
  }
  else{
    int op = dominant_operator(p,
 q);
    if(op == -1){
      printf("Bad expression!\n");
      assert(0);
    }
    if(tokens[op].type == TK_NEGATIVE || tokens[op].type == TK_DEREFERENCE) return eval(op, q); //unary operators
    uint32_t val1 = eval(p, op - 1);
    uint32_t val2 = eval(op + 1, q);
    switch(tokens[op].type){
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      case TK_EQ: return val1 == val2;
      case TK_NEQ: return val1 != val2;
      case TK_AND: return val1 && val2;
      case TK_OR: return val1 || val2;
      case TK_NEGATIVE: return -val2;
      case TK_DEREFERENCE: return vaddr_read(val2, 4);
      default: assert(0);
    }
  }
  return 0;
}



uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  // TODO();
  *success = true;
  int i;
  for(i = 0; i < nr_token; i++){
    if(tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != TK_NUM && tokens[i - 1].type != TK_HEX && tokens[i - 1].type != TK_REG && tokens[i - 1].type != ')'))){
      tokens[i].type = TK_DEREFERENCE;
    }
    else if(tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type != TK_NUM && tokens[i - 1].type != TK_HEX && tokens[i - 1].type != TK_REG && tokens[i - 1].type != ')'))){
      tokens[i].type = TK_NEGATIVE;
    }
  }
  return eval(0, nr_token - 1);
}
