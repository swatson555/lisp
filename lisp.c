#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

char token[128][32];

int lexer(char* input) {
  int ii = 0; // input index
  int ti = 0; // token index

  while(input[ii] != '\0')
    switch(input[ii]) {
    // Ignore whitespace and newlines
    case ' ':
    case '\n':
      ++ii;
      break;

    // Turn a left parenthesis into a token.
    case '(':
      token[ti][0] = '(';
      token[ti][1] = '\0';
      ++ii;
      ++ti;
      break;

    // Turn a right parenthesis into a token.
    case ')':
      token[ti][0] = ')';
      token[ti][1] = '\0';
      ++ii;
      ++ti;
      break;

    // Turn an apostrophe into a token.
    case '\'':
      token[ti][0] = '\'';
      token[ti][1] = '\0';
      ++ii;
      ++ti;
      break;

    // Anything else is a symbol
    default:
      for(int i = 0;; ++i) {
	if(input[ii] != ' '  &&
	   input[ii] != ')'  &&
           input[ii] != '('  &&
           input[ii] != '\n' &&
           input[ii] != '\0') {
          token[ti][i] = input[ii++];
        }
        else {
          token[ti][i] = '\0';
          break;
        }
      }
      ++ti;
      break;
    }
  return ti;
}

int curtok;

char* nexttok() {
  return token[curtok++];
}

char* peektok() {
  return token[curtok];
}


typedef struct Pair {
  void* car;
  void* cdr;
} Pair;

typedef struct Text {
  char* car;
  struct Text* cdr;
} Text;

Pair text[256];
Pair* textptr;

int istext(void* x) {
  return x >= (void*)&text &&
         x <  (void*)&text[256];
}

Pair* tcons(void* x, void* y) {
  assert(istext(textptr));
  textptr->car = x;
  textptr->cdr = y;
  return textptr++;
}

void* read(char* ln);
void* read_exp();
void* read_list();

void* read(char* ln) {
  // Initialize the lexer and list memory.
  curtok = 0;
  textptr = text;

  lexer(ln);
  return read_exp();
}

void* read_exp() {
  char* tok = nexttok();
  if (tok[0] == '(' && peektok()[0] == ')') {
    nexttok();
    return NULL;
  }
  else if (tok[0] == '\'')
    return tcons("quote", tcons(read_exp(), NULL));
  else if (tok[0] == '(')
    return read_list();
  else
    return tok;
}

void* read_list() {
  char* tok = peektok();
  if(tok[0] == ')') {
    nexttok();
    return NULL;
  }
  else if(tok[0] == '.') {
    nexttok();
    tok = read_exp();
    nexttok();    
    return tok;
  }
  else {
    void* fst = read_exp();
    void* snd = read_list();
    return tcons(fst, snd);
  }
}

int islist(void* x);
int isenv(void* x);

void print(void* exp);
void print_exp(void* exp);
void print_list(Pair* list);
void print_cons(Pair* pair);

void print(void* exp) {
  print_exp(exp);
  printf("\n");
}

void print_exp(void* exp) {
  if (istext(exp) || islist(exp)) {
    Pair* pair = exp;
    if (isenv(pair->car)) {
      printf("#<lambda>");
    }
    else if(!istext(pair->cdr) && !islist(pair->cdr) && pair->cdr != NULL) {
      printf("(");
      print_cons(exp);
      printf(")");
    }
    else {
      printf("(");
      print_list(exp);
    }
  }
  else
    printf("%s", exp ? (char*)exp : "()");
}

void print_list(Pair* list) {
  if (list->cdr == NULL) {
    print_exp(list->car);
    printf(")");
  }
  else {
    if(!islist(list->cdr) && !istext(list->cdr) && list->cdr != NULL) {
      print_cons(list);
      printf(")");
    }
    else {
      print_exp(list->car);
      printf(" ");
      print_list(list->cdr);
    }
  }
}

void print_cons(Pair* pair) {
  print_exp(pair->car);
  printf(" . ");
  print_exp(pair->cdr);
}


char symbol[2048];
char* symbolptr = symbol;

Pair list[1280];
Pair* listptr = list;

