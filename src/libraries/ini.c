/* ini.c : header file for INI file parser */
/* Jon Mayo - PUBLIC DOMAIN - July 13, 2007
* initial: 2007-07-13
* $Id: ini.c 186 2007-08-01 09:23:57Z orange $ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef _XOPEN_SOURCE
# include <strings.h>
#endif
#include "ini.h"

struct parameter {
  char *name;
  char *value;
  struct parameter *next;
};

struct section {
  char *name;
  struct parameter *head;
  struct section *next;
};

struct ini_info {
  struct section *head;
  /* the rest is used for parsing */
  char *filename;
  unsigned line;
  struct section *curr_sect;
  struct parameter *curr_param;
};

void ini_free(struct ini_info *ii) {
  struct section *curr_sect, *next_sect;
  struct parameter *curr, *next;
  if(ii) {
    for(curr_sect=ii->head;curr_sect;curr_sect=next_sect) {
      for(curr=curr_sect->head;curr;curr=next) {
        next=curr->next;
        free(curr->name);
        free(curr->value);
        free(curr);
      }
      next_sect=curr_sect->next;
      free(curr_sect->name);
      free(curr_sect);
    }

    free(ii->filename);
    free(ii);
  }
}

/** loads an INI file, parses it and sticks it in the data structure.
*/
struct ini_info *ini_load(const char *filename) {
  char buf[512], *s;
  struct ini_info *ret;
  struct section *curr_sect, **prev_sect;
  struct parameter *curr, **prev;
  unsigned line;
  FILE *f;

  f=fopen(filename, "r");
  if(!f) {
    fprintf(stderr, "%s:%s\n", filename, strerror(errno));
    return 0;
  }

  /* create a new ini_info */
  ret=malloc(sizeof *ret);
  ret->filename=strdup(filename);
  ret->line=0;
  ret->curr_sect=0;
  ret->curr_param=0;

  prev_sect=&ret->head;
  curr=0;
  prev=0;
  line=0;
  while(fgets(buf, sizeof buf, f)) {
    line++;
    s=buf;
    while(isspace(*s)) s++; /* ignore leading whitespace */

      if(*s==';' || *s=='#' || *s==0) { /* comment or blank line */

      } else if(*s=='[') { /* new section */
        char *tmp;

        /* parse the section */
        s=buf+1;
        tmp=strchr(s, ']');
        if(tmp) {
          *tmp=0;
        } else { /* error : unterminated section names */
          fprintf(stderr, "%s:%d:unterminated section name.\n", filename, line);
          ini_free(ret);
          ret=0; /* failure */
          break;
        }

        /* create section */
        curr_sect=malloc(sizeof *curr_sect);
        curr_sect->next=0;
        curr_sect->head=0;
        curr_sect->name=strdup(s);
        /* append it to the end */
        *prev_sect=curr_sect;
        prev_sect=&curr_sect->next;

        prev=&curr_sect->head;
        curr=*prev;
      } else if(prev) { /* parameter inside section */
        char *value, *tmp;

        /* trim trailing comment */
        tmp=strchr(s, ';');
        if(tmp) {
          *tmp=0;
        }

        /* trim trailing whitespace */
        tmp=s+strlen(s);
        while(tmp!=s) {
          tmp--;
          if(!isspace(*tmp)) break;
          *tmp=0;
        }

        /* look for '=' sign to mark the start of the value */
        value=s;
        while(*value && *value!='=') value++;
          if(*value) {
            /* build the value string */
            *value=0; /* chop parameter string - now s is parameter */
            value++;
            while(isspace(*value)) value++; /* ignore leading whitespace */

              /* build the parameter string */
              /* trim trailing whitespace */
              tmp=s+strlen(s);
              while(tmp!=s) {
                tmp--;
                if(!isspace(*tmp)) break;
                *tmp=0;
              }

              /* create parameter */
              curr=malloc(sizeof *curr);
              curr->name=strdup(s);
              curr->value=strdup(value);
              curr->next=0;
              /* append it to the end */
              *prev=curr;
              prev=&curr->next;
            } else { /* error: no value found */
              fprintf(stderr, "%s:%d:no value found for parameter.\n", filename, line);
              ini_free(ret);
              ret=0; /* failure */
              break;
            }
          } else { /* error: parameters outside section */
            fprintf(stderr, "%s:%d:parse error in INI file.\n", filename, line);
            /* it's important to always keep the structure in a consistant
            * state so that we can use the standard free utility */
            ini_free(ret);
            ret=0; /* failure */
            break;
          }
        }

        fclose(f);
        return ret;
      }

/** sequential i/o
* return section name, NULL on end of file
*/
const char *ini_next_section(struct ini_info *ii) {
  ii->curr_param=0; /* always reset the parameter */

  if(ii->curr_sect) {
    ii->curr_sect=ii->curr_sect->next;
  } else {
    ii->curr_sect=ii->head;
  }

  if(ii->curr_sect) {
    return ii->curr_sect->name;
  } else {
    return 0; /* end */
  }
}

/** sequential i/o
* return value of parameter. NULL on end of section
*/
const char *ini_next_parameter(struct ini_info *ii, const char **parameter) {
  /* go to next parameter, or the first if set to 0 */
  if(ii->curr_param) {
    ii->curr_param=ii->curr_param->next;
  } else {
    ii->curr_param=ii->curr_sect->head;
  }

  if(ii->curr_param) {
    if(parameter) *parameter=ii->curr_param->name;
    return ii->curr_param->value;
  } else {
    return 0; /* end */
  }
}

/** rewind the pointers so the ini_next_xxx functions can start from the top */
void ini_rewind(struct ini_info *ii) {
  if(ii) {
    ii->curr_param=0;
    ii->curr_sect=0;
  }
}

/** random access function
* return value for a parameter */
const char *ini_get(struct ini_info *ii, const char *section, const char *parameter) {
  struct section *curr_sect;
  struct parameter *curr_param;

  for(curr_sect=ii->head;curr_sect;curr_sect=curr_sect->next) {
    if(strcasecmp(curr_sect->name, section)==0) {
      break;
    }
  }
  if(!curr_sect) return 0;
  for(curr_param=curr_sect->head;curr_param;curr_param=curr_param->next) {
    if(strcasecmp(curr_param->name, parameter)==0) {
      return curr_param->value;
    }
  }
  return 0;
}
