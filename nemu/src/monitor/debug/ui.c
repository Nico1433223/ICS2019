#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

// Single Step
static int cmd_si(char *args) {
  int step = 1;
  if (args != NULL) {
    step = atoi(args);
  }
  cpu_exec(step);
  return 0;
}

// Print register state
static int cmd_info(char *args) {
  if (args == NULL) {
    printf("No argument given\n");
    return 0;
  }
  if (strcmp(args, "r") == 0) {
    isa_reg_display();
  }
  else {
    printf("Unknown argument '%s'\n", args);
  }
  return 0;
}

// Scan memory
static int cmd_x(char *args) {
  if (args == NULL) {
    printf("No argument given\n");
    return 0;
  }
  char *arg = strtok(args, " ");
  if (arg == NULL) {
    printf("No argument given\n");
    return 0;
  }
  int n = atoi(arg);
  arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("No argument given\n");
    return 0;
  }
  vaddr_t addr = strtol(arg, NULL, 16);
  for (int i = 0; i < n; i++) {
    printf("0x%08x: 0x%08x\n", addr, vaddr_read(addr, 4));
    addr += 4;
  }
  return 0;
}

// Evaluate expression
static int cmd_p(char *args) {
  if (args == NULL) {
    printf("No argument given\n");
    return 0;
  }
  bool success = true;
  uint32_t result = expr(args, &success);
  if (success) {
    printf("%u\n", result);
  }
  else {
    printf("Invalid expression\n");
  }
  return 0;
}

// Set watchpoint
static int cmd_w(char *args) {
  if (args == NULL) {
    printf("No argument given\n");
    return 0;
  }
  bool success = true;
  uint32_t result = expr(args, &success);
  if (success) {
    WP *wp = new_wp();
    strcpy(wp->expr, args);
    wp->value = result;
    printf("Watchpoint %d: %s\n", wp->NO, wp->expr);
  }
  else {
    printf("Invalid expression\n");
  }
  return 0;
}

// Delete watchpoint
static int cmd_d(char *args) {
  if (args == NULL) {
    printf("No argument given\n");
    return 0;
  }
  int n = atoi(args);
  for(WP* tmp = head; tmp; tmp = tmp->next){
    if(tmp->NO == number){
      free_wp(tmp);
      return 0;
    }
  }
  return 0;
}

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "Single step", cmd_si },
  { "info", "Print register state", cmd_info },
  { "x", "Scan memory", cmd_x },
  { "p", "Evaluate expression", cmd_p },
  { "w", "Set watchpoint", cmd_w },
  { "d", "Delete watchpoint", cmd_d },
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
