#!/usr/bin/perl
use JSON;
 
# Copyright (c) 2018 Zpalmtree (Turtle coin) and Zawy
# MIT license
#
# Perl script to simulate a hash attack and timestamp manipulation on single or dual-node Cryptonote testnet.
# Change settings in this file and execute with the following after miner and node are running:
# >perl hash_attack.pl
# CTRL-C to stop and stop attack simulated attacker. CTRL-C does not stop this script. 
# "Stopping attack" means preventing your 1-thread dedicated miner from being able to mine all the time. 
# "Starting attack" means allowing continuous mining. 
# "pkill hash_attack.pl" to stop this script.
#
# Timestamp manipulation instructions (optional). 
# What you need:
# You need 1 or more other nodes and only 1 other 1-thread miner on 1 of them to see effect
# of > 50% timestamp attacker, provided attacker here has 4 threads. 
# The other node(s) will have the valid chain because their clock is correct. 
# Get difficulties and timestamps from those nodes for analysis.
# Bad timestamp testing procedure:
# 0) Select the linux system of this node in the settings below and save script. 
# 1) Turn off automatic time updates on this node. 
# 2) Select 1 of the 4 recommended timestamp adjustments below, save.
# 3) Run this script in attack mode for > 10 blocks. 
# 4) "pkill hash_attack.pl" to stop this script.
# 5) Turn on automatic time update to fix this node's system clock.
# 6) Repeat 1) to 5) above for each of the 4 recommended bad timestamp settings. 
# 7) Analyze difficulties and timestamps from the other nodes to see the results.
 
############## BEGIN SETTINGS #############
$port = '31898';
$address='TRTLv1eJUEeV3uHbMpjbDefuhTdphfnDTij2skBJJMDi5EPcmEWpGTsJvRLpUA3F1T5wrjCjGS3GYCHyQiRxXaGR8FW8cjjv8Wi';

$T = 30; # coin's target solvetime
$AS = 10; # attack size as multiple of dedicated miner's hashrate

# This will help by allowing dedicated miner to run longer fraction of block time
# The settings must be 4 and 1 
$attacker_threads = 4;
$dedicated_threads = 1;

# optional timestamp manipulation settings
$use_timestamp_manipulation = 0; # if 1 then attacker will send sequence of bad timestamps. 
$bad_timestamp_size = 0.5*$T;  # Strongly recommended settings to try: 0.5*$T, -0.5*$T, 10*$T, and -10*$T;
# non-ubuntu systems need root privilidges.
$linux_system = 0; # 0 = not linux, 1 = ubuntu, 2 = other GNU, 3  = BSD 

############ END SETTINGS ############# 
 
$dedicated_miner_off_time = int((1-$attacker_threads/$dedicated_threads/$AS)*$T);
print "Dedicated miner on-time is " . $T - $dedicated_miner_off_time . " seconds per block\n";

start_miner($dedicated_threads);
$last_height = get_height();
 
$SIG{INT}  = sub { $interrupted = 1; };
 
while ( 1 ) {
    sleep 1;
    if ($interrupted) {
        $interrupted = 0;
        print "Starting attack.\n";
        &start_miner($attacker_threads);
   
        while(!$interrupted) {
            sleep 1;  
            $height = get_height();
            if ($height != $last_height) {
                $last_height = $height;
                print "Attacker found $height.\n";
                if ( $use_timestamp_manipulation == 1 ) {
                    $secs=time; 
                    $secs+=$bad_timestamp_size; 
                    if    ( $linux_system == 1 ) { $temp =`sudo date -s '\@$secs'`;  }
                    elsif ( $linux_system == 2 ) { $temp =`date -s '\@$secs'`;  } 
                    elsif ( $linux_system == 3 ) { 
                       $temp =`date "$(date -r $secs +'%y%m%d%H%M.%S')"`; # untested
                    }
                    print "$secs\n";
                }
            }
        }
        $interrupted = 0;
        print "Attack Stopped.\n";
    }
 
    $height = get_height();
 
    if ($height != $last_height) {
        $last_height = $height;
        print "Dedicated miner found $height.\n";
        `pkill miner`;
        print "Miner sleeping for $dedicated_miner_off_time seconds.\n";
        sleep $dedicated_miner_off_time;
        print "Starting dedicated miner.\n";
        start_miner($dedicated_threads);
    }
}
 
sub get_height {
    my $height=`curl -s -X POST http://127.0.0.1:$port/json_rpc -d '{"params": {},"jsonrpc":"2.0","id":"test","method":"getblockcount"}' -H 'Content-Type: application/json'`;
    $decoded = decode_json($height);
    $height = $decoded->{'result'}{'count'};
    return $height;
}
 
sub start_miner {
    my $threads = $_[0];
    system("./miner --threads $threads --address $address --daemon-rpc-port $port &");
}
