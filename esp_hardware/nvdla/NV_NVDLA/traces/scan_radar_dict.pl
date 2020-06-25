#!/bin/perl

$entry_no = -1;
$in_line_no = 0;

@dict = ();
@entry_min = ();
@entry_max = ();

@entry_amin = ();
@entry_amax = ();

    
while(<>) {
    chop;
    @f = split;
    #print "$_\n";
    if ($in_line_no == 0) {
	$num_entries = $f[0];
	#print "Set num_entries to $num_entries\n";
    } else {
	if ($#f == 1) {
	    $entry_no++;
	    $ent_line = 0;
	    $entry_min[$entry_no] = 9.99e+20;
	    $entry_amin[$entry_no] = 9.99e+20;
	    $entry_max[$entry_no] = -9.99e+20;
	    $entry_amax[$entry_no] = -9.99e+20;
	}
	$dict[(32768*$entry_no) + $ent_line] = $f[0];
	$av = $f[0];
	if ($av < 0) {
	    $av = -$av;
	}


	#print "  Entry[$entry_no][$ent_line] : $f[0] : min $entry_min[$entry_no] max $entry_max[$entry_no] amin $entry_amin[$entry_no] amax $entry_amax[$entry_no]\n";
	if ($#f == 0) {
	    if ($f[0] < $entry_min[$entry_no]) { $entry_min[$entry_no] = $f[0]; }
	    if ($f[0] > $entry_max[$entry_no]) { $entry_max[$entry_no] = $f[0]; }
	    if ($av < $entry_amin[$entry_no]) { $entry_amin[$entry_no] = $av; }
	    if ($av > $entry_amax[$entry_no]) { $entry_amax[$entry_no] = $av; }
	}
    }
    $in_line_no++;
    $ent_line++;
}

print "There are $in_line_no total input lines\n";
print "There are $num_entries entry indices : 0 to $entry_no\n";
for ($i = 0; $i < $num_entries; $i++) {
    print "  Entry $i : min $entry_min[$i] max $entry_max[$i] amin $entry_amin[$i] amax $entry_amax[$i]\n";
}
    
