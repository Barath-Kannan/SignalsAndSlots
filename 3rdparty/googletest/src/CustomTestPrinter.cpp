#include "gtest/gtest.h"
#include <iostream>

using std::cout;
using std::endl;
using namespace testing;

static const char kUniversalFilter[] = "*";
static const char kTypeParamLabel[] = "TypeParam";
static const char kValueParamLabel[] = "GetParam()";

#define PRINT_SEPARATOR printf("-----------------------------------------------------------------------------------\n");
#define PRINT_MAIN_SEPARATOR printf("|---------------------------------------------------------------------------------|\n");
#define PRINT_UNDERSEPARATOR printf("|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -|\n");
#define PRINT_EXTENDER printf("- ");
#define PRINT_ALT_EXTENDER printf("~| ");
#define PRINT_LONG_EXTENDER printf("    ");
#define PRINT_FAILSTART_MARKER printf("|#!#!#! TEST_FAILURE >>>>>>> TEST_FAILURE <<<<<<< TEST_FAILURE #!#!#!|\n");
#define PRINT_FAILEND_MARKER printf(" ^       ^      ^     ^    ^   ^  ^ ^  ^   ^    ^     ^      ^       ^ \n");

class CustomTestPrinter : public EmptyTestEventListener {
    // Fired before each iteration of tests starts.

    void OnTestIterationStart(const UnitTest& unit_test, int iteration) {
        if (GTEST_FLAG(repeat) != 1) {
            printf("\nRepeating all tests (iteration %d) . . .\n\n", iteration + 1);
        }
        const char* const filter = GTEST_FLAG(filter).c_str();

        // Prints the filter if it's not *.  This reminds the user that some
        // tests may be skipped.
        if (strcmp(filter, kUniversalFilter) != 0) {
            printf("Note: %s filter = %s\n", GTEST_NAME_, filter);
        }

        if (GTEST_FLAG(shuffle)) {
            printf("Note: Randomizing tests' orders with a seed of %d .\n", unit_test.random_seed());
        }

        PRINT_SEPARATOR;
        PRINT_EXTENDER;
        printf("Running %d test%s from %d test case%s.\n",
                unit_test.test_to_run_count(), unit_test.test_to_run_count() == 1 ? "" : "s",
                unit_test.test_case_to_run_count(), unit_test.test_case_to_run_count() == 1 ? "" : "s");
        PRINT_SEPARATOR;
        fflush(stdout);
    }

    void OnEnvironmentsSetUpStart(const UnitTest& /*unit_test*/) {
        //PRINT_EXTENDER;
        //printf("Global test environment set-up.\n");
        fflush(stdout);
    }

    void OnTestCaseStart(const TestCase& test_case) {
        PRINT_ALT_EXTENDER;
        printf("Test Case: %s", test_case.name());
        if (test_case.type_param() == NULL) {
            printf("\n");
        } else {
            printf(", where %s = %s\n", kTypeParamLabel, test_case.type_param());
        }
        PRINT_ALT_EXTENDER;
        printf("Tests: \n");
        for (int i=0; i<test_case.test_to_run_count(); i++){
            PRINT_ALT_EXTENDER;
            PRINT_LONG_EXTENDER;
            PRINT_EXTENDER;
            printf("%s\n", test_case.GetTestInfo(i)->name());
            
        }
        /*printf("%d %s from %s", test_case.test_to_run_count(), test_case.test_to_run_count() == 1 ? "test" : "tests", test_case.name());
        if (test_case.type_param() == NULL) {
            printf("\n");
        } else {
            printf(", where %s = %s\n", kTypeParamLabel, test_case.type_param());
        }*/
        PRINT_SEPARATOR;
        fflush(stdout);
    }

    void OnTestStart(const TestInfo& test_info) {
        PRINT_LONG_EXTENDER;
        printf("Test %s.%s RUNNING", test_info.test_case_name(), test_info.name());
        printf("\n");
        PRINT_SEPARATOR;
        fflush(stdout);
    }

    // Called after an assertion failure.

    void OnTestPartResult(
            const TestPartResult& result) {
        // If the test part succeeded, we don't need to do anything.
        if (result.type() == TestPartResult::kSuccess)
            return;
        PRINT_FAILSTART_MARKER;
        printf("Failure in %s:%d\n%s\n",
                result.file_name(),
                result.line_number(),
                result.summary());
        PRINT_FAILEND_MARKER;
        fflush(stdout);
    }

    void PrintFullTestCommentIfPresent(const TestInfo& test_info) {
        const char* const type_param = test_info.type_param();
        const char* const value_param = test_info.value_param();

        if (type_param != NULL || value_param != NULL) {
            printf(", where ");
            if (type_param != NULL) {
                printf("%s = %s", kTypeParamLabel, type_param);
                if (value_param != NULL)
                    printf(" and ");
            }
            if (value_param != NULL) {
                printf("%s = %s", kValueParamLabel, value_param);
            }
        }
    }

