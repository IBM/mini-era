#!/bin/perl

#$norm_val = 1023;
$norm_val = 1;

$entry_no = -1;
$in_line_no = 0;

@entry_min = ();
@entry_max = ();

@entry_amin = ();
@entry_amax = ();

@inlines = ();
@outlines = ();
$oidx = 0;
while(<>) {
    chop;
    $inlines[$in_line_no] = $_;
    @f = split;
    #print "$_\n";
    if ($in_line_no == 0) {
	$num_entries = $f[0];
	#print "Set num_entries to $num_entries\n";
	$olines[$oidx++] = $_;
    } else {
	if ($#f == 1) {
	    #print "NEW ENTRY: oidx = $oidx and in_line_no = $in_line_no\n";
	    # Output next entry, but normalized
	    for ($i = $oidx; $i < $in_line_no; $i++) {
		# DP $olines[$oidx++] = sprintf("%.18e", ($inlines[$i] / $entry_amax[$entry_no]));
		$olines[$oidx++] = sprintf("%e", $norm_val * ($inlines[$i] / $entry_amax[$entry_no]));
		#print "$olines[$i]\n";
	    }
	    # Set up for the next entry
	    $entry_no++;
	    $ent_line = 0;
	    $entry_min[$entry_no] = 9.99e+20;
	    $entry_amin[$entry_no] = 9.99e+20;
	    $entry_max[$entry_no] = -9.99e+20;
	    $entry_amax[$entry_no] = -9.99e+20;
	    $olines[$oidx++] = $_;
	} else {
	    $av = $f[0];
	    if ($av < 0) {
		$av = -$av;
	    }

	    #print "  Entry[$entry_no][$ent_line] : $f[0] : min $entry_min[$entry_no] max $entry_max[$entry_no] amin $entry_amin[$entry_no] amax $entry_amax[$entry_no]\n";
	    if ($f[0] < $entry_min[$entry_no]) { $entry_min[$entry_no] = $f[0]; }
	    if ($f[0] > $entry_max[$entry_no]) { $entry_max[$entry_no] = $f[0]; }
	    if ($av < $entry_amin[$entry_no]) { $entry_amin[$entry_no] = $av; }
	    if ($av > $entry_amax[$entry_no]) { $entry_amax[$entry_no] = $av; }
	}
    }
    $in_line_no++;
    $ent_line++;
}

# Output the last entry (normalized)
for ($i = $oidx; $i < $in_line_no; $i++) {
    $olines[$oidx++] = ($in_lines[$i] / $entry_amax[$entry_no]);
}

if ($in_line_no != $oidx) {
    print STDERR "WHOOPS : $in_line_no = in_line_no != oidx = $oidx\n";
}
    
for ($i = 0; $i < $oidx; $i++) {
    print "$olines[$i]\n";
}
