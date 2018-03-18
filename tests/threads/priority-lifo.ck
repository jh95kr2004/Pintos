# -*- perl -*-

# The expected output looks like this:
#
# (priority-lifo) iteration: 15 15 15 15 15 15 15 15 15 15 15 15 15 15 15 15
# (priority-lifo) iteration: 14 14 14 14 14 14 14 14 14 14 14 14 14 14 14 14
# (priority-lifo) iteration: 13 13 13 13 13 13 13 13 13 13 13 13 13 13 13 13
# (priority-lifo) iteration: 12 12 12 12 12 12 12 12 12 12 12 12 12 12 12 12
# (priority-lifo) iteration: 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11 11
# (priority-lifo) iteration: 10 10 10 10 10 10 10 10 10 10 10 10 10 10 10 10
# (priority-lifo) iteration: 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9
# (priority-lifo) iteration: 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8 8
# (priority-lifo) iteration: 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7 7
# (priority-lifo) iteration: 6 6 6 6 6 6 6 6 6 6 6 6 6 6 6 6
# (priority-lifo) iteration: 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5
# (priority-lifo) iteration: 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4
# (priority-lifo) iteration: 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3
# (priority-lifo) iteration: 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2
# (priority-lifo) iteration: 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
# (priority-lifo) iteration: 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
#

use strict;
use warnings;
use tests::tests;

our ($test);
my (@output) = read_text_file ("$test.output");

common_checks ("run", @output);

my ($thread_cnt) = 16;
my ($iter_cnt) = 16;
my (@order);
my (@t) = (-1) x $thread_cnt;

my (@iterations) = grep (/iteration:/, @output);
fail "No iterations found in output.\n" if !@iterations;

my (@numbering) = $iterations[0] =~ /(\d+)/g;
fail "First iteration does not list exactly $thread_cnt threads.\n"
  if @numbering != $thread_cnt;

for my $i (0...$iter_cnt-1) {
    my (@current) = $iterations[$i] =~ /(\d+)/g;
    my (@sorted_numbering) = sort { $a <=> $b } @current;
    my ($correct) = $iter_cnt-$i-1;
    for my $j (0...$thread_cnt-1) {
        if ($sorted_numbering[$j] != $correct) {
	    fail "All elements of iteration $i should be $correct.\n"
	}
    }
}

fail "$iter_cnt iterations expected but " . scalar (@iterations)  . " found\n"
  if $iter_cnt != @iterations;

pass;