    void OnTestEnd(const TestInfo& test_info) {
        PRINT_SEPARATOR;
        PRINT_LONG_EXTENDER;
        printf("Test %s.%s %s", test_info.test_case_name(), test_info.name(), test_info.result()->Passed() ? "PASSED" : "FAILED");

        if (test_info.result()->Failed())
            PrintFullTestCommentIfPresent(test_info);

        if (GTEST_FLAG(print_time)) {
            printf(" (%ld ms)\n", (long)test_info.result()->elapsed_time());
        } else {
            printf("\n");
        }
        PRINT_SEPARATOR;
        fflush(stdout);
    }

    void OnTestCaseEnd(const TestCase& test_case) {
        if (!GTEST_FLAG(print_time)) return;
        PRINT_ALT_EXTENDER;
        printf("%d %s from %s (%ld ms total)\n", 
                test_case.test_to_run_count(), test_case.test_to_run_count() == 1 ? "test" : "tests", test_case.name(), (long)test_case.elapsed_time());
        PRINT_SEPARATOR;
        fflush(stdout);
    }

    void OnEnvironmentsTearDownStart(
            const UnitTest& /*unit_test*/) {
        //PRINT_EXTENDER;
        //printf("Global test environment tear-down\n");
        fflush(stdout);
    }

    // Internal helper for printing the list of failed tests.

    void PrintFailedTests(const UnitTest& unit_test) {
        const int failed_test_count = unit_test.failed_test_count();
        if (failed_test_count == 0) {
            return;
        }
        PRINT_SEPARATOR;
        printf("Failed tests: \n");
        for (int i = 0; i < unit_test.total_test_case_count(); ++i) {
            const TestCase& test_case = *unit_test.GetTestCase(i);
            if (!test_case.should_run() || (test_case.failed_test_count() == 0)) {
                continue;
            }

            for (int j = 0; j < test_case.total_test_count(); ++j) {
                const TestInfo& test_info = *test_case.GetTestInfo(j);
                if (!test_info.should_run() || test_info.result()->Passed()) {
                    continue;
                }
                PRINT_EXTENDER;
                printf("%s.%s", test_case.name(), test_info.name());
                PrintFullTestCommentIfPresent(test_info);
                printf("\n");
            }   
        }
        PRINT_SEPARATOR;
    }

    void OnTestIterationEnd(const UnitTest& unit_test, int /*iteration*/) {
        PRINT_EXTENDER;
        printf("%d %s from %d %s ran", 
                unit_test.test_to_run_count(), unit_test.test_to_run_count() == 1 ? "test" : "tests",
                unit_test.test_case_to_run_count(), unit_test.test_case_to_run_count() == 1 ? "test case" : "test cases");
        
        if (GTEST_FLAG(print_time)) {
            printf(" (%s ms total)",
                    internal::StreamableToString(unit_test.elapsed_time()).c_str());
        }
        printf("\n");
        PRINT_EXTENDER;
        printf("%d %s PASSED.\n", unit_test.successful_test_count(), unit_test.successful_test_count() == 1 ? "test" : "tests");

        int num_failures = unit_test.failed_test_count();
        if (!unit_test.Passed()) {
            const int failed_test_count = unit_test.failed_test_count();
            PRINT_EXTENDER;
            printf("%d %s FAILED.\n", failed_test_count, failed_test_count == 1 ? "test" : "tests");
            PrintFailedTests(unit_test);
        }

        int num_disabled = unit_test.reportable_disabled_test_count();
        if (num_disabled && !GTEST_FLAG(also_run_disabled_tests)) {
            if (!num_failures) {
                printf("\n"); // Add a spacer if no FAILURE banner is displayed.
            }
            printf(
                    "  YOU HAVE %d DISABLED %s\n\n",
                    num_disabled,
                    num_disabled == 1 ? "TEST" : "TESTS");
        }
        // Ensure that Google Test output is printed before, e.g., heapchecker output.
        fflush(stdout);
    }

};

int main(int argc, char **argv) {
    InitGoogleTest(&argc, argv);
    UnitTest& unit_test = *UnitTest::GetInstance();
    TestEventListeners& listeners = unit_test.listeners();

    // Removes the default console output listener from the list so it will
    // not receive events from Google Test and won't print any output. Since
    // this operation transfers ownership of the listener to the caller we
    // have to delete it as well.
    delete listeners.Release(listeners.default_result_printer());

    // Adds the custom output listener to the list. It will now receive
    // events from Google Test and print the alternative output. We don't
    // have to worry about deleting it since Google Test assumes ownership
    // over it after adding it to the list.
    listeners.Append(new CustomTestPrinter);
    int ret_val = RUN_ALL_TESTS();
    return ret_val;
}