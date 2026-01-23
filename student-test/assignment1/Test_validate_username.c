#include "unity.h"
#include <stdbool.h>
#include <stdlib.h>
#include "../../examples/autotest-validate/autotest-validate.h"
#include "../../assignment-autotest/test/assignment1/username-from-conf-file.h"

/**
* This function should:
*   1) Call the my_username() function in Test_assignment_validate.c to get your hard coded username.
*   2) Obtain the value returned from function malloc_username_from_conf_file() in username-from-conf-file.h within
*       the assignment autotest submodule at assignment-autotest/test/assignment1/
*   3) Use unity assertion TEST_ASSERT_EQUAL_STRING_MESSAGE the two strings are equal.  See
*       the [unity assertion reference](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md)
*
* Disclosure: This software was created in collaboration with Claude Code. The above comments are my own, and were used to guide Claude
* while it aided me in the production of this software.
* Claude Code Chats:
*  - https://gist.github.com/jsnapoli1/daef21a674e192b11fa0f5327a074fe8
*   Note the error that was debugged in the above chat was an artifact of my host system, and I was able to fix it in later iterations.
*   This change is no longer required, and has been removed.
*/
void test_validate_my_username()
{
    // Step 1: Store expected value
    // my_username() returns a pointer to a string literal (static storage),
    // so it's const and does not need to be freed.
    const char *my_name = my_username();

    // Step 2: Store the current value
    char *conf_name = malloc_username_from_conf_file();

    // Step 3: Compare and report
    TEST_ASSERT_EQUAL_STRING_MESSAGE(my_name, conf_name, "ERROR: conf file and username from my_username() do not match");
    free(conf_name);
}
