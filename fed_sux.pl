#!usr/bin/perl

# Copyright (c) 2023 by Zawy under MIT license.

# ####  The Fed Sux  ####
# A decentralized proof of truth.

# If a lot of decentralized non-profit societal value is spent 
# in the creation of a message, it's evidence the message is true. 

# This hashes a message with a changing nonce until a max number of hex 
# symbols in the hash REPEAT. It makes the hash look like it has more 
# order ("truth") than the "leading zeros" in normal proof of work. 

# If everyone wants to work on the same message, nonce ranges need
# be assigned to workers.

# This has a difficulty algorithm for demonstration and monitoring. 
# There's no blockchain, only a single message for which I want 
# to find the nonce that has the "most ordered" hash.

# ### Usage: ###
# Install Perl's Crypto::Digest module
# $ cpan install Crypt:Digest:SHA256
# Run script:
# $ perl sha256_fed_sux.pl
# The best hashes and nonces are appeneded to "fed_sux.txt".

# The following is a 1-liner to do it without the full program. It only 
# searches and displays best winners. It doesn't have the difficulty 
# algorithm that allows a steady slow stream of non-winners. It writes to the 
# same fed_sux.txt file. Don't forget to change the nonce if you're running 
# several threads that need to be working on different nonces.

# perl -e 'use Crypt::Digest::SHA256 sha256_hex;$|=1;$nonce=1;$w=14;while(1){$nonce++;$m="The Fed Sux \$8.$nonce trillion'\''s worth.";$k=$j=sha256_hex($m);next if $k!~/^.?(.)\1+.?(.)\2+/;$k=~s/(.)\1+/\1/g;$s=64-length($k);next if $s<$w;$w=$s;print "$j $s $m\n\07";open(F,">>fed_sux.txt");print F "$j $s $m\n";close F;}'

use Crypt::Digest::SHA256 sha256_hex;

$|=1; # immediately flush output buffer

########   MESSAGE TO HASH  ########
$message_before_nonce = 'The Fed Sux $8.';
$message_after_nonce = " trillion's worth.";

#######  The STARTING NONCE  #########
# Find the highest-valued nonce attempted to not duplicate prior work
$nonce=2e12; 

#######  BLOCK TIME ########
# increase this to slow the steady stream of non-winners
$T = 5; # in seconds

$D = 15; # Initial DIFFICULTY. Scale depends on metric. Default: $D=15 takes 1 second, 20 takes a day. 

$begin= time(); # Seconds since 1970
print "Message being hashed: '$message_before_nonce(nonce)$message_after_nonce'\n";
print "Time = $begin, Difficulty = $D, Block time = $T seconds";
print '
Filter used:
$hash=~s/(.)\1+/\1/g; 

Scoring function used (difficulty): 
score = 64-length(hash) where hash has repeat chars 
removed with regex hash=~s/(.)\1+/\1/g; 

'; 
print "height clock hash score next_difficulty best_solution time nonce (in Millions) avg_block_time {mid-block difficulty adjust}\n";
$nonce=0; # increase if necessary to prevent multiple threads on same message from doing identical work.
while(1) {
	$nonce++;
	$message = $message_before_nonce . $nonce . $message_after_nonce;  #### THE MESSAGE ####
	
	$hash=$hash2=sha256_hex($message);  ##### HASH THE MESSAGE ####
	 
	# print "$now\n" if ($i % 1e7 == 0); # for testing. Print the time every 10 M loops.
	
	##### FOR SPEED, FILTER OUT HASHES THAT WOULD PROBABALY LOSE #########
	next if $hash!~/^.?(.)\1+.?(.)\2+/; # "xAAxBBxxxxx" is min requirement. A, B, & x are "any hex".
	
	# next; # for testing the above filter's speed.
	
	$now = time()-$begin; # time since genesis
	
	####     REGEX TO REMOVE SEQUENTIAL REPEATS.    #######
	#### THE MORE REMOVED, THE BETTER THE SCORE ##########
	$hash=~s/(.)\1+/\1/g; # Remove all sequential repeats. Gives a twin a score of 1 & a triple 2.
	
	$score=64-length($hash); # This is the measure of success, aka difficulty, the number removed.
	if($now-$t>3*$T){ $D-=0.05; $t=$now; print "(-0.1)"; } # RTT: make difficulty easier if winners are slow
	next if $score<$D; # try again if it's not a winner.
	$height++;
	$D*=(1+1/100-($now-$timestamp)/$T/100); # difficulty algorithm WTEMA =~ ASERT to maintain blocktime
	# if ($now-$timestamp)>$T) { $D+=0.04; } else {$D-=0.1;} # alternate difficulty algo
	$timestamp = $t = $now;
	print "\n$height $now $hash2 $score " . sprintf("%.2f",$D) . " $best $timestamp $nonce (" . int($nonce/1e6) . " M) ";
	print sprintf("%.1f",$now/$height) . " s"; # average block time
	if ($score>=$best) { 
		$new_record="$height $hash2 $score '$message'\n";
		open(F,">>fed_sux.txt"); print F $new_record; close F;  # file also 		
		if ($score>$best) { print "\07\nBEST SCORE: $score '$message'"; } # 07 causes a beep on computer
		$best=$score; 
	}
}

# Another interesting 1-liner that does a difficulty adjustment. It doesn't remember best winners.
#  perl -e 'use Crypt::Digest::SHA256 sha256_hex;$t=time();$d=55;for ($i=1;$i<1e8;$i++){$k=$j=sha256_hex($i); $k=~s/(.)\1+/\1/g; if(time()-$t>3){ $d+=1;$t=time();} if (length($k) < $d){print"$j\n"; time()-$t>1?$d+=1:$d-=1; $t=time();}}'








