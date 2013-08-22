/* file minunit_example.c */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/common.h"
#include "minunit.h"

int tests_run = 0;


void * xmalloc=malloc;
void * xrealloc=realloc;


char *test_lowercase() {
  char source[] = "This is a Test!";
  int result;
  
  lowercase(source);
  result=memcmp("this is a test!", source, strlen(source));

  mu_assert("lowercase failed",result == 0);
  return 0;
 }

 
char *all_tests() {
     mu_run_test(test_lowercase);
     return 0;
 }
 
int main(int argc, char **argv) {
     char *result = all_tests();
     printf ("%d\n", (int) result);
     if (result != 0) {
         printf("%s\n", result);
     }
     else {
         printf("ALL TESTS PASSED\n");
     }
     printf("Tests run: %d\n", tests_run);
 
     return result != 0;
 }