int islist(void* x) {
  return x >= (void*)&list &&
         x <  (void*)&list[1280];
}

Pair* cons(void* x, void* y) {
  assert(islist(listptr));
  listptr->car = x;
  listptr->cdr = y;
  return listptr++;
}

void* cpysym(void* sym) {
  if (sym) {
    sym = strcpy(symbolptr, sym);
    symbolptr = symbolptr + strlen(sym) + 1;
  }
  return sym;
}

void* cpy(Text* text) {
  if (istext(text) || islist(text)) {
    if (istext(text->car) || islist(text->car))
      return cons(cpy((Text*)text->car), text->cdr ? cpy(text->cdr) : NULL);
    else
      return cons(cpysym(text->car), text->cdr ? cpy(text->cdr) : NULL);
  }
  return cpysym(text);
}

void* cpylambda(Pair* val) {
  Pair* lambda = val->cdr;
  lambda->car = lambda->car ? cpy(lambda->car) : NULL;
  lambda->cdr = cpy(lambda->cdr);
  return val;
}

void* lambda(Text* args, Text* body, void* env) {
  return cons(env, cons(args, body));
}

typedef struct {
  char sym[32];
  void* val;
} Entry;

typedef struct Env {
  Entry entry[32];
  Entry* entryptr;
  struct Env* next;
} Env;

Env global = {
  {{ .sym = "+", .val=(void*)1 },
   { .sym = "-", .val=(void*)2 },
   { .sym = "*", .val=(void*)3 },
   { .sym = "/", .val=(void*)4 },
   { .sym = "car", .val=(void*)5 },
   { .sym = "cdr", .val=(void*)6 },
   { .sym = "=", .val=(void*)7 },
   { .sym = "cons", .val=(void*)8 },
   { .sym = "list", .val=(void*)9 },},
  .entryptr = global.entry+9,
  NULL
};

Env frame[128];
Env* frameptr = frame;

Env* extend(Env* env) {
  assert(isenv(frameptr));
  frameptr->next = env;
  frameptr->entryptr = frameptr->entry;
  return frameptr++;
}

void retract() {
  assert(isenv(frameptr));
  frameptr--;
  memset(frameptr->entry, 0, sizeof(Entry[32]));
}

int isenv(void* x) {
  return x >= (void*)&frame &&
         x <  (void*)&frame[128] ||
         x == (void*)&global;
}

void put(void* sym, void* val, Env* env) {
  assert(env);
  assert(env->entryptr >= (Entry*)&env->entry && env->entryptr < (Entry*)&env->entry[32]);
  strcpy(env->entryptr->sym, sym);
  if (istext(val) || islist(val)) {
    Pair* pair = val;
    if (isenv(pair->car))
      env->entryptr->val = cpylambda(val);
    else
      env->entryptr->val = cpy(val);
  }
  else
    env->entryptr->val = cpysym(val);
  env->entryptr++;
}

void* get(void* sym, Env* env) {
  assert(env);
  Entry* seek = env->entryptr;
  for (;seek != env->entry-1; --seek)
    if (strcmp(seek->sym, sym) == 0)
        return seek->val;
  // Look in the next Environment
  return get(sym, env->next);
}

void set(void* sym, void* val, Env* env) {
  assert(env);
  Entry* seek = env->entryptr;
  for (;seek != env->entry-1; --seek)
    if (strcmp(seek->sym, sym) == 0) {
      if (istext(val) || islist(val)) {
        Pair* pair = val;
        if (isenv(pair->car))
          seek->val = cpylambda(val);
        else
          seek->val = cpy(val);
      }
      else
        seek->val = cpysym(val);
      return;
    }
  return set(sym, val, env->next);
}


void* eval(void* exp);
void* eval_exp(void* exp, Env* env);
void* apply(void* func, Text* args, Env* env);

void* eval(void* exp) {
  return eval_exp(exp, &global);
}

