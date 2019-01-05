import tester

GENERAL_ERROR = "[eE]rror:.*"

t = tester.HomeworkTester()

"""Modify this file with your tests.

The test is already filled out with some basic tests.

Basically, your main usage is:

    t.add_test("command to execute 1", "expected output as a regex string")
    t.add_test("command to execute 2", "expected output as a regex string")
    ...
    t.add_test("command to execute 3", "expected output as a regex string")
    t.run()
    t.print_results()
    t.reset()
"""


# Test that history/bang work as expected
t.add_test("history", "0 history")
t.add_test("history -c", "")
t.add_test("    abc   abc   ", GENERAL_ERROR)
t.add_test("def", GENERAL_ERROR)
expected_output = [
    "2     abc   abc   |2 abc   abc   ",
    "3 def",
    "4 history",
]
t.add_test("history", "\n".join(expected_output))
t.add_test("history -c", "")
t.add_test("history", "6 history")
t.add_test("history blahblahblah", GENERAL_ERROR)
expected_output = [
    "6 history",
    "7 history blahblahblah",
    "8 history",
]
t.add_test("history", "\n".join(expected_output))

t.add_test("cd /usr", "")
t.add_test("!c", "")
t.add_test("!!", "")
expected_output.extend(["9 cd /usr", "10 cd /usr", "11 cd /usr", "12 history"])

t.add_test("history", "\n".join(expected_output))
t.run()
t.print_results()
t.reset()

# ############################# Pipes #############################
# t.add_test("/bin/echo hello world | /bin/grep hello", "hello world")
# t.add_test("/bin/echo blah          |/usr/bin/cut -b 3,4", "ah")
# t.add_test("/bin/echo blah|/usr/bin/cut -b 3,4", "ah")
# t.run()
# t.print_results()
# t.reset()

# t.add_test("/bin/echo hello world", "hello world")
# t.add_test("!/b | /usr/bin/wc -w", "2")
# t.run()
# t.print_results()
# t.reset()