void* eval_exp(void* exp, Env* env) {
  if (istext(exp) || islist(exp)) {
    Text* text = exp;
    if (strcmp(text->car, "define") == 0) {
      void* var = text->cdr->car;
      void* val = eval_exp(text->cdr->cdr->car, env);
      put(var, val, env);
      return NULL;
    }
    if (strcmp(text->car, "set!") == 0) {
      void* var = text->cdr->car;
      void* val = eval_exp(text->cdr->cdr->car, env);
      set(var, val, env);
      return NULL;
    }
    else if (strcmp(text->car, "quote") == 0) {
      return text->cdr->car;
    }
    else if (strcmp(text->car, "if") == 0) {
      void* cond = eval_exp(text->cdr->car, env);
      if (strcmp(cond, "#t") == 0)
        return eval_exp(text->cdr->cdr->car, env);
      else
        return eval_exp(text->cdr->cdr->cdr->car, env);
    }
    else if (strcmp(text->car, "lambda") == 0) {
      return lambda((Text*)text->cdr->car, text->cdr->cdr, env);
    }
    else {
      void* fun = eval_exp(text->car, env);
      return apply(fun, text->cdr, env);
    }
  }
  // evaluate the symbol in the environment if it's not self-evaluating.
  return isdigit(*((char*)exp)) || strcmp(exp, "#f") == 0 || strcmp(exp, "#t") == 0 ? exp : get(exp, env);
}

void* evalargs(Text* args, Env* env) {
  return cons(eval_exp(args->car, env), args->cdr ? evalargs(args->cdr, env) : NULL);
}

void* evalbody(Text* body, Env* env) {
  void* val = eval_exp(body->car, env);
  if (body->cdr)
    return evalbody(body->cdr, env);
  else
    return val;
}

void parameterize(Text* args, Text* para, Env* env) {
  put(para->car, args->car, env);
  if (args->cdr != NULL)
    parameterize(args->cdr, para->cdr, env);
}

void* apply(void* func, Text* args, Env* env) {
  if (islist(func)) {
    Pair* pair = func;
    Env* closure = pair->car;
    Pair* lambda = pair->cdr;
    Text* para = lambda->car;
    Text* body = lambda->cdr;
    Env* lambdaenv = extend(closure);
    if (para) {
      Text* evargs = evalargs(args, env);
      parameterize(evargs, para, lambdaenv);
    }
    return evalbody(body, lambdaenv);
  }
  else {
    char evret[32];
    if (func == (void*)1) {
      int left = atoi(eval_exp(args->car, env));
      int right = atoi(eval_exp(args->cdr->car, env));
      sprintf(evret, "%d", left+right);
      return cpysym(evret);
    }
    else if (func == (void*)2) {
      int left = atoi(eval_exp(args->car, env));
      int right = atoi(eval_exp(args->cdr->car, env));
      sprintf(evret, "%d", left-right);
      return cpysym(evret);
    }
    else if (func == (void*)3) {
      int left = atoi(eval_exp(args->car, env));
      int right = atoi(eval_exp(args->cdr->car, env));
      sprintf(evret, "%d", left*right);
      return cpysym(evret);
    }
    else if (func == (void*)4) {
      int left = atoi(eval_exp(args->car, env));
      int right = atoi(eval_exp(args->cdr->car, env));
      sprintf(evret, "%d", left/right);
      return cpysym(evret);
    }
    else if (func == (void*)5) {
      Pair* pair = eval_exp(args->car, env);
      return pair->car;
    }
    else if (func == (void*)6) {
      Pair* pair = eval_exp(args->car, env);
      return pair->cdr;
    }
    else if (func == (void*)7) {
      char* left = eval_exp(args->car, env);
      char* right = eval_exp(args->cdr->car, env);
      if(left && right)
        return strcmp(left, right) == 0 ? "#t" : "#f";
      return left == right ? "#t" : "#f";
    }
    else if (func == (void*)8) {
      void* left = eval_exp(args->car, env);
      void* right = eval_exp(args->cdr->car, env);
      return cons(left, right);
    }
    else if (func == (void*)9) {
      return evalargs(args, env);
    }
  }
  assert(1);
  return 0;
}


int main(int argc, char** argv) {
  printf("Lisp REPL\n\n");
  printf(">> ");

  char buffer[256];
  while (fgets(buffer, 256, stdin)) {
    print(eval(read(buffer)));
    printf(">> ");
  }
  return 0;
}
